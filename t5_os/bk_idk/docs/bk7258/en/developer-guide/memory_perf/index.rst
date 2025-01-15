Memory and Performance
===================================

:link_to_translation:`zh_CN:[中文]`

Memory overview
----------------------------------------

    The BK7258 chip has 8M(or 16M) psram, 640K sram as a whole, and the three Cortex M33 cores each have independent 16k dtcm and 16k itcm


SRAM Overview
---------------------------------------

    - The BK7258 chip is currently a three-core CM33 AMP architecture; it has 640K sram. You can configure the sram sizes of the three CPUs through the following three configuration items::

         CONFIG_CPU0_SPE_RAM_SIZE Configure the size of cpu0 sram
         CONFIG_CPU1_APP_RAM_SIZE Configure the size of cpu1 sram
         CONFIG_CPU2_APP_RAM_SIZE Configure the size of cpu2 sram

    - Just modify these three configurations in the project. The sum of the three is equal to 640K::

        Note that the config files of three CPUs need to be modified at the same time:
        {yourprojectname}/config/bk7258/config
        {yourprojectname}/config/bk7258_cp1/config
        {yourprojectname}/config/bk7258_cp2/config


PSRAM Overview
---------------------------------------

    PSRAM provides enough memory space for different modules to use, the current BK7258 supports two types of PSRAM, their memory size is different,
    respectively, 8Mbyte and 16Mbyte, using different macros to distinguish. There are four types of modules in multimedia that need to use PSRAM.
    Since PSRAM is also used as heap on different cpus, not all of the PSRAM space is used by multimedia, and the length and space need to be distinguished.

1. PSRAM Type
----------------------------

    The BK7258 supports two types of PSRAM, which are configured as follows:

    +--------------------+---------------+
    |        name        |  size(Mbyte)  |
    +--------------------+---------------+
    |    APS6408L_O      |       8       |
    +--------------------+---------------+
    |   APS128XXO_OB9    |       16      |
    +--------------------+---------------+

	Note: Different projects use different psram types by default, you need to see the compiled sdkconfig file.

2. Division of PSRAM
--------------------------

	PSRAM is mainly divided into two parts, one part is used as data memory, the other part is used as heap space of different cpus,
	you need to refer to the following macro definition to identify the size of their use:

    +-------------------------------------+---------------+----------------------------------------------------+
    |              marco                  |     value     |           implication                              |
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_PSRAM                       |       Y       |  enable PSRAM module                               |
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_PSRAM_AS_SYS_MEMORY         |       Y       |  enable PSRAM as Heap                              |
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_USE_PSRAM_HEAP_AT_SRAM_OOM  |       Y       |  enable alloc from sram fail, then alloc from psram|
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_PSRAM_HEAP_BASE             |  0x60700000   |  the start address of psram as heap in current cpu |
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_PSRAM_HEAP_SIZE             |    0x80000    |  the size psram as heap in cluurent cpu            |
    +-------------------------------------+---------------+----------------------------------------------------+
    |  CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER  |  0x60700000   |  the start address of psram as heap in this chip   |
    +-------------------------------------+---------------+----------------------------------------------------+

    - Function switch CONFIG_PSRAM_AS_SYS_MEMORY depends on CONFIG_PSRAM and CONFIG_FREERTOS_V10

.. note::

	The above values are defined differently on different cores, CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER indicates that PSRAM serves as the heap starting address,
	and that psram serves as the heap space from this address. By default, the starting address of PSRAM (0x60000000) through CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER
	is allocated for multimedia use, and the rest is allocated for Heap use. Use the macro CONFIG_PSRAM_HEAP_BASE Define PSRAM on the current kernel as the Heap start address,
	and use CONFIG_PSRAM_HEAP_SIZE to define the space size. Different cores have different defined values, CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER has the same value on different cores.
	This should be consistent with the minimum value of CONFIG_PSRAM_HEAP_BASE on different cores, such as the default BK7258 project, CONFIG_PSRAM_HEAP_BASE=0x60700000 on CPU0,
	CONFIG_PSRAM_HEAP_BASE=0x60700000, CONFIG_PSRAM_HEAP_SIZE=0x80000, then CPU1 CONFIG_PSRAM_HEAP_BASE=0x60780000, CONFIG_PSRAM_HEAP_SIZE=0x80000,
	PSRAM is not used as Heap by default on CPU2. So CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60700000. Whether psram is required as heap on a kernel is
	controlled by CONFIG_PSRAM_AS_SYS_MEMORY.

3. PSRAM used for heap
------------------------

    - Configure the starting address and size of psram heap through the following two configuration items::

         CONFIG_PSRAM_HEAP_BASE configures the starting address of psram heap
         CONFIG_PSRAM_HEAP_SIZE configures the starting address of psram size

     - Memory application and release interface::

        void *psram_malloc(size_t size);
        void  psram_free(void *p);

     - The low-voltage function PARAM of the BK7258 chip will be powered off. If the psram memory used by the upper layer is not released, it will result in the inability to enter low voltage. You can print the current psram memory usage through the following interface::

        void bk_psram_heap_get_used_state(void);

     - The API of create task on psram::

        bk_err_t rtos_create_psram_thread(beken_thread_t *thread, uint8_t priority, const char *name, beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg);
        bk_err_t rtos_delete_thread(beken_thread_t *thread); // The API of delete task is not change


4. PSRAM in the multimedia division
-------------------------------------

	PSRAM is used in multimedia and is divided into four modules according to function, reference structure: `psram_heap_type_t`, position: `bk_idk/include/driver/psram_types.h`.

	The memory used by each module is defined by the structure: `psram_mem_slab_mapping`. The amount of memory allocated by each module is controlled by macros,
	reference position: `bk_avdk_main/components/media_utils`, as follows:

    +-------------------------------------+---------------+-------------------------------------+
    |              marco                  |  value(byte)  |           implication               |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_USER_SIZE    |    102400     |     the size alloc to user          |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE   |    102400     |     the size alloc to audio         |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE  |    1433600    | the size alloc to encode(jpeg/h264) |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE |    5701632    |       the size alloc to display     |
    +-------------------------------------+---------------+-------------------------------------+

	Note: The value defined by the above macro is defined by default, which can be dynamically adjusted according to its own needs when used,
	and can be modified directly in the cpu config of the corresponding project. However, note that the length added up above cannot exceed the address used by the
	Heap(CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER), otherwise there will be problems.

5. Each module of multimedia memory adjustment
-----------------------------------------------

	According to the previous section, psram is divided into four modules, different modules store different types of data, as follows:

	- UASER: allocated to users. The allocated size is defined by the macro CONFIG_PSRAM_MEM_SLAB_USER_SIZE.
	- AUDIO: allocated to audio. The allocated size is defined by the macro CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE. It stores audio data;
	- ENCODE: allocated to encoding, the allocated size is defined by the macro CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE, which stores complete JPEG images or H264 images;
	- DISPLAY: allocated to the display. The allocated size is defined by the macro CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE, which stores the displayed data type, such as YUV, RGB565, RGB888, etc.

	The amount of data stored varies according to the function and size of the module above. For example, the ENCODE module can store more than one frame of JPEG image or H264 image.
	The size of the system also defines a frame of macros, reference files: ``./bk_idk/middleware/driver/camera/Kconfig``:

    +----------------------------+---------------+-------------------------------------+------------------+
    |           marco            |  value(byte)  |           implication               |     range        |
    +----------------------------+---------------+-------------------------------------+------------------+
    |  CONFIG_JPEG_FRAME_SIZE    |    153600     | the size of one complete jpeg frame |   [0, 204800]    |
    +----------------------------+---------------+-------------------------------------+------------------+
    |  CONFIG_H264_FRAME_SIZE    |     65536     | the size of one complete h264 frame |   [0, 102400]    |
    +----------------------------+---------------+-------------------------------------+------------------+

	The above size needs to be adjusted according to their own needs, such as the need to store 1280X720 JPEG images, 150K space may not be enough,
	need to be changed to 200K(204800), or even larger, according to the actual use of adjustment. Also for H264 data, sometimes need to adjust the
	compression rate of H264, in order to achieve a clearer picture quality, the default 64K May not be enough, need to continue to increase,
	so also need to adjust according to the actual situation.

	According to the size defined above, the number of different block storage can be calculated. Assuming the RGB565 used by the DISPLAY module, and the resolution is 800X480,
	then the size of an image is 800*480*2=768000. The number that can be stored is: CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE/768000=7, which means that the maximum storage capacity
	is 7 frames of 800X480 RGB565 images.

	Assuming the ENCODE module is used to store JPEG images, the maximum number of stores is: CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE/CONFIG_JPEG_FRAME_SIZE=9;
	However, the actual situation will store both JPEG and H264 data, which is defined in the code the largest number, each image module reference:
	``./bk_avdk_main/components/multimedia/comm/frame_buffer.c``, definition in the following statement:

	``uint8_t fb_count[FB_INDEX_MAX] = {5, 4, H264_GOP_FRAME_CNT * 2};``

	It means that the maximum storage is 5 frames of DISPLAY data, 4 frames of JPEG data, H264_GOP_FRAME_CNT*2 frames of H264 data.
	The above quantity can be adjusted, as long as the total amount of data does not exceed the size of the respective module.

6. Using psram on multimedia
-----------------------------

	Because all multimedia functions are used in CPU1, the use of PSRAM can only be directly invoked in CPU1. After CPU1 is started,
	the system will automatically initialize the entire PSRAM for the multimedia, and users do not need to invoke the implementation themselves.
	When CPU1 is powered down, multimedia do not use PSRAM, and no additional call to the logged out interface is required to free up the corresponding memory.

- Memory initialization interface ::

		bk_psram_frame_buffer_init

- Memory request and release interface ::

		void *bk_psram_frame_buffer_malloc(psram_heap_type_t type, uint32_t size);
		void bk_psram_frame_buffer_free(void* mem_ptr);

.. note::

	When used by customers, it is recommended to use the system interface to apply for and release psram memory (psram_malloc\psram_free),
	and it is not recommended to use the above multimedia module defined interface to apply for and release psram memory.