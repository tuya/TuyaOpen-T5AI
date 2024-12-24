BLE Cli
================

:link_to_translation:`zh_CN:[中文]`

1、Function overview
--------------------------
	In order to verify BLE-related functions, a series of AT CMDs are designed. All BLE AT CMDs start with ``AT+``, followed by specific command and parameters. If the CMD is executed successfully, ``CMDRSP:OK`` is returned. If the CMD execution fails, ``CMDRSP:ERROR`` is returned.

2、Code path
--------------------------
	path: ``\components\bk_bluetooth\api\bk_at_ble.c``

3、Cli command introduction
---------------------------------
The supported CMDs are as follows:

	1.AT+BLEADVPARAM
	 - Function Description
		Used to set the advertising parameters.（including legacy advertising、extended advertising and periodic advertising）
	 - CMD Format
		AT+BLEADVPARAM=param1,param2,param3,param4,param5,param6,param7,param8

		+-----------+------------------------------------------------------------------------+
		|param1     | adv channel map                                                        |
		+-----------+------------------------------------------------------------------------+
		|param2     | Minimum advertising interval                                           |
		|           |  Range: 0x000020 to 0xFFFFFF(Time = N * 0.625 ms)                      |
		+-----------+------------------------------------------------------------------------+
		|param3     | Maximum advertising interval                                           |
		|           |  Range: 0x000020 to 0xFFFFFF(Time = N * 0.625 ms)                      |
		+-----------+------------------------------------------------------------------------+
		|param4     | Own_Address_Type                                                       |
		|           |  | 0x00 : Public Device Address                                        |
		|           |  | 0x01 : Random Device Address                                        |
		|           |  | 0x02 : Resolvable Private Address,if no IRK use the public address  |
		|           |  | 0x03 : Resolvable Private Address,if no IRK use the random address  |
		+-----------+------------------------------------------------------------------------+
		|param5     | adv_type                                                               |
		|           |  | 0x00 : Legacy advertising                                           |
		|           |  | 0x01 : Extended advertising                                         |
		|           |  | 0x02 : Periodic advertising                                         |
		+-----------+------------------------------------------------------------------------+
		|param6     | adv_properties(Bit number)                                             |
		|           |  | 0 : Connectable advertising                                         |
		|           |  | 1 : Scannable advertising                                           |
		|           |  | 2 : Directed advertising                                            |
		|           |  | 3 : High Duty Cycle Directed Connectable advertising                |
		|           |  | 4 : Use legacy advertising PDUs                                     |
		|           |  | 5 : Omit advertiser's address from all PDUs("anonymous advertising")|
		|           |  | 6 : Include TxPower in the extended header of the advertising PDU   |
		+-----------+------------------------------------------------------------------------+
		|param7     | Primary_Advertising_PHY                                                |
		|           |  | 0x01 : Primary advertisement PHY is LE 1M                           |
		|           |  | 0x03 : Primary advertisement PHY is LE Coded                        |
		+-----------+------------------------------------------------------------------------+
		|param8     | Secondary_Advertising_PHY                                              |
		|           |  | 0x01 : Secondary advertisement PHY is LE 1M                         |
		|           |  | 0x02 : Secondary advertisement PHY is LE 2M                         |
		|           |  | 0x03 : Secondary advertisement PHY is LE Coded                      |
		+-----------+------------------------------------------------------------------------+
	 - Test Case
		| Set the legacy advertising parameters：``AT+BLEADVPARAM=7,120,160,0,0,3,1,1``
		| Set the extended advertising parameters：``AT+BLEADVPARAM=7,120,160,0,1,1,1,1``
		| Set the periodic advertising parameters：``AT+BLEADVPARAM=7,120,160,0,2,0,1,3``

	2.AT+BLEADVDATA
	 - Function Description
		Used to set the data used in advertising packets
	 - CMD Format
		AT+BLEADVDATA=param1,param2

		+-----------+------------------------------------------------------------------------+
		|param1     | Advertising data, the data format is shown in Figure 1                 |
		+-----------+------------------------------------------------------------------------+
		|param2     | Advertising_Data_Length                                                |
		|           |  Range: 0x00 to 0x1F                                                   |
		+-----------+------------------------------------------------------------------------+

		.. figure:: ../../../../_static/adv_data_format.png
			:align: center
			:alt: adv_data_format
			:figclass: align-center

			Figure 1. Advertising and Scan Response data format

		The common AD type identifier values are defined in the table below：

		+---------------+-------------------------------------------------+
		|Type Value     | Description                                     |
		+===============+=================================================+
		|0x01           | «Flags»                                         |
		+---------------+-------------------------------------------------+
		|0x02           | «Incomplete List of 16-bit Service Class UUIDs» |
		+---------------+-------------------------------------------------+
		|0x03           | «Complete List of 16-bit Service Class UUIDs»   |
		+---------------+-------------------------------------------------+
		|0x04           | «Incomplete List of 32-bit Service Class UUIDs» |
		+---------------+-------------------------------------------------+
		|0x05           | «Complete List of 32-bit Service Class UUIDs»   |
		+---------------+-------------------------------------------------+
		|0x06           | «Incomplete List of 128-bit Service Class UUIDs»|
		+---------------+-------------------------------------------------+
		|0x07           | «Complete List of 128-bit Service Class UUIDs»  |
		+---------------+-------------------------------------------------+
		|0x08           | «Shortened Local Name»                          |
		+---------------+-------------------------------------------------+
		|0x09           | «Complete Local Name»                           |
		+---------------+-------------------------------------------------+

	 - Test Case
		``AT+BLEADVDATA=0201060909373235365f424c45,D``，in this advertising uses the AD type «Complete Local Name» ，referring to ASSIC code table，it can be seen that the Name is ``7256_BLE``.

	3.AT+BLESCANRSPDATA
	 - Function Description
		Used to provide data used in Scanning Packets
	 - CMD Format
		AT+BLE=SETSCANRSPDATA=param1,param2

		+-----------+------------------------------------------------------------------------+
		|param1     | Scan_Response_Data, the data format is shown in Figure 1               |
		+-----------+------------------------------------------------------------------------+
		|param2     | Scan_Response_Data_Length                                              |
		|           |  Range: 0x00 to 0x1F                                                   |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLESCANRSPDATA=0909373235365f424c45,A``

	4.AT+BLEPERADVDATA
	 - Function Description
		Used to set the data used in periodic advertising packets
	 - CMD Format
		AT+BLEPERADVDATA=param1,param2

		+-----------+------------------------------------------------------------------------+
		|param1     | Periodic_Advertising_Data, the data format is shown in Figure 1        |
		+-----------+------------------------------------------------------------------------+
		|param2     | Periodic_Advertising_Data_Length                                       |
		|           |  Range: 0x00 to 0x1F                                                   |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEPERADVDATA=0909373235365f424c45,A``

	5.AT+BLEADVENABLE
	 - Function Description
		Used to start or stop advertising
	 - CMD Format
		AT+BLEADVENABLE=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | Advertising_Enable                                                     |
		|           |  | 0x00 : Advertising is disabled                                      |
		|           |  | 0x01 : Advertising is enabled                                       |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		| Stop advertising：``AT+BLEADVENABLE=0``
		| Start advertising：``AT+BLEADVENABLE=1``

	6.AT+BLESCANPARAM
	 - Function Description
		Used to set the scanning parameters
	 - CMD Format
		AT+BLESCANPARAM=param1,param2,param3,param4

		+-----------+------------------------------------------------------------------------+
		|param1     | Own_Address_Type                                                       |
		|           |  | 0x00 : Public Device Address                                        |
		|           |  | 0x01 : Random Device Address                                        |
		|           |  | 0x02 : Resolvable Private Address,if no IRK use the public address  |
		|           |  | 0x03 : Resolvable Private Address,if no IRK use the random address  |
		+-----------+------------------------------------------------------------------------+
		|param2     | Scanning_PHYs                                                          |
		|           |  | 0x01 : Scan advertisements on the LE 1M PHY                         |
		|           |  | 0x04 : Scan advertisements on the LE Coded PHY                      |
		|           |  | 0x05 : Scan advertisements on the LE 1M PHY and Coded PHY           |
		+-----------+------------------------------------------------------------------------+
		|param3     | Scan_Interval                                                          |
		|           |  Range: 0x0004 to 0xFFFF(Time = N * 0.625 ms)                          |
		+-----------+------------------------------------------------------------------------+
		|param4     | Scan_Window                                                            |
		|           |  Range: 0x0004 to 0xFFFF(Time = N * 0.625 ms)                          |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLESCANPARAM=0,1,64,1e``

	7.AT+BLESCANENABLE
	 - Function Description
		Used to start or stop scanning
	 - CMD Format
		AT+BLESCANENABLE=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | LE_Scan_Enable                                                         |
		|           |  | 0x00 : Scanning disabled                                            |
		|           |  | 0x01 : Scanning enabled                                             |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		| Stop scanning：``AT+BLESCANENABLE=0``
		| Start scanning：``AT+BLESCANENABLE=1``

	8.AT+BLECREATECONNECT
	 - Function Description
		Used to create an ACL connection
	 - CMD Format
		AT+BLECREATECONNECT=param1,param2,param3,param4,param5,param6

		+-----------+------------------------------------------------------------------------+
		|param1     | Connection_Interval                                                    |
		|           |  Range: 0x0006 to 0x0C80(Time = N * 1.25 ms)                           |
		+-----------+------------------------------------------------------------------------+
		|param2     | Con_Latency                                                            |
		|           |  Range: 0x0000 to 0x01F3                                               |
		+-----------+------------------------------------------------------------------------+
		|param3     | Supervision_Timeout                                                    |
		|           |  Range: 0x000A to 0x0C80(Time = N * 10 ms)                             |
		+-----------+------------------------------------------------------------------------+
		|param4     | Initiating_PHYs(Bit number)                                            |
		|           |  | 0 : Scan connectable advertisements on the LE 1M PHY                |
		|           |  | 1 : Connection parameters for the LE 2M PHY are provided            |
		|           |  | 2 : Scan connectable advertisements on the LE Coded PHY             |
		+-----------+------------------------------------------------------------------------+
		|param5     | Peer_Address_Type                                                      |
		|           |  | 0x00 : Public Device Address or Public Identity Address             |
		|           |  | 0x01 : Random Device Address or Random (static) Identity Address    |
		+-----------+------------------------------------------------------------------------+
		|param6     | Peer_Address                                                           |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLECREATECONNECT=17,0,1f4,1,1,db:b4:6f:66:8a:db``

	9.AT+BLECANCELCONNECT
	 - Function Description
		Used to cancel the creating connection
	 - CMD Format
		AT+BLECANCELCONNECT

		This CMD has no parameters.
	 - Test Case
		``AT+BLECANCELCONNECT``

	10.AT+BLEDISCONNECT
	 - Function Description
		Used to terminate an existing connection
	 - CMD Format
		AT+BLEDISCONNECT=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | Peer_Address                                                           |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEDISCONNECT=db:b4:6f:66:8a:db``

	11.AT+BLEPOWER
	 - Function Description
		Used to turn on/off bluetooth
	 - CMD Format
		AT+BLEPOWER=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | bluetooth on/off                                                       |
		|           |  | 0x00 : turn on bluetooth                                            |
		|           |  | 0x01 : turn off bluetooth                                           |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		| Turn off bluetooth：``AT+BLEPOWER=0``
		| Turn on bluetooth：``AT+BLEPOWER=1``
		
	12.AT+BLEREGISTERSERVICE
	 - Function Description
		Used to register a service
	 - CMD Format
		AT+BLEREGISTERSERVICE=param1,param2,[param3,param4,param5]

		+-----------+------------------------------------------------------------------------+
		|param1     | service_id                                                             |
		|           |  Range: 0 to 35                                                        |
		+-----------+------------------------------------------------------------------------+
		|param2     | Service UUID                                                           |
		|           |  Range: 0x0000 to 0xFFFF                                               |
		+-----------+------------------------------------------------------------------------+
		|param3     | Characteristic UUID(optional)                                          |
		|           |  Range: 0x0000 to 0xFFFF                                               |
		+-----------+------------------------------------------------------------------------+
		|param4     | data_len                                                               |
		|           |  | this param is optional, used for notification test, that specifys   |
		|           |  | the length of value to be notified                                  |
		+-----------+------------------------------------------------------------------------+
		|param5     | interval(ms)                                                           |
		|           |  | this param is optional, used for notification test, that specifys   |
		|           |  | the interval to notify value                                        |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEREGISTERSERVICE=1,ff11,ff12,20,2000``
		
	13.AT+BLEUNREGISTERSERVICE
	 - Function Description
		Used to unregister a service
	 - CMD Format
		AT+BLEUNREGISTERSERVICE=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | Service UUID                                                           |
		|           |  Range: 0x0000 to 0xFFFF                                               |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEUNREGISTERSERVICE=ff11``

	14.AT+BLEBOND
	 - Function Description
		Used to ble pairing
	 - CMD Format
		AT+BLEBOND=param1

		+-----------+------------------------------------------------------------------------+
		|param1     | Peer_Address                                                           |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEBOND=db:b4:6f:66:8a:db``

	15.AT+BLEATTREAD
	 - Function Description
		Used to read the characteristic value of the slave when the device is used as the master
	 - CMD Format
		AT+BLEATTREAD=param1,param2,param3

		+-----------+------------------------------------------------------------------------+
		|param1     | Device_ID                                                              |
		|           |  Default: 0  (Start from value 0)                                      |
		+-----------+------------------------------------------------------------------------+
		|param2     | Con_Idx                                                                |
		|           |  Default: 0  (Start from value 0)                                      |
		+-----------+------------------------------------------------------------------------+
		|param3     | Attribute_Handle                                                       |
		|           |  Start from value 0  (The value is reported during the BLE connection) |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		``AT+BLEATTREAD=0,0,9``