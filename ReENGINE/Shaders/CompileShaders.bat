:: Deferred rendering shader compilation.
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Deferred/Source/gbuffer.vert -o Deferred/gbuffer.vert.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Deferred/Source/gbuffer.frag -o Deferred/gbuffer.frag.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Deferred/Source/lighting.vert -o Deferred/lighting.vert.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Deferred/Source/lighting.frag -o Deferred/lighting.frag.spv

:: Forward rendering shader compilation.
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Forward/Source/default.vert -o Forward/default.vert.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V Forward/Source/default.frag -o Forward/default.frag.spv

pause
