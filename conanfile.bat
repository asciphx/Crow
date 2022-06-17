@echo off
title test conan
echo Here, i only test the connection of all header files, and can't output them correctly��
if exist amalgamate\cc_all.h del amalgamate\cc_all.h;
for /f "tokens=2 skip=1 delims= " %%i in (.\include\cc.h) do (
	type .\include\cc\%%~nxi >> amalgamate\cc_all.h
)
pause