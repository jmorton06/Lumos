cd "$(dirname "$0")"

/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V shader.vert -o /CompiledSPV/shader.vert.spv
/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V shader.frag -o /CompiledSPV/shader.frag.spv

/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V Skybox.vert -o /CompiledSPV/Skybox.vert.spv
/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V Skybox.frag -o /CompiledSPV/Skybox.frag.spv

/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V DeferredColour.vert -o /CompiledSPV/DeferredColour.vert.spv
/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V DeferredColour.frag -o /CompiledSPV/DeferredColour.frag.spv

/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V DeferredLight.vert -o /CompiledSPV/DeferredLight.vert.spv
/VulkanSDK/1.1.85.0/x86_64/bin/glslangValidator -V DeferredLight.frag -o /CompiledSPV/DeferredLight.frag.spv

