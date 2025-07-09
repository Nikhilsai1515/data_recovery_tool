@echo off
setlocal enabledelayedexpansion

:: Loop through all DLL files in the current directory
for %%f in (*.dll) do (
    set "dllname=%%~nf"
    echo Processing: !dllname!.dll

    :: Check if the DLL file exists
    if exist "!dllname!.dll" (
        :: Generate exports.txt using dumpbin
        dumpbin /exports "!dllname!.dll" > exports.txt

        :: Create a DEF file
        echo LIBRARY !dllname! > "!dllname!.def"
        echo EXPORTS >> "!dllname!.def"

        :: Extract exported function names and append to the DEF file
        for /f "skip=19 tokens=4" %%A in ('findstr /R "^ *[0-9][0-9]* *[0-9A-F][0-9A-F]* *[0-9A-F][0-9A-F]* *.*" exports.txt') do (
            echo %%A >> "!dllname!.def"
        )

        :: Generate the LIB file
        lib /def:"!dllname!.def" /out:"!dllname!.lib" /machine:x64

        :: Clean up intermediate files
        del exports.txt
        del "!dllname!.def"

        echo.
        echo Import library "!dllname!.lib" has been created successfully.
        echo.
    ) else (
        echo Error: File "!dllname!.dll" not found.
    )
)

pause
