#include "app/framework/include/af.h"

#include <string.h>
#include "aurora-host-hal.h"
#include "aurora-host-protocol.h"
#include "aurora-host-bootload.h"

void auroraHostHalStartTimeoutTimer(void);

EmberEventControl emberAfPluginAuroraHostProtocolBootloadTimeoutEventControl;
void emberAfPluginAuroraHostProtocolBootloadTimeoutEventHandler(void);
EmberEventControl emberAfPluginAuroraHostProtocolResetEventControl;
void emberAfPluginAuroraHostProtocolResetEventHandler(void);
EmberEventControl emberAfPluginAuroraHostProtocolBootloadEventControl;
void emberAfPluginAuroraHostProtocolBootloadEventHandler(void);

static int8u receivedData[RECEIVE_DATA_BUFFER_SIZE];
static int8u receiveReadIndex = 0;
static int8u receiveWriteIndex = 0;

void emberAfPluginAuroraHostProtocolResetEventHandler(void)
{
    //resetInactive();
    //emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolBootloadEventControl, MILLISECOND_TICKS_PER_SECOND);
}
    
void emberAfPluginAuroraHostHalResetPin(boolean state)
{
    // This is the wrong way due to an inverting circuit on Azoteq's hardware
    if (state == FALSE) halClearLed(STM32_RESET);
    else halSetLed(STM32_RESET);
}

void emberAfPluginAuroraHostHalBootloadPin(boolean state)
{
    if (state == TRUE) halClearLed(STM32_BOOT0);
    else halSetLed(STM32_BOOT0);
}

void emberAfPluginAuroraHostHalSerialInitHostBootload(void)
{
    emberSerialInit(HOST_SERIAL_PORT, HOST_BAUD_RATE, PARITY_EVEN, 1);
}

void emberAfPluginAuroraHostHalSerialInitHostPlc(void)
{
    emberSerialInit(HOST_SERIAL_PORT, HOST_BAUD_RATE, PARITY_NONE, 1);
}

void emberAfPluginAuroraHostHalPlcSendCommand(int8u *command, int8u length)
{
    emberAfDebugPrint("\r\nPLC Send Cmd ");    
    debugBuffer(command, length);
    emberSerialWriteData(HOST_SERIAL_PORT, command, length); 
    auroraHostHalStartTimeoutTimer();
}

EmberStatus emberAfPluginAuroraHostHalPlcReadBytes(void)
{
    EmberStatus status = EMBER_SERIAL_RX_EMPTY;
    int16u bytesAvailable = 0;
    int16u bytesRead = 0;
    int8u command[10];
    int8u bytes = 10;
    
    // Check for data available to read
    while (bytesAvailable = emberSerialReadAvailable(HOST_SERIAL_PORT)) 
    {
        // Read the data 
        status = emberSerialReadData(HOST_SERIAL_PORT, command, (bytesAvailable <= bytes) ? bytesAvailable : bytes, &bytesRead);
        receiveWriteBuffer(command, bytesRead);
    }

    return status;
}

int16u emberAfPluginAuroraHostHalPlcBytesAvailable(void)
{
    return (receiveWriteIndex - receiveReadIndex) & RECEIVE_DATA_BUFFER_SIZE;
}

boolean emberAfPluginAuroraHostHalGetResponse(void *buffer, int16u *length)
{
    int8u bytesToRead;
    
    bytesToRead = *length;
    *length = receiveReadBuffer(buffer, bytesToRead);
    
    if (*length == bytesToRead) 
    {
        return TRUE;
    } 
    else 
    {
        return FALSE;
    }
}

void auroraHostHalStartTimeoutTimer(void)
{
    //emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolBootloadTimeoutEventControl, MILLISECOND_TICKS_PER_SECOND);
}

void emberAfPluginAuroraHostProtocolBootloadTimeoutEventHandler(void)
{
    emberEventControlSetInactive(emberAfPluginAuroraHostProtocolBootloadTimeoutEventControl);
    //setHostBootloadHalState(STM32_BOOTLOAD_STATE_IDLE);    
    emberAfDebugPrint("\r\nTimeout waiting for Host Bootloader. Resetting state machine");    
}

void emberAfPluginAuroraHostProtocolTimeoutCancel(void)
{
    emberEventControlSetInactive(emberAfPluginAuroraHostProtocolBootloadTimeoutEventControl);
    
    emberAfDebugPrint("\r\nCancel bootload timeout");    
}

void receiveWriteBuffer(int8u *data, int8u length)
{
    int8u index;

    for (index = 0; index < length; index++) 
    {
        receivedData[receiveWriteIndex] = data[index];
        receiveWriteIndex = (receiveWriteIndex + 1) & RECEIVE_DATA_BUFFER_SIZE;
    }    
}

int8u receiveReadBuffer(int8u *data, int8u length)
{
    int8u index;

    if (receiveReadIndex == receiveWriteIndex) 
    { 
        return 0;
    } 
    else 
    {
        for (index = 0; (index < length) && (receiveReadIndex != receiveWriteIndex); index++) 
        {
            data[index] = receivedData[receiveReadIndex];
            receiveReadIndex = (receiveReadIndex + 1) & RECEIVE_DATA_BUFFER_SIZE;
        }
    }
    
    //emberAfDebugPrint("Receive Read (%d) (%d)\r\n", receiveReadIndex, (receiveReadIndex + 1) & RECEIVE_DATA_BUFFER_SIZE);    
    //debugBuffer(data, length);
    return index;
}

int8u peekReadBuffer(int8u *data, int8u length)
{
    int8u index;
    int8u localReceiveReadIndex = receiveReadIndex;
    int8u localReceiveWriteIndex = receiveWriteIndex;
    
    if (localReceiveReadIndex == localReceiveWriteIndex) 
    { 
        return 0;
    } 
    else 
    {
        for (index = 0; (index < length) && (localReceiveReadIndex != localReceiveWriteIndex); index++) 
        {
            data[index] = receivedData[localReceiveReadIndex];
            localReceiveReadIndex = (localReceiveReadIndex + 1) & RECEIVE_DATA_BUFFER_SIZE;
        }
    }

//    emberAfDebugPrint("Receive peek (%d) (%d)\r\n", localReceiveReadIndex, (localReceiveReadIndex + 1) & RECEIVE_DATA_BUFFER_SIZE);    
//    debugBuffer(data, length);
    return index;
}

void setReceiveBufferIndexValues(int8u readIdx, int8u writeIdx)
{
    // Access fn.
  
    receiveReadIndex = readIdx;
    receiveWriteIndex = writeIdx;
}

void copyToReceiveBuffer(int8u* src, int8u length)
{
    // Copies data block to start of received data buffer.
    memcpy(receivedData, src, length);                  
}

void clearReceiveBuffer(void)
{
    // Clears received data buffer.
    memset(receivedData, 0x00, sizeof(receivedData));
           
    // Reset indexes.
    receiveReadIndex = 0;
    receiveWriteIndex = 0;
}

void emberAfPluginAuroraHostProtocolBootloadEventHandler(void)
{

}