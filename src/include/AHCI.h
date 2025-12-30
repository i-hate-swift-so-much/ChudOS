#pragma once

#include "PCI.h"
#include "IO.h"

// real coding for this driver started
// dec 29, 2025.
// note to self, AHCI specification includes 
// the definition for the HBA's PCI header
// chapter 2 section 1

// for some reason, HBA includes full documentation
// of the PCI header type 1, it's beautiful.

// important info:
// the bar AHCI uses is bar 5, apparently it only supports 32 bit addresses, easy!
// 