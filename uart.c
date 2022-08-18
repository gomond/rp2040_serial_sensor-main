/*****************************************************************************/
//
/* Attempt to perform a single read from a DfRobot A01NYUB serial Sensor     */
// Greg Omond 13-Aug-2022
//

#include <time.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/sleep.h"
#include "hardware/rtc.h"

static bool awake;

static void sleep_callback(void){
    awake = true;
}

static void rtc_sleep(void) {

  datetime_t t = {
            .year  = 2020,
            .month = 8,
            .day   = 19,
            .dotw  = 5,
            .hour  = 10,
            .min   = 10,
            .sec   = 00
  };
  datetime_t t_alarm = {
            .year  = 2020,
            .month = 8,
            .day   = 19,
            .dotw  = 5,
            .hour  = 10,
            .min   = 20,    // 10 minutes after
            .sec   = 00
  };
  rtc_init();
  rtc_set_datetime(&t);

  sleep_goto_sleep_until(&t_alarm, &sleep_callback);
}
unsigned char data[4];
/// \tag::uart_advanced[]

#define UART_ID uart0
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1
uint8_t ch=0;
static int chars_rxed = 0;
int16_t distance;  // The last measured distance
bool newData = false; // Whether new data is available from the sensor
uint8_t buffer[4];  // our buffer for storing data
uint8_t idx = 0;  // our idx into the storage buffer

// Serial Data Recieve handler

int16_t read_serial_sensor() {
    while (uart_is_readable(UART_ID)) {
    uint8_t c = uart_getc(UART_ID);
    // See if this is a header byte
    if (idx == 0 && c == 0xFF) {
      buffer[idx++] = c;
    }
    // Two middle bytes can be anything
    else if ((idx == 1) || (idx == 2)) {
      buffer[idx++] = c;
    }
    else if (idx == 3) {
      uint8_t sum = 0;
      sum = buffer[0] + buffer[1] + buffer[2];
      if (sum == c) {
        distance = ((uint16_t)buffer[1] << 8) | buffer[2];
        newData = true;
      }
      idx = 0;
    }
    //busy_wait_us(1500);
    }
    if (newData) {
    // printf("Distance: ");
    // printf("%d",distance);
    // printf(" mm \n");
    newData = false;
    return(distance);
    }
}

int main() {
    stdio_init_all();

    /* Alternative deeper sleep mode */
    sleep_run_from_xosc();
    
    awake = false;
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 9600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, true);

    rtc_sleep(); // Put the Pico to sleep

    while (1){
    uint16_t usdat = 0;
    busy_wait_us(6000000);
    usdat = read_serial_sensor();
    printf("Distance: ");
    printf("%d",usdat);
    printf(" mm \n");
    }
       
}


