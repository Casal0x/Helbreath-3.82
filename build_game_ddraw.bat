@echo off
REM Build Game with Debug-DDraw configuration (DirectDraw renderer)
powershell.exe -ExecutionPolicy Bypass -File "%~dp0build_game.ps1" -Config Debug-DDraw
