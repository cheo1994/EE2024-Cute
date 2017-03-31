/***************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 **************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "light.h"
#include "temp.h"

volatile uint32_t msTicks = 0;
volatile uint32_t swTicks;
volatile int monitorFlag = 0;
uint32_t lightReading = 1;
int32_t temperatureReading;
uint8_t lightLowWarning;
uint8_t fireAlert = 0;
uint8_t moveInDarkAlert = 0;
int8_t xReading;
int8_t yReading;
int8_t zReading;
//centre of OLED
uint8_t xoled = 48;
uint8_t yoled = 32;
uint8_t* light_buffer[15];
uint8_t* temp_buffer[15];
uint8_t* x_buffer[15];
uint8_t* y_buffer[15];
uint8_t* z_buffer[15];
int32_t xoff = 0;
int32_t yoff = 0;
int32_t zoff = 0;

// This function is called every 1us
void SysTick_Handler(void) {
	msTicks++;
}

// This function updates the temperature reading, light reading and accelerometer readings
void updateAccSensor() {
	acc_read(&xReading, &yReading, &zReading);
	xReading = xReading + xoff;
	yReading = yReading + yoff;
	zReading = zReading + zoff;
}

void updateTempSensor() {
	temperatureReading = temp_read();
}

void updateLightSensor() {
	lightReading = light_read();
}

static void updateSensors() {
	updateLightSensor();
	updateTempSensor();
	updateAccSensor();
}

void temperatureToString(char *str) {
	strcat(str, "Temp: ");
	char tempBuffer[5] = "";
	sprintf(tempBuffer, "%.1f", temperatureReading / 10.0);
	strcat(str, tempBuffer);
	strcat(str, " C");
}

void temperatureToString2(char *str) {
	char tempBuffer[5] = "";
	sprintf(tempBuffer, "%.1f", temperatureReading / 10.0);
	strcat(str, tempBuffer);
	strcat(str, " ");
}

void lightToString(char *str) {
	strcat(str, "L: ");
	char lightBuffer[9] = "";
	sprintf(lightBuffer, "%d Lux", lightReading);
	strcat(str, lightBuffer);
	strcat(str, " ");
}

void accToString(char* xStr, char* yStr, char* zStr) {
	strcat(xStr, "x: ");
	strcat(yStr, "y: ");
	strcat(zStr, "z: ");
	char xBuffer[5] = "";
	char yBuffer[5] = "";
	char zBuffer[5] = "";
	sprintf(xBuffer, "%d", xReading);
	strcat(xStr, xBuffer);
	strcat(xStr, "  ");
	sprintf(yBuffer, "%d", yReading);
	strcat(yStr, yBuffer);
	strcat(yStr, "  ");
	sprintf(zBuffer, "%d", zReading);
	strcat(zStr, zBuffer);
	strcat(zStr, "  ");
}

static void initMonitorOLED() {
	oled_putString(xoled - 20, yoled - 32, "MONITOR", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 12, "Light: -", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(60, 12, "Lux", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 26, "Temp : -", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_circle(62, 27, 2, OLED_COLOR_WHITE);
	oled_putString (65, 27, "C", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "x : -", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 47, "y : -", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 55, "z : -", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
}

// This function updates the OLED display of the sensor readings
static void updateOLED() {

	char tempString[15] = "";
	temperatureToString(tempString);

	char lightString[15] = "";
	lightToString(lightString);

	char xString[15] = "";
	char yString[15] = "";
	char zString[15] = "";
	accToString(xString, yString, zString);

	oled_putString(xoled - 20, yoled - 32, "MONITOR", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 8, lightString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 16, tempString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_circle(63, 17, 2, OLED_COLOR_WHITE);
	oled_putString(xoled - 48, yoled - 6, xString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(xoled - 48, yoled + 3, yString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(xoled - 48, yoled + 12, zString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
}

static uint32_t getMsTick(void) {
	return msTicks;
}
static void init_ssp(void) {
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void) {
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void) {
	// Initialize button 3
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 4;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, 1 << 4, 0);

	// Initialize sw4
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 31;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, 1 << 31, 0);

	// Initialize P2.5
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 5;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, (1 << 5), 0);
	LPC_GPIOINT ->IO2IntEnF |= 1 << 5; // Enable GPIO interrupt for light sensor
	LPC_GPIOINT ->IO2IntClr |= (1 << 5);

}
// EINT0 Interrupt Handler
//void EINT0_IRQHandler(void) {
//	if ((LPC_GPIOINT ->IO2IntStatF >> 10) & 0x1) {
//		if (monitorFlag == 0) {
//			printf("Entering MONITOR Mode.\r\n");
//			monitorFlag = 1;
//			msTicks = 0;
//
//		} else if (monitorFlag == 1) {
//			printf("Entering STABLE Mode.\r\n");
//			monitorFlag = 0;
//		}
//		LPC_GPIOINT ->IO2IntClr = 1 << 10;
//	}
//
//	NVIC_ClearPendingIRQ(EINT0_IRQn);
//}

// EINT3 Interrupt Handler
void EINT3_IRQHandler(void) {
//	int i;
//	printf("Entering EINT3 \n");
//
//	if (acc_getInt1Status()) {
//		printf("Accelerometer detection \n");
//		acc_clearIntStatus();
//	}

	if ((light_getIrqStatus())) {
//		printf("Entered light interrupt ISR\n");
		if (lightLowWarning == 0) {
			printf("Low light conditions, %d\n", light_read());
			lightLowWarning = 1;
			light_setLoThreshold(0);
			light_setHiThreshold(51);
		} else if (lightLowWarning == 1) {
			printf("Safe light conditions, %d\n", light_read());
			lightLowWarning = 0;
			light_setHiThreshold(3891);
			light_setLoThreshold(50);
		}
		LPC_GPIOINT ->IO2IntClr |= (1 << 5);
		light_clearIrqStatus();
	}
	NVIC_ClearPendingIRQ(EINT3_IRQn);
	printf("clear NVIC in IQR\n");

//	// Determine whether GPIO Interrupt P2.10 has occurred
//	if ((LPC_GPIOINT ->IO2IntStatF >> 10) & 0x1) {
//		if (monitorFlag == 0) {
//			printf("Entering MONITOR Mode.\r\n");
//			monitorFlag = 1;
//			msTicks = 0;
//
//		} else if (monitorFlag == 1) {
//			printf("Entering STABLE Mode.\r\n");
//			monitorFlag = 0;
//		}
////      for (i=0;i<9999999;i++);
//		LPC_GPIOINT ->IO2IntClr = 1 << 10; // Clear GPIO Interrupt P2.10
//	}
//
//	if ((LPC_GPIOINT ->IO0IntStatF >> 25) & 0x1) {
//		printf("GPIO Interrupt 0.25\n");
//		LPC_GPIOINT ->IO0IntClr = 1 << 25;
//	}
//
//	else if ((LPC_GPIOINT ->IO0IntStatF >> 24) & 0x1) {
//		printf("GPIO Interrupt 0.24\n");
//		LPC_GPIOINT ->IO0IntClr = 1 << 24;
//	}
}

void blinkBlueLed(volatile uint32_t msTicks, uint32_t rate) {
	if ((msTicks / rate) % 2) {
		GPIO_SetValue(0, (1 << 26));
	} else {
		GPIO_ClearValue(0, (1 << 26));
	}
}

void blinkRedLed(volatile uint32_t msTicks, uint32_t rate) {
	if ((msTicks / rate) % 2) {
		GPIO_SetValue(2, 1);
	} else {
		GPIO_ClearValue(2, 1);
	}
}

int main(void) {

	SysTick_Config(SystemCoreClock / 1000);  // every 1ms

//	uint8_t dir = 1;
//	uint8_t wait = 0;
//
//	uint8_t state = 0;
//
//	uint8_t btn1 = 1;
	uint8_t btn2 = 1;
	swTicks = msTicks;

//	uint8_t buzzerFlag = 0;
	int sampleFlag = 0;
	int segNum;
	uint32_t blinkRate = 333;

	init_i2c();
	init_ssp();
	init_GPIO();

	pca9532_init();
	joystick_init();
	acc_init();
//	acc_clearIntStatus();
	oled_init();
	led7seg_init();
	rgb_init();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_clearIrqStatus();
	if (light_read() < 51) {
		lightLowWarning = 1;
		light_setLoThreshold(0);
		light_setHiThreshold(51);
	} else {
		lightLowWarning = 0;
		light_setLoThreshold(50);
	}
	temp_init(getMsTick);

//	NVIC_EnableIRQ(EINT0_IRQn); // Enable EINT0 interrupt
//	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn); // Enable EINT3 interrupt
	NVIC_ClearPendingIRQ(EINT3_IRQn);

//	// Enable GPIO Interrupt P1.31
//	LPC_GPIOINT ->IO2IntEnF |= 1 << 31;

//	// Enable GPIO Interrupt P2.10 (SW3)
//	LPC_GPIOINT ->IO2IntEnF |= 1 << 10;

//	// Enable GPIO Interrupt P0.24 & P0.25 (SW5)
//	LPC_GPIOINT ->IO0IntEnF |= 1 << 24;
//	LPC_GPIOINT ->IO0IntEnF |= 1 << 25;

//	//Accelerometer init
//	PINSEL_CFG_Type PinCfg;
//	PinCfg.Funcnum = 0;
//	PinCfg.Pinnum = 3;
//	PinCfg.Portnum = 0;
//	PinCfg.Pinmode = 0;
//	PinCfg.OpenDrain = 0;
//	PINSEL_ConfigPin(&PinCfg);
//	GPIO_SetDir(0, (1 << 3), 0);
//	LPC_GPIOINT ->IO0IntEnF |= 1 << 3;

	/*
	 * Assume base board in zero-g position when reading first value.
	 */
	acc_read(&xReading, &yReading, &zReading);
	xoff = 0 - xReading;
	yoff = 0 - yReading;
	zoff = 64 - zReading;

	/* ---- Speaker ------> */

	GPIO_SetDir(2, 1 << 0, 1);
	GPIO_SetDir(2, 1 << 1, 1);

	GPIO_SetDir(0, 1 << 27, 1);
	GPIO_SetDir(0, 1 << 28, 1);
	GPIO_SetDir(2, 1 << 13, 1);
	GPIO_SetDir(0, 1 << 26, 1);

	GPIO_ClearValue(0, 1 << 27); //LM4811-clk
	GPIO_ClearValue(0, 1 << 28); //LM4811-up/dn
	GPIO_ClearValue(2, 1 << 13); //LM4811-shutdn
//
//	/* <---- Speaker ------ */

//	moveBar(1, dir);

	while (1) {

		while (monitorFlag == 1) {

			/* ####### Accelerometer and LEDs  ###### */
			/* # */
			/* # */
			/* ############################################# */

			/* ####### Joystick and OLED  ###### */
			/* # */
			/* ############################################# */

			/* ############ 7 Segment LED Timer  ########### */
			/* # */

			segNum = msTicks / 1000 % 16;
			if (segNum > 9)
				segNum += 7;
			led7seg_setChar(segNum + 48, TRUE);
			if ((segNum + 48 == '5' || segNum + 48 == 'A' || segNum + 48 == 'F')
					&& sampleFlag == 0) {
				//printf("%c : updating sensors at 5AF\n", segNum + 48);
				sampleFlag = 1;
				updateSensors();
//				updateOLED();
			}
			if (!(segNum + 48 == '5' || segNum + 48 == 'A' || segNum + 48 == 'F')) {
				sampleFlag = 0;
			}
			/* # */
			/* ############################################# */
			/* ############ Light sensor  ########### */
			/* # */
			/* # */
			/* ############################################# */

			/* ############ Trimpot and RGB LED  ########### */
			/* # */
			if (fireAlert == 0 && temperatureReading > 290) {
				printf("DANGER ALERT: %.1f\n", temp_read() / 10.0);
				fireAlert = 1;
			}

			if (moveInDarkAlert == 0 && lightLowWarning == 1) {
				light_setHiThreshold(3891);
				updateAccSensor();
				light_setHiThreshold(51);
				if (abs(xReading) > 96 || abs(yReading) > 96
						|| abs(zReading) > 96) {
					moveInDarkAlert = 1;
				}
			}

			if (moveInDarkAlert == 1) {
				blinkBlueLed(msTicks, blinkRate);
			}

			if (fireAlert == 1) {
				blinkRedLed(msTicks, blinkRate);
			}

			btn2 = (GPIO_ReadValue(1) >> 31) & 0x01;
			if (btn2 == 0 && (msTicks - swTicks >= 200)) {
				swTicks = msTicks;
				monitorFlag = 0;
				printf("Entering STABLE Mode.\r\n");
			}
			/* # */
			/* ############################################# */

			Timer0_Wait(1);
		}

		oled_clearScreen(OLED_COLOR_BLACK);
		led7seg_setChar(' ', FALSE);
		GPIO_ClearValue(2, 1);
		moveInDarkAlert = 0;
		fireAlert = 0;
		lightLowWarning = 0;

		while (monitorFlag == 0) {

			btn2 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (btn2 == 0 && (msTicks - swTicks >= 500)) {
				swTicks = msTicks;
				monitorFlag = 1;
				printf("Entering MONITOR Mode.\r\n");
				updateSensors();
				initMonitorOLED();
//				updateOLED();
				msTicks = 0;
			}
		}

	}
}

void check_failed(uint8_t *file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
		;
}
