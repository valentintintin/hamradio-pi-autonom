#pragma once
#include <FreeRTOS-Kernel/include/queue.h>

class BaseController {
public:
    explicit BaseController(QueueHandle_t queue);
    virtual bool begin() = 0;
    virtual bool processCommand() = 0;
protected:
    static void task(void* pv);
    QueueHandle_t queue;
};
