#pragma once

void bootInitCore();
void bootInitEeprom();
// Idempotente : réappelée périodiquement par tasks/task_watchdog.cpp tant que le RTC externe échoue.
bool bootInitRtc();
void bootLoadConfig();
void bootInitRadios();
void bootSeedRng();
void bootInitIdentity();
void bootInitSensors();
void bootInitRelays();
void bootInitMeshAndAprs();
void bootCreateTasks();
