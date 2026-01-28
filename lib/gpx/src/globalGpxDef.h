/**
 * @file globalGpxDef.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Global GPX Variables
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <stdint.h>
#include <vector>
#include "PsramAllocator.hpp"

static const char* wptFile = "/sdcard/WPT/waypoint.gpx"; /**< Path to the waypoint GPX file on the SD card. */
static const char* wptFolder = "/sdcard/WPT";            /**< Path to the waypoint folder on the SD card. */
static const char* trkFolder = "/sdcard/TRK";            /**< Path to the track folder on the SD card. */

/**
 * @brief Waypoint Structure
 *
 * @details Stores information related to a GPS waypoint.
 */
struct wayPoint
{
    float     lat;     /**< Latitude of the waypoint. */
    float     lon;     /**< Longitude of the waypoint. */
    float     ele;     /**< Elevation of the waypoint. */
    char*     time;    /**< Timestamp of the waypoint (ISO 8601). */
    char*     name;    /**< Name of the waypoint. */
    char*     desc;    /**< Description of the waypoint. */
    char*     src;     /**< Source of the waypoint data. */
    char*     sym;     /**< Symbol associated with the waypoint. */
    char*     type;    /**< Type/category of the waypoint. */
    uint8_t   sat;     /**< Number of satellites used for this fix. */
    float     hdop;    /**< Horizontal dilution of precision. */
    float     vdop;    /**< Vertical dilution of precision. */
    float     pdop;    /**< Position dilution of precision. */
    float     accumDist; /**< Accumulated distance from start (meters). */
};

/**
 * @brief Track Segment for Spatial Indexing
 *
 * @details Represents a segment of the track with its bounding box.
 *          Used for hierarchical search (O(log n)) instead of linear search.
 */
struct TrackSegment
{
    int startIdx;
    int endIdx;
    float minLat, maxLat;
    float minLon, maxLon;
};

/**
 * @brief Track Vector Type using PSRAM Allocator
 */
typedef std::vector<wayPoint, PsramAllocator<wayPoint>> TrackVector;

/**
 * @brief Waypoint action enum
 *
 * @details Enumeration of possible actions for GPX waypoints.
 */
enum gpxAction_t
{
    WPT_NONE,   /**< No waypoint action. */
    WPT_ADD,    /**< Add a new waypoint. */
    GPX_LOAD,   /**< Load waypoints from GPX file. */
    GPX_EDIT,   /**< Edit an existing waypoint. */
    GPX_DEL,    /**< Delete a waypoint. */
};

extern uint8_t gpxAction; /**< Indicates the current GPX waypoint action to be performed. */

/**
 * @brief Track turn points structure
 *
 * @details Structure representing a track turn point
 */
struct TurnPoint 
{
    int idx;           /**< Index of the track point */
    float angle;       /**< Turn angle at this point (positive = right, negative = left) */
    float distance;    /**< Distance from start to this point (in meters) */
}; 

/**
 * @Brief GPX header file format
 *
 * @details Static string containing the standard GPX 1.0 file header, to be used when creating new GPX files.
 */
static const char* gpxHeader = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<gpx\n"
                      " version=\"1.0\"\n"
                      " creator=\"IceNav\"\n"
                      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                      " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
                      "</gpx>" };