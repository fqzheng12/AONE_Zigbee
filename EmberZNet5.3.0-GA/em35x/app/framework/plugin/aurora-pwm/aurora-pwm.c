#include "app/framework/include/af.h"
#include "app/framework/include/af-types.h"
#include "aurora-pwm.h"
#include "aurora-pwm-hal.h"
#include "../aurora-level-control/aurora-level-control-hal.h"

#define DUTY_CYCLE_PERCENT(value) (int8u)(((int16u)value * 100) / 255)

static boolean pwmEnable = FALSE;   // Store pwm On/Off enable state.

void emberAfPluginAuroraPwmSetDutyCycle(t_RGB *rgb) 
{
    // Sets the RGBW pwm duty cycle values to the hw pwm pins.
  
    if (getAuroraDimmerMode() != AURORA_MODE_RGBW)
    {
        return;
    }
        
    // If pwmEnable flag is false 0% duty cycle is set on all pwm pins.  
    if (!pwmEnable)
    {
        rgb->R = 0;
        rgb->G = 0;
        rgb->B = 0;
        rgb->W = 0;        
    }
  
    pwmSetDutyCycle(PWM_OUTPUT_RED, DUTY_CYCLE_PERCENT(rgb->R));
    pwmSetDutyCycle(PWM_OUTPUT_GREEN, DUTY_CYCLE_PERCENT(rgb->G));
    pwmSetDutyCycle(PWM_OUTPUT_BLUE, DUTY_CYCLE_PERCENT(rgb->B));    
    pwmSetDutyCycle(PWM_OUTPUT_WHITE, DUTY_CYCLE_PERCENT(rgb->W));    
 
    emberAfDebugPrintln("pwm RGBW%%: r=%d, g=%d, b=%d, w=%d\r\n", DUTY_CYCLE_PERCENT(rgb->R), DUTY_CYCLE_PERCENT(rgb->G), DUTY_CYCLE_PERCENT(rgb->B), DUTY_CYCLE_PERCENT(rgb->W));   
}

void emberAfPluginAuroraPwmEnable(boolean newState) 
{
    pwmEnable = newState;    
}
