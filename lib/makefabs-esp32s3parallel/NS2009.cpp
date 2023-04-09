#include "NS2009.h"

//I2C receive
void ns2009_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                 size_t receive_buf_len)
{
    Wire.beginTransmission(NS2009_ADDR);
    Wire.write(send_buf, send_buf_len);
    Wire.endTransmission();
    Wire.requestFrom(NS2009_ADDR, receive_buf_len);
    while (Wire.available())
    {
        *receive_buf++ = Wire.read();
    }
}

//read 12bit data
unsigned int ns2009_read(uint8_t cmd)
{
    uint8_t buf[2];
    ns2009_recv(&cmd, 1, buf, 2);
    return (buf[0] << 4) | (buf[1] >> 4);
}

int ns2009_get_press()
{
    return ns2009_read(NS2009_LOW_POWER_READ_Z1);
}

int ns2009_pos(int pos[2])
{
    int press = ns2009_read(NS2009_LOW_POWER_READ_Z1);
    if (press > 30)
    {
        int x, y, z = 0;

        x = ns2009_read(NS2009_LOW_POWER_READ_X);
        y = ns2009_read(NS2009_LOW_POWER_READ_Y);

        pos[0] = x * SCREEN_X_PIXEL / 4096; //4096 = 2 ^ 12
        pos[1] = y * SCREEN_Y_PIXEL / 4096;
        return 1;
    }
    else
    {
        pos[0] = -1;
        pos[1] = -1;
        return 0;
    }
}