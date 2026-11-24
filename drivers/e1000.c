#include "e1000.h"
#include "heap.h"
#include "isr.h"
#include "pic.h"

e1000_device_t g_nic;

static struct e1000_rx_desc *rx_descs = NULL;
static struct e1000_tx_desc *tx_descs = NULL;
static uint8_t *rx_buffers[NUM_RX_DESC];
static uint8_t *tx_buffers[NUM_TX_DESC];

static inline void mmio_write32(uint32_t reg, uint32_t val) {
    *(volatile uint32_t *)(g_nic.mmio_base + reg) = val;
}

static inline uint32_t mmio_read32(uint32_t reg) {
    return *(volatile uint32_t *)(g_nic.mmio_base + reg);
}

static uint16_t e1000_read_eeprom(uint8_t addr) {
    mmio_write32(REG_EEPROM, 1 | ((uint32_t)addr << 8));
    uint32_t val = 0;
    while (!((val = mmio_read32(REG_EEPROM)) & (1 << 4))) {
        io_wait();
    }
    return (uint16_t)((val >> 16) & 0xFFFF);
}

static void e1000_read_mac(void) {
    /* First try reading MAC from EEPROM */
    uint32_t eeprom_test = mmio_read32(REG_EEPROM);
    if (eeprom_test & (1 << 4)) {
        uint16_t val0 = e1000_read_eeprom(0);
        uint16_t val1 = e1000_read_eeprom(1);
        uint16_t val2 = e1000_read_eeprom(2);
        g_nic.mac[0] = val0 & 0xFF;
        g_nic.mac[1] = val0 >> 8;
        g_nic.mac[2] = val1 & 0xFF;
        g_nic.mac[3] = val1 >> 8;
        g_nic.mac[4] = val2 & 0xFF;
        g_nic.mac[5] = val2 >> 8;
    } else {
        /* Read from RAL and RAH registers (VirtualBox populates these) */
        uint32_t ral = mmio_read32(REG_RAL);
        uint32_t rah = mmio_read32(REG_RAH);
        g_nic.mac[0] = ral & 0xFF;
        g_nic.mac[1] = (ral >> 8) & 0xFF;
        g_nic.mac[2] = (ral >> 16) & 0xFF;
        g_nic.mac[3] = (ral >> 24) & 0xFF;
        g_nic.mac[4] = rah & 0xFF;
        g_nic.mac[5] = (rah >> 8) & 0xFF;
    }

    /* Fallback default VirtualBox MAC if 0 */
    if (g_nic.mac[0] == 0 && g_nic.mac[1] == 0 && g_nic.mac[2] == 0) {
        g_nic.mac[0] = 0x08;
        g_nic.mac[1] = 0x00;
        g_nic.mac[2] = 0x27;
        g_nic.mac[3] = 0x12;
        g_nic.mac[4] = 0x34;
        g_nic.mac[5] = 0x56;
    }
}

bool e1000_init(pci_device_t *pci_dev) {
    if (!pci_dev) return false;

    pci_enable_bus_master(pci_dev->bus, pci_dev->device, pci_dev->function);
    g_nic.mmio_base = pci_dev->bar0 & ~0xF;
    g_nic.rx_cur = 0;
    g_nic.tx_cur = 0;
    g_nic.rx_packets = 0;
    g_nic.tx_packets = 0;
    g_nic.link_up = true;

    e1000_read_mac();

    /* Allocate aligned RX and TX descriptors */
    rx_descs = (struct e1000_rx_desc *)kmalloc(sizeof(struct e1000_rx_desc) * NUM_RX_DESC + 16);
    tx_descs = (struct e1000_tx_desc *)kmalloc(sizeof(struct e1000_tx_desc) * NUM_TX_DESC + 16);

    for (int i = 0; i < NUM_RX_DESC; i++) {
        rx_buffers[i] = (uint8_t *)kmalloc(2048);
        rx_descs[i].addr = (uint32_t)rx_buffers[i];
        rx_descs[i].status = 0;
    }

    for (int i = 0; i < NUM_TX_DESC; i++) {
        tx_buffers[i] = (uint8_t *)kmalloc(2048);
        tx_descs[i].addr = (uint32_t)tx_buffers[i];
        tx_descs[i].cmd = 0;
        tx_descs[i].status = (1 << 0); /* TX Done */
    }

    /* Configure RX */
    mmio_write32(REG_RXDESCLO, (uint32_t)rx_descs);
    mmio_write32(REG_RXDESCHI, 0);
    mmio_write32(REG_RXDESCLEN, NUM_RX_DESC * sizeof(struct e1000_rx_desc));
    mmio_write32(REG_RXDESCHEAD, 0);
    mmio_write32(REG_RXDESCTAIL, NUM_RX_DESC - 1);

    /* RCTL: EN (bit 1), SBP (bit 2), UPE (bit 3), MPE (bit 4), BAM (bit 15), BSIZE 2048 (bits 17:16 = 00) */
    mmio_write32(REG_RCTRL, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 15));

    /* Configure TX */
    mmio_write32(REG_TXDESCLO, (uint32_t)tx_descs);
    mmio_write32(REG_TXDESCHI, 0);
    mmio_write32(REG_TXDESCLEN, NUM_TX_DESC * sizeof(struct e1000_tx_desc));
    mmio_write32(REG_TXDESCHEAD, 0);
    mmio_write32(REG_TXDESCTAIL, 0);

    /* TCTL: EN (bit 1), PSP (bit 3), CT (Collision Threshold = 0x0F), COLD (0x01FF00) */
    mmio_write32(REG_TCTRL, (1 << 1) | (1 << 3) | (0x0F << 4) | (0x40 << 12));

    return true;
}

int e1000_send_packet(const void *data, uint16_t len) {
    if (len > 2048 || !tx_descs) return -1;

    uint32_t cur = g_nic.tx_cur;
    memcpy(tx_buffers[cur], data, len);

    tx_descs[cur].length = len;
    tx_descs[cur].cmd = (1 << 0) | (1 << 1) | (1 << 3); /* EOP | IFCS | RS */
    tx_descs[cur].status = 0;

    g_nic.tx_cur = (cur + 1) % NUM_TX_DESC;
    mmio_write32(REG_TXDESCTAIL, g_nic.tx_cur);

    /* Wait for transmission complete */
    uint32_t timeout = 100000;
    while (!(tx_descs[cur].status & 0x01) && timeout--) {
        io_wait();
    }

    g_nic.tx_packets++;
    return len;
}

int e1000_receive_packet(void *buf, uint16_t max_len) {
    if (!rx_descs) return 0;

    uint32_t cur = g_nic.rx_cur;
    if (!(rx_descs[cur].status & 0x01)) {
        return 0; /* No new packet ready */
    }

    uint16_t len = rx_descs[cur].length;
    if (len > max_len) len = max_len;

    memcpy(buf, rx_buffers[cur], len);
    rx_descs[cur].status = 0;

    uint32_t old_cur = cur;
    g_nic.rx_cur = (cur + 1) % NUM_RX_DESC;
    mmio_write32(REG_RXDESCTAIL, old_cur);

    g_nic.rx_packets++;
    return len;
}
