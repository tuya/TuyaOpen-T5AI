set(incs)
set(srcs)

if (CONFIG_OS_THREAD_DEMO)
	list(APPEND srcs os_thread/os_thread.c)
	list(APPEND incs os_thread)
endif()

if (CONFIG_OS_QUEUE_DEMO)
	list(APPEND srcs os_queue/os_queue.c)
	list(APPEND incs os_queue)
endif()

if (CONFIG_OS_SEM_DEMO)
	list(APPEND srcs os_sem/os_sem.c)
	list(APPEND incs os_sem)
endif()

if (CONFIG_OS_MUTEX_DEMO)
	list(APPEND srcs os_mutex/os_mutex.c)
	list(APPEND incs os_mutex)
endif()

if (CONFIG_OS_TIMER_DEMO)
	list(APPEND srcs os_timer/os_timer.c)
	list(APPEND incs os_timer)
endif()

if (CONFIG_OS_SMP_BLINKY_DEMO)
	list(APPEND srcs smp/blinky/main_blinky.c)
	list(APPEND incs smp/)
endif()

if (CONFIG_OS_SMP_AFFINITY_DEMO)
	list(APPEND srcs smp/core_affinity/affinity.c)
	list(APPEND incs smp/)
endif()

if (CONFIG_OS_SMP_INHERIT_DEMO)
	list(APPEND srcs smp/inherit/priority_inversion.c)
	list(APPEND incs smp/)
endif()

if (CONFIG_OS_SMP_DEEP_LV_DEMO)
	list(APPEND srcs 
		smp/deep_lv/deep_lv.c
		smp/deep_lv/scene.c)
	list(APPEND incs smp/)
endif()

armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common)
