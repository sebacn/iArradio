#include "user_input_buttons.hpp"
#include "tasks.hpp"

extern WiFiManager wifiManager;
extern Settings settings;

EasyButton home_button(HOME_BUTTON, DEBOUNCETIME);
EasyButton prev_button(PREV_BUTTON, DEBOUNCETIME);
EasyButton next_button(NEXT_BUTTON, DEBOUNCETIME);
EasyButton ok_button(OK_BUTTON, DEBOUNCETIME);
EasyButton vol_inc_button(VOL_INC_BUTTON, DEBOUNCETIME);
EasyButton vol_dec_button(VOL_DEC_BUTTON, DEBOUNCETIME);

//bool volume_mode = false;

void configure_buttons()
{
    home_button.begin();
    prev_button.begin();
    next_button.begin();
    ok_button.begin();
    vol_inc_button.begin();
    vol_dec_button.begin();

    home_button.onPressed(handle_home);
    home_button.onPressedFor(HOLD_TIMEOUT, handle_home_timeout);
    prev_button.onPressed(handle_prev_button);
    next_button.onPressed(handle_next_button);
    ok_button.onPressed(handle_ok_button);
    vol_inc_button.onPressed(handle_vol_inc_button);
    vol_dec_button.onPressed(handle_vol_dec_button);
}

void buttons_rutine()
{
    home_button.read();
    prev_button.read();
    next_button.read();
    ok_button.read();
    vol_inc_button.read();
    vol_dec_button.read();
}

void handle_prev_button()
{
    change_station(-1);
}

void handle_next_button()
{
    change_station(1);
}

void handle_ok_button()
{
    llog_e("OK Buttin");
}

void handle_vol_inc_button()
{
    increase_volume(1);
}

void handle_vol_dec_button()
{
    increase_volume(-1);
}

void handle_home_timeout()
{
    power_off();
}

void handle_home()
{
    if (wifiManager.getWebPortalActive())
    {
        wifiManager.stopWebPortal();

        if (settings.ConfigOk)
        {
            //try to connect wifi
            logo_screen("Try to connect WiFi..");
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            logo_screen("WiFi not connected, switch off..");
            delay(3000);
            power_off();
        }
        return;
    }

    //xTaskCreate(task_epaper_cursor, "TaskEpaperCursor", 5000, &volume_mode, 1, NULL);
}