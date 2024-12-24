SARADC
==============

:link_to_translation:`en:[English]`

概述
---------------

    SARADC 目前最多支持 16 个 channel,在 BK7236中,channel 1-6,10,12-15为数字通道,其余为模拟通道,SARADC 精度默认为13bit,
    一般精度在 SARADC 驱动中不做动态调整。

SARADC 模式类型：
------------------

 - 休眠模式：

   休眠模式下 ADC 停止任何操作，相当于 power down 的状态

 - 单步模式：

   单步模式下 ADC 每次只会采集一个数据，采集完成之后自动变为休眠模式，相应的模式状态位也会变为休眠模式
   然后等待 MCU 来读取数据，每一次的采样都需要将模式设置为单步模式.

 - 软件控制模式

   软件控制模式下会产生 ADC 中断,当处于此模式时,ADC 转换完成之后会产生一个中断，这个时候会等待 MCU 读取数据，
   如果不读取数据就不会产生中断,如果MCU读取数据之后,清除掉中断后,ADC又开始新一轮的转换,继续等待 MCU 读取数据...

 - 连续模式

   连续模式是在软件控制的模式下，去掉了 MCU 读取数据的等待，不管 MCU 取不取数据,ADC 总是按照固定节拍采样转换数据
   不受任何信号影响，只有停止 ADC 才能停止 ADC 读取数据.

.. note::

  当 ADC 处于连续模式时会一直上报中断和采集数据，这样频繁产生中断，影响系统性能，所以当 ADC 模式处于连续模式的时候需要每采取一次到想要的 ADC 数据长度之后就停止 ADC.

SARADC 获取数据流程：
----------------------

    SARADC 在连续模式下转换数据如下图所示。

.. figure:: ../../../_static/saradc_new.png
    :align: center
    :alt: saradc Overview
    :figclass: align-center

    SARADC Overview


如上图所示：
----------------------

 - S1，S2。。。表示对 ADC 某一通道上电压进行一次采样和转换，采样一次输出 16 bits 数据；采样电压范围为 0 到 2.4v; 一次采样时长 16 个 clock.

 - Sample Rate：两次采样间隔的倒数为采样率，仅在连续模式下生效.

 - Sample Cnt：有两层含义，一方面，它表示 ADC 硬件 Buffer 的大小，单位为 16bits, ADC 将每次采样得到的数据存于硬件 Buffer；另一方面，它也表示中断上报时间点，即每经过多少次采样上报一次中断；
   Sample Cnt 可由 API bk_adc_set_sample_cnt() 进行配置，默认值为 32。

 - ADC Hardware Buffer: 即硬件存放采样数据的 Buffer，大小与 Sample Cnt 相同。

 - ADC Software Buffer: ADC 驱动存放采样数据的 Buffer，每次产生 ADC 中断后，ADC 驱动会将 ADC Hardware Buffer 中的数据 copy 到 ADC Software Buffer，通常 ADC Software Buffer 大小不小于 ADC Hardware Buffer. 
   每次调用 bk_adc_read()/bk_adc_read_raw() 会先清空 ADC Software Buffer 清空，然后启动 ADC 采样直到 ADC Software Buffer 被采满才结束。


以连续模式为例，调用ADC接口的流程如下：
-----------------------------------------------------------------

 - ADC Driver Init  bk_adc_driver_init(): 通常来说初始化上电时会初始化ADC_Driver，所以一般不需要这一步。
 - ADC Acquire      bk_adc_acquire(): 加锁访问临界资源。
 - ADC Channel Init bk_adc_init(adc_channel): 打开电源、时钟源等，配置对应通道的GPIO
 - ADC Config       bk_adc_set_config(adc_config): 根据需求对ADC进行相关的配置
 - ADC Start        bk_adc_start(): 使能ADC模块
 - ADC Read         bk_adc_read()/bk_adc_read_raw(): 打开ADC中断使能，等待中断，硬件依据配置的 sample rate 进行采样，将每次采样数据存 Hardware Buffer，
                                                    当采样 sample cnt 次之后，产生 ADC 中断，ADC 驱动将 ADC Hardware Buffer 中的数据搬到 ADC Software Buffer， 最后关闭ADC中断使能
 - ADC Deinit       bk_adc_deinit(adc_channel): 关闭ADC，解除相关配置。
 - ADC Release      bk_adc_release(): 释放临界资源



