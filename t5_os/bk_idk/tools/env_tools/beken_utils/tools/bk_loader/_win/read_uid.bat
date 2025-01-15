@echo off


set port=35
set baudrate=2000000
set para=readinfo -p %port% -b %baudrate% --read-uid --reboot 0 --fast-link 1
bk_loader.exe %para%

::echo %errorlevel%

pause
