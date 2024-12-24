内存与性能
====================================

:link_to_translation:`en:[English]`

内存概述
--------------------------

    BK7258芯片整体有8M(或16M) psram，640K sram，三个Cortex M33核各有独立的16k dtcm和16k itcm

SRAM概述
-------------------------

    - BK7258芯片当前为三核 CM33 AMP架构；有640K sram，可以通过如下三个配置项分别配置三个cpu的sram大小::

        CONFIG_CPU0_SPE_RAM_SIZE //配置cpu0 sram的大小
        CONFIG_CPU1_APP_RAM_SIZE //配置cpu1 sram的大小
        CONFIG_CPU2_APP_RAM_SIZE //配置cpu2 sram的大小

     - 工程里修改这三个配置就好，三个加起来等于640K::

        注意, 需要三个cpu的config文件同时修改:
        {yourprojectname}/config/bk7258/config
        {yourprojectname}/config/bk7258_cp1/config
        {yourprojectname}/config/bk7258_cp2/config


PSRAM概述
-------------------------

    PSRAM提供足够的内存空间，供不同的模块使用，当前BK7258支持两种类型的PSRAM，它们的内存大小不同，分别为8Mbyte和16Mbyte，使用不同的宏进行区分。
    多媒体中定义了四种类型的模块需要使用PSRAM，由于PSRAM还会用来在不同的CPU上作为heap使用，所以并不是全部的PSRAM空间都被多媒体使用，长度和空间都是需要进行区分的。

1、PSRAM的类型
----------------------------

    BK7258支持两种类型的PSRAM，具体配置如下：

    +--------------------+---------------+
    |        name        |  size(Mbyte)  |
    +--------------------+---------------+
    |    APS6408L_O      |       8       |
    +--------------------+---------------+
    |   APS128XXO_OB9    |       16      |
    +--------------------+---------------+

    注意：不同的工程默认使用的psram类型不一样，需要看编译后的sdkconfig文件。

2、PSRAM的划分
--------------------------

    PSRAM主要分为两部分，一部分是多媒体当作数据内存进行使用，另一部分当作不同CPU的heap空间进行使用，需要参考下面的宏定义去识别各自使用的大小：

    +-------------------------------------+---------------+-------------------------------------+
    |              marco                  |     value     |           implication               |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM                       |       Y       |   使能PSRAM模块                     |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_AS_SYS_MEMORY         |       Y       |  使能PSRAM作为Heap                  |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_USE_PSRAM_HEAP_AT_SRAM_OOM  |       Y       |  使能sram申请不到内存，从psram申请  |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_HEAP_BASE             |  0x60700000   |  当前CPU上psram作为heap的起始地址   |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_HEAP_SIZE             |    0x80000    |  当前CPU上psram作为heap的长度       |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER  |  0x60700000   |  当前芯片上psram作为heap的起始地址  |
    +-------------------------------------+---------------+-------------------------------------+

     - 功能开关CONFIG_PSRAM_AS_SYS_MEMORY依赖CONFIG_PSRAM和CONFIG_FREERTOS_V10

.. note::

	注意：上面的值在不同的核上定义的值是有区别的，CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER表示PSRAM作为heap的起始地址，表示psram从这个地址开始作为heap空间。
	系统默认从PSRAM的起始地址(0x60000000)，到CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER为止都是分配给多媒体使用，剩余的部分给Heap使用。用宏CONFIG_PSRAM_HEAP_BASE
	定义当前核上PSRAM作为Heap的起始地址，用CONFIG_PSRAM_HEAP_SIZE定义空间大小，不同的核上定义的值不一样，CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER在不同的核上值是一样的。
	应该与不同核上CONFIG_PSRAM_HEAP_BASE的最小值保持一致，比如默认BK7258的工程，CPU0上CONFIG_PSRAM_HEAP_BASE=0x60700000，CONFIG_PSRAM_HEAP_SIZE=0x80000，那么CPU1上
	CONFIG_PSRAM_HEAP_BASE=0x60780000，CONFIG_PSRAM_HEAP_SIZE=0x80000，CPU2上默认PSRAM不作为Heap使用，所以CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60700000。
	某个核上是否需要将psram作为heap，是由CONFIG_PSRAM_AS_SYS_MEMORY控制。

3、PSRAM作为heap使用
-----------------------

 - 通过如下两个配置项配置psram heap的起始地址和大小::

        CONFIG_PSRAM_HEAP_BASE //配置psram heap的起始地址
        CONFIG_PSRAM_HEAP_SIZE //配置psram size的起始地址

 - 内存申请和释放接口::

        void *psram_malloc(size_t size);
        void  psram_free(void *p);

 - BK7258芯片低压功能PSRAM会下电， 如果上层有使用的psram内存没有释放, 会导致无法进入低压, 可以通过如下接口打印当前psram内存使用情况::

        void bk_psram_heap_get_used_state(void);

 - 将Task创建在psram上接口::

        bk_err_t rtos_create_psram_thread(beken_thread_t *thread, uint8_t priority, const char *name, beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg);
        bk_err_t rtos_delete_thread(beken_thread_t *thread); // 删除task接口不变

4、PSRAM在多媒体上的划分
----------------------------

	PSRAM在多媒体上使用，根据功能被划分为四个模块，参考结构体：``psram_heap_type_t``，位置：``bk_idk/include/driver/psram_types.h``.

	各个模块使用的内存是由结构体：``psram_mem_slab_mapping``，定义的，参考路径：``bk_avdk_main/components/media_utils/Kconfig``。各个模块分配的内存大小由宏进行控制，如下所示：

    +-------------------------------------+---------------+-------------------------------------+
    |              marco                  |  value(byte)  |           implication               |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_USER_SIZE    |    102400     |     分配给user使用的大小            |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE   |    102400     |     分配给audio使用的大小           |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE  |    1433600    |   分配给编码(jpeg/h264)使用的大小   |
    +-------------------------------------+---------------+-------------------------------------+
    |  CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE |    5701632    |      分配给显示使用的大小           |
    +-------------------------------------+---------------+-------------------------------------+

.. note::

	注意：上面宏定义的值是默认定义的，使用时可以根据自身需求，动态调整，直接在对应project中cpu的config进行修改即可，但是注意上面的长度加起来,
	不能超过Heap使用的地址(CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER)，否则会出问题。

5、各个模块内存调整
-----------------------------

	根据上节所述，psram被划分为四个模块，不同的模块存储的数据类型不同，具体如下：

	- UASER：分配给用户使用，分配的大小为宏CONFIG_PSRAM_MEM_SLAB_USER_SIZE定义；
	- AUDIO：分配给audio使用，分配的大小为宏CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE定义；存储的是音频数据；
	- ENCODE：分配给编码使用，分配的大小为宏CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE定义，存储的是完整的JPEG图像或者H264图像；
	- DISPLAY：分配给显示使用，分配的大小为宏CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE定义，存储的是显示的数据类型，如YUV、RGB565、RGB888等；

	根据上面不同的模块功能，以及大小，存储的数据量也不一样，比如ENCODE模块可以存储不止一帧的JPEG图像或H264图像，
	系统还定义了一帧图像的大小的宏，参考文件：``./bk_idk/middleware/driver/camera/Kconfig``:

    +----------------------------+---------------+-------------------------------------+------------------+
    |           marco            |  value(byte)  |           implication               |     range        |
    +----------------------------+---------------+-------------------------------------+------------------+
    |  CONFIG_JPEG_FRAME_SIZE    |    153600     |     定义一帧JPEG图像的大小          |   [0, 204800]    |
    +----------------------------+---------------+-------------------------------------+------------------+
    |  CONFIG_H264_FRAME_SIZE    |     65536     |     分配给audio使用的大小           |   [0, 102400]    |
    +----------------------------+---------------+-------------------------------------+------------------+

	上面的大小需要根据自身需求需要调整，比如需要存储1280X720的JPEG图像，150K的空间可能不够，需要改成200K(204800)，甚至更大，根据实际使用情况调整。
	同样针对H264数据，有时候需要调整H264的压缩率，以达到更清楚的画质，默认的64K可能不够，需要继续调大，所以也需要根据实际情况做调整。

	根据上面的定义的大小，不同块存储的个数可以计算出来，假设DISPLAY模块使用的RGB565，且分辨率为：800X480，那么一帧图像的大小为：800*480*2=768000，
	可存储的个数为：CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE/768000=7，表示最大存7帧800X480的RGB565图像；

	假设ENCODE模块都用来存储JPEG图像，最多存储数量：CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE/CONFIG_JPEG_FRAME_SIZE=9；但实际情况会同时存储JPEG和H264数据，代码中定义了
	各个图像模块最大的存储个数，参考：``bk_avdk_main/components/multimedia/comm/frame_buffer.c``，如下语句定义：

	``uint8_t fb_count[FB_INDEX_MAX] = {5, 4, H264_GOP_FRAME_CNT * 2};``，表示最大保存5帧DISPLAY数据，4帧JPEG数据，H264_GOP_FRAME_CNT*2帧H264数据。

	上面的数量可以调整，只要保证总的数据量不超过各自模块的size即可。

6、多媒体上PSRAM使用
----------------------

	因为多媒体的功能都在CPU1上使用，所以针对PSRAM的使用，只能在CPU1上直接调用，系统CPU1启动后会自动给多媒体整个PSRAM进行初始化，用户不需要去自己调用实现。
	当CPU1掉电时，多媒体不再使用PSRAM，不需要额外调用注销的接口去释放相应的内存。

 - 内存初始化接口::

        bk_psram_frame_buffer_init

 - 内存申请和释放接口::

        void *bk_psram_frame_buffer_malloc(psram_heap_type_t type, uint32_t size);
        void bk_psram_frame_buffer_free(void* mem_ptr);

.. note::

	客户使用时，建议使用系统的接口去申请和释放psram内存(psram_malloc\psram_free)，不建议使用上面多媒体模块自定义的申请和释放psram内存接口