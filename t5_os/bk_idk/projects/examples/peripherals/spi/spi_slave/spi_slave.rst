.. _project_spi_slave:

+-------------------+------------+
| Supported Targets | **BK7236** |
+-------------------+------------+

SPI_SLAVE
==========

Overview
--------

This project is used for demonstrating  `SPI User Guide <https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/peripheral/bk_spi.html>`_ .

**The SPI function of BK7236 is still under development and improvement, please note the following restrictions**

  - This project only supports running on 8M FLASH.

Hardware Requirements
---------------------

- BK7236 development board with 8MB FLASH and 40MHz crystal.(e.g., `BK7236_V1 QFN80_8X8 2022.10.18 <https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/bk7236.html>`_).
- BK7236 download firmware with uart board, such as BEKEN 20221108 UART_V8.

.. Note::
   How to download firmware please reference this doc `Burn Code <https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html#hurn-code>`_

- BK7236 SPI project need 2 boards at least, and all of them need to support SPI function.And they connection is shown as Figure **SPI Connection** below.

.. figure:: ../../../../../../_static/spi_connection.png
   :align: center
   :alt: SPI Connection
   :figclass: align-center

   **SPI Connection**

.. Note::
   SPI project should be consist of only one SPI_mater device and one SPI_slave at least.If the SPI system just consist of two devices, so they must be one master and one slave.

Software Requirements
---------------------

- `Armino SDK <https://github.com/bekencorp/armino>`_.

.. Note::
   You can clone Armino SDK by referring to this doc `Armino SDK Code download <https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html#armino-sdk-code-download>`_ step by step.

Configure and Build
-------------------

Configure
+++++++++

The SPI slave device should be configured as follows:

  - SPI ID
  - SPI_BAUD_RATE
  - SPI_BUF_LEN
  - SPI_TX_TIMEOUT
  - SPI_RX_TIMEOUT
  - UART_ID
  - SPI_ROLE
  - SPI_BIT_WIDTH
  - SPI_POLARITY_MODE
  - SPI_PHASE_MODE
  - SPI_WIRE_MODE
  - SPI_BIT_ORDER_MODE
  - DMA_MODE

.. Note::
   The configuration of SPI master device is almost the same as that configuration of SPI slave device.The only difference is the configuration of **SPI_ROLE** which will be **SPI_master_role** or **SPI_slave_role**.

Build
+++++

Enter the following command in Armino root directory to compile:

``make bk7236 PROJECT=examples/peripherals/spi/spi_slave``

For preparation such as toolchain installation, please refer to `Quick Start <https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html>`_.

Connect UART
++++++++++++

UART0 is used for:

  - BKFIL to download images
  - Logging and CLI

FLASH *all-app.bin*
+++++++++++++++++++

Burn *Armino/build/spi_slave/bk7236/app-app.bin* to the board using BKFIL.

Running and Output
------------------

Case 1 - SPI_slave Send data
+++++++++++++++++++++++++++++

SPI_slave_device Send Operation
********************************

**Case 1.1** edit the function of main() in *spi_slave_main.c*, and you will send 0~1023 to SPI_master_device.

.. code-block:: c
   :linenos:

   int main(void)
   {
        BK_LOG_ON_ERR(bk_spi_driver_init());
        BK_LOG_ON_ERR(spi_example_send_data());
     // BK_LOG_ON_ERR(spi_example_dma_send_data());
     // BK_LOG_ON_ERR(spi_example_recv_data());
     // BK_LOG_ON_ERR(spi_example_dma_recv_data());

        return 0;
   }

**Case 1.2** edit the function of main() in *spi_slave_main.c*, and you will send 0~1023 to SPI_master_device through DMA which write data to SPI regesiter.

.. code-block:: c
   :linenos:

   int main(void)
   {
        BK_LOG_ON_ERR(bk_spi_driver_init());
     // BK_LOG_ON_ERR(spi_example_send_data());
        BK_LOG_ON_ERR(spi_example_dma_send_data());
     // BK_LOG_ON_ERR(spi_example_recv_data());
     // BK_LOG_ON_ERR(spi_example_dma_recv_data());

        return 0;
   }

Case 2 - SPI_slave Receive data
++++++++++++++++++++++++++++++++

SPI_slave_device Receive Operation
***********************************

**Case 2.1** edit the function of main() in *spi_slave_main.c*, and you will receive data from SPI_master_device.

.. code-block:: c
   :linenos:

   int main(void)
   {
        BK_LOG_ON_ERR(bk_spi_driver_init());
     // BK_LOG_ON_ERR(spi_example_send_data());
     // BK_LOG_ON_ERR(spi_example_dma_send_data());
        BK_LOG_ON_ERR(spi_example_recv_data());
     // BK_LOG_ON_ERR(spi_example_dma_recv_data());

        return 0;
   }

**Case 2.2** edit the function of main() in *spi_slave_main.c*, and you will receive data from SPI_master_device through DMA which read data from SPI regesiter.

.. code-block:: c
   :linenos:

   int main(void)
   {
        BK_LOG_ON_ERR(bk_spi_driver_init());
     // BK_LOG_ON_ERR(spi_example_send_data());
     // BK_LOG_ON_ERR(spi_example_dma_send_data());
     // BK_LOG_ON_ERR(spi_example_recv_data());
        BK_LOG_ON_ERR(spi_example_dma_recv_data());

        return 0;
   }


