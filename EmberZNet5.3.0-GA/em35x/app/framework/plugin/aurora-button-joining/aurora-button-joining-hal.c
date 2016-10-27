#include "app/framework/include/af.h"
#include "app/framework/util/af-event.h"
#include "aurora-button-joining-hal.h"

static boolean buttonState = FALSE;

void emberAfPluginAuroraButtonJoiningButtonIoInit(void)
{
    emberAfDebugPrintln("Initialise button IO");
    
    //start from a fresh state just in case
    BUTTON_INTCFG = 0;              //disable BUTTON triggering
    INT_CFGCLR = BUTTON_INT_EN_BIT; //clear BUTTON top level int enable
    INT_GPIOFLAG = BUTTON_FLAG_BIT; //clear stale BUTTON interrupt
    INT_MISS = BUTTON_MISS_BIT;     //clear stale missed BUTTON interrupt
    
    //configure BUTTON interrupt
    if (BUTTON_IRQ_SEL != 0)
    {
        GPIO_IRQCSEL = BUTTON_IRQ_SEL;                              //point IRQ at the desired pin
    }
    BUTTON_INTCFG = (GPIOINTMOD_BOTH_EDGES << GPIO_INTMOD_BIT);  
    
    INT_CFGSET = BUTTON_INT_EN_BIT;
    
    buttonConfigureInput();
}

void buttonConfigureInput(void)
{
    // configure BUTTON input
    BUTTON_GPIO_CFG &= ~(BUTTON_CFG_MASK);
    BUTTON_GPIO_CFG |= BUTTON_CFG_INPUT;
    BUTTON_OUT |= BUTTON_PULL_UP;
}

void BUTTON_ISR(void)
{
    // Acknowledge the interrupt
    INT_MISS = BUTTON_MISS_BIT;
    INT_GPIOFLAG = BUTTON_FLAG_BIT;

    buttonState = buttonGetState();
    
    emberAfPluginAuroraButtonJoiningButtonPress(buttonState);
}

boolean buttonGetState(void)
{
    return (BUTTON_IN & BUTTON_PIN) ? BUTTON_RELEASED : BUTTON_PRESSED;
}

void disableButtonInterrpt(void)
{
    BUTTON_INTCFG = 0;              //disable BUTTON triggering
    INT_CFGCLR = BUTTON_INT_EN_BIT; //clear BUTTON top level int enable
    INT_GPIOFLAG = BUTTON_FLAG_BIT; //clear stale BUTTON interrupt
    INT_MISS = BUTTON_MISS_BIT;     //clear stale missed BUTTON interrupt
}

void enableButtonInterrpt(void)
{
    //configure BUTTON interrupt
    BUTTON_INTCFG = (GPIOINTMOD_BOTH_EDGES << GPIO_INTMOD_BIT);  
    INT_CFGSET = BUTTON_INT_EN_BIT; // Set BUTTON top level int enable
}
