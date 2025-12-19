#include "controllers/BaseController.hpp"

BaseController::BaseController(QueueHandle_t *queue) : queue(queue) {
}