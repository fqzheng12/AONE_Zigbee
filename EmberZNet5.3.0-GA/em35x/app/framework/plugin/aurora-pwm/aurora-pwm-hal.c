#include "app/framework/include/af.h"
#include "app/framework/include/af-types.h"
#include "aurora-pwm.h"
#include "aurora-pwm-hal.h"

#define PWM_MODE                0x00006060
#define PWM_OUTPUT_ENABLE       0x00001111
#define PWM_PSC_VALUE           6
#define PWM_ARR_VALUE           375

void emberAfPluginAuroraPwmInitialiseIo(void)
{

    PWM_RG_GPIO_CFG &= ~(PWM_RED_CFG_MASK);
    PWM_RG_GPIO_CFG &= ~(PWM_GREEN_CFG_MASK);

    PWM_BW_GPIO_CFG &= ~(PWM_BLUE_CFG_MASK);
    PWM_BW_GPIO_CFG &= ~(PWM_WHITE_CFG_MASK);

    PWM_RG_GPIO_CFG |= PWM_RED_CFG_OUTPUT;
    PWM_RG_GPIO_CFG |= PWM_GREEN_CFG_OUTPUT;
    
    PWM_BW_GPIO_CFG |= PWM_BLUE_CFG_OUTPUT;
    PWM_BW_GPIO_CFG |= PWM_WHITE_CFG_OUTPUT;
}

void emberAfPluginAuroraPwmInitializeTimer(void)
{
    emberAfPluginAuroraPwmInitialiseIo();
    
    TIM1_SMCR = 0;         // Disable slave mode
    
    TIM1_CCMR1 = PWM_MODE;
    TIM1_CCMR2 = PWM_MODE;
    
    TIM1_CCER = PWM_OUTPUT_ENABLE;

    TIM1_CCR1 = 0;
    TIM1_CCR2 = 0;
    TIM1_CCR3 = 0;
    TIM1_CCR4 = 0;
    
    TIM1_PSC = PWM_PSC_VALUE;
    TIM1_ARR = PWM_ARR_VALUE;
    
    TIM1_CR1 = TIM_ARBE | TIM_CEN;       // Initialise the control register
    TIM1_EGR = TIM_UG;
}

void pwmTimerEnable(void)
{
    TIM1_CR1 |= TIM_CEN;
}

void pwmTimerDisable(void)
{
    TIM1_CR1 &= ~(TIM_CEN);
}

void pwmSetDutyCycle(int8u output, int8u percentage)
{
    int32u ccrValue;
    
    ccrValue = (percentage * PWM_ARR_VALUE) / 100;
    
    switch (output) 
    {
        case PWM_OUTPUT_RED:
            TIM1_CCR4 = ccrValue;
            break;    
            
        case PWM_OUTPUT_GREEN:
            TIM1_CCR3 = ccrValue;
            break;        
            
        case PWM_OUTPUT_BLUE:
            TIM1_CCR1 = ccrValue;
            break;
            
        case PWM_OUTPUT_WHITE:
            TIM1_CCR2 = ccrValue;
            break;
        
    }
}


