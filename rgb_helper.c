/*****************************************************************************
 *   rgb_helper.c:  Helper for the RGB LED
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "lpc17xx_gpio.h"
#include "rgb.h"
#include "rgb_helper.h"

/******************************************************************************
 * Defines and typedefs
 *****************************************************************************/

/******************************************************************************
 * External global variables
 *****************************************************************************/

/******************************************************************************
 * Local variables
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/******************************************************************************
 * Public Functions
 *****************************************************************************/

/******************************************************************************
 *
 * Description:
 *    Turn off Red LED
 *
 *****************************************************************************/
void offRedLed(void) {
	GPIO_ClearValue(2, 1);
}

/******************************************************************************
 *
 * Description:
 *    Turn on Red LED
 *
 *****************************************************************************/
void onRedLed(void) {
	GPIO_SetValue(2, 1);
}

/******************************************************************************
 *
 * Description:
 *    Turn off Blue LED
 *
 *****************************************************************************/
void offBlueLed(void) {
	GPIO_ClearValue(0, (1 << 26));
}

/******************************************************************************
 *
 * Description:
 *    Turn on Blue LED
 *
 *****************************************************************************/
void onBlueLed(void) {
	GPIO_SetValue(0, (1 << 26));
}

