@echo off

set bin=E:\BK_File\BK7236\all-app.bin
set json=E:\BK_File\BK7236\otp_efuse_config.json
set key=73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18
set port=35
set baudrate=2000000
set para=download -p %port% -b %baudrate% -i %bin% --reboot 0 --fast-link 1
::set para=download -p %port% -b %baudrate% -i %bin% --safe-json %json% --aes-key %key% --reboot 0 --fast-link 1
bk_loader.exe %para%

::echo %errorlevel%

pause
