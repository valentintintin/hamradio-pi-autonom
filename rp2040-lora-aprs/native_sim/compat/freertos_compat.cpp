#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
// vTaskDelete(nullptr) = auto-suppression uniquement (jamais le handle d'une
// autre tâche) : simulée en déroulant la pile via cette exception, attrapée
// dans le wrapper de xTaskCreate, pour sortir sans tuer le process.
struct TaskSelfDelete {};

struct NativeTask {
  std::mutex m;
  std::condition_variable cv;
  uint32_t notify_count = 0;
};

thread_local NativeTask* tls_current_task = nullptr;
}  // namespace

BaseType_t xTaskCreate(TaskFunction_t fn, const char* /*name*/, uint32_t /*stackWords*/,
                       void* params, uint32_t /*priority*/, TaskHandle_t* handle) {
  auto* task = new NativeTask();
  std::thread t([fn, params, task]() {
    tls_current_task = task;
    try {
      fn(params);
    } catch (const TaskSelfDelete&) {
    }
  });
  if (handle) {
    *handle = task;
  }
  t.detach();
  return pdTRUE;
}

void vTaskDelay(TickType_t ticks) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ticks));
}

void vTaskDelete(TaskHandle_t handle) {
  if (handle == nullptr) {
    throw TaskSelfDelete{};
  }
}

TaskHandle_t xTaskGetCurrentTaskHandle() {
  return tls_current_task;
}

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
  auto* task = tls_current_task;
  if (!task) {
    return 0;
  }

  std::unique_lock<std::mutex> lock(task->m);
  if (task->notify_count == 0) {
    if (xTicksToWait == portMAX_DELAY) {
      task->cv.wait(lock, [task] { return task->notify_count != 0; });
    } else {
      task->cv.wait_for(lock, std::chrono::milliseconds(xTicksToWait),
                         [task] { return task->notify_count != 0; });
    }
  }

  uint32_t count = task->notify_count;
  if (xClearCountOnExit) {
    task->notify_count = 0;
  } else if (count > 0) {
    task->notify_count = count - 1;
  }
  return count;
}

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t* pxHigherPriorityTaskWoken) {
  if (pxHigherPriorityTaskWoken) {
    *pxHigherPriorityTaskWoken = pdFALSE;
  }
  if (!xTaskToNotify) {
    return;
  }
  auto* task = static_cast<NativeTask*>(xTaskToNotify);
  {
    std::lock_guard<std::mutex> lock(task->m);
    task->notify_count++;
  }
  task->cv.notify_one();
}

BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify) {
  vTaskNotifyGiveFromISR(xTaskToNotify, nullptr);
  return pdTRUE;
}

namespace {
// Un seul type de mutex récursif+temporisé sert pour mutex normal ET
// récursif: plus permissif qu'un vrai mutex non-récursif, jamais moins sûr.
struct NativeSem {
  std::recursive_timed_mutex m;
};
}

SemaphoreHandle_t xSemaphoreCreateMutex() { return new NativeSem(); }
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { return new NativeSem(); }

BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) {
  auto* s = static_cast<NativeSem*>(sem);
  if (timeout == portMAX_DELAY) {
    s->m.lock();
    return pdTRUE;
  }
  return s->m.try_lock_for(std::chrono::milliseconds(timeout)) ? pdTRUE : pdFALSE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) {
  static_cast<NativeSem*>(sem)->m.unlock();
  return pdTRUE;
}

BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t sem, TickType_t timeout) {
  return xSemaphoreTake(sem, timeout);
}

BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t sem) {
  return xSemaphoreGive(sem);
}
