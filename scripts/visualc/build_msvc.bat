@echo off

setlocal enabledelayedexpansion
IF "%1"=="" GOTO InvalidArgs
IF "%2"=="" GOTO InvalidArgs

set MSVC_VERSION=%1
set MSVC_PLATFORM=%2

IF "%MSVC_PLATFORM%" == "x64" (
    set MSVC_CMAKE_ARCH=x64
    set MSVC_VCVARSALL_ARGS=amd64
) ELSE IF "%MSVC_PLATFORM%" == "x86" (
    set MSVC_CMAKE_ARCH=Win32
    set MSVC_VCVARSALL_ARGS=x86
) ELSE GOTO InvalidArgs

if "%VS170COMNTOOLS%" == "" GOTO InvalidArgs

if "%MSVC_VERSION%" == "2022" (
    set "MSVC_CMAKE_GENERATOR=Visual Studio 17 2022"
    call "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %MSVC_VCVARSALL_ARGS%
) ELSE  GOTO InvalidArgs

set PSMOVE_API_ROOT_DIR=%~dp0..\..\
set PSMOVE_API_EXTERNAL_DIR=%PSMOVE_API_ROOT_DIR%\external
set LIBUSB_DIR=%PSMOVE_API_EXTERNAL_DIR%\libusb-1.0
set OPENCV_DIR=%PSMOVE_API_EXTERNAL_DIR%\opencv

REM Build libusb
echo.
echo Building libusb ^(Debug^)
msbuild.exe %LIBUSB_DIR%/msvc/libusb_static.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to build libusb ^(Debug^)
	goto Error
)
echo.
echo Building libusb ^(Release^)
msbuild.exe %LIBUSB_DIR%/msvc/libusb_static.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to build libusb ^(Release^)
	goto Error
)

REM Clone OpenCV
IF NOT EXIST %OPENCV_DIR% (
	cd %PSMOVE_API_EXTERNAL_DIR%
	git clone --depth 1 --branch 4.x https://github.com/opencv/opencv.git
) ELSE (
	echo.
	echo OpenCV dir already exists; assuming it has been cloned already
)

REM Generate OpenCV solution

echo.
echo Generating OpenCV solution

cd %OPENCV_DIR%
IF NOT EXIST build-%MSVC_PLATFORM% mkdir build-%MSVC_PLATFORM%
cd build-%MSVC_PLATFORM%

cmake .. -G "%MSVC_CMAKE_GENERATOR%" -A "%MSVC_CMAKE_ARCH%" -DBUILD_SHARED_LIBS=0 -DBUILD_WITH_STATIC_CRT=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF -DBUILD_opencv_apps=OFF -DBUILD_opencv_flann=ON -DBUILD_opencv_features2d=ON -DBUILD_opencv_objdetect=OFF -DBUILD_opencv_photo=OFF -DBUILD_opencv_ts=OFF -DBUILD_opencv_ml=ON -DBUILD_opencv_video=OFF -DBUILD_opencv_java=OFF -DWITH_OPENEXR=OFF -DWITH_FFMPEG=OFF -DWITH_JASPER=OFF -DWITH_TIFF=OFF
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to generate OpenCV solution
	goto Error
)

REM Build OpenCV
echo.
echo Building OpenCV ^(Debug^)
msbuild.exe %OPENCV_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV ^(Debug^)
	goto Error
)
echo.
echo Building OpenCV ^(Release^)
msbuild.exe %OPENCV_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV ^(Release^)
	goto Error
)

REM Generate PSMoveAPI solution

echo.
echo Generating PSMoveAPI solution

cd %PSMOVE_API_ROOT_DIR%
IF NOT EXIST build-%MSVC_PLATFORM% mkdir build-%MSVC_PLATFORM%
cd build-%MSVC_PLATFORM%

cmake .. -G "%MSVC_CMAKE_GENERATOR%" -A "%MSVC_CMAKE_ARCH%" -DPSMOVE_USE_PS3EYE_DRIVER=1 -DOpenCV_DIR=./external/opencv/build-%MSVC_PLATFORM%/
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to generate PSMoveAPI solution
	goto Error
)

REM Build PSMoveAPI
echo.
echo Building PSMoveAPI ^(Debug^)
msbuild.exe %PSMOVE_API_ROOT_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build PSMoveAPI ^(Debug^)
	goto Error
)
echo.
echo Building PSMoveAPI ^(Release^)
msbuild.exe %PSMOVE_API_ROOT_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build PSMoveAPI ^(Release^)
	goto Error
)

goto Done

:Error
cd %PSMOVE_API_ROOT_DIR%
echo.
echo There was an error running one of the build commands. Please see the output for more information.
exit /B 1

:InvalidArgs
cd %PSMOVE_API_ROOT_DIR%
echo.
echo Usage: %0 visual-studio-version build-platform
echo        visual-studio-version .... 2022
echo        build-platform ........... x86 (32-bit) or x64 (64-bit)
echo.
echo Make sure VS170COMNTOOLS is set (either manually or by starting from the
echo Developer Command Prompt for VS 2022 start menu entry).
echo.
echo Example for VS2022 64-bit build: %0 2022 x64
exit /B 1

:Done
cd %PSMOVE_API_ROOT_DIR%
echo.
echo Build completed successfully
exit /B 0
