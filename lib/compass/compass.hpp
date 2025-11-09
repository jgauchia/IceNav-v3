/**
 * @file compass.hpp
 * @brief Compass definition and functions
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include "tft.hpp"
#include <EasyPreferences.hpp>

#ifdef HMC5883L
	#include <DFRobot_QMC5883.h>
	#define ENABLE_COMPASS
#endif

#ifdef QMC5883
	#include <DFRobot_QMC5883.h>
	#define ENABLE_COMPASS
#endif

#ifdef IMU_MPU9250
	#include <MPU9250.h>
	#define ENABLE_COMPASS
	#define ENABLE_IMU ;
#endif

#define COMPASS_CAL_TIME 16000 /**< Compass calibration duration in milliseconds. */

/**
 * @class KalmanFilter
 * @brief Implements a simple 1D Kalman filter for angle estimation.
 */
class KalmanFilter
{
public:
	KalmanFilter(float processNoise, float measurementNoise, float estimateError, float initialEstimate)
		: q(processNoise), r(measurementNoise), p(estimateError), x(initialEstimate) {}

	/**
	* @brief Updates the state estimate using the Kalman filter algorithm for angular measurements.
	*
	* @details Applies the Kalman filter update step, taking into account the wrapped angular measurement,
	* 		   and updates the internal state and covariance variables accordingly.
	*
	* @param measurement The new angle measurement to incorporate (in radians).
	* @return float The updated state estimate (in radians, wrapped to [-π, π]).
	*/
	float update(float measurement)
	{
		measurement = wrapToPi(measurement);
		float y = measurement - x;
		y = wrapToPi(y);

		p = p + q;
		k = p / (p + r);
		x = x + k * y;

		x = wrapToPi(x);

		p = (1 - k) * p;
		return x;
	}

	/**
	* @brief Sets the process and measurement noise constants for the Kalman filter.
	*
	* @details Updates the internal parameters for process noise covariance (q) and measurement noise covariance (r).
	*
	* @param processNoise Value for the process noise covariance.
	* @param measurementNoise Value for the measurement noise covariance.
	*/
	void setConstants(float processNoise, float measurementNoise)
	{
		q = processNoise;
		r = measurementNoise;
	}

private:
	float q; /**< Process noise covariance (Q), representing the process variance. */
	float r; /**< Measurement noise covariance (R), representing the sensor variance. */
	float p; /**< Estimate error covariance (P), representing the estimated error. */
	float k; /**< Kalman gain (K), used to update the state estimate. */
	float x; /**< Value (X), the current state estimate. */

	float wrapToPi(float angle)
	{
		while (angle < -M_PI)
			angle += 2 * M_PI;
		while (angle > M_PI)
			angle -= 2 * M_PI;
		return angle;
	}
};

/**
 * @class Compass
 * @brief Provides high-level interface for compass (magnetometer) sensor management and heading calculation.
 */
class Compass
{
public:
	Compass();
	void init();
	void read(float &x, float &y, float &z);
	int getHeading();
	bool isUpdated();
	void calibrate();
	void setDeclinationAngle(float angle);
	void setOffsets(float offsetX, float offsetY);
	void enableKalmanFilter(bool enabled);
	void setKalmanFilterConst(float processNoise, float measureNoise);

private:
	float declinationAngle;       /**< Magnetic declination angle (in radians or degrees, depending on use). */
	float offX;                   /**< Magnetometer offset for X axis. */
	float offY;                   /**< Magnetometer offset for Y axis. */
	float headingSmooth;          /**< Smoothed heading value. */
	float headingPrevious;        /**< Previous heading value (for smoothing or change detection). */
	float minX;                   /**< Minimum observed value for X axis (for calibration). */
	float maxX;                   /**< Maximum observed value for X axis (for calibration). */
	float minY;                   /**< Minimum observed value for Y axis (for calibration). */
	float maxY;                   /**< Maximum observed value for Y axis (for calibration). */
	bool kalmanFilterEnabled;     /**< True if the Kalman filter is enabled for heading smoothing. */
	KalmanFilter kalmanFilter;    /**< Kalman filter instance used for heading estimation. */
	int previousDegrees;          /**< Previous heading in degrees (integer value). */

	float wrapToPi(float angle);
	float unwrapFromPi(float angle, float previousAngle);
};