/**
 * @file globalGpxDef.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Global GPX Variables
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <pgmspace.h>
#include <stdint.h>

static const char* wptFile PROGMEM = "/sdcard/WPT/waypoint.gpx"; /**< Path to the waypoint GPX file on the SD card. */
static const char* wptFolder PROGMEM = "/sdcard/WPT";            /**< Path to the waypoint folder on the SD card. */
static const char* trkFolder PROGMEM = "/sdcard/TRK";            /**< Path to the track folder on the SD card. */

/**
 * @brief Waypoint action enum
 *
 * Enumeration of possible actions for GPX waypoints.
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
 * @brief Waypoint Structure
 *
 * Stores information related to a GPS waypoint.
 */
struct wayPoint
{
	double    lat;     /**< Latitude of the waypoint. */
	double    lon;     /**< Longitude of the waypoint. */
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
};

/**
 * @brief Track turn points structure
 *
 * Structure representing a track turn point
 */
struct TurnPoint 
{
	int idx;            /**< Index of the track point */
	double angle;       /**< Turn angle at this point (positive = right, negative = left) */
	double distance;    /**< Distance from start to this point (in meters) */
}; 

/**
 * @Brief GPX header file format
 *
 * Static string containing the standard GPX 1.0 file header, to be used when creating new GPX files.
 */
static const char* gpxHeader PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<gpx\n"
                      " version=\"1.0\"\n"
                      " creator=\"IceNav\"\n"
                      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                      " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
                      "</gpx>" };