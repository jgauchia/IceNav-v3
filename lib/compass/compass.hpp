/**
 * @file compass.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Compass definition and functions
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef COMPASS_HPP
#define COMPASS_HPP

#include "tft.hpp"

extern int mapHeading;

#ifdef HMC5883L
#include <DFRobot_QMC5883.h>
extern DFRobot_QMC5883 compass;
#define ENABLE_COMPASS
#endif

#ifdef QMC5883
#include <DFRobot_QMC5883.h>
extern DFRobot_QMC5883 compass;
#define ENABLE_COMPASS
#endif

#ifdef IMU_MPU9250
#include <MPU9250.h>
extern MPU9250 IMU;
#define ENABLE_COMPASS
#endif

#define COMPASS_CAL_TIME 16000
void saveCompassCal(float offsetX, float offsetY);
extern float declinationAngle;
extern int heading;

extern float offX;
extern float offY; 
static float headingSmooth = 0.0;
static float headingPrevious = 0.0;
static float minX, maxX, minY, maxY = 0.0;

void initCompass();
void readCompass(float &x, float &y, float &z);
int getHeading();
void compassCalibrate();

#endif


