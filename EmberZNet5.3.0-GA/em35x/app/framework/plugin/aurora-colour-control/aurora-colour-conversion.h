#ifndef _AURORA_COLOUR_CONVERSION__H_ 
#define _AURORA_COLOUR_CONVERSION__H_


//******************************************************************************
// 2-dimensional vector.
//******************************************************************************
typedef struct { 
    float x;
    float y; 
} tVec2 ;
  
//******************************************************************************
// 3-dimensional vector.
//******************************************************************************
typedef struct { 
    float x;
    float y;
    float z;
} tVec3;
  
//******************************************************************************
// 3x3 matrix
//******************************************************************************
typedef struct { 
    float m[3][3]; 
} tMat3x3;

typedef struct {
    float linear;
    float nonlinear;
    unsigned char hex;
} tGammaConversion;


void emberAfPluginAuroraColourControlInitColorSpaceConversion(void);
void emberAfPluginAuroraColourControlConvertColor(tVec2 *Yxy_Color, float Y, tVec3 *RGB_Color);

#endif // _AURORA_COLOUR_CONVERSION__H_