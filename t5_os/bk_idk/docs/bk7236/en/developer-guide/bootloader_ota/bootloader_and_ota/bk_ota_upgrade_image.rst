七.The Bootloader is used to upgrade firmware
-------------------------------------------------
- 1.There are two methods for creating firmware upgrades: Method 1: Manually generate firmware upgrades using tools. Method 2: Automatic generation by compiling and packaging; (Preferred method 2)
- 2.Upgrade firmware path as follows: in the build/app/bk7236/encrypt/app_pack RBL.
- 3.Open Everything-1.4.1.935.x64-Setup.exe, Tools -> Options ->The HTTP server,  then binding the local ip address. Use a browser to open the local ip address, locate the local upgrade file \*.rbl,and copy the url to use in the cli command.

 .. figure:: ../../../../_static/bootlaoder_everthing.png
    :align: center
    :alt: bootlaoder_everthing
    :figclass: align-center

    Figure 6 Everything tool page

- 4. Use the serial port to send the cli command, for example:
  http_ota http://192.168.21.101/D%3A/E/build/app.rbl
