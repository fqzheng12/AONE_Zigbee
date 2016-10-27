#define AURORA_CMD_TYPE_EP1                                 0x00
#define AURORA_CMD_TYPE_EP2                                 0x01

#define AURORA_HOST_PLC_START_BYTE                          0xAA

#define AURORA_HOST_PLC_ON_COMMAND                          0x10
#define AURORA_HOST_PLC_OFF_COMMAND                         0x11
#define AURORA_HOST_PLC_SET_POWER_LEVEL_COMMAND             0x20
#define AURORA_HOST_PLC_SET_COLOUR_COMMAND                  0x21
#define AURORA_HOST_PLC_SET_CCT_COMMAND                     0x22
#define AURORA_HOST_PLC_STATUS_REQUEST_COMMAND              0x00

#define AURORA_HOST_PLC_UNSOLICITED_RESPONSE                0x00
#define AURORA_HOST_PLC_DATA_ACK                            0xFF
#define AURORA_HOST_PLC_TUNNEL_RESPONSE                     0x81
#define AURORA_HOST_PLC_STATUS_RESPONSE                     0x80

#define AURORA_HOST_PLC_GLOBAL_ADDRESS                      0x00

#define AURORA_PLC_CMD_ACK_STATUS                           0x04
#define AURORA_PLC_CMD_ACK_STATUS_OK                        0x00
#define AURORA_PLC_CMD_ACK_STATUS_CRC_ERROR                 0x01
#define AURORA_PLC_CMD_ACK_STATUS_BUSY                      0x02
#define AURORA_PLC_CMD_ACK_STATUS_UNKNOWN                   0x03    
#define AURORA_PLC_CMD_ACK_STARTUP_NOT_COMPLETE             0x04    

#define MAX_PLC_COMMAND_LENGTH                              64

void plcSendTunnelCommand(int8u *data, int8u length);
void plcSendOnCommand(void);
void plcSendOffCommand(void);
void plcSendSetPowerLevelCommand(int8u powerLevel);
void plcSendSetColorCommand(int8u red, int8u green, int8u blue, int8u white);
void plcSendSetCctCommand(int8u cct);
void plcSendStatusRequestCommand(void);

void emberAfPluginAuroraHostHalPlcCheckHost(void);
boolean emberAfPluginAuroraHostHalPlcHandleReceivedData(int8u* data, int8u size);
void emberAfPluginAuroraHostHalPlcClearReceivedData(void);
void emberAfPluginAuroraHostHalPlcDecodeResponse(int8u* data, int8u size);

void emberAfPluginAuroraHostHalProcessDataAck(int8u* data, int8u size);
void emberAfPluginAuroraHostHalProcessTunnelResponse(int8u* data, int8u size);
void emberAfPluginAuroraHostHalProcessStatusResponse(int8u* data, int8u size);
void emberAfPluginAuroraHostProcessTunnelResponseCallback(int8u* data, int8u size);
void emberAfPluginAuroraHostHalProcessUnsolicitedResponse(int8u* data, int8u size);
