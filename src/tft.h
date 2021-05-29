#include <Adafruit_ILI9341.h>
extern Adafruit_ILI9341 tft;

void tft_init(void);
void drawMenu(void);
void drawScreen(void);
void turnTFTOn(void);

extern int screen;
const int SCREEN_MAIN              = 1;
const int SCREEN_GRAPH             = 2;
const int SCREEN_SETTINGS          = 3;
const int SCREEN_SETTINGS_CONFIRM1 = 4;
const int SCREEN_SETTINGS_CONFIRM2 = 5;
const int SCREEN_SETTINGS_CONFIRM3 = 6;

extern int tftMainRect[4];
extern int tftGraphRect[4];
extern int tftSettingsRect[4];
extern int tftOnOffRect[4];

extern int tftWiFiOnOffRect[4];
extern int tftCalibrateCO2[4];
extern int tftDisplayAutoOn[4];

extern int tftConfirm1Yes[4];
extern int tftConfirm1No[4];
extern int tftConfirm2Yes[4];
extern int tftConfirm2No[4];
extern int tftConfirm3Yes[4];
extern int tftConfirm3No[4];

int16_t tft_getWidth(void);
int16_t tft_getHeight(void);
void tft_fillScreen(void);
uint16_t readPixA(int x, int y); // get pixel color code in rgb565 format
void printText(int areaX, int areaY, int areaWidth, const char *str, uint8_t textSize, const GFXfont *f, uint16_t fontColor, uint16_t backgroundColor, bool xCentered);

extern bool autoTurnOnTFT;

// http://www.barth-dev.de/online/rgb565-color-picker/  
#define TFT_LIGHTGREEN_WIFI  0x5E0A
#define TFT_DARKGREEN_WIFI   0x3BEA
#define TFT_DARKGREY_MENU    0xA534 // darkblue 0x1BBD;
#define TFT_LIGTHGREY_MENU   0xE71C // lightblue 0x753D;
#define TFT_LIGTHYELLOW_BATT 0xEFCE
#define TFT_OCEANBLUE_TEMP   0x3C7E
#define TFT_LIGHTORANGE_CO2  0xF467
#define TFT_DARKORANGE_CO2   0x62C2
#define TFT_RED_ABORT        0xF8C3
#define TFT_RED_AUTODISPLAY  0xFCF3
#define TFT_GREEN_XAXIS      0x0320
#define TFT_YELLOW_XAXIS     0x4220
#define TFT_RED_XAXIS        0x4000
#define TFT_INCIDENCE_XAXIS  0x3186

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F      /* 255, 192, 203 */
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */
