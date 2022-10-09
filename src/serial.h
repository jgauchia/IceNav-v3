/**
 * @file serial.h
 * @author Jordi Gauchía
 * @brief Serial output functions (debug)
 * @version 0.1
 * @date 2022-10-09
 */

HardwareSerial *debug = &Serial;

/**
 * @brief Serial port init (debug)
 *
 */
void init_serial()
{
#ifdef DEBUG
    debug->begin(BAUDRATE);
#endif
}
