# RTC 驱动

# 简要说明

​	实时时钟（RTC）是专用于维持一秒时基的计时器。此外，RTC通常用于在软件或硬件中跟踪时钟时间和日历日期。RTC的许多功能是非常专业的，是维持高精度和非常可靠的操作所必需的。

​	一般的SOC内部自带RTC硬件单元，可以直接操作寄存器去设置读取RTC时间。部分的SOC也可以也可以通过IIC SPI接口外扩RTC的外设

# Api描述

### 1.tkl_rtc_init

```
OPERATE_RET tkl_rtc_init(void);
```

- 功能描述:
  - 初始化rtc，返回初始化结果 。
  
- 参数:
  - void 
  
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h定义部分。

### 2.tkl_rtc_deinit

```
OPERATE_RET tkl_rtc_deinit(void);
```

- 功能描述:
  - rtc反初始化，停止rtc
- 参数:
  - void
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h定义部分。

### 3.tkl_rtc_time_set

```
OPERATE_RET tkl_rtc_time_set(TIME_T time_sec);
```

- 功能描述:

  - 设置rtc的时间

- 参数:

  - time_sec: 一个UTC时间。

  
```c
  typedef unsigned int TIME_T;
```
  
- 返回值:

  - OPRT_OK 成功，其他请参考文件tuya_error_code.h定义部分。

### 4.tkl_rtc_time_get

```
OPERATE_RET tkl_rtc_time_get(TIME_T *time_sec);
```

- 功能描述:
  
  - 获取rtc的时间
- 参数:
  
  - time_sec：utc时间
  
    ```c
    typedef unsigned int TIME_T;
    ```
- 返回值:
  
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h定义部分。

# 示例

## 1.RTC示例一

```c
/*初始化RTC*/
tkl_rtc_init(void);

/*设置RTC时间*/
TIME_T time_sec_set = 0x1000000;
tkl_rtc_time_set(&time_sec_set);

/*获取RTC时间*/
TIME_T time_sec_get
tkl_rtc_time_get(&time_sec_get);

/*反初始化RTC*/
tkl_rtc_deinit(void);

```

