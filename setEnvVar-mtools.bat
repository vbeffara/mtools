@echo off

>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
	echo You must run the script with admin rights...
	pause > nul
    exit /B    
) 

CD /D "%~dp0"

echo This script must be run from the base directory of the mtools library.
echo Setting the global environment variables MTOOLS_LIB....
echo on

setx /m MTOOLS_LIB "%CD%\\" 

echo off
echo done.

pause > nul
