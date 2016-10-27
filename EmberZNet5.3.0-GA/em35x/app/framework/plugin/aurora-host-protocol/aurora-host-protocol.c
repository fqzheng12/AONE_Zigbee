#include <string.h>
#include "aurora-host-protocol.h" 
#include "../aurora-level-control/aurora-level-control-hal.h" 

static void auroraSendCommand(int8u* command);

static void auroraSendCommand(int8u* command)
{   
    int8u mode = getAuroraDimmerMode();    
    
    if ((mode != AURORA_MODE_RGBW) &&
        (mode != AURORA_MODE_CX) &&
        (mode != AURORA_MODE_DIM))
    {
        return;  // Invalid device mode for sending a host serial msg.      
    }
     
    command[0] = AURORA_HOST_START_CODE;  
    
    command[1] = mode;
    
    for (int8u i=0; i<8; i++) 
    {
        command[8] ^= command[i];        
    }
    
    command[9] = AURORA_HOST_END_CODE;

    debugBuffer(command, 10);    
    
    for (int8u i=0; i<AURORA_HOST_SEND_TIMES; i++) 
    {
        emberSerialWriteData(HOST_SERIAL_PORT, command, 10); 
    }
}

void emberAfPluginAuroraHostProtocolSetRgbColor(t_RGB* RGB)
{
    int8u command[10];
    
    memset(command, 0, 10);
    
    command[2] = RGB->R;
    command[3] = RGB->G;
    command[4] = RGB->B;
    command[5] = RGB->W;
        
    auroraSendCommand(command);
}

/** @brief Printf buffer
 * Prints the give number of bytes for debug
 */
void debugBuffer(int8u* buffer, int8u count)
{
    int8u index;
 
    emberAfDebugPrint("Buffer: %d: ", count);
    for (index = 0; index < count; index++) 
    {
        emberAfDebugPrint(" %x", buffer[index]);
    }
    emberAfDebugPrint("\r\n");
}
