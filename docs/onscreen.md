# Onscreen menu

AWTRIX 3 provides a **onscreen menu** directly on your clock.  
Press and hold the middle button for 2 seconds to access the menu.   
Navigate through the items with the left and right buttons and choose the submenu with a push on the middle button.  
Hold down the middle button for 2s to exit the current menu and to save your setting.  
  
!>  You can easily turn your AWTRIX matrix on or off by simply double-pressing the middle button if youre not in Menu.
    
| Menu Item | Description |
| --- | --- |
| `BRIGHT` | Allows adjustment of the brightness of the display. Switch between Auto and manual brightnesscontrol with the middle button. |
| `COLOR` | Allows selection of one of 13 different colors for text. Hex values displayed.  |
| `SWITCH` | Determines if pages should automatically switch. |
| `T-SPEED` | Adjusts transition speed between apps. |
| `APPTIME` | Adjusts duration of app display before switching to next. |
| `TIME` | Allows selection of time format. |
| `DATE` | Allows selection of date format. |
| `WEEKDAY` | Allows selection of start of week. |
| `LANGUAGE` | Menu language: English (EN), Deutsch (DE), Español (ES), Français (FR). |
| `TEMP` | Allows selection of temperature system (°C or °F).  |
| `APPS` | Allows to enable or disable internal apps  |
| `SOUND` | Allows to enable or disable sound output   |
| `VOLUME` | Sound volume (if supported). |
| `CAL` | **Pixel calibration**: full red/green/blue screens to check uniformity, and per-pixel gain (100–200%) to compensate weak or different LEDs. See below. |
| `UPDATE` | Check and download new firmware if available. |

### Pixel calibration (CAL)

From the menu, select **CAL** and press the middle button to enter calibration mode.

- **Left / Right**: In colour mode, cycle **Red → Green → Blue**. In pixel mode, move the selected pixel (cursor) left/right.
- **Middle (short)**: In colour mode, switch to **pixel mode** (dim background + one bright pixel). In pixel mode, cycle the gain for the selected pixel: 100% → 120% → 150% → 180% → 200% (to boost weak LEDs).
- **Middle (long, 2 s)**: Save gains and exit calibration. Gains are stored and applied to the display from then on.

The onscreen menu can be displayed in four languages. Choose **LANGUAGE** and cycle with left/right to select **EN** (English), **DE** (Deutsch), **ES** (Español), or **FR** (Français). The setting is saved when you leave the menu (long press middle button).

You can also set the language from the **embedded web**: open `http://<IP>/backup`, click **Language** (or Idioma / Sprache / Langue), then choose the language. The pages **/backup** and **/lang** are shown in the selected language. To force a language in the URL use `?lang=0` (EN), `?lang=1` (DE), `?lang=2` (ES), `?lang=3` (FR), e.g. `http://<IP>/backup?lang=2` for Spanish.

