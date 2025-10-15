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
static const char* logFolder PROGMEM = "/sdcard/LOG";            /**< Path to the log folder on the SD card. */

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
};

struct trkPoint
{
	float lat;       /**< Latitude of the track point. */
	float lon;       /**< Longitude of the track point. */
	float ele;       /**< Elevation of the track point. */
	float temp;	  /**< Temperature at the track point. */
};

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
static const char* gpxHeader PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<gpx\n"
                      " version=\"1.0\"\n"
                      " creator=\"IceNav\"\n"
                      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                      " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
                      "</gpx>" };

static const char* gpxTrkHeader PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
					  "<gpx\n"
					  " version=\"1.0\"\n"
					  " creator=\"IceNav\"\n"
					  " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
					  " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
					  " xmlns=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"\n"
					  " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
					  "<trk>\n"
					  "<name>IceNav Track</name>\n"
					  "<type>cycling</type>\n"
					  "<trkseg>\n"
					  "</trkseg>\n"
					  "</trk>\n"
					  "</gpx>" };