Mesh
======================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
----------------------------

    Used in low-power nodes and other ad hoc networking scenarios, enabling the ble and mesh features.

2 Code Path
----------------------------

    demo: `./projects/bluetooth/mesh <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/bluetooth/mesh>`_

    build cmd：``make bk7258 PROJECT=bluetooth/mesh``

3 cli
-------------------------------------

    Mesh command for testing between two boards

    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision init provisionee              | Provisionee joins the Mesh network as a node   |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision init provisioner              | Provisioner forms a Mesh network               |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision deprovision <MAC>             | Provisioner moves devices out of the network   |
    +--------------------------------------------------+------------------------------------------------+
    | ble_mesh provision send_count <time>             | Provisioner sends data on a scheduled basis    |
    +--------------------------------------------------+------------------------------------------------+

    The board joins the Mesh network formed by Tmall Genie

    +--------------------------------------------------+------------------------------------------------------------+
    | ble_mesh tmall init <product_id> <device_secret> | Join the Mesh network formed by Tmall Genie as a Node node |
    +--------------------------------------------------+------------------------------------------------------------+

4 Attention
-------------------------------------

    | 1. The Mesh network that joins the Tmall Genie component requires the input of the device's Product ID and Secret, which can be found on the `Tmall Genie Open Platform <https://www.aligenie.com/doc/357554/gtgprq>`_ Apply according to the ``Direct Access`` process;


Reference Link
----------------

    `API Reference : <../../../api-reference/bluetooth/index.html>`_ Introduced the Bluetooth API interface

    `User and Developer Guide : <../../../developer-guide/bluetooth/index.html>`_ Introduced common usage scenarios of Bluetooth

    `Samples and Demos: <../../../examples/bluetooth/index.html>`_ Introduced the use and operation of Bluetooth samples

    `Bluetooth Project: <../../../projects_work/bluetooth/index.html>`_ Introduced Bluetooth related Project