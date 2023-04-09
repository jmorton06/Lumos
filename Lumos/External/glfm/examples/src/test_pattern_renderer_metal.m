#include "test_pattern_renderer.h"

#import <MetalKit/MetalKit.h>

#if !__has_feature(objc_arc)
#error This example requires ARC
#endif

typedef struct {
    Renderer renderer;
    MTKView *mtkView;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLSamplerState> sampler;
    id<MTLCommandQueue> commandQueue;
    id<MTLCommandBuffer> commandBuffer;
    id<MTLRenderCommandEncoder> renderCommandEncoder;
    
    NSMutableDictionary<NSNumber *, id<MTLTexture>> *textures;
    Texture nextTexture;
} RendererMetal;

#define impl_of(this_renderer) ((RendererMetal *)(void *)((uint8_t *)this_renderer - offsetof(RendererMetal, renderer)))

static Texture textureUpload(Renderer *renderer, uint32_t width, uint32_t height, uint8_t *data) {
    RendererMetal *impl = impl_of(renderer);
    id<MTLDevice> device = impl->mtkView.device;
    Texture textureId = NULL_TEXTURE;
    
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor new];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    if (texture) {
        MTLRegion region = { { 0, 0, 0 }, { width, height, 1 } };
        [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:width * 4];
        textureId = impl->nextTexture++;
        impl->textures[@(textureId)] = texture;
    }
    return textureId;
}

static void textureDestroy(Renderer *renderer, Texture texture) {
    RendererMetal *impl = impl_of(renderer);
    [impl->textures removeObjectForKey:@(texture)];
}

static void drawFrameStart(Renderer *renderer, int screenWidth, int screenHeight) {
    RendererMetal *impl = impl_of(renderer);
    MTKView *view = impl->mtkView;
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor) {
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
        
        id<MTLCommandBuffer> commandBuffer = [impl->commandQueue commandBuffer];
        commandBuffer.label = @"Test Pattern Command Buffer";
        
        id<MTLRenderCommandEncoder> renderCommandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderCommandEncoder.label = @"Test Pattern Render Command Encoder";
        [renderCommandEncoder setViewport:(MTLViewport){ 0.0, 0.0, screenWidth, screenHeight, 0.0, 1.0 }];
        [renderCommandEncoder setRenderPipelineState:impl->pipelineState];
        [renderCommandEncoder setFragmentSamplerState:impl->sampler atIndex:0];

        impl->commandBuffer = commandBuffer;
        impl->renderCommandEncoder = renderCommandEncoder;
    }
}

static void drawFrameEnd(Renderer *renderer) {
    RendererMetal *impl = impl_of(renderer);
    MTKView *view = impl->mtkView;
    
    [impl->renderCommandEncoder endEncoding];
    [impl->commandBuffer presentDrawable:view.currentDrawable];
    [impl->commandBuffer commit];
    impl->commandBuffer = nil;
    impl->renderCommandEncoder = nil;
}

static void drawQuad(Renderer *renderer, Texture textureId, const Vertex (*vertices)[4]) {
    // NOTE: This function draws one quad at a time, which is slow. Don't use in production.
    RendererMetal *impl = impl_of(renderer);
    id<MTLDevice> device = impl->mtkView.device;
    id<MTLTexture> texture = [impl->textures objectForKey:@(textureId)];
    id<MTLBuffer> vertexBuffer = [device newBufferWithBytes:vertices length:sizeof(*vertices) options:MTLResourceCPUCacheModeWriteCombined];
    if (texture && vertexBuffer) {
        [impl->renderCommandEncoder setFragmentTexture:texture atIndex:0];
        [impl->renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [impl->renderCommandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
}

static void destroy(Renderer *renderer) {
    RendererMetal *impl = impl_of(renderer);
    impl->mtkView = nil;
    impl->pipelineState = nil;
    impl->sampler = nil;
    impl->textures = nil;
    impl->commandQueue = nil;
    impl->commandBuffer = nil;
    impl->renderCommandEncoder = nil;
    free(impl);
}

Renderer *createRendererMetal(GLFMDisplay *display) {
    MTKView *mtkView = (__bridge MTKView *)glfmGetMetalView(display);
    id<MTLDevice> device = mtkView.device;
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        return NULL;
    }
    
    // Sampler
    MTLSamplerDescriptor *linearSamplerDescriptor = [MTLSamplerDescriptor new];
    linearSamplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    linearSamplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:linearSamplerDescriptor];
    if (!sampler) {
        return NULL;
    }
    
    // Shader
    NSError *error = nil;
    id<MTLLibrary> library = [device newDefaultLibrary];
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineStateDescriptor.vertexFunction = [library newFunctionWithName:@"textureVertexShader"];
    pipelineStateDescriptor.fragmentFunction = [library newFunctionWithName:@"textureFragmentShader"];
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineStateDescriptor.colorAttachments[0].blendingEnabled = NO;
    id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!pipelineState) {
        return NULL;
    }
    
    // Renderer
    RendererMetal *impl = calloc(1, sizeof(RendererMetal));
    impl->mtkView = mtkView;
    impl->sampler = sampler;
    impl->pipelineState = pipelineState;
    impl->commandQueue = commandQueue;
    impl->textures = [NSMutableDictionary new];
    impl->nextTexture = 1;
    
    Renderer *renderer = &impl->renderer;
    renderer->textureUpload = textureUpload;
    renderer->textureDestroy = textureDestroy;
    renderer->drawFrameStart = drawFrameStart;
    renderer->drawFrameEnd = drawFrameEnd;
    renderer->drawQuad = drawQuad;
    renderer->destroy = destroy;
    return renderer;
}
