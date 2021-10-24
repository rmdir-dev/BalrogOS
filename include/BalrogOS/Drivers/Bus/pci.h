#pragma once

#include <stdint.h>
#include "pci_class.h"
#include "pci_header.h"

/*
Peripheral Component Interconnect
Documentation : 
    PCI : https://wiki.osdev.org/PCI
    PCI : https://en.wikipedia.org/wiki/PCI_configuration_space
    PCI : https://stackoverflow.com/a/47859822
    PCI IDE : https://wiki.osdev.org/PCI_IDE_Controller

    Header type 0x00 : https://wiki.osdev.org/PCI#Header_Type_0x00
    Header Type : https://wiki.osdev.org/PCI#Header_Type_0x02_.28PCI-to-CardBus_bridge.29
*/

/*
PCI HEADERS 
https://wiki.osdev.org/PCI#Common_Header_Fields
*/

#define PCI_CONFIG_ADDRESS              0xcf8
#define PCI_CONFIG_DATA                 0xcfc

#define PCI_MULTIFUNCTIONAL_DEVICE      0x80

typedef struct __pci_t
{
    uint8_t bus;
    uint8_t slot;
    uint8_t func;   
} __attribute__((packed)) pci_t;

typedef struct __pci_device_t
{
    pci_t bus;
    uint8_t class;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint16_t vendor_id;
    uint16_t device_id;
} __attribute__((packed)) pci_device_t;

/**
 * @brief initialize the PCI bus
 * 
 */
void init_pci();

/**
 * @brief read double word from the pci bus.
 * 
 * @param pci 
 * @param offset 
 * @return uint32_t 
 */
uint32_t pci_read_dword(pci_t pci, uint16_t offset);

/**
 * @brief read word from the pci bus.
 * 
 * @param pci 
 * @param offset 
 * @return uint32_t 
 */
uint16_t pci_read_word(pci_t pci, uint16_t offset);

/**
 * @brief read byte from the pci bus
 * 
 * @param pci 
 * @param offset 
 * @return uint32_t 
 */
uint8_t pci_read_byte(pci_t pci, uint16_t offset);

/**
 * @brief write double word to the pci bus.
 * 
 * @param pci 
 * @param offset 
 * @return uint32_t 
 */
void pci_write_dword(pci_t pci, uint32_t value, uint16_t offset);
