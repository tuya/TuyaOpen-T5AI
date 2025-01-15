GPIO
================

Beken chip supports abundant GPIO pins, some GPIOs can't be used by the application:

 - In most Beken chips, UART2 is enabled by default and GPIO 0 and GPIO 1 are used by UART2.
 - Some GPIOs may be used by specific peripheral device, the application can't use the GPIOs used by that device if the device is enabled by software. E.g. in BK7236, the SPI-1 can use GPIO 2/3/4/5, the application can't use GPIO 2/3/4/5 if SPI-1 is enabled by the software, the application can still use GPIO 2/3/4/5 if the SPI-1 is disabled by software.
 - Some GPIO groups may be used by specific peripheral device, the application can't use that GPIO group if the device chooses that GPIO group and the device is enabled by software. E.g. SPI-3 can use GPIO 30/31/32/33 or GPIO GPIO 36/37/38/39, if the software enable the SPI-3 and configure it to use the first GPIO group (GPIO 30/31/32/33), the application can't use GPIO 30/31/32/33, but the application can still use 2nd GPIO group (GPIO 36/37/38/39) if they are not used by other devices. The application can use both group if SPI-3 is NOT enabled and other devices also don't use them.

If the GPIOs are already used by the periperal devices, the GPIO API, such as cpp:func:`bk_gpio_set_config` will return GPIO_ERR_INTERNAL_USED.

Generally speaking, the GPIO user can take following steps to use the GPIO:

 - Read the chip hardware datasheet to gain overview about how the peripheral use the GPIOs
 - Check the enabled peripheral device in your application and find out the GPIOs used by the devices
 - Always check the return value of GPIO API, make sure it's not GPIO_ERR_INTERNAL_USED

.. note::

  If peripheral is enabled after the application configure it's GPIO, we have no way to detect the GPIO conflict between the application and peripheral usage, the application need to pay attention to it.


GPIO MAP Config
------------------
  gpio_map.h

  GPIO is configured according to the MAP(GPIO_DEFAULT_DEV_CONFIG) during driver initialization. Each row in the map contains nine elements.
  The 9 elements are as follows:

   - gpio_id:Corresponding PIN number starting from 0
   - second_func_en:Whether to enable the secondary function.
   - second_func_dev:Select the second function of the PIN.(Currently, a single GPIO PIN can reuse up to eight secondary functions. see GPIO_DEV_MAP.)
   - io_mode:Select the IO operating mode, input\output\high resistance
   - pull_mode:Select IO level pull up or pull down
   - int_en:Whether to turn on interrupt
   - int_type:Select trigger interrupt condition, high/low/rise/fall
   - low_power_io_ctrl:Low power whether to maintain the output level.(If not set, the GPIO will be in high resistance when entering low power mode)
   - driver_capacity:Driver ability selection, a total of four levels.



GPIO use guidance:
------------------
  
  1. Customers need to configure the **GPIO_DEFAULT_DEV_CONFIG** table according to their board configuration requirements.You can freely configure the **GPIO_DEFAULT_DEV_CONFIG** table 
     according to the capabilities of each GPIO defined in the *GPIO_DEV_MAP* table. If you want to change the function of a GPIO while the program is running, you can use the gpio_dev_unmap() 
     function to cancel the original mapping, and then use the gpio_dev_map() function to perform a new mapping. 
  2. When you want to **override** the **default GPIO_DEFAULT_DEV_CONFIG** in a customized way, you can follow the steps below:
    
    - Open macro definition: Open the **CONFIG_USR_GPIO_CFG_EN** macro in your project configuration file to enable the custom GPIO configuration function.
    - Create the usr_gpio_cfg.h file: Create the usr_gpio_cfg.h header file in an appropriate location in your project and define your custom GPIO_DEFAULT_DEV_CONFIG in this file.
      This configuration should contain the initialization settings and mappings for all GPIOs you want. For example, You can add usr_gpio_cfg.h header file to the 
      'bk_avdk_main_test\projects\lvgl\86box\config\bk7258'.
      
     It should be noted that the newly mapped function must have been defined in the GPIO_DEV_MAP table. During initialization, the chip will configure the GPIO status according to the 
     GPIO_DEFAULT_DEV_CONFIG table.(**note**:Make sure to include the usr_gpio_cfg.h file in your code so that your custom GPIO configuration is used when compiling.)
  
  3. Low power state: Can be configured as input mode and output mode, and do not need to care about the GPIO during low power mode. If the corresponding GPIO (GPIO_LOW_POWER_DISCARD_IO_STATUS) 
     is not used during low power , the low power mode program will set the GPIO to a **high resistance state** to prevent leakage during low power mode.
  4. There are **two ways to configure** low power states:

    - Determined at the compilation stage, that is, using the GPIO_DEFAULT_DEV_CONFIG table.
    - Dynamic execution phase decision, use bk_gpio_register_lowpower_keep_status() function.

  5. When you need to **maintain the output status** of GPIO in low-power mode, you can configure the corresponding GPIO through the third method.
  6. When entering low-power mode, multiple external GPIOs can be set as wake-up sources. If any GPIO has an interrupt signal, the chip can wake up from low-power mode. For example, you can set 
     multiple wake-up sources in **GPIO_STATIC_WAKEUP_SOURCE_MAP**.
  7. When entering low power mode(Low voltage only), the status of GPIO will be backed up. After exiting, GPIO will be automatically restored to the state before entering low power mode.
     (**note**:After exiting from deep sleep mode, it is equivalent to a **soft restart**, and the GPIO status will not be automatically restored.)
  
   

.. note::
   
    - gpio_map.h must be configured.
    - The GPIO pin operates from 1.8V to 3.3V, with a maximum of 0.6V for V\ :sub:`IL`\  and a minimum of 1.2V for V\ :sub:`IH`\  in 1.8V mode. 
      With a maximum of 0.8V for V\ :sub:`IL`\ and a minimum of 2V for V\ :sub:`IH`\  in 3.3V mode. 
    - The driving capacity of GPIO is shown in the table below:
        +-------+------+--------+--------+---------+
        |       | REG=2| REG=102| REG=202| REG=302 |
        +=======+======+========+========+=========+
        | P14   | 10.49| 19.95  | 33.72  | 40.8    |
        +-------+------+--------+--------+---------+
        | P15   | 10.48| 19.9   | 33.28  | 40.03   |
        +-------+------+--------+--------+---------+
        | P16   | 10.41| 19.65  | 32.76  | 39.35   |
        +-------+------+--------+--------+---------+
        | P17   | 10.36| 19.45  | 32.26  | 38.6    |
        +-------+------+--------+--------+---------+
         
                  - High level pull current

        +-------+------+--------+--------+---------+
        |       | REG=0| REG=100| REG=200| REG=300 |
        +=======+======+========+========+=========+
        | P14   | 8.54 | 16.09  | 26.05  | 32      |
        +-------+------+--------+--------+---------+
        | P15   | 8.56 | 16.19  | 26.47  | 32.6    |
        +-------+------+--------+--------+---------+
        | P16   | 8.56 | 16.35  | 26.87  | 33.24   |
        +-------+------+--------+--------+---------+
        | P17   | 8.62 | 16.5   | 27.24  | 33.85   |
        +-------+------+--------+--------+---------+

                 - Low level-sink current

 

Example: 

Configure GPIO_0 to function [GPIO_DEV_I2C1_SCL] and GPIO_1 to function [GPIO_DEV_I2C1_SDA]:
	+---------+-------------------------+-------------------+-----------------+------------------+-----------------+------------------------+---------------------------------+-----------------------+
	| gpio_id |     second_func_en      |  second_func_dev  |    io_mode      |     pull_mode    |     int_en      |        int_type        |          low_power_io_ctrl      |    driver_capacity    |
	+=========+=========================+===================+=================+==================+=================+========================+=================================+=======================+
	| GPIO_0  | GPIO_SECOND_FUNC_ENABLE | GPIO_DEV_I2C1_SCL | GPIO_IO_DISABLE | GPIO_PULL_DISABLE| GPIO_INT_DISABLE| GPIO_INT_TYPE_LOW_LEVEL| GPIO_LOW_POWER_DISCARD_IO_STATUS| GPIO_DRIVER_CAPACITY_3|
	+---------+-------------------------+-------------------+-----------------+------------------+-----------------+------------------------+---------------------------------+-----------------------+
	| GPIO_1  | GPIO_SECOND_FUNC_ENABLE | GPIO_DEV_I2C1_SDA | GPIO_IO_DISABLE | GPIO_PULL_DISABLE| GPIO_INT_DISABLE| GPIO_INT_TYPE_LOW_LEVEL| GPIO_LOW_POWER_DISCARD_IO_STATUS| GPIO_DRIVER_CAPACITY_3|
	+---------+-------------------------+-------------------+-----------------+------------------+-----------------+------------------------+---------------------------------+-----------------------+
	 
	 - PS:GPIO is turned off by default when the second function is used (io_mode is [GPIO_IO_DISABLE]). I2C needs more driving capability (driver_capacity is [GPIO_DRIVER_CAPACITY_3]).

GPIO_0 is set to high resistance(Default state):
	+---------+-------------------------+-------------------+-----------------+------------------+-----------------+------------------------+---------------------------------+-----------------------+
	| gpio_id |     second_func_en      |  second_func_dev  |    io_mode      |     pull_mode    |     int_en      |        int_type        |          low_power_io_ctrl      |    driver_capacity    |
	+=========+=========================+===================+=================+==================+=================+========================+=================================+=======================+
	| GPIO_0  | GPIO_SECOND_FUNC_DISABLE| GPIO_DEV_INVALID  | GPIO_IO_DISABLE | GPIO_PULL_DISABLE| GPIO_INT_DISABLE| GPIO_INT_TYPE_LOW_LEVEL| GPIO_LOW_POWER_DISCARD_IO_STATUS| GPIO_DRIVER_CAPACITY_0|
	+---------+-------------------------+-------------------+-----------------+------------------+-----------------+------------------------+---------------------------------+-----------------------+
	  
	  - PS:Disable all config

GPIO_0 is set to input key and falling edge to trigger the interrupt:
	+---------+-------------------------+-----------------+-----------------+----------------+----------------+--------------------------+----------------------------------+-----------------------+
	| gpio_id |     second_func_en      |  second_func_dev|    io_mode      |    pull_mode   |     int_en     |         int_type         |          low_power_io_ctrl       |    driver_capacity    |
	+=========+=========================+=================+=================+================+================+==========================+==================================+=======================+
	| GPIO_0  | GPIO_SECOND_FUNC_DISABLE| GPIO_DEV_INVALID|GPIO_INPUT_ENABLE| GPIO_PULL_UP_EN| GPIO_INT_ENABLE|GPIO_INT_TYPE_FALLING_EDGE| GPIO_LOW_POWER_DISCARD_IO_STATUS | GPIO_DRIVER_CAPACITY_0|
	+---------+-------------------------+-----------------+-----------------+----------------+----------------+--------------------------+----------------------------------+-----------------------+
	  
	  - PS:Turn off the second function related. There are 4 interrupt trigger conditions:[GPIO_INT_TYPE_LOW_LEVEL],[GPIO_INT_TYPE_HIGH_LEVEL],[GPIO_INT_TYPE_RISING_EDGE],[GPIO_INT_TYPE_FALLING_EDGE].

GPIO_0 acts as a low power wake-up source:
	+---------+-------------------------+-----------------+-----------------+----------------+----------------+--------------------------+----------------------------------+-----------------------+
	| gpio_id |     second_func_en      |  second_func_dev|    io_mode      |    pull_mode   |     int_en     |         int_type         |          low_power_io_ctrl       |    driver_capacity    |
	+=========+=========================+=================+=================+================+================+==========================+==================================+=======================+
	| GPIO_0  | GPIO_SECOND_FUNC_DISABLE| GPIO_DEV_INVALID|GPIO_INPUT_ENABLE| GPIO_PULL_UP_EN| GPIO_INT_ENABLE|GPIO_INT_TYPE_FALLING_EDGE| GPIO_LOW_POWER_DISCARD_IO_STATUS | GPIO_DRIVER_CAPACITY_0|
	+---------+-------------------------+-----------------+-----------------+----------------+----------------+--------------------------+----------------------------------+-----------------------+
	  
	  - PS:low_power_io_ctrl is [GPIO_LOW_POWER_KEEP_INPUT_STATUS], int_type is [GPIO_INT_TYPE_RISING_EDGE] or [GPIO_INT_TYPE_FALLING_EDGE];



GPIO API Status
------------------


+----------------------------------------------+---------+------------+
| API                                          | BK7236  | BK7236_cp1 |
+==============================================+=========+============+
| :cpp:func:`bk_gpio_driver_init`              | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_driver_deinit`            | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_enable_output`            | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_disable_output`           | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_enable_input`             | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_disable_input`            | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_enable_pull`              | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_disable_pull`             | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_pull_up`                  | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_pull_down`                | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_set_output_high`          | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_set_output_low`           | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_get_input`                | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_set_config`               | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_register_isr`             | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_enable_interrupt`         | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_disable_interrupt`        | Y       | Y          |
+----------------------------------------------+---------+------------+
| :cpp:func:`bk_gpio_set_interrupt_type`       | Y       | Y          |
+----------------------------------------------+---------+------------+


Application Example
------------------------------------


DEMO1:
	  GPIO_0 as wake-up source for DeepSleep or LowPower, pseudo code description and interpretation.

.. figure:: ../../../../common/_static/GPIO_Lowpower_Deepsleep.png
    :align: center
    :alt: GPIO
    :figclass: align-center

    Figure 1. GPIO as wake-up source for DeepSleep or LowPower


GPIO API Reference
---------------------

.. include:: ../../_build/inc/gpio.inc

GPIO API Typedefs
---------------------
.. include:: ../../_build/inc/gpio_types.inc


