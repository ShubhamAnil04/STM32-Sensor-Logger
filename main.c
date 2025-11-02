#include "stm32f405xx.h"
#include <stdint.h>

#include "system_config.h"
#include "buffer.h"
#include "sensor.h"
#include "logger.h"

/* Fallback SystemCoreClock if system_stm32f4xx.c absent */
#ifndef SystemCoreClock
uint32_t SystemCoreClock = 16000000U; /*HSI 16 MHz */
#endif

/* Global ms tick */
volatile uint32_t g_millis = 0;

/* SysTick register addresses (Cortex-M) - we use them directly to avoid core_cm4.h */
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010U)  /* SysTick Control & Status */
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014U)  /* SysTick Reload Value */
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018U)  /* SysTick Current Value */
#define SYST_CALIB (*(volatile uint32_t *)0xE000E01CU)  /* SysTick Calibration Value */

/* SysTick CONTROL bits (masks) */
#define SYST_CSR_ENABLE_Pos    0U
#define SYST_CSR_TICKINT_Pos   1U
#define SYST_CSR_CLKSOURCE_Pos 2U
#define SYST_CSR_ENABLE_Msk    (1UL << SYST_CSR_ENABLE_Pos)
#define SYST_CSR_TICKINT_Msk   (1UL << SYST_CSR_TICKINT_Pos)
#define SYST_CSR_CLKSOURCE_Msk (1UL << SYST_CSR_CLKSOURCE_Pos)

/* Minimal replacements for core functions (avoid core_cm4.h) */
static inline void __WFI_fallback(void) { __asm volatile ("wfi"); }
#define __WFI() __WFI_fallback()

/* NVIC enable: write to NVIC ISER registers directly (0xE000E100) */
static inline void NVIC_EnableIRQ_fallback(int32_t irqn)
{
    uint32_t reg = (uint32_t)0xE000E100U + ((irqn >> 5) * 4U);
    uint32_t bit = 1UL << (irqn & 0x1F);
    (*(volatile uint32_t *)reg) = bit;
}

/* NVIC set priority (8-bit IPR registers at 0xE000E400) */
static inline void NVIC_SetPriority_fallback(int32_t irqn, uint32_t priority)
{
    /* Each IPR register is a byte per IRQ */
    uint32_t addr = 0xE000E400U + ((uint32_t)irqn);
    /* Many implementations use only high bits of priority; we store the value shifted to MSB nibble like CMSIS does */
    (*(volatile uint8_t *)addr) = (uint8_t)(priority & 0xFFU);
}

/* Use these instead of the CMSIS functions if core header absent */
#define NVIC_EnableIRQ(x) NVIC_EnableIRQ_fallback((int32_t)(x))
#define NVIC_SetPriority(x,p) NVIC_SetPriority_fallback((int32_t)(x),(uint32_t)(p))

/* SysTick handler (we implement via vector table in startup) but we will use register-level SYSTICK->CTRL interrupt */
void SysTick_Handler(void)
{
    g_millis++;
}

/* We don't have SysTick_Type symbol, so use direct registers for setup */
static void systick_init_1ms(void)
{
    /* SysTick LOAD = (SystemCoreClock/1000) - 1 */
    uint32_t reload = (SystemCoreClock / 1000U);
    if (reload) reload -= 1U;
    SYST_RVR = reload;
    SYST_CVR = 0U; /* clear current value */
    /* Enable SysTick: CLKSOURCE = processor clock, TICKINT = 1, ENABLE = 1 */
    SYST_CSR = SYST_CSR_CLKSOURCE_Msk | SYST_CSR_TICKINT_Msk | SYST_CSR_ENABLE_Msk;
}

/* TIM2 IRQ handler - must match your vector table name TIM2_IRQHandler */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; /* clear update flag */
        SensorData d = sensor_read();
        (void)buffer_push(d);
    }
}

/* TIM2 init: set PSC and ARR to generate interrupt every 'ms' milliseconds */
static void tim2_init_for_sample_ms(uint32_t ms)
{
    /* Enable TIM2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Simple prescaler: try to set timer tick = 1 kHz (1 ms tick) */
    uint32_t timer_clock = SystemCoreClock; /* adjust if APB1 prescaler != 1 */
    uint32_t prescaler = (timer_clock / 1000U);
    if (prescaler) prescaler -= 1U;
    if (prescaler > 0xFFFFU) prescaler = 0xFFFFU;
    TIM2->PSC = (uint16_t)prescaler;

    TIM2->ARR = (ms - 1U);
    TIM2->CNT = 0U;
    TIM2->SR  = 0U;
    TIM2->DIER |= TIM_DIER_UIE; /* update interrupt enable */
    TIM2->CR1 |= TIM_CR1_CEN;   /* start */
}

/* Enable NVIC for TIM2 */
static void enable_irq_tim2(void)
{
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 2U);
}


static void SystemClock_Config_Default(void)
{
}


int main(void)
{
    SystemClock_Config_Default();
    systick_init_1ms();

    buffer_init();
    sensor_init();
    logger_init();

    tim2_init_for_sample_ms(SAMPLE_INTERVAL_MS);
    enable_irq_tim2();

    uint32_t last_log = g_millis;
    while (1) {
        uint32_t now = g_millis;
        if ((now - last_log) >= LOG_INTERVAL_MS) {
            last_log = now;
            logger_task();
        }
        /* Wait for interrupt (low-power hint) */
        __WFI();
    }
}
