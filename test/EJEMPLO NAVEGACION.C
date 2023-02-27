int get_navigation_direction(double lat_actual, double lon_actual, double lat_dest, double lon_dest, float heading) {
  const float max_diff_heading = 30.0f; // Máxima diferencia de ángulo para considerar adelante/atras
  const float max_bearing_diff = 45.0f; // Máxima diferencia de bearing para considerar izquierda/derecha
  
  float distance_m = TinyGPSPlus::distanceBetween(lat_actual, lon_actual, lat_dest, lon_dest) * 1000.0f;
  float bearing = TinyGPSPlus::courseTo(lat_actual, lon_actual, lat_dest, lon_dest);

  // Corrección del bearing si se encuentra en el cuadrante 3 o 4
  if (bearing < 0) {
    bearing += 360.0f;
  }

  // Cálculo de la diferencia de bearing
  float bearing_diff = fabs(heading - bearing);
  if (bearing_diff > 180.0f) {
    bearing_diff = 360.0f - bearing_diff;
  }

  // Cálculo de la dirección de navegación
  if (distance_m <= 10.0f) {
    return 0; // Dentro del rango de llegada
  } else if (bearing_diff <= max_diff_heading) {
    return 1; // Adelante
  } else if (bearing_diff >= 180.0f - max_diff_heading) {
    return 2; // Atrás
  } else if (bearing_diff <= max_bearing_diff) {
    return 3; // Derecha
  } else if (bearing_diff >= 180.0f - max_bearing_diff) {
    return 4; // Izquierda
  } else if (heading >= bearing) {
    return 3; // Derecha
  } else {
    return 4; // Izquierda
  }
}