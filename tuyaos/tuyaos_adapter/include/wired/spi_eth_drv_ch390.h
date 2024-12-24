/********************************** (C) COPYRIGHT *******************************
* File Name          : spi_eth_drv_ch390.h
* Author             : WCH
* Version            : V1.0
* Date               : 2023/05/23
* Description        : CH390 Ethernet controller header file
*******************************************************************************/
#ifndef __CH390_H
#define __CH390_H

#include <stdint.h>

/********************************************************************
 * Interface selection
 */

#define CH390_INTERFACE_SPI     // CH390H
//#define CH390_INTERFACE_8_BIT   // CH390L 8-bit mode
//#define CH390_INTERFACE_16_BIT  // CH390L 16-bit mode


/********************************************************************
 * Type definition
 */

/*
 * Interface
 * This structure defines CH390 operation interface (Parallel
 * or SPI). User should implement one of them.
 */
#ifdef CH390_INTERFACE_SPI
typedef struct{
    void (*rst)(uint8_t level);      // RSTB pin
    void (*delay_us)(uint32_t time); // Delay, unit: us
    void (*scs)(uint8_t level);    // SCS pin
    // Exchange 1 byte data by write and read spi
    uint8_t (*spi_exc_data)(uint8_t data);
}ch390_interface_t;
#else // CH390_INTERFACE_8_BIT
typedef struct{
    void (*rst)(uint8_t level);      // RSTB pin
    void (*delay_us)(uint32_t time); // Delay, unit: us
    void (*cs)(uint8_t level);    	 // CS pin
    void (*cmd)(uint8_t level);   	 // CMD pin
    void (*rdb)(uint8_t level);   	 // RDB pin
    void (*wrb)(uint8_t level);   	 // WRB pin
    void (*ll_ioinput)();          	 // Switch io to input mode
    /* Both 8-bit and 16-bit mode share these two functions,
     * just ignore the high byte in 8-bit mode */
    uint16_t (*ll_read)();           // Read data
    void (*ll_write)(uint16_t data); // Switch io to output mode and write data
}ch390_interface_t;
#endif

/* PHY mode definition */
enum ch390_phy_mode
{
    CH390_10MHD,   // 10M half-duplex
    CH390_100MHD,  // 100M half-duplex
    CH390_10MFD,   // 10M full-duplex
    CH390_100MFD,  // 100M full-duplex
    CH390_AUTO,    // Auto negotiation
};

/********************************************************************
 * Register definition
 */

/**
 * There are some differences between the register definitions of
 * CH390H and CH390L
 */

#ifdef CH390_INTERFACE_SPI
#define CH390_NCR       0x00
    #define NCR_WAKEEN      (1<<6) // Enable wakeup function
    #define NCR_FDX         (1<<3) // Duplex mode of the internal PHY
    #define NCR_LBK_MAC     (1<<1) // MAC loop-back
    #define NCR_RST         (1<<0) // Softwate reset
#define CH390_NSR       0x01
    #define NSR_SPEED       (1<<7) // Speed of internal PHY
    #define NSR_LINKST      (1<<6) // Link status of internal PHY
    #define NSR_WAKEST      (1<<5) // Wakeup event status
    #define NSR_TX2END      (1<<3) // Tx packet B complete status
    #define NSR_TX1END      (1<<2) // Tx packet A complete status
    #define NSR_RXOV        (1<<1) // Rx fifo overflow
    #define NSR_RXRDY       (1<<0)
#define CH390_TCR       0x02
    #define TCR_TJDIS       (1<<6) // Transmit jabber timer
    #define TCR_PAD_DIS2    (1<<4) // PAD appends for packet B
    #define TCR_CRC_DIS2    (1<<3) // CRC appends for packet B
    #define TCR_PAD_DIS1    (1<<2) // PAD appends for packet A
    #define TCR_CRC_DIS1    (1<<1) // CRC appends for packet A
    #define TCR_TXREQ       (1<<0) // Tx request
#define CH390_TSRA      0x03
#define CH390_TSRB      0x04
    #define TSR_TJTO        (1<<7) // Transmit jabber time out
    #define TSR_LC          (1<<6) // Loss of carrier
    #define TSR_NC          (1<<5) // No carrier
    #define TSR_LCOL        (1<<4) // Late collision
    #define TSR_COL         (1<<3) // Collision packet
    #define TSR_EC          (1<<2) // Excessive collision
#define CH390_RCR       0x05
    #define RCR_DEFAULT     0x00   // Default settings
    #define RCR_WTDIS       (1<<6) // Disable 2048 bytes watch dog
    #define RCR_DIS_CRC     (1<<4) // Discard CRC error packet
    #define RCR_ALL         (1<<3) // Pass all multicast
    #define RCR_RUNT        (1<<2) // Pass runt packet
    #define RCR_PRMSC       (1<<1) // Promiscuous mode
    #define RCR_RXEN        (1<<0) // Enable RX
#define CH390_RSR       0x06
    #define RSR_RF          (1<<7) // Rnt frame
    #define RSR_MF          (1<<6) // Multicast frame
    #define RSR_LCS         (1<<5) // Late collision seen
    #define RSR_RWTO        (1<<4) // Receive watchdog time-out
    #define RSR_PLE         (1<<3) // Physical layer error
    #define RSR_AE          (1<<2) // Alignment error
    #define RSR_CE          (1<<1) // CRC error
    #define RSR_FOE         (1<<0) // FIFO overflow error
#define CH390_ROCR      0x07
#define CH390_BPTR      0x08
#define CH390_FCTR      0x09
    #define FCTR_HWOT(ot)   (( ot & 0xf ) << 4)
    #define FCTR_LWOT(ot)   ( ot & 0xf )
#define CH390_FCR       0x0A
#define CH390_EPCR      0x0B
    #define EPCR_REEP       (1<<5) // Reload EEPROM
    #define EPCR_WEP        (1<<4) // Write EEPROM enable
    #define EPCR_EPOS       (1<<3) // EEPROM or PHY operation select
    #define EPCR_ERPRR      (1<<2) // EEPROM or PHY read command
    #define EPCR_ERPRW      (1<<1) // EEPROM or PHY write command
    #define EPCR_ERRE       (1<<0) // EEPROM or PHY access status
#define CH390_EPAR      0x0C
#define CH390_EPDRL     0x0D
#define CH390_EPDRH     0x0E
#define CH390_WCR       0x0F
    #define WCR_LINKEN      (1<<5) // Link status change wakeup
    #define WCR_SAMPLEEN    (1<<4) // Sample frame wakeup
    #define WCR_MAGICEN     (1<<3) // Magic packet wakeup
    #define WCR_LINKST      (1<<2) // Link status change event
    #define WCR_SAMPLEST    (1<<1) // Sample frame event
    #define WCR_MAGICST     (1<<0) // Magic packet event
#define CH390_PAR       0x10
#define CH390_MAR       0x16
#define CH390_GPCR      0x1E
#define CH390_GPR       0x1F
#define CH390_TRPAL     0x22
#define CH390_TRPAH     0x23
#define CH390_RWPAL     0x24
#define CH390_RWPAH     0x25
#define CH390_VIDL      0x28
#define CH390_VIDH      0x29
#define CH390_PIDL      0x2A
#define CH390_PIDH      0x2B
#define CH390_CHIPR     0x2C
#define CH390_TCR2      0x2D
#define CH390_ATCR      0x30
#define CH390_TCSCR     0x31
    #define TCSCR_ALL         0x1F
    #define TCSCR_IPv6TCPCSE  (1<<4) // IPv6 TCP checksum generation
    #define TCSCR_IPv6UDPCSE  (1<<3) // IPv6 UDP checksum generation
    #define TCSCR_UDPCSE      (1<<2) // UDP checksum generation
    #define TCSCR_TCPCSE      (1<<1) // TCP checksum generation
    #define TCSCR_IPCSE       (1<<0) // IP checksum generation
#define CH390_RCSCSR    0x32
    #define RCSCSR_UDPS     (1<<7) // UDP checksum status
    #define RCSCSR_TCPS     (1<<6) // TCP checksum status
    #define RCSCSR_IPS      (1<<5) // IP checksum status
    #define RCSCSR_UDPP     (1<<4) // UDP packet of current received packet
    #define RCSCSR_TCPP     (1<<3) // TCP packet of current received packet
    #define RCSCSR_IPP      (1<<2) // IP packet of current received packet
    #define RCSCSR_RCSEN    (1<<1) // Receive checksum checking enable
    #define RCSCSR_DCSE     (1<<0) // Discard checksum error packet
#define CH390_MPAR      0x33
#define CH390_SBCR      0x38
#define CH390_INTCR     0x39
    #define INCR_TYPE_OD    0x02
    #define INCR_TYPE_PP    0x00
    #define INCR_POL_L      0x01
    #define INCR_POL_H      0x00
#define CH390_ALNCR     0x4A
#define CH390_SCCR      0x50
#define CH390_RSCCR     0x51
#define CH390_RLENCR    0x52
#define CH390_BCASTCR   0x53
#define CH390_INTCKCR   0x54
#define CH390_MPTRCR    0x55
#define CH390_MLEDCR    0x57
#define CH390_MRCMDX    0x70
#define CH390_MRCMDX1   0x71
#define CH390_MRCMD     0x72
#define CH390_MRRL      0x74
#define CH390_MRRH      0x75
#define CH390_MWCMDX    0x76
#define CH390_MWCMD     0x78
#define CH390_MWRL      0x7A
#define CH390_MWRH      0x7B
#define CH390_TXPLL     0x7C
#define CH390_TXPLH     0x7D
#define CH390_ISR       0x7E
    #define ISR_LNKCHG      (1<<5)  // Link status change
    #define ISR_ROO         (1<<3)  // Receive overflow counter overflow
    #define ISR_ROS         (1<<2)  // Receive overflow
    #define ISR_PT          (1<<1)  // Packet transmitted
    #define ISR_PR          (1<<0)  // Packet received
#define CH390_IMR       0x7F
    #define IMR_NONE        0x00    // Disable all interrupt
    #define IMR_ALL         0xFF    // Enable all interrupt
    #define IMR_PAR         (1<<7)  // Pointer auto-return mode
    #define IMR_LNKCHGI     (1<<5)  // Enable link status change interrupt
    #define IMR_UDRUNI      (1<<4)  // Enable transmit under-run interrupt
    #define IMR_ROOI        (1<<3)  // Enable receive overflow counter overflow interrupt
    #define IMR_ROI         (1<<2)  // Enable receive overflow interrupt
    #define IMR_PTI         (1<<1)  // Enable packet transmitted interrupt
    #define IMR_PRI         (1<<0)  // Enable packet received interrupt

// SPI commands
#define OPC_REG_W       0x80  // Register Write
#define OPC_REG_R       0x00  // Register Read
#define OPC_MEM_DMY_R   0x70  // Memory Dummy Read
#define OPC_MEM_WRITE   0xF8  // Memory Write
#define OPC_MEM_READ    0x72  // Memory Read

// GPIO
#define CH390_GPIO1     0x02
#define CH390_GPIO2     0x04
#define CH390_GPIO3     0x08

#else

#define CH390_NCR       0x00
    #define NCR_WAKEEN      (1<<6) // Enable wakeup function
    #define NCR_FDX         (1<<3) // Duplex mode of the internal PHY
    #define NCR_LBK_MAC     (1<<1) // MAC loop-back
    #define NCR_RST         (1<<0) // Softwate reset
#define CH390_NSR       0x01
    #define NSR_SPEED       (1<<7) // Speed of internal PHY
    #define NSR_LINKST      (1<<6) // Link status of internal PHY
    #define NSR_WAKEST      (1<<5) // Wakeup event status
    #define NSR_TX2END      (1<<3) // Tx packet B complete status
    #define NSR_TX1END      (1<<2) // Tx packet A complete status
    #define NSR_RXOV        (1<<1) // Rx fifo overflow
#define CH390_TCR       0x02
    #define TCR_TJDIS       (1<<6) // Transmit jabber timer
    #define TCR_PAD_DIS2    (1<<4) // PAD appends for packet B
    #define TCR_CRC_DIS2    (1<<3) // CRC appends for packet B
    #define TCR_PAD_DIS1    (1<<2) // PAD appends for packet A
    #define TCR_CRC_DIS1    (1<<1) // CRC appends for packet A
    #define TCR_TXREQ       (1<<0) // Tx request
#define CH390_TSRA      0x03
#define CH390_TSRB      0x04
    #define TSR_TJTO        (1<<7) // Transmit jabber time out
    #define TSR_LC          (1<<6) // Loss of carrier
    #define TSR_NC          (1<<5) // No carrier
    #define TSR_LCOL        (1<<4) // Late collision
    #define TSR_COL         (1<<3) // Collision packet
    #define TSR_EC          (1<<2) // Excessive collision
#define CH390_RCR       0x05
    #define RCR_DEFAULT     0x00   // Default settings
    #define RCR_WTDIS       (1<<6) // Disable 2048 bytes watch dog
    #define RCR_DIS_CRC     (1<<4) // Discard CRC error packet
    #define RCR_ALL         (1<<3) // Pass all multicast
    #define RCR_RUNT        (1<<2) // Pass runt packet
    #define RCR_PRMSC       (1<<1) // Promiscuous mode
    #define RCR_RXEN        (1<<0) // Enable RX
#define CH390_RSR       0x06
    #define RSR_RF          (1<<7) // Rnt frame
    #define RSR_MF          (1<<6) // Multicast frame
    #define RSR_LCS         (1<<5) // Late collision seen
    #define RSR_RWTO        (1<<4) // Receive watchdog time-out
    #define RSR_PLE         (1<<3) // Physical layer error
    #define RSR_AE          (1<<2) // Alignment error
    #define RSR_CE          (1<<1) // CRC error
    #define RSR_FOE         (1<<0) // FIFO overflow error
#define CH390_ROCR      0x07
#define CH390_BPTR      0x08
#define CH390_FCTR      0x09
    #define FCTR_HWOT(ot)   (( ot & 0xf ) << 4)
    #define FCTR_LWOT(ot)   ( ot & 0xf )
#define CH390_FCR       0x0A
#define CH390_EPCR      0x0B
    #define EPCR_REEP       (1<<5) // Reload EEPROM
    #define EPCR_WEP        (1<<4) // Write EEPROM enable
    #define EPCR_EPOS       (1<<3) // EEPROM or PHY operation select
    #define EPCR_ERPRR      (1<<2) // EEPROM or PHY read command
    #define EPCR_ERPRW      (1<<1) // EEPROM or PHY write command
    #define EPCR_ERRE       (1<<0) // EEPROM or PHY access status
#define CH390_EPAR      0x0C
#define CH390_EPDRL     0x0D
#define CH390_EPDRH     0x0E
#define CH390_WCR       0x0F
    #define WCR_LINKEN      (1<<5) // Link status change wakeup
    #define WCR_SAMPLEEN    (1<<4) // Sample frame wakeup
    #define WCR_MAGICEN     (1<<3) // Magic packet wakeup
    #define WCR_LINKST      (1<<2) // Link status change event
    #define WCR_SAMPLEST    (1<<1) // Sample frame event
    #define WCR_MAGICST     (1<<0) // Magic packet event
#define CH390_PAR       0x10
#define CH390_MAR       0x16
#define CH390_GPCR      0x1E
#define CH390_GPR       0x1F
#define CH390_TRPAL     0x22
#define CH390_TRPAH     0x23
#define CH390_RWPAL     0x24
#define CH390_RWPAH     0x25
#define CH390_VIDL      0x28
#define CH390_VIDH      0x29
#define CH390_PIDL      0x2A
#define CH390_PIDH      0x2B
#define CH390_CHIPR     0x2C
#define CH390_TCR2      0x2D
#define CH390_ETXCSR    0x30
#define CH390_TCSCR     0x31
    #define TCSCR_ALL         0x1F
    #define TCSCR_IPv6TCPCSE  (1<<4) // IPv6 TCP checksum generation
    #define TCSCR_IPv6UDPCSE  (1<<3) // IPv6 UDP checksum generation
    #define TCSCR_UDPCSE      (1<<2) // UDP checksum generation
    #define TCSCR_TCPCSE      (1<<1) // TCP checksum generation
    #define TCSCR_IPCSE       (1<<0) // IP checksum generation
#define CH390_RCSCSR    0x32
    #define RCSCSR_UDPS     (1<<7) // UDP checksum status
    #define RCSCSR_TCPS     (1<<6) // TCP checksum status
    #define RCSCSR_IPS      (1<<5) // IP checksum status
    #define RCSCSR_UDPP     (1<<4) // UDP packet of current received packet
    #define RCSCSR_TCPP     (1<<3) // TCP packet of current received packet
    #define RCSCSR_IPP      (1<<2) // IP packet of current received packet
    #define RCSCSR_RCSEN    (1<<1) // Receive checksum checking enable
    #define RCSCSR_DCSE     (1<<0) // Discard checksum error packet
#define CH390_MPAR      0x33
#define CH390_LEDCR     0x34
#define CH390_INTCR     0x39
    #define INCR_TYPE_OD    0x02
    #define INCR_TYPE_PP    0x00
    #define INCR_POL_L      0x01
    #define INCR_POL_H      0x00
#define CH390_SCCR      0x50
#define CH390_RSCCR     0x51
#define CH390_RLENCR    0x52
#define CH390_BCASTCR   0x53
#define CH390_MPTRCR    0x55
#define CH390_MRCMDX    0xF0
#define CH390_MRCMDX1   0xF1
#define CH390_MRCMD     0xF2
#define CH390_MRRL      0xF4
#define CH390_MRRH      0xF5
#define CH390_MWCMDX    0xF6
#define CH390_MWCMD     0xF8
#define CH390_MWRL      0xFA
#define CH390_MWRH      0xFB
#define CH390_TXPLL     0xFC
#define CH390_TXPLH     0xFD
#define CH390_ISR       0xFE
    #define ISR_IOMODE      (1<<7)  // Parallel interface mode
    #define ISR_LNKCHG      (1<<5)  // Link status change
    #define ISR_UDRUN       (1<<4)  // Transmit under-run
    #define ISR_ROO         (1<<3)  // Receive overflow counter overflow
    #define ISR_ROS         (1<<2)  // Receive overflow
    #define ISR_PT          (1<<1)  // Packet transmitted
    #define ISR_PR          (1<<0)  // Packet received
#define CH390_IMR       0xFF
    #define IMR_NONE        0x00    // Disable all interrupt
    #define IMR_ALL         0xFF    // Enable all interrupt
    #define IMR_PAR         (1<<7)  // Pointer auto-return mode
    #define IMR_LNKCHGI     (1<<5)  // Enable link status change interrupt
    #define IMR_UDRUNI      (1<<4)  // Enable transmit under-run interrupt
    #define IMR_ROOI        (1<<3)  // Enable receive overflow counter overflow interrupt
    #define IMR_ROI         (1<<2)  // Enable receive overflow interrupt
    #define IMR_PTI         (1<<1)  // Enable packet transmitted interrupt
    #define IMR_PRI         (1<<0)  // Enable packet received interrupt

// GPIO
#define CH390_GPIO1     0x02
#define CH390_GPIO2     0x04
#define CH390_GPIO3     0x08
#define CH390_GPIO4     0x10
#define CH390_GPIO5     0x20
#define CH390_GPIO6     0x40
#endif

// PHY registers
#define CH390_PHY          0x40
#define CH390_PHY_BMCR     0x00
#define CH390_PHY_BMSR     0x01
#define CH390_PHY_PHYID1   0x02
#define CH390_PHY_PHYID2   0x03
#define CH390_PHY_ANAR     0x04
#define CH390_PHY_ANLPAR   0x05
#define CH390_PHY_ANER     0x06
#define CH390_PHY_PAGE_SEL 0x1F

// Packet status
#define CH390_PKT_NONE  0x00    /* No packet received */
#define CH390_PKT_RDY   0x01    /* Packet ready to receive */
#define CH390_PKT_ERR   0xFE    /* Un-stable states */
#define CH390_PKT_MAX   1536    /* Received packet max size */
#define CH390_PKT_MIN   64

/********************************************************************
 * Functions
 */

/**
 * @name ch390_interface_register
 * @brief Register IO interface
 * @param reg - The interface corresponding to the macro
 *              CH390_INTERFACE_xxx
 */
void ch390_interface_register(ch390_interface_t interface);

/**
 * @name ch390_read_reg
 * @brief Read register
 * @param reg - Target register address
 * @return Register value
 */
uint8_t ch390_read_reg(uint8_t reg);

/**
 * @name ch390_write_reg
 * @brief Write register
 * @param reg - Target register address
 * @param value - Value to be written
 */
void ch390_write_reg(uint8_t reg, uint8_t value);

/**
 * @name ch390_read_mem
 * @brief Read data from RX SRAM
 * @param data - Data buffer
 * @param length - Length to read
 */
void ch390_read_mem(uint8_t *data, int length);

/**
 * @name ch390_write_mem
 * @brief Write data to TX SRAM
 * @param data - Data buffer
 * @param length - Length to write
 */
void ch390_write_mem(uint8_t *data, int length);

/**
 * @name ch390_receive_packet
 * @brief Receive packet
 * @param buff - Size equal to CH390_PKT_MAX
 * @param rx_status - Output abnormal status while receiving packet.
 *                    It has the same format as RSR(06h).
 * @return Packet length
 */
uint32_t ch390_receive_packet(uint8_t *buff, uint8_t *rx_status);

/**
 * @name ch390_send_packet
 * @brief Send packet
 * @param buff - Data to be sent
 * @param length - Less than 3k bytes.
 */
void ch390_send_packet(uint8_t *buff, uint16_t length);

/**
 * @name ch390_send_request
 * @brief Issue transmit request
 */
void ch390_send_request(void);

/**
 * @name ch390_drop_packet
 * @brief Drop packet in RX SRAM if don't want to read it. This function
 *        modify the memory data read pointer and skip specified length
 * @param len - Skip length, length of the current packet.
 */
void ch390_drop_packet(uint16_t len);

/**
 * @name ch390_read_phy
 * @brief Read PHY register
 * @param reg - PHY register address
 */
uint16_t ch390_read_phy(uint8_t reg);

/**
 * @name ch390_write_phy
 * @brief Write PHY register
 * @param reg - PHY register address
 * @param value - Value to be written
 */
void ch390_write_phy(uint8_t reg, uint16_t value);

/**
 * @name ch390_write_eeprom
 * @brief Write EEPROM register
 * @param reg - EEPROM register address
 * @param value - Value to be written
 */
void ch390_write_eeprom(uint8_t reg, uint16_t value);

/**
 * @name ch390_software_reset
 * @brief Software reset CH390 by NCR
 */
void ch390_software_reset(void);
/**
 * @name ch390_hardware_reset
 * @brief Hardware reset CH390 by pull down RSTB pin
 */
void ch390_hardware_reset(void);

/**
 * @name ch390_default_config
 * @brief Config CH390 with default options:
 *        LED mode 1;
 *        Enable transmit check sum generation;
 *        Enable RX;
 *        Enable all interrupt and PAR
 */
void ch390_default_config(void);

/**
 * @name ch390_set_phy_mode
 * @brief Set PHY mode and enable PHY.
 *        PHY mode: Auto-negotiation, 10M/100M, full-duplex/half-duplex
 * @param mode - PHY mode
 */
void ch390_set_phy_mode(enum ch390_phy_mode mode);

/**
 * @name ch390_set_mac_address
 * @brief Set mac address
 * @param mac_addr - 6-byte length mac address array
 */
void ch390_set_mac_address(uint8_t *mac_addr);

/**
 * @name ch390_set_multicast
 * @brief Set multicast address hash table
 * @param multicast_addr - 8-byte length multicast address hash table array
 */
void ch390_set_multicast(uint8_t *multicast_hash);

/**
 * @brief Set MAR bit for a particular MAC address
 * @param mac - Destination address
 */
void ch390_set_hash_bit(uint8_t *mac);

/**
 * @name ch390_get_mac
 * @brief Get mac address
 * @param mac_addr - 6 bytes mac address output
 */
void ch390_get_mac(uint8_t *mac_addr);

/**
 * @name ch390_get_multicast
 * @brief Get multicast address hash table
 * @param multicast_addr - 8-byte length multicast address hash table output
 */
void ch390_get_multicast(uint8_t *multicast_addr);

/**
 * @name ch390_get_vendor_id
 * @brief Get vendor ID
 * @return Vendor ID
 */
uint16_t ch390_get_vendor_id(void);

/**
 * @name ch390_get_product_id
 * @brief Get product ID
 * @return Product ID
 */
uint16_t ch390_get_product_id(void);

/**
 * @name ch390_get_revision
 * @brief Get chip revision
 * @return Chip revision
 */
uint8_t ch390_get_revision(void);

/**
 * @name ch390_interrupt_config
 * @brief Interrupt configuration
 * @param mask - Interrupt to be enabled, see "spi_eth_drv_ch390.h" IMR_xxx
 */
void ch390_interrupt_config(uint8_t mask);

/**
 * @name ch390_rx_enable
 * @brief Enable or disable packet receive
 * @param op - 0: disable  1: enable
 */
void ch390_rx_enable(int op);

/**
 * @name ch390_rx_filter_config
 * @brief Configure receive filter.
 * @param config - See "spi_eth_drv_ch390.h" RCR_xxx
 */
void ch390_rx_filter_config(uint8_t config);

/**
 * @name ch390_wakeup_config
 * @brief Enable or disable wakeup_function
 * @param events - Events that trigger wakeup,
 *                 WCR_LINKEN   - Link status change
 *                 WCR_SAMPLEEN - Sample frame
 *                 WCR_MAGICEN  - Magic packet
 *                 0            - Disable wakeup function
 */
void ch390_wakeup_config(uint8_t events);

/**
 * @name ch390_wake_notify
 * @brief Wait for Magic Packet or Sample Frame and discard all
 *        other packets.
 *        If the application needs to use Wake On LAN, call this
 *        function before MCU enters low power mode. An external
 *        interrupt signal is accessible on WOL pin when wake
 *        up event occurred.
 */
void ch390_wake_notify();

/**
 * @name ch390_loop_back_enable
 * @brief Enable loop back mode
 * @param op - 0: disable  1: enable
 */
void ch390_loop_back_enable(int op);

/**
 * @name ch390_get_duplex_mode
 * @brief Get current duplex mode of the internal PHY
 * @return 0: Half-duplex  1: Full-duplex
 */
int ch390_get_duplex_mode(void);

/**
 * @name ch390_get_phy_speed
 * @brief Get the speed of the internal PHY.
 *        Only valid after PHY linked
 * @return 0: 100Mbps  1: 10Mbps
 */
int ch390_get_phy_speed(void);

/**
 * @name ch390_get_link_status
 * @brief Get link status of the internal PHY
 * @return 0: Link failed  1: Link OK
 */
int ch390_get_link_status(void);

/**
 * @name ch390_sleep_control
 * @brief Enter or exit sleep mode
 * @param op - 0: Power up  1: Power down
 */
void ch390_sleep_control(int op);

#ifndef CH390_INTERFACE_16_BIT
/**
 * @name ch390_gpio_config
 * @brief Config the input/output direction of GPIO1~3
 *        Only GPIO1~3 can be defined as input, GPIO4~6 are output only
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO3
 *        dir - 0: Input  1: Output
 */
void ch390_gpio_config(uint8_t GPIOx, uint8_t dir);

/**
 * @name ch390_gpio_write_bit
 * @brief Sets or clears the selected gpio bit.
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO6
 *        level - 0: Clear pin  1: Set pin
 */
void ch390_gpio_write_bit(uint8_t GPIOx, uint8_t level);

/**
 * @name ch390_gpio_read_bit
 * @brief Read gpio input, only CH390_GPIO1 ~ 3 are available
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO3
 * @return Input pin value
 */
uint8_t ch390_gpio_read_bit(uint8_t GPIOx);
#endif

/**
 * @name ch390_int_pin_config
 * @brief Configure INT pin output type and polarity
 * @param type - INCR_TYPE_OD: Open drain output
 *               INCR_TYPE_PP: Push pull output
 *        pol - INCR_POL_L: Active low
 *              INCR_POL_H: Active high
 */
void ch390_int_pin_config(uint8_t type, uint8_t pol);

/**
 * @name ch390_get_int_status
 * @brief Get CH390 interrupt status and clear them
 * @return Interrupt status
 */
uint8_t ch390_get_int_status(void);

/**
 * @name ch390_rx_reset
 * @brief Reset CH390 Rx FIFO Pointer
 * @return Interrupt status
 */
void ch390_rx_reset(void);

uint16_t ch390_MWR(void);

void ch390_reg_dump(void);

#endif /* __CH390_H */
