#ifndef _AURORA_HOST_PROTOCOL_H_
#define  _AURORA_HOST_PROTOCOL_H_
#include "app/framework/include/af.h"
#include "../aurora-colour-control/aurora-colour-control.h"

#define AURORA_HOST_START_CODE          0xAA
#define AURORA_HOST_END_CODE            0x55
 
#define AURORA_HOST_SEND_TIMES          1

void emberAfPluginAuroraHostProtocolSetRgbColor(t_RGB *RGB);
void debugBuffer(int8u *buffer, int8u count);

#endif // _AURORA_HOST_PROTOCOL_H_