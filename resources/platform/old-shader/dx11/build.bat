@echo off
dxc /T cs_6_0 /E main /Fo skinning.cs skinning.cs.hlsl
dxc /T vs_6_0 /E vmain /Fo fwd_test.vs fwd_test.vs.hlsl
dxc /T ps_6_0 /E pmain /Fo fwd_test.ps fwd_test.ps.hlsl
pause