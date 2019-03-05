#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad_wgl.h>

static void* get_proc(const char *namez);

#ifdef _WIN32
#include <windows.h>
static HMODULE libGL;

typedef void* (APIENTRYP PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);
PFNWGLGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;

static
int open_gl(void) {
    libGL = LoadLibraryA("opengl32.dll");
    if(libGL != NULL) {
        gladGetProcAddressPtr = (PFNWGLGETPROCADDRESSPROC_PRIVATE)GetProcAddress(
                libGL, "wglGetProcAddress");
        return gladGetProcAddressPtr != NULL;
    }

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        FreeLibrary(libGL);
        libGL = NULL;
    }
}
#else
#include <dlfcn.h>
static void* libGL;

#ifndef __APPLE__
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
PFNGLXGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;
#endif

static
int open_gl(void) {
#ifdef __APPLE__
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#else
    static const char *NAMES[] = {"libGL.so.1", "libGL.so"};
#endif

    unsigned int index = 0;
    for(index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
        libGL = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL);

        if(libGL != NULL) {
#ifdef __APPLE__
            return 1;
#else
            gladGetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL,
                "glXGetProcAddressARB");
            return gladGetProcAddressPtr != NULL;
#endif
        }
    }

    return 0;
}

static
void close_gl() {
    if(libGL != NULL) {
        dlclose(libGL);
        libGL = NULL;
    }
}
#endif

static
void* get_proc(const char *namez) {
    void* result = NULL;
    if(libGL == NULL) return NULL;

#ifndef __APPLE__
    if(gladGetProcAddressPtr != NULL) {
        result = gladGetProcAddressPtr(namez);
    }
#endif
    if(result == NULL) {
#ifdef _WIN32
        result = (void*)GetProcAddress(libGL, namez);
#else
        result = dlsym(libGL, namez);
#endif
    }

    return result;
}

int gladLoadWGL(HDC hdc) {
    int status = 0;

    if(open_gl()) {
        status = gladLoadWGLLoader((GLADloadproc)get_proc, hdc);
        close_gl();
    }

    return status;
}

static HDC GLADWGLhdc = (HDC)INVALID_HANDLE_VALUE;

static void get_exts(void) {
}

static int has_ext(const char *ext) {
    const char *terminator;
    const char *loc;
    const char *extensions;

    if(wglGetExtensionsStringEXT == NULL && wglGetExtensionsStringARB == NULL)
        return 0;

    if(wglGetExtensionsStringARB == NULL || GLADWGLhdc == INVALID_HANDLE_VALUE)
        extensions = wglGetExtensionsStringEXT();
    else
        extensions = wglGetExtensionsStringARB(GLADWGLhdc);

    if(extensions == NULL || ext == NULL)
        return 0;

    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL)
            break;

        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0'))
        {
            return 1;
        }
        extensions = terminator;
    }

    return 0;
}
int GLAD_WGL_VERSION_1_0;
int GLAD_WGL_EXT_framebuffer_sRGB;
int GLAD_WGL_ARB_framebuffer_sRGB;
int GLAD_WGL_EXT_swap_control;
int GLAD_WGL_ARB_pixel_format;
int GLAD_WGL_ARB_create_context_profile;
int GLAD_WGL_ARB_create_context;
int GLAD_WGL_ARB_extensions_string;
int GLAD_WGL_EXT_extensions_string;
PFNWGLCREATECONTEXTATTRIBSARBPROC glad_wglCreateContextAttribsARB;
PFNWGLGETEXTENSIONSSTRINGARBPROC glad_wglGetExtensionsStringARB;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC glad_wglGetPixelFormatAttribivARB;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC glad_wglGetPixelFormatAttribfvARB;
PFNWGLCHOOSEPIXELFORMATARBPROC glad_wglChoosePixelFormatARB;
PFNWGLGETEXTENSIONSSTRINGEXTPROC glad_wglGetExtensionsStringEXT;
PFNWGLSWAPINTERVALEXTPROC glad_wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC glad_wglGetSwapIntervalEXT;
static void load_WGL_ARB_create_context(GLADloadproc load) {
	if(!GLAD_WGL_ARB_create_context) return;
	glad_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load("wglCreateContextAttribsARB");
}
static void load_WGL_ARB_extensions_string(GLADloadproc load) {
	if(!GLAD_WGL_ARB_extensions_string) return;
	glad_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)load("wglGetExtensionsStringARB");
}
static void load_WGL_ARB_pixel_format(GLADloadproc load) {
	if(!GLAD_WGL_ARB_pixel_format) return;
	glad_wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)load("wglGetPixelFormatAttribivARB");
	glad_wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)load("wglGetPixelFormatAttribfvARB");
	glad_wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load("wglChoosePixelFormatARB");
}
static void load_WGL_EXT_extensions_string(GLADloadproc load) {
	if(!GLAD_WGL_EXT_extensions_string) return;
	glad_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)load("wglGetExtensionsStringEXT");
}
static void load_WGL_EXT_swap_control(GLADloadproc load) {
	if(!GLAD_WGL_EXT_swap_control) return;
	glad_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)load("wglSwapIntervalEXT");
	glad_wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)load("wglGetSwapIntervalEXT");
}
static void find_extensionsWGL(void) {
	get_exts();
	GLAD_WGL_ARB_create_context = has_ext("WGL_ARB_create_context");
	GLAD_WGL_ARB_create_context_profile = has_ext("WGL_ARB_create_context_profile");
	GLAD_WGL_ARB_extensions_string = has_ext("WGL_ARB_extensions_string");
	GLAD_WGL_ARB_framebuffer_sRGB = has_ext("WGL_ARB_framebuffer_sRGB");
	GLAD_WGL_ARB_pixel_format = has_ext("WGL_ARB_pixel_format");
	GLAD_WGL_EXT_extensions_string = has_ext("WGL_EXT_extensions_string");
	GLAD_WGL_EXT_framebuffer_sRGB = has_ext("WGL_EXT_framebuffer_sRGB");
	GLAD_WGL_EXT_swap_control = has_ext("WGL_EXT_swap_control");
}

static void find_coreWGL(HDC hdc) {
	GLADWGLhdc = hdc;
}

int gladLoadWGLLoader(GLADloadproc load, HDC hdc) {
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)load("wglGetExtensionsStringARB");
	wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)load("wglGetExtensionsStringEXT");
	if(wglGetExtensionsStringARB == NULL && wglGetExtensionsStringEXT == NULL) return 0;
	find_coreWGL(hdc);

	find_extensionsWGL();
	load_WGL_ARB_create_context(load);
	load_WGL_ARB_extensions_string(load);
	load_WGL_ARB_pixel_format(load);
	load_WGL_EXT_extensions_string(load);
	load_WGL_EXT_swap_control(load);
	return 1;
}

