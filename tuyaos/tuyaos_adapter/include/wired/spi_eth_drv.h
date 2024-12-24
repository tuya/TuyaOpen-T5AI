#ifndef __TKL_SPI_ETHERNET_H__
#define __TKL_SPI_ETHERNET_H__

#define SPI_LAN_DBG_EN
//#define SPI_LAN_DBG_TRACE
//#define SPI_LAN_HARDWARE_RESET_EN
//#define CH390_TX_ERR_CHK_EN
//#define CH390_TX_ERR_CHK_EN_RESET
//#define SPI_LAN_PKT_TRACE
//#define SPI_LAN_LINK_SAFE_NET_CHK
#define CH390_TX_FIFO_POINTER_DOUBLE_CHK
#define CH390_TX_FIFO_POINTER_EN
#define CH390_TX_FIFO_POINTER_EN_RESET
#define SPI_LAN_LOCK_SUPPORT

extern void bk_printf(const char *fmt, ...);

#ifdef SPI_LAN_DBG_EN
#define SPI_LAN_DBG(fmt, args...) bk_printf(fmt"\r\n", args)
#define SPI_LAN_LOG(fmt, args...) bk_printf(fmt"\r\n", args)
#define SPI_LAN_ERR(fmt, args...) bk_printf(fmt"\r\n", args)
#else
#define SPI_LAN_DBG(fmt, args...) do{}while(0)
#define SPI_LAN_LOG(fmt, args...) bk_printf(fmt"\r\n", args)
#define SPI_LAN_ERR(fmt, args...) bk_printf(fmt"\r\n", args)
#endif /* SPI_LAN_DBG_EN */

#ifdef SPI_LAN_LOCK_SUPPORT
#define SPI_ETHERNET_RELEASE()        tkl_mutex_release(spi_eth_lock)
#define SPI_ETHERNET_LOCK_X()         tkl_mutex_lock(spi_eth_lock)
#define SPI_ETHERNET_UNLOCK_X()       tkl_mutex_unlock(spi_eth_lock)
#define SPI_ETHERNET_LOCK()
#define SPI_ETHERNET_UNLOCK()
#else
#define SPI_ETHERNET_LOCK()
#define SPI_ETHERNET_UNLOCK()
#define SPI_ETHERNET_RELEASE()
#define SPI_ETHERNET_LOCK_X()
#define SPI_ETHERNET_UNLOCK_X()
#endif /* SPI_LAN_LOCK_SUPPORT */
#endif /* __TKL_SPI_ETHERNET_H__ */