#ifndef MENUMANAGER_I18N_H
#define MENUMANAGER_I18N_H

#include <Arduino.h>
#include "Globals.h"

extern const char *menuItems[];
extern const char *menuItems_DE[];
extern const char *menuItems_ES[];
extern const char *menuItems_FR[];
extern uint8_t MENU_LANGUAGE;

inline const char *getMenuItem(uint8_t index)
{
    if (MENU_LANGUAGE == 0) return menuItems[index];
    if (MENU_LANGUAGE == 1) return menuItems_DE[index];
    if (MENU_LANGUAGE == 2) return menuItems_ES[index];
    return menuItems_FR[index];
}
inline const char *getStrOn() { return (MENU_LANGUAGE == 0) ? "ON" : (MENU_LANGUAGE == 1) ? "AN" : (MENU_LANGUAGE == 2) ? "SI" : "OUI"; }
inline const char *getStrOff() { return (MENU_LANGUAGE == 0) ? "OFF" : (MENU_LANGUAGE == 1) ? "AUS" : (MENU_LANGUAGE == 2) ? "NO" : "NON"; }
inline const char *getStrMon() { return (MENU_LANGUAGE == 0) ? "MON" : (MENU_LANGUAGE == 1) ? "MO" : (MENU_LANGUAGE == 2) ? "LU" : "LU"; }
inline const char *getStrSun() { return (MENU_LANGUAGE == 0) ? "SUN" : (MENU_LANGUAGE == 1) ? "SO" : (MENU_LANGUAGE == 2) ? "DO" : "DI"; }
inline const char *getStrNa() { return (MENU_LANGUAGE == 0) ? "N/A" : (MENU_LANGUAGE == 1) ? "N/V" : (MENU_LANGUAGE == 2) ? "N/D" : "N/A"; }
inline const char *getLangName() { return (MENU_LANGUAGE == 0) ? "EN" : (MENU_LANGUAGE == 1) ? "DE" : (MENU_LANGUAGE == 2) ? "ES" : "FR"; }

#endif
