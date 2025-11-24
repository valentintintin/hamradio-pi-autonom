#include "../include/MessageBus.hpp"

QueueHandle_t MessageBus::createQueue(size_t size) {
    return xQueueCreate(size, sizeof(Message));
}

bool MessageBus::send(const QueueHandle_t queue, const Message &message, TickType_t wait) {
    return xQueueSend(queue, &message, wait) != pdTRUE;
}

bool MessageBus::receive(const QueueHandle_t queue, Message &message, TickType_t wait) {
    return xQueueReceive(queue, &message, wait) == pdTRUE;
}

bool MessageBus::isEmpty(const QueueHandle_t queue) {
    return uxQueueMessagesWaiting(queue) == 0;
}

bool MessageBus::isFull(const QueueHandle_t queue) {
    return uxQueueSpacesAvailable(queue) == 0;
}

UBaseType_t MessageBus::count(const QueueHandle_t queue) {
    return uxQueueMessagesWaiting(queue);
}
