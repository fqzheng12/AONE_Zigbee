#define MAX_BUTTON_PRESSES                             5
#define MAX_BUTTON_SEQUENCES                           4
#define BUTTON_UP_END_OF_SEQUENCE_TIMEOUT              2000
#define BUTTON_DEBOUNCE_TIME                           50
#define BUTTON_PRESS_MINUS_TOLERANCE                   1000
#define BUTTON_PRESS_PLUS_TOLERANCE                    1000
#define PERMIT_JOIN_TIMEOUT 						   60

#define LED_OFF         0
#define LED_ON          1
#define LED_FLASH       2 

#define LED_SWITCH_TIME 500

#define MAX_LEDS                2

// Device state flag definitions
#define DEVICE_STATE_CLEAR                  0x00
#define DEVICE_STATE_JOINING                0x01
#define DEVICE_STATE_JOINED                 0x02
#define DEVICE_STATE_IDENTIFYING            0x04
#define DEVICE_STATE_PERMIT_JOINING         0x08
#define DEVICE_STATE_CLEAR_MASK             0x0F
#define DEVICE_STATE_NETWORK_UNAVAILABLE    0x10

#define DEVICE_STATE_FLAGS_SET              1
#define DEVICE_STATE_FLAGS_CLEAR            0


typedef enum  {
  LED_NWK_UNAVAILABLE,
  LED_JOINING,
  LED_JOINED,
  LED_NOT_JOINED,
  LED_PERMIT_JOINING,
  LED_LEAVING,
  LED_JOINING_FAILED,
  LED_IDENTIFY,  
  LED_LAST,
} t_networkState;

typedef struct {
    HalBoardLed id;
    int8u setting;
} t_ledConfig;

typedef  struct {
    t_networkState state;
    t_ledConfig config[MAX_LEDS];
} t_ledSettings;

#define BUTTON_MAX_PRESSES                5

typedef void (*t_buttonFunc)(void);

typedef struct {
  int8u presses;
  int16u downTime[BUTTON_MAX_PRESSES];
  t_buttonFunc buttonFunc;
} t_buttonSeqence;

typedef enum  {
  BUTTON_JOIN = 0,
  BUTTON_LEAVE,
  BUTTON_PERMIT_JOIN,
} t_sequence;

void buttonDownEventHandler(void);
void buttonUpEventHandler(void);
boolean buttonPress(boolean state);
void handleButtonPress(int32u buttonDownTime);
void checkButtonSequence(void);

void clearJoiningFlag(void);
void joinNetwork(void);
void permitJoiningNetwork(void);
void leaveNetwork(void);

void sendMatchDescriptor(void);
void emberAfPluginAuroraButtonJoiningInitialiseLeds(void);
void emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(int8u newState, boolean set);
void emberAfPluginAuroraButtonJoiningUpdateStatusLed(void);
void emberAfPluginAuroraButtonJoiningClearJoiningFlag(void);
