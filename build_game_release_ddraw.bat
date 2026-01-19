@echo off
REM Build Game with Release-DDraw configuration (optimized DirectDraw)
powershell.exe -ExecutionPolicy Bypass -File "%~dp0build_game.ps1" -Config Release-DDraw
