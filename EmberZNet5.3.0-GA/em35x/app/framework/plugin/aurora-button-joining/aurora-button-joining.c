//

// This callback file is created for your convenience. You may add application code
// to this file. If you regenerate this file over a previous version, the previous
// version will be overwritten and any code you have added will be lost.
#include <string.h>

#include "app/framework/include/af.h"
#include "app/framework/include/af-types.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/service-discovery.h"
#include "app/util/common/form-and-join.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include "debug-printing.h"
#include "aurora-button-joining.h"
#include "aurora-button-joining-hal.h"
//#include "app/builder/Aurora_dimmer/Aurora_dimmer_board_1.h

#define REQUEST_ROUTE_TIMER                 8 // 240
#define MAX_ROUTE_REQUESTS                  10

// Custom event stubs. Custom events will be run along with all other events in the
// application framework. They should be managed using the Ember Event API
// documented in stack/include/events.h

// Event control struct declarations
EmberEventControl emberAfPluginAuroraButtonJoiningButtonDownEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningButtonUpEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningButtonDownDebounceEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningButtonUpDebounceEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningButtonEndSequenceEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventControl;
EmberEventControl emberAfPluginAuroraButtonJoiningLedEventEventControl;
EmberEventControl routeRequestTimerEventControl;
EmberEventControl stackStatusEventControl;

// Event function forward declarations
void emberAfPluginAuroraButtonJoiningButtonDownEventHandler(void); 
void emberAfPluginAuroraButtonJoiningButtonUpEventHandler(void); 
void emberAfPluginAuroraButtonJoiningButtonDownDebounceEventHandler(void); 
void emberAfPluginAuroraButtonJoiningButtonUpDebounceEventHandler(void); 
void emberAfPluginAuroraButtonJoiningButtonEndSequenceEventHandler(void); 
void emberAfPluginAuroraButtonJoiningLedEventEventHandler(void); 
void emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventHandler(void);

void emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(int8u newState, boolean set);


static void emberAfPluginAuroraButtonJoiningReset(void);  //MN
//static void emberAfPluginAuroraButtonJoiningJoinNetwork(void);//MN 
static void emberAfPluginAuroraButtonJoiningPermitJoiningNetwork(void);
static void emberAfPluginAuroraButtonJoiningLeaveNetwork(void);
static void emberAfPluginAuroraButtonJoiningCheckButtonSequence(void);
static void emberAfPluginAuroraButtonJoiningHandleButtonPress(int32u buttonDownTime);
static void emberAfPluginAuroraButtonJoiningFlashLed(t_networkState newState);
static t_ledSettings *getLedSettings(t_networkState networkState);

EmberEventData events[] =
 {
   { &emberAfPluginAuroraButtonJoiningLedEventEventControl,   emberAfPluginAuroraButtonJoiningLedEventEventHandler },
   { NULL, NULL }                            // terminator
 };


static boolean ledsCurrentStates[] = {LED_OFF, LED_OFF};
t_networkState currentNetworkState = LED_NWK_UNAVAILABLE;

const t_ledSettings ledSettings[] = {
    {LED_NWK_UNAVAILABLE,   {{LED_RED, LED_FLASH},  {LED_GREEN, LED_OFF}}},
    {LED_NOT_JOINED,        {{LED_RED, LED_ON},     {LED_GREEN, LED_OFF}}},
    {LED_JOINED,            {{LED_RED, LED_OFF},    {LED_GREEN, LED_OFF}}},
    {LED_PERMIT_JOINING,    {{LED_RED, LED_OFF},    {LED_GREEN, LED_OFF}}},
    {LED_JOINING,           {{LED_RED, LED_OFF},    {LED_GREEN, LED_FLASH}}},
    {LED_IDENTIFY,          {{LED_RED, LED_OFF},    {LED_GREEN, LED_FLASH}}}
};

#define MAX_LED_STATES          sizeof(ledSettings) / sizeof(t_ledSettings)

static t_buttonSeqence buttonSequence[] = {
  {1, {3000, 0, 0, 0, 0}, emberAfPluginAuroraButtonJoiningReset},                         // Join MATCH 0 MN CHANGED TO RESET on a 3 second button press
  {3, {100, 100, 100, 0, 0}, emberAfPluginAuroraButtonJoiningReset}//,            // Leave MATCH 1 MN CHANGED to RESET on 3x 100ms button presses
 // {2, {1000, 1000, 0, 0, 0}, emberAfPluginAuroraButtonJoiningPermitJoiningNetwork}              // Identify / Permit Joining MATCH 2
};

static int16u buttonDownTimes[BUTTON_MAX_PRESSES];
static int8u buttonDownCount;
static boolean buttonDownDetectStarted = FALSE;
static boolean buttonUpDetectStarted = FALSE;
static int32u buttonDownTime = 0;
static boolean buttonSequenceStarted = FALSE;
static boolean joiningFLag = FALSE;
static int8u deviceStateFlags = DEVICE_STATE_CLEAR | DEVICE_STATE_NETWORK_UNAVAILABLE;  
static boolean checkedIfPaired = FALSE;
//static int32u lastCheckNwkTime =0;  //MN new static global

char deviceStateText[][20] = {
     "Joining",
     "Joined",
     "Identifying",
     "Permit Joining",
     "Network Unavailable"
     };

boolean nwkNewJoinPending = FALSE; //= FALSE; //MN declare global variable

void emRadioSetEdCcaThreshold(int8s threshold);     // ember provided



void setRadioPowerAndMode(void);


/** @brief Button pressed event
 *  
 *  This event handler is called when the button is pressed. This starts the
 *  debounce timer.
 * 
 * @param none
 */
void emberAfPluginAuroraButtonJoiningButtonDownEventHandler(void)
{
  // Deactivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonDownEventControl);

  // See if we have already started button detect
  if (!buttonDownDetectStarted) 
  {
    // Set event for the debounce time
    emberEventControlSetDelayMS(emberAfPluginAuroraButtonJoiningButtonDownDebounceEventControl, BUTTON_DEBOUNCE_TIME);
    buttonDownDetectStarted = TRUE;

    if (!buttonSequenceStarted) 
    {
      buttonSequenceStarted = TRUE;
    }
  } 
}

/** @brief Debounce button press
 *  
 *  This event handler is called when the button is pressed. This debounces 
 *  the button press and starts a timer to measure the length of time the 
 *  button is pressed for.
 * 
 * @param none
 */
void emberAfPluginAuroraButtonJoiningButtonDownDebounceEventHandler(void)
{
  boolean buttonStateNow;

  // Deavtivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonDownDebounceEventControl);

  // Cancel the end of sequence timeout since we have a new button press
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonEndSequenceEventControl);

  // Get the current state of the input and check to confirm button press
  buttonStateNow = buttonGetState();
  if (buttonStateNow == BUTTON_PRESSED) 
  {
    buttonDownTime = halCommonGetInt32uMillisecondTick();
  }

  // Indicate we are not starting detection
  buttonDownDetectStarted = FALSE;
}

/** @brief Button released event
 *  
 *  This event handler is called when the button is released. This starts the
 *  debounce timer.
 * 
 * @param none
 */
void emberAfPluginAuroraButtonJoiningButtonUpEventHandler(void)
{
  // Deactivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonUpEventControl);

  // If we are not already detecting button release set debounce delayed event
  if (!buttonUpDetectStarted) 
  {
    emberEventControlSetDelayMS(emberAfPluginAuroraButtonJoiningButtonUpDebounceEventControl, BUTTON_DEBOUNCE_TIME);
    buttonUpDetectStarted = TRUE;
  }
}

/** @brief Debounce button release
 *  
 *  This debounces the button release and works out the time the button was pressed.
 *  It calls emberAfPluginAuroraButtonJoiningHandleButtonPress() to record the button press.
 * 
 * @param none
 */
void emberAfPluginAuroraButtonJoiningButtonUpDebounceEventHandler(void)
{
  boolean buttonStateNow;

  // Deactivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonUpDebounceEventControl);

  // Get the current state of the input and check to confirm button release
  buttonStateNow = buttonGetState();
  if (buttonStateNow == BUTTON_RELEASED) 
  {
    // Work out the duration of the button press
    buttonDownTime = halCommonGetInt32uMillisecondTick() - buttonDownTime;
    
    emberAfDebugPrint("Button release (%d)\r\n", buttonDownTime);
   
    // Record the button press
    emberAfPluginAuroraButtonJoiningHandleButtonPress(buttonDownTime);    
  }

  buttonUpDetectStarted = FALSE;
}


/** @brief Records the button pressed time
 *  
 *  The time is recorded in an array and a timer to detect 
 *  the end of the button sequence is started.
 *
 * @param buttonDownTime - the time the button was pressed
 */
static void emberAfPluginAuroraButtonJoiningHandleButtonPress(int32u buttonDownTime)
{
  // Make sure we don't overrun the button presses array
  if (buttonDownCount < MAX_BUTTON_PRESSES) 
  {

    // Record the button press duration and count it
    buttonDownTimes[buttonDownCount] = buttonDownTime;
    buttonDownCount++;
  }
  
  // Set delayed event for the end of sequence
  emberEventControlSetDelayMS(emberAfPluginAuroraButtonJoiningButtonEndSequenceEventControl, BUTTON_UP_END_OF_SEQUENCE_TIMEOUT);
}

/** @brief End of button press sequence
 *  
 *  This is called when the end of sequence timer expires. It calls the 
 *  emberAfPluginAuroraButtonJoiningCheckButtonSequence()function to see if is a valid sequence and
 *  then resets the sequence recording data.
 *
 * @param none
 */
void emberAfPluginAuroraButtonJoiningButtonEndSequenceEventHandler(void)
{
  //emberAfDebugPrint("End sequence\r\n");
  
  // Deactivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningButtonEndSequenceEventControl);

  // Check the sequence to see if it matches any pre defined ones
  emberAfPluginAuroraButtonJoiningCheckButtonSequence();
  
  // Reset the keypress detector for the next sequence
  memset(buttonDownTimes, 0, sizeof(buttonDownTimes));
  buttonDownTime = 0;
  buttonDownCount = 0;
}

/** @brief Check for valid button sequence
 *  
 *  The set of valid button sequences is stored in buttonSequence array.
 *  The buttonDownTimes array is compared with the buttonSequence array. Time tolerances are 
 *  taken into account. If there is a match, the appropriate function is called. 
 *  If not the sequence is ignored.
 *
 * @param none
 */
static void emberAfPluginAuroraButtonJoiningCheckButtonSequence(void) 
{
  int8u sequenceIndex;
  int8u index = 0;
  boolean sequenceMatch;
  
  // Iterate over the defined sequences
  for (sequenceIndex = 0; sequenceIndex < MAX_BUTTON_SEQUENCES; sequenceIndex++) 
  {
    sequenceMatch = TRUE;   
        
    if (sequenceIndex != 1) //MN check all sequences except the special power cycling sequence against normal tolerances
    {
      // Iterate over the button press times
      for (index = 0; index < buttonSequence[sequenceIndex].presses; index++) 
      { 
        // If the time is outside the defined time go to the next sequence
        if ((buttonDownTimes[index] == 0) || 
            (!((buttonDownTimes[index] >= buttonSequence[sequenceIndex].downTime[index] - BUTTON_PRESS_MINUS_TOLERANCE) && 
            (buttonDownTimes[index] < buttonSequence[sequenceIndex].downTime[index] + BUTTON_PRESS_PLUS_TOLERANCE)))) 
        { 
          sequenceMatch = FALSE;
          break;
        } 
      }

      // If we have found a match exit the loop
      if (sequenceMatch) 
      {
        break;
      }
    }  
    
    
    if (sequenceIndex == 1) //MN if this is the special power cyling sequence, use super tight tolerances
    {
      // Iterate over the button press times
      for (index = 0; index < buttonSequence[sequenceIndex].presses; index++) 
      { 
        // If the time is outside the defined time go to the next sequence
        if ((buttonDownTimes[index] == 0) || 
            (!((buttonDownTimes[index] >= buttonSequence[sequenceIndex].downTime[index] - BUTTON_PRESS_MINUS_TOLERANCE_TIGHT) && 
            (buttonDownTimes[index] < buttonSequence[sequenceIndex].downTime[index] + BUTTON_PRESS_PLUS_TOLERANCE_TIGHT)))) 
        { 
          sequenceMatch = FALSE;
          break;
        } 
      }

      // If we have found a match exit the loop
      if (sequenceMatch) 
      {
        break;
      }
    }
    
    
  }

  // If we have a match, call the appropriate function
  if (sequenceMatch) 
  {
    emberAfDebugPrint("Match: %d\r\n", sequenceIndex);
    buttonSequence[sequenceIndex].buttonFunc();
  } 
  else 
  {
    emberAfDebugPrint("No match\r\n");
  }
}

    //if the device is not paired to a network, it will enter pairing mode

void checkIfPaired(void) //MN
{
  if (!checkedIfPaired)
    {
    emberAfDebugPrint("Reached checkIfPaired\n"); //MN
    emberAfDebugPrint("@line 316, device state is: %x\n", deviceStateFlags); //MN
    
    if (emberNetworkState() == EMBER_NO_NETWORK)
    {
       emberAfPluginAuroraButtonJoiningReset(); //MN changed from Join to Reset
    }
    
    checkedIfPaired=TRUE;

  }
    
}  

//Reset function to leave and then join a network
static void emberAfPluginAuroraButtonJoiningReset(void)
{

    emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningReset line 338 aurora-button-joining\n"); //MN
    
    //emberAfPluginAuroraButtonJoiningLeaveNetwork();
    
/*    emberAfPluginAuroraButtonJoiningJoinNetwork();

        if (emberNetworkState() == EMBER_NETWORK_DOWN) //try again
        {
              emberAfDebugPrint("trying to join again\n"); //MN
                emberAfPluginAuroraButtonJoiningJoinNetwork();
        }
    
   */
    
    //halReboot();
    
    //MN sample code from Gary
    EmberNetworkStatus nwkState = emberAfNetworkState();  // Get present network state.    
    switch (nwkState) 
    {
        case EMBER_NO_NETWORK:
        {
            emberAfDebugPrintln("Find joinable networks");
                        
            emberAfStartSearchForJoinableNetwork();  // Start Joining scan.
            
            break; 
        }            
        case EMBER_JOINED_NETWORK_NO_PARENT:
        {
            // Join request behaviour (if Network:Lost) - do a leave nwk then then do a new Join.
            if (!nwkNewJoinPending)
            {   
                  emberLeaveNetwork(); //emberAfPluginAuroraButtonJoiningLeaveNetwork();
              
              nwkNewJoinPending = TRUE;  // SET GLOBAL FLAG, a new Join will be triggered when device has LEFT nwk.                				
            }
            break;
        }
        case EMBER_JOINED_NETWORK:
        {          
              // Join request behaviour (if Network:Joined) - do a leave nwk then then do a new Join.
              if (!nwkNewJoinPending)
              {
                      emberLeaveNetwork();//emberAfPluginAuroraButtonJoiningLeaveNetwork(); // Leave network straight away.                
            emberAfDebugPrintln("REBOOT NOW EMBER_JOINED_NETWORK");
                      nwkNewJoinPending = TRUE;  // Set global flag, a new Join will be triggered when device has LEFT nwk.                				
              }				
            break;
        }
        case EMBER_JOINING_NETWORK:
        {
            emberAfDebugPrintln("Joining network");
            
            break;
        }
        case EMBER_LEAVING_NETWORK:
        {
            emberAfDebugPrintln("Leaving network");
            
            break;
        }
        default :
        {
            emberAfDebugPrintln("Join request, nwk state=%x", nwkState);
            break; 
        }    
    }
    
    
    

}




/** @brief Join a network
 *  
 *  Start the joining process. Flashes the led to indicate this and 
 *  sets the joining flag to prevent interruption of the joining process
 *  by button presses
 *
 * @param none
 */

static void emberAfPluginAuroraButtonJoiningJoinNetwork(void)
{
   
        //MN 
    emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningJoinNetwork line 319 aurora-button-joining\n");
  
  emberAfDebugPrint("Find joinable networks (%x)\r\n", emberNetworkState());

    emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_JOINING, DEVICE_STATE_FLAGS_SET);

    emberAfDebugPrint("@line 333, device state is: %x\n", deviceStateFlags);

   
        //MN  
    emberAfDebugPrint("Reached line 327 aurora-button-joining\n");
  
    // Searches for and joins network
    emberAfStartSearchForJoinableNetwork();
      
    emberAfCorePrintln("%p: join", "BUTTON\r\n");
    
}



/** @brief Clear the joining flag
 *  
 *  Called from emberAfPluginNetworkFindFinishedCallback(). Indicates that 
 *  the joining process is finished.
 *
 * @param none
 */
void emberAfPluginAuroraButtonJoiningClearJoiningFlag(void)
{
  joiningFLag = FALSE;
}

/** @brief Permit joining
 *  
 *  Broadcasts a permit join request. Flashes the led to indicate this.
 *  Permit join is enabled for the PERMIT_JOIN_TIMEOUT period. A timer is 
 *  started to reset the led to the LED_JOINED state when the permit join 
 *  timeout expires.
 *  
 * @param none
 */
static void emberAfPluginAuroraButtonJoiningPermitJoiningNetwork(void)
{
  
      emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningPermitJoiningNetwork line 374 aurora-button-joining\n");//MN

  int8u permitJoinDuration;
  
  emberAfDebugPrint("Setting permit join flag\r\n");

  if (emberNetworkState() == EMBER_JOINED_NETWORK) 
  {
    emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_PERMIT_JOINING | DEVICE_STATE_IDENTIFYING, DEVICE_STATE_FLAGS_SET);
    
          emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningPermitJoiningNetwork line 385 aurora-button-joining\n"); //MN

    
    permitJoinDuration = EMBER_AF_PLUGIN_EZMODE_COMMISSIONING_IDENTIFY_TIMEOUT;
    emAfPermitJoin(permitJoinDuration, TRUE);  // broadcast permit join

    emberEventControlSetDelayQS(emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventControl, permitJoinDuration * 4);
  }
}

/** @brief Permit joining
 *  
 *  Broadcasts a permit join request. Flashes the led to indicate this.
 *  Permit join is enabled for the PERMIT_JOIN_TIMEOUT period. A timer is 
 *  started to reset the led to the LED_JOINED state when the permit join 
 *  timeout expires.
 *  
 * @param none
 */
void emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventHandler(void)
{
  // Deavtivate the event
  emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventControl);

  emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_PERMIT_JOINING | DEVICE_STATE_IDENTIFYING, DEVICE_STATE_FLAGS_CLEAR);
  
        emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningPermitJoiningExpiryEventHandler line 409 aurora-button-joining\n"); //MN

}



/** @brief Leave network
 *  
 *  Sends a leave network request and flashes the led with 
 *  the LED_NOT_JOINED.
 *
 * @param none
 */

static void emberAfPluginAuroraButtonJoiningLeaveNetwork(void)
{
  
          emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningLeaveNetwork line 424 aurora-button-joining\n"); //MN

  
  int8u status;

  emberAfDebugPrint("Leaving network\r\n");

    emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_JOINED, DEVICE_STATE_FLAGS_CLEAR);

              emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningLeaveNetwork line 438 aurora-button-joining\n"); //MN

    
    status = emberLeaveNetwork();
    
    switch (status)
    {
      case EMBER_INVALID_CALL :       // in case we are not part of a pan
        emberAfDebugPrint("Error leaving network (%02X)\r\n", status);
        break;

      case EMBER_SUCCESS:             // if everything is OK
        emberAfDebugPrint("Success\r\n");
          
        break;

      default:                        // in case of a misc error
        emberAfDebugPrint("Error (%02X)\r\n", status);
        break;
    }
}


// ISR context functions!!!

// WARNING: these functions are in ISR so we must do minimal
// processing and not make any blocking calls (like printf)
// or calls that take a long time.

boolean emberAfPluginAuroraButtonJoiningButtonPress(boolean state)
{
    // ISR CONTEXT!!!
    
    if (state == BUTTON_DOWN) 
    {
        //emberAfDebugPrint("Button Down(%d)\r\n", state);
        emberEventControlSetActive(emberAfPluginAuroraButtonJoiningButtonDownEventControl);
    } 
    else 
    {
        //emberAfDebugPrint("Button Up(%d)\r\n", state); 
        emberEventControlSetActive(emberAfPluginAuroraButtonJoiningButtonUpEventControl);
    }

    return TRUE;
}

/** @brief Flash the LED
 *  
 * Sets the led state and calls the emberAfPluginAuroraButtonJoiningLedEventEventHandler().
 *  
 * @param cadence - the new led flashing cadence
 */
void emberAfPluginAuroraButtonJoiningInitialiseLeds(void)
{
  emberAfDebugPrintln("Initialise Leds");
  
  currentNetworkState = LED_NOT_JOINED;
  
  emberAfPluginAuroraButtonJoiningLedEventEventHandler();
}

/** @brief Flash the LED
 *  
 * Sets the led state and calls the emberAfPluginAuroraButtonJoiningLedEventEventHandler().
 *  
 * @param cadence - the new led flashing cadence
 */
static void emberAfPluginAuroraButtonJoiningFlashLed(t_networkState newState)
{
  
      emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningFlashLed line 506 aurora-button-joining\n"); //MN
      emberAfDebugPrint("currentNetworkState set as: %d\n", newState); //MN

  currentNetworkState = newState;
  
  emberAfPluginAuroraButtonJoiningLedEventEventHandler();
}

/** @brief Handle led timer event
 *  
 *  Flashes the led. Uses the ledCadence array where the on and off 
 *  times are defined for each cadence.
 *  
 * @param none
 */
void emberAfPluginAuroraButtonJoiningLedEventEventHandler(void)
{
    t_ledSettings *settings;
    int8u index;
    
    emberEventControlSetInactive(emberAfPluginAuroraButtonJoiningLedEventEventControl);
    
    settings = (t_ledSettings *)getLedSettings(currentNetworkState);
    
    if (settings != (t_ledSettings *)NULL) 
    {
        for (index = 0; index < MAX_LEDS; index++) 
        {
            if (settings->config[index].setting == LED_FLASH) 
            {
                if (ledsCurrentStates[index] == LED_OFF) 
                {
                    halSetLed(settings->config[index].id);
                    ledsCurrentStates[index] = LED_ON;
                } 
                else 
                {
                    halClearLed(settings->config[index].id);
                    ledsCurrentStates[index] = LED_OFF;
                }    
                emberEventControlSetDelayMS(emberAfPluginAuroraButtonJoiningLedEventEventControl, LED_SWITCH_TIME);
            } 
            else if (settings->config[index].setting == LED_ON) 
            {
                halClearLed(settings->config[index].id);
                ledsCurrentStates[index] = LED_ON;
            } 
            else 
            {
                halSetLed(settings->config[index].id);
                ledsCurrentStates[index] = LED_OFF;
            }
        }
    }
}

static t_ledSettings *getLedSettings(t_networkState networkState)
{
    int8u index;
    t_ledSettings *settings = (t_ledSettings *)NULL;
    
    for (index = 0; index < MAX_LED_STATES; index++) 
    {
        if (ledSettings[index].state == networkState) 
        {
            //emberAfDebugPrint("Index: (%d) Setting : (%d) Network State: (%d)\r\n", index, settings->config[index].setting, currentNetworkState);//MN
            //emberAfDebugPrint("State: (%d) Network State: (%d) Index: (%d)\r\n", ledSettings[index].state, currentNetworkState, index);//MN
            settings = (t_ledSettings *)&ledSettings[index];
            break;
        }
    } 
    
    return settings;
}

/** @brief Cli join command
 *  
 *  Calls emberAfPluginAuroraButtonJoiningJoinNetwork()
 *  
 * @param none
 */
void cliJoin(void)
{
  emberAfPluginAuroraButtonJoiningReset();
}

void stackStatusEventHandler(void)
{
  
  emberAfDebugPrint("Reached stackStatusEventHandler line 591 aurora-button-joining\n"); //MN

  
  emberEventControlSetInactive(stackStatusEventControl);
  
  // Send permit join, if necessary
  if (joiningFLag) 
  {
    emAfPermitJoin(EMBER_AF_PLUGIN_EZMODE_COMMISSIONING_IDENTIFY_TIMEOUT, TRUE);  // broadcast permit join
    emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(DEVICE_STATE_PERMIT_JOINING, DEVICE_STATE_FLAGS_SET);
    
      emberAfDebugPrint("Reached stackStatusEventHandler line 602 aurora-button-joining\n"); //MN

    
  }
  
}

/** @brief Set the radio power and power mode
 *
 *
 */
void setRadioPowerAndMode(void)
{
    int8u board[17] = {0};
    int8s txPower = 8;
    int16u txPowerMode = EMBER_TX_POWER_MODE_DEFAULT;
    int16u temp16u;

    halCommonGetToken(board,TOKEN_MFG_BOARD_NAME);
  
    if ((MEMCOMPARE(&board[8],"LRS", 3) == 0) || ((MEMCOMPARE(&board[10],"LRS", 3) == 0))) 
    {
        GPIO_PBCFGL &= 0xFFF0;
        GPIO_PBCFGL |= 0x0001;
        GPIO_PBSET = 0x00000001;
        GPIO_PCCFGH &=  0xFF0F;
        GPIO_PCCFGH |=  0x0090;
        txPower = -7;                   
        emRadioSetEdCcaThreshold(-65);  // M&M - this increses the RSSI threshold seen by the CSMA for the modules with LNA (by 10dBm)
    } 
    /* GB ERS Not supported
    else if ((MEMCOMPARE(&board[8],"ERS",3) == 0) || ((MEMCOMPARE(&board[10],"ERS",3) == 0))) 
    {
        GPIO_PBCFGL &= 0xFFF0;
        GPIO_PBCFGL |= 0x0001;
        GPIO_PBCFGH &= 0xFF0F;
        GPIO_PBCFGL |= 0x0010;
        GPIO_PBSET = 0x00000021;
        GPIO_PCCFGH &=  0xF00F;
        GPIO_PCCFGH |=  0x0990;
        txPower = -6;
        emRadioSetEdCcaThreshold(-65); // M&M - this increses the RSSI threshold seen by the CSMA for the modules with LNA (by 10dBm)
    }
    */ 
    else if ((MEMCOMPARE(&board[8],"LR", 2) == 0) || ((MEMCOMPARE(&board[10],"LR", 2) == 0))) 
    {
        GPIO_PCCFGH &=  0xFF0F;
        GPIO_PCCFGH |=  0x0090;
        emRadioSetEdCcaThreshold(-65); // M&M - this increses the RSSI threshold seen by the CSMA for the modules with LNA (by 10dBm)
    }

    halCommonGetToken(&temp16u, TOKEN_MFG_PHY_CONFIG);
    if (!(temp16u & 0x0002)) 
    {
        txPowerMode |= EMBER_TX_POWER_MODE_ALTERNATE;
    }

    if (txPower > 3)
    {
        txPowerMode |= EMBER_TX_POWER_MODE_BOOST;
    }

    emberAfDebugPrint("setRadioPowerAndMode txPower %d\r\n", txPower);
    emberAfDebugPrint("setRadioPowerAndMode txPowerMode %d\r\n", txPowerMode);
  
    emberSetRadioPower(txPower);
    emberSetTxPowerMode(txPowerMode);
  
    emberAfDebugPrint("setRadioPowerAndMode read back power %d\r\n", emberGetRadioPower());
    emberAfDebugPrint("setRadioPowerAndMode read back power mode %d\r\n", emberGetTxPowerMode());
}

void emberAfPluginAuroraButtonJoiningUpdateStatusLed(void)
{
  
    //emberAfDebugPrint("Reached emberAfPluginAuroraButtonJoiningUpdateStatusLed line 677 aurora-button-joining\n"); //MN

    static int8u lastDeviceStateFlags = DEVICE_STATE_CLEAR;
  
  //if the DEVICE_STATE_NETWORK_UNAVAILABLE, don't say that the lastDeviceStateFlags is clear MN
  

//  if (deviceStateFlags == DEVICE_STATE_NETWORK_UNAVAILABLE)
  //{
    //lastDeviceStateFlags = deviceStateFlags;
   // emberAfDebugPrintln("keep DEVICE_STATE_NETWORK_UNAVAILABLE (%x)\r\n", deviceStateFlags); MN

  //}
      //  emberAfDebugPrintln("deviceStateFlags b4 if update status led: (%x)\r\n", deviceStateFlags);
        //emberAfDebugPrintln("lastDeviceStateFlags: (%x)\r\n", lastDeviceStateFlags);

  
  if (deviceStateFlags != lastDeviceStateFlags) //if the device state flags have changed... MN
  {
    emberAfDebugPrintln("update status led: (%x)\r\n", deviceStateFlags);
    
    switch(deviceStateFlags) 
    {
      case DEVICE_STATE_NETWORK_UNAVAILABLE:
            emberAfDebugPrintln("Device State is DEVICE_STATE_NETWORK_UNAVAILABLE\n"); //MN

        emberAfPluginAuroraButtonJoiningFlashLed(LED_NWK_UNAVAILABLE);
        
   //  emberAfDebugPrintln("lamps flash twice to show entering pairing mode"); //MN

        
//emberAfPluginAuroraButtonJoiningJoinNetwork(); //this will put AONE into pairing mode if it is not already joined at power on MN
        

        break;      
      
      case DEVICE_STATE_NETWORK_UNAVAILABLE | DEVICE_STATE_JOINING:
      case DEVICE_STATE_JOINING:
        emberAfDebugPrintln("Device State is DEVICE_STATE_JOINING\n"); //MN
        emberAfPluginAuroraButtonJoiningFlashLed(LED_JOINING);
        break;
  
      case DEVICE_STATE_CLEAR:
                emberAfDebugPrintln("Device State is DEVICE_STATE_CLEAR\n"); //MN

        emberAfPluginAuroraButtonJoiningFlashLed(LED_NOT_JOINED);
        break;
        
      case DEVICE_STATE_JOINED:
                        emberAfDebugPrintln("Device State is DEVICE_STATE_JOINED\n"); //MN


        emberAfPluginAuroraButtonJoiningFlashLed(LED_JOINED);
        break;
      
      case DEVICE_STATE_JOINED | DEVICE_STATE_IDENTIFYING:
      case DEVICE_STATE_JOINED | DEVICE_STATE_IDENTIFYING | DEVICE_STATE_PERMIT_JOINING:
                                emberAfDebugPrintln("Device State is       case DEVICE_STATE_JOINED | DEVICE_STATE_IDENTIFYING | DEVICE_STATE_PERMIT_JOINING:\n"); //MN

        emberAfPluginAuroraButtonJoiningFlashLed(LED_IDENTIFY);
        break;

      case DEVICE_STATE_JOINED | DEVICE_STATE_PERMIT_JOINING:
                                        emberAfDebugPrintln("Device State is        case DEVICE_STATE_JOINED | DEVICE_STATE_PERMIT_JOINING:\n"); //MN

        emberAfPluginAuroraButtonJoiningFlashLed(LED_PERMIT_JOINING);
        break;

      default:
        emberAfDebugPrintln("update status led (default): (%x)\r\n", deviceStateFlags);
        break;
    }

    lastDeviceStateFlags = deviceStateFlags;
  }
  
}

void emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags(int8u newState, boolean set)
{
  
    emberAfDebugPrintln("Reached emberAfPluginAuroraButtonJoiningUpdateDeviceStateFlags line 756 aurora-button-joining\n"); //MN

    emberAfDebugPrintln("@line 769 deviceStateFlags are: (%x)\r\n", deviceStateFlags); //MN
    emberAfDebugPrintln("new state: (%x) set is: (%x)\r\n", newState, set); //MN

    
  if (set) 
  {
    deviceStateFlags |= newState;
    
        emberAfDebugPrintln("in deviceStateFlags |= newState;"); //MN

    
  } 
  else 
  {
    deviceStateFlags &= ~(newState);
    
            emberAfDebugPrintln("in deviceStateFlags &= ~(newState);"); //MN

    
  }
  
  emberAfDebugPrintln("@line 779 deviceStateFlags are: (%x)\r\n", deviceStateFlags); //MN

}

void sendMatchDescriptor(void)
{
  EmberAfStatus status;
  
  status = emAfSendMatchDescriptor(0x0000, 0x0104, 0x000, TRUE);
}

/** @brief Poll Completed
 *
 * This function is called by the application framework after a poll is
 * completed.
 *
 * @param status Return status of a completed poll operation  Ver.: always
 */
void emberAfPluginEndDeviceSupportPollCompletedCallback(EmberStatus status)
{
  
}
