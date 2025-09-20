@echo off

if "%1"=="Release"  ( goto release )
if "%1"=="release"  ( goto release )
if "%1"=="rel"      ( goto release )

goto debug

:release
pushd deps
call rl_build_release.bat 
popd
call build_release.bat
goto end

:debug
pushd deps
call rl_build.bat 
popd
call build.bat
goto end

:end
