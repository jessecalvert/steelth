@REM @Author: Jesse
@REM @Date:   2016-08-26 15:46:32
@REM @Last Modified by:   Jesse
@REM Modified time: 2017-07-16 22:50:29

@echo off

set CommonCompilerFlags=-Od -diagnostics:column -MTd -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505 -FC -Z7 /F 0x10000000
set CommonCompilerFlags=-DGAME_DEBUG=1 -DGAME_WIN32=1 %CommonCompilerFlags%
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib ws2_32.lib

IF NOT EXIST W:\steelth01\build mkdir W:\steelth01\build
pushd W:\steelth01\build
W:\ctime\ctime -begin steelth.ctm
del *.pdb > NUL 2> NUL

rem cl %CommonCompilerFlags% -wd4456 W:\steelth01\code\asset_builder.cpp /link %CommonLinkerFlags%
rem cl %CommonCompilerFlags% -wd4456 W:\steelth01\code\simple_meta.cpp /link %CommonLinkerFlags% -PDB:simple_meta.pdd
rem cl %CommonCompilerFlags% -wd4456 W:\steelth01\code\win32_steelth_server.cpp /link %CommonLinkerFlags%
rem cl %CommonCompilerFlags% -wd4456 W:\steelth01\code\render_test.cpp /link %CommonLinkerFlags%
rem cl %CommonCompilerFlags% -wd4456 W:\steelth01\code\slaedit.cpp /link %CommonLinkerFlags%

rem w:\steelth01\build\simple_meta.exe w:\steelth01\code 1 steelth.cpp generated.h

rem rem 64-bit build
rem rem Optimization switches /O2
echo WAITING FOR PDB > lock.tmp

cl %CommonCompilerFlags% W:\steelth01\code\steelth.cpp -Fmsteelth.map -LD /link -incremental:no -opt:ref -PDB:steelth01_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:DebugFrameEnd
cl %CommonCompilerFlags% W:\steelth01\code\win32_steelth_opengl.cpp -Fmwin32_steelth_opengl.map -LD /link %CommonLinkerFlags% -PDB:steelth01_renderer%random%.pdb -EXPORT:Win32LoadOpenGLRenderer

del lock.tmp

cl %CommonCompilerFlags% W:\steelth01\code\win32_steelth.cpp -Fmwin32_steelth.map /link %CommonLinkerFlags%

W:\ctime\ctime -end steelth.ctm
popd
