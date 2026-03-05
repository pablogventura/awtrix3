#ifndef AppS_H
#define AppS_H

#include <map>
#include <vector>
#include "MatrixDisplayUi.h"
#include "effects.h"

#define MAX_CUSTOM_APPS 20

struct CustomApp
{
    int bounceDir = 0;
    bool hasCustomColor = false;
    uint8_t currentFrame = 0;
    String iconName;
    String drawInstructions;
    float scrollposition = 0;
    int16_t scrollDelay = 0;
    byte lifetimeMode = 0;
    String text;
    bool bounce = false;
    uint32_t color;
    File icon;
    bool isGif;
    bool rainbow;
    bool center;
    int fade = 0;
    int blink = 0;
    int effect = -1;
    long duration = 0;
    byte textCase = 0;
    int16_t repeat = 0;
    int16_t currentRepeat = 0;
    String name;
    OverlayEffect overlay;
    byte pushIcon = 0;
    float iconPosition = 0;
    bool iconWasPushed = false;
    int barData[16] = {0};
    uint32_t barBG = 0;
    int lineData[16] = {0};
    int gradient[2] = {0};
    int barSize;
    int lineSize;
    long lastUpdate;
    uint64_t lifetime;
    std::vector<uint32_t> colors;
    std::vector<String> fragments;
    int textOffset;
    int iconOffset;
    int progress = -1;
    uint32_t pColor;
    uint32_t background = 0;
    uint32_t pbColor;
    float scrollSpeed = 100;
    bool topText = true;
    bool noScrolling = true;
    bool lifeTimeEnd = false;
    std::vector<uint8_t> jpegDataBuffer;
};

extern std::vector<std::pair<String, AppCallback>> Apps;
extern String currentCustomApp;
extern std::map<String, CustomApp> customApps;
extern void (*customAppCallbacks[MAX_CUSTOM_APPS])(FastLED_NeoMatrix *, MatrixDisplayUiState *, int16_t, int16_t, GifPlayer *);

CustomApp *getCustomAppByName(String name);

String getAppNameByFunction(AppCallback AppFunction);

String getAppNameAtIndex(int index);

int findAppIndexByName(const String &name);

const char *getTimeFormat();

void TimeApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void DateApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void TempApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void HumApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

#ifndef awtrix2_upgrade
void BatApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
#endif

void ShowCustomApp(String name, FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

#define DECLARE_CAPP(N) \
    void CApp##N(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

DECLARE_CAPP(1)  DECLARE_CAPP(2)  DECLARE_CAPP(3)  DECLARE_CAPP(4)  DECLARE_CAPP(5)
DECLARE_CAPP(6)  DECLARE_CAPP(7)  DECLARE_CAPP(8)  DECLARE_CAPP(9)  DECLARE_CAPP(10)
DECLARE_CAPP(11) DECLARE_CAPP(12) DECLARE_CAPP(13) DECLARE_CAPP(14) DECLARE_CAPP(15)
DECLARE_CAPP(16) DECLARE_CAPP(17) DECLARE_CAPP(18) DECLARE_CAPP(19) DECLARE_CAPP(20)

#endif