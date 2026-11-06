@echo off
setlocal
set "LB_ENGINE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%LB_ENGINE%" (
    echo Unreal Engine 5.8 was not found. Open EX_1.uproject with your installed engine.
    pause
    exit /b 1
)
start "Last Broadcast" "%LB_ENGINE%" "%~dp0EX_1.uproject" /Game/LastBroadcast/Maps/L_LastBroadcast -game -windowed -ResX=1280 -ResY=800 -nosplash -NoScreenMessages
