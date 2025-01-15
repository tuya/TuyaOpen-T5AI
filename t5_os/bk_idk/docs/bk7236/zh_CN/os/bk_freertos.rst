FreeRTOS
===============================

:link_to_translation:`en:[English]`

FreeRTOS简介
------------------------

 - FreeRTOS是一个迷你的实时操作系统内核。
 - 作为一个轻量级的操作系统，功能包括：任务管理、时间管理、信号量、消息队列、内存管理、记录功能、软件定时器、协程等，可基本满足较小系统的需要
 - FreeRTOS能在小RAM单片机上运行。FreeRTOS操作系统是完全免费的开源操作系统，具有源码公开、可移植、可裁减、调度策略灵活的特点，可以方便地移植到各种单片机上运行


FreeRTOS通用架构
------------------------

.. figure:: ../../_static/freertos_arch.png
    :align: center
    :alt: FreeRTOS Architecture
    :figclass: align-center

    FreeRTOS 架构

- 一个FreeRTOS 系统主要由BSP驱动+内核+组件组成（如上图）。内核包含多任务调度、内存管理、任务间通信的功能，组件包含网络协议、外设支持等。
- FreeRTOS内核是可剪裁的，组件也是可选的。由于嵌入式应用往往对内存空间的要求十分苛刻，所以一个可剪裁的RTOS对于嵌入式应用非常重要。这使得FreeRTOS的核心代码只有9000行左右。
- Armino平台FreeRTOS通用架构使用的是内核代码路径: components/os_source/freertos_v10

功能和特点
------------------------

 - 用户可配置内核功能
 - 多平台的支持
 - 提供一个高层次的信任代码的完整性
 - 目标代码小，简单易用
 - 遵循MISRA-C标准的编程规范
 - 强大的执行跟踪功能
 - 堆栈溢出检测
 - 没有限制的任务数量
 - 没有限制的任务优先级
 - 多个任务可以分配相同的优先权
 - 队列，二进制信号量，计数信号灯和递归通信和同步的任务
 - 优先级继承
 - 免费开源的源代码


FreeRTOS 小结
------------------------

 - 作为一个轻量级的操作系统，FreeRTOS提供的功能包括：任务管理、时间管理、信号量、消息队列、内存管理、记录功能等，可基本满足较小系统的需要。FreeRTOS内核支持优先级调度算法，每个任务可根据重要程度的不同被赋予一定的优先级，CPU总是让处于就绪态的、优先级最高的任务先运行。FreeRTOS内核同时支持轮换调度算法，系统允许不同的任务使用相同的优先级，在没有更高优先级任务就绪的情况下，同一优先级的任务共享CPU的使用时间。
 - FreeRTOS的内核可根据用户需要设置为可剥夺型内核或不可剥夺型内核。当FreeRTOS被设置为可剥夺型内核时，处于就绪态的高优先级任务能剥夺低优先级任务的CPU使用权，这样可保证系统满足实时性的要求；当FreeRTOS被设置为不可剥夺型内核时，处于就绪态的高优先级任务只有等当前运行任务主动释放CPU的使用权后才能获得运行，这样可提高CPU的运行效率。
 - 在嵌入式领域，FreeRTOS是不多的同时具有实时性，开源性，可靠性，易用性，多平台支持等特点的嵌入式操作系统。目前，FreeRTOS已经发展到支持包含X86，Xilinx，Altera等多达30种的硬件平台，其广阔的应用前景已经越来越受到业内人士的瞩目。


FreeRTOS SMP架构
------------------------

SMP简介
++++++++++++++++++++++++

 - Armino平台FreeRTOS SMP架构使用的是内核代码路径: components/os_source/freertos_smp
 - 当涉及到多处理器系统（Multiprocessor Systems）时，Symmetric Multiprocessing（SMP）是一种常见的架构。
 - BK7236XX 支持CM33双核FreeRTOS SMP架构
 - 以下是对SMP的基本概念、在FreeRTOS中实现SMP的目的和优势的简要介绍：

Symmetric Multiprocessing（SMP）的基本概念：
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1. **多处理器系统（Multiprocessor Systems）**：
   - 多处理器系统是由多个处理器核心组成的计算机系统。每个处理器核心都能够独立执行任务和运行程序。

2. **对称性（Symmetric）**：
   - SMP系统中，所有处理器核心都具有对称的地位，即它们都能够执行相同的任务和访问相同的系统资源。

3. **共享内存（Shared Memory）**：
   - SMP系统中的处理器核心通过共享相同的内存空间来进行通信。这使得数据共享更加容易，但也需要采取措施来防止竞态条件等问题。

在FreeRTOS中实现SMP的目的和优势：
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1. **提高性能**：
   - SMP允许多个处理器核心同时执行任务，从而提高系统整体性能。特别是在处理大量并行任务时，SMP能够有效地分担负载，加速系统的响应速度。

2. **任务并行执行**：
   - 在SMP系统中，不同的处理器核心可以独立执行不同的任务。这种并行性有助于提高系统的吞吐量和响应能力。

3. **更好的资源利用**：
   - SMP允许系统在不同的处理器核心上同时执行任务，有效地利用了硬件资源。这对于处理实时任务和对性能要求较高的应用程序非常重要。

4. **系统灵活性**：
   - SMP架构使系统更具灵活性，可以根据需要扩展处理能力。增加处理器核心的数量可以简化系统的升级和维护过程。

5. **实时性能**：
   - 对于实时操作系统如FreeRTOS，SMP的实现可以提供更好的实时性能，确保任务在规定的时间内得到执行，从而满足实时系统的要求。

在FreeRTOS中实现SMP需要考虑处理器间同步、共享资源管理和任务调度等方面的挑战，但通过正确的实现，可以显著提高系统的性能和响应能力。

在FreeRTOS SMP架构中资源同步：
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.. figure:: ../../_static/smp_resource.png
    :align: center
    :alt: SMP Architecture Resource
    :figclass: align-center


.. figure:: ../../_static/smp_resource_sync.png
    :align: center
    :alt: SMP Architecture Resource Synchronization
    :figclass: align-center

    SMP 架构共享资源同步

1. **Task间共享资源同步**：
   
    - 操作系统的信号量(Semaphore)
    - 操作系统的同步锁(Mutex)
    - 关调度

2. **ISR间共享资源同步**：

    - 自旋锁(SpinLock)

3. **Task/ISR间共享资源同步**：

    - 自旋锁(SpinLock)


在FreeRTOS SMP架构中API介绍：
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 - 创建smp task到指定cpu::

    bk_err_t rtos_create_thread_with_affinity( beken_thread_t *thread,
                                                uint32_t affinity,
                                                uint8_t priority,
                                                const char* name,
                                                beken_thread_function_t function,
                                                uint32_t stack_size,
                                                beken_thread_arg_t arg );

 - 创建smp task到cpu0/cpu1::

    bk_err_t rtos_smp_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

 - 创建task指定到cpu0::

    bk_err_t rtos_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

    bk_err_t rtos_core0_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

 - 创建task指定到cpu1::

    bk_err_t rtos_core1_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

 - 同步信号量::

    bk_err_t rtos_init_semaphore( beken_semaphore_t* semaphore, int maxCount );
    bk_err_t rtos_init_semaphore_ex( beken_semaphore_t* semaphore, int maxCount, int init_count);
    bk_err_t rtos_get_semaphore( beken_semaphore_t* semaphore, uint32_t timeout_ms );
    int rtos_get_semaphore_count( beken_semaphore_t* semaphore );
    int rtos_set_semaphore( beken_semaphore_t* semaphore );
    bk_err_t rtos_deinit_semaphore( beken_semaphore_t* semaphore );

 - 同步锁::

    bk_err_t rtos_init_mutex( beken_mutex_t* mutex );
    bk_err_t rtos_trylock_mutex(beken_mutex_t *mutex);
    bk_err_t rtos_lock_mutex( beken_mutex_t* mutex );
    bk_err_t rtos_unlock_mutex( beken_mutex_t* mutex );
    bk_err_t rtos_deinit_mutex( beken_mutex_t* mutex );


 - 自旋锁API及Demo::

     uint32_t rtos_enter_critical( void );
     void rtos_exit_critical( uint32_t state );

     uint32_t state = rtos_enter_critical();
     // Critical section: access to shared resources
     rtos_exit_critical(state);



在FreeRTOS SMP架构工程配置：
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 - BK7236 默认app工程是AMP架构
 - BK723L 默认app工程是双核FreeRTOS SMP架构, 工程配置如下::

    CONFIG_CPU_CNT=2
    CONFIG_SOC_SMP=y
    CONFIG_SOC_BK7236_SMP_TEMP=y

    CONFIG_FREERTOS_SMP=y
    CONFIG_FREERTOS_SMP_TEMP=y
    CONFIG_FREERTOS_USE_TICKLESS_IDLE=0

    CONFIG_OS_SMP_AFFINITY_DEMO=y

    CONFIG_CPU0_SPE_RAM_SIZE=0xA0000

    CONFIG_CACHE_ENABLE=n

