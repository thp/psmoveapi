@echo off

setlocal enabledelayedexpansion
IF "%1"=="" GOTO InvalidArgs
IF "%2"=="" GOTO InvalidArgs

set MSVC_VERSION=%1
set MSVC_PLATFORM=%2

IF "%MSVC_PLATFORM%" == "x64" (
    set "MSVC_CMAKE_EXTRA= Win64"
) ELSE IF "%MSVC_PLATFORM%" == "x86" (
    set MSVC_CMAKE_EXTRA=
) ELSE (
    GOTO InvalidArgs
)

if "%MSVC_VERSION%" == "2017" (
    set "MSVC_CMAKE_GENERATOR=Visual Studio 15 2017%MSVC_CMAKE_EXTRA%"
    call "%VS150COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE IF "%MSVC_VERSION%" == "2015" (
    set "MSVC_CMAKE_GENERATOR=Visual Studio 14%MSVC_CMAKE_EXTRA%"
    call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE IF "%MSVC_VERSION%" == "2013" (
    set "MSVC_CMAKE_GENERATOR=Visual Studio 12%MSVC_CMAKE_EXTRA%"
    call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE (
    GOTO InvalidArgs
)

set PSMOVE_API_ROOT_DIR=%~dp0..\..\
set PSMOVE_API_EXTERNAL_DIR=%PSMOVE_API_ROOT_DIR%\external
set LIBUSB_DIR=%PSMOVE_API_EXTERNAL_DIR%\libusb-1.0
set OPENCV_DIR=%PSMOVE_API_EXTERNAL_DIR%\opencv

REM Apply libusb patch so that it links against the dynamic (instead of static) CRT
echo Applying libusb dynamic CRT patch...
cd %LIBUSB_DIR%
git apply --ignore-space-change --ignore-whitespace %PSMOVE_API_ROOT_DIR%\scripts\visualc\libusb_dynamic_crt.patch
IF %ERRORLEVEL% NEQ 0 ( echo Failed to apply libusb patch. Perhaps it was already applied or libub was not checked out. )

REM Build libusb
echo.
echo Building libusb
msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_%MSVC_VERSION%.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to build libusb
	goto Error
)
msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_%MSVC_VERSION%.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to build libusb
	goto Error
)

REM Clone OpenCV
IF NOT EXIST %OPENCV_DIR% (
	cd %PSMOVE_API_EXTERNAL_DIR%
	git clone --depth 1 --branch 3.4 git://github.com/opencv/opencv.git
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

cmake .. -G "%MSVC_CMAKE_GENERATOR%" -DBUILD_SHARED_LIBS=0 -DBUILD_WITH_STATIC_CRT=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF -DBUILD_opencv_apps=OFF -DBUILD_opencv_flann=ON -DBUILD_opencv_features2d=ON -DBUILD_opencv_objdetect=OFF -DBUILD_opencv_photo=OFF -DBUILD_opencv_ts=OFF -DBUILD_opencv_ml=ON -DBUILD_opencv_video=OFF -DBUILD_opencv_java=OFF -DWITH_OPENEXR=OFF -DWITH_FFMPEG=OFF -DWITH_JASPER=OFF -DWITH_TIFF=OFF
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to generate OpenCV solution
	goto Error
)

REM Build OpenCV
echo.
echo Building OpenCV
msbuild.exe %OPENCV_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV
	goto Error
)
msbuild.exe %OPENCV_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV
	goto Error
)

REM Generate PSMoveAPI solution

echo.
echo Generating PSMoveAPI solution

cd %PSMOVE_API_ROOT_DIR%
IF NOT EXIST build-%MSVC_PLATFORM% mkdir build-%MSVC_PLATFORM%
cd build-%MSVC_PLATFORM%

cmake .. -G "%MSVC_CMAKE_GENERATOR%" -DPSMOVE_USE_PS3EYE_DRIVER=1 -DPSMOVE_BUILD_OPENGL_EXAMPLES=1 -DOpenCV_DIR=./external/opencv/build-%MSVC_PLATFORM%/
IF !ERRORLEVEL! NEQ 0 (
	echo Failed to generate PSMoveAPI solution
	goto Error
)

REM Build PSMoveAPI
echo.
echo Building PSMoveAPI
msbuild.exe %PSMOVE_API_ROOT_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build PSMoveAPI
	goto Error
)
msbuild.exe %PSMOVE_API_ROOT_DIR%/build-%MSVC_PLATFORM%/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=%MSVC_PLATFORM% /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build PSMoveAPI
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
echo        visual-studio-version .... 2013, 2015 or 2017
echo        build-platform ........... x86 (32-bit) or x64 (64-bit)
echo.
echo Example for VS2017 32-bit build: %0 2017 x86
exit /B 1

:Done
cd %PSMOVE_API_ROOT_DIR%
echo.
echo Build completed successfully
exit /B 0
