AB分区位置无关的升级流程介绍
---------------------------------

:link_to_translation:`en:[English]`

  AB分区位置无关的升级流程分为三部分：升级镜像下载流程；Bootloader确认升级镜像完整性流程；APP确认最终运行分区(有的客户需求，最终执行哪个分区由他们决定，由宏控制，如果客户不反馈，执行默认跳转分区）。升级的过程中涉及三个flag标志位：均保存在flash某个4K空间内（当前位置在0x3FD000）。

.. important::
    - flag1：ota_temp_exec_flag: 3—A； 4--B (temp update)。
    - flag2：cust_confirm_flag : 1--代表进行ota并下载成功；3—A； 4--B (代表没有进行进行ota且app中最终决定执行哪个分区)。
    - flag3：ota_exec_flag :  0/0xFF—A；1—B。

ab分区升级流程用到的三个标志位具体解释如下：

1）ota_temp_exec_flag ：升级镜像下载完成后，客户临时决定跳转哪个分区；

.. note::
    - 3：代表客户临时决定跳转分区A；
    - 4：代表客户临时决定跳转分区B；（该标志位根据宏控制，如果客户不反馈，执行默认标志位），然后数值会做转换，3转换为0（A分区），4转换为1（B分区），会将0/1写入flash。

2）cust_confirm_flag ：有两层含义a) 升级镜像下载完成的标志；b)重启之后客户最终决定执行哪个分区。

.. note::
    - 1：代表下载成功（只有ota下载成功的时候会置为1 ），然后写入flash；
    - 3：代表执行分区A；
    - 4：代表执行分区B；（该标志位根据宏控制，如果客户不反馈，执行默认标志位），代码里最终会转为设置ota_exec_flag为0/1，然后写入flash。

3）ota_exec_flag ：升级镜像下载之前，根据当前代码获知当前要升级哪个分区；

.. note::
    - 0：代表升级分区A；（0xFF：代表第一次烧录运行在分区A）
    - 1：代表升级分区B；（如果下载流程及验签成功，reboot之后，就会执行在该分区上，和cust_confirm_flag代表的默认分区应该一致），然后写入flash。


AB分区的升级镜像的下载流程如图二所示：

  .. figure:: ../../../../_static/ab_app.png
     :align: center
     :alt: ab_app
     :figclass: align-center

     图2 镜像下载流程

.. note::
    图2主要实现升级镜像的下载功能，以及设置对应的flag，flag标志位组合如上面红色备注所示。

AB分区的跳转流程如图三所示。

  .. figure:: ../../../../_static/ab_bootloader.png
     :align: center
     :alt: ab_bootloader
     :figclass: align-center

     图3 bootloader跳转流程

.. note::
    - 1.图3根据图2中的flag，配合hash验证，确定跳转哪个分区；
    - 2.跳转之前，先设置flash相关寄存器（A区起始地址和长度、B区相对A区的offset） 以及跳转地址均为A区起始地址；
    - 3.直接跳转A区：此过程中会先设置flash相关控制寄存器的bit[0] = 0;
    - 4.直接跳转B区：此过程中会先设置flash相关控制寄存器的bit[0] = 1;
