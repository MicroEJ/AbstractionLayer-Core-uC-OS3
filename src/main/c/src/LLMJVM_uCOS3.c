/*
 * C
 *
 * Copyright 2014-2023 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */

/**
 * @file
 * @brief LLMJVM implementation over UC-OS III.
 * @author MicroEJ Developer Team
 * @version 1.0.0
 */


/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */

#include <stdlib.h>
#include <stdbool.h>

// Micrium µCosIII includes
#include "os_cfg_app.h"
#include "os.h"
#include "lib_def.h"

#include "LLMJVM_impl.h"
#include "microej.h"
#include "microej_time.h"
#include "LLMJVM_uCOS3_configuration.h"

#if !defined LLMJVM_UCOS3_CONFIGURATION_VERSION
     #error "Undefined LLMJVM_UCOS3_CONFIGURATION_VERSION, it must be defined in microvg_configuration.h"
#endif

#if defined LLMJVM_UCOS3_CONFIGURATION_VERSION && LLMJVM_UCOS3_CONFIGURATION_VERSION != 1
     #error "Version of the configuration file LLMJVM_UCOS3_CONFIGURATION.h is not compatible with this implementation."
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/*
 *********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 */

/* Defines -------------------------------------------------------------------*/

/* Initial delay of the timer */
#define LLMJVM_UCOS3_WAKEUP_TIMER_DELAY_MS          ((int64_t)100)

/* The 'period' being repeated for the timer. */
#define LLMJVM_UCOS3_WAKEUP_TIMER_PERIOD_MS         ((int64_t)0)

/* Assert macro */
#define LLMJVM_ASSERT(x)                                          \
  if (!(x))                                                       \
  {                                                               \
      LLMJVM_ASSERT_TRACE_OUTPUT("%s, %d\n", __FILE__, __LINE__); \
      while (1){                                                  \
                                                                  \
      }                                                           \
                                                                  \
  }                                                               \
  else                                                            \
  {                                                               \
    ;                                                             \
  }                                                               \

/*
 * Shared variables
 */

/* Set to true when the timer expires, set to true when the timer is started. */
static volatile bool LLMJVM_UCOS3_timer_expired = false;

//Absolute time in ticks at which timer will be launched
// cppcheck-suppress [misra-c2012-8.9] this variable needs to be declared globally.
static int64_t LLMJVM_UCOS3_next_wake_up_time = INT64_MAX;

// Timer for scheduling next alarm
static OS_TMR LLMJVM_UCOS3_wake_up_timer = NULL;

/* Binary semaphore to wakeup MicroJVM */
static OS_SEM LLMJVM_UCOS3_Semaphore = NULL;

/* Private functions ---------------------------------------------------------*/

static void wake_up_timer_callback(void *p_tmr, void *p_arg);

/* Since LLMJVM_schedule() prototype does not match the timer callback prototype,
   we create a wrapper around it and check the ID of the timer. */
// cppcheck-suppress [misra-c2012-2.7, constParameter] Need to comply with OS_TMR_CALLBACK_PTR definition
static void wake_up_timer_callback(void *p_tmr, void *p_arg) {
    if (p_tmr == &LLMJVM_UCOS3_wake_up_timer) {
        LLMJVM_UCOS3_timer_expired = true;
        LLMJVM_schedule();
    }
}

/* Public functions ----------------------------------------------------------*/

/*
 * Implementation of functions from LLMJVM_impl.h
 * and other helping functions.
 */

// Creates the timer used to callback the LLMJVM_schedule() function.
// After its creation, the timer is idle.
int32_t LLMJVM_IMPL_initialize(void)
{  

  int32_t rslt = LLMJVM_OK;
  OS_ERR err;

  if (OSCfg_TmrTaskRate_Hz <= (OS_RATE_HZ) 0) {
      /* MicriumOS timer task disabled or not configured correctly. */
      rslt = LLMJVM_ERROR;
  }

  if (LLMJVM_OK == rslt) {
    /* Create a timer to scheduler alarm for the VM. Delay and period values are dummy initialization values which will never be used. */
    OSTmrCreate(&LLMJVM_UCOS3_wake_up_timer,
                "MicroJVM wake up",
                (OS_TICK)microej_time_time_to_tick(LLMJVM_UCOS3_WAKEUP_TIMER_DELAY_MS),
                (OS_TICK)microej_time_time_to_tick(LLMJVM_UCOS3_WAKEUP_TIMER_PERIOD_MS),
                OS_OPT_TMR_ONE_SHOT,
                wake_up_timer_callback,
                NULL,
                &err);

    if (OS_ERR_NONE != err) {
        rslt = LLMJVM_ERROR;
    }
  }

  if (LLMJVM_OK == rslt) {
    OSSemCreate(&LLMJVM_UCOS3_Semaphore,
                "MicroJVM wake up",
                (CPU_INT32U)0,
                &err);
    
    microej_time_init();
    
    if (OS_ERR_NONE != err) {
        rslt = LLMJVM_ERROR;
    }
  }

  return rslt;

}

// Once the task is started, save a handle to it.
int32_t LLMJVM_IMPL_vmTaskStarted(void)
{
	return LLMJVM_OK;
}

// Schedules requests from the VM
int32_t LLMJVM_IMPL_scheduleRequest(int64_t absoluteTime)
{
        int32_t rslt = LLMJVM_OK;
        OS_ERR err_set = OS_ERR_NONE;
        OS_ERR err_start = OS_ERR_NONE;
        OS_ERR err_stop = OS_ERR_NONE;
	      int64_t relativeTime;
        int64_t relativeTick;
        CPU_BOOLEAN xTimerStartResult;

  int64_t currentTime = LLMJVM_IMPL_getCurrentTime((uint8_t)MICROEJ_TRUE);

  relativeTime = absoluteTime - currentTime;
  /* Determine relative time/tick */
  relativeTick = microej_time_time_to_tick(relativeTime);

  if (relativeTick <= 0) {
    /* 'absoluteTime' has been reached yet */

    /* No pending request anymore */
    LLMJVM_UCOS3_next_wake_up_time = INT64_MAX;

    /* Stop current timer (no delay) */
    if (OS_TMR_STATE_RUNNING == LLMJVM_UCOS3_wake_up_timer.State) {
        OSTmrStop(&LLMJVM_UCOS3_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
        LLMJVM_ASSERT(OS_ERR_NONE == err_stop);
    }

    /* Notify the VM now */
    rslt = LLMJVM_schedule();
  } else if ((true == LLMJVM_UCOS3_timer_expired) ||
        (absoluteTime < LLMJVM_UCOS3_next_wake_up_time) ||
        (LLMJVM_UCOS3_next_wake_up_time <= currentTime)) {
      /* We want to schedule a request in the future but before the existing request
         or the existing request is already done */

      /* Save new alarm absolute time */
      LLMJVM_UCOS3_next_wake_up_time = absoluteTime;

      /* Stop current timer (no delay) */
      if (OS_TMR_STATE_RUNNING == LLMJVM_UCOS3_wake_up_timer.State) {
          OSTmrStop(&LLMJVM_UCOS3_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
          LLMJVM_ASSERT(OS_ERR_NONE == err_stop);
      }
      LLMJVM_UCOS3_timer_expired = false;

      /* Schedule the new alarm */
      OSTmrSet(&LLMJVM_UCOS3_wake_up_timer,
               (OS_TICK)relativeTick,
               (OS_TICK)0,
               wake_up_timer_callback,
               NULL,
               &err_set);
      xTimerStartResult = OSTmrStart(&LLMJVM_UCOS3_wake_up_timer, &err_start);

      if ((DEF_TRUE != xTimerStartResult) ||
          (OS_ERR_NONE != err_set) ||
          (OS_ERR_NONE != err_start)) {
          rslt = LLMJVM_ERROR;
      }
  } else {
    /* else: there is a pending request that will occur before the new one -> do nothing */
  }
  
  return rslt;
}


// Suspends the VM task if the pending flag is not set
int32_t LLMJVM_IMPL_idleVM(void)
{
    OS_ERR err = OS_ERR_NONE;
    OSSemPend(&LLMJVM_UCOS3_Semaphore,
              (OS_TICK)0,
              OS_OPT_PEND_BLOCKING,
              NULL,
              &err);

    return (OS_ERR_NONE == err) ? (int32_t)LLMJVM_OK : (int32_t)LLMJVM_ERROR;

}

// Wakes up the VM task and reset next wake up time
int32_t LLMJVM_IMPL_wakeupVM(void)
{
      OS_ERR err = OS_ERR_NONE;
      OSSemPost(&LLMJVM_UCOS3_Semaphore,
          OS_OPT_POST_ALL,
          &err);

      return (OS_ERR_NONE == err) ? (int32_t)LLMJVM_OK : (int32_t)LLMJVM_ERROR;

}

// Clear the pending wake up flag
int32_t LLMJVM_IMPL_ackWakeup(void)
{
	return LLMJVM_OK;
}

int32_t LLMJVM_IMPL_getCurrentTaskID(void)
{
	return (int32_t)OSTCBCurPtr;
}

// Sets application time
void LLMJVM_IMPL_setApplicationTime(int64_t t)
{
    microej_time_set_application_time(t);
}

// Gets the system or the application time in milliseconds
// cppcheck-suppress [misra-c2012-8.7] this function needs to be accessed by the Core Engine, see LLMJVM_impl.h.
int64_t LLMJVM_IMPL_getCurrentTime(uint8_t sys)
{      
    return microej_time_get_current_time(sys);
}

// Gets the current system time in nanoseconds
int64_t LLMJVM_IMPL_getTimeNanos(void){
    return microej_time_get_time_nanos();
}

int32_t LLMJVM_IMPL_shutdown(void)
{
	// nothing to do
	return LLMJVM_OK;
}

#ifdef __cplusplus
    }
#endif