#pragma once
#include <FreeRTOS.h>
#include <queue.h>

class BaseController {
public:
    explicit BaseController(QueueHandle_t *queue);
    virtual bool begin() = 0;
protected:
    QueueHandle_t *queue;
};
