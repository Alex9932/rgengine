@echo off
spirv-cross.exe .\vk\skinning.cs --output .\vk\src\skinning.cs.glsl
spirv-cross.exe .\vk\output.vs --output .\vk\src\output.vs.glsl
spirv-cross.exe .\vk\output.ps --output .\vk\src\output.ps.glsl
spirv-cross.exe .\vk\combine.vs --output .\vk\src\combine.vs.glsl
spirv-cross.exe .\vk\combine.ps --output .\vk\src\combine.ps.glsl
spirv-cross.exe .\vk\blur.vs --output .\vk\src\blur.vs.glsl
spirv-cross.exe .\vk\blur.ps --output .\vk\src\blur.ps.glsl
spirv-cross.exe .\vk\ssgi.vs --output .\vk\src\ssgi.vs.glsl
spirv-cross.exe .\vk\ssgi.ps --output .\vk\src\ssgi.ps.glsl
spirv-cross.exe .\vk\ssr.vs --output .\vk\src\ssr.vs.glsl
spirv-cross.exe .\vk\ssr.ps --output .\vk\src\ssr.ps.glsl
spirv-cross.exe .\vk\gbuffer.vs --output .\vk\src\gbuffer.vs.glsl
spirv-cross.exe .\vk\gbuffer.ps --output .\vk\src\gbuffer.ps.glsl
spirv-cross.exe .\vk\accum.vs --output .\vk\src\accum.vs.glsl
spirv-cross.exe .\vk\accum_point.ps --output .\vk\src\accum_point.ps.glsl
spirv-cross.exe .\vk\accum_global.ps --output .\vk\src\accum_global.ps.glsl
pause