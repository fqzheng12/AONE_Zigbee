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


// ----------------
// -- Includes
// ----------------
#include "app/framework/include/af.h"
#include "aurora-host-hal.h"
#include "aurora-host-bootload.h"

// Option to enable or disable debug printing. See .h file
#ifdef STM32_BOOTLOAD_DEBUG_PRINT_EN
    #define STMBLDebugPrint(...) emberAfPrint(EMBER_AF_PRINT_DEBUG, __VA_ARGS__)
#else
    #define STMBLDebugPrint(...)
#endif

// ----------------
// -- Variables
// ----------------
int8u flashBuffer[STM_DEFAULT_PAGE]; // Flash buffer and send buffer are what they say they are.
int8u sendBuffer[STM_DEFAULT_PAGE + 5] = {0x00}; // give a bit of room for a few more checksum and option bytes
int8u checksum = 0x00; // checksum variable we use once in a while

// ----------------
// -- Local prototypes
// ----------------
void BootloadStateHandler(void);
boolean waitForAck(int32u timeout);
void delay(int32u time);


// ----------------
// -- Main function
// ----------------

// Main bootload routine. This is a completly linear operation
// and should be completed in order as a routine. If an error
// occurs, FALSE will be returned. Otherwise TRUE and the
// bootload should be successful.
//
// If you need to debug this code, read and follow it very carefully
// according to the STM32 appnote "AN3155".
boolean emberAfPluginAuroraBootloadUpgrade(int32u offset, int32u endOffset)
{

// --------------------------------------------------------
// -- Init ------------------------------------------------
// --------------------------------------------------------

    STMBLDebugPrint("\r\nOffset: %4X, End: %4X", offset, endOffset);
    // Init serial port
    emberAfPluginAuroraHostHalSerialInitHostBootload();

    // First we bring BOOT0 high and reset the STM32
    emberAfPluginAuroraHostHalResetPin(FALSE);
    emberAfPluginAuroraHostHalBootloadPin(TRUE);

    // Give a bit of time for the chip to sit in reset
    delay(10);

    // Bring chip out of reset and into. We should now be in bl mode.
    emberAfPluginAuroraHostHalResetPin(TRUE);
    
    // Let's send an init command to the STM32
    sendBuffer[0] = 0x7F;
    emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 1);

    // We should receive an ack
    if (!waitForAck(ACK_TIMEOUT)) return FALSE;

// --------------------------------------------------------
// -- Get ------------------------------------------------
// --------------------------------------------------------

    // We don't need to bother with the GET command because
    // we know exactly what processor we're using anyway

// --------------------------------------------------------
// -- Erase ------------------------------------------------
// --------------------------------------------------------

    // Time to erase chip, lets send the erase command
    STMBLDebugPrint("\r\nErasing");
    sendBuffer[0] = 0x44;
    sendBuffer[1] = 0xBB; // Checksum
    emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 2);
    
    // We should receive an ack
    if (!waitForAck(ACK_TIMEOUT)) return FALSE;

    // Now to send global erase cmd to erase entire chip
    STMBLDebugPrint("\r\nGlobal erase");
    sendBuffer[0] = 0xFF;
    sendBuffer[1] = 0xFF;
    sendBuffer[2] = 0x00; // Checksum
    emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 3);

    // We should receive an ack
    if (!waitForAck(ACK_TIMEOUT)) return FALSE;
    STMBLDebugPrint("\r\nErased!");

// --------------------------------------------------------
// -- Write ------------------------------------------------
// --------------------------------------------------------

    int32u currentAddress = 0;
    int32u endAddress = endOffset - offset;
    int8u pageSize = STM_DEFAULT_PAGE;

    // Loop to write data. NOTE: this loop will overun the last page by pagesize
    // this is because endOffset isn't actually the end of data. It's 64 bytes short
    // so we can't use that. This is actually okay as long as the external Flash has
    // 0xFFFF in all those overrun locations (it should because its erased before a
    // write.
    while(currentAddress < endAddress + pageSize)
    {

// -------------------- Write cmd

        // Begin writing to chip, first we send write command
        sendBuffer[0] = 0x31;
        sendBuffer[1] = 0xCE;
        emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 2);

        // We should receive an ack
        if (!waitForAck(ACK_TIMEOUT)) return FALSE;

// -------------------- Address 

        // Now we send the start address of the program code and checksum
        sendBuffer[0] = (int8u) ((currentAddress + STM_FLASH_OFFSET) >> 24) & 0xFF; // MSB
        sendBuffer[1] = (int8u) ((currentAddress + STM_FLASH_OFFSET) >> 16) & 0xFF;
        sendBuffer[2] = (int8u) ((currentAddress + STM_FLASH_OFFSET) >> 8)  & 0xFF;
        sendBuffer[3] = (int8u) ((currentAddress + STM_FLASH_OFFSET))  & 0xFF; // LSB
        sendBuffer[4] = sendBuffer[0] ^ sendBuffer[1] ^ sendBuffer[2] ^ sendBuffer[3]; // Checksum
        emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 5);
        STMBLDebugPrint("\r\nAddress: %X %X %X %X", sendBuffer[0], sendBuffer[1], sendBuffer[2], sendBuffer[3]);
        
        // We should receive an ack
        if (!waitForAck(ACK_TIMEOUT)) return FALSE;

// -------------------- Amount of data

        // Now we need to tell the STM32 how many bytes we'll send
        sendBuffer[0] = pageSize - 1; // We send N-1 , not N, according to the appnote
        checksum = sendBuffer[0]; // Xor to checksum

// -------------------- Send data 

        // Read a page from flash, print offset and page size.
        STMBLDebugPrint("\r\nReading flash at offset: %4X, length %d", currentAddress + offset, pageSize);
        if (!emberAfOtaStorageDriverReadCallback(currentAddress + offset, pageSize, flashBuffer)) return FALSE;

        // Copy data from Flash read buffer to sending buffer
        for (int i = 0; i < pageSize; i++)
        {
            sendBuffer[i+1] = flashBuffer[i]; // +1 because we have the number byte at the start
            checksum ^= sendBuffer[i+1]; // Xor this to checksum
        }
        sendBuffer[pageSize + 1] = checksum; // +1 because we have num of bytes in [0] and data up to [pagesize + 1] (we overwrite the + 1)
       
        // Send num of bytes, data and checksum
        STMBLDebugPrint("\r\nWriting %d bytes. Checksum is: %X", pageSize, checksum);
        emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, pageSize + 2); // +2 because we have num of pages + pagesize-1 + checksum

        // We should receive an ack
        if (!waitForAck(ACK_TIMEOUT)) return FALSE;

// -------------------- Increment pagesize for next data 

        // Increment out currentAddress
        currentAddress += pageSize;
        STMBLDebugPrint("\r\nNext write address will be: %4X", currentAddress);
    }


// --------------------------------------------------------
// -- Set BOOT0 pin low -----------------------------------
// --------------------------------------------------------

    emberAfPluginAuroraHostHalBootloadPin(FALSE);

// --------------------------------------------------------
// -- Go ---- !!!! UNUSED !!!! ----------------------------
// --------------------------------------------------------
/*
    // Send Go command
    STMBLDebugPrint("\r\nJumping to application start location!");
    sendBuffer[0] = 0x21;
    sendBuffer[1] = 0xDE; // Checksum
    emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 2);

    // We should receive an ack
    if (!waitForAck(ACK_TIMEOUT)) return FALSE;

    // Now we send the start address of the program code and checksum
    sendBuffer[0] = (int8u) (STM_FLASH_OFFSET >> 24) & 0xFF; // MSB
    sendBuffer[1] = (int8u) (STM_FLASH_OFFSET >> 16) & 0xFF;
    sendBuffer[2] = (int8u) (STM_FLASH_OFFSET >> 8)  & 0xFF;
    sendBuffer[3] = (int8u) (STM_FLASH_OFFSET >> 0)  & 0xFF; // LSB
    sendBuffer[4] = sendBuffer[0] ^ sendBuffer[1] ^ sendBuffer[2] ^ sendBuffer[3]; // Checksum
    emberAfPluginAuroraHostHalPlcSendCommand(sendBuffer, 5);

    // We should receive an ack
    if (!waitForAck(ACK_TIMEOUT)) return FALSE;
    STMBLDebugPrint("\r\nComplete!");
*/
    return TRUE;
}


// This function waits for an ACK byte to be received. If the timeout is
// reached, it will return FALSE.
//
// Timeout is a number in mS
boolean waitForAck(int32u timeout)
{
    int8u ack; // Variable that we'll store the incomming data in
    int16u length = 1; // Only care about reading 1 byte at a time here

    STMBLDebugPrint("\r\nWaiting for ACK. Timeout in: %dms", timeout);
    
    int32u startTime = halCommonGetInt32uMillisecondTick(); 
    while(halCommonGetInt32uMillisecondTick() - startTime < timeout)
    {
        if (emberAfPluginAuroraHostHalPlcReadBytes() == EMBER_SUCCESS)
        {
            // Read the bytes
            length = receiveReadBuffer(&ack, length);
            if(length == 1) 
            {  
                if (ack == ACK) 
                {
                    STMBLDebugPrint(": ACK");    
                    return TRUE;
                } 
                else if (ack == NACK) 
                {
                    STMBLDebugPrint(": NACK");    
                    return FALSE;
                }
            }

        }
        
        // We have to reset the wd timer because this can take a while
        halResetWatchdog(); 
    }

    STMBLDebugPrint("\r\nNo ACK, timed out", timeout);
    return FALSE;
}

// Resets all devices on the reset line of the STM32
void STM32ToggleReset()
{
    STMBLDebugPrint("\r\nPerforming hard reset to all devices..\r\n");
    
    // Drop the bootload pin low
    emberAfPluginAuroraHostHalBootloadPin(FALSE);

    // Reset device
    emberAfPluginAuroraHostHalResetPin(FALSE);
    delay(3);
    emberAfPluginAuroraHostHalResetPin(TRUE);
}

// A simple function to create blocking delays.
//
// Time is a number in mS
void delay(int32u time)
{
    STMBLDebugPrint("\r\nDelaying: %dms", time);
    int32u startTime = halCommonGetInt32uMillisecondTick(); 
    while(halCommonGetInt32uMillisecondTick() - startTime < time)
    {
        halResetWatchdog(); 
    }
}