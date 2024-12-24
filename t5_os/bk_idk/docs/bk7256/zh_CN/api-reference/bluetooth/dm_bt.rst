DM BT APIs
================

:link_to_translation:`en:[English]`

.. 重要::

    DM BT API v1.0 是最新稳定的 DM BT API。所有新应用程序都应使用 DM BT API v1.0。


DM BT API 类别
----------------------------

    大多数 DM BT API 可分为：

      - BT 通用接口
      - BT A2DP Sink 接口


应用程序示例
-------------------

    示例工程 `projects/lvgl/soundbar <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/lvgl/soundbar>`_ ，其中包含以下应用程序：

    * 一个 A2DP 接收器客户端 Demo，可以被 A2DP Source 设备发现并连接，并从远程设备接收音频流。

API 接口定义
----------------------------------------

.. include:: ../../_build/inc/bk_dm_bt.inc

API 类型定义
----------------------------------------

.. include:: ../../_build/inc/bk_dm_bt_types.inc


参考链接
----------------

    `API 参考手册: <../../api-reference/bluetooth/index.html>`_ 介绍了蓝牙API接口

    `开发者指南: <../../developer-guide/bluetooth/index.html>`_ 介绍了蓝牙常用使用场景

    `样例演示: <../../examples/bluetooth/index.html>`_ 介绍了蓝牙示例的使用和操作

    `蓝牙工程: <../../projects_work/bluetooth/index.html>`_ 介绍了蓝牙相关工程
