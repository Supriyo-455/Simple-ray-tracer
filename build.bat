@echo off

set CommonCompilerFlags=-nologo -O2 -W4 -FC -Zi -WX -wd4706 -wd4701 -wd4273 -wd4201 -wd4100 -wd4305 -wd4996 -wd4309 -wd4244 -wd4091
set CommonLinkerFlags=OpenGL32.lib GLu32.lib Gdi32.lib User32.lib

IF not exist build mkdir build

pushd build

set file=..\code\Ray.cpp
cl %CommonCompilerFlags% %file% /link %CommonLinkerFlags% /out:RayTracer.exe

popd

pushd build
RayTracer.exe
start Test.bmp
popd