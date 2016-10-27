// *******************************************************************
// * Eurotronic-hal.h
// *
// * 
// *******************************************************************


#ifdef AONE_HW_USE_JOIN_INPUT_ON_PB6

// AONE hw using the alternate joining pin PB6 ...
/**
 * @brief The actual GPIO BUTTON is connected to.  This define should
 * be used whenever referencing BUTTON.
 */
#define BUTTON                  PORTB_PIN(6)
/**
 * @brief The GPIO input register for BUTTON.
 */
#define BUTTON_IN               GPIO_PBIN
/**
 * @brief The GPIO output register for BUTTON.
 */
#define BUTTON_OUT              GPIO_PBOUT
/**
 * @brief Point the proper IRQ at the desired pin for BUTTON.
 * @note IRQB is fixed and as such does not need any selection operation.
 */
#define BUTTON_GPIO_CFG         GPIO_PBCFGH
#define BUTTON_CFG_MASK         PB6_CFG_MASK
#define BUTTON_CFG_INPUT        0x00000800 
#define BUTTON_PULL_UP          0x40
#define BUTTON_IRQ_SEL          0x00        // Uses fixed IntB so no need to select int.     
#define BUTTON_PIN              0x40

/**
 * @brief The interrupt service routine for BUTTON.
 */
#define BUTTON_ISR         halIrqBIsr
/**
 * @brief The interrupt configuration register for BUTTON.
 */
#define BUTTON_INTCFG      GPIO_INTCFGB
/**
 * @brief The interrupt enable bit for BUTTON.
 */
#define BUTTON_INT_EN_BIT  INT_IRQB
/**
 * @brief The interrupt flag bit for BUTTON.
 */
#define BUTTON_FLAG_BIT    INT_IRQBFLAG
/**
 * @brief The missed interrupt bit for BUTTON.
 */
#define BUTTON_MISS_BIT    INT_MISSIRQB
/**
 * @brief CPIO configuration register for BUTTON.
 */
      
#else

// Original Aurora hw, joining input is PC1...
/**
 * @brief The actual GPIO BUTTON is connected to.  This define should
 * be used whenever referencing BUTTON.
 */
#define BUTTON                  PORTC_PIN(1)

/**
 * @brief The GPIO input register for BUTTON.
 */
#define BUTTON_IN               GPIO_PCIN
/**
 * @brief The GPIO output register for BUTTON.
 */
#define BUTTON_OUT              GPIO_PCOUT
/**
 * @brief Point the proper IRQ at the desired pin for BUTTON.
 * @note IRQB is fixed and as such does not need any selection operation.
 */
#define BUTTON_GPIO_CFG         GPIO_PCCFGL
#define BUTTON_CFG_MASK         PC1_CFG_MASK
#define BUTTON_CFG_INPUT        0x00000080 
#define BUTTON_PULL_UP          0x02
#define BUTTON_IRQ_SEL          0x11
#define BUTTON_PIN              0x02

/**
 * @brief The interrupt service routine for BUTTON.
 */
#define BUTTON_ISR         halIrqCIsr
/**
 * @brief The interrupt configuration register for BUTTON.
 */
#define BUTTON_INTCFG      GPIO_INTCFGC
/**
 * @brief The interrupt enable bit for BUTTON.
 */
#define BUTTON_INT_EN_BIT  INT_IRQC
/**
 * @brief The interrupt flag bit for BUTTON.
 */
#define BUTTON_FLAG_BIT    INT_IRQCFLAG
/**
 * @brief The missed interrupt bit for BUTTON.
 */
#define BUTTON_MISS_BIT    INT_MISSIRQC
/**
 * @brief CPIO configuration register for BUTTON.
 */
   
#endif

//------------------------------------------------------------------------------



#define BUTTON_UP                                   FALSE
#define BUTTON_DOWN                                 TRUE

typedef void (*tButtonPulseCompleteCallback) (void);

boolean emberAfPluginAuroraButtonJoiningButtonPress(boolean state);
void emberAfPluginAuroraButtonJoiningButtonIoInit(void);
void buttonConfigureOutput(void);
void buttonConfigureInput(void);
void BUTTON_ISR(void);
boolean buttonGetState(void);
void buttonSetState(boolean state);
void buttonGeneratePulse(int8u pulseWidth, tButtonPulseCompleteCallback callback);
void buttonDetectPulse(int8u pulseWidth);
void disableButtonInterrpt(void);
void enableButtonInterrpt(void);
boolean getButtonInterruptState(void);
void emberAfPluginAuroraButtonJoiningUpdateStatusLed(void);

