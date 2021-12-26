/*
       @file       0_Vars.h
       @brief      Declaración de variables y elementos usados en el programa

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Definición pines GPS y GPS
// **********************************************
#define GPS_TX  16
#define GPS_RX  17
HardwareSerial *gps = &Serial2;
#define GPS_UPDATE_TIME  1500
#define MAX_SATELLITES   40

TinyGPSPlus GPS;
TinyGPSCustom totalGPGSVMessages(GPS, "GPGSV", 1); // $GPGSV sentence, first element
TinyGPSCustom messageNumber(GPS, "GPGSV", 2);      // $GPGSV sentence, second element
TinyGPSCustom satsInView(GPS, "GPGSV", 3);         // $GPGSV sentence, third element
TinyGPSCustom satNumber[4];                        // to be initialized later
TinyGPSCustom elevation[4];
TinyGPSCustom azimuth[4];
TinyGPSCustom snr[4];

// **********************************************
//  Definición pines microSD
// **********************************************
#define SD_CS   5
#define SD_MISO 27
#define SD_MOSI 13
#define SD_CLK  14

// **********************************************
//  Declaración para el puerto serie de Debug
// **********************************************
HardwareSerial *debug = &Serial;

// **********************************************
//  Declaración para el TFT ILI9341
// **********************************************
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sat_sprite = TFT_eSprite(&tft);

// **********************************************
//  Declaración para la microSD
// **********************************************
SPIClass spiSD(HSPI);

// **********************************************
//  Declaración para el teclado
// **********************************************
PCF8574 keyboard(0x38);

// **********************************************
//  Declaración para la brújula
// **********************************************
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// **********************************************
//  Declaración de variables
// **********************************************
unsigned long millis_actual = 0;    // Para almacenar tiempo para delay con millis()
bool is_gps_fixed = false;          // indica si la señal GPS está fijada
bool is_draw = false;               // Controlar el "pintado" en pantalla.
#define TIME_OFFSET  1              // Zona horaria

struct                              // Estructura para mostrar el tracking de satélites
{
  bool active;
  int elevation;
  int azimuth;
  int snr;
  int pos_x;
  int pos_y;
} sat_tracker[MAX_SATELLITES];

TaskHandle_t Task1;                 // Tareas para los cores del ESP32
TaskHandle_t Task2;

#define SNR_BAR_W  25
#define SNR_BAR_H  80
//uint16_t* snr_bkg = (uint16_t*) malloc( ((SNR_BAR_W) + 2) * ((SNR_BAR_H) + 2) * 2 );
uint16_t snr_bkg[4428] = {0};

enum Keys { NONE, UP, DOWN, LEFT, RIGHT, PUSH, BLEFT, BRIGHT };
int key_pressed = NONE;

bool is_menu_screen = false;
char s_buf[64];                   // Buffer para sprintf

float f_rumbo = 0;                // Variables para la brújula
float f_rumbo_temp = 0;
float declinationAngle = 0.2200;

float batt = 0;                     // Variables % batería
float batt_old = 0;
#define Icon_Notify_Width  24
#define Icon_Notify_Height 24

// **********************************************
//  Declaración funciones
// **********************************************

void init_tasks();
void Read_GPS( void * pvParameters );
void Main_prog( void * pvParameters );
void init_serial();
void init_gps();
void gps_out_monitor();
void init_ili9341();
void init_sd();
void init_icenav();
uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);
void drawBmp(const char *filename, int16_t x, int16_t y, bool microsd);
void read_NMEA(unsigned long ms);
float Read_Battery();
void show_battery(int x, int y);
void search_init_sat();
void show_sat_icon(int x, int y);
void show_sat_hour(int x, int y, int font);
void show_sat_tracking();
void show_sat_track_screen();
double RADtoDEG(double rad);
double DEGtoRAD(double deg);
void Latitude_formatString(int x, int y, int font,  double lat);
void Longitude_formatString(int x, int y, int font,  double lon);
