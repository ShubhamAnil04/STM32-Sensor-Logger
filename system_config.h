#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE         32u      
#define SAMPLE_INTERVAL_MS  100u     
#define LOG_INTERVAL_MS     1000u    

typedef struct {
    uint32_t timestamp;     
    float temperature;      
    uint16_t light;         
} SensorData;

#endif
