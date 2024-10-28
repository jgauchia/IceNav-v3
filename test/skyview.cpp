#include <TFT_eSPI.h>  // Librería TFT_eSPI

TFT_eSPI tft = TFT_eSPI();  // Crear el objeto para la pantalla TFT

// Estructura para las coordenadas del satélite
struct SatPos {
  int x;
  int y;
};

// Dimensiones de la pantalla TFT y centro
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;
const int CENTER_X = SCREEN_WIDTH / 2;
const int CENTER_Y = SCREEN_HEIGHT / 2;
const int MAX_RADIUS = 100;  // Radio máximo en función de la elevación máxima (ajustable)

// Función para calcular la posición en pantalla de un satélite dado su elevación y azimut
SatPos getSatPos(uint8_t elev, uint16_t azim) {
  SatPos pos;

  // Calcular el radio en función de la elevación: mayor elevación = más cerca del centro
  int H = MAX_RADIUS * (90 - elev) / 90;  // Escalar elevación a un rango de 0 a MAX_RADIUS

  // Convertir azimut y calcular coordenadas
  pos.x = CENTER_X + H * sin(DEG2RAD(azim));
  pos.y = CENTER_Y - H * cos(DEG2RAD(azim));  // Se invierte Y para adaptarse al sistema de coordenadas de TFT

  return pos;
}

void setup() {
  tft.init();
  tft.setRotation(1);   // Ajusta la rotación de la pantalla si es necesario
  tft.fillScreen(TFT_BLACK);  // Limpiar la pantalla

  // Dibuja círculos concéntricos para representar elevaciones (opcional)
  tft.drawCircle(CENTER_X, CENTER_Y, MAX_RADIUS, TFT_WHITE);        // Horizonte (0°)
  tft.drawCircle(CENTER_X, CENTER_Y, MAX_RADIUS * 2 / 3, TFT_WHITE); // 30° de elevación
  tft.drawCircle(CENTER_X, CENTER_Y, MAX_RADIUS / 3, TFT_WHITE);     // 60° de elevación
  tft.drawPixel(CENTER_X, CENTER_Y, TFT_WHITE);                     // Cénit (90°)

  // Ejemplo de uso de getSatPos con varios satélites
  uint8_t elevaciones[] = {10, 20, 30, 40, 50, 60, 70, 80};  // Ejemplo de elevaciones
  uint16_t azimuts[] = {0, 45, 90, 135, 180, 225, 270, 315}; // Ejemplo de azimuts
  int numSatellites = sizeof(elevaciones) / sizeof(elevaciones[0]);

  for (int i = 0; i < numSatellites; i++) {
    SatPos pos = getSatPos(elevaciones[i], azimuts[i]);
    tft.fillCircle(pos.x, pos.y, 5, TFT_GREEN);  // Dibujar satélite como un círculo verde
    tft.setTextColor(TFT_WHITE);
    tft.drawNumber(i + 1, pos.x + 6, pos.y - 6);  // Etiqueta opcional con el número del satélite
  }
}

void loop() {
  // Aquí puedes actualizar las posiciones de los satélites si tienes datos en tiempo real
}




//*******************************************************************************

#include <lvgl.h>

// Tamaño del canvas y centro
const int CANVAS_SIZE = 240; // Tamaño del canvas (asumimos que el display es cuadrado)
const int CENTER_X = CANVAS_SIZE / 2;
const int CENTER_Y = CANVAS_SIZE / 2;
const int MAX_RADIUS = 100; // Radio máximo para representar el horizonte (0° elevación)

// Estructura para la posición de un satélite
struct SatPos {
    int x;
    int y;
};

// Función para calcular la posición en pantalla de un satélite dado su elevación y azimut
SatPos getSatPos(uint8_t elev, uint16_t azim) {
    SatPos pos;
    int H = MAX_RADIUS * (90 - elev) / 90;  // Escalar elevación en un rango de 0 a MAX_RADIUS
    pos.x = CENTER_X + H * sin(DEG2RAD(azim));
    pos.y = CENTER_Y - H * cos(DEG2RAD(azim)); // Invertir Y para el sistema de coordenadas de LVGL
    return pos;
}

void setup() {
    lv_init();

    // Inicializar el canvas
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_SIZE, CANVAS_SIZE)];
    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, LV_COLOR_BLACK, LV_OPA_COVER); // Fondo negro

    // Dibujar círculos de elevación en el canvas
    lv_color_t circle_color = LV_COLOR_WHITE;
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS, circle_color);        // Horizonte (0° elevación)
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS * 2 / 3, circle_color); // 30° elevación
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS / 3, circle_color);     // 60° elevación
    lv_canvas_draw_pixel(canvas, CENTER_X, CENTER_Y, circle_color);                     // Cénit (90° elevación)

    // Datos de ejemplo: azimut y elevación de algunos satélites
    uint8_t elevaciones[] = {10, 20, 30, 40, 50, 60, 70, 80}; // Elevación en grados
    uint16_t azimuts[] = {0, 45, 90, 135, 180, 225, 270, 315}; // Azimut en grados
    int numSatellites = sizeof(elevaciones) / sizeof(elevaciones[0]);

    // Dibujar satélites en el canvas
    lv_color_t sat_color = LV_COLOR_GREEN;
    for (int i = 0; i < numSatellites; i++) {
        SatPos pos = getSatPos(elevaciones[i], azimuts[i]);
        lv_canvas_draw_circle(canvas, pos.x, pos.y, 3, sat_color); // Dibuja el satélite como un pequeño círculo
    }

    // Agregar etiquetas N, S, E, W para las direcciones
    lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    // Etiqueta Norte (N)
    lv_obj_t *label_n = lv_label_create(canvas);
    lv_obj_add_style(label_n, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_n, "N");
    lv_obj_align(label_n, NULL, LV_ALIGN_CENTER, 0, -MAX_RADIUS - 10); // Posición hacia el norte

    // Etiqueta Sur (S)
    lv_obj_t *label_s = lv_label_create(canvas);
    lv_obj_add_style(label_s, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_s, "S");
    lv_obj_align(label_s, NULL, LV_ALIGN_CENTER, 0, MAX_RADIUS + 10); // Posición hacia el sur

    // Etiqueta Este (E)
    lv_obj_t *label_e = lv_label_create(canvas);
    lv_obj_add_style(label_e, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_e, "E");
    lv_obj_align(label_e, NULL, LV_ALIGN_CENTER, MAX_RADIUS + 10, 0); // Posición hacia el este

    // Etiqueta Oeste (W)
    lv_obj_t *label_w = lv_label_create(canvas);
    lv_obj_add_style(label_w, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_w, "W");
    lv_obj_align(label_w, NULL, LV_ALIGN_CENTER, -MAX_RADIUS - 10, 0); // Posición hacia el oeste

    // Opcional: Agregar etiquetas a los satélites
    for (int i = 0; i < numSatellites; i++) {
        SatPos pos = getSatPos(elevaciones[i], azimuts[i]);
        lv_obj_t *label = lv_label_create(canvas);
        lv_obj_add_style(label, LV_LABEL_PART_MAIN, &style_label);
        lv_label_set_text_fmt(label, "%d", i + 1);  // Etiqueta con el número de satélite
        lv_obj_align(label, NULL, LV_ALIGN_CENTER, pos.x - CENTER_X, pos.y - CENTER_Y); // Posicionar etiqueta
    }
}

void loop() {
    lv_task_handler(); // Necesario para que LVGL maneje sus tareas
    delay(5); // Pequeño retraso
}


/************************************************************************* */

#include <lvgl.h>

// Tamaño del canvas y centro
const int CANVAS_SIZE = 240; // Tamaño del canvas (asumimos que el display es cuadrado)
const int CENTER_X = CANVAS_SIZE / 2;
const int CENTER_Y = CANVAS_SIZE / 2;
const int MAX_RADIUS = 100; // Radio máximo para representar el horizonte (0° elevación)

// Estructura para la posición de un satélite
struct SatPos {
    int x;
    int y;
};

// Datos de ejemplo (podrías reemplazar estos arreglos por datos en tiempo real)
uint8_t elevaciones[] = {10, 20, 30, 40, 50, 60, 70, 80}; // Elevación en grados
uint16_t azimuts[] = {0, 45, 90, 135, 180, 225, 270, 315}; // Azimut en grados
int numSatellites = sizeof(elevaciones) / sizeof(elevaciones[0]);

// Canvas de LVGL y su búfer
lv_obj_t *canvas;
static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_SIZE, CANVAS_SIZE)];

// Función para calcular la posición en pantalla de un satélite dado su elevación y azimut
SatPos getSatPos(uint8_t elev, uint16_t azim) {
    SatPos pos;
    int H = MAX_RADIUS * (90 - elev) / 90;  // Escalar elevación en un rango de 0 a MAX_RADIUS
    pos.x = CENTER_X + H * sin(DEG2RAD(azim));
    pos.y = CENTER_Y - H * cos(DEG2RAD(azim)); // Invertir Y para el sistema de coordenadas de LVGL
    return pos;
}

// Función para dibujar el gráfico completo en el canvas
void draw_skyview() {
    // Limpiar el canvas (rellenar con color negro)
    lv_canvas_fill_bg(canvas, LV_COLOR_BLACK, LV_OPA_COVER);

    // Dibujar círculos de elevación en el canvas
    lv_color_t circle_color = LV_COLOR_WHITE;
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS, circle_color);        // Horizonte (0° elevación)
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS * 2 / 3, circle_color); // 30° elevación
    lv_canvas_draw_circle(canvas, CENTER_X, CENTER_Y, MAX_RADIUS / 3, circle_color);     // 60° elevación
    lv_canvas_draw_pixel(canvas, CENTER_X, CENTER_Y, circle_color);                      // Cénit (90° elevación)

    // Dibujar las etiquetas de dirección N, S, E, W
    lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    // Norte (N)
    lv_obj_t *label_n = lv_label_create(canvas);
    lv_obj_add_style(label_n, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_n, "N");
    lv_obj_align(label_n, NULL, LV_ALIGN_CENTER, 0, -MAX_RADIUS - 10);

    // Sur (S)
    lv_obj_t *label_s = lv_label_create(canvas);
    lv_obj_add_style(label_s, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_s, "S");
    lv_obj_align(label_s, NULL, LV_ALIGN_CENTER, 0, MAX_RADIUS + 10);

    // Este (E)
    lv_obj_t *label_e = lv_label_create(canvas);
    lv_obj_add_style(label_e, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_e, "E");
    lv_obj_align(label_e, NULL, LV_ALIGN_CENTER, MAX_RADIUS + 10, 0);

    // Oeste (W)
    lv_obj_t *label_w = lv_label_create(canvas);
    lv_obj_add_style(label_w, LV_LABEL_PART_MAIN, &style_label);
    lv_label_set_text(label_w, "W");
    lv_obj_align(label_w, NULL, LV_ALIGN_CENTER, -MAX_RADIUS - 10, 0);

    // Dibujar los satélites en el canvas en su posición actual
    lv_color_t sat_color = LV_COLOR_GREEN;
    for (int i = 0; i < numSatellites; i++) {
        SatPos pos = getSatPos(elevaciones[i], azimuts[i]);
        lv_canvas_draw_circle(canvas, pos.x, pos.y, 3, sat_color); // Dibuja el satélite como un pequeño círculo
    }
}

void setup() {
    lv_init();

    // Crear e inicializar el canvas
    canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(canvas, 0, 0); // Colocar el canvas en la esquina superior izquierda
}

void loop() {
    // Actualizar las posiciones de los satélites (puedes usar datos en tiempo real aquí)
    // Por ejemplo, podrías actualizar `elevaciones` y `azimuts` con datos nuevos

    // Llamar a la función que redibuja el skyview
    draw_skyview();

    // Deja que LVGL maneje sus tareas
    lv_task_handler();
    delay(500); // Actualizar cada 500 ms
}
