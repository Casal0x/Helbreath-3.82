@echo off
REM Build Game with Debug-SFML configuration (SFML renderer)
powershell.exe -ExecutionPolicy Bypass -File "%~dp0build_game.ps1" -Config Debug-SFML
