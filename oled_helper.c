/*****************************************************************************
 *   oled_helper.c: Helper functions for OLED
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <string.h>
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "font5x7.h"
#include "oled.h"
#include "oled_helper.h"

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
 *    Initialize OLED for the Monitor mode readings
 *
 *****************************************************************************/
void initMonitorOled(void) {
	oled_putString(28, 0, "MONITOR", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 12, "Light:", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(65, 12, "Lux", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 26, "Temp :", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_circle(67, 27, 2, OLED_COLOR_WHITE);
	oled_putString(70, 27, "C", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "x :", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 47, "y :", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 55, "z :", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}
