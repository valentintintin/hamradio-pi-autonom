#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

// Mutex récursif partagé par AprsEngine et CommandHandler pour éviter un interblocage AB-BA entre les deux.
class RecursiveLockGuard {
public:
  explicit RecursiveLockGuard(SemaphoreHandle_t sem) : _sem(sem) {
    xSemaphoreTakeRecursive(_sem, portMAX_DELAY);
  }
  ~RecursiveLockGuard() {
    xSemaphoreGiveRecursive(_sem);
  }
private:
  SemaphoreHandle_t _sem;
};

class LockGuard {
public:
  explicit LockGuard(SemaphoreHandle_t sem) : _sem(sem) {
    xSemaphoreTake(_sem, portMAX_DELAY);
  }
  ~LockGuard() {
    xSemaphoreGive(_sem);
  }
private:
  SemaphoreHandle_t _sem;
};
