#pragma once

#include "stddef.h"
#include "stdint.h"

typedef struct {
    uint64_t ECM_Base;
    uint16_t PCI_Segment_Group;
    uint8_t PCI_Bus_Start;
    uint8_t PCI_Bus_End;
}PCIe_Config_BAR_Allocation;

// if something doesn't have a comment,
// refer to the OSDev WiKi's page on
// ACPI tables, I wrote this without
// internet and didn't have access
// at the time
struct PCIe_ACPI_MCFG_Table{
    uint32_t Signature; // must be equivalent to "MCFG"
    uint32_t Length; // length of table in bytes
    uint8_t Revision; // should be 1
    uint8_t Checksum; // should be 0
    uint16_t OEM_ID_UPPER;
    uint32_t OEM_ID_LOWER;
    uint64_t OEM_TABLE_ID; // manufacturer
    uint32_t OEM_REVISION;
    uint32_t CREATOR_ID;
    uint32_t CREATOR_REVISION;
};

