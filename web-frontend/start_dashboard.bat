@echo off
cd /d "%~dp0"

echo ==============================================
echo      Smart Agri Dashboard Launcher
echo ==============================================

echo [1/3] Checking Node.js environment...
call npm -v
if %errorlevel% neq 0 (
    echo [ERROR] npm command not found!
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b
)

if not exist node_modules (
    echo [2/3] Installing dependencies because 'node_modules' is missing...
    call npm install
    if %errorlevel% neq 0 (
        echo [ERROR] Dependency installation failed!
        echo Check your internet connection.
        pause
        exit /b
    )
) else (
    echo [2/3] Dependencies found, skipping install.
)

echo [3/3] Starting Vite Server...
echo.
echo ----------------------------------------------
echo If the browser does not open automatically,
echo please manually visit the URLs shown below.
echo ** DO NOT CLOSE THIS WINDOW **
echo ----------------------------------------------
echo.

call npm run dev

echo.
echo [WARING] Server stopped.
pause
