#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
extern uint32_t SystemCoreClock;

/*-----------------------------------------------------------
 * Application-specific definitions.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                    0
#define configUSE_TICK_HOOK                    1

#define configCPU_CLOCK_HZ                     ( SystemCoreClock )
#define configTICK_RATE_HZ                     ( ( TickType_t ) 1000 )

#define configMAX_PRIORITIES                   5
#define configMINIMAL_STACK_SIZE               128
#define configTOTAL_HEAP_SIZE                 ( ( size_t ) ( 14 * 1024 ) )

#define configMAX_TASK_NAME_LEN                16

#define configUSE_16_BIT_TICKS                 0
#define configIDLE_SHOULD_YIELD               1

#define configUSE_MUTEXES                      1
#define configUSE_RECURSIVE_MUTEXES            0

#define configUSE_COUNTING_SEMAPHORES          1

#define configUSE_QUEUE_SETS                   0
#define configUSE_TIME_SLICING                 1

#define configUSE_EVENT_GROUPS                 1

#define configUSE_TASK_NOTIFICATIONS           1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES  1

#define configCHECK_FOR_STACK_OVERFLOW         2
#define configUSE_MALLOC_FAILED_HOOK           0

#define configSUPPORT_DYNAMIC_ALLOCATION       1
#define configSUPPORT_STATIC_ALLOCATION        0

/* The patched port does not use the CLZ-based task selection. */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Cortex-M3 interrupt configuration */

#define configPRIO_BITS                        4

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY    15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* API functions required by this laboratory */

#define INCLUDE_vTaskDelay                     1
#define INCLUDE_vTaskDelayUntil               1
#define INCLUDE_xTaskGetSchedulerState        1
#define INCLUDE_xTaskGetCurrentTaskHandle     1
#define INCLUDE_uxTaskPriorityGet             1
#define INCLUDE_vTaskPrioritySet              1

/*-----------------------------------------------------------*/

/*
 * The patched port (lib/freertos_port_patch) defines SVC_Handler and
 * SysTick_Handler directly in port.c. No handler name remapping is needed.
 */

#endif /* FREERTOS_CONFIG_H */