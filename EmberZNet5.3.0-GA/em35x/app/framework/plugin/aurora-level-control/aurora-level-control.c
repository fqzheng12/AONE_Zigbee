// *******************************************************************
// * level-control.c
// *
// *
// * Copyright 2012 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

// this file contains all the common includes for clusters in the util
#include <string.h>
#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

// clusters specific header
#include "aurora-level-control.h"

#ifdef EMBER_AF_PLUGIN_SCENES
  #include "app/framework/plugin/scenes/scenes.h"
#endif //EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_ON_OFF
  #include "app/framework/plugin/on-off/on-off.h"
#endif //EMBER_AF_PLUGIN_ON_OFF

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER

#if (EMBER_AF_PLUGIN_LEVEL_CONTROL_RATE == 0)
  #define FASTEST_TRANSITION_TIME 0
#else
  #define FASTEST_TRANSITION_TIME (MILLISECOND_TICKS_PER_SECOND / EMBER_AF_PLUGIN_LEVEL_CONTROL_RATE)
#endif

#include "aurora-level-control-hal.h"
#include "../aurora-host-protocol/aurora-host-protocol.h"
#include "../aurora-colour-control/aurora-colour-control.h"

extern void calculateEventDuration_and_stepSize(int16u transitionTimeMs, int16u amount, int32u* eventDurationMs, int16u* stepSize);

typedef struct {
  boolean active;
  int8u commandId;
  int8u moveToLevel;
  int8u moveMode;
  int16u stepSize;
  boolean moveToLevelUp; // TRUE for up, FALSE for down
  boolean useOnLevel;
  int8u onLevel;
  int32u eventDuration;
  int32u transitionTime;
  int32u elapsedTime;
} EmberAfLevelControlState;

static int8u minLevel;
static int8u maxLevel;

static EmberAfLevelControlState stateTable[EMBER_AF_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

static EmberAfLevelControlState *emAfGetLevelControlState(int8u endpoint);
static EmberAfStatus emAfLevelControlClusterMoveToLevelHandler(int8u commandId, int8u level, int16u transitionTime);
static EmberAfStatus emAfLevelControlClusterMoveHandler(int8u commandId, int8u moveMode, int8u rate);
static EmberAfStatus emAfLevelControlClusterStepHandler(int8u commandId, int8u stepMode, int8u stepSize, int16u transitionTime);
static EmberAfStatus emAfLevelControlClusterStopHandler(int8u commandId);

static void setOnOffValue(int8u endpoint, boolean onOff);

static void emAfActivateLevelControl(EmberAfLevelControlState *state,
                                     int8u endpoint) 
{
  state->active = TRUE;
  // schedule the first tick
  emberAfScheduleServerTickExtended(endpoint,
                                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                    0,
                                    EMBER_AF_LONG_POLL,
                                    EMBER_AF_STAY_AWAKE);
}

static void emAfDeactivateLevelControl(EmberAfLevelControlState *state) 
{
  state->active = FALSE;
}

static EmberAfLevelControlState *emAfGetLevelControlState(int8u endpoint)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_LEVEL_CONTROL_CLUSTER_ID);
  return (ep == 0xFF ? NULL : &stateTable[ep]);
}

void emberAfLevelControlClusterServerInitCallback(int8u endpoint)
{
  EmberAfLevelControlState *state = emAfGetLevelControlState(endpoint);
  if (state != NULL) 
  {
    emAfDeactivateLevelControl(state);
  }

  // Set the min and max levels
#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  minLevel = EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER_MINIMUM_LEVEL;
  maxLevel = EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER_MAXIMUM_LEVEL;
#else
  minLevel = EMBER_AF_PLUGIN_AURORA_LEVEL_CONTROL_MINIMUM_LEVEL;
  maxLevel = EMBER_AF_PLUGIN_AURORA_LEVEL_CONTROL_MAXIMUM_LEVEL;
#endif
}


void emberAfLevelControlClusterServerTickCallback(int8u endpoint)
{
  EmberAfLevelControlState *state = emAfGetLevelControlState(endpoint);
  EmberAfStatus status;
  int8u currentLevel, setNewLevel;
  int16u newLevel;

  if (state == NULL) 
  {
    return;
  }

  //emberAfDebugPrint("Level control tick\r\n");  
    
  state->elapsedTime += state->eventDuration;

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  if (emberAfPluginZllLevelControlServerIgnoreMoveToLevelMoveStepStop(endpoint,
                                                                      state->commandId)) 
  {
    state->active = FALSE;
  }
#endif

  if (!state->active) 
  {
    return;
  } 
  else 
  {
    // Reschedule the tick
    emberAfScheduleServerTickExtended(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      state->eventDuration,
                                      EMBER_AF_LONG_POLL,
                                      EMBER_AF_STAY_AWAKE);
  }

  // Read the attribute; print error message and return if it can't be read
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) 
  {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return;
  }

  // If something goes wrong, we will set the newLevel to the currentLevel
  newLevel = currentLevel;

  // Handle actions: move-to-level, move, or step
  switch (state->commandId) {

    // Step and move-to-level are treated the same
  case ZCL_MOVE_TO_LEVEL_COMMAND_ID:
  case ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID:
  case ZCL_STEP_COMMAND_ID:
  case ZCL_STEP_WITH_ON_OFF_COMMAND_ID:
    {
      // Are we at the requested level?
      if (currentLevel == state->moveToLevel) 
      {
        // Done; stop moving
        state->elapsedTime = state->transitionTime;
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE
        {
          int16u remainingTime = 0;
          status = emberAfWriteServerAttribute(endpoint,
                                               ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                               ZCL_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                               (int8u *)&remainingTime,
                                               sizeof(remainingTime));
          if (status != EMBER_ZCL_STATUS_SUCCESS) 
          {
            emberAfLevelControlClusterPrintln("ERR: writing remaining time %x", status);
          }
        }
#endif
        emAfDeactivateLevelControl(state);
        if (currentLevel == minLevel) 
        {
          if (state->commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID
              || state->commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) 
          {
            setOnOffValue(endpoint, FALSE);
            if (state->useOnLevel) 
            {
              currentLevel = state->onLevel;
              status = emberAfWriteServerAttribute(endpoint,
                                                   ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                                   ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                                   (int8u *)&currentLevel,
                                                   ZCL_INT8U_ATTRIBUTE_TYPE);
              if (status != EMBER_ZCL_STATUS_SUCCESS) 
              {
                emberAfLevelControlClusterPrintln("ERR: writing current level %x",
                                                  status);
                return;
              }
            }
          }
        }
        return;
      }
      
      // adjust by the proper amount, either up or down
      if (state->moveToLevelUp) 
      {
        // Changing level up... 
        if (state->commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID
            || state->commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) 
        {
          setOnOffValue(endpoint, TRUE);
        }
        if ((state->moveToLevel - currentLevel) < state->stepSize)
        {
            newLevel = state->moveToLevel;  // prevent +ve overflow.
        }
        else
        {
            newLevel = currentLevel + state->stepSize;
        }        
      } 
      else 
      {
        // Changing level down...                
        if ((currentLevel - state->moveToLevel) < state->stepSize)
        {
            newLevel = state->moveToLevel;  // prevent -ve overflow.
        }
        else
        {
            newLevel = currentLevel - state->stepSize;
        }         
        if (newLevel == minLevel) 
        {
          if (state->commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID
              || state->commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) 
          {
            setOnOffValue(endpoint, FALSE);
          }
        }
      }
      
      //emberAfLevelControlClusterPrint("Event: move Level %x->%x ", currentLevel, newLevel);
      //emberAfLevelControlClusterPrintln("(diff %p%x)", state->moveToLevelUp ? "+" : "-",  amountToMove);
      
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE
      {
        int16u remainingTime = ((int16u)(state->transitionTime
                                        - state->elapsedTime)) / 100;
        status = emberAfWriteServerAttribute(endpoint,
                                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                             ZCL_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                             (int8u *)&remainingTime,
                                             sizeof(remainingTime));
        if(status != EMBER_ZCL_STATUS_SUCCESS) 
        {
          emberAfLevelControlClusterPrintln("ERR: writing remaining time %x", status);
        }
      }
#endif
    }
    break;

  case ZCL_MOVE_COMMAND_ID:
  case ZCL_MOVE_WITH_ON_OFF_COMMAND_ID:
    // adjust either up or down
    if (state->moveMode == EMBER_ZCL_MOVE_MODE_UP) 
    {
      newLevel++;
      if (state->commandId == ZCL_MOVE_WITH_ON_OFF_COMMAND_ID) 
      {
        setOnOffValue(endpoint, TRUE);
      }
      // make sure we dont go over the max
      if (newLevel > maxLevel) 
      {
        newLevel = maxLevel;
        emAfDeactivateLevelControl(state);
      }
    }
    else if (state->moveMode == EMBER_ZCL_MOVE_MODE_DOWN) 
    {
      newLevel--;
      // make sure we dont go under 0
      // unsigned, the number wraps which means the high byte gets a value
      if (HIGH_BYTE(newLevel) > 0) 
      {
        newLevel = minLevel;
        emAfDeactivateLevelControl(state);
        if (state->commandId == ZCL_MOVE_WITH_ON_OFF_COMMAND_ID) 
        {
          setOnOffValue(endpoint, FALSE);
        }
      }
    }
    emberAfLevelControlClusterPrintln("Event: move Level %x->%x", currentLevel, newLevel);
    break;
  }

  // newLevel is 2 bytes to detect rollover in either direction
  // need to make it 1 byte to pass to the write attributes call
  setNewLevel = LOW_BYTE(newLevel);
 
  // set the current level, implement a callback if you want
  // hardware action at this point.
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                       ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                       (int8u *)&setNewLevel,
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) 
  {
    emberAfLevelControlClusterPrintln("Err: writing current level %x", status);
  }
  
  emberAfPluginAuroraLevelControlSetNewLevelCallback(newLevel);
  
  // the scene has been changed (the value of level has changed) so
  // the current scene as descibed in the attribute table is invalid,
  // so mark it as invalid (just writes the valid/invalid attribute)
  if (emberAfContainsServer(endpoint, ZCL_SCENES_CLUSTER_ID)) 
  {
    emberAfScenesClusterMakeInvalidCallback(endpoint);
  }
}

static void setOnOffValue(int8u endpoint, boolean onOff)
{   
  if (emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) 
  {
    emberAfLevelControlClusterPrintln("Setting on/off to %p due to level change",
                                      onOff ? "ON" : "OFF");
    emberAfOnOffClusterSetValueCallback(endpoint,
                         (onOff ? ZCL_ON_COMMAND_ID : ZCL_OFF_COMMAND_ID),
                         TRUE); 
  }       
}

boolean emberAfLevelControlClusterMoveToLevelCallback(int8u level, int16u transitionTime)
{
    emberAfLevelControlClusterPrintln("%pMOVE_TO_LEVEL %x %2x", "RX level-control:", level, transitionTime);    
    EmberAfStatus status = emAfLevelControlClusterMoveToLevelHandler(ZCL_MOVE_TO_LEVEL_COMMAND_ID, level, transitionTime);    
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;  
}

boolean emberAfLevelControlClusterMoveToLevelWithOnOffCallback(int8u level, int16u transitionTime)
{
    emberAfLevelControlClusterPrintln("%pMOVE_TO_LEVEL_WITH_ON_OFF %x %2x", "RX level-control:", level, transitionTime);  
    EmberAfStatus status = emAfLevelControlClusterMoveToLevelHandler(ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID, level, transitionTime);    
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;  
}

boolean emberAfLevelControlClusterMoveCallback(int8u moveMode, int8u rate)
{
    emberAfLevelControlClusterPrintln("%pMOVE %x %x", "RX level-control:", moveMode, rate);
    EmberAfStatus status = emAfLevelControlClusterMoveHandler(ZCL_MOVE_COMMAND_ID, moveMode, rate);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;
}

boolean emberAfLevelControlClusterMoveWithOnOffCallback(int8u moveMode, int8u rate)
{
    emberAfLevelControlClusterPrintln("%pMOVE_WITH_ON_OFF %x %x", "RX level-control:", moveMode, rate);
    emAfLevelControlClusterMoveHandler(ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, moveMode, rate);
    EmberAfStatus status = emAfLevelControlClusterMoveHandler(ZCL_MOVE_COMMAND_ID, moveMode, rate);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;
}

boolean emberAfLevelControlClusterStepCallback(int8u stepMode, int8u stepSize, int16u transitionTime)
{
    emberAfLevelControlClusterPrintln("%pSTEP %x %x %2x", "RX level-control:", stepMode, stepSize, transitionTime);  
    EmberAfStatus status = emAfLevelControlClusterStepHandler(ZCL_STEP_COMMAND_ID, stepMode, stepSize, transitionTime);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;  
}

boolean emberAfLevelControlClusterStepWithOnOffCallback(int8u stepMode, int8u stepSize, int16u transitionTime)
{
    emberAfLevelControlClusterPrintln("%pSTEP_WITH_ON_OFF %x %x %2x", "RX level-control:", stepMode, stepSize, transitionTime);
    EmberAfStatus status = emAfLevelControlClusterStepHandler(ZCL_STEP_WITH_ON_OFF_COMMAND_ID, stepMode, stepSize, transitionTime);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;      
}

boolean emberAfLevelControlClusterStopCallback(void)
{
    emberAfLevelControlClusterPrintln("%pSTOP", "RX level-control:");
    EmberAfStatus status = emAfLevelControlClusterStopHandler(ZCL_STOP_COMMAND_ID);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;      
}

boolean emberAfLevelControlClusterStopWithOnOffCallback(void)
{
    emberAfLevelControlClusterPrintln("%pSTOP_WITH_ON_OFF", "RX level-control:");
    EmberAfStatus status = emAfLevelControlClusterStopHandler(ZCL_STOP_WITH_ON_OFF_COMMAND_ID);
    emberAfSendImmediateDefaultResponse(status);  // Send ZCl cmd default response.        
    return TRUE;
}

static EmberAfStatus emAfLevelControlClusterMoveToLevelHandler(int8u commandId, int8u level, int16u transitionTime)
{
    EmberAfStatus status;
    int8u currentLevel;
    int32u transTimeMs;

    EmberAfLevelControlState* state = emAfGetLevelControlState(emberAfCurrentEndpoint());  
    if (state == NULL) 
    {
        return EMBER_ZCL_STATUS_FAILURE;        
    }

    // Cancel any currently active command before fiddling with the state.
    emAfDeactivateLevelControl(state);

    status = emberAfReadServerAttribute(emberAfCurrentEndpoint(),
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
    if (status != EMBER_ZCL_STATUS_SUCCESS) 
    {
        emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
        return status;
    }

    // Don't want to use the on level here
    state->useOnLevel = FALSE;

    // Keep the new level within range.
    if (level >= maxLevel) 
    {
        state->moveToLevel = maxLevel;
    } 
    else if (level <= minLevel) 
    {
        state->moveToLevel = minLevel;
    } 
    else 
    {
        state->moveToLevel = level;
    }

    // Figure out if we're moving up or down.
    if (state->moveToLevel > currentLevel) 
    {
        state->moveToLevelUp = TRUE;
    } 
    else if (state->moveToLevel < currentLevel) 
    {
        state->moveToLevelUp = FALSE;
    } 
    else 
    {
        // No change in level, so just send default response.
        return EMBER_ZCL_STATUS_SUCCESS;        
    }
        
    // If the Transition time field takes the value 0xFFFF, then the time taken to
    // move to the new level shall instead be determined by the On/Off Transition
    // Time attribute.  If On/Off Transition Time, which is an optional attribute,
    // is not present, the device shall move to its new level as fast as it is
    // able.
    if (transitionTime == 0xFFFF) 
    {
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
        status = emberAfReadServerAttribute(emberAfCurrentEndpoint(),
                                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                        ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
                                        (int8u*)&transTimeMs,
                                        sizeof(transTimeMs));
        if (status != EMBER_ZCL_STATUS_SUCCESS) 
        {
            emberAfLevelControlClusterPrintln("ERR: reading on/off transition time %x",
                                                status);
            return status;
        }

        // Transition time comes in (or is stored, in the case of On/Off Transition
        // Time) as tenths of a second, but we work in milliseconds.
        transTimeMs = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10;
#else //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    // If the Transition Time field is 0xFFFF and On/Off Transition Time,
    // which is an optional attribute, is not present, the device shall move to
    // its new level as fast as it is able.
        transTimeMs = FASTEST_TRANSITION_TIME;
#endif //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    } 
    else 
    {
        // Transition time comes in (or is stored, in the case of On/Off Transition
        // Time) as tenths of a second, but we work in milliseconds.
        transTimeMs = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10;
    }
    
    int8u amount;
    if (state->moveToLevel > currentLevel)
    {      
        amount = state->moveToLevel - currentLevel;    
        state->moveToLevelUp = TRUE;
    }
    else
    {      
        amount = currentLevel - state->moveToLevel;    
        state->moveToLevelUp = FALSE;
    }
    state->transitionTime = transTimeMs;
    calculateEventDuration_and_stepSize(state->transitionTime, amount, &state->eventDuration, &state->stepSize);     
    
    // The setup was successful, so mark the new state as active and return.
    emAfActivateLevelControl(state, emberAfCurrentEndpoint());

    state->elapsedTime = 0;
    state->commandId = commandId;

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
    if (commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID) 
    {
        emberAfPluginZllLevelControlServerMoveToLevelWithOnOffZllExtensions(emberAfCurrentCommand());
    }
#endif

    return EMBER_ZCL_STATUS_SUCCESS;    
}

static EmberAfStatus emAfLevelControlClusterMoveHandler(int8u commandId, int8u moveMode, int8u rate)
{
    EmberAfLevelControlState *state = emAfGetLevelControlState(emberAfCurrentEndpoint());
    EmberAfStatus status;
    int8u currentLevel;

    if (state == NULL) 
    {
        return EMBER_ZCL_STATUS_FAILURE;    
    }

    // Cancel any currently active command before fiddling with the state.
    emAfDeactivateLevelControl(state);

    status = emberAfReadServerAttribute(emberAfCurrentEndpoint(),
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
    if (status != EMBER_ZCL_STATUS_SUCCESS) 
    {
        emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
        return status;
    }

    // Don't want to use the on level here
    state->useOnLevel = FALSE;

    // Start the level calculation (initialization)
    state->moveMode = moveMode;
    switch (moveMode) 
    {
        case EMBER_ZCL_MOVE_MODE_UP:
            if (currentLevel == maxLevel) 
            {
                return EMBER_ZCL_STATUS_SUCCESS;      
            }
            state->moveToLevelUp = TRUE;
            break;
    
        case EMBER_ZCL_MOVE_MODE_DOWN:
            if (currentLevel == minLevel) 
            {
                return EMBER_ZCL_STATUS_SUCCESS;              
            }
            state->moveToLevelUp = FALSE;
            break;
            
        default:
            return EMBER_ZCL_STATUS_INVALID_FIELD;
            
    }

    // If the Rate field is 0xFF, the device should move as fast as it is able.
    if (rate == 0xff) 
    {
        state->eventDuration = 0;
    } 
    else 
    {
        state->eventDuration = MILLISECOND_TICKS_PER_SECOND / rate;
    }

    // The setup was successful, so mark the new state as active and return.
    emAfActivateLevelControl(state, emberAfCurrentEndpoint());
    state->commandId = commandId;

    return EMBER_ZCL_STATUS_SUCCESS;  
}

static EmberAfStatus emAfLevelControlClusterStepHandler(int8u commandId, int8u stepMode, int8u stepSize, int16u transitionTime)
{
    EmberAfLevelControlState *state = emAfGetLevelControlState(emberAfCurrentEndpoint());
    EmberAfStatus status;
    int8u currentLevel;
    int32u transTime;
    boolean overUnder = FALSE;

    if (state == NULL) 
    {
        return EMBER_ZCL_STATUS_FAILURE;    
    }

    // Cancel any currently active command before fiddling with the state.
    emAfDeactivateLevelControl(state);

    status = emberAfReadServerAttribute(emberAfCurrentEndpoint(),
                                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                        ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                        (int8u *)&currentLevel,
                                        sizeof(currentLevel));
    if (status != EMBER_ZCL_STATUS_SUCCESS) 
    {
        emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
        return status;
    }

    // Don't want to use the on level here
    state->useOnLevel = FALSE;

    state->moveMode = stepMode;
    switch (stepMode) 
    {
        case EMBER_ZCL_STEP_MODE_UP:
            if (currentLevel == maxLevel) 
            {
                return EMBER_ZCL_STATUS_SUCCESS;              
            } 
            else if (maxLevel - currentLevel > stepSize) 
            {
                state->moveToLevel = currentLevel + stepSize;
            } 
            else 
            {
                // If the new level was pegged at the maximum level, the transition
                // time shall be proportionally reduced.
                overUnder = TRUE;
                transTime = (transitionTime * (maxLevel - currentLevel)) / stepSize;
                state->moveToLevel = maxLevel;
            }
            state->moveToLevelUp = TRUE;
            break;
            
        case EMBER_ZCL_STEP_MODE_DOWN:
            if (currentLevel == minLevel) 
            {
                return EMBER_ZCL_STATUS_SUCCESS;              
            } 
            else if (currentLevel - minLevel > stepSize) 
            {
                state->moveToLevel = currentLevel - stepSize;
            } 
            else 
            {
                // If the new level was pegged at the minimum level, the transition
                // time shall be proportionally reduced.
                overUnder = TRUE;
                transTime = (transitionTime * (currentLevel - minLevel)) / stepSize;
                state->moveToLevel = minLevel;
            }
            state->moveToLevelUp = FALSE;
            break;
            
        default:
            return EMBER_ZCL_STATUS_INVALID_FIELD;            
    }

    if (state->moveToLevel == currentLevel) 
    {
        return EMBER_ZCL_STATUS_SUCCESS;        
    }
    
    // If the Transition Time field is 0xFFFF, the device should move as
    // fast as it is able.
    if (transitionTime == 0xFFFF) 
    {
        transTime = FASTEST_TRANSITION_TIME;
    } 
    else if (!overUnder) 
    {
        // Transition time comes in as tenths of a second, but we work in milliseconds.
        transTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10;
    } 
    else 
    {
        transTime = transTime * MILLISECOND_TICKS_PER_SECOND / 10;
    }
    
    // The duration between events will be the transition time divided by
    // the distance we must move.
    state->eventDuration = transTime / stepSize;    
    state->transitionTime = transTime;
    
    // The setup was successful, so mark the new state as active and return.
    emAfActivateLevelControl(state, emberAfCurrentEndpoint());
    state->commandId = commandId;
    state->elapsedTime = 0;

    return EMBER_ZCL_STATUS_SUCCESS;  
}

static EmberAfStatus  emAfLevelControlClusterStopHandler(int8u commandId)
{
    EmberAfLevelControlState *state = emAfGetLevelControlState(emberAfCurrentEndpoint());
    EmberAfStatus status;

    if (state == NULL) 
    {
        return EMBER_ZCL_STATUS_FAILURE;    
    }

    // Cancel any currently active command.
    emAfDeactivateLevelControl(state);
    state->commandId = commandId;
    return EMBER_ZCL_STATUS_SUCCESS;  
}

void emAfPluginLevelControlClusterOnOffEffectHandler(int8u commandId,
                                                     int8u level,
                                                     boolean onLevel,
                                                     int16u transitionTime)
{
  EmberAfLevelControlState *state = emAfGetLevelControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentLevel;
  int32u transTimeMs;

  if (state == NULL) 
  {
    status = EMBER_ZCL_STATUS_FAILURE;
    return;
  }

  // Cancel any currently active command before fiddling with the state.
  emAfDeactivateLevelControl(state);

  status = emberAfReadServerAttribute(emberAfCurrentEndpoint(),
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return;
  }

  // Is the level the ad hoc on level?
  state->useOnLevel = onLevel;
  state->onLevel = minLevel;

  // Keep the new level within range.
  if (onLevel) 
  {
    state->onLevel = level;
    state->moveToLevel = minLevel;
  } 
  else if (level >= maxLevel) 
  {
    state->moveToLevel = maxLevel;
  } 
  else if (level <= minLevel) 
  {
    state->moveToLevel = minLevel;
  } 
  else 
  {
    state->moveToLevel = level;
  }

  // Figure out if we're moving up or down and by how much.
  if (state->moveToLevel > currentLevel) 
  {
    state->moveToLevelUp = TRUE;
  } 
  else if (state->moveToLevel < currentLevel) 
  {
    state->moveToLevelUp = FALSE;
  } 
  else 
  {
    return;
  }
  
  // If the Transition time field takes the value 0xFFFF, then the time taken to
  // move to the new level shall instead be determined by the On/Off Transition
  // Time attribute.  If On/Off Transition Time, which is an optional attribute,
  // is not present, the device shall move to its new level as fast as it is
  // able.
  if (transitionTime == 0xFFFF) 
  {
    transTimeMs = FASTEST_TRANSITION_TIME;
  } 
  else 
  {
    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    transTimeMs = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10;
  }
  
  int8u amount;
  if (state->moveToLevel > currentLevel)
  {      
      amount = state->moveToLevel - currentLevel;    
      state->moveToLevelUp = TRUE;
  }
  else
  {      
      amount = currentLevel - state->moveToLevel;    
      state->moveToLevelUp = FALSE;
  }            
  state->transitionTime = transTimeMs;
  calculateEventDuration_and_stepSize(state->transitionTime, amount, &state->eventDuration, &state->stepSize);   
    
  state->elapsedTime = 0;
  state->commandId = commandId;

  // The setup was successful, so mark the new state as active and return.
  emAfActivateLevelControl(state, emberAfCurrentEndpoint());
}

EmberAfStatus emAfPluginLevelControlClusterGetCurrentLevel(int8u* currentLevel)
{
    EmberAfStatus status;

    status = emberAfReadServerAttribute(1,
                                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                        ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                        currentLevel,
                                        1);
    
    if (status != EMBER_ZCL_STATUS_SUCCESS) 
    {
        emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);                                        
    } 
    else 
    {
        //GB emberAfLevelControlClusterPrintln("Current level %d", *currentLevel);
    }
    
    return status;
}
