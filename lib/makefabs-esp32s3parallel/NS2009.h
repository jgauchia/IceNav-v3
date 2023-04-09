#include <Wire.h>

//NS2009
#define NS2009_ADDR 0x48 //10010000

#define NS2009_LOW_POWER_READ_X 0xc0
#define NS2009_LOW_POWER_READ_Y 0xd0
#define NS2009_LOW_POWER_READ_Z1 0xe0

#define SCREEN_X_PIXEL 320
#define SCREEN_Y_PIXEL 480

void ns2009_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                size_t receive_buf_len);

unsigned ns2009_read(uint8_t cmd, int *val);

int ns2009_pos(int pos[2]);

int ns2009_get_press();