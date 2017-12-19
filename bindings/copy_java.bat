if "%~1"=="" (
	echo Destination directory missing
	exit /b 1
)

copy *.java "%~1"

exit /b 0
