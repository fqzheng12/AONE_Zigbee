#include <string.h>
#include "app/framework/include/af.h"
#include "aurora-host-hal.h" 
#include "aurora-host-protocol.h" 
#include "aurora-host-plc-protocol.h" 


#define UART_NOT_READY_NOTIFY_LIMIT         32     // gives a Uart ready TO of 8 seconds
#define MAX_NUM_PLC_CMD_RETRIES             3     

#define EXTERNAL_BUFFER_LENGTH              128     // For building zcl transparent cmds.

#define ZCL_PLC_ERROR_TYPE_ID               0xEE   // For reporting plc serial errors to gateway.
#define ZCL_PLC_UART_READY_TIMEOUT_ERROR    0x04
#define ZCL_PLC_RSP_TIMEOUT_ERROR           0x05    
#define ZCL_PLC_TUNNEL_RSP_ERROR            0x06
#define ZCL_PLC_UNSOLICITED_RSP_ERROR       0x07

EmberEventControl emberAfPluginAuroraHostProtocolPlcTimeoutEventControl;
void emberAfPluginAuroraHostProtocolPlcTimeoutEventHandler(void);

EmberEventControl emberAfPluginAuroraHostProtocolInterCommandDelayEventControl;
void emberAfPluginAuroraHostProtocolInterCommandDelayEventHandler(void);
void emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStart(boolean restart, int16u toMs);
void emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStop(void);
void emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(int8u* payload, int8u payloadLen);

static int8u plcCountUartNotReady = 0;

static int8u plcCalculateCrc(int8u* command);
static void plcBufferCommand(int8u* data);
static void plcSendCommand(void);

static int8u plcCmdBuffer[MAX_PLC_COMMAND_LENGTH];
static int8u plcCmdLength;
static int8u plcNumCmdRetries = 0;

static boolean plcEnableSend = TRUE;
static int8u plcRspBuffer[MAX_PLC_COMMAND_LENGTH];
static int8u plcRspLength;

static boolean plcSerialTimeOutActive = FALSE;


void plcSendTunnelCommand(int8u* data, int8u length)
{
    int8u command[MAX_PLC_COMMAND_LENGTH] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP2};
        
    emberAfDebugPrint("Tunnel command %d\r\n", length);    
    
    command[1] = length + 2;
    memcpy(&command[3], data, length);
    plcBufferCommand(command);
}

void plcSendOnCommand(void)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1,  AURORA_HOST_PLC_ON_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0};
      
    // Set the command length
    command[1] = 4;
    plcBufferCommand(command);
}

void plcSendOffCommand(void)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1, AURORA_HOST_PLC_OFF_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0};
      
    // Set the command length
    command[1] = 4;
    plcBufferCommand(command);
}

void plcSendSetPowerLevelCommand(int8u powerLevel)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1, AURORA_HOST_PLC_SET_POWER_LEVEL_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0, 0};
    
    // Set the command length
    command[1] = 5;

    command[5] = powerLevel;
    
    plcBufferCommand(command);
}

void plcSendSetColorCommand(int8u red, int8u green, int8u blue, int8u white)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1, AURORA_HOST_PLC_SET_COLOUR_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0, 0, 0, 0, 0};
    
    // Set the command length
    command[1] = 8;

    command[5] = red;
    command[6] = green;
    command[7] = blue;
    command[8] = white;
    
    plcBufferCommand(command);
}

void plcSendSetCctCommand(int8u cct)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1, AURORA_HOST_PLC_SET_CCT_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0, 0};
    
    // Set the command length
    command[1] = 5;

    command[5] = cct;
    
    plcBufferCommand(command);
}

void plcSendStatusRequestCommand(void)
{
    int8u command[] = {AURORA_HOST_PLC_START_BYTE, 0, AURORA_CMD_TYPE_EP1, AURORA_HOST_PLC_STATUS_REQUEST_COMMAND, AURORA_HOST_PLC_GLOBAL_ADDRESS, 0};
    
    // Set the command length
    command[1] = 4;

    plcBufferCommand(command);
}

void emberAfPluginAuroraHostHalPlcCheckHost(void)
{
    while (1)  // Loop until a PLC message is decoded or no valid PLC message is found.
    {              
        emberAfPluginAuroraHostHalPlcReadBytes();
               
        plcRspLength = peekReadBuffer(plcRspBuffer, MAX_PLC_COMMAND_LENGTH);                
        if (plcRspLength > 0) 
        {            
            emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStart(FALSE, 500);  // Start 500mS rx TO (dont restart if TO already running).
        }        
        if (plcRspLength < 2) 
        {
            return;  // Keep collecting PLC serial bytes.   
        }
        if (plcRspLength < plcRspBuffer[1] + 2) 
        {
            return;  // Keep collecting PLC serial bytes.
        }
        
        receiveReadBuffer(plcRspBuffer, plcRspLength);
                
        if (emberAfPluginAuroraHostHalPlcHandleReceivedData(plcRspBuffer, plcRspLength))
        {
            // Buffer processing completed.          
            emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStop();          
            return;  // terminate receive buffer process loop.
        }
        else
        {  
            emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStart(TRUE, 500);  // Start 500mS rx TO (always restart).
        }        
    }        
}

boolean emberAfPluginAuroraHostHalPlcHandleReceivedData(int8u* data, int8u size)
{
    int8u crc;
    int8u crcLocation;
    
    debugBuffer(data, size);
    
    // Check PLC message is at start of data buffer.
    crc = plcCalculateCrc(data);
    crcLocation = data[1] + 1;
    
    if (crc == data[crcLocation]) 
    {
        // Valid PLC message found.
        emberAfPluginAuroraHostHalPlcDecodeResponse(data, size);
        
        int8u nextMsgLocation = crcLocation + 1;
        
        if (nextMsgLocation < size)
        {
            // Handle any truncated data - copy any bytes following the decoded msg to start of the received data buffer. 
            int8u numTruncatedBytes = size - nextMsgLocation;            
            emberAfDebugPrintln("Msg decoded - handle %d truncated data bytes", numTruncatedBytes);       
            
            copyToReceiveBuffer(&data[nextMsgLocation], numTruncatedBytes);  
            
            // Modify receive buffer pointers so we can receive more serial data from host in-thread.
            setReceiveBufferIndexValues(0,                  // ReadIdx, always set back to start of buffer. 
                                        numTruncatedBytes); // WriteIdx            
            
            return FALSE; // == buffer processing NOT completed.
        }        
        else
        {
            // PLC msg decoded, no truncated data.
            emberAfDebugPrintln("Msg decoded - No truncated data");               
        }
    } 
    else 
    {
        emberAfDebugPrintln("plc resp CRC error 0x%x 0x%x %d", crc, data[crcLocation], crcLocation);           
    }        
    
    emberAfPluginAuroraHostHalPlcClearReceivedData();        
    return TRUE; // == buffer processing completed.   
}

void emberAfPluginAuroraHostHalPlcClearReceivedData(void)
{ 
    // Clear hal serial receive buffer.
    clearReceiveBuffer();  
}
  
void emberAfPluginAuroraHostHalPlcDecodeResponse(int8u* data, int8u size)
{  
    int8u responseType = data[2];
    
    switch (responseType) 
    {
        case AURORA_HOST_PLC_DATA_ACK:
        {
            emberAfPluginAuroraHostHalProcessDataAck(data, size);
            break;
        }            
        case AURORA_HOST_PLC_TUNNEL_RESPONSE:
        {
            emberAfPluginAuroraHostHalProcessTunnelResponse(data, size);
            break;
        }            
        case AURORA_HOST_PLC_STATUS_RESPONSE:
        {
            emberAfPluginAuroraHostHalProcessStatusResponse(data, size);
            break;
        }
        case AURORA_HOST_PLC_UNSOLICITED_RESPONSE:
        {
            emberAfPluginAuroraHostHalProcessUnsolicitedResponse(data, size);
            break;
        }            
    }
}

void emberAfPluginAuroraHostHalProcessDataAck(int8u* data, int8u size)
{
    int8u status = data[AURORA_PLC_CMD_ACK_STATUS];
    
    switch (status) 
    {
        case AURORA_PLC_CMD_ACK_STATUS_OK:
        {
            emberAfDebugPrintln("Data Ack: OK");                
            plcCmdLength = 0;  // Allows next plc cmd.   
   
            //GBHERE Push command via zcl transparent if the size is > ?? (i.e. an unsolicied Push cmd,
            // not a data ACK
            
            break;
        }            
        case AURORA_PLC_CMD_ACK_STATUS_CRC_ERROR:
        case AURORA_PLC_CMD_ACK_STATUS_UNKNOWN:
        {
            emberAfDebugPrintln("Data Ack: %s", (status == AURORA_PLC_CMD_ACK_STATUS_CRC_ERROR) ? "CRC Error" : "Unknown Error");    
            
            // Check if we should resend last plc cmd.
            if (plcCmdLength > 0)
            {            
                ++plcNumCmdRetries;
                if (plcNumCmdRetries < MAX_NUM_PLC_CMD_RETRIES)
                {
                    // Send retry cmd to host.
                    emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 10); // Send retry in 10mS.
                }
                else
                {   
                    plcCmdLength = 0;  // Allows next plc cmd.
                    
                    // Report CMD ACK TO ERROR to Gateway.
            
                    // Build a zcl error response.
                    int8u zclErrMsg[3];
                    int8u zclErrMsgLen = 3;
                    zclErrMsg[0] = ZCL_PLC_ERROR_TYPE_ID;  
                    zclErrMsg[1] = status;            // insert the error type.
                    zclErrMsg[2] = plcCmdBuffer[1];   // the cmd Id that timed-out.
                                
                    // Were we expecting a tunnel response?
                    if (plcCmdBuffer[2] == AURORA_CMD_TYPE_EP2)
                    {
                        // Send error msg to coo over tunnel. 
                        emberAfPluginAuroraHostProcessTunnelResponseCallback(zclErrMsg, zclErrMsgLen);
                    }
                    else
                    {
                        // Send error msg to coo using zcl transparent msp command. 
                        emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(zclErrMsg, zclErrMsgLen);
                    }
                }
            }            
            break;
        }            
        case AURORA_PLC_CMD_ACK_STATUS_BUSY:
        {
            emberAfDebugPrintln("Data Ack: Busy");    
            plcEnableSend = FALSE;
            emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 300);  // Dont allow a PLC cmd for 300mS.
            break;
        }            
        case AURORA_PLC_CMD_ACK_STARTUP_NOT_COMPLETE:
        {
            emberAfDebugPrintln("Data Ack: Startup Not Complete");    
            plcEnableSend = FALSE;
            emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 2000);  // Dont allow a PLC cmd for 2000mS.
            break;
        }    
        default:
        {
            emberAfDebugPrintln("Data Ack: Invalid");    
            break;
        }
    }
}

void emberAfPluginAuroraHostHalProcessTunnelResponse(int8u* data, int8u size)
{
    if (size >= 4) 
    {
        emberAfDebugPrintln("Tunnel Response OK: 0x%x bytes", size);            
        
        plcCmdLength = 0;  // Allows next plc cmd.
        
        // Format the response, discarding the header and CRC from the PLC Host protocol.
        // Add the length to the start of the response
        data[2] = size - 4;
        emberAfPluginAuroraHostProcessTunnelResponseCallback(&data[2], size - 3);
    }
    else
    {
        emberAfDebugPrintln("Tunnel Response error: 0x%x bytes", size);            
        
        // Check if we should resend last plc (tunnel) cmd to host.
        if (plcCmdLength > 0)
        {            
            ++plcNumCmdRetries;
            if (plcNumCmdRetries < MAX_NUM_PLC_CMD_RETRIES)
            {
                // Send retry cmd to host.
                emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 10); // Send retry in 10mS.
            }
            else
            {
                plcCmdLength = 0;  // Allows next plc cmd.
                
                // Build a zcl error response.
                int8u zclErrMsg[3];
                int8u zclErrMsgLen = 3;
                zclErrMsg[0] = ZCL_PLC_ERROR_TYPE_ID;  
                zclErrMsg[1] = ZCL_PLC_TUNNEL_RSP_ERROR; 
                zclErrMsg[2] = plcCmdBuffer[1];   // the cmd Id that gave the error rsp.
                        
                // Send error msg to coo over tunnel. 
                emberAfPluginAuroraHostProcessTunnelResponseCallback(zclErrMsg, zclErrMsgLen);            
            }
        }
    }
}

void emberAfPluginAuroraHostHalProcessUnsolicitedResponse(int8u* data, int8u size)
{
    if (size >= 4) 
    {
        emberAfDebugPrintln("Unsolicited Response OK: 0x%x bytes", size);            
                
        // Format the response, discarding the header and CRC from the PLC Host protocol.
        // Add the length to the start of the response and send to coo using zcl transparent msp command.         
        data[2] = size - 4;
        emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(&data[2], size-3);
    }
    else
    {
        emberAfDebugPrintln("Unsolicited Response error: 0x%x bytes", size);            

        // Note: No plc send msg retrying as we got an unsolicited response.
        
        // Build a zcl error response.
        int8u zclErrMsg[3];
        int8u zclErrMsgLen = 3;
        zclErrMsg[0] = ZCL_PLC_ERROR_TYPE_ID;  
        zclErrMsg[1] = ZCL_PLC_UNSOLICITED_RSP_ERROR; 
        zclErrMsg[2] = 0;   // the cmd Id that gave the error rsp (n/a since we got an unsolicited response).
                
        // Send error msg to coo using zcl transparent msp command. 
        emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(zclErrMsg, zclErrMsgLen);            
    }
}

void emberAfPluginAuroraHostHalProcessStatusResponse(int8u *data, int8u size)
{
    if (size >= 10) 
    {
        emberAfDebugPrintln("Status Response: 0x%x bytes", size);                       
    } 
    else 
    {
        emberAfDebugPrintln("Status Response error: 0x%x bytes", size);                       
    }
}

void emberAfPluginAuroraHostProtocolPlcTimeoutEventHandler(void)
{
    emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStop();

    // Clear the PLC receive buffer.
    emberAfPluginAuroraHostHalPlcClearReceivedData();
    
    emberAfDebugPrintln("PLC response TO"); 
        
    // Check if we should resend last plc cmd.
    if (plcCmdLength > 0)
    {            
        ++plcNumCmdRetries;
        if (plcNumCmdRetries < MAX_NUM_PLC_CMD_RETRIES)
        {
            // Send retry cmd to host.
            emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 10); // Send retry in 10mS.
        }
        else
        {
            plcCmdLength = 0;  // Allows next plc cmd.
            
            // Report CMD ACK TO ERROR to Gateway.
            
            // Build a zcl error response.
            int8u zclErrMsg[3];
            int8u zclErrMsgLen = 3;
            zclErrMsg[0] = ZCL_PLC_ERROR_TYPE_ID;  
            zclErrMsg[1] = ZCL_PLC_RSP_TIMEOUT_ERROR;
            zclErrMsg[2] = plcCmdBuffer[1];   // the cmd Id that timed-out.
                        
            // Were we expecting a tunnel response?
            if (plcCmdBuffer[2] == AURORA_CMD_TYPE_EP2)
            {
                // Send error msg to coo over tunnel. 
                emberAfPluginAuroraHostProcessTunnelResponseCallback(zclErrMsg, zclErrMsgLen);
            }
            else
            {
                // Send error msg to coo using zcl transparent msp command. 
                emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(zclErrMsg, zclErrMsgLen);
            }
        }
    }
}

static void plcBufferCommand(int8u *data)
{
    if (plcCmdLength > 0)
    {
        return;  // There's already a command waiting to be sent.
    }
    
    plcNumCmdRetries = 0;
    
    memcpy(plcCmdBuffer, data, data[1] + 1);
    plcCmdLength = data[1] + 2;

    plcCmdBuffer[plcCmdLength-1] = plcCalculateCrc(plcCmdBuffer);
    plcSendCommand();
}

static int8u plcCalculateCrc(int8u *command)
{
    int8u index;
    int8u crc = 0;
    
    for (index = 0; index < command[1] + 1; index++) 
    {
        crc ^= command[index];
    }
    
    return crc;
}

static void plcSendCommand(void)
{
    if (!plcEnableSend)
    {
        return;
    }
    
    // Check if Host uart is ready to receive data from module.   
    boolean plcUartReady = (HOST_UART_READY_IN & BIT(HOST_UART_READY));  
        
    if (plcUartReady) 
    {
        emberAfDebugPrintln("PLC plc cmd, retry=%d", plcNumCmdRetries);
        
        emberAfPluginAuroraHostHalPlcSendCommand(plcCmdBuffer, plcCmdLength);
        emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStart(TRUE, 2000);  // Set plc serial response (ack) TO = 2S.
    }
    else
    {
        plcEnableSend = FALSE;
        emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl, 250);  // Try to send the command again in 250mS.
        
        ++plcCountUartNotReady;        
        if (plcCountUartNotReady >= UART_NOT_READY_NOTIFY_LIMIT)
        {
            plcCountUartNotReady = 0;
            
            // Report PLC Uart Timeout to Gateway.
            
            // Build a zcl error response.
            int8u zclErrMsg[3];
            int8u zclErrMsgLen = 3;
            zclErrMsg[0] = ZCL_PLC_ERROR_TYPE_ID;  
            zclErrMsg[1] = ZCL_PLC_UART_READY_TIMEOUT_ERROR;
            zclErrMsg[2] = plcCmdBuffer[1];   // the cmd Id that timed-out.
                        
            // Were we expecting a tunnel response?
            if (plcCmdBuffer[2] == AURORA_CMD_TYPE_EP2)
            {
                // Send error msg to coo over tunnel. 
                emberAfPluginAuroraHostProcessTunnelResponseCallback(zclErrMsg, zclErrMsgLen);
            }
            else
            {
                // Send error msg to coo using zcl transparent msp command. 
                emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(zclErrMsg, zclErrMsgLen);
            }            
        }                       
    }    
}

void emberAfPluginAuroraHostProtocolInterCommandDelayEventHandler(void)
{
    emberEventControlSetInactive(emberAfPluginAuroraHostProtocolInterCommandDelayEventControl);
    
    plcEnableSend = TRUE;
    
    if (plcCmdLength > 0) 
    {
        plcSendCommand();  // Send any pending plc cmd.
    }
}

void emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStart(boolean restart, int16u toMs)
{
    if ((restart) || (!plcSerialTimeOutActive))
    { 
        plcSerialTimeOutActive = TRUE;
        
        // Start a timeout given value (mS).
        emberEventControlSetDelayMS(emberAfPluginAuroraHostProtocolPlcTimeoutEventControl, toMs);          
    }            
}

void emberAfPluginAuroraHostProtocolPlcSerialRxTimeoutStop(void)
{
    plcSerialTimeOutActive = FALSE;
    
    emberEventControlSetInactive(emberAfPluginAuroraHostProtocolPlcTimeoutEventControl);
}

void emberAfPluginAuroraHostProtocolSendZclTransparentResponseErrorMsg(int8u* payload, int8u payloadLen)
{
    int8u externalBuffer[EXTERNAL_BUFFER_LENGTH];
    int16u lenPtr;
    EmberApsFrame apsFrame;
    EmberStatus status;

    apsFrame.profileId = HA_PROFILE_ID;         
    apsFrame.sourceEndpoint = 1;
    apsFrame.destinationEndpoint = 1;
    apsFrame.options = EMBER_AF_DEFAULT_APS_OPTIONS | EMBER_APS_OPTION_RETRY;     
    apsFrame.sequence = emberAfNextSequence();
    
    emberAfSetExternalBuffer(externalBuffer, EXTERNAL_BUFFER_LENGTH, &lenPtr, &apsFrame);  
    emberAfFillCommandBasicClusterSendTransparentCommandResponse(payload, (int16u)payloadLen);      
    
    status = emberAfSendCommandUnicastToBindings();    
    if (status != EMBER_SUCCESS)
    {
         // Could not send via bindings so send unicast to hard coded dest=0000 (ep=01)          
        emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, 0x0000);  
    }                              
}
            


