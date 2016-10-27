//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include <string.h>
#include "app/framework/include/af.h"
#include "app/framework/plugin/aurora-button-joining/aurora-button-joining.h"
#include "app/framework/plugin/aurora-button-joining/aurora-button-joining-hal.h"
#include "app/framework/plugin/aurora-pwm/aurora-pwm.h"
#include "app/framework/plugin/aurora-pwm/aurora-pwm-hal.h"
#include "app/framework/plugin/aurora-colour-control/aurora-colour-control.h"
#include "app/framework/plugin/aurora-level-control/aurora-level-control-hal.h"
#include "app/framework/plugin/aurora-host-protocol/aurora-host-plc-protocol.h"
#include "app/framework/plugin/aurora-host-protocol/aurora-host-protocol.h"
#include "app/framework/plugin/aurora-host-protocol/aurora-host-hal.h"
#include "app/framework/plugin/aurora-colour-control/aurora-colour-conversion.h"
#include "app/framework/plugin/tunneling-server/tunneling-server.h"

typedef struct {
    boolean open;
    int16u tunnelId;
    int8u protocolId;
    int16u manufacturerCode;
    boolean flowControlSupport;
    int16u maximumIncomingTransferSize;
} tTunnelStatus;

tTunnelStatus tunnelStatus = {0};

/** @brief Move To Color - ZCL cmd callback
 *
 * @param colorX   Ver.: always
 * @param colorY   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfColorControlClusterMoveToColorCallback(int16u colorX,
                                                      int16u colorY,
                                                      int16u transitionTime)
{
    EmberAfStatus status = emberAfPluginAuroraColourControlMoveToColor(colorX, colorY, transitionTime);    
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.      
    return TRUE;
}

/** @brief Move Color - ZCL cmd callback
 *
 * @param rateX   Ver.: always
 * @param rateY   Ver.: always
 */
boolean emberAfColorControlClusterMoveColorCallback(int16s rateX,
                                                    int16s rateY)
{
    EmberAfStatus status = emberAfPluginAuroraColourControlMoveColor(rateX, rateY);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.      
    return TRUE;
}

/** @brief Step Color - ZCL cmd callback
 *
 * @param stepX   Ver.: always
 * @param stepY   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfColorControlClusterStepColorCallback(int16s stepX,
                                                    int16s stepY,
                                                    int16u transitionTime)
{
    EmberAfStatus status = emberAfPluginAuroraColourControlStepColor(stepX, stepY, transitionTime);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.      
    return TRUE;
}

/** @brief Move To Color Temperature
 *
 * 
 *
 * @param colorTemperature   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfColorControlClusterMoveToColorTemperatureCallback(int16u colorTemperature,
                                                                 int16u transitionTime)
{
    EmberAfStatus status =  emberAfPluginAuroraColourControlMoveToColorTemperature(colorTemperature, transitionTime);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.      
    return TRUE;
}

/** @brief Match Protocol Address
 *
 * @param protocolAddress   Ver.: always
 */
boolean emberAfGenericTunnelClusterMatchProtocolAddressCallback(int8u* protocolAddress)
{
  return FALSE;
}

/** @brief Image Block Response
 *
 * 
 *
 * @param status   Ver.: always
 * @param manufacturerId   Ver.: always
 * @param imageType   Ver.: always
 * @param fileVersion   Ver.: always
 * @param fileOffset   Ver.: always
 * @param dataSize   Ver.: always
 * @param imageData   Ver.: always
 */
boolean emberAfOtaBootloadClusterImageBlockResponseCallback(int8u status,
                                                            int16u manufacturerId,
                                                            int16u imageType,
                                                            int32u fileVersion,
                                                            int32u fileOffset,
                                                            int8u dataSize,
                                                            int8u* imageData)
{
  return FALSE;
}

/** @brief Upgrade End Response
 *
 * 
 *
 * @param manufacturerId   Ver.: always
 * @param imageType   Ver.: always
 * @param fileVersion   Ver.: always
 * @param currentTime   Ver.: always
 * @param upgradeTime   Ver.: always
 */
boolean emberAfOtaBootloadClusterUpgradeEndResponseCallback(int16u manufacturerId,
                                                            int16u imageType,
                                                            int32u fileVersion,
                                                            int32u currentTime,
                                                            int32u upgradeTime)
{
  return FALSE;
}

/** @brief Query Next Image Response
 *
 * 
 *
 * @param status   Ver.: always
 * @param manufacturerId   Ver.: always
 * @param imageType   Ver.: always
 * @param fileVersion   Ver.: always
 * @param imageSize   Ver.: always
 */
boolean emberAfOtaBootloadClusterQueryNextImageResponseCallback(int8u status,
                                                                int16u manufacturerId,
                                                                int16u imageType,
                                                                int32u fileVersion,
                                                                int32u imageSize)
{
  return FALSE;
}

/** @brief Main Start
 *
 * This function is called at the start of main after the HAL has been
 * initialized.  The standard main function arguments of argc and argv are
 * passed in.  However not all platforms have support for main() function
 * arguments.  Those that do not are passed NULL for argv, therefore argv should
 * be checked for NULL before using it.  If the callback determines that the
 * program must exit, it should return TRUE.  The value returned by main() will
 * be the value written to the returnCode pointer.  Otherwise the callback
 * should return FALSE to let normal execution continue.
 *
 * @param returnCode   Ver.: always
 * @param argc   Ver.: always
 * @param argv   Ver.: always
 */
boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
  // NOTE:  argc and argv may not be supported on all platforms, so argv MUST be
  // checked for NULL before referencing it.  On those platforms without argc 
  // and argv "0" and "NULL" are passed respectively.
  
  return FALSE;  // exit?
}

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup. Any
 * code that you would normally put into the top of the application's main()
 * routine should be put into this function.
 */
void emberAfMainInitCallback(void)
{  
    emberAfDebugPrintln("\r\n*** Aurora Light Dimmmer ***");    
    emberAfDebugPrintln("Module fw version: 0x%2X", EMBER_AF_PLUGIN_AURORA_OTA_CLIENT_POLICY_FIRMWARE_VERSION2);    
    
#ifdef AONE_HW_USE_JOIN_INPUT_ON_PB6
    emberAfDebugPrintln("AONE Hw, using alternate joining pin=PB6");
#endif

    int8u mode = getAuroraDimmerMode();  
    emberAfDebugPrint("AURORA_MODE=%d, ", mode);    
    switch (mode) 
    {
        case AURORA_MODE_RGBW:
        {
            emberAfDebugPrintln("RGBW");
            break;        
        }
        case AURORA_MODE_CX:
        {
            emberAfDebugPrintln("CX");
            break;        
        }
        case AURORA_MODE_DIM:
        {
            emberAfDebugPrintln("DIM");
            break;
        }    
        case AURORA_MODE_PLC:
        {
            emberAfDebugPrintln("PLC");
            break;
        }
        default:
        {
            emberAfDebugPrintln("Invalid");
            break;
        }
    }
    
    emberAfPluginAuroraButtonJoiningButtonIoInit();
    
    emberAfPluginAuroraButtonJoiningInitialiseLeds();
    
    emberAfPluginAuroraHostHalSerialInitHostPlc();

#ifndef AONE_HW_USE_JOIN_INPUT_ON_PB6
    emberAfPluginAuroraPwmInitializeTimer();  // Cant use pwm timer as this uses PB6.
#endif
    
    emberAfPluginAuroraColourControlInitColorSpaceConversion();      
}

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at the
 * end of the main tick execution.
 *
 */
void emberAfMainTickCallback(void)
{
    int8u mode;
    emberAfPluginAuroraButtonJoiningUpdateStatusLed();

    // Read any data from the host
    mode = getAuroraDimmerMode();
    switch(mode) 
    {

        case AURORA_MODE_PLC:
            emberAfPluginAuroraHostHalPlcCheckHost();
            break;
            
        case AURORA_MODE_RGBW:
        case AURORA_MODE_CX:
        case AURORA_MODE_DIM:
        default:
            break;
    }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
boolean emberAfStackStatusCallback(EmberStatus status)
{
    switch(status) 
    {
        case EMBER_NETWORK_UP:
            emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_JOINED, DEVICE_STATE_FLAGS_SET);
            emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_JOINING, DEVICE_STATE_FLAGS_CLEAR);
            emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_NETWORK_UNAVAILABLE, DEVICE_STATE_FLAGS_CLEAR); 
            
            emberAfOtaClientStartCallback(); 
            //GB dont need this on linkup!   emberAfPluginAuroraPwmInitializeTimer();
            break;

        case EMBER_NETWORK_DOWN:
            emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_CLEAR_MASK, DEVICE_STATE_FLAGS_CLEAR);
            break;

        case EMBER_JOIN_FAILED:
            emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_CLEAR_MASK, DEVICE_STATE_FLAGS_CLEAR); 
            break;

        default:
          break;
    }

    return FALSE;
}

/** @brief Finished
 *
 * This callback is fired when the network-find plugin is finished with the
 * forming or joining process.  The result of the operation will be returned in
 * the status parameter.
 *
 * @param status   Ver.: always
 */
void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
    if (status != EMBER_SUCCESS) 
    {
        emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_CLEAR_MASK , DEVICE_STATE_FLAGS_CLEAR);
    }
    
    emberAfPluginAuroraButtonJoiningClearJoiningFlag();
}


/** @brief Image Notify
 *
 * 
 *
 * @param payloadType   Ver.: always
 * @param queryJitter   Ver.: always
 * @param manufacturerId   Ver.: always
 * @param imageType   Ver.: always
 * @param newFileVersion   Ver.: always
 */
boolean emberAfOtaBootloadClusterImageNotifyCallback(int8u payloadType,
                                                     int8u queryJitter,
                                                     int16u manufacturerId,
                                                     int16u imageType,
                                                     int32u newFileVersion)
{
  return FALSE;
}

/** @brief callback for setting the level
 *
 * This controls the level of the light. Currently just recalculates the RGB values based 
 * on the new level and moves to that colour.
 */
void emberAfPluginAuroraLevelControlSetNewLevelCallback(int8u newLevel)
{
    emberAfDebugPrintln("SetNewLevel %d", newLevel);
    
    int8u mode = getAuroraDimmerMode();    
    //emberAfDebugPrintln("AURORA_MODE=%d", mode);    
    switch (mode) 
    {
        case AURORA_MODE_RGBW:
        case AURORA_MODE_CX:
        case AURORA_MODE_DIM:
        {
            emberAfPluginAuroraColourControlSetNewLevel();
            break;
        }    
        case AURORA_MODE_PLC:
        {
            plcSendSetPowerLevelCommand(newLevel);
            emberAfPluginAuroraColourControlSetNewLevel();
            break;
        }    
        default:
        {
            emberAfDebugPrint("inv mode (0x%x)\r\n", mode);
            break;
        }
    }
}

/** @brief Set the colour on the LED
 *
 * Set the RGB colour on the led.
 */
void emberAfPluginAuroraColourControlSetRgbColorCallback(t_RGB* RGB)
{
    emberAfDebugPrintln("Set Color");
    
    int8u mode = getAuroraDimmerMode();  
    //emberAfDebugPrintln("AURORA_MODE=%d", mode);    
    switch (mode) 
    {
        case AURORA_MODE_RGBW:
        case AURORA_MODE_CX:
        case AURORA_MODE_DIM:
        {
            emberAfPluginAuroraPwmSetDutyCycle(RGB);
            emberAfPluginAuroraHostProtocolSetRgbColor(RGB);            
            break;
        }    
        case AURORA_MODE_PLC:
        {
            emberAfPluginAuroraPwmSetDutyCycle(RGB);
            plcSendSetColorCommand(RGB->R, RGB->G, RGB->B, RGB->W);            
            break;
        }   
        default:
        {
            emberAfDebugPrint("inv mode (0x%x)\r\n", mode);
            break;
        }
    }
}

void emberAfOnOffClusterSetNewValueCallback(boolean newValue)
{
    emberAfDebugPrint("SET ON/OFF\r\n");
    
    int8u mode = getAuroraDimmerMode();        
    //emberAfDebugPrintln("AURORA_MODE=%d", mode);    
    switch (mode) 
    {
        case AURORA_MODE_RGBW:
        case AURORA_MODE_CX:
        case AURORA_MODE_DIM:
        {
            emberAfPluginAuroraColourControlSetOnOff(newValue);            
            break;
        }    
        case AURORA_MODE_PLC:
        {
            emberAfPluginAuroraColourControlSetOnOff(newValue);
            if (newValue) 
            {
                plcSendOnCommand();
            } 
            else 
            {
                plcSendOffCommand();
            }
            
            break;
        }    
        default:
        {
            emberAfDebugPrintln("inv mode %d", mode);
            break;
        }
    }
}

#define EMBER_ZCL_TUNNELING_PROTOCOL_ID_AURORA_PLC               0xC8

/** @brief Is Protocol Supported
 *
 * This function is called by the Tunneling server plugin whenever a Request
 * Tunnel command is received.  The application should return TRUE if the
 * protocol is supported and FALSE otherwise.
 *
 * @param protocolId The identifier of the metering communication protocol for
 * which the tunnel is requested.  Ver.: always
 * @param manufacturerCode The manufacturer code for manufacturer-defined
 * protocols or 0xFFFF in unused.  Ver.: always
 */
boolean emberAfPluginAuroraTunnelingServerIsProtocolSupportedCallback(int8u protocolId,
                                                                      int16u manufacturerCode)
{
    if ((protocolId == EMBER_ZCL_TUNNELING_PROTOCOL_ID_AURORA_PLC) && 
        (manufacturerCode == EMBER_AF_MANUFACTURER_CODE)) 
    {
        return TRUE;
    }

  return FALSE;
}

/** @brief Data Received
 *
 * This function is called by the Tunneling server plugin whenever data is
 * received from a client through a tunnel.
 *
 * @param tunnelId The identifier of the tunnel through which the data was
 * received.  Ver.: always
 * @param data Buffer containing the raw octets of the data.  Ver.: always
 * @param dataLen The length in octets of the data.  Ver.: always
 */
void emberAfPluginAuroraTunnelingServerDataReceivedCallback(int16u tunnelId,
                                                            int8u* data,
                                                            int16u dataLen)
{
    // Strip off the length byte[0] that is included by the CICIE firmware
    dataLen--;
    plcSendTunnelCommand(&data[1], (int8u)dataLen);
}

/** @brief Tunnel Closed
 *
 * This function is called by the Tunneling server plugin whenever a tunnel is
 * closed.  Clients may close tunnels by sending a Close Tunnel command.  The
 * server can preemptively close inactive tunnels after a timeout.
 *
 * @param tunnelId The identifier of the tunnel that has been closed.  Ver.:
 * always
 * @param clientInitiated TRUE if the client initiated the closing of the tunnel
 * or FALSE if the server closed the tunnel due to inactivity.  Ver.: always
 */
void emberAfPluginAuroraTunnelingServerTunnelClosedCallback(int16u tunnelId,
                                                            boolean clientInitiated)
{
    memset((void*)&tunnelStatus, 0, sizeof(tTunnelStatus));
}

/** @brief Tunnel Opened
 *
 * This function is called by the Tunneling server plugin whenever a tunnel is
 * opened.  Clients may open tunnels by sending a Request Tunnel command.
 *
 * @param tunnelId The identifier of the tunnel that has been opened.  Ver.:
 * always
 * @param protocolId The identifier of the metering communication protocol for
 * the tunnel.  Ver.: always
 * @param manufacturerCode The manufacturer code for manufacturer-defined
 * protocols or 0xFFFF in unused.  Ver.: always
 * @param flowControlSupport TRUE is flow control support is requested or FALSE
 * if it is not.  Ver.: always
 * @param maximumIncomingTransferSize The maximum incoming transfer size of the
 * client.  Ver.: always
 */
void emberAfPluginAuroraTunnelingServerTunnelOpenedCallback(int16u tunnelId,
                                                            int8u protocolId,
                                                            int16u manufacturerCode,
                                                            boolean flowControlSupport,
                                                            int16u maximumIncomingTransferSize)
{
    // Indicate that the tunnel is open and save tunnel data locally
    tunnelStatus.open = TRUE;
    tunnelStatus.tunnelId = tunnelId;
    tunnelStatus.protocolId = protocolId;
    tunnelStatus.manufacturerCode = manufacturerCode;
    tunnelStatus.flowControlSupport = flowControlSupport;
    tunnelStatus.maximumIncomingTransferSize = maximumIncomingTransferSize;
}

/** @brief Tunnel Server to Client transfer
 *
 * If the tunnel is open, sends tunnel data to client
 */
void emberAfPluginAuroraHostProcessTunnelResponseCallback(int8u *data, int8u size)
{
    // The data includes the length byte at the start.
    if (tunnelStatus.open) 
    {
        emberAfPluginTunnelingServerTransferData(tunnelStatus.tunnelId, data, size);
    }
}

void calculateEventDuration_and_stepSize(int16u transitionTimeMs, int16u amount, int32u* eventDurationMs, int16u* stepSize) 
{
    // Helper fn calculates event duration & step size for given transition time & amount.
       
    #define MAX_EVENT_DURATION_MS    MILLISECOND_TICKS_PER_SECOND * 5     // Define a realistic maximum event duration.
    
    int16u noOfSteps;
       
    if (amount == 0)
    {
        emberAfDebugPrintln("invalid amount");
        *eventDurationMs = 0; 
        *stepSize = 0;        
        return;
    }
    
    if (transitionTimeMs == 0)
    {
        *eventDurationMs = 0; // Will schedule a single event.
        *stepSize = amount;
        noOfSteps = 1;
    }
    else
    {
        *eventDurationMs = transitionTimeMs / amount; 
        
        int32u minEventDuration = MILLISECOND_TICKS_PER_DECISECOND;  //100mS
        int32u maxEventDuration = MAX_EVENT_DURATION_MS;  // apply a realistic maximum event duration.        
        if (getAuroraDimmerMode() == AURORA_MODE_DIM)
        {
            minEventDuration = (MILLISECOND_TICKS_PER_DECISECOND * 3);  //300mS
        }
        
        if (*eventDurationMs < minEventDuration)
        {
            *eventDurationMs = minEventDuration;
        }
        else
        if (*eventDurationMs > maxEventDuration)
        {
            *eventDurationMs = maxEventDuration;
        }
        
        while (*eventDurationMs < maxEventDuration)
        {    
            noOfSteps = transitionTimeMs / *eventDurationMs;  
            if (noOfSteps < 1)
            {
                noOfSteps = 1;   // apply min no of steps.
            }   
            *stepSize = amount / noOfSteps;        
            if (*stepSize != 0)
            {
                break;      
            }        
            *eventDurationMs += 100;  // Add 100ms to event duration and recalculate steps.
        }  
        
        if (*stepSize == 0)
        {
            *stepSize = 1;  // Finally apply minimum limit to step size.
        }    
    }
    
    //emberAfDebugPrintln("move cfg: eventDurationMs=%d, noOfSteps=%d, stepSize=%d", *eventDurationMs, noOfSteps, *stepSize);
}
