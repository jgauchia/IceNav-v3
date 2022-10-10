// **********************************************
//  Declaración de variables
// **********************************************
unsigned long millis_actual = 0; // Para almacenar tiempo para delay con millis()
// uint16_t* snr_bkg = (uint16_t*) malloc( ((SNR_BAR_W) + 2) * ((SNR_BAR_H) + 2) * 2 );
uint16_t snr_bkg[4428] = {0};
int tilex = 0; // Tile para archivo mapa
int tiley = 0;
char s_fichmap[40];
int x = 0;
int y = 0;

// **********************************************
//  Declaración funciones
// **********************************************
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


