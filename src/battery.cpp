#include "battery.hpp"
#include "settings.hpp"

volatile uint8_t low_batt_count = 0;

uint8_t get_battery_capacity()
{
    uint8_t percentage = 100;

    float voltage = analogRead(35) / 4096.0 * 7.46;
    if (voltage > 1 ) { // Only display if there is a valid reading
        percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;

        percentage = percentage > 100? 100 : percentage;
        percentage = percentage < 0? 0 : percentage;
    }

    dbgPrintln("Voltage: " + String(voltage) + ", percentage: " + String(percentage));

    return percentage;
}