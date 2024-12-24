BLE APIs
================

:link_to_translation:`zh_CN:[中文]`

.. important::

    The BLE API v1.0 is the lastest stable BLE APIs. All new applications should use BLE API v1.0.

BLE API Categories
----------------------------

    Most of BLE APIs can be categoried as:

         - BLE common interface
         - BLE scan interface
         - BLE ADV interface
         - BLE connect interface

Compatibility and Extension
----------------------------------------

The BLE APIs are flexible, easy to be extended and backward compatible. For most of the BLE configurations, we put some reserved fields in the config struct for future extendence. The API users need to make sure the reserved fields are initialized to 0, otherwise the compatibility may be broken as more fields are added.

Programing Principle
----------------------------------------

.. important::
  Here is some general principle for BLE API users:
   - Always init the reserved fields of config stuct to 0
   - Use BK_ERR_CHECK to check the return value of the BLE API
   - If you are not sure how to use BLE APIs, study the BLE example code first
   - If you are not sure how to initialize some fields of config struct, use the default configuration macro to use the config first and then set application specific fields.
   - Don't do too much work in BLE event callback, relay the event to your own application task.

User Development Model
----------------------------------------

Similar as most popular BLE driver, the Beken BLE driver is implemented as event driver. The application call BLE APIs to operate the BLE driver and get notified by BLE event.


API Reference
----------------------------------------

.. include:: ../../_build/inc/bk_ble.inc

API Typedefs
----------------------------------------
.. include:: ../../_build/inc/bk_ble_types.inc



Reference Link
----------------

    `API Reference : <../../api-reference/bluetooth/index.html>`_ Introduced the Bluetooth API interface

    `User and Developer Guide : <../../developer-guide/bluetooth/index.html>`_ Introduced common usage scenarios of Bluetooth

    `Samples and Demos: <../../examples/bluetooth/index.html>`_ Introduced the use and operation of Bluetooth samples

    `Bluetooth Project: <../../projects_work/bluetooth/index.html>`_ Introduced Bluetooth related Project