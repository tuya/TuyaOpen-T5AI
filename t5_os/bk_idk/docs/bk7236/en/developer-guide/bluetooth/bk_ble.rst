Bluetooth Low Energy(BLE)
===========================

:link_to_translation:`zh_CN:[中文]`



Overview
""""""""""""""""""""""""""

This module provides users with interface functions such as scanning, connection, advertising and so on.
BLE consists of one or more tasks that rely on BLE interrupt-driven operation.
BLE has multiple events and callbacks, which constitute the feedback called by the user.


Role
""""""""""""""""""""""""""
Generally speaking, an actively connecting device is called central/master/client，The connected device is called peripheral/slaver/server.
Once the connection relationship between the two ends is determined, it will basically not change.




Note
""""""""""""""""""""""""""

Most APIs have a callback parameter, and you should wait for the callback to complete before proceeding to the next step.
The processing of callback and event callback should not have blocking operations.
The call stack of callback cannot be too deep.

.. important::
    Try to avoid blocking the BLE task, otherwise there will be abnormal phenomena such as disconnection, failure to scan, and failure to connect.
	
	
Common usage scenarios
""""""""""""""""""""""""""
For details, please refer to demo project ``projects/bluetooth/gatt_client`` 与 ``projects/bluetooth/gatt_server``

bluetooth init
*******************************************************
Custom's application need to register the following callback fucntion in the initialization phase.
 - Register ble event notification callback: ``bk_ble_set_notice_cb``
 - Register gatt notification/indication/read/write result event callback: ``bk_ble_register_app_sdp_charac_callback``
 - Register gatt database information callback: ``bk_ble_register_app_sdp_common_callback``

.. figure:: ../../../_static/ble_init.png
    :align: center
    :alt: ble init flow
    :figclass: align-center

    Figure 1. ble init flow

As a slave, create an ATT database for peer browsing
*******************************************************
BLE uses the ATT database as a double-ended operation entity, and all operations such as reading, writing and notification are performed on the ATT database.
In order to build a standards-compliant database, you need to understand the concepts of services, characteristics, and UUIDs.

- Attribute: A piece of data in the database is called a attribute, which consists of handle, type, and value.
- Services: Each ATT database has one or more services such as HID, HeartRate.
- Characteristic：Each service contains one or more characteristics. For example, HID includes HID map and HID report. The former is a key mapping table, and the latter is a key report. The specific operation is to read the HID map first, and then analyze the HID report according to the map to know the key corresponding to a specific value.
- UUID: The above all exist in the ATT database in the form of attributes. In order to know these special attributes, the UUID value specified by the Bluetooth standard is used to assign the type of the attribute. For example, DECL_PRIMARY_SERVICE_128 (0x2800) indicates that this attribute is a service declaration.

Slvae role can make up a service database through the interface ``bk_ble_create_db``, gatt read,write and notify,indicate operation could be performed respectively by client and server after ble connection created.

.. figure:: ../../../_static/ble_gatts_op.png
    :align: center
    :alt: ble gatts op flow
    :figclass: align-center

    Figure 2. ble gatt server operation

The following are specific examples
::

	//service definition
	#define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	//characteristic definition
	#define DECL_CHARACTERISTIC_128      {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	//Client Characteristic Configuration declaration。This is a special UUID, indicating that this attribute is used to configure the described characteristics having notification or indication function.
	#define DESC_CLIENT_CHAR_CFG_128     {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

	//database subscript:
	enum {
		TEST_IDX_SVC,
		TEST_IDX_CHAR_DECL,
		TEST_IDX_CHAR_VALUE,
		TEST_IDX_CHAR_DESC,

		TEST_IDX_NB,
	};
	//database.
	//BK_BLE_PERM_SET is used to set the permission of the attribute. For example, BK_BLE_PERM_SET(RD, ENABLE) means that this attribute can be read
	ble_attm_desc_t test_service_db[TEST_IDX_NB] = {
	   //  Service Declaration
	   [TEST_IDX_SVC]              = {DECL_PRIMARY_SERVICE_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
	   // Characteristic declaration
	   [TEST_IDX_CHAR_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
	   // Characteristic Value declaration,here indicates that this is a characteristic of type 0x1234, BK_BLE_PERM_SET(NTF, ENABLE) indicates that it has the permission to notify
	   // BK_BLE_PERM_SET(RI, ENABLE)Indicates that if this attribute is readable
	   [TEST_IDX_CHAR_VALUE]   = {{0x34, 0x12, 0},     BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
	   //Client Characteristic Configuration Descriptor
	   //once the position of notification is set to 1 through this attribute, TEST_IDX_CHAR_VALUE will turn on notification.
	   [TEST_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},
	};


	struct bk_ble_db_cfg ble_db_cfg;
	const uint16_t service_uuid = 0xffff;
	
	ble_db_cfg.att_db = (ble_attm_desc_t *)test_service_db;
	ble_db_cfg.att_db_nb = TEST_IDX_NB;
	//app handle，every time you create a database, it should be different.
	ble_db_cfg.prf_task_id = g_test_prf_task_id;
	ble_db_cfg.start_hdl = 0;
	//The type of UUID，here is 16bit
	ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
	//Copy for service specific values
    os_memcpy(&(ble_db_cfg.uuid[0]), &service_uuid, 2);

	//set callback
	bk_ble_set_notice_cb(ble_at_notice_cb);
	//create database
	bk_ble_create_db(&ble_db_cfg);

At this point, we get a 0xffff service, which contains a 0x1234 characteristic, which can enable notification by setting TEST_IDX_CHAR_DESC to 1.

We also need to handle the read and write processing event in the ble_at_notice_cb callback function.

::

	void ble_at_notice_cb(ble_notice_t notice, void *param)
	{
		switch (notice) {
		//write event from the peer，w_req->att_idx corresponds to the database subscript
		case BLE_5_WRITE_EVENT: {

			if (w_req->prf_id == g_test_prf_task_id)
			{
			    //
				switch(w_req->att_idx)
				{
				case TEST_IDX_CHAR_DECL:
					break;
				case TEST_IDX_CHAR_VALUE:
					break;
				case TEST_IDX_CHAR_DESC:
					//The writing method of TEST_IDX_CHAR_DESC has a standard definition. Here, it is simply considered that as long as it is written, the notification of TEST_IDX_CHAR_VALUE will be turned on.
					//Notify the peer through bk_ble_send_noti_value.
					//write_buffer = (uint8_t *)os_malloc(s_test_data_len);
					//bk_ble_send_noti_value(s_test_data_len, write_buffer, g_test_prf_task_id, TEST_IDX_CHAR_VALUE);
					break;

				default:
					break;
				}
			}
			break;
		}
		case BLE_5_READ_EVENT: 
		{
			ble_read_req_t *r_req = (ble_read_req_t *)param;
			BLE_LOGI("read_cb:conn_idx:%d, prf_id:%d, att_idx:%d\r\n",
					r_req->conn_idx, r_req->prf_id, r_req->att_idx);

			if (r_req->prf_id == g_test_prf_task_id) 
			{
				switch(r_req->att_idx)
				{
					case TEST_IDX_CHAR_DECL:
						break;
					case TEST_IDX_CHAR_VALUE:
						break;
					case TEST_IDX_CHAR_DESC:
						break;
					default:
						break;
				}
			}
			break;
		}
		case BLE_5_CREATE_DB:
		//bk_ble_create_db was created successfully.
		break;
		}
	}


Enable Advertising
****************************************

After setting the database, you need to enable Advertising to allow the peer to discover us

.. figure:: ../../../_static/ble_adv.png
    :align: center
    :alt: ble adv flow
    :figclass: align-center

    Figure 3. ble advertising

::

	ble_adv_param_t adv_param;

	adv_param.own_addr_type = 0;//BLE_STATIC_ADDR
	adv_param.adv_type = 0; //ADV_IND
	//7(default)
	adv_param.chnl_map = 7;
	adv_param.adv_prop = 3;
	//Minimum advertising interval
	adv_param.adv_intv_min = 0x120; //min
	//Maximum advertising interval.Generally, the smaller the value, the higher the probability of being scanned.
	adv_param.adv_intv_max = 0x160; //max
	adv_param.prim_phy = 1;// 1M
	adv_param.second_phy = 1;// 1M
	
	//Get the currently idle active index to start advertising
	actv_idx = bk_ble_get_idle_actv_idx_handle();
	if (actv_idx != UNKNOW_ACT_IDX) {
		bk_ble_create_advertising(actv_idx, &adv_param, ble_at_cmd_cb);
	}

	//Wait for BLE_CREATE_ADV event in ble_at_cmd_cb
	...
	//

	//BLE advertising data, please refer to BLE standard format
	const uint8_t adv_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
	bk_ble_set_adv_data(actv_idx, adv_data, sizeof(adv_data), ble_at_cmd_cb);

	//Wait for BLE_SET_ADV_DATA event in ble_at_cmd_cb
	...
	//

	//Scan Response Data，please refer to BLE standard format
	const uint8_t scan_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
	bk_ble_set_scan_rsp_data(actv_idx, scan_data, sizeof(scan_data), ble_at_cmd_cb);


	//Wait for BLE_SET_RSP_DATA event in ble_at_cmd_cb
	...
	//

	//Enable Advertising
	bk_ble_start_advertising(actv_idx, 0, ble_at_cmd_cb);
	
	//Wait for BLE_START_ADV event in ble_at_cmd_cb
	...
	//


The broadcast format is shown in the following figure:
    .. figure:: ../../../_static/adv_data.png
        :align: center
        :alt: menuconfig gui
        :figclass: align-center

AD Type defined in `Assigned Numbers <https://www.bluetooth.com/specifications/assigned-numbers>`_。

Enable Scan and create connection,disconnect
***********************************************************

Master role can scan and create a connection after remote device start a advertising.

.. figure:: ../../../_static/ble_scan_conn.png
    :align: center
    :alt: ble scan&conn flow
    :figclass: align-center

    Figure 4. ble scan and connection

Enable Scan

::

	ble_scan_param_t scan_param;

	scan_param.own_addr_type = 0;//BLE_STATIC_ADDR
	scan_param.scan_phy = 5;
	
	//Generally, the smaller the interval, the larger the windows, and the faster to scan data.
	scan_param.scan_intv = 0x64; //scan interval
	scan_param.scan_wd = 0x1e; //scan windows
	//Get the currently idle active index to enable scan
	actv_idx = bk_ble_get_idle_actv_idx_handle();
	bk_ble_create_scaning(actv_idx, &scan_param, ble_at_cmd);

	//Wait for BLE_CREATE_SCAN event in ble_at_cmd_cb
	...
	//
	
	bk_ble_start_scaning(actv_idx, ble_at_cmd);
	
	//Wait for BLE_START_SCAN event in ble_at_cmd_cb
	...
	//
	
	//Process BLE_5_REPORT_ADV in ble_notice_cb_t for advertising data


create connection

::

	ble_conn_param_t conn_param;
	//Generally, the smaller the interval, the better performance of the link, but the performance of other links, scanning and advertising will be poor.
	conn_param.intv_min = 0x40; //interval
	conn_param.intv_max = 0x40; //interval
	conn_param.con_latency = 0;
	conn_param.sup_to = 0x200;//supervision timeout
	conn_param.init_phys = 1;// 1M
	//Get the currently idle active index to setup connection
	con_idx = bk_ble_get_idle_conn_idx_handle();
	
	
	bk_ble_create_init(con_idx, &conn_param, ble_at_cmd);
	
	//Wait for BLE_INIT_CREATE event in ble_at_cmd_cb
	...
	//
	
	//set the peer address type, mismatch will result in failure to connect
	bk_ble_init_set_connect_dev_addr(con_idx, bt_mac, 1);
	
	
	bk_ble_init_start_conn(con_idx, ble_at_cmd)

	//Wait for BLE_INIT_START_CONN event in ble_at_cmd_cb
	...
	//

disconnect

::

    //get handle from address
    conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);

    //disconnect
    err = bk_ble_disconnect(conn_idx, ble_at_cmd);


master read and write operation
************************************************************

Master role can do read,write opertaion after remote service database information obtained in ble connected state.

.. figure:: ../../../_static/ble_gattc_op.png
    :align: center
    :alt: ble gattc op flow
    :figclass: align-center

    Figure 5. ble gatt client operation

::

    //database service information callback
    static void gattc_sdp_comm_callback(MASTER_COMMON_TYPE type,uint8 conidx,void *param)
    {
        uint16_t uuid = 0xFF;
        if(MST_TYPE_SVR_UUID == type)
        {
            struct ble_sdp_svc_ind *srv_ind = (struct ble_sdp_svc_ind*)param;
            if(srv_ind->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(srv_ind->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
                }else
                {
                    uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
                }
            }else if(srv_ind->uuid_len == 2)
            {
                uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
            }
        }else if (MST_TYPE_ATT_UUID == type)
        {
            struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf*)param;
            if(char_inf->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(char_inf->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
                }else
                {
                    uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
                }
            }else if(char_inf->uuid_len == 2)
            {
                uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
            }
        }else if(MST_TYPE_ATT_DESC == type)
        {
            struct ble_sdp_char_desc_inf *desc_inf = (struct ble_sdp_char_desc_inf*)param;
            if(desc_inf->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(desc_inf->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
                }else
                {
                    uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
                }
            }else if(desc_inf->uuid_len == 2)
            {
                uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0x%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
            }
        }else if (MST_TYPE_SDP_END == type)
        {
            BLEGATTC_LOGI("=============\r\n");
            bk_ble_gatt_mtu_change(conidx);
        }
        else if(type == MST_TYPE_UPP_ASK)
        {
            struct mst_comm_updata_para *tmp = (typeof(tmp))param;
            BLEGATTC_LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
            tmp->is_agree = 1;
        }

    }

    //read write resulte event callback
    static void gattc_sdp_charac_callback(CHAR_TYPE type,uint8 conidx,uint16_t hdl,uint16_t len,uint8 *data)
    {
    //    BLEGATTC_LOGI("%s type:%d len:%d, data:%s\n",__func__, type, len, data);
        if (CHARAC_NOTIFY == type || CHARAC_INDICATE == type)
        {
            BLEGATTC_LOGI("CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x%02x, len:%d \n", hdl, len);
        }
        else if (CHARAC_WRITE_DONE == type)
        {
            BLEGATTC_LOGI("CHARAC_WRITE_DONE, handle:0x%02x, len:%d \n", hdl, len);
        }
        else if(CHARAC_READ == type || CHARAC_READ_DONE==type)
        {
            BLEGATTC_LOGI("CHARAC_READ|CHARAC_READ_DONE, handle:0x%02x, len:%d \n", hdl, len);
        }

        if(len)
        {
            BLEGATTC_LOGI("\n==================\n");
            char s[100] = {0};
            os_memcpy(s, data, len);
            BLEGATTC_LOGI("%s \n", s);
            BLEGATTC_LOGI("\n==================\n");
            if(len>=4)
            BLEGATTC_LOGI("0x%02x 0x%02x 0x%02x 0x%02x\n", data[0],data[1],data[2],data[3]);
            BLEGATTC_LOGI("\n==================\n");
        }
    }
    //register callback
    bk_ble_set_notice_cb(gattc_notice_cb);
    bk_ble_register_app_sdp_common_callback(gattc_sdp_comm_callback);
    bk_ble_register_app_sdp_charac_callback(gattc_sdp_charac_callback);


    // notify enable and disable
    if(en)
    {
        ret = bk_ble_gatt_write_ccc(gatt_conn_ind, char_handle, 1);
    }else
    {
        ret = bk_ble_gatt_write_ccc(gatt_conn_ind, char_handle, 0);
    }
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble notify|indcate en fail :%d\n", ret);
        goto error;
    }

    //read
    uint16_t char_handle = 0x17;
    ret = bk_ble_att_read(gatt_conn_ind, char_handle);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble read att :%d\n", ret);
        goto error;
    }

    //write
    uint16_t char_handle = 0x17;
    char *data = "test";
    uint8_t len = os_strlen(data);
    ret = bk_ble_gatt_write_value(gatt_conn_ind, char_handle, len, (uint8_t *)data);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble read att :%d\n", ret);
        goto error;
    }

Reference link
""""""""""""""""

    `API Reference : <../../api-reference/bluetooth/index.html>`_ Introduced the Bluetooth API interface

    `User and Developer Guide : <../../developer-guide/bluetooth/index.html>`_ Introduced common usage scenarios of Bluetooth

    `Samples and Demos: <../../examples/bluetooth/index.html>`_ Introduced the use and operation of Bluetooth samples