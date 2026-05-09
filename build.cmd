@echo off
setlocal

REM Parse command line arguments
set CONFIG=Release
set PLATFORM=x64

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="Debug" set CONFIG=Debug
if /i "%~1"=="Release" set CONFIG=Release
if /i "%~1"=="x64" set PLATFORM=x64
if /i "%~1"=="ARM64" set PLATFORM=ARM64
if /i "%~1"=="-h" goto show_help
if /i "%~1"=="--help" goto show_help
shift
goto parse_args

:show_help
echo Usage: build_platform.cmd [CONFIG] [PLATFORM]
echo.
echo CONFIG:   Debug or Release (default: Release)
echo PLATFORM: x64, or ARM64 (default: x64)
echo.
echo Examples:
echo   build_platform.cmd
echo   build_platform.cmd Debug
echo   build_platform.cmd Release x64
echo   build_platform.cmd Debug ARM64
echo   build_platform.cmd Release ARM64
exit /b 0

:end_parse

echo Building configuration: %CONFIG%
echo Target platform: %PLATFORM%
echo.

REM Navigate to the build directory
cd /d "%~dp0"

REM Create build directory (platform-specific) if it doesn't exist
if not exist "build-%PLATFORM%" mkdir "build-%PLATFORM%"
cd "build-%PLATFORM%"

REM Configure CMake project
echo Configuring CMake project...
cmake .. -A %PLATFORM% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
  echo CMake configuration failed!
  exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config %CONFIG%
if errorlevel 1 (
  echo Build failed!
  exit /b 1
)

REM Detect if cross-compiling
set CROSS_COMPILE=0
if /i "%PLATFORM%"=="ARM64" if /i not "%PROCESSOR_ARCHITECTURE%"=="ARM64" set CROSS_COMPILE=1

if "%CROSS_COMPILE%"=="1" (
  echo Cross-compiling detected, skipping tests...
  goto skip_tests
)

REM Run tests
echo Running tests...
ctest -C %CONFIG% --output-on-failure
if errorlevel 1 (
  echo Tests failed!
  exit /b 1
)

:skip_tests
echo Build and tests completed successfully!