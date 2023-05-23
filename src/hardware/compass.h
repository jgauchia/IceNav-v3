/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass definition and functions
 * @version 0.1.4
 * @date 2023-05-23
 */


#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(12345);

/**
 * @brief Magnetic declination
 * 
 */
float declinationAngle = 0.22;


/**
 * @brief Compass Heading Angle
 * 
 */
uint16_t heading = 0;


/**
 * @brief Read compass data
 *
 * @return compass heading
 */
int read_compass()
{
    sensors_event_t event;
    compass.getEvent(&event);
    float heading = atan2(event.magnetic.y, event.magnetic.x);
    heading += declinationAngle;
    if (heading < 0)
        heading += 2 * PI;
    if (heading > 2 * PI)
        heading -= 2 * PI;
    return (uint16_t)(heading * 180 / M_PI);
}

