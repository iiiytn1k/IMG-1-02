/*
**************************************************************************************
*   IMG-1-02 Plasma Display Driver
**************************************************************************************
*    MIT License
*
*    Copyright (c) 2021 iiiytn1k
*
*    Permission is hereby granted, free of charge, to any person obtaining a copy
*    of this software and associated documentation files (the "Software"), to deal
*    in the Software without restriction, including without limitation the rights
*    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
*    of the Software, and to permit persons to whom the Software is furnished to do so,
*    subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
*    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
*    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
*    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************

  PA10  <------ USART1_RX
  PA4   ------> SPI1_CS (LAT)
  PA5   ------> SPI1_SCK (CLK)
  PA7   ------> SPI1_MOSI (DATA)

*/

#include <libopencm3/stm32/rcc.h>
#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>
#include <string.h>

#include "splash_screen.h"

#define USART_BAUDRATE      2000000UL
#define RX_BUFFER_SIZE      13 * 100 * 2
#define CS_UP()             GPIO_BSRR(GPIOA) = GPIO4
#define CS_DWN()            GPIO_BRR(GPIOA) = GPIO4

void SPI1_SendByte(uint8_t data);
void IMG1_show_row(uint8_t buffer_offset);
static void DMA1_USART_RX_setup(void);
void DMA1_RecieveComplete(void);

volatile uint8_t refresh_flag = 0;
volatile uint8_t buffer_num = 1;
volatile int8_t current_row = 0;

static const uint8_t offset_seq[3]={0, 100, 100};
static const uint8_t row_count[100]={
        0x99, 0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89, 0x09,
        0x91, 0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81, 0x01,
        0x9E, 0x1E, 0xEE, 0x6E, 0xAE, 0x2E, 0xCE, 0x4E, 0x8E, 0x0E,
        0x96, 0x16, 0xE6, 0x66, 0xA6, 0x26, 0xC6, 0x46, 0x86, 0x06,
        0x9A, 0x1A, 0xEA, 0x6A, 0xAA, 0x2A, 0xCA, 0x4A, 0x8A, 0x0A,
        0x92, 0x12, 0xE2, 0x62, 0xA2, 0x22, 0xC2, 0x42, 0x82, 0x02,
        0x9C, 0x1C, 0xEC, 0x6C, 0xAC, 0x2C, 0xCC, 0x4C, 0x8C, 0x0C,
        0x94, 0x14, 0xE4, 0x64, 0xA4, 0x24, 0xC4, 0x44, 0x84, 0x04,
        0x98, 0x18, 0xE8, 0x68, 0xA8, 0x28, 0xC8, 0x48, 0x88, 0x08,
        0x90, 0x10, 0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00
};
volatile uint8_t rx_buffer_2[2600]={};

void SPI1_SendByte(uint8_t data) {
    SPI_DR(SPI1) = (uint8_t)(data);
    while (!(SPI_SR(SPI1) & SPI_SR_TXE)) {}
    while (SPI_SR(SPI1) & SPI_SR_BSY) {}
}

void IMG1_show_row(uint8_t buffer_offset) {
    uint8_t byte_index = 0;
    if (buffer_num == 1) {
        CS_DWN();
        for (byte_index = 0; byte_index < 13; byte_index++) {
            SPI1_SendByte(rx_buffer_1[(buffer_offset+current_row) * 13 + byte_index]);
    }
        SPI1_SendByte(row_count[current_row]);
        CS_UP();
    } else {
        CS_DWN();
        for (byte_index = 0; byte_index < 13; byte_index++) {
            SPI1_SendByte(rx_buffer_2[(buffer_offset+current_row) * 13 + byte_index]);
        }
        SPI1_SendByte(row_count[current_row]);
        CS_UP();
    }
}

static void clock_setup(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_DMA1);
}

static void USART1_setup(void) {
    /* Setup GPIO pin GPIO_USART1_TX and GPIO_USART1_RX. */
    /*gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);*/
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
              GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    /* Setup UART parameters. */
    usart_set_baudrate(USART1, USART_BAUDRATE);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    /* Finally enable the USART. */
    usart_enable(USART1);

    /* Enable DMA1 Ch5 IRQ */
    nvic_set_priority(NVIC_DMA1_CHANNEL5_IRQ, 0);
    nvic_enable_irq(NVIC_DMA1_CHANNEL5_IRQ);
}

static void TIM2_setup(void) {
    /* Enable TIM2 clock. */
    rcc_periph_clock_enable(RCC_TIM2);

    /* Enable TIM2 interrupt. */
    /* Set lowest priority. */
    nvic_set_priority(NVIC_TIM2_IRQ, 64);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    /* Reset TIM2 peripheral to defaults. */
    rcc_periph_reset_pulse(RST_TIM2);

    timer_set_prescaler(TIM2, 0);

    /* Disable preload. */
    timer_disable_preload(TIM2);
    timer_continuous_mode(TIM2);

    /* 36kHz horizontal frequency = 60 Hz framerate*/
    timer_set_period(TIM2, 2000);

    /* Set the initual output compare value for OC1. */
    timer_set_oc_value(TIM2, TIM_OC1, 0);

    /* Counter enable. */
    timer_enable_counter(TIM2);


    /* Enable Channel 1 compare interrupt */
    timer_enable_irq(TIM2, TIM_DIER_CC1IE);
}

void tim2_isr(void) {
    if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {
        refresh_flag = 1;

        /* Clear compare interrupt flag. */
        timer_clear_flag(TIM2, TIM_SR_CC1IF);
    }
}

static void DMA1_USART_RX_setup() {
    /*
     * Using DMA1 channel 5 for USART1_RX
     */
    dma_channel_reset(DMA1, DMA_CHANNEL5);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL5, (uint32_t)&USART1_DR);
    dma_set_memory_address(DMA1, DMA_CHANNEL5, (uint32_t)&rx_buffer_2);
    dma_set_number_of_data(DMA1, DMA_CHANNEL5, RX_BUFFER_SIZE);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL5);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL5);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL5, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL5, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL5, DMA_CCR_PL_HIGH);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL5);
    dma_enable_channel(DMA1, DMA_CHANNEL5);
    usart_enable_rx_dma(USART1);
}

void dma1_channel5_isr(void) {
    if ((DMA1_ISR &DMA_ISR_TCIF5) != 0) {
        DMA1_IFCR |= DMA_IFCR_CTCIF5;
        DMA1_RecieveComplete();
    }
}

void DMA1_RecieveComplete(void) {
    dma_disable_channel(DMA1, DMA_CHANNEL5);
    if (buffer_num == 1) {
        buffer_num = 2;
        dma_set_memory_address(DMA1, DMA_CHANNEL5, (uint32_t)&rx_buffer_1);
        dma_set_number_of_data(DMA1, DMA_CHANNEL5, RX_BUFFER_SIZE);
    } else {
        buffer_num = 1;
        dma_set_memory_address(DMA1, DMA_CHANNEL5, (uint32_t)&rx_buffer_2);
        dma_set_number_of_data(DMA1, DMA_CHANNEL5, RX_BUFFER_SIZE);
    }
    dma_enable_channel(DMA1, DMA_CHANNEL5);
}

static void SPI1_setup(void) {
    /**SPI1 GPIO Configuration
    PA4   ------> SPI1_SS
    PA5   ------> SPI1_SCK
    PA7   ------> SPI1_MOSI
    */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    spi_reset(SPI1);
    SPI1_I2SCFGR = 0;

    /* Set up SPI in Master mode with:
    * Clock freq: 9 MHz
    * Clock polarity: Idle low
    * Clock phase: Data valid on 1nd clock pulse
    * Data frame format: 8-bit
    * Frame format: LSB First
    */
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_LSBFIRST);

    /*
     * Set NSS management to software.
    */
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);

    /* Enable SPI1 periph. */
    spi_enable(SPI1);
}

int main(void) {
    uint8_t row_upwards = 1;
    uint8_t offset_index = 0;

    clock_setup();
    USART1_setup();
    SPI1_setup();
    DMA1_USART_RX_setup();
    TIM2_setup();

    while (1) {
        if(refresh_flag) {
            IMG1_show_row(offset_seq[offset_index]);
            offset_index++;
            if (offset_index == 3) {
                offset_index = 0;
                if (row_upwards) {
                    current_row++;
                    if (current_row > 98) row_upwards = 0;
                } else {
                    current_row--;
                    if (current_row < 1) row_upwards = 1;
                }
            }
            refresh_flag = 0;
        }
    }
    return 0;
}
