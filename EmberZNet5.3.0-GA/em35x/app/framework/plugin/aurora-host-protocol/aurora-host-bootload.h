// aurora-host-bootload.c & aurora-host-bootload.c
//
// This plugin allows for bootloading of SMT32 microcontrollers
// over UART. The procedure is very simple and can be referenced
// from the UART booloading app note AN3155 found at:
//
// www.st.com/web/en/resource/technical/document/.../CD00264342.pdf
// (google that..)
//
// Created: 09/03/2015 | Rajesh Nakarja
//
// (C) Telegesis UK Ltd 2015 - All rights reserved

// --- Some defines
#define ACK                 0x79
#define NACK                0x1F

#define ACK_TIMEOUT         100 // Timeout in mS

#define INIT_CMD            0x7F
#define GET_CMD             0x00
#define READ_MEMORY_CMD     0x11
#define ERASE_MEMORY_CMD    0x44
#define ERASE_GLOBAL_CMD    0xFF
#define WRITE_MEMORY_CMD    0x31 

#define STM_FLASH_OFFSET    0x08000000
#define STM_DEFAULT_PAGE    64 // This can go up to 256 for the STM323F077

// --- Do you want debug printing for STM32 bootloading?
#define STM32_BOOTLOAD_DEBUG_PRINT_EN

// --- Main function to call a bootload procedure
boolean emberAfPluginAuroraBootloadUpgrade(int32u offset, int32u endOffset);
void STM32ToggleReset();