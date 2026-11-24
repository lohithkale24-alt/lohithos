#ifndef _E1000_H_
#define _E1000_H_

#include "types.h"
#include "pci.h"

#define E1000_VENDOR_INTEL 0x8086
#define E1000_DEV_82540EM  0x100E  /* VirtualBox Default NIC */
#define E1000_DEV_82545EM  0x100F
#define E1000_DEV_82543GC  0x1004

/* e1000 MMIO Register Offsets */
#define REG_CTRL       0x00000
#define REG_STATUS     0x00008
#define REG_EEPROM     0x00014
#define REG_CTRL_EXT   0x00018
#define REG_IMASK      0x000D0
#define REG_RCTRL      0x00100
#define REG_RXDESCLO   0x02800
#define REG_RXDESCHI   0x02804
#define REG_RXDESCLEN  0x02808
#define REG_RXDESCHEAD 0x02810
#define REG_RXDESCTAIL 0x02818
#define REG_TCTRL      0x00400
#define REG_TXDESCLO   0x03800
#define REG_TXDESCHI   0x03804
#define REG_TXDESCLEN  0x03808
#define REG_TXDESCHEAD 0x03810
#define REG_TXDESCTAIL 0x03818
#define REG_RAL        0x05400
#define REG_RAH        0x05404

#define NUM_RX_DESC 32
#define NUM_TX_DESC 16

struct e1000_rx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t  status;
    volatile uint8_t  errors;
    volatile uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t  cso;
    volatile uint8_t  cmd;
    volatile uint8_t  status;
    volatile uint8_t  css;
    volatile uint16_t special;
} __attribute__((packed));

typedef struct {
    uint32_t mmio_base;
    uint8_t  mac[6];
    bool     link_up;
    uint32_t rx_cur;
    uint32_t tx_cur;
    uint32_t rx_packets;
    uint32_t tx_packets;
} e1000_device_t;

extern e1000_device_t g_nic;

bool e1000_init(pci_device_t *pci_dev);
int  e1000_send_packet(const void *data, uint16_t len);
int  e1000_receive_packet(void *buf, uint16_t max_len);

#endif
