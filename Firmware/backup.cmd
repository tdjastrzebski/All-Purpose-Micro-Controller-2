@echo off

rem Get current folder name
for %%F in ("%CD%") do set FOLDER=%%~nxF

rem Get date
for /f %%i in ('powershell -command "Get-Date -Format yyyy-MM-dd"') do set DATE=%%i

rem Create archive
7z a -tzip -xr!*.zip -xr!*.pdf -xr!.\BUILD -xr!.\.git "%FOLDER%_%DATE%.zip" ".\*"
