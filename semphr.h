/*
 * semphr.h — Minimal mock for native PlatformIO unit tests.
 * Only types and macros needed by src/ are provided.
 */
#ifndef SEMPHR_H_MOCK
#define SEMPHR_H_MOCK

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* xSemaphoreCreateMutex, xSemaphoreTake, xSemaphoreGive are in FreeRTOS.h mock */

#ifdef __cplusplus
}
#endif

#endif /* SEMPHR_H_MOCK */
