#ifndef LOGGER_H
#define LOGGER_H

#include "system_config.h"

void logger_init(void);
void logger_write(const SensorData *d);
void logger_task(void); 

#endif
