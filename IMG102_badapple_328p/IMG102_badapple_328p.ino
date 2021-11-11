/*****************************************
Схема соединения панели с Arduino Nano v3:

VCC (Красый)    --> 5V
LAT (Оранжевый) --> 10 (PB2, SS)
CLK (Желтый)    --> 13 (PB5, SCK)
DATA (Зелёный)  --> 11 (PB3, MOSI)
GND (Чёрный)    --> GND
******************************************/

#include <SPI.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "splash.h"

#define RX_BUFFER_SIZE 1300
volatile uint8_t received_bytes_count = 0, buffer_y = 0, buffer_x = 0;

static const uint8_t row_count_LSB[100]={
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

int8_t current_row = 0;
const int slaveSelectPin = 10;

void USART0_init(void) {
    UCSR0B |= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);  // 8data, 1 stop bit.
    UBRR0 = 0;  // 1M
}

ISR(USART_RX_vect) {
    rx_buffer[buffer_y][buffer_x] = UDR0;
    buffer_x++;
    if (buffer_x == 13) {
        buffer_x = 0;
        buffer_y++;
    }

    if (buffer_y == 100) {
        received_bytes_count = 0;
        buffer_x = 0;
        buffer_y = 0;
    }
}

void setup() {
    pinMode(slaveSelectPin, OUTPUT);
    SPI.setBitOrder(LSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPSR |= (1 << SPI2X);
    SPI.begin();
    USART0_init();
    _delay_ms(20);
    sei();
}

void IMG1_Send_framebuffer(uint8_t current_row) {
    uint8_t current_byte = 0;
    PORTB &= ~(1 << 2);     // digitalWrite(slaveSelectPin, LOW);
    for (current_byte = 0; current_byte < 13; current_byte++) {
        SPI.transfer(rx_buffer[current_row][current_byte]);
    }
    SPI.transfer(row_count_LSB[current_row]);
    PORTB |= (1 << 2);      // digitalWrite(slaveSelectPin, HIGH);
   // _delay_us(7);
}

void loop() {
// Прогон сверху-вниз
    for (current_row = 0; current_row < 100 ; current_row++) {
        IMG1_Send_framebuffer(current_row);
    }
// Прогон снизу-верх
    for (current_row = 98; current_row > 0 ; current_row--) {
        IMG1_Send_framebuffer(current_row);
    }
}
