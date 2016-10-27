#define AURORA_MODE_RGBW           0x00
#define AURORA_MODE_CX             0x01
#define AURORA_MODE_DIM            0x02
#define AURORA_MODE_PLC            0x03
#define AURORA_MODE_INVALID        0xFF

int8u getAuroraDimmerMode(void);
int8u setAuroraDimmerMode(int8u mode);
