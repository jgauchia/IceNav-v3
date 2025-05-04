/**
 * @file compass.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass definition and functions
 * @version 0.2.0
 * @date 2024-12
 */

#ifndef COMPASS_HPP
#define COMPASS_HPP

#include "tft.hpp"
#include <EasyPreferences.hpp>

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
#define ENABLE_IMU;
#endif

#define COMPASS_CAL_TIME 16000
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


