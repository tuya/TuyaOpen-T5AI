@echo off


set port=35
set baudrate=1000000
set para=readinfo -p %port% -b %baudrate% -f 0x3cc000-5a0 --reboot 0 --fast-link 1
bk_loader.exe %para%

::echo %errorlevel%

pause
