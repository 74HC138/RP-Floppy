
#include <Arduino.h>
#include <pico.h>
#include "floppy.hpp"

void reset() __attribute__ ((noreturn));
void reset() {
    watchdog_reboot(0, 0, 1);
    while (true);
}

void setup() {
    Serial.begin();
    while (!Serial);
    delay(1000);
}

void loop() {
    Serial.printf("initializing floppy...");
    if (initFloppy() != 0) {
        Serial.printf("failed!\n");
    } else {
        Serial.printf("success.\n");
    }
    while (!BOOTSEL);
    while (BOOTSEL);
    //pio_set_irq0_source_enabled();
    //pio_interrupt_clear()
}
