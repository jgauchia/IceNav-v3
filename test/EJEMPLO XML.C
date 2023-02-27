Por supuesto, aquí te lo presento de nuevo:

    Lectura secuencial del archivo OSM.XML usando la librería Tinyxml2:

C++

#include <TinyXML2.h>

void readOSMInArea(const char* filename, float minLat, float maxLat, float minLon, float maxLon) {
  // Create an instance of the XML document
  tinyxml2::XMLDocument doc;
  // Load the XML file into the document
  doc.LoadFile(filename);

  // Get the root element of the document
  tinyxml2::XMLElement* root = doc.RootElement();

  // Iterate over all child elements of the root element
  for (tinyxml2::XMLElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()) {
    // Check if the element is a way
    if (strcmp(elem->Name(), "way") == 0) {
      // Get the way ID attribute
      const char* wayId = elem->Attribute("id");
      // Iterate over all child elements of the way element
      for (tinyxml2::XMLElement* nd = elem->FirstChildElement(); nd != NULL; nd = nd->NextSiblingElement()) {
        // Check if the element is a node reference
        if (strcmp(nd->Name(), "nd") == 0) {
          // Get the node ID attribute
          const char* nodeId = nd->Attribute("ref");
          // Get the node element with the matching ID
          tinyxml2::XMLElement* node = root->FirstChildElement("node");
          while (node != NULL && strcmp(node->Attribute("id"), nodeId) != 0) {
            node = node->NextSiblingElement("node");
          }
          // Check if the node element was found
          if (node != NULL) {
            // Get the latitude and longitude attributes of the node
            float lat = node->FloatAttribute("lat");
            float lon = node->FloatAttribute("lon");
            // Check if the node is within the specified area
            if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
              // Draw the road on the TFT display
              drawRoad(lat, lon);
            }
          }
        }
      }
    }
  }
}

    Función para dibujar las carreteras en la TFT display:

C++

#include <TFT_eSPI.h>

TFT_eSPI tft;

void drawRoad(float lat, float lon) {
  // Convert latitude and longitude to TFT screen coordinates
  int x = lonToX(lon);
  int y = latToY(lat);
  // Draw a road segment at the TFT screen coordinates
  tft.drawLine(x - 1, y - 1, x + 1, y + 1, TFT_WHITE);
}

    Funciones para convertir la longitud y la latitud a coordenadas X e Y de la pantalla TFT:

C++

float lonToX(float lon) {
  return (lon - MIN_LON) / (MAX_LON - MIN_LON) * TFT_WIDTH;
}

float latToY(float lat) {
  return (MAX_LAT - lat) / (MAX_LAT - MIN_LAT) * TFT_HEIGHT;
}

Donde MIN_LAT, MAX_LAT, MIN_LON, MAX_LON, TFT_WIDTH y TFT_HEIGHT son constantes que definen la zona geográfica y las dimensiones de la pantalla TFT.

***************************************************************************************************************************************************************
Por supuesto, aquí te dejo la función drawRoadsInArea revisada:

cpp

void drawRoadsInArea(float lat, float lon, float latDelta, float lonDelta) {
  Serial.print("Searching for roads within ");
  Serial.print(lat);
  Serial.print(", ");
  Serial.print(lon);
  Serial.print(", ");
  Serial.print(latDelta);
  Serial.print(", ");
  Serial.println(lonDelta);

  // Open the OSM file
  Serial.print("Opening file ");
  Serial.println(OSM_FILE_NAME);
  File file = SPIFFS.open(OSM_FILE_NAME);

  if (!file) {
    Serial.println("Failed to open file");
    return;
  }

  // Setup TFT display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);

  // Setup parser
  tinyxml2::XMLDocument doc;
  doc.Parse(file.readString().c_str());
  tinyxml2::XMLElement* root = doc.FirstChildElement("osm");
  if (!root) {
    Serial.println("Failed to parse OSM file");
    return;
  }

  // Loop through all ways in the file
  tinyxml2::XMLElement* way = root->FirstChildElement("way");
  while (way) {
    // Check if this way is a road
    if (isRoad(way)) {
      // Loop through all nodes in the way
      tinyxml2::XMLElement* nd = way->FirstChildElement("nd");
      while (nd) {
        // Get the node's latitude and longitude
        float ndLat = getNodeLat(nd->Attribute("ref"), root);
        float ndLon = getNodeLon(nd->Attribute("ref"), root);

        // Check if the node is within the desired area
        if (ndLat >= lat && ndLat <= lat + latDelta &&
            ndLon >= lon && ndLon <= lon + lonDelta) {
          // Draw the road segment
          drawRoadSegment(way, TFT_WHITE);
          break;
        }

        nd = nd->NextSiblingElement("nd");
      }
    }

    way = way->NextSiblingElement("way");
  }

  file.close();
}

La función ahora recibe como segundo parámetro un color (TFT_WHITE en este caso) que se utiliza para dibujar la carretera correspondiente. De esta manera, puedes llamar a la función múltiples veces, cada una con un color diferente, para que las carreteras se dibujen en colores distintos.
