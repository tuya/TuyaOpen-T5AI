低功耗蓝牙(BLE)
========================

:link_to_translation:`en:[English]`



概述
""""""""""""""""""""""""""

蓝牙模块向用户提供扫描、连接、广播、传输数据等接口功能，用于短距通讯。
蓝牙由一个或多个task执行体组成，依靠蓝牙中断驱动运行。
蓝牙有多个event和callback，这些构成了用户调用的反馈。


角色
""""""""""""""""""""""""""
一般来讲，主动连接的设备称之为central/master/client，被连接的设备称之为peripheral/slaver/server。
一旦两端连接关系确定下来，则基本不会变化。




API调用注意事项
""""""""""""""""""""""""""

大部分API具有callback参数，应当等待callback执行完成后再进行下一步。
callback、event callback的处理不应有阻塞操作。
callback的调用栈不能太深。

.. important::
    应极力避免蓝牙task被阻塞，否则会出现断连、扫不到、连不上等异常现象。
    因内存复用原因，media和蓝牙无法共存，使用蓝牙前需要关闭media相关功能，使用media前也需要将蓝牙功能关闭。


常用使用场景
""""""""""""""""""""""""""
可参考demo工程 ``projects/bluetooth/gatt_client`` 与 ``projects/bluetooth/gatt_server``

蓝牙初始化
****************************************
在应用初始化阶段，可通过以下接口注册server与client的回调函数。
 - Register ble event notification callback: ``bk_ble_set_notice_cb``
 - Register gatt notification/indication/read/write result event callback: ``bk_ble_register_app_sdp_charac_callback``
 - Register gatt database information callback: ``bk_ble_register_app_sdp_common_callback``


.. figure:: ../../../_static/ble_init.png
    :align: center
    :alt: ble init flow
    :figclass: align-center

    Figure 1. ble init flow


作为slaver，创建ATT数据库供对端浏览
****************************************
ble通过ATT数据库作为双端的操作实体，所有的读写通知等操作都是对ATT数据库进行的。
为了建立一个符合标准的数据库，需要了解Service、Characteristic、UUID的概念。

- Attribute：数据库的一条数据称之为记录，由handle，类型、值组成。
- Service：每个ATT数据库具有一个或多个服务，例如HID、HeartRate。
- Characteristic：每个服务包含一个或多个特征，例如HID包括HID map、HID report，前者是按键映射表，后者是按键上报，具体操作是先读取HID map，再根据map解析HID report就能知道按键具体值。
- UUID：以上几个均以记录的形式存在于ATT数据库中，为了知晓这些特殊记录，要用蓝牙标准规定的UUID值赋予记录的type。例如，DECL_PRIMARY_SERVICE_128(0x2800)表示这条记录为服务声明。

通过接口 ``bk_ble_create_db`` 创建gatt database，建立连接后，client，server可分别对database中定义的characteristic进行read write，notify indicate操作。

.. figure:: ../../../_static/ble_gatts_op.png
    :align: center
    :alt: ble gatts op flow
    :figclass: align-center

    Figure 2. ble gatt server operation

以下为具体示例
::

	//服务声明
	#define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	//特征声明
	#define DECL_CHARACTERISTIC_128      {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	//特征client配置声明。这是一个特殊的UUID，表示这条记录用于配置被描述的特征，一般有notify、indicate
	#define DESC_CLIENT_CHAR_CFG_128     {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

	//数据库下标:
	enum {
		TEST_IDX_SVC,
		TEST_IDX_CHAR_DECL,
		TEST_IDX_CHAR_VALUE,
		TEST_IDX_CHAR_DESC,

		TEST_IDX_NB,
	};
	//数据库.
	//BK_BLE_PERM_SET用于设置该记录的权限，例如BK_BLE_PERM_SET(RD, ENABLE)表示这条记录可被读
	ble_attm_desc_t test_service_db[TEST_IDX_NB] = {
	   //  Service Declaration
	   [TEST_IDX_SVC]              = {DECL_PRIMARY_SERVICE_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
	   // Characteristic declare
	   [TEST_IDX_CHAR_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
	   // 特征的真实值记录，这里表示这是一个type为0x1234的特征，BK_BLE_PERM_SET(NTF, ENABLE)表示具备notify的权限
	   // BK_BLE_PERM_SET(RI, ENABLE)表示如果这条记录被读，则开启nofity
	   [TEST_IDX_CHAR_VALUE]   = {{0x34, 0x12, 0},     BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
	   //Client Characteristic Configuration Descriptor
	   //具备被读被写的权限，一旦通过这条记录把nofify的位置1，则TEST_IDX_CHAR_VALUE会开启notiify。
	   [TEST_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},
	};


	struct bk_ble_db_cfg ble_db_cfg;
	const uint16_t service_uuid = 0xffff;
	
	ble_db_cfg.att_db = (ble_attm_desc_t *)test_service_db;
	ble_db_cfg.att_db_nb = TEST_IDX_NB;
	//app handle，每次创建数据库，应当不同。
	ble_db_cfg.prf_task_id = g_test_prf_task_id;
	ble_db_cfg.start_hdl = 0;
	//服务记录的UUID的类型，这里为16bit
	ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
	//给服务具体值复制
	os_memcpy(&(ble_db_cfg.uuid[0]), &service_uuid, 2);

	//设置回调
	bk_ble_set_notice_cb(ble_at_notice_cb);
	//创建数据库
	bk_ble_create_db(&ble_db_cfg);

到此，我们得到一个0xfff的服务，该服务包含一个0x1234的特征，该特征可以通过读，或写TEST_IDX_CHAR_DESC来开启notify的行为。

我们还需要在ble_at_notice_cb中处理对端读写操作的回调event。

::

	void ble_at_notice_cb(ble_notice_t notice, void *param)
	{
		switch (notice) {
		//对端的写事件，w_req->att_idx对应着数据库下标
		case BLE_5_WRITE_EVENT: 
		{

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
					//TEST_IDX_CHAR_DESC的写法有标准定义，这里简单地认为只要写入，就开启TEST_IDX_CHAR_VALUE的notify
					//通过bk_ble_send_noti_value notify对端。
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
			//bk_ble_create_db 创建成功。
			break;
		}
	}


开启广播
****************************************

设定好数据库后，需要开启广播以让对端扫描到我们。


.. figure:: ../../../_static/ble_adv.png
    :align: center
    :alt: ble adv flow
    :figclass: align-center

    Figure 3. ble advertising


::

	ble_adv_param_t adv_param;

	adv_param.own_addr_type = 0;//BLE_STATIC_ADDR
	adv_param.adv_type = 0; //ADV_IND
	//一般为7
	adv_param.chnl_map = 7;
	adv_param.adv_prop = 3;
	//最小interval
	adv_param.adv_intv_min = 0x120; //min
	//最大interval，一般越小，被扫描到的概率越高
	adv_param.adv_intv_max = 0x160; //max
	adv_param.prim_phy = 1;// 1M
	adv_param.second_phy = 1;// 1M
	
	//获取当前空闲的active index，用于开启广播
	actv_idx = bk_ble_get_idle_actv_idx_handle();
	if (actv_idx != UNKNOW_ACT_IDX) {
		bk_ble_create_advertising(actv_idx, &adv_param, ble_at_cmd_cb);
	}

	//在ble_at_cmd_cb中，等待BLE_CREATE_ADV事件
	...
	//

	//蓝牙广播数据，请参考ble标准格式
	const uint8_t adv_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
	bk_ble_set_adv_data(actv_idx, adv_data, sizeof(adv_data), ble_at_cmd_cb);

	//在ble_at_cmd_cb中，等待BLE_SET_ADV_DATA事件
	...
	//

	//扫描响应数据，请参考ble标准格式
	const uint8_t scan_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
	bk_ble_set_scan_rsp_data(actv_idx, scan_data, sizeof(scan_data), ble_at_cmd_cb);


	//在ble_at_cmd_cb中，等待BLE_SET_RSP_DATA事件
	...
	//

	//开启广播
	bk_ble_start_advertising(actv_idx, 0, ble_at_cmd_cb);

	//在ble_at_cmd_cb中，等待BLE_START_ADV事件
	...
	//

广播格式如下图:
    .. figure:: ../../../_static/adv_data.png
        :align: center
        :alt: menuconfig gui
        :figclass: align-center

AD Type定义在 `Assigned Numbers <https://www.bluetooth.com/specifications/assigned-numbers>`_

开启扫描与连接,断开连接
****************************************

对端广播开启后，master可通过scan进行扫描连接。


.. figure:: ../../../_static/ble_scan_conn.png
    :align: center
    :alt: ble scan&conn flow
    :figclass: align-center

    Figure 4. ble scan and connection

开启扫描

::

	ble_scan_param_t scan_param;

	scan_param.own_addr_type = 0;//BLE_STATIC_ADDR
	scan_param.scan_phy = 5;
	
	//一般interval越小，windows越大，越有可能扫描到数据
	scan_param.scan_intv = 0x64; //scan interval
	scan_param.scan_wd = 0x1e; //scan windows
	//获取当前空闲的active index，用于开启扫描
	actv_idx = bk_ble_get_idle_actv_idx_handle();
	bk_ble_create_scaning(actv_idx, &scan_param, ble_at_cmd);

	//在ble_at_cmd_cb中，等待BLE_CREATE_SCAN
	...
	//
	
	bk_ble_start_scaning(actv_idx, ble_at_cmd);
	
	//在ble_at_cmd_cb中，等待BLE_START_SCAN
	...
	//
	
	//在ble_notice_cb_t中处理BLE_5_REPORT_ADV ，为广播数据


	
建立连接

::

	ble_conn_param_t conn_param;
	//一般interval越小，该链路性能越好，但其他链路、扫描、广播性能会差
	conn_param.intv_min = 0x40; //interval
	conn_param.intv_max = 0x40; //interval
	conn_param.con_latency = 0;
	conn_param.sup_to = 0x200;//supervision timeout
	conn_param.init_phys = 1;// 1M
	//获取当前空闲的active index，用于建立连接
	con_idx = bk_ble_get_idle_conn_idx_handle();


	bk_ble_create_init(con_idx, &conn_param, ble_at_cmd);

	//在ble_at_cmd_cb中，等待BLE_INIT_CREATE
	...
	//

	//设置对端地址类型，不匹配会导致连接不上
	bk_ble_init_set_connect_dev_addr(con_idx, bt_mac, 1);


	bk_ble_init_start_conn(con_idx, ble_at_cmd)

	//在ble_at_cmd_cb中，等待BLE_INIT_START_CONN


断开连接

::

    //通过蓝牙地址获取连接handle
    conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);

    //断开连接
    err = bk_ble_disconnect(conn_idx, ble_at_cmd);


master进行读写操作
************************************************************

建立连接后，master可获取到对端database service信息，并对characteristic进行读写操作。


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

参考链接
""""""""""

    `API参考: <../../api-reference/bluetooth/index.html>`_ 介绍了蓝牙API接口

    `开发者指南: <../../developer-guide/bluetooth/index.html>`_ 介绍了蓝牙常用使用场景

    `样例演示: <../../examples/bluetooth/index.html>`_ 介绍了蓝牙样例使用和操作