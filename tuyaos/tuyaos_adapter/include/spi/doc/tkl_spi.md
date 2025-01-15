# SPI 驱动

# 简要说明

SPI（Serial Peripheral Interface）是一种高速、全双工、同步的通信总线。SPI以主从方式工作，通常有一个主设备和一个或多个从设备。

SPI 控制器的信号线描述如下：

- MISO：主设备数据输入，从设备数据输出；
- MOSI：主设备数据输出，从设备数据输入；
- SCK： 时钟信号，由主设备产生；
- CS：从设备使能信号，由主设备控制。这个信号可以是SPI外设的一部分，也可用GPIO引脚实现。

SPI 典型接线方式如下：

![image-20220411140500420](https://images.tuyacn.com/fe-static/docs/img/fd2d8ee2-cef2-44a0-ae58-7039833d1e0c.png)

SPI总线支持的四种工作方式，取决于串行同步时钟极性CPOL和串行同步时钟相位CPHA的组合。

CPOL是用来决定SCLK时钟信号空闲时的电平，CPOL=0，空闲电平为低电平，CPOL=1时，空闲电平为高电平。CPHA是用来决定采样时刻的，CPHA=0，在每个周期的第一个时钟沿采样，第二个时钟沿数据输出；CPHA=1，在每个周期的第二个时钟沿采样，第一个时钟沿数据输出。SPI主模块和与之通信的外设时钟相位和极性应该一致。

四种工作方式时序描述如下：

模式0：CPOL= 0，CPHA=0。SCK串行时钟线空闲是为低电平，数据在SCK时钟的上升沿被采样，数据在SCK时钟的下降沿切换

模式1：CPOL= 0，CPHA=1。SCK串行时钟线空闲是为低电平，数据在SCK时钟的下降沿被采样，数据在SCK时钟的上升沿切换

模式2：CPOL= 1，CPHA=0。SCK串行时钟线空闲是为高电平，数据在SCK时钟的下降沿被采样，数据在SCK时钟的上升沿切换

模式3：CPOL= 1，CPHA=1。SCK串行时钟线空闲是为高电平，数据在SCK时钟的上升沿被采样，数据在SCK时钟的下降沿切换

# Api描述

### tkl_spi_init

```
OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, const TUYA_SPI_BASE_CFG_T *cfg);
```

- 功能描述:

  - 通过端口号和基础配置初始化对应的spi实例，返回初始化结果 。

- 参数:

  - `port`: 端口号。
  - `cfg`: spi基础配置 。

  ```c
  typedef struct {
      TUYA_SPI_ROLE_E      role;
      TUYA_SPI_MODE_E      mode;
      TUYA_SPI_TYPE_E      type;
      TUYA_SPI_DATABITS_E  databits;
      TUYA_SPI_BIT_ORDER_E bitorder;
      uint32_t               freq_hz;
      uint32_t               spi_dma_flags; /*!< SPI dma format , 1 use dma */
  } TUYA_SPI_BASE_CFG_T;
  ```

  #### TUYA_SPI_ROLE_E:

  | 名字                         | 定义            | 备注 |
  | :--------------------------- | :-------------- | :--- |
  | TUYA_SPI_ROLE_INACTIVE       | spi闲置         |      |
  | TUYA_SPI_ROLE_MASTER         | spi全双工主模式 |      |
  | TUYA_SPI_ROLE_SLAVE          | spi全双工从模式 |      |
  | TUYA_SPI_ROLE_MASTER_SIMPLEX | spi半双工主模式 |      |
  | TUYA_SPI_ROLE_SLAVE_SIMPLEX  | spi半双工从模式 |      |

  #### TUYA_SPI_MODE_E:

  | 名字           | 定义               | 备注 |
  | :------------- | :----------------- | :--- |
  | TUYA_SPI_MODE0 | CPOL = 0, CPHA = 0 |      |
  | TUYA_SPI_MODE1 | CPOL = 0, CPHA = 1 |      |
  | TUYA_SPI_MODE2 | CPOL = 1, CPHA = 0 |      |
  | TUYA_SPI_MODE3 | CPOL = 1, CPHA = 1 |      |

  #### TUYA_SPI_TYPE_E:

  | 名字                        | 定义                     | 备注                             |
  | --------------------------- | :----------------------- | :------------------------------- |
  | TUYA_SPI_AUTO_TYPE          | SS管脚模式，硬件自动配置 | SS：slave select，对应CS片选管脚 |
  | TUYA_SPI_SOFT_TYPE          | SS管脚模式，软件手动配置 |                                  |
  | TUYA_SPI_SOFT_ONE_WIRE_TYPE | 三线模式，SS管脚无效     |                                  |

  #### TUYA_SPI_DATABITS_E：

  | 名字                | 定义           | 备注 |
  | ------------------- | :------------- | :--- |
  | TUYA_SPI_DATA_BIT8  | 8位数据位模式  |      |
  | TUYA_SPI_DATA_BIT16 | 16位数据位模式 |      |

  #### TUYA_SPI_BIT_ORDER_E:

  | 名字                   | 定义                             | 备注 |
  | ---------------------- | :------------------------------- | :--- |
  | TUYA_SPI_ORDER_MSB2LSB | 高位（MSB）在前，低位（LSB）在后 |      |
  | TUYA_SPI_ORDER_LSB2MSB | 低位（LSB）在前，高位（MSB）在后 |      |

- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_deinit

```
OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port);
```

- 功能描述:
  - SPI实例反初始化。该接口会停止SPI
  - 实例正在进行的传输（如果有），并且释放相关的软硬件资源。
- 参数:
  - `port`: 端口号。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_send

```
OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, void *data, uint16_t size);
```

- 功能描述:
  - SPI启动数据发送。
- 参数:
  - `port`: 端口号。
  - `data`: 待发送数据的缓冲区地址。
  - `size`: 待发送数据的长度。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_recv

```
OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, void *data, uint16_t size);
```

- 功能描述:
  - SPI启动数据接收。
- 参数:
  - `port`: 端口号。
  - `data`: 待接收数据的缓冲区地址。
  - `size`: 待接收数据的长度。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_transfer

```
OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, void* send_buf, void* receive_buf, uint32_t length);
```

- 功能描述:
  - SPI启动数据传输。
- 参数:
  - `port`: 端口号。
  - `send_buf`: 待发送数据的缓冲区地址。
  - `receive_buf`: 待接收数据的缓冲区地址。
  - length：长度。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_abort_transfer

```
OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port);
```

- 功能描述:
  - SPI终止数据传输，或者终止数据发送(接收)。
- 参数:
  - `port`: 端口号。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_get_status

```
OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status); 
```

- 功能描述:
  - 获取当前时刻SPI的状态。
- 参数:
  - `port`:端口号。
- 返回值:
  - SPI状态的结构体，spi的状态定义见TUYA_SPI_STATUS_T的定义。

#### TUYA_SPI_STATUS_T:

| 名字           | 定义              | 备注  |
| :------------- | :---------------- | :---- |
| busy : 1       | 传输/收发忙状态位 | 1有效 |
| data_lost : 1  | 数据丢失          | 1有效 |
| mode_fault : 1 | 模式错误          | 1有效 |

### tkl_spi_irq_init

```
OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb);
```

- 功能描述:
  - SPI的中断初始化。
- 参数:
  - `port`:端口号。
  - `cb`:中断回调函数。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_irq_enable

```
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port);
```

- 功能描述:
  - 使能SPI的中断。
- 参数:
  - `port`:端口号。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_irq_disable

```
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port);
```

- 功能描述:
  - 关闭SPI的中断。
- 参数:
  - `port`:端口号。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h，OS_ADAPTER_SPI定义部分。

### tkl_spi_get_data_count

```
int32_t tkl_spi_get_data_count(TUYA_SPI_NUM_E port);
```

- 功能描述:
  - 获取spi传输字节长度。
- 参数:
  - `port`:端口号。
- 返回值:
  - <0,err。>= 0,上次传输的字节长度。可以是tkl_spi_send, tkl_spi_recv 和tkl_spi_transfer任一个的操作。

# 示例

## 1.spi示例一

```c
void tuya_spi_test1(void)
{
    OPERATE_RET ret;
    TUYA_SPI_BASE_CFG_T cfg;
    TUYA_SPI_STATUS_T status；
    //receive buffer
    char rcv_buf[8];
    //data to send
    char send_buf[8] = {0,1,2,3,4,5,6,7};
    
    tkl_io_pinmux_config(TUYA_IO_PIN_0, TUYA_SPI0_MISO);
    tkl_io_pinmux_config(TUYA_IO_PIN_1, TUYA_SPI0_MOSI);
    tkl_io_pinmux_config(TUYA_IO_PIN_2, TUYA_SPI0_CS);
    tkl_io_pinmux_config(TUYA_IO_PIN_3, TUYA_SPI0_CLK);
    
    cfg.role = TUYA_SPI_ROLE_MASTER;
    cfg.mode = TUYA_SPI_MODE0;
    cfg.type = TUYA_SPI_AUTO_TYPE;
    cfg.databits = TUYA_SPI_DATA_BIT8；
    cfg.bitorder = TUYA_SPI_ORDER_MSB2LSB；
    cfg.freq_hz = 1000000；
    
    ret = tkl_spi_init(TUYA_SPI_NUM_0, &cfg);
    if (ret != OPRT_OK) {
        //fail
        return;
    }
    
    ret = tkl_spi_send(TUYA_SPI_NUM_0, send_buf, 8);
    if (ret < 0) {
        //failed
    }
    
    tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
    while (status.busy) {
        tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
        tkl_system_sleep(2);
    }
    
    ret = tkl_spi_recv(TUYA_SPI_NUM_0, rcv_buf, 8);
    if (ret < 0) {
        //failed
    }
    
    tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
    while (status.busy) {
        tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
        tkl_system_sleep(2);
    }
    ret = tkl_spi_transfer(TUYA_SPI_NUM_0, send_buf,rcv_buf, 6);
    if (ret < 0) {
        //failed
    }
    
    tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
    while (status.busy) {
        tkl_spi_get_status(TUYA_SPI_NUM_0, &status);
        tkl_system_sleep(2);
    }
    //uninitialize iic 
    ret = tkl_spi_deinit(TUYA_SPI_NUM_0);
    if (ret != 0) {
       //failed
    }
}
```

## 2.spi使用中断示例二

```
int event_flag = -1;
static void spi_event_cb(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_EVT_E event)
{
    //printf("\nspi_event_cb_fun:%d\n",event);
    event_flag = event；
}

void tuya_spi_test2(void)
{
    OPERATE_RET ret;
    TUYA_SPI_BASE_CFG_T cfg;
    TUYA_SPI_STATUS_T status；
    //receive buffer
    char rcv_buf[6];
    //data to send
    char send_buf[6] = {0x90,0x0,0x0,0x0,0x0,0x0};
    
    tkl_io_pinmux_config(TUYA_IO_PIN_0, TUYA_SPI0_MISO);
    tkl_io_pinmux_config(TUYA_IO_PIN_1, TUYA_SPI0_MOSI);
    tkl_io_pinmux_config(TUYA_IO_PIN_2, TUYA_SPI0_CS);
    tkl_io_pinmux_config(TUYA_IO_PIN_3, TUYA_SPI0_CLK);
    
    cfg.role = TUYA_SPI_ROLE_MASTER;
    cfg.mode = TUYA_SPI_MODE0;
    cfg.type = TUYA_SPI_AUTO_TYPE;
    cfg.databits = TUYA_SPI_DATA_BIT8；
    cfg.bitorder = TUYA_SPI_ORDER_MSB2LSB；
    cfg.freq_hz = 1000000；
    
    ret = tkl_spi_init(TUYA_SPI_NUM_0, &cfg);
    if (ret != OPRT_OK) {
        //fail
        return;
    }
    
    tkl_spi_irq_init(TUYA_SPI_NUM_0 , spi_event_cb);
    tkl_spi_irq_enable(TUYA_SPI_NUM_0);
    
    event_flag = -1;
    ret = tkl_spi_transfer(TUYA_SPI_NUM_0, send_buf,rcv_buf, 6);
    if (ret < 0) {
        //failed
    }
    
    while (TUYA_SPI_EVENT_TRANSFER_COMPLETE != event_flag) {
        tkl_system_sleep(2);
    }
    //up Transfer Complete
    event_flag = -1;
    ret = tkl_spi_send(TUYA_SPI_NUM_0, send_buf, 6);
    if (ret < 0) {
        //failed
    }
    while (TUYA_SPI_EVENT_TX_COMPLETE != event_flag) {
        tkl_system_sleep(2);
    }
    // up send Complete,
    
    event_flag = -1;
    ret = tkl_spi_recv(TUYA_SPI_NUM_0, rcv_buf, 6);
    if (ret < 0) {
        //failed
    }
    while (TUYA_SPI_EVENT_RX_COMPLETE != event_flag) {
        tkl_system_sleep(2);
    }
    // up recv Complete,
    tkl_spi_irq_disable(TUYA_SPI_NUM_0);
    //uninitialize iic 
    ret = tkl_spi_deinit(TUYA_SPI_NUM_0);
    if (ret != 0) {
       //failed
    }
}
```























