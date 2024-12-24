FreeRTOS
===============================

:link_to_translation:`zh_CN:[中文]`

Introduction to FreeRTOS
--------------------------

  - FreeRTOS is a mini real-time operating system kernel.
  - As a lightweight operating system, its functions include: task management, time management, semaphores, message queues, memory management, recording functions, software timers, coroutines, etc., which can basically meet the needs of smaller systems.
  - FreeRTOS can run on small RAM microcontrollers. The FreeRTOS operating system is a completely free open source operating system. It has the characteristics of open source code, portability, reduction, and flexible scheduling strategy. It can be easily transplanted to run on various microcontrollers.
  - The Armino platform currently uses the version released in July 2021, a Release version slightly newer than the 10.4 LTS version (FreeRTOSv202107.00), version number: FreeRTOS Kernel V10.4.4

FreeRTOS general architecture
---------------------------------

.. figure:: ../../_static/freertos_arch.png
     :align: center
     :alt: FreeRTOS Architecture
     :figclass: align-center

     FreeRTOS architecture

- A FreeRTOS system is mainly composed of BSP driver + kernel + components (as shown in the picture above). The kernel includes functions of multi-task scheduling, memory management, and inter-task communication, and components include network protocols, peripheral support, etc.
- The FreeRTOS kernel is tailorable and components are optional. Since embedded applications often have very strict requirements on memory space, a tailorable RTOS is very important for embedded applications. This makes the core code of FreeRTOS only about 9,000 lines.


Functions and features
--------------------------

  - User configurable kernel features
  - Multi-platform support
  - Provides a high level of trust in code integrity
  - The target code is small, simple and easy to use
  - Follow the programming specifications of MISRA-C standard
  - Powerful execution tracking function
  - Stack overflow detection
  - Unlimited number of tasks
  - Unlimited task priority
  - Multiple tasks can be assigned the same priority
  - Queues, binary semaphores, counting semaphores and recursive communication and synchronization tasks
  - Priority inheritance
  - Free and open source source code


FreeRTOS Summary
--------------------------

  - As a lightweight operating system, FreeRTOS provides functions including: task management, time management, semaphores, message queues, memory management, recording functions, etc., which can basically meet the needs of smaller systems. The FreeRTOS kernel supports a priority scheduling algorithm. Each task can be assigned a certain priority based on its importance. The CPU always allows the task in the ready state with the highest priority to run first. The FreeRTOS kernel also supports a rotation scheduling algorithm. The system allows different tasks to use the same priority. When there is no higher-priority task ready, tasks of the same priority share the CPU usage time.
  - The kernel of FreeRTOS can be set as a deprivable kernel or a non-deprivable kernel according to user needs. When FreeRTOS is set as a preemptive kernel, high-priority tasks in the ready state can deprive low-priority tasks of CPU usage rights, which ensures that the system meets real-time requirements; when FreeRTOS is set as a non-preemptive kernel , high-priority tasks in the ready state can only be run after the currently running task actively releases the right to use the CPU, which can improve the operating efficiency of the CPU.
  - In the embedded field, FreeRTOS is one of the few embedded operating systems that has the characteristics of real-time, open source, reliability, ease of use, and multi-platform support. At present, FreeRTOS has developed to support up to 30 hardware platforms including X86, Xilinx, Altera, etc., and its broad application prospects have attracted more and more attention from industry insiders.




FreeRTOS SMP architecture
--------------------------

Introduction to SMP
++++++++++++++++++++++++

  - The Armino platform FreeRTOS SMP architecture uses the kernel code path: components/os_source/freertos_smp
  - Symmetric Multiprocessing (SMP) is a common architecture when it comes to multiprocessor systems.
  - BK7236XX supports CM33 dual-core FreeRTOS SMP architecture
  - The following is a brief introduction to the basic concepts of SMP, the purpose and advantages of implementing SMP in FreeRTOS:

Basic concepts of Symmetric Multiprocessing (SMP):
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1. **Multiprocessor Systems**:
    - A multiprocessor system is a computer system composed of multiple processor cores. Each processor core is capable of executing tasks and running programs independently.

2. **Symmetric**:
    - In an SMP system, all processor cores have a symmetrical status, that is, they are able to perform the same tasks and access the same system resources.

3. **Shared Memory**:
    - Processor cores in an SMP system communicate by sharing the same memory space. This makes data sharing easier, but also requires measures to prevent issues such as race conditions.

The purpose and advantages of implementing SMP in FreeRTOS:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1. **Improve performance**:
    - SMP allows multiple processor cores to perform tasks simultaneously, thereby improving overall system performance. Especially when processing a large number of parallel tasks, SMP can effectively share the load and accelerate the response speed of the system.

2. **Parallel execution of tasks**:
    - In an SMP system, different processor cores can perform different tasks independently. This parallelism helps improve the throughput and responsiveness of the system.

3. **Better Resource Utilization**:
    - SMP allows the system to execute tasks simultaneously on different processor cores, effectively utilizing hardware resources. This is important for handling real-time tasks and applications with high performance requirements.

4. **System Flexibility**:
    - SMP architecture makes the system more flexible and can expand processing capabilities as needed. Increasing the number of processor cores simplifies system upgrades and maintenance.

5. **Real-time performance**:
    - For real-time operating systems such as FreeRTOS, the implementation of SMP can provide better real-time performance and ensure that tasks are executed within the specified time, thereby meeting the requirements of real-time systems.

Implementing SMP in FreeRTOS requires consideration of challenges in inter-processor synchronization, shared resource management, and task scheduling, but with the correct implementation, the performance and responsiveness of the system can be significantly improved.

Resource synchronization in FreeRTOS SMP architecture:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. figure:: ../../_static/smp_resource.png
     :align: center
     :alt: SMP Architecture Resource
     :figclass: align-center


.. figure:: ../../_static/smp_resource_sync.png
     :align: center
     :alt: SMP Architecture Resource Synchronization
     :figclass: align-center

     SMP architecture shared resource synchronization

1. **Shared resource synchronization between Tasks**:
   
     - Operating system semaphore (Semaphore)
     - Operating system synchronization lock (Mutex)
     - Off scheduling

2. **Shared resource synchronization between ISRs**:

     - SpinLock

3. **Shared resource synchronization between Task/ISR**:

     - SpinLock


API introduction in FreeRTOS SMP architecture:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  - Create smp task to specified cpu::

     bk_err_t rtos_create_thread_with_affinity( beken_thread_t *thread,
                                                 uint32_t affinity,
                                                 uint8_t priority,
                                                 const char* name,
                                                 beken_thread_function_t function,
                                                 uint32_t stack_size,
                                                 beken_thread_arg_t arg );

  - Create smp task to cpu0/cpu1::

     bk_err_t rtos_smp_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                         beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

  - Create task assigned to cpu0::

     bk_err_t rtos_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                         beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

     bk_err_t rtos_core0_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                         beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

  - Create task assigned to cpu1::

     bk_err_t rtos_core1_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                         beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg );

  - Synchronization semaphore::

     bk_err_t rtos_init_semaphore( beken_semaphore_t* semaphore, int maxCount );
     bk_err_t rtos_init_semaphore_ex( beken_semaphore_t* semaphore, int maxCount, int init_count);
     bk_err_t rtos_get_semaphore( beken_semaphore_t* semaphore, uint32_t timeout_ms );
     int rtos_get_semaphore_count( beken_semaphore_t* semaphore );
     int rtos_set_semaphore( beken_semaphore_t* semaphore );
     bk_err_t rtos_deinit_semaphore( beken_semaphore_t* semaphore );

  - Sync Lock::

     bk_err_t rtos_init_mutex( beken_mutex_t* mutex );
     bk_err_t rtos_trylock_mutex(beken_mutex_t *mutex);
     bk_err_t rtos_lock_mutex( beken_mutex_t* mutex );
     bk_err_t rtos_unlock_mutex( beken_mutex_t* mutex );
     bk_err_t rtos_deinit_mutex( beken_mutex_t* mutex );


  - Spin lock API and Demo::

     uint32_t rtos_enter_critical( void );
     void rtos_exit_critical( uint32_t state );

     uint32_t state = rtos_enter_critical();
     // Critical section: access to shared resources
     rtos_exit_critical(state);



In FreeRTOS SMP architecture project configuration:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  - The default app project of BK7236 is AMP architecture
  - The default app project of BK723L is dual-core FreeRTOS SMP architecture, and the project configuration is as follows::

     CONFIG_CPU_CNT=2
     CONFIG_SOC_SMP=y
     CONFIG_SOC_BK7236_SMP_TEMP=y

     CONFIG_FREERTOS_SMP=y
     CONFIG_FREERTOS_SMP_TEMP=y
     CONFIG_FREERTOS_USE_TICKLESS_IDLE=0

     CONFIG_OS_SMP_AFFINITY_DEMO=y

     CONFIG_CPU0_SPE_RAM_SIZE=0xA0000

     CONFIG_CACHE_ENABLE=n
