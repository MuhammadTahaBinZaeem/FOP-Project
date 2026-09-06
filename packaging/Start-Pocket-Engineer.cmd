@echo off
"%~dp0bin\pocket-engineer-server.exe" --open "%~dp0share\pocket-engineer\www"
if errorlevel 1 pause
