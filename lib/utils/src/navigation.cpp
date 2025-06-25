/**
 * @file navigation.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.3
 * @date 2025-06
 */

 #include "navigation.hpp"

 /**
 * @brief Find the closest track point index to the user's current position.
 *
 * This function searches for the closest waypoint in the track to the user's given latitude and longitude.
 * To optimize performance, it only searches within a window of points around the last known closest index (lastIdx).
 * If a closer point is found within this window, its index is returned.
 * Additionally, to avoid small unwanted backward jumps (e.g., due to GPS noise), 
 * the function does not allow small regressions: if the closest point found is behind lastIdx 
 * but less than 5 points back, it will keep lastIdx as the result.
 *
 * @param userLat Current latitude of the user.
 * @param userLon Current longitude of the user.
 * @param track The vector of wayPoints representing the full track.
 * @param lastIdx The index of the last known closest point on the track.
 * @return The index of the closest point in the track within the search window.
 */
int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx) 
{
    int window = 10;
    int start = std::max(0, lastIdx - window);
    int end = std::min((int)track.size() - 1, lastIdx + window);

    int closestIdx = lastIdx;
    float minDist = calcDist(userLat, userLon, track[lastIdx].lat, track[lastIdx].lon);
    for (int i = start; i <= end; ++i) {
        float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
        if (d < minDist) {
            minDist = d;
            closestIdx = i;
        }
    }
    // Prevent small backward jumps on the track
    if (closestIdx < lastIdx && (lastIdx - closestIdx) < 5) {
        closestIdx = lastIdx;
    }
    return closestIdx;
}

/**
 * @brief Turn-by-turn navigation logic and user notification handler.
 *
 * This function updates the state of navigation and determines what turn instructions or alerts should be shown to the user.
 * It handles the detection of track abandonment, the identification of the next relevant turn, and the management of different navigation warnings (straight, pre-turn, and turn).
 * The logic ensures that each type of notification is only shown once per relevant segment, and resets notification flags as the user progresses.
 * 
 * Behavior:
 * - Shows an "off route" warning if the user strays too far from the nearest track point.
 * - Notifies the user to "continue straight" when no turn is imminent.
 * - Issues a pre-turn warning at a configurable distance before a turn, indicating the direction (left/right) and sharpness (slight, moderate, sharp).
 * - Issues a final turn warning at a shorter distance before the actual turn.
 * - Handles resetting of warnings as the navigation context changes.
 *
 *
 * @param userLat Current user latitude.
 * @param userLon Current user longitude.
 * @param userHeading Current heading of the user (degrees).
 * @param speed_kmh Current user speed in km/h.
 * @param track Vector of wayPoints representing the GPX track.
 * @param turns Vector of TurnPoint, each representing a detected turn on the track.
 * @param state Navigation state struct, updated persistently across function calls.
 */
void updateNavigation(float userLat, float userLon, float userHeading, float speed_kmh,
                      const std::vector<wayPoint>& track, const std::vector<TurnPoint>& turns,
                      NavState& state)
{
    float umbralSeguirRecto = 200.0; 
    float preWarnDist = speed_kmh > 11.0 ? 150.0 : 80.0;
    float warnDist = 50.0; 

    int closestIdx = findClosestTrackPoint(userLat, userLon, track, state.lastTrackIdx);

    float distToTrack = calcDist(userLat, userLon, track[closestIdx].lat, track[closestIdx].lon);
    if (distToTrack > 30.0) 
    {
        // Pseudocódigo: mostrar bitmap de "fuera de ruta"
        // mostrarBitmap(LVGL_BITMAP_FUERA_RUTA);
        log_i("¡Fuera de ruta! Reincorpórate al track");
        state.warnedStraight = false;
        state.warnedTurn = false;
        state.warnedPreTurn = false;
        return;
    }

    // Buscar siguiente giro
    while (state.nextTurnIdx < (int)turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;

    if (state.nextTurnIdx < (int)turns.size()) 
    {
        float turnLat = track[turns[state.nextTurnIdx].idx].lat;
        float turnLon = track[turns[state.nextTurnIdx].idx].lon;
        float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);

        float abs_angle = fabs(turns[state.nextTurnIdx].angle);
        bool derecha = (turns[state.nextTurnIdx].angle > 0);

        // --- NUEVO: Aviso de seguir recto ---
        if (distanceToTurn > umbralSeguirRecto && !state.warnedStraight) 
        {
            // Pseudocódigo: mostrar imagen de "seguir recto"
            // mostrarBitmap(LVGL_BITMAP_SEGUIR_RECTO);
            log_i("Sigue recto");
            state.warnedStraight = true;
            state.warnedPreTurn = false;
            state.warnedTurn = false;
            return;
        }

        // --- Pre-aviso de giro ---
        if (distanceToTurn < preWarnDist && !state.warnedPreTurn)
        {
            // Selección de tipo de giro
            if (abs_angle < 60) 
            {
                // Leve
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_LEVE_DERECHA);
                    log_i("Preaviso: giro leve a la DERECHA en %d m", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_LEVE_IZQUIERDA);
                    log_i("Preaviso: giro leve a la IZQUIERDA en %d m", (int)distanceToTurn);
            }
            else if (abs_angle < 120)
            {
                // Medio
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_MEDIA_DERECHA);
                    log_i("Preaviso: giro medio a la DERECHA en %d m", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_MEDIA_IZQUIERDA);
                    log_i("Preaviso: giro medio a la IZQUIERDA en %d m", (int)distanceToTurn);
            }
            else
            {
                // Cerrado
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_CERRADA_DERECHA);
                    log_i("Preaviso: giro cerrado a la DERECHA en %d m", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_PREAVISO_FLECHA_CERRADA_IZQUIERDA);
                    log_i("Preaviso: giro cerrado a la IZQUIERDA en %d m", (int)distanceToTurn);
            }
            state.warnedPreTurn = true;
            state.warnedStraight = false;
        }
        // --- Aviso definitivo de giro ---
        if (distanceToTurn < warnDist && !state.warnedTurn)
        {
            // Selección de tipo de giro
            if (abs_angle < 60) 
            {
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_GIRO_LEVE_DERECHA);
                    log_i("¡Giro leve a la DERECHA en %d m!", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_GIRO_LEVE_IZQUIERDA);
                    log_i("¡Giro leve a la IZQUIERDA en %d m!", (int)distanceToTurn);
            } 
            else if (abs_angle < 120)
            {
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_GIRO_MEDIO_DERECHA);
                    log_i("¡Giro medio a la DERECHA en %d m!", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_GIRO_MEDIO_IZQUIERDA);
                    log_i("¡Giro medio a la IZQUIERDA en %d m!", (int)distanceToTurn);
            } 
            else
            {
                if (derecha)
                    // mostrarBitmap(LVGL_BITMAP_GIRO_CERRADO_DERECHA);
                    log_i("¡Giro cerrado a la DERECHA en %d m!", (int)distanceToTurn);
                else
                    // mostrarBitmap(LVGL_BITMAP_GIRO_CERRADO_IZQUIERDA);
                    log_i("¡Giro cerrado a la IZQUIERDA en %d m!", (int)distanceToTurn);
            }
            state.warnedTurn = true;
            state.warnedStraight = false;
        }
        // Reset de avisos si nos alejamos
        if (distanceToTurn > preWarnDist) 
        {
            state.warnedTurn = false;
            state.warnedPreTurn = false;
        }
    }
    else 
    {
        // Pseudocódigo: mostrar bitmap de "fin de ruta"
        // mostrarBitmap(LVGL_BITMAP_FIN_RUTA);
        log_i("Fin de ruta o sin más giros.");
        state.warnedStraight = false;
        state.warnedTurn = false;
        state.warnedPreTurn = false;
    }
    state.lastTrackIdx = closestIdx;
}