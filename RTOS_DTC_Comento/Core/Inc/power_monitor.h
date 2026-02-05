#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <stdbool.h>

void PowerMonitor_Task(void);
bool PMIC_IsUV(void);

#endif
