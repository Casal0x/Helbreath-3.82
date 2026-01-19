@echo off
REM Build Game with Release-SFML configuration (optimized SFML)
powershell.exe -ExecutionPolicy Bypass -File "%~dp0build_game.ps1" -Config Release-SFML
