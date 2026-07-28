#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

// ============================================================================
// vTaskDelete(nullptr) — seul task_watchdog.cpp l'utilise, toujours en
// auto-suppression (jamais avec le handle d'une AUTRE tâche). On simule ça en
// déroulant la pile du thread via une exception dédiée, attrapée juste ici
// dans le wrapper de xTaskCreate — la tâche sort proprement sans tuer le
// process.
// ============================================================================
namespace {
struct TaskSelfDelete {};

// ============================================================================
// Notification de tâche — un compteur protégé par mutex/condition_variable,
// équivalent à "un réveil binaire" (seul usage fait par ce projet : voir
// ulTaskNotifyTake ci-dessous). thread_local pointe vers le NativeTask du
// thread courant, posé au tout début du thread créé par xTaskCreate — c'est
// ce qui permet à xTaskGetCurrentTaskHandle() de répondre correctement même
// appelé tout en haut de la fonction de tâche.
// ============================================================================
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
      // sortie propre demandée par vTaskDelete(nullptr)
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
    return 0;  // appelée hors d'une tâche créée par xTaskCreate — ne devrait pas arriver
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
    *pxHigherPriorityTaskWoken = pdFALSE;  // jamais utilisé en natif, cf. FreeRTOS.h
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

// ============================================================================
// Sémaphores — un seul type de mutex récursif+temporisé sous-jacent sert pour
// mutex normal ET récursif : plus permissif qu'un vrai mutex non-récursif,
// jamais moins sûr (un thread ne se bloque jamais lui-même).
// ============================================================================
namespace {
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
