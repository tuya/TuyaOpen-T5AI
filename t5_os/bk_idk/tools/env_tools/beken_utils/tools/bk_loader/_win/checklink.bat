@echo off

set port=35
set baudrate=2000000
set para=readinfo -p %port% -b %baudrate% --check-link --reboot 0
bk_loader.exe %para%

::echo %errorlevel%

pause
