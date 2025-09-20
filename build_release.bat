@echo off

setlocal

setlocal enabledelayedexpansion

set "cc=cl.exe"
set "ar=lib.exe"

set "bin_dir=bin\"
set "src_dir=src\"
set "obj_dir=obj\"

set "inc_dir=.\include"
set "lib_dir=.\lib"

set "warn=/wd4244 /wd5105"
set "cdefines=/D _CRT_SECURE_NO_WARNINGS /D _UNICODE /D UNICODE /D PLATFORM_DESKTOP /D GRAPHICS_API_OPENGL_33"

set "cflags=/nologo /std:c11 /utf-8 /W4 /WX- /diagnostics:column /TC /Zi /fp:fast /I%inc_dir% /validate-charset"

set "flag=/D NDEBUG /MT /Ox /GS- /MP /cgthreads8 /GL"

set "link_param=/link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LIBPATH:%lib_dir% raylib.lib opengl32.lib kernel32.lib shell32.lib user32.lib gdi32.lib winmm.lib"

if exist %bin_dir% ( rmdir /s /q %bin_dir% )
if exist %obj_dir% ( rmdir /s /q %obj_dir% )

mkdir %bin_dir%
mkdir %obj_dir%

%cc% %cflags% %warn% %flag% %cdefines% %src_dir%entry.c /Fo%obj_dir%c_wizard.obj /Fd%bin_dir%c_wizard.pdb /Fe%bin_dir%c_wizard.exe %link_param%

endlocal
