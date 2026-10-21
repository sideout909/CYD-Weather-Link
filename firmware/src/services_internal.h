#pragma once
#include "services.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
namespace services::detail {
extern Settings config;
extern State current;
extern SemaphoreHandle_t state_mutex;
extern QueueHandle_t observations;
extern Observation latest;
void startWorker();
void writeState(const State& next);
}
