#include "sensor.h"
#include "stm32f405xx.h"
#include <stdint.h>

/*
 Pin assumptions (change if your board differs):
  - PA0 -> ADC1_IN0 (temperature sensor input)
  - PA1 -> ADC1_IN1 (light sensor input)
  - Configure these pins as analog.
*/

/* helper to get ms tick */
extern volatile uint32_t g_millis;

/* ADC helper: single conversion of given channel (0..15) */
static uint16_t adc_single_read(uint8_t channel)
{
    
    
    ADC1->SQR3 = channel & 0x1F;        
    
    ADC1->CR2 |= ADC_CR2_SWSTART;
    
    while ((ADC1->SR & ADC_SR_EOC) == 0) { /* wait */ }
    uint16_t val = (uint16_t)(ADC1->DR & 0xFFFF);
    
    return val & 0x0FFF; 
}

void sensor_init(void)
{
    
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    
    GPIOA->MODER |= (3U << (0*2)) | (3U << (1*2)); 
    GPIOA->PUPDR &= ~((3U << (0*2)) | (3U << (1*2)));

    
    
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |= (ADC_CCR_ADCPRE_1); 
    
    ADC1->CR1 = 0;
    ADC1->CR2 = 0;
    
    
    
    
    uint32_t smpr = 0;
    
    ADC1->SMPR2 |= (7U << (0*3));
    ADC1->SMPR2 |= (7U << (1*3));

    
    ADC1->CR2 |= ADC_CR2_ADON;
    
    for (volatile int i=0;i<1000;i++);

    
}

SensorData sensor_read(void)
{
    SensorData d;
    d.timestamp = g_millis;
    uint16_t raw_temp = adc_single_read(0); 
    uint16_t raw_light = adc_single_read(1); 

    
    
    d.temperature = ((float)raw_temp) * (100.0f / 4095.0f);
    d.light = raw_light;
    return d;
}
