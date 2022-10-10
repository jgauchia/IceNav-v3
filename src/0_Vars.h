// **********************************************
//  Declaración de variables
// **********************************************
unsigned long millis_actual = 0; // Para almacenar tiempo para delay con millis()
bool is_gps_fixed = false;       // indica si la señal GPS está fijada
bool is_draw = false;            // Controlar el "pintado" en pantalla.

TaskHandle_t Task1; // Tareas para los cores del ESP32
TaskHandle_t Task2;

#define SNR_BAR_W 25
#define SNR_BAR_H 80
// uint16_t* snr_bkg = (uint16_t*) malloc( ((SNR_BAR_W) + 2) * ((SNR_BAR_H) + 2) * 2 );
uint16_t snr_bkg[4428] = {0};

bool is_menu_screen = false;
bool is_main_screen = false;
bool is_map_screen = false;
bool is_sat_screen = false;
bool is_compass_screen = false;
bool is_show_degree = true;
char s_buf[64]; // Buffer para sprintf

int rumbo = 0; // Variables para la brújula

#define Icon_Notify_Width 24
#define Icon_Notify_Height 24

int tilex = 0; // Tile para archivo mapa
int tiley = 0;
char s_fichmap[40];
int x = 0;
int y = 0;
int zoom = 16; // Zoom por defecto del mapa
int zoom_old = 0;
#define MIN_ZOOM 6
#define MAX_ZOOM 18


int batt_level = 0;

// **********************************************
//  Declaración funciones
// **********************************************
void init_tasks();
void Read_GPS(void *pvParameters);
void Main_prog(void *pvParameters);
void setPngPosition(int16_t x, int16_t y);
void show_battery(int x, int y);
void show_sat_icon(int x, int y);
void show_sat_hour(int x, int y, int font);
void show_sat_tracking();
void show_sat_track_screen();
void show_main_screen();
void show_sat_track_screen();
void show_map_screen();

//******************************************************

/**
 * @brief Vectors to a screen functions
 * 
 */
#define MAX_MAIN_SCREEN 3
typedef void (*MainScreenFunc)();
MainScreenFunc MainScreen[] = {0, show_main_screen, show_map_screen, show_sat_track_screen};
int sel_MainScreen = 1;