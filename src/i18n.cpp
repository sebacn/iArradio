#include "i18n.hpp"

int LANG = LANG_EN;

const char* LANGS[] = { "en" };

const char* WEEKDAYS[][7] = {
    { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" },
};

const char* get_weekday(int day) {
    return WEEKDAYS[LANG][day];
}