#include "tasks.hpp"
#include "fmt.hpp"

String time_str;
volatile bool updating = false;
uint8_t old_capacity = 0;
int old_rssi = 0;
String old_stream_title = "";
String old_station_title = "";

extern Settings settings;
extern String stream_title;
extern String station_title;
extern bool power_off_on;

void set_updating(bool value)
{
    while (updating && value) //wait updating completed
    {
        vTaskDelay(portTICK_PERIOD_MS * 100);
    }
    updating = value;
}

void task_time(void *parameter)
{
    for (;;)
    {        
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            llog_d("Task Redraw time");
            epaper_redraw_time();
            updating = false;
            vTaskDelay(portTICK_PERIOD_MS * 1000);
        }
        vTaskDelay(portTICK_PERIOD_MS * 30000);
    }
    vTaskDelete(NULL);
}


void task_weather(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            if (!updating)
            {
                updating = true;
                llog_d("Task Redraw weather");
                //weather_update_rutine();
                updating = false;
            }
            vTaskDelay(portTICK_PERIOD_MS * 32);
        }
        vTaskDelay(portTICK_PERIOD_MS * 1200000L); //20min
    }
    vTaskDelete(NULL);
}

void task_epaper_battery(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            uint8_t capacity = get_battery_capacity();
            if (capacity != old_capacity)
            {
                llog_d("Task Redraw battery cap: %d, old_capacity: %d", capacity, old_capacity);
                old_capacity = capacity;
                epaper_redraw_battery(capacity);
            }
            updating = false;
            vTaskDelay(portTICK_PERIOD_MS * 30500); //30 sec
        }
        vTaskDelay(portTICK_PERIOD_MS * 24000); //24 sec
    }
    vTaskDelete(NULL);
}

/*
void task_epaper_header(void *parameter)
{
    for (;;)
    {
        if (!updating)
        {
            updating = true;
            epaper_draw_heading_section();
            updating = false;
        }
        vTaskDelay(portTICK_PERIOD_MS * 30000); // 30 sec
    }
    vTaskDelete(NULL);
}
*/

void task_epaper_rssi(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            int rssi = WiFi.RSSI();
            if (rssi != old_rssi && abs(rssi - old_rssi) > 3)
            {
                llog_d("Task Redraw: RSSI: %d, old_rssi: %d", rssi, old_rssi);
                old_rssi = rssi;
                epaper_redraw_rssi(rssi);
            }
            updating = false;
            vTaskDelay(portTICK_PERIOD_MS * 1250);
        }
        vTaskDelay(portTICK_PERIOD_MS * 15000); //150);
    }
    vTaskDelete(NULL);
}

void task_stream_title(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {     
            updating = true;           

            if (stream_title != old_stream_title)
            {
                //llog_d("Task Redraw station: %s, prev_station: %s", station_text.c_str(), old_stream_name.c_str());
                old_stream_title = stream_title;
                epaper_redraw_stream_title(stream_title, true);
            }      
            else if (stream_title != "")
            {
                epaper_redraw_stream_title(stream_title, false);
            }   

            if (station_title != old_station_title)
            {
                old_station_title = station_title;
                epaper_redraw_station_title(station_title);
            }  
               
            updating = false;
        }
        vTaskDelay(portTICK_PERIOD_MS * 500);
    }
    vTaskDelete(NULL);
}

void task_epaper_station_number(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            set_epaper_station_number(*((uint16_t *)parameter));
            updating = false;
            break;
        }
        vTaskDelay(portTICK_PERIOD_MS * 5);
    }
    vTaskDelete(NULL);
}

void task_eeprom_station(void *parameter)
{
    eeprom_set_station(*((uint8_t *)parameter));
    vTaskDelete(NULL);
}

void task_epaper_volume(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            set_epaper_volume(*((uint8_t *)parameter));
            updating = false;
            break;
        }
        vTaskDelay(portTICK_PERIOD_MS * 3);
    }
    vTaskDelete(NULL);
}

void task_eeprom_volume(void *parameter)
{
    eeprom_set_volume(*((uint8_t *)parameter));
    vTaskDelete(NULL);
}

void task_epaper_cursor(void *parameter)
{
    for (;;)
    {
        if (power_off_on)
        {
            break;
        }

        if (!updating)
        {
            updating = true;
            set_epaper_cursor(*((bool *)parameter));
            updating = false;
            break;
        }
        vTaskDelay(portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}