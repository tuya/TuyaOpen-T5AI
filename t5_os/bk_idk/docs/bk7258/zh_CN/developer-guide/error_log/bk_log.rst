系统日志
===================

:link_to_translation:`en:[English]`

本文档主要介绍Armino平台BK7258三核log机制

 - BK7258 CPU0的log通过串口DL_UART0输出(默认波特率为115200)
 - BK7258 CPU1的log通过mailbox转发到CPU0串口DL_UART0输出
 - BK7258 CPU2的log通过串口UART2输出(默认波特率为115200)
 - CPU1 log带cpu1标签（异常log除外）
 - Armino平台默认采用异步Log输出机制，缓存缓存分为静态缓存和动态缓存，输出Log时优先使用静态缓存，当静态缓存不足时，使用动态缓存。
 - 当静态缓存和动态缓存均耗尽时，采用阻塞机制，若处于不可阻塞状态下，缓存耗尽时，将使Log直接输出至串口，此时Log带有前缀"INSRT:"，此Log会插入在其他Log中间，并影响此Log输出时序。
 - 通过串口输入log命令查看当前log配置
 - log 1 3 0 命令第一个参数为echo开关 (0/1)，第二个参数为log级别(0~6)，第三个参数为同步开关(0异步,1同步)

BK7258 多核log机制
------------------------

.. figure:: ../../../_static/log_system.png
    :align: center
    :alt: 多核log机制
    :figclass: align-center

    多核log机制


API说明
------------------------

 - Shell 模块支持log 等级机制，支持APP模块log输出的控制机制。
 - APP模块log的输出控制，主要用在电脑的显示界面上，而不是在程序内部。

API 中支持log等级，模块名字等参数。

 - Log的等级目前设计了6级（0~5），每级的含义如下所述，系统配置的log输出等级为0时，关闭所有log的输出。
 - 系统配置的log输出等级为N时，可以输出1~N 等级的log，(N+1)~5 等级的log 不能输出。
 - Log的等级定义如下：

::

    #define BK_LOG_NONE       0      /*!< No log output */
    #define BK_LOG_ERROR      1      /*!< Critical errors, software module can not recover on its own */
    #define BK_LOG_WARN       2      /*!< Error conditions from which recovery measures have been taken */
    #define BK_LOG_INFO       3      /*!< Information messages which describe normal flow of events */
    #define BK_LOG_DEBUG      4      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    #define BK_LOG_VERBOSE    5      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    #define LOG_LEVEL         BK_LOG_INFO    /* 配置系统的log输出等级 */


如下是经过封装后的Log 输出API：
    - Tag：表示输出log的模块名。
    - format：是log输出的格式字符串。

::

    BK_LOGE( tag, format, ... );       // Tag所指的模块输出 ERROR 等级的log
    BK_LOGW( tag, format, ... );       // Tag所指的模块输出 Warning 等级的log
    BK_LOGI( tag, format, ... );       // Tag所指的模块输出 Information 等级的log
    BK_LOGD( tag, format, ... );       // Tag所指的模块输出 Debug等级的log
    BK_LOGV( tag, format, ... );       // Tag所指的模块输出 一些不重要的log
    BK_ASSERT(exp);                    // 如果 exp表达式为假，关闭系统中断，shell会输出当前的函数名，当前的行号，当前的系统时间等信息。然后输出系统的dump数据（CPU寄存器，内存，栈等信息）
    BK_ASSERT_EX(exp, format, ... );  // 如果 exp表达式为假，关闭系统中断，shell会输出当前的函数名，当前的行号，当前的系统时间，format格式串的相关变量，等信息。然后输出系统的dump数据（CPU寄存器，内存，栈等信息）
    BK_DUMP_OUT(format, ... );         // 关中断下，用format格式的方式输出相关信息。
    BK_DUMP_RAW_OUT(buf, len);         // 关中断下，输出buf 中len长度的数据。


Log缓存大小配置
------------------------
当前Log采用异步Log机制，为了防止Log丢失并加入阻塞机制和动态缓存机制。静态缓存和动态缓存的大小均可配置。配置方法如下：

    静态Log缓存配置
        静态Log缓存有40、80和136字节三种缓存。cpu0和cpu2在release模式下，三种静态缓存数量分别为4、16、32，在debug模式下分别为8、40、60。cpu1的三种静态缓存分别为2、4、8。
        静态Log缓存数量可在 ``shell_task.c`` 中配置，分别通过宏定义 ``SHELL_LOG_BUF3_NUM`` 、``SHELL_LOG_BUF2_NUM`` 和 ``SHELL_LOG_BUF1_NUM`` 进行配置。
        
    动态Log缓存配置
        可配置Log动态缓存内存大小限制和数量上限。
        可在当前project下相应soc目录下的config文件中添加配置 ``CONFIG_DYM_LOG_MEM_MAX`` ，来修改相应的Log动态内存使用上限，如 ``CONFIG_DYM_LOG_MEM_MAX=0x4000`` 设置Log动态内存上限设置为16K。
        动态Log缓存数量可在 ``shell_task.c`` 中配置，通过宏定义 ``SHELL_DYM_LOG_NUM_MAX`` 进行配置动态Log使用上限。
