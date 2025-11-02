#ifndef BUFFER_H
#define BUFFER_H

#include "system_config.h"
#include <stdbool.h>

void buffer_init(void);
bool buffer_push(SensorData data);    
bool buffer_pop(SensorData *data);    
bool buffer_is_empty(void);
bool buffer_is_full(void);

#endif
