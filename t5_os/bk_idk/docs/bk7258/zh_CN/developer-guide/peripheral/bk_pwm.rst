PWM (TBC)
==========

:link_to_translation:`en:[English]`


PWM 介绍
------------------
脉冲宽度调制（Pulse width modulation，简称 PWM）是一种用数字方式实现模拟电压控制的技术。它是通过对一系列脉冲的宽度进行调制，等效出所需要的波形（包含形状以及幅值），对模拟信号电平进行数字编码，也就是说通过调节占空比的变化来调节信号、能量等的变化，占空比就是指在一个周期内，信号处于高电平的时间占据整个信号周期的百分比，例如方波的占空比就是50%。



PWM API 基本使用
------------------

.. figure:: ../../../_static/pwm_out_start_stop.png
    :align: center
    :alt: PWM start stop 
    :figclass: align-center

    PWM Start/Stop