自组网(Mesh)
======================================

:link_to_translation:`en:[English]`

1 功能概述
----------------------------

    用于低功耗节点等自组网场景，开启ble、mesh feature。

2 代码路径
----------------------------

    Demo路径: `projects/bluetooth/mesh <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/bluetooth/mesh>`_

    编译命令：make bk7258 PROJECT=bluetooth/mesh

3 cli命令
-------------------------------------

    双板之间测试Mesh命令

    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision init provisionee              | Provisonee作为节点加入Mesh网络                 |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision init provisioner              | Provisioner组建Mesh网络                        |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision deprovision <MAC>             | Provisioner将设备移出网络                      |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision send_count <time>             | Provisioner定时发送数据                        |
    +--------------------------------------------------+------------------------------------------------+


    板子加入天猫精灵组建的Mesh网络

    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh tmall init <product_id> <device_secret> | 作为Node节点加入天猫精灵组建的Mesh网络         |
    +--------------------------------------------------+------------------------------------------------+

4 注意事项
-------------------------------------

    | 1.加入天猫精灵组件的Mesh网络需要输入设备的 Product ID 和 Secret，在 `天猫精灵开放平台  <https://www.aligenie.com/doc/357554/gtgprq>`_  上，按照直接接入流程进行申请；


参考链接
----------

    `API参考: <../../../api-reference/bluetooth/index.html>`_ 介绍了蓝牙API接口

    `开发者指南: <../../../developer-guide/bluetooth/index.html>`_ 介绍了蓝牙常用使用场景

    `样例演示: <../../../examples/bluetooth/index.html>`_ 介绍了蓝牙样例使用和操作

    `蓝牙工程: <../../../projects_work/bluetooth/index.html>`_ 介绍了蓝牙相关工程
