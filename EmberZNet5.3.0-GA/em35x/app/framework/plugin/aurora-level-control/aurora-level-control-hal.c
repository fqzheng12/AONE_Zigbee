#include "app/framework/include/af.h"
#include "aurora-level-control-hal.h"

/** @brief Get the mode from the tokens
 *
 * read the tokens to find the mode
 *
 */
int8u getAuroraDimmerMode(void)
{
    int8u manufacturingToken[8];
    int8u mode;
    
    halCommonGetToken(&mode, TOKEN_VIRTUAL_DIP_SWITCH);
    if (mode == AURORA_MODE_INVALID) 
    {
        halCommonGetToken(manufacturingToken, TOKEN_MFG_EZSP_STORAGE);
        mode = manufacturingToken[0];
        if (mode == AURORA_MODE_INVALID) 
        {
            mode = AURORA_MODE_PLC;
        }
    }    
    
    return mode;
}

/** @brief Set the mode
 *
 * Set the mode token
 *
 */
int8u setAuroraDimmerMode(int8u mode)
{
    halCommonSetToken(TOKEN_VIRTUAL_DIP_SWITCH, &mode);
    
    return 0;
}
