@echo off
glslang.exe -V .\src\skinning.comp -o .\vk\skinning.cs
glslang.exe -V .\src\output.vert -o .\vk\output.vs
glslang.exe -V .\src\output.frag -o .\vk\output.ps
glslang.exe -V .\src\combine.vert -o .\vk\combine.vs
glslang.exe -V .\src\combine.frag -o .\vk\combine.ps
glslang.exe -V .\src\blur.vert -o .\vk\blur.vs
glslang.exe -V .\src\blur.frag -o .\vk\blur.ps
glslang.exe -V .\src\ssgi.vert -o .\vk\ssgi.vs
glslang.exe -V .\src\ssgi.frag -o .\vk\ssgi.ps
glslang.exe -V .\src\ssr.vert -o .\vk\ssr.vs
glslang.exe -V .\src\ssr.frag -o .\vk\ssr.ps
glslang.exe -V .\src\gbuffer.vert -o .\vk\gbuffer.vs
glslang.exe -V .\src\gbuffer.frag -o .\vk\gbuffer.ps
glslang.exe -V .\src\accum.vert -o .\vk\accum.vs
glslang.exe -V .\src\accum_point.frag -o .\vk\accum_point.ps
glslang.exe -V .\src\accum_global.frag -o .\vk\accum_global.ps
pause