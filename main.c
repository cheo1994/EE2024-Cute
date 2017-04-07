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
#include "lpc17xx_rit.h"
#include "lpc17xx_uart.h"
#include "joystick.h"

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
volatile int updateTempReadingFlag = 0;
uint8_t lightLowWarning;
uint8_t fireAlert = 0;
uint8_t moveInDarkAlert = 0;
int ritInterruptEnabledFlag = 0;
int8_t xReading;
int8_t yReading;
int8_t zReading;
int NNN = 0;
int segNum = 0;
volatile int updateOledFlag = 0;
volatile int sendCemsFlag = 0;
volatile int sendHelpMsgFlag = 0;
volatile int toggleBlink = 0;
int currentScreen = 0;
int oledUpdatedFlag = 0;
uint8_t* light_buffer[15];
uint8_t* temp_buffer[15];
uint8_t* x_buffer[15];
uint8_t* y_buffer[15];
uint8_t* z_buffer[15];
int32_t xoff = 0;
int32_t yoff = 0;
int32_t zoff = 0;
static uint8_t invertedChars[] = {
/* digits 0 - 9 */
0x24, 0x7D, 0xE0, 0x70, 0x39, 0x32, 0x22, 0x7C, 0x20, 0x30,
/* A - F */
0x28, 0x23, 0xA6, 0x61, 0xA2, 0xAA };

void updateAccSensor() {
	NVIC_DisableIRQ(EINT3_IRQn);
	acc_read(&xReading, &yReading, &zReading);
	NVIC_EnableIRQ(EINT3_IRQn);
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

void tempReadToString(char *str) {
	char tempBuffer[5] = "";
	sprintf(tempBuffer, "%.1f", temperatureReading / 10.0);
	strcat(str, tempBuffer);
}

void lightReadToString(char *str) {
	char lightBuffer[9] = "";
	sprintf(lightBuffer, "%4d", lightReading);
	strcat(str, lightBuffer);
}

void accReadToString(char* xStr, char* yStr, char* zStr) {
	char xBuffer[5] = "";
	char yBuffer[5] = "";
	char zBuffer[5] = "";
	sprintf(xBuffer, "%3d", xReading);
	strcat(xStr, xBuffer);
//	strcat(xStr, "  ");
	sprintf(yBuffer, "%3d", yReading);
	strcat(yStr, yBuffer);
//	strcat(yStr, "  ");
	sprintf(zBuffer, "%3d", zReading);
	strcat(zStr, zBuffer);
//	strcat(zStr, "  ");
}

static void initMonitorOled() {
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

void initMonitor2Oled() {
	oled_putString(0, 12, "Request", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "Cancel ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

static void updateOled() {

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
	oled_putString(20, 39, xString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(20, 47, yString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(20, 55, zString, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

static uint32_t getMsTick(void) {
	return msTicks;
}

void pinsel_uart3(void) {
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
}

void onRedLed() {
	GPIO_SetValue(2, 1);
}

void offRedLed() {
	GPIO_ClearValue(2, 1);
}

void offBlueLed() {
	GPIO_ClearValue(0, (1 << 26));
}

void onBlueLed() {
	GPIO_SetValue(0, (1 << 26));
}

void SysTick_Handler(void) {
	msTicks++;
}
void RIT_IRQHandler(void) {

	if (toggleBlink == 0) {
		if (fireAlert == 1) {
			onRedLed();
		}

		if (moveInDarkAlert == 1) {
			onBlueLed();
		}
		toggleBlink = 1;
	} else if (toggleBlink == 1) {
		if (fireAlert == 1) {
			offRedLed();
		}
		if (moveInDarkAlert == 1) {
			offBlueLed();
		}
		toggleBlink = 0;
	}
	LPC_RIT ->RICTRL |= 0x1;
	NVIC_ClearPendingIRQ(RIT_IRQn);
}
void EINT0_IRQHandler(void) {
//	printf("eint0 handler \n");
	sendHelpMsgFlag = 1;
	LPC_SC ->EXTINT = 1;
	NVIC_ClearPendingIRQ(EINT0_IRQn);
}
void EINT3_IRQHandler(void) {
//	printf("enter eint3 handler\n");
	if (light_getIrqStatus()) {
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
void TIMER0_IRQHandler(void) {
	if (LPC_TIM0 ->IR & (1 << 0)) {
		segNum = (++segNum) % 16;
		led7seg_setChar(invertedChars[segNum], TRUE);
		if (segNum == 5 || segNum == 10 || segNum == 15) {
			updateOledFlag = 1;
			if (segNum == 15) {
				sendCemsFlag = 1;
			}
		} else {
			updateTempReadingFlag = 1;
		}
		TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
		NVIC_ClearPendingIRQ(TIMER0_IRQn);
	}
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
	// Initialize sw3
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);

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

//	// Initialize P0.2
//	PinCfg.Funcnum = 0;
//	PinCfg.OpenDrain = 0;
//	PinCfg.Pinmode = 0;
//	PinCfg.Portnum = 0;
//	PinCfg.Pinnum = 2;
//	PINSEL_ConfigPin(&PinCfg);
//	GPIO_SetDir(0, (1 << 2), 0);
//	LPC_GPIOINT ->IO0IntEnF |= 1 << 2; // Enable GPIO interrupt for temperature
//	LPC_GPIOINT ->IO0IntClr |= (1 << 2);

}
static void init_uart(void) {
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
static void initRitInterrupt() {
	RIT_Init(LPC_RIT );
	RIT_TimerClearCmd(LPC_RIT, ENABLE);
}
static void initUart3Interrupt() {
	// enable UART interrupts to send/receive
	LPC_UART3 ->IER |= UART_IER_RBRINT_EN;
	LPC_UART3 ->IER |= UART_IER_THREINT_EN;
	NVIC_EnableIRQ(UART3_IRQn);
}
static void initTimer0Interrupt() {
	TIM_MATCHCFG_Type timMatchCfg;
	timMatchCfg.MatchChannel = 0;
	timMatchCfg.IntOnMatch = 1;
	timMatchCfg.StopOnMatch = 0;
	timMatchCfg.ResetOnMatch = 1;
	timMatchCfg.ExtMatchOutputType = 0;
	timMatchCfg.MatchValue = 1000;
	TIM_ConfigMatch(LPC_TIM0, &timMatchCfg);

	TIM_TIMERCFG_Type timTimerCfg;
	timTimerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
	timTimerCfg.PrescaleValue = 1000;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timTimerCfg);

	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);
}
static void initAll() {
	init_i2c();
	init_ssp();
	init_GPIO();
//	pca9532_init();
	joystick_init();
	acc_init();
	oled_init();
	led7seg_init();
	rgb_init();
	init_uart();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_clearIrqStatus();
	lightThresholdInit();
	temp_init(getMsTick);
}

void prepareStableMode() {
	oled_clearScreen(OLED_COLOR_BLACK);	//Clear the OLED display
	led7seg_setChar(' ', FALSE);		//Blank off the LED 7 Segment display
	segNum = 0; 						//Reset the segnum count to 0
	fireAlert = 0; 						//reset fire alert
	ritInterruptEnabledFlag = 0; 		//turn off the RGB led
	moveInDarkAlert = 0;				//Reset moving in dark flag
	lightLowWarning = 0;				//Reset low light flag
	offBlueLed();
	offRedLed();
	TIM_Cmd(LPC_TIM0, DISABLE); 		//Disables timer for 7Seg
	TIM_ResetCounter(LPC_TIM0 );		//Resets the counter timer for 7seg
	NVIC_DisableIRQ(RIT_IRQn);

	//used for 2nd screen
	currentScreen = 0;
	oledUpdatedFlag = 0;
}

int main(void) {

	SysTick_Config(SystemCoreClock / 1000);  // every 1ms

	uint8_t sw4 = 1;
	uint8_t joystickStatus = JOYSTICK_CENTER;
	int sw4HoldStatus = 0;
	int joystickHold = 0;

	char* monitorMsg = "Entering MONITOR Mode.\r\n";
	char* darknessMsg = "Movement in darkness was Detected.\r\n";
	char* fireMsg = "Fire was Detected.\r\n";
	int darknessMsgLen = strlen(darknessMsg);
	int fireMsgLen = strlen(fireMsg);
	int monitorMsgLen = strlen(monitorMsg);

	initAll();
	initRitInterrupt();
	initTimer0Interrupt();

	LPC_SC ->EXTINT = 1;
	LPC_SC ->EXTMODE = 1;
	LPC_SC ->EXTPOLAR = 0;

	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_SetPriority(EINT0_IRQn, NVIC_EncodePriority(5, 2, 0));
	NVIC_EnableIRQ(EINT0_IRQn); // Enable EINT0 interrupt

	NVIC_ClearPendingIRQ(EINT3_IRQn);
	NVIC_SetPriority(EINT3_IRQn, NVIC_EncodePriority(5, 3, 0)); //NVIC_EncodePriority outputs 24 = 0x18
	NVIC_EnableIRQ(EINT3_IRQn); // Enable EINT3 interrupt

	//Assume base board in zero-g position when reading first value.
	acc_read(&xReading, &yReading, &zReading);
	xoff = 0 - xReading;
	yoff = 0 - yReading;
	zoff = 64 - zReading;

	while (1) {

		prepareStableMode();

		while (monitorFlag == 0) {

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

			if (sw4 == 0 && sw4HoldStatus == 0) {
				monitorFlag = 1;
				UART_Send(LPC_UART3, monitorMsg, monitorMsgLen, BLOCKING);
				sw4HoldStatus = 1;
				sendHelpMsgFlag = 0;
				initMonitorOled();
				TIM_Cmd(LPC_TIM0, ENABLE);
				led7seg_setChar(invertedChars[0], TRUE);
				updateSensors();
				lightThresholdInit();
			}
		}

		while (monitorFlag == 1) {
			if (sendCemsFlag == 1) {
				char str[37] = "";
				sprintf(str, "%03d_-_T%.1f_L%d_AX%d_AY%d_AZ%d\r\n", NNN++,
						temperatureReading / 10.0, lightReading, xReading,
						yReading, zReading);

				if (fireAlert == 1) {
					UART_Send(LPC_UART3, fireMsg, fireMsgLen, BLOCKING);
				}

				if (moveInDarkAlert == 1) {
					UART_Send(LPC_UART3, darknessMsg, darknessMsgLen, BLOCKING);
				}

				UART_Send(LPC_UART3, (uint8_t *) str, strlen(str), BLOCKING);

				sendCemsFlag = 0;
			}

			if (updateOledFlag == 1) {
				updateSensors();
				if (currentScreen == 0) {
					updateOled();
				}
				oledUpdatedFlag = 1;
				updateOledFlag = 0;
			}

			if (fireAlert == 0 && temperatureReading > 290) {
				fireAlert = 1;
				if (ritInterruptEnabledFlag == 0) {
					RIT_Cmd(LPC_RIT, ENABLE);
					NVIC_ClearPendingIRQ(RIT_IRQn);
					NVIC_EnableIRQ(RIT_IRQn);
					ritInterruptEnabledFlag = 1;
				}
			}

			if (moveInDarkAlert == 0 && lightLowWarning == 1) {
				updateAccSensor();
				if (abs(xReading) > 96 || abs(yReading) > 96
						|| abs(zReading) > 96) {
					moveInDarkAlert = 1;
					if (ritInterruptEnabledFlag == 0) {
						RIT_Cmd(LPC_RIT, ENABLE);
						NVIC_ClearPendingIRQ(RIT_IRQn);
						NVIC_EnableIRQ(RIT_IRQn);
						ritInterruptEnabledFlag = 1;
					}
				}
			}

			if (updateTempReadingFlag == 1) {
				updateTempSensor();
				updateTempReadingFlag = 0;
			}

//			joystickStatus = joystick_read();
//			if (joystickStatus == JOYSTICK_CENTER) {
////				printf("it is in the centre\n");
//			}
//
//			if (joystickStatus != JOYSTICK_RIGHT
//					&& joystickStatus != JOYSTICK_LEFT) {
//				joystickHold = 0;
//			}
//
//			if ((joystickHold == 0) &&
//					(joystickStatus == JOYSTICK_RIGHT
//					|| joystickStatus == JOYSTICK_LEFT)) {
////				printf("joystick moved left or right\n");
//				if (currentScreen == 0) {
//					oled_clearScreen(OLED_COLOR_BLACK);
//					initMonitor2Oled();
//					currentScreen = 1;
//				} else if (currentScreen == 1) {
//					oled_clearScreen(OLED_COLOR_BLACK);
//					initMonitorOled();
//					if (oledUpdatedFlag == 1) {
//						updateOled();
//					}
//					currentScreen = 0;
//				}
//				joystickHold = 1;
//			}

			if (sendHelpMsgFlag == 1) {
				UART_Send(LPC_UART3, (uint8_t *) "Please send help.\r\n", 19,
						BLOCKING);
				sendHelpMsgFlag = 0;
			}

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 0 && sw4HoldStatus == 0) {
				monitorFlag = 0;
				sw4HoldStatus = 1;
				RIT_Cmd(LPC_RIT, DISABLE);
			}

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

//			Timer0_Wait(1);
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
