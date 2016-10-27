#ifndef _AURORA_COLOUR_CONTROL_H_
#define _AURORA_COLOUR_CONTROL_H_

// This structure holds the RGB result
typedef struct {
    int8u R;
    int8u G;
    int8u B;
    int8u W;
} t_RGB;

EmberAfStatus emberAfPluginAuroraColourControlMoveToColor(int16u colorX, int16u colorY, int16u transitionTime);
EmberAfStatus emberAfPluginAuroraColourControlMoveColor(int16s rateX, int16s rateY);
EmberAfStatus emberAfPluginAuroraColourControlStepColor(int16s stepX, int16s stepY, int16u transitionTime);
EmberAfStatus emberAfPluginAuroraColourControlMoveToColorTemperature(int16u colorTemperature, int16u transitionTime);
void emberAfPluginAuroraColourControlSetOnOff(boolean on);
void emberAfPluginAuroraColourControlSetNewLevel(void);
void emberAfPluginAuroraColourControlSetRgbColorCallback(t_RGB *RGB);

#endif // _AURORA_COLOUR_CONTROL_H_