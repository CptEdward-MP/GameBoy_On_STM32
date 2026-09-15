#include "gb_input.h"

/*
 * STM32 HAL is isolated to this implementation file.
 * Other modules only depend on gb_input.h.
 */
#include "main.h"

/*
 * Physical button mapping.
 *
 * Wiring:
 *
 *     GPIO ---- BUTTON ---- GND
 *
 * GPIO configured as:
 *     Input + Pull-up
 *
 * Therefore:
 *     HIGH = released
 *     LOW  = pressed
 */

#define GB_BTN_UP_PORT       GPIOA
#define GB_BTN_UP_PIN        GPIO_PIN_0

#define GB_BTN_DOWN_PORT     GPIOA
#define GB_BTN_DOWN_PIN      GPIO_PIN_1

#define GB_BTN_LEFT_PORT     GPIOA
#define GB_BTN_LEFT_PIN      GPIO_PIN_2

#define GB_BTN_RIGHT_PORT    GPIOA
#define GB_BTN_RIGHT_PIN     GPIO_PIN_3

#define GB_BTN_A_PORT        GPIOA
#define GB_BTN_A_PIN         GPIO_PIN_4

#define GB_BTN_B_PORT        GPIOA
#define GB_BTN_B_PIN         GPIO_PIN_5

#define GB_BTN_SELECT_PORT   GPIOA
#define GB_BTN_SELECT_PIN    GPIO_PIN_6

#define GB_BTN_START_PORT    GPIOA
#define GB_BTN_START_PIN     GPIO_PIN_7


/* Debounce time in milliseconds */
#define GB_INPUT_DEBOUNCE_MS 20U


/*
 * Current stable button state.
 *
 * A bit value of:
 *     1 = pressed
 *     0 = released
 */
static uint8_t input_state = 0;


/*
 * Last raw state read from GPIO.
 */
static uint8_t last_raw_state = 0;


/*
 * Time at which the raw state last changed.
 */
static uint32_t last_change_time = 0;


/*
 * Read one physical button.
 *
 * Pull-up input:
 *
 *     RESET -> pressed
 *     SET   -> released
 */
static uint8_t GB_Input_ReadButton(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1U : 0U;
}


/*
 * Read all eight physical buttons.
 */
static uint8_t GB_Input_ReadRawState(void)
{
    uint8_t state = 0;

    if (GB_Input_ReadButton(GB_BTN_UP_PORT, GB_BTN_UP_PIN))
        state |= (1U << GB_INPUT_UP);

    if (GB_Input_ReadButton(GB_BTN_DOWN_PORT, GB_BTN_DOWN_PIN))
        state |= (1U << GB_INPUT_DOWN);

    if (GB_Input_ReadButton(GB_BTN_LEFT_PORT, GB_BTN_LEFT_PIN))
        state |= (1U << GB_INPUT_LEFT);

    if (GB_Input_ReadButton(GB_BTN_RIGHT_PORT, GB_BTN_RIGHT_PIN))
        state |= (1U << GB_INPUT_RIGHT);

    if (GB_Input_ReadButton(GB_BTN_A_PORT, GB_BTN_A_PIN))
        state |= (1U << GB_INPUT_A);

    if (GB_Input_ReadButton(GB_BTN_B_PORT, GB_BTN_B_PIN))
        state |= (1U << GB_INPUT_B);

    if (GB_Input_ReadButton(GB_BTN_SELECT_PORT, GB_BTN_SELECT_PIN))
        state |= (1U << GB_INPUT_SELECT);

    if (GB_Input_ReadButton(GB_BTN_START_PORT, GB_BTN_START_PIN))
        state |= (1U << GB_INPUT_START);

    return state;
}


/*
 * Initialize input system.
 */
void GB_Input_Init(void)
{
    input_state = 0;

    last_raw_state = GB_Input_ReadRawState();

    last_change_time = HAL_GetTick();
}


/*
 * Update button state with debounce.
 *
 * The raw GPIO state must remain unchanged for
 * GB_INPUT_DEBOUNCE_MS before becoming the new
 * stable input state.
 */
void GB_Input_Update(void)
{
    uint8_t raw_state = GB_Input_ReadRawState();

    /*
     * Raw state changed.
     *
     * Start/restart debounce timer.
     */
    if (raw_state != last_raw_state)
    {
        last_raw_state = raw_state;
        last_change_time = HAL_GetTick();

        return;
    }

    /*
     * Raw state has remained unchanged.
     *
     * Check whether debounce period has expired.
     */
    if ((HAL_GetTick() - last_change_time) >= GB_INPUT_DEBOUNCE_MS)
    {
        input_state = raw_state;
    }
}


/*
 * Check whether a button is currently pressed.
 */
uint8_t GB_Input_IsPressed(GB_InputButton button)
{
    if (button >= GB_INPUT_COUNT)
        return 0U;

    return (input_state >> button) & 0x01U;
}


/*
 * Get the complete stable button state.
 */
uint8_t GB_Input_GetState(void)
{
    return input_state;
}
