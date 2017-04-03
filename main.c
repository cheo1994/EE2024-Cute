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
#include "lpc17xx_uart.h"

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
int NNN = 0;
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
static char* msg = NULL;

// This function is called every 1us
void SysTick_Handler(void) {
	msTicks++;
}

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
// This function updates the temperature reading, light reading and accelerometer readings
static void updateSensors() {
	updateLightSensor();
	updateTempSensor();
	updateAccSensor();
}

void tempReadToString(char *str) {
	char tempBuffer[5] = "";
	sprintf(tempBuffer, "%.1f", temperatureReading / 10.0);
	strcat(str, tempBuffer);
}

void lightReadToString(char *str) {
	char lightBuffer[9] = "";
	sprintf(lightBuffer, "%d", lightReading);
	strcat(str, lightBuffer);
	if (lightReading < 10)
		strcat(str, "    ");
	else if (lightReading < 100)
		strcat(str, "  ");
	else if (lightReading < 1000)
		strcat(str, " ");
}

void accReadToString(char* xStr, char* yStr, char* zStr) {
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
	oled_putString(0, 12, "Light:", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(65, 12, "Lux", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 26, "Temp :", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_circle(67, 27, 2, OLED_COLOR_WHITE);
	oled_putString (70, 27, "C", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "x :", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 47, "y :", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(0, 55, "z :", OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
}

// This function updates the OLED display of the sensor readings
static void updateOLED() {

	char tempString[10] = "";
	tempReadToString(tempString);

	char lightString[10] = "";
	lightReadToString(lightString);

	char xString[8] = "";
	char yString[8] = "";
	char zString[8] = "";
	accReadToString(xString, yString, zString);

	oled_putString(35, 12, lightString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(35, 26, tempString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(20, 39, xString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(20, 47, yString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
	oled_putString(20, 55, zString, OLED_COLOR_WHITE,
			OLED_COLOR_BLACK);
}

static uint32_t getMsTick(void) {
	return msTicks;
}

void pinsel_uart3(void){
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 2;
    PinCfg.Pinnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 1;
    PINSEL_ConfigPin(&PinCfg);
}

void init_uart(void){
    UART_CFG_Type uartCfg;
    uartCfg.Baud_rate = 115200;
    uartCfg.Databits = UART_DATABIT_8;
    uartCfg.Parity = UART_PARITY_NONE;
    uartCfg.Stopbits = UART_STOPBIT_1;
    //pin select for uart3;
    pinsel_uart3();
    //supply power & setup working parameters for uart3
    UART_Init(LPC_UART3, &uartCfg);
    //enable transmit for uart3
    UART_TxCmd(LPC_UART3, ENABLE);
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
//	if (acc_getInt1Status()) {
//		printf("Accelerometer detection \n");
//		acc_clearIntStatus();
//	}

	if ((light_getIrqStatus())) {
//		printf("Entered light interrupt ISR\n");
		if (lightLowWarning == 0) {
//			printf("Low light conditions, %d\n", light_read());
			lightLowWarning = 1;
			light_setLoThreshold(0);
			light_setHiThreshold(51);
		} else if (lightLowWarning == 1) {
//			printf("Safe light conditions, %d\n", light_read());
			lightLowWarning = 0;
			light_setHiThreshold(3891);
			light_setLoThreshold(50);
		}
		LPC_GPIOINT ->IO2IntClr |= (1 << 5);
		light_clearIrqStatus();
	}
	NVIC_ClearPendingIRQ(EINT3_IRQn);
}

void blinkBlueLed(volatile uint32_t msTicks, uint32_t rate) {
	if ((msTicks / rate) % 2) {
		GPIO_SetValue(0, (1 << 26));
	} else {
		GPIO_ClearValue(0, (1 << 26));
	}
}

void offBlueLed() {
	GPIO_ClearValue(0, (1 << 26));
}

void blinkRedLed(volatile uint32_t msTicks, uint32_t rate) {
	if ((msTicks / rate) % 2) {
		GPIO_SetValue(2, 1);
	} else {
		GPIO_ClearValue(2, 1);
	}
}

void offRedLed() {
	GPIO_ClearValue(2, 1);
}

void lightThresholdInit() {
	if (light_read() < 51) {
		lightLowWarning = 1;
		light_setLoThreshold(0);
		light_setHiThreshold(51);
	} else {
		lightLowWarning = 0;
		light_setLoThreshold(50);
	}
}

int main(void) {

	SysTick_Config(SystemCoreClock / 1000);  // every 1ms

	uint8_t sw4 = 1;
	int sw4HoldStatus = 0;
	swTicks = msTicks;

	int sampleFlag = 0;
	int segNum;
	uint32_t blinkRate = 333;

	char* monitorMsg = "Entering MONITOR Mode.\r\n";
    char* darknessMsg = "Movement in darkness was Detected.\r\n";
    char* fireMsg = "Fire was Detected.\r\n";
    int darknessMsgLen = strlen(darknessMsg);
    int fireMsgLen = strlen(fireMsg);
    int monitorMsgLen = strlen(monitorMsg);

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
    init_uart();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_clearIrqStatus();
	lightThresholdInit();
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

//	uart test code from lecture
//    uint8_t data = 0;
//    uint32_t len = 0;
//    uint8_t line[64];
//    //test sending message
//    msg = "Welcome to CY & K's EE2024 project \r\n";
//    UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
//    //test receiving a letter and sending back to port
//    UART_Receive(LPC_UART3, &data, 1, BLOCKING);
//    UART_Send(LPC_UART3, &data, 1, BLOCKING);
////    test receiving message without knowing message length
//    len = 0;
//    do
//    {   UART_Receive(LPC_UART3, &data, 1, BLOCKING);
//
//        if (data != '\r')
//        {
//            len++;
//            line[len-1] = data;
//        }
//    } while ((len<64) && (data != '\r'));
//    line[len]=0;
//    UART_SendString(LPC_UART3, &line);
//    printf("--%s--\n", line);
//    while (1);
//    return 0;

	while (1) {

		while (monitorFlag == 1) {
			/* ############ 7 Segment LED Timer  ########### */
			/* # */

			segNum = msTicks / 1000 % 16;
			if (segNum > 9)
				segNum += 7;
			led7seg_setChar(segNum + 48, TRUE);
			if ((segNum + 48 == '5' || segNum + 48 == 'A' || segNum + 48 == 'F')
					&& sampleFlag == 0) {
				sampleFlag = 1;
				updateSensors();
				updateOLED();
				if (segNum + 48 == 'F') {
					char str[30] = "";
					sprintf(str, "%d_-_T%.1f_L%d_AX%d_AY%d_AZ%d\r\n", NNN++, temperatureReading / 10.0,
							lightReading, xReading, yReading, zReading);

					if (fireAlert == 1) {
						UART_Send(LPC_UART3, fireMsg, fireMsgLen, BLOCKING);
					}

					if (moveInDarkAlert == 1) {
						UART_Send(LPC_UART3, darknessMsg, darknessMsgLen, BLOCKING);
					}

					if (NNN < 10) {
						char dataToSend[30] = "00";
						strcat(dataToSend, str);
						UART_Send(LPC_UART3, (uint8_t *) dataToSend, strlen(dataToSend), BLOCKING);
					}
					else if (NNN < 100){
						char dataToSend[30] = "0";
						strcat(dataToSend, str);
						UART_Send(LPC_UART3, (uint8_t *) dataToSend, strlen(dataToSend), BLOCKING);
					}
					else {
						UART_Send(LPC_UART3, (uint8_t *) str, strlen(str), BLOCKING);
					}
				}
			}

			if (!(segNum + 48 == '5' || segNum + 48 == 'A' || segNum + 48 == 'F')) {
				sampleFlag = 0;
			}

			/* ########### RGB LED  ########### */
			/* # */
			if (fireAlert == 0 && temperatureReading > 290) {
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
			/* # */
			/* ############################################# */

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

				if (sw4 == 0 && sw4HoldStatus == 0) {
				swTicks = msTicks;
				monitorFlag = 0;
				sw4HoldStatus = 1;
			}

			Timer0_Wait(1);
		}

		oled_clearScreen(OLED_COLOR_BLACK);
		led7seg_setChar(' ', FALSE);
		GPIO_ClearValue(2, 1);
		moveInDarkAlert = 0;
		fireAlert = 0;
		lightLowWarning = 0;
		offBlueLed();
		offRedLed();

		while (monitorFlag == 0) {

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

			if (sw4 == 0 && sw4HoldStatus == 0) {
				monitorFlag = 1;
				UART_Send(LPC_UART3, monitorMsg, monitorMsgLen, BLOCKING);
				initMonitorOLED();
				msTicks = 0;
				swTicks = 0;
				sw4HoldStatus = 1;
			}
		}
		lightThresholdInit();
	}
}

void check_failed(uint8_t *file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
		;
}
