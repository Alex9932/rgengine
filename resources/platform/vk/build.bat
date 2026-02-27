@echo off
glslang.exe -V .\skinning.comp -o .\skinning.cs
glslang.exe -V .\output.vert -o .\output.vs
glslang.exe -V .\output.frag -o .\output.ps
glslang.exe -V .\combine.vert -o .\combine.vs
glslang.exe -V .\combine.frag -o .\combine.ps
glslang.exe -V .\blur.vert -o .\blur.vs
glslang.exe -V .\blur.frag -o .\blur.ps
glslang.exe -V .\ssgi.vert -o .\ssgi.vs
glslang.exe -V .\ssgi.frag -o .\ssgi.ps
glslang.exe -V .\ssr.vert -o .\ssr.vs
glslang.exe -V .\ssr.frag -o .\ssr.ps
glslang.exe -V .\gbuffer.vert -o .\gbuffer.vs
glslang.exe -V .\gbuffer.frag -o .\gbuffer.ps
glslang.exe -V .\accum.vert -o .\accum.vs
glslang.exe -V .\accum_point.frag -o .\accum_point.ps
glslang.exe -V .\accum_global.frag -o .\accum_global.ps
pause