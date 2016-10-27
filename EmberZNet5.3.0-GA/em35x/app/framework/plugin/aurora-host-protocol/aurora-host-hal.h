#ifndef _AURORA_HOST_HAL_H_
#define _AURORA_HOST_HAL_H_

#define RECEIVE_DATA_BUFFER_SIZE            0x7F

#define PLC_HOST_RESET_TIME                 100

void emberAfPluginAuroraHostHalResetPin(boolean state);
void emberAfPluginAuroraHostHalBootloadPin(boolean state);
void emberAfPluginAuroraHostProtocolIoInit(void);
void emberAfPluginAuroraHostProtocolResetEventHandler(void);
void emberAfPluginAuroraHostProtocolBootloadMode(void);
void emberAfPluginAuroraHostHalSerialInitHostBootload(void);
void emberAfPluginAuroraHostHalSerialInitHostPlc(void);

EmberStatus emberAfPluginAuroraHostHalPlcReadBytes(void);
void emberAfPluginAuroraHostHalPlcSendCommand(int8u *command, int8u length);
boolean emberAfPluginAuroraHostHalGetResponse(void *buffer, int16u *length);
void receiveWriteBuffer(int8u *data, int8u length);
int8u receiveReadBuffer(int8u *data, int8u length);
int8u peekReadBuffer(int8u *data, int8u length);

void setReceiveBufferIndexValues(int8u readIdx, int8u writeIdx);
void copyToReceiveBuffer(int8u* src, int8u length);
void clearReceiveBuffer(void);

void printControlState(void);

#endif // _AURORA_HOST_HAL_H_