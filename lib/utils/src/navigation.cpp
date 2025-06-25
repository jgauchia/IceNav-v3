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

 /**
 * @brief Finds the closest track point index to the user's current position, with an adaptive search window.
 *
 * This function searches for the closest waypoint in the track to the user's given latitude and longitude.
 * To optimize performance, it initially searches within a window of points around the last known closest index (lastIdx).
 * If the user is detected to be far from the last known point (e.g., due to a fast jump, simulation, or GPS error),
 * the function automatically expands the search to cover the entire track, ensuring robust operation even with rapid movements.
 * Additionally, to avoid small unwanted backward jumps (e.g., due to GPS noise), small regressions are ignored:
 * if the closest point found is behind lastIdx but less than 5 points back, it keeps lastIdx as the result.
 *
 * @param userLat   Current latitude of the user.
 * @param userLon   Current longitude of the user.
 * @param track     The vector of wayPoints representing the full track.
 * @param lastIdx   The index of the last known closest point on the track.
 * @return          The index of the closest point in the track within the search window or full track if needed.
 */


int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx) 
{
    // int window = 20;
    // int start = std::max(0, lastIdx - window);
    // int end = std::min((int)track.size() - 1, lastIdx + window);

    // int closestIdx = lastIdx;
    // float minDist = calcDist(userLat, userLon, track[lastIdx].lat, track[lastIdx].lon);
    // for (int i = start; i <= end; ++i) {
    //     float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
    //     if (d < minDist) {
    //         minDist = d;
    //         closestIdx = i;
    //     }
    // }
    // // Prevent small backward jumps on the track
    // if (closestIdx < lastIdx && (lastIdx - closestIdx) < 5) {
    //     closestIdx = lastIdx;
    // }
    // return closestIdx;

    int window = 20;
    int n = (int)track.size();
    int start = std::max(0, lastIdx - window);
    int end = std::min(n - 1, lastIdx + window);

    // Si el usuario se ha alejado mucho, busca en todo el track
    float minDist = calcDist(userLat, userLon, track[lastIdx].lat, track[lastIdx].lon);
    int closestIdx = lastIdx;

    // Si la distancia al último punto es grande, busca en todo el track
    if (minDist > 2 * window) { // o un umbral en metros, ej: 50m
        start = 0;
        end = n - 1;
    }

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
 * @brief Enhanced turn-by-turn navigation logic with "soft curve" detection for S-shaped and gentle bends.
 *
 * This function updates the navigation state and determines which instructions or alerts should be shown to the user.
 * - It supports off-track detection.
 * - The distance to the next turn is updated and shown dynamically ("live") on every cycle.
 * - Introduces a "soft curve" warning for angles between 15 and 60 degrees, so S-shaped and gentle curves are not interpreted as straight.
 * - The warning flags (warnedPreTurn, warnedTurn) are kept for possible unique actions (sound, vibration), but visual messages are refreshed every cycle.
 *
 * @param userLat         Current user latitude.
 * @param userLon         Current user longitude.
 * @param userHeading     Current heading of the user (degrees).
 * @param speed_kmh       Current user speed in km/h.
 * @param track           Vector of wayPoints representing the GPX track.
 * @param turns           Vector of TurnPoint, each representing a detected turn on the track.
 * @param state           Navigation state struct, updated persistently across function calls.
 */
void updateNavigation(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const std::vector<wayPoint>& track,
    const std::vector<TurnPoint>& turns,
    NavState& state
) {
    float lookahead = 1000.0; // meters: how far ahead to consider upcoming turns
    float umbralSeguirRecto = 200.0; // meters to show "go straight" or "soft curve"
    float preWarnDist = speed_kmh > 11.0 ? 150.0 : 80.0; // meters for pre-turn warning
    float warnDist = 50.0; // meters for final turn warning
    float minAngleForCurve = 15.0; // degrees: minimum angle to consider as "curva suave"

    int closestIdx = findClosestTrackPoint(userLat, userLon, track, state.lastTrackIdx);

    float distToTrack = calcDist(userLat, userLon, track[closestIdx].lat, track[closestIdx].lon);
    if (distToTrack > 30.0) 
    {
        // Pseudocode: show "off track" bitmap
        // mostrarBitmap(LVGL_BITMAP_FUERA_RUTA);
        log_i("¡Fuera de ruta! Reincorpórate al track");
        state.warnedStraight = false;
        state.warnedTurn = false;
        state.warnedPreTurn = false;
        return;
    }

    // --- Find the most relevant next turn within lookahead ---
    int relevantTurnIdx = -1;
    float distanceToRelevantTurn = std::numeric_limits<float>::max();
    float abs_angle = 0.0;
    bool derecha = false;

    // Scan all upcoming turns
    for (int i = state.nextTurnIdx; i < (int)turns.size(); ++i) {
        float turnLat = track[turns[i].idx].lat;
        float turnLon = track[turns[i].idx].lon;
        float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);

        if (distanceToTurn < distanceToRelevantTurn) {
            distanceToRelevantTurn = distanceToTurn;
            relevantTurnIdx = i;
            abs_angle = fabs(turns[i].angle);
            derecha = (turns[i].angle > 0);
        }
        // stop search if the next turn is outside the lookahead window
        if (distanceToTurn > lookahead) break;
    }

    // If no relevant turn in lookahead: show recto to the next turn (even if it is far)
    if (relevantTurnIdx == -1) {
        if (!turns.empty()) {
            // Still show "seguir recto" to the very next turn, even if > lookahead
            float turnLat = track[turns[state.nextTurnIdx].idx].lat;
            float turnLon = track[turns[state.nextTurnIdx].idx].lon;
            float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);
            log_i("Sigue recto durante %d m", (int)distanceToTurn);
        } else {
            log_i("Fin de ruta o sin más giros.");
        }
        state.warnedStraight = false;
        state.warnedTurn = false;
        state.warnedPreTurn = false;
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Update state.nextTurnIdx if we've passed a turn
    while (state.nextTurnIdx < (int)turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;

    // --- Nueva lógica de avisos para el giro relevante ---
    if (distanceToRelevantTurn > umbralSeguirRecto) 
    {
        if (abs_angle > minAngleForCurve && abs_angle < 60) {
            // Soft curve
            if (derecha)
                log_i("Curva suave a la DERECHA en %d m", (int)distanceToRelevantTurn);
            else
                log_i("Curva suave a la IZQUIERDA en %d m", (int)distanceToRelevantTurn);
        } else {
            // Go straight
            log_i("Sigue recto durante %d m", (int)distanceToRelevantTurn);
        }
        state.warnedStraight = true;
        state.warnedPreTurn = false;
        state.warnedTurn = false;
        state.lastTrackIdx = closestIdx;
        return;
    }

    // --- Pre-turn warning: dynamically update distance ---
    if (distanceToRelevantTurn < preWarnDist && distanceToRelevantTurn > warnDist) {
        if (abs_angle < 60 && abs_angle >= minAngleForCurve) {
            if (derecha)
                log_i("Preaviso: curva suave a la DERECHA en %d m", (int)distanceToRelevantTurn);
            else
                log_i("Preaviso: curva suave a la IZQUIERDA en %d m", (int)distanceToRelevantTurn);
        } else if (abs_angle < 60) {
            if (derecha)
                log_i("Preaviso: giro leve a la DERECHA en %d m", (int)distanceToRelevantTurn);
            else
                log_i("Preaviso: giro leve a la IZQUIERDA en %d m", (int)distanceToRelevantTurn);
        }
        else if (abs_angle < 120) {
            if (derecha)
                log_i("Preaviso: giro medio a la DERECHA en %d m", (int)distanceToRelevantTurn);
            else
                log_i("Preaviso: giro medio a la IZQUIERDA en %d m", (int)distanceToRelevantTurn);
        }
        else {
            if (derecha)
                log_i("Preaviso: giro cerrado a la DERECHA en %d m", (int)distanceToRelevantTurn);
            else
                log_i("Preaviso: giro cerrado a la IZQUIERDA en %d m", (int)distanceToRelevantTurn);
        }
        state.warnedPreTurn = true;
        state.warnedStraight = false;
    }
    // --- Final turn warning: dynamically update distance ---
    if (distanceToRelevantTurn <= warnDist) {
        if (abs_angle < 60 && abs_angle >= minAngleForCurve) {
            if (derecha)
                log_i("¡Curva suave a la DERECHA en %d m!", (int)distanceToRelevantTurn);
            else
                log_i("¡Curva suave a la IZQUIERDA en %d m!", (int)distanceToRelevantTurn);
        } else if (abs_angle < 60) {
            if (derecha)
                log_i("¡Giro leve a la DERECHA en %d m!", (int)distanceToRelevantTurn);
            else
                log_i("¡Giro leve a la IZQUIERDA en %d m!", (int)distanceToRelevantTurn);
        } 
        else if (abs_angle < 120) {
            if (derecha)
                log_i("¡Giro medio a la DERECHA en %d m!", (int)distanceToRelevantTurn);
            else
                log_i("¡Giro medio a la IZQUIERDA en %d m!", (int)distanceToRelevantTurn);
        } 
        else {
            if (derecha)
                log_i("¡Giro cerrado a la DERECHA en %d m!", (int)distanceToRelevantTurn);
            else
                log_i("¡Giro cerrado a la IZQUIERDA en %d m!", (int)distanceToRelevantTurn);
        }
        state.warnedTurn = true;
        state.warnedStraight = false;
    }
    // Reset warnings if moving away from turn
    if (distanceToRelevantTurn > preWarnDist) 
    {
        state.warnedTurn = false;
        state.warnedPreTurn = false;
    }
    state.lastTrackIdx = closestIdx;
}