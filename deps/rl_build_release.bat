@echo off
setlocal
setlocal enabledelayedexpansion

pushd raylib
set "cc=cl.exe"
set "ar=lib.exe"

set "src_dir=src\"
set "obj_dir=..\..\obj\"
set "lib_dir=..\..\lib\"

set "cdefines=/D _CRT_SECURE_NO_WARNINGS /D _UNICODE /D UNICODE /D PLATFORM_DESKTOP /D GRAPHICS_API_OPENGL_33"

set "cflags=/nologo /std:c11 /utf-8 /W4 /WX- /diagnostics:column /TC /Z7 /fp:fast /I%src_dir%external\glfw\include /validate-charset"

set "flag=/D NDEBUG /MT /Ox /GS- /MP /cgthreads8 /GL"

set "link_param=/link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup opengl32.lib kernel32.lib shell32.lib user32.lib gdi32.lib winmm.lib"

if exist %obj_dir% ( rmdir /s /q %obj_dir% )
if exist %lib_dir% ( rmdir /s /q %lib_dir% )
mkdir %obj_dir%
mkdir %lib_dir%

%cc% %cflags% %flag% %cdefines% -c %src_dir%rcore.c /Fo%obj_dir%rcore.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%rglfw.c /Fo%obj_dir%rglfw.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%rshapes.c /Fo%obj_dir%rshapes.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%rtextures.c /Fo%obj_dir%rtextures.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%rtext.c /Fo%obj_dir%rtext.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%rmodels.c /Fo%obj_dir%rmodels.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%raudio.c /Fo%obj_dir%raudio.obj  %link_param%
%cc% %cflags% %flag% %cdefines% -c %src_dir%utils.c /Fo%obj_dir%utils.obj  %link_param%

%ar% /nologo /OUT:%lib_dir%\raylib.lib %obj_dir%rcore.obj %obj_dir%rglfw.obj %obj_dir%rshapes.obj %obj_dir%rtextures.obj %obj_dir%rtext.obj %obj_dir%rmodels.obj %obj_dir%raudio.obj %obj_dir%utils.obj

popd
endlocal
