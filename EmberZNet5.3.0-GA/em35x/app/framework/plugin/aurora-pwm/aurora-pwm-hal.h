#define PWM_GPIO_RED                        PA6
#define PWM_GPIO_GREEN                      PA7
#define PWM_GPIO_BLUE                       PB6
#define PWM_GPIO_WHITE                      PB7

#define PWM_RG_OUT                          GPIO_PAOUT
#define PWM_RG_GPIO_CFG                     GPIO_PACFGH
            
#define PWM_BW_OUT                          GPIO_PBOUT
#define PWM_BW_GPIO_CFG                     GPIO_PBCFGH

#define PWM_RED_CFG_MASK                    PA6_CFG_MASK
#define PWM_GREEN_CFG_MASK                  PA7_CFG_MASK
#define PWM_BLUE_CFG_MASK                   PB6_CFG_MASK    
#define PWM_WHITE_CFG_MASK                  PB7_CFG_MASK

#define PWM_RED_CFG_OUTPUT                  GPIOCFG_OUT_ALT << PA6_CFG_BIT
#define PWM_GREEN_CFG_OUTPUT                GPIOCFG_OUT_ALT << PA7_CFG_BIT
#define PWM_BLUE_CFG_OUTPUT                 GPIOCFG_OUT_ALT << PB6_CFG_BIT  
#define PWM_WHITE_CFG_OUTPUT                GPIOCFG_OUT_ALT << PB7_CFG_BIT

void emberAfPluginAuroraPwmInitializeTimer(void);
void pwmTimerEnable(void);
void pwmTimerDisable(void);

void pwmSetDutyCycle(int8u output, int8u percentage);