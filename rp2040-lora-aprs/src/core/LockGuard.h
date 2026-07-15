#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

// ============================================================================
// LockGuard / RecursiveLockGuard — verrous RAII génériques.
//
// RecursiveLockGuard est utilisé par AprsEngine et CommandHandler, qui
// partagent le même mutex récursif (cf. AprsEngine::getMutex()) pour éviter
// un interblocage AB-BA entre les deux classes.
// LockGuard (non récursif) est utilisé par AprsDispatcher pour protéger sa
// file d'émission.
// ============================================================================

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
