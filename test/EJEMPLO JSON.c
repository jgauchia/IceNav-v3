C++

#include <ArduinoJson.h>
#include <FS.h>
#include <TFT_eSPI.h>

// Define las coordenadas de la zona que deseas leer
const float LAT_MIN = 40.0;
const float LAT_MAX = 41.0;
const float LON_MIN = -3.0;
const float LON_MAX = -2.0;

// Lee un archivo GeoJSON y dibuja los caminos y carreteras dentro de una zona definida por unas coordenadas en una pantalla TFT_ESPI
void dibujarGeoJsonEnZona(String archivo, float latMin, float latMax, float lonMin, float lonMax, TFT_eSPI& tft) {
  // Conecta el ESP32 al sistema de archivos
  if (!SPIFFS.begin()) {
    Serial.println("No se pudo montar el sistema de archivos");
    return;
  }

  // Abre el archivo GeoJSON
  File geoJsonFile = SPIFFS.open(archivo, "r");
  if (!geoJsonFile) {
    Serial.println("No se pudo abrir el archivo GeoJSON");
    return;
  }

  // Crea un objeto DynamicJsonDocument para almacenar los datos GeoJSON
  DynamicJsonDocument geoJsonDoc(2048); // Tamaño de la memoria en bytes que se asignará

  // Lee el archivo GeoJSON y almacena los datos en el objeto geoJsonDoc
  DeserializationError error = deserializeJson(geoJsonDoc, geoJsonFile);
  if (error) {
    Serial.println("No se pudo leer el archivo GeoJSON");
    return;
  }

  // Cierra el archivo GeoJSON
  geoJsonFile.close();

  // Itera sobre las características (features) del archivo GeoJSON y dibuja los caminos y carreteras que están dentro de la zona definida
  JsonArray features = geoJsonDoc["features"].as<JsonArray>();
  for (JsonVariant feature : features) {
    JsonObject geometry = feature["geometry"];
    if (geometry["type"] == "LineString") {
      JsonArray coordinates = geometry["coordinates"];
      bool dentroDeZona = false;
      for (JsonVariant coordinate : coordinates) {
        float lon = coordinate[0];
        float lat = coordinate[1];
        if (lon > lonMin && lon < lonMax && lat > latMin && lat < latMax) {
          dentroDeZona = true;
        } else {
          dentroDeZona = false;
          break;
        }
      }
      if (dentroDeZona) {
        uint16_t color;
        if (feature["properties"]["highway"]) {
          color = TFT_BLUE;
        } else {
          color = TFT_DARKGREY;
        }
        tft.startWrite();
        tft.drawPolyLine(coordinates, color);
        tft.endWrite();
      }
    }
  }
}

Esta función recibe como parámetros el nombre del archivo GeoJSON, así como las coordenadas que delimitan la zona que deseas leer y la pantalla TFT_ESPI en la que se dibujarán los caminos y carreteras. En lugar de devolver un objeto JsonArray, la función itera sobre las características (features) del archivo que se encuentran dentro de la zona definida y dibuja los caminos y carreteras correspondientes en la pantalla