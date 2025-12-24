@echo off
glslang.exe -V .\skinning.comp -o .\skinning.cs
glslang.exe -V .\fwd_test.vert -o .\fwd_test.vs
glslang.exe -V .\fwd_test.frag -o .\fwd_test.ps
pause