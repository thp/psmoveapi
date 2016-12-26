@echo off

setlocal enabledelayedexpansion
IF "%1"=="" GOTO InvalidArgs

set MSVC_VERSION=%1

IF "%MSVC_VERSION%" NEQ "2015" (
	IF "%MSVC_VERSION%" NEQ "2013" (
		GOTO InvalidArgs
	)
)

set PSMOVE_API_ROOT_DIR=%~dp0..\..\
set PSMOVE_API_EXTERNAL_DIR=%PSMOVE_API_ROOT_DIR%\external
set LIBUSB_DIR=%PSMOVE_API_EXTERNAL_DIR%\libusb-1.0
set OPENCV_DIR=%PSMOVE_API_EXTERNAL_DIR%\opencv

IF "%MSVC_VERSION%"=="2015" (
	call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE (
	IF "%MSVC_VERSION%" == "2013" (
		call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat"
	)
)

REM Apply libusb patch so that it links against the dynamic (instead of static) CRT
echo Applying libusb dynamic CRT patch...
cd %LIBUSB_DIR%
git apply --ignore-space-change --ignore-whitespace %PSMOVE_API_ROOT_DIR%\scripts\visualc\libusb_dynamic_crt.patch
IF %ERRORLEVEL% NEQ 0 ( echo Failed to apply libusb patch. Perhaps it was already applied or libub was not checked out. )

REM Build libusb
echo.
echo Building libusb
IF "%MSVC_VERSION%"=="2015" (
	msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_2015.vcxproj /p:Configuration=Debug /property:Platform=x64 /verbosity:minimal /maxcpucount
	IF !ERRORLEVEL! NEQ 0 ( 
		echo Failed to build libusb
		goto Error
	)
	msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_2015.vcxproj /p:Configuration=Release /property:Platform=x64 /verbosity:minimal /maxcpucount
	IF !ERRORLEVEL! NEQ 0 ( 
		echo Failed to build libusb
		goto Error
	)
) ELSE (
	IF "%MSVC_VERSION%" == "2013" (
		msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_2013.vcxproj /p:Configuration=Debug /property:Platform=x64 /verbosity:minimal /maxcpucount
		IF !ERRORLEVEL! NEQ 0 (
			echo Failed to build libusb
			goto Error
		)
		msbuild.exe %LIBUSB_DIR%/msvc/libusb_static_2013.vcxproj /p:Configuration=Release /property:Platform=x64 /verbosity:minimal /maxcpucount
		IF !ERRORLEVEL! NEQ 0 (
			echo Failed to build libusb
			goto Error
		)
	)
)

REM Clone OpenCV
IF NOT EXIST %OPENCV_DIR% (
	cd %PSMOVE_API_EXTERNAL_DIR%
	git clone --depth 1 --branch 3.1.0 git://github.com/Itseez/opencv.git
) ELSE (
	echo.
	echo OpenCV dir already exists; assuming it has been cloned already
)

REM Generate OpenCV solution

echo.
echo Generating OpenCV solution

cd %OPENCV_DIR%
IF NOT EXIST build mkdir build
cd build

IF "%MSVC_VERSION%"=="2015" (
	cmake .. -G "Visual Studio 14 Win64" -DBUILD_SHARED_LIBS=0 -DBUILD_WITH_STATIC_CRT=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF -DBUILD_opencv_apps=OFF -DBUILD_opencv_flann=ON -DBUILD_opencv_features2d=ON -DBUILD_opencv_objdetect=OFF -DBUILD_opencv_photo=OFF -DBUILD_opencv_ts=OFF -DBUILD_opencv_ml=ON -DBUILD_opencv_video=OFF -DBUILD_opencv_java=OFF -DWITH_OPENEXR=OFF -DWITH_FFMPEG=OFF -DWITH_JASPER=OFF -DWITH_TIFF=OFF
	IF !ERRORLEVEL! NEQ 0 (
		echo Failed to generate OpenCV solution
		goto Error
	)	
) ELSE (
	IF "%MSVC_VERSION%" == "2013" (
		cmake .. -G "Visual Studio 12 Win64" -DBUILD_SHARED_LIBS=0 -DBUILD_WITH_STATIC_CRT=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF -DBUILD_opencv_apps=OFF -DBUILD_opencv_flann=ON -DBUILD_opencv_features2d=ON -DBUILD_opencv_objdetect=OFF -DBUILD_opencv_photo=OFF -DBUILD_opencv_ts=OFF -DBUILD_opencv_ml=ON -DBUILD_opencv_video=OFF -DBUILD_opencv_java=OFF -DWITH_OPENEXR=OFF -DWITH_FFMPEG=OFF -DWITH_JASPER=OFF -DWITH_TIFF=OFF
		IF !ERRORLEVEL! NEQ 0 ( 
			echo Failed to generate OpenCV solution
			goto Error
		)	
	)
)

REM Build OpenCV
echo.
echo Building OpenCV
msbuild.exe %OPENCV_DIR%/build/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=x64 /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV
	goto Error
)
msbuild.exe %OPENCV_DIR%/build/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=x64 /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build OpenCV
	goto Error
)

REM Generate PSMoveAPI solution

echo.
echo Generating PSMoveAPI solution

cd %PSMOVE_API_ROOT_DIR%
IF NOT EXIST build mkdir build
cd build

IF "%MSVC_VERSION%"=="2015" (
	cmake .. -G "Visual Studio 14 Win64" -DPSMOVE_USE_PS3EYE_DRIVER=1 -DPSMOVE_BUILD_OPENGL_EXAMPLES=1 -DOpenCV_DIR=./external/opencv/build/
	IF !ERRORLEVEL! NEQ 0 (
		echo Failed to generate PSMoveAPI solution
		goto Error
	)	
) ELSE (
	IF "%MSVC_VERSION%" == "2013" (
		cmake .. -G "Visual Studio 12 Win64" -DPSMOVE_USE_PS3EYE_DRIVER=1 -DPSMOVE_BUILD_OPENGL_EXAMPLES=1 -DOpenCV_DIR=./external/opencv/build/
		IF !ERRORLEVEL! NEQ 0 ( 
			echo Failed to generate PSMoveAPI solution
			goto Error
		)	
	)
)

REM Build PSMoveAPI
echo.
echo Building PSMoveAPI
msbuild.exe %PSMOVE_API_ROOT_DIR%/build/ALL_BUILD.vcxproj /p:Configuration=Debug /property:Platform=x64 /verbosity:minimal /maxcpucount
IF %ERRORLEVEL% NEQ 0 (
	echo Failed to build PSMoveAPI
	goto Error
)
msbuild.exe %PSMOVE_API_ROOT_DIR%/build/ALL_BUILD.vcxproj /p:Configuration=Release /property:Platform=x64 /verbosity:minimal /maxcpucount
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
echo Invalid arguments specified. Specify either 2013 or 2015 as argument.
exit /B 1

:Done
cd %PSMOVE_API_ROOT_DIR%
echo.
echo Build completed successfully
exit /B 0
