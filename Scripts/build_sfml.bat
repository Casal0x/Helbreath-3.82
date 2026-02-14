@echo off
setlocal enabledelayedexpansion
:: =====================================================================
::  SFML 3 Build Script for Windows
::
::  Downloads, builds, and installs SFML 3 into the project's
::  Sources\Dependencies\SFML\ directory (headers + x64 static libs).
::
::  Prerequisites: cmake, git, Visual Studio (MSBuild)
::
::  Usage:
::    build_sfml.bat              Build and install SFML 3
::    build_sfml.bat clean        Remove build temp directory only
::    build_sfml.bat --version X  Override SFML version (default: 3.0.2)
:: =====================================================================

set SFML_VERSION=3.0.2
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
set SFML_DEST=%PROJECT_DIR%\Sources\Dependencies\SFML
set BUILD_ROOT=%TEMP%\hb_sfml_build

:: Parse arguments
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="clean" (
    if exist "%BUILD_ROOT%" (
        echo Cleaning build directory: %BUILD_ROOT%
        rmdir /s /q "%BUILD_ROOT%"
        echo Done.
    ) else (
        echo Nothing to clean.
    )
    exit /b 0
)
if /i "%~1"=="--version" (
    set SFML_VERSION=%~2
    shift
)
shift
goto :parse_args
:args_done

echo ========================================
echo  SFML %SFML_VERSION% Build Script
echo ========================================
echo.
echo  Source:  github.com/SFML/SFML @ %SFML_VERSION%
echo  Target:  %SFML_DEST%
echo.

:: --- Check prerequisites ---
where cmake >nul 2>&1 || (
    echo ERROR: cmake not found in PATH.
    echo        Install CMake from https://cmake.org/download/
    exit /b 1
)
where git >nul 2>&1 || (
    echo ERROR: git not found in PATH.
    exit /b 1
)

:: --- Clone ---
if exist "%BUILD_ROOT%\src" (
    echo Found previous clone, reusing...
) else (
    echo [1/6] Cloning SFML %SFML_VERSION%...
    git clone --depth 1 --branch %SFML_VERSION% https://github.com/SFML/SFML.git "%BUILD_ROOT%\src"
    if errorlevel 1 (
        echo ERROR: git clone failed.
        exit /b 1
    )
)

:: --- Configure ---
echo.
echo [2/6] Configuring CMake (x64, static, graphics/window/system only)...
cmake -S "%BUILD_ROOT%\src" -B "%BUILD_ROOT%\build" -A x64 ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DSFML_BUILD_AUDIO=OFF ^
    -DSFML_BUILD_NETWORK=OFF
if errorlevel 1 (
    echo ERROR: CMake configure failed.
    exit /b 1
)

:: --- Build Debug ---
echo.
echo [3/6] Building Debug...
cmake --build "%BUILD_ROOT%\build" --config Debug --parallel
if errorlevel 1 (
    echo ERROR: Debug build failed.
    exit /b 1
)

:: --- Build Release ---
echo.
echo [4/6] Building Release...
cmake --build "%BUILD_ROOT%\build" --config Release --parallel
if errorlevel 1 (
    echo ERROR: Release build failed.
    exit /b 1
)

:: --- Install both configs to a staging directory ---
echo.
echo [5/6] Installing to staging directory...
cmake --install "%BUILD_ROOT%\build" --config Debug --prefix "%BUILD_ROOT%\install"
if errorlevel 1 (
    echo ERROR: Debug install failed.
    exit /b 1
)
cmake --install "%BUILD_ROOT%\build" --config Release --prefix "%BUILD_ROOT%\install"
if errorlevel 1 (
    echo ERROR: Release install failed.
    exit /b 1
)

:: --- Copy to project ---
echo.
echo [6/6] Copying to project...

:: Headers
echo  - Headers...
if exist "%SFML_DEST%\include" rmdir /s /q "%SFML_DEST%\include"
xcopy /s /i /q "%BUILD_ROOT%\install\include" "%SFML_DEST%\include" >nul

:: x64 static libs (from install/lib/)
echo  - x64 libraries...
if not exist "%SFML_DEST%\lib_x64" mkdir "%SFML_DEST%\lib_x64"
copy /y "%BUILD_ROOT%\install\lib\*.lib" "%SFML_DEST%\lib_x64\" >nul

:: Freetype may be in a different location depending on SFML's build
:: Check the _deps directory as a fallback
if not exist "%SFML_DEST%\lib_x64\freetype.lib" (
    echo  - Searching for freetype libs in build tree...
    for /r "%BUILD_ROOT%\build" %%f in (freetype.lib) do (
        copy /y "%%f" "%SFML_DEST%\lib_x64\" >nul
        echo    Found: %%f
    )
    for /r "%BUILD_ROOT%\build" %%f in (freetyped.lib) do (
        copy /y "%%f" "%SFML_DEST%\lib_x64\" >nul
        echo    Found: %%f
    )
)

:: --- Report ---
echo.
echo ========================================
echo  SFML %SFML_VERSION% installed successfully
echo ========================================
echo.
echo  Headers:   %SFML_DEST%\include\
echo  Libraries: %SFML_DEST%\lib_x64\
echo.
echo  Installed libs:
for %%f in ("%SFML_DEST%\lib_x64\*.lib") do echo    %%~nxf
echo.
echo  Build temp at %BUILD_ROOT%
echo  Run "build_sfml.bat clean" to remove it.
echo ========================================

endlocal
