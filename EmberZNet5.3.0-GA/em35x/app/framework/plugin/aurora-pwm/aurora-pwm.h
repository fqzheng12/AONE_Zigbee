#ifndef _AURORA_PWM_H_
#define _AURORA_PWM_H_

#include "../aurora-colour-control/aurora-colour-control.h"

#define PWM_OUTPUT_BLUE             1
#define PWM_OUTPUT_WHITE            2
#define PWM_OUTPUT_GREEN            3
#define PWM_OUTPUT_RED              4


void emberAfPluginAuroraPwmSetDutyCycle(t_RGB *rgb);
void emberAfPluginAuroraPwmEnable(boolean newState);

#endif // _AURORA_PWM_H_