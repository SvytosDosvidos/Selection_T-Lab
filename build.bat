@echo off
setlocal

cmake -S . -B build
if errorlevel 1 exit /b 1

cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1

ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b 1

echo Build and tests completed successfully.
