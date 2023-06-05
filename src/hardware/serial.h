/**
 * @file serial.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Serial output functions (debug)
 * @version 0.1.5
 * @date 2023-06-04
 */

HardwareSerial *debug = &Serial;

/**
 * @brief Serial port init (debug)
 *
 */
void init_serial()
{
    debug->begin(BAUDRATE);
}
