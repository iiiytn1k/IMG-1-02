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
#include <stdint.h>
#include <string.h>
#include "splash_screen.h"

#define USART_BAUDRATE      2000000UL
#define RX_BUFFER_SIZE      14 * 100 * 2
#define CS_UP()             GPIO_BSRR(GPIOA) = GPIO4
#define CS_DWN()            GPIO_BRR(GPIOA) = GPIO4

void SPI1_SendByte(uint8_t data);
void IMG1_show_row(uint8_t buffer_offset);
static void DMA1_SPI_TX_setup(void);
static void DMA1_USART_RX_setup(void);
void DMA1_RecieveComplete(void);

volatile uint8_t buffer_num = 1;
volatile int8_t current_row = 0;
static const uint8_t offset_seq[3]={100, 100, 0};
volatile uint8_t rx_buffer_2[2800]={};
uint8_t row_upwards = 1;
uint8_t offset_index = 0;

void SPI1_SendByte(uint8_t data) {
    SPI_DR(SPI1) = (uint8_t)(data);
    while (!(SPI_SR(SPI1) & SPI_SR_TXE)) {}
    while (SPI_SR(SPI1) & SPI_SR_BSY) {}
}

void IMG1_show_row(uint8_t buffer_offset) {
    if (buffer_num == 1) {
        dma_set_memory_address(DMA1, DMA_CHANNEL3, (uint32_t)&rx_buffer_1[(current_row + buffer_offset) * 14]);
    } else {
        dma_set_memory_address(DMA1, DMA_CHANNEL3, (uint32_t)&rx_buffer_2[(current_row + buffer_offset) * 14]);
    }
    dma_set_number_of_data(DMA1, DMA_CHANNEL3, 14);

    CS_DWN();
    dma_enable_channel(DMA1, DMA_CHANNEL3);
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
    /**USART1 GPIO Configuration
    PA10   ------> USART1_RX
    */
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

    nvic_set_priority(NVIC_TIM2_IRQ, 0);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    /* Reset TIM2 peripheral to defaults. */
    rcc_periph_reset_pulse(RST_TIM2);

    /* Disable preload. */
    timer_disable_preload(TIM2);

    /* Interrupt is called for display each row 3 times                                          */
    /* 60Hz refresh rate  (actually 120Hz because i used two-sided scanning)                     */
    /* Horizontal scan rate: 60 * 3 * 100 * 2 = 36000 Hz                                         */
    /* TIMupdateHZ = FCLK / ((prescaler + 1) * (period + 1))                                     */
    /* period = FCLK/(refresh rate)/(100 rows)/(3 (color frames))/(2 (two-sided scanning))-1     */
    /* (72000000/60/100/3/2)-1 = 1999                                                            */
    timer_set_prescaler(TIM2, 0);
    timer_set_period(TIM2, 1999);

    /* Enable update interrupt */
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    /* Counter enable. */
    timer_enable_counter(TIM2);
}

void tim2_isr(void) {
    if (timer_get_flag(TIM2, TIM_SR_UIF)) {
        timer_clear_flag(TIM2, TIM_SR_UIF);

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
    }
}

static void DMA1_SPI_TX_setup() {
    /* Using DMA1 channel 3 for SPI1_TX */
    dma_channel_reset(DMA1, DMA_CHANNEL3);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t)&SPI1_DR);
    dma_set_number_of_data(DMA1, DMA_CHANNEL3, 14);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL3);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL3, DMA_CCR_PL_HIGH);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);
    spi_enable_tx_dma(SPI1);
}

static void DMA1_USART_RX_setup() {
    /* Using DMA1 channel 5 for USART1_RX */
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

void dma1_channel3_isr(void) {
    if ((DMA1_ISR &DMA_ISR_TCIF3) != 0) {
        DMA1_IFCR |= DMA_IFCR_CTCIF3;
    }
    while (SPI_SR(SPI1) & SPI_SR_BSY) {}
    CS_UP();
    dma_disable_channel(DMA1, DMA_CHANNEL3);
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

    nvic_set_priority(NVIC_DMA1_CHANNEL3_IRQ, 0);
    nvic_enable_irq(NVIC_DMA1_CHANNEL3_IRQ);
}

int main(void) {
    clock_setup();
    USART1_setup();
    SPI1_setup();
    DMA1_SPI_TX_setup();
    DMA1_USART_RX_setup();
    TIM2_setup();
    while (1) { }
}
