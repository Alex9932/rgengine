@echo off

set "RGTOOL="..\..\..\x64\Release\rgtools.exe""

glslang.exe -V .\src\skinning.comp -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\skinning.cs -i .\dx11\tmp.spv

glslang.exe -V .\src\output.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\output.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\output.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\output.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\combine.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\combine.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\combine.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\combine.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\blur.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\blur.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\blur.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\blur.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\ssgi.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\ssgi.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\ssgi.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\ssgi.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\ssr.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\ssr.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\ssr.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\ssr.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\gbuffer.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\gbuffer.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\gbuffer.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\gbuffer.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\accum.vert -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\accum.vs -i .\dx11\tmp.spv

glslang.exe -V .\src\accum_point.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\accum_point.ps -i .\dx11\tmp.spv

glslang.exe -V .\src\accum_global.frag -o .\dx11\tmp.spv
%RGTOOL% -sl -o .\dx11\accum_global.ps -i .\dx11\tmp.spv

pause