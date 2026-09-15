#ifndef GB_INPUT_H
#define GB_INPUT_H

#include <stdint.h>

/* HAL-independent Game Boy button interface. */

typedef enum
{
    GB_INPUT_UP = 0,
    GB_INPUT_DOWN,
    GB_INPUT_LEFT,
    GB_INPUT_RIGHT,
    GB_INPUT_A,
    GB_INPUT_B,
    GB_INPUT_SELECT,
    GB_INPUT_START,
    GB_INPUT_COUNT
} GB_InputButton;

void GB_Input_Init(void);
void GB_Input_Update(void);
uint8_t GB_Input_IsPressed(GB_InputButton button);
uint8_t GB_Input_GetState(void);

#endif /* GB_INPUT_H */
