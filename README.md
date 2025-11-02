# STM32 Sensor Logger

A bare-metal firmware project for **STM32F405** that samples temperature and light sensor data using ADC, buffers it, and logs it through UART.

## Features
- Periodic ADC sampling using timer interrupts
- Circular buffer for data handling
- UART logging at defined intervals
- Written entirely in C (no HAL, no RTOS)

## File Overview
- `main.c` – System initialization and main loop
- `sensor.c/.h` – Sensor data acquisition
- `buffer.c/.h` – Circular buffer management
- `logger.c/.h` – UART logging
- `cmn.c/.h` – Common utility functions
- `syscalls.c`, `sysmem.c` – System stubs
- `system_config.h` – Configurable constants
