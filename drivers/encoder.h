#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define ENCODER_EVENT_NONE              0x00u
#define ENCODER_EVENT_CW                0x01u
#define ENCODER_EVENT_CCW               0x02u
#define ENCODER_EVENT_BUTTON            0x04u

void encoder_init(void);
uint8_t encoder_take_events(void);

#endif

