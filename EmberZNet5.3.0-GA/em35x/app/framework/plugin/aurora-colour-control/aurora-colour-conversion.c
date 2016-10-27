/******************************************************************************
  Copyright (c) 2010 Ryan Juckett
  http://www.ryanjuckett.com/
  
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
  
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  
  3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/
#include "app/framework/include/af.h"
#include "aurora-colour-conversion.h"
#include "aurora-colour-conversion-table.h"

// define chromaticity coordinates for sRGB space
tVec2 sRGB_red_xy   = { 0.64f, 0.33f };
tVec2 sRGB_green_xy = { 0.30f, 0.60f };
tVec2 sRGB_blue_xy  = { 0.15f, 0.06f };
tVec2 sRGB_white_xy = { 0.3127f, 0.3290f };

// Conversion matrices between linear sRGB space and XYZ space
tMat3x3 convert_sRGB_to_XYZ;
tMat3x3 convert_XYZ_to_sRGB;
 
void PrintVect(tVec3* myColor, char* msg);
void Convert_XYZ_To_Yxy(tVec2* Yxy_Color, tVec3* XYZ_Color);
void Convert_Yxy_To_XYZ(tVec3* XYZ_Color, tVec2* Yxy_Color, float Y);
void PrintVect3(tVec3* myColor, char* msg);
void PrintVect2(tVec2* myColor, char* msg);
void xyYtoRGBConversion(float currentX, float currentY, float currentZ);

//******************************************************************************
// Set an indexed matrix column to a given vector.
//******************************************************************************
void Mat_SetCol(tMat3x3 *pMat, int colIdx, const tVec3 *vec)
{
    pMat->m[0][colIdx] = vec->x;
    pMat->m[1][colIdx] = vec->y;
    pMat->m[2][colIdx] = vec->z;
}
  
//******************************************************************************
// Calculate the inverse of a 3x3 matrix. Return false if it is non-invertible.
//******************************************************************************
boolean Mat_Invert(tMat3x3 *pOutMat, const tMat3x3 *inMat)
{
    // calculate the minors for the first row
    float minor00 = inMat->m[1][1]*inMat->m[2][2] - inMat->m[1][2]*inMat->m[2][1];
    float minor01 = inMat->m[1][2]*inMat->m[2][0] - inMat->m[1][0]*inMat->m[2][2];
    float minor02 = inMat->m[1][0]*inMat->m[2][1] - inMat->m[1][1]*inMat->m[2][0];
  
    // calculate the determinant
    float determinant =   inMat->m[0][0] * minor00
                        + inMat->m[0][1] * minor01
                        + inMat->m[0][2] * minor02;
  
    // check if the input is a singular matrix (non-invertable)
    // (note that the epsilon here was arbitrarily chosen)
    if( determinant > -0.000001f && determinant < 0.000001f )
        return FALSE;
  
    // the inverse of inMat is (1 / determinant) * adjoint(inMat)
    float invDet = 1.0f / determinant;
    pOutMat->m[0][0] = invDet * minor00;
    pOutMat->m[0][1] = invDet * (inMat->m[2][1]*inMat->m[0][2] - inMat->m[2][2]*inMat->m[0][1]);
    pOutMat->m[0][2] = invDet * (inMat->m[0][1]*inMat->m[1][2] - inMat->m[0][2]*inMat->m[1][1]);
  
    pOutMat->m[1][0] = invDet * minor01;
    pOutMat->m[1][1] = invDet * (inMat->m[2][2]*inMat->m[0][0] - inMat->m[2][0]*inMat->m[0][2]);
    pOutMat->m[1][2] = invDet * (inMat->m[0][2]*inMat->m[1][0] - inMat->m[0][0]*inMat->m[1][2]);
  
    pOutMat->m[2][0] = invDet * minor02;
    pOutMat->m[2][1] = invDet * (inMat->m[2][0]*inMat->m[0][1] - inMat->m[2][1]*inMat->m[0][0]);
    pOutMat->m[2][2] = invDet * (inMat->m[0][0]*inMat->m[1][1] - inMat->m[0][1]*inMat->m[1][0]);
  
    return TRUE;
}
  
//******************************************************************************
// Multiply a column vector on the right of a 3x3 matrix.
//******************************************************************************
void Mat_MulVec( tVec3 * pOutVec, const tMat3x3 *mat, const tVec3 inVec )
{
    pOutVec->x = mat->m[0][0]*inVec.x + mat->m[0][1]*inVec.y + mat->m[0][2]*inVec.z;
    pOutVec->y = mat->m[1][0]*inVec.x + mat->m[1][1]*inVec.y + mat->m[1][2]*inVec.z;
    pOutVec->z = mat->m[2][0]*inVec.x + mat->m[2][1]*inVec.y + mat->m[2][2]*inVec.z;
}
 
//******************************************************************************
// Convert an sRGB color channel to a linear sRGB color channel.
//******************************************************************************
float LookUpTableCalcGammaExpand_sRGB(float nonlinear)
{    
    int index;

    for (index = 0; index < 255; index++) 
    {
        if (GammaConversionTable[index].nonlinear > nonlinear) 
        {
            break;
        }
    }
        
    return GammaConversionTable[index].linear;
}
  
//******************************************************************************
// Convert a linear sRGB color channel to a sRGB color channel.
//******************************************************************************
float LookUpTableGammaCompress_sRGB(float linear)
{    
    int index;

    for (index = 0; index < 255; index++) 
    {
        if (GammaConversionTable[index].linear > linear) 
        {
            break;
        }
    }
        
    return GammaConversionTable[index].nonlinear;
}

//******************************************************************************
// Convert an sRGB color to a linear sRGB color.
//******************************************************************************
void LookUpGammaExpand_sRGB(tVec3 * pColor)
{
    pColor->x = LookUpTableCalcGammaExpand_sRGB( pColor->x );
    pColor->y = LookUpTableCalcGammaExpand_sRGB( pColor->y );
    pColor->z = LookUpTableCalcGammaExpand_sRGB( pColor->z );
}
  
//******************************************************************************
// Convert a linear sRGB color to an sRGB color.
//******************************************************************************
void LookUpGammaCompress_sRGB(tVec3 * pColor)
{
    pColor->x = LookUpTableGammaCompress_sRGB( pColor->x );
    pColor->y = LookUpTableGammaCompress_sRGB( pColor->y );
    pColor->z = LookUpTableGammaCompress_sRGB( pColor->z );
}
 
//******************************************************************************
// Convert a linear sRGB color to an sRGB color 
//******************************************************************************
void CalcColorSpaceConversion_RGB_to_XYZ
(
    tMat3x3 *pOutput,  // conversion matrix
    const tVec2 red_xy,   // xy chromaticity coordinates of the red primary
    const tVec2 green_xy, // xy chromaticity coordinates of the green primary
    const tVec2 blue_xy,  // xy chromaticity coordinates of the blue primary
    const tVec2 white_xy  // xy chromaticity coordinates of the white point
)
{
    // generate xyz chromaticity coordinates (x + y + z = 1) from xy coordinates
    tVec3 r = { red_xy.x,   red_xy.y,   1.0f - (red_xy.x + red_xy.y) };
    tVec3 g = { green_xy.x, green_xy.y, 1.0f - (green_xy.x + green_xy.y) };
    tVec3 b = { blue_xy.x,  blue_xy.y,  1.0f - (blue_xy.x + blue_xy.y) };
    tVec3 w = { white_xy.x, white_xy.y, 1.0f - (white_xy.x + white_xy.y) };
  
    // Convert white xyz coordinate to XYZ coordinate by letting that the white
    // point have and XYZ relative luminance of 1.0. Relative luminance is the Y
    // component of and XYZ color.
    //   XYZ = xyz * (Y / y) 
    w.x /= white_xy.y;
    w.y /= white_xy.y;
    w.z /= white_xy.y;
  
    // Solve for the transformation matrix 'M' from RGB to XYZ
    // * We know that the columns of M are equal to the unknown XYZ values of r, g and b.
    // * We know that the XYZ values of r, g and b are each a scaled version of the known
    //   corresponding xyz chromaticity values.
    // * We know the XYZ value of white based on its xyz value and the assigned relative
    //   luminance of 1.0.
    // * We know the RGB value of white is (1,1,1).
    //                  
    //   white_XYZ = M * white_RGB
    //
    //       [r.x g.x b.x]
    //   N = [r.y g.y b.y]
    //       [r.z g.z b.z]
    //
    //       [sR 0  0 ]
    //   S = [0  sG 0 ]
    //       [0  0  sB]
    //
    //   M = N * S
    //   white_XYZ = N * S * white_RGB
    //   N^-1 * white_XYZ = S * white_RGB = (sR,sG,sB)
    //
    // We now have an equation for the components of the scale matrix 'S' and
    // can compute 'M' from 'N' and 'S'
  
    Mat_SetCol( pOutput, 0, &r );
    Mat_SetCol( pOutput, 1, &g );
    Mat_SetCol( pOutput, 2, &b );
  
    tMat3x3 invMat;
    Mat_Invert( &invMat, pOutput );
  
    tVec3 scale;
    Mat_MulVec( &scale, &invMat, w );
  
    pOutput->m[0][0] *= scale.x;
    pOutput->m[1][0] *= scale.x;
    pOutput->m[2][0] *= scale.x;
  
    pOutput->m[0][1] *= scale.y;
    pOutput->m[1][1] *= scale.y;
    pOutput->m[2][1] *= scale.y;
  
    pOutput->m[0][2] *= scale.z;
    pOutput->m[1][2] *= scale.z;
    pOutput->m[2][2] *= scale.z;
}

//******************************************************************************
// Example of using the color space conversion functions
//******************************************************************************
void emberAfPluginAuroraColourControlInitColorSpaceConversion(void)
{
    
    CalcColorSpaceConversion_RGB_to_XYZ(&convert_sRGB_to_XYZ,
                                        sRGB_red_xy,
                                        sRGB_green_xy,
                                        sRGB_blue_xy,
                                        sRGB_white_xy);
  
    // generate conversion matrix from XYZ space to linear sRGB space
    Mat_Invert(&convert_XYZ_to_sRGB, &convert_sRGB_to_XYZ);  
}

void emberAfPluginAuroraColourControlConvertColor(tVec2* Yxy_Color, float Y, tVec3* RGB_Color)
{
    tVec3 XYZ_Color;
    
    Convert_Yxy_To_XYZ(&XYZ_Color, Yxy_Color, Y);
    
    //GB PrintVect3(&XYZ_Color, "Original color");
    
    // convert from XYZ to linear sRGB
    Mat_MulVec(RGB_Color, &convert_XYZ_to_sRGB, XYZ_Color);
    //GB PrintVect3(RGB_Color, "Linear sRGB Color");

    LookUpGammaCompress_sRGB(RGB_Color);
    //GB PrintVect3(RGB_Color, "Gamma-corrected sRGB Color (Look Up)");
}

// convert from XYZ to xyz
// Y = Y
// x = X / (X + Y + Z)
// y = Y / (X + Y + Z)
void Convert_XYZ_To_Yxy(tVec2 * Yxy_Color, tVec3 * XYZ_Color)
{
    Yxy_Color->x = (XYZ_Color->x / (XYZ_Color->x + XYZ_Color->y + XYZ_Color->z));
    Yxy_Color->y = (XYZ_Color->y / (XYZ_Color->x + XYZ_Color->y + XYZ_Color->z));
    
    //GB PrintVect3(XYZ_Color, "XYZ_Color");
    //GB PrintVect2(Yxy_Color, "Yxy_Color");
}

// Convert from xyz to XYZ
// X = x * (Y / y)
// Y = Y
// Z = (1 - x - y) * (Y / y)
void Convert_Yxy_To_XYZ(tVec3* XYZ_Color, tVec2* Yxy_Color, float Y)
{
    XYZ_Color->y = Y;
    XYZ_Color->x = Yxy_Color->x * (XYZ_Color->y / Yxy_Color->y);
    XYZ_Color->z = (1 - Yxy_Color->x - Yxy_Color->y ) * (XYZ_Color->y / Yxy_Color->y);
    
    //GB PrintVect2(Yxy_Color, "Yxy_Color");
    //GB PrintVect3(XYZ_Color, "XYZ_Color");
}
 
void PrintVect3(tVec3*myColor, char* msg)
{
    emberAfDebugPrint("%s:\tColor %d (%x) %d (%x) %d (%x)\r\n", msg, (int)(myColor->x * 1000), (int)(myColor->x * 255), (int)(myColor->y * 1000), (int)(myColor->y * 255), (int)(myColor->z * 1000), (int)(myColor->z * 255));
}
 
void PrintVect2(tVec2*myColor, char* msg)
{
    emberAfDebugPrint("%s:\tColor %d (%x) %d (%x)\r\n", msg, (int)(myColor->x * 1000), (int)(myColor->x * 255), (int)(myColor->y * 1000), (int)(myColor->y * 255));
}
 
