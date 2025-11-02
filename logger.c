/* logger.c - uses USART2 (PA2 TX, PA3 RX) for logging; fixed includes */
/* logger.c - uses USART2 (PA2 TX, PA3 RX) for logging; no core_cm4.h required */
#include "logger.h"
#include "buffer.h"
#include "stm32f405xx.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Forward UART helpers (user-style) */
void USART2_init(void);
void USART2_TX(uint8_t ch);
uint8_t USART2_RX(void);
void delayMs(int n);

/* Initialize logger (init UART) */
void logger_init(void)
{
    USART2_init();
}

/* helper to send a buffer via USART2 */
static void uart_tx_buf(const char *s, uint32_t len)
{
    for (uint32_t i = 0U; i < len; ++i) {
        USART2_TX((uint8_t)s[i]);
    }
}
static int format_temp_fixed(char *out, size_t out_sz, float temperature)
{
    /* Scale temperature by 100 and round to nearest integer */
    /* We use float multiply once; snprintf will not print floats. */
    int32_t scaled = (int32_t)(temperature * 100.0f + (temperature >= 0.0f ? 0.5f : -0.5f));
    int32_t integer_part = scaled / 100;
    int32_t frac_part = scaled % 100;
    if (frac_part < 0) frac_part = -frac_part;

    /* Write into out buffer using integer formatting (no %f) */
    /* Return number of bytes written */
    return snprintf(out, out_sz, "%d.%02d", (int)integer_part, (int)frac_part);
}

void logger_write(const SensorData *d)
{
    char buf[128];
    char temp_buf[32];

    /* Format temperature to temp_buf using integer-only formatting */
    format_temp_fixed(temp_buf, sizeof(temp_buf), d->temperature);

    /* Build final CSV line; note: we DO NOT use "%f" */
    int n = snprintf(buf, sizeof(buf), "%lu,%s,%u\r\n",
                     (unsigned long)d->timestamp,
                     temp_buf,
                     (unsigned int)d->light);

    if (n > 0) {
        uart_tx_buf(buf, (uint32_t)n);
    }
}

/* logger_task: pop all entries and send */
void logger_task(void)
{
    SensorData d;
    while (buffer_pop(&d)) {
        logger_write(&d);
    }
}

/* ---------------- USART2 (user's style) ---------------- */

/* Initialize USART2: PA2 TX, PA3 RX, 9600 @ 16 MHz (BRR = 0x0683 as provided) */
void USART2_init(void)
{
    /* Enable GPIOA and USART2 clocks */
    RCC->AHB1ENR |= (1U << 0);  /* GPIOAEN */
    RCC->APB1ENR |= (1U << 17); /* USART2EN */

    /* PA2 AF7, PA3 AF7 */
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |=  (2U << (2 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (4 * 2));
    GPIOA->AFR[0] |=  (7U << (4 * 2));

    GPIOA->MODER &= ~(3U << (3 * 2));
    GPIOA->MODER |=  (2U << (3 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (4 * 3));
    GPIOA->AFR[0] |=  (7U << (4 * 3));

    /* No pull-up/pull-down */
    GPIOA->PUPDR &= ~((3U << (2 * 2)) | (3U << (3 * 2)));

    /* Baud: 9600 @ 16MHz -> BRR 0x0683 (from your snippet) */
    USART2->BRR = 0x0683U;

    /* Enable RE and TE (bits 2 and 3) */
    USART2->CR1 |= (1U << 2) | (1U << 3);

    USART2->CR2 = 0U;
    USART2->CR3 = 0U;

    /* Enable USART (UE bit 13) */
    USART2->CR1 |= (1U << 13);
}

/* Blocking transmit of single byte */
void USART2_TX(uint8_t ch)
{
    /* Wait until TXE (bit 7) is set */
    while (!(USART2->SR & (1U << 7))) { }
    USART2->DR = (uint16_t)(ch & 0xFF);

    /* Wait until TC (bit 6) is set (transmission complete) */
    while (!(USART2->SR & (1U << 6))) { }
}

/* Blocking receive of single byte */
uint8_t USART2_RX(void)
{
    /* Wait until RXNE (bit 5) is set */
    while (!(USART2->SR & (1U << 5))) { }
    return (uint8_t)(USART2->DR & 0xFF);
}

/* crude busy-loop delay (keeps compatibility with your snippet) */
void delayMs(int n)
{
    for (; n > 0; n--) {
        for (volatile int i = 0; i < 2000; ++i) { __asm__("nop"); }
    }
}
