使用外部32K
=======================

:link_to_translation:`en:[English]`

开启外部32K时钟源方法
-----------------------------------------------

-  确保硬件已经焊接了外部32K时钟源
-  在middleware/soc/bk72xx/bk7258.defconfig中设置CONFIG_EXTERN_32K=y

.. important::
    当本地工程应用于其他项目时，为避免定义外部32K影响该工程，请新建工程，将CONFIG_EXTERN_32K定义在根目录下projects的新建工程的config/bk72xx.config中。

