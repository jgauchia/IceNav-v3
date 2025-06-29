/**
 * @file navigation.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "navigation.hpp"

extern lv_obj_t *turnDistLabel; 
extern lv_obj_t *turnImg;

LV_IMG_DECLARE(straight);
LV_IMG_DECLARE(slleft);
LV_IMG_DECLARE(slright);
LV_IMG_DECLARE(tleft);
LV_IMG_DECLARE(tright);
LV_IMG_DECLARE(uleft);
LV_IMG_DECLARE(uright);
LV_IMG_DECLARE(finish);

 /**
 * @brief Finds the closest track point index to the user's current position, with an adaptive search window.
 *
 * @details Searches for the closest waypoint in the track to the user's given latitude and longitude.
 *          To optimize performance, it initially searches within a window of points around the last known closest index (lastIdx).
 *          If the user is detected to be far from the last known point (e.g., due to a fast jump, simulation, or GPS error),
 *          the function automatically expands the search to cover the entire track, ensuring robust operation even with rapid movements.
 *          Additionally, to avoid small unwanted backward jumps (e.g., due to GPS noise), small regressions are ignored:
 *          if the closest point found is behind lastIdx but less than 5 points back, it keeps lastIdx as the result.
 *
 * @param userLat   Current latitude of the user.
 * @param userLon   Current longitude of the user.
 * @param track     The vector of wayPoints representing the full track.
 * @param lastIdx   The index of the last known closest point on the track.
 * @return          The index of the closest point in the track within the search window or full track if needed.
 */

int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx) 
{
    int window = 50;
    int n = (int)track.size();
    int start = std::max(0, lastIdx - window);
    int end = std::min(n - 1, lastIdx + window);

    // Si el usuario se ha alejado mucho, busca en todo el track
    float minDist = calcDist(userLat, userLon, track[lastIdx].lat, track[lastIdx].lon);
    int closestIdx = lastIdx;

    // Si la distancia al último punto es grande, busca en todo el track
    if (minDist > 2 * window)       
    { // o un umbral en metros, ej: 50m
        start = 0;
        end = n - 1;
    }

    for (int i = start; i <= end; ++i)
    {
        float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
        if (d < minDist)
        {
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
 * @brief Navegación simplificada: muestra solo el siguiente evento (recto, curva suave o fuerte) y únicamente lo avisa cuando quedan warnDist metros o menos.
 *
 * @param userLat             Latitud del usuario.
 * @param userLon             Longitud del usuario.
 * @param userHeading         Rumbo del usuario (grados).
 * @param speed_kmh           Velocidad del usuario (km/h).
 * @param track               Vector de wayPoints del track GPX.
 * @param turns               Vector de TurnPoint (giros detectados).
 * @param state               Estado de navegación.
 * @param minAngleForCurve    Umbral angular mínimo para considerar curva relevante (por defecto 15°).
 * @param warnDist            Distancia (en metros) para mostrar el aviso del evento (por defecto 100).
 */
void updateNavigation(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const std::vector<wayPoint>& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist
)
{
    const float minTurnDist = 5.0f;  // Umbral para saltar turnpoints ya pasados o justo encima

    int closestIdx = findClosestTrackPoint(userLat, userLon, track, state.lastTrackIdx);
    float distToTrack = calcDist(userLat, userLon, track[closestIdx].lat, track[closestIdx].lon);
    if (distToTrack > 30.0) 
    {
        log_i("¡Fuera de ruta! Reincorpórate al track");
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Avanza el índice de turnpoint si ya lo hemos pasado (por índice)
    while (state.nextTurnIdx < (int)turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;

    // Busca el siguiente evento relevante (el primero con distancia suficiente)
    int nextEventIdx = -1;
    float distanceToNextEvent = std::numeric_limits<float>::max();
    float abs_angle = 0.0;
    bool derecha = false;

    for (int i = state.nextTurnIdx; i < (int)turns.size(); ++i)
    {
        float turnLat = track[turns[i].idx].lat;
        float turnLon = track[turns[i].idx].lon;
        float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);
        // Si el evento está “demasiado cerca” (o ya pasado), sáltalo y sigue buscando
        if (distanceToTurn < minTurnDist)
            continue;

        distanceToNextEvent = distanceToTurn;
        nextEventIdx = i;
        abs_angle = fabs(turns[i].angle);
        derecha = (turns[i].angle > 0);
        break; // Encontrado el siguiente relevante, sal del bucle
    }

    // Si no queda ningún evento relevante
    if (nextEventIdx == -1) 
    {
        lv_img_set_src(turnImg, &finish);
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Mostrar solo cuando falten <= warnDist metros
    if (distanceToNextEvent <= warnDist)
    {
        if (abs_angle >= minAngleForCurve && abs_angle < 60) 
        {
            if (derecha)
                lv_img_set_src(turnImg, &slright);
            else
                lv_img_set_src(turnImg, &slleft);
        }
        else if (abs_angle >= 60) 
        {
            if (derecha)
                lv_img_set_src(turnImg, &tright);
            else
                lv_img_set_src(turnImg, &tleft);

        }
        // else if (abs_angle > 3.0) 
        // {
        //     log_i("Desvío leve a la %s en %d m (%.1f° en idx %d)",
        //         derecha ? "DERECHA" : "IZQUIERDA",
        //         (int)distanceToNextEvent,
        //         abs_angle,
        //         turns[nextEventIdx].idx
        //     );
        // }
        else 
        {
            lv_img_set_src(turnImg, &straight);     
        }
    }
    else
    {
        lv_img_set_src(turnImg, &straight);
    }

    lv_label_set_text_fmt(turnDistLabel, "%4d", (int)distanceToNextEvent);

    state.lastTrackIdx = closestIdx;
}