/*
 * FreeRTOS.h — Minimal mock for native PlatformIO unit tests.
 *
 * This file provides ONLY the type definitions, macros, and stub
 * function signatures needed by src/alarm.cpp, src/system_state.cpp,
 * and src/input.cpp so they can compile and link on the host.
 *
 * This file is NOT part of the firmware and is only used when
 * platformio builds with platform = native.
 */
#ifndef FREERTOS_H_MOCK
#define FREERTOS_H_MOCK

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Base types ────────────────────────────────────────── */

typedef long      BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t  TickType_t;
typedef void *    TaskHandle_t;
typedef void *    QueueHandle_t;
typedef void *    SemaphoreHandle_t;
typedef void (*TaskFunction_t)(void *);

/* ── Constants ─────────────────────────────────────────── */

#define pdTRUE        1
#define pdFALSE       0
#define pdPASS        1
#define pdFAIL        0
#define portMAX_DELAY 0xFFFFFFFFU
#define configTICK_RATE_HZ 1000
#define pdMS_TO_TICKS(x) ((TickType_t)(x))

/* ── Critical section stubs ────────────────────────────── */

#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()

/* ── Task stubs ────────────────────────────────────────── */

static inline BaseType_t xTaskCreate(
    TaskFunction_t,
    const char * const,
    const uint16_t,
    void * const,
    UBaseType_t,
    TaskHandle_t * const)
{ return pdPASS; }

static inline void vTaskDelay(const TickType_t) {}
static inline void vTaskDelayUntil(TickType_t * const, const TickType_t) {}
static inline TickType_t xTaskGetTickCount(void) { return 0; }
static inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) { return 0; }
static inline void xTaskNotifyGive(TaskHandle_t) {}

/* ── Queue stubs ───────────────────────────────────────── */

static inline QueueHandle_t xQueueCreate(UBaseType_t, UBaseType_t) { return NULL; }
static inline BaseType_t xQueueSend(QueueHandle_t, const void * const, TickType_t) { return pdPASS; }
static inline BaseType_t xQueuePeek(QueueHandle_t, void * const, TickType_t) { return pdPASS; }

/* ── Semaphore stubs ───────────────────────────────────── */

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return NULL; }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
static inline void xSemaphoreGive(SemaphoreHandle_t) {}

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_H_MOCK */
