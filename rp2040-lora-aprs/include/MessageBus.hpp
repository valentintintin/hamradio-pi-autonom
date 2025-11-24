#pragma once
#include <FreeRTOS-Kernel/include/queue.h>
#include "TypeDef.h"

#define MAX_PAYLOAD 256

struct Message {
    enum Type { RELAY } type;
    uint32_t timestamp;
    uint8_t payload[MAX_PAYLOAD];
};

class MessageBus {
public:
    static QueueHandle_t createQueue(size_t size);
    static bool send(QueueHandle_t queue, const Message& message, TickType_t wait);
    static bool receive(QueueHandle_t queue, Message& message, TickType_t wait);
    static bool isEmpty(QueueHandle_t queue);
    static bool isFull(QueueHandle_t queue);
    static UBaseType_t count(QueueHandle_t queue);
};