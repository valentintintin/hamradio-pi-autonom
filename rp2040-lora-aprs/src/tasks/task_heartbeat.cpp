#include "task_heartbeat.h"

volatile unsigned long g_heartbeat_ms[HB_COUNT] = {0};
