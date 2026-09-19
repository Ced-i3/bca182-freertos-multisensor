/*
 * queue.h — Minimal mock for native PlatformIO unit tests.
 * Only types and macros needed by src/ are provided.
 */
#ifndef QUEUE_H_MOCK
#define QUEUE_H_MOCK

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* xQueueCreate, xQueueSend, xQueuePeek are in FreeRTOS.h mock */

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_MOCK */
