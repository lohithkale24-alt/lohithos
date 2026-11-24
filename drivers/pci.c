#include "pci.h"

#define MAX_PCI_DEVICES 64
static pci_device_t pci_device_list[MAX_PCI_DEVICES];
static int pci_device_count = 0;

uint32_t pci_read_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_read_config_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return (uint8_t)((inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF);
}

void pci_write_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}

void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t current = inl(PCI_CONFIG_DATA);
    int shift = (offset & 2) * 8;
    current &= ~(0xFFFF << shift);
    current |= ((uint32_t)val << shift);
    outl(PCI_CONFIG_DATA, current);
}

void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t cmd = pci_read_config_word(bus, slot, func, 0x04);
    cmd |= 0x0007; /* Enable I/O Space, Memory Space, and Bus Master */
    pci_write_config_word(bus, slot, func, 0x04, cmd);
}

pci_device_t *pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_device_list[i].vendor_id == vendor_id &&
            (device_id == 0xFFFF || pci_device_list[i].device_id == device_id)) {
            return &pci_device_list[i];
        }
    }
    return NULL;
}

void pci_scan(void) {
    pci_device_count = 0;
    for (uint16_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor = pci_read_config_word(bus, slot, func, 0x00);
                if (vendor == 0xFFFF || vendor == 0x0000) continue;

                uint16_t device = pci_read_config_word(bus, slot, func, 0x02);
                uint8_t header_type = pci_read_config_byte(bus, slot, func, 0x0E);

                if (pci_device_count < MAX_PCI_DEVICES) {
                    pci_device_t *dev = &pci_device_list[pci_device_count++];
                    dev->bus = (uint8_t)bus;
                    dev->device = slot;
                    dev->function = func;
                    dev->vendor_id = vendor;
                    dev->device_id = device;
                    dev->header_type = header_type;
                    dev->class_id = pci_read_config_byte(bus, slot, func, 0x0B);
                    dev->subclass_id = pci_read_config_byte(bus, slot, func, 0x0A);
                    dev->prog_if = pci_read_config_byte(bus, slot, func, 0x09);
                    dev->bar0 = pci_read_config_dword(bus, slot, func, 0x10);
                    dev->irq = pci_read_config_byte(bus, slot, func, 0x3C);
                }

                if (func == 0 && !(header_type & 0x80)) {
                    break; /* Single function device */
                }
            }
        }
    }
}
