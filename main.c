/***************************
 *   EE2024 Assignment 2 for CUTE
 *   Student 1: Kelvin Ng Poh Ching
 *   Student 2: Teh Chee Yeo A0139671X
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 **************************/

/*****************************************************************************/
/**************************** Includes ***************************************/
/*****************************************************************************/
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
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "light.h"
#include "temp.h"

#define TEMPERATURE_THRESHOLD 320 // 290 for 29.0 Degree celcius
#define LIGHT_LOW_THRESHOLD 50 // 50 Lux is the low light threshold
#define TEMP_SCALAR_DIV10 1
#define TEMP_HALF_PERIODS 340

typedef enum {
	MONITOR_STATE, STABLE_STATE
} CUTE_STATE;

typedef enum {
	MONITOR_READINGS, MONITOR_OPTIONS, STABLE
} OLED_STATE;

volatile uint32_t msTicks = 0;
CUTE_STATE cuteStatus = STABLE_STATE;
OLED_STATE oledStatus = STABLE;
uint32_t lightReading;
int32_t temperatureReading;
uint8_t lightLowWarning;
uint8_t fireAlert = 0;
uint8_t moveInDarkAlert = 0;
int ritInterruptEnabledFlag = 0;
int cancelOptionFlag = 0;
int8_t xReading;
int8_t yReading;
int8_t zReading;
int NNN = 0;
volatile int segNum = 0;
volatile int updateOledFlag = 0;
volatile int sendCemsFlag = 0;
volatile int sendHelpMsgFlag = 0;
volatile int cancelHelpMsgFlag = 0;
volatile int toggleBlink = 0;
int oledUpdatedFlag = 0;
int32_t xoff = 0;
int32_t yoff = 0;
int32_t zoff = 0;
char* monitorMsg = "Entering MONITOR Mode.\r\n";
char* darknessMsg = "Movement in darkness was Detected.\r\n";
char* fireMsg = "Fire was Detected.\r\n";
int darknessMsgLen = 36;
int fireMsgLen = 20;
int monitorMsgLen = 24;

int32_t temp_t1 = 0;
int32_t temp_t2 = 0;
int temp_count = 0;

static uint8_t invertedChars[] = {
/* digits 0 - 9 inverted */
0x24, 0x7D, 0xE0, 0x70, 0x39, 0x32, 0x22, 0x7C, 0x20, 0x30,
/* A - F inverted */
0x28, 0x23, 0xA6, 0x61, 0xA2, 0xAA };

/*****************************************************************************/
/*************************** RGB Helper Functions ****************************/
/*****************************************************************************/
void offRedLed(void) {
	GPIO_ClearValue(2, 1);
}

void onRedLed(void) {
	GPIO_SetValue(2, 1);
}

void offBlueLed(void) {
	GPIO_ClearValue(0, (1 << 26));
}

void onBlueLed(void) {
	GPIO_SetValue(0, (1 << 26));
}

void enableRitRGBinterrupt(void) {
	RIT_Cmd(LPC_RIT, ENABLE);
	NVIC_ClearPendingIRQ(RIT_IRQn);
	NVIC_EnableIRQ(RIT_IRQn);
}

/*****************************************************************************/
/********************** Light Sensor Helper Functions ************************/
/*****************************************************************************/
void updateLightSensor(void) {
	lightReading = light_read();
}

void setLightThreshold(void) {
	if (light_read() <= LIGHT_LOW_THRESHOLD) {
		lightLowWarning = 1;
		light_setLoThreshold(0);
		light_setHiThreshold(51);
	} else {
		lightLowWarning = 0;
		light_setLoThreshold(LIGHT_LOW_THRESHOLD);
	}
}

void lightReadToString(char *str) {
	char lightBuffer[9] = "";
	sprintf(lightBuffer, "%4d", lightReading);
	strcat(str, lightBuffer);
}

void disableGPIOLightInterrupt(void) {
	LPC_GPIOINT ->IO2IntEnF &= ~(1 << 5);
}

void enableGPIOLightInterrupt(void) {
	LPC_GPIOINT ->IO2IntEnF |= (1 << 5);
}

/*****************************************************************************/
/***************** Accelerometer Helper Functions ****************************/
/*****************************************************************************/
void updateAccSensor(void) {
	disableGPIOLightInterrupt();
	acc_read(&xReading, &yReading, &zReading);
	enableGPIOLightInterrupt();
	xReading = xReading + xoff;
	yReading = yReading + yoff;
	zReading = zReading + zoff;
}

int checkForMovement(void) {
	return abs(xReading) > 96 || abs(yReading) > 96 || abs(zReading) > 96;
}

void accReadToString(char* xStr, char* yStr, char* zStr) {
	char xBuffer[5] = "";
	char yBuffer[5] = "";
	char zBuffer[5] = "";
	sprintf(xBuffer, "%3d", xReading);
	strcat(xStr, xBuffer);
	sprintf(yBuffer, "%3d", yReading);
	strcat(yStr, yBuffer);
	sprintf(zBuffer, "%3d", zReading);
	strcat(zStr, zBuffer);
}

/*****************************************************************************/
/***************** Temperature Helper Functions ****************************/
/*****************************************************************************/
void updateTempSensor(void) {
	temperatureReading = temp_read();
}

void updateSensors(void) {
	updateLightSensor();
	//updateTempSensor();
	updateAccSensor();
}

void tempReadToString(char *str) {
	char tempBuffer[5] = "";
	sprintf(tempBuffer, "%.1f", temperatureReading / 10.0);
	strcat(str, tempBuffer);
}


/*****************************************************************************/
/************************* OLED Helper Functions *****************************/
/*****************************************************************************/
void prepareMonitorReadingsOled(void) {
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

void updateOledReadings(void) {

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

void prepareMonitorOptionsOled(void) {
	oled_putString(0, 12, "Request <", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "Cancel ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void toggleToRequest(void) {
	oled_putString(0, 12, "Request <", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "Cancel   ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	cancelOptionFlag = 0;
}

void toggleToCancel(void) {
	oled_putString(0, 12, "Request  ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 39, "Cancel  <", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	cancelOptionFlag = 1;
}

void switchToMonitorOptions(void) {
	oled_clearScreen(OLED_COLOR_BLACK);
	prepareMonitorOptionsOled();
	cancelOptionFlag = 0;
}

void switchToMonitorReadings(void) {
	oled_clearScreen(OLED_COLOR_BLACK);
	prepareMonitorReadingsOled();
	if (oledUpdatedFlag == 1) {
		updateOledReadings();
	}
}

/*****************************************************************************/
/*************************** UART Helper Functions ***************************/
/*****************************************************************************/
void sendHelpRequest(void) {
	UART_Send(LPC_UART3, (uint8_t *) "Requesting help.\r\n", 18, BLOCKING);
	int i;
	for (i = 0; i < 8; i++) {
		pca9532_setLeds(0x1 << i, 0xFFFF);
		pca9532_setLeds(0x8000 >> i, 0x0);
		Timer0_Wait(25);
	}
	pca9532_setLeds(0x0, 0xFFFF);
}

void sendCancelLastRequest(void) {
	UART_Send(LPC_UART3, (uint8_t *) "Cancel last request.\r\n", 22, BLOCKING);
	pca9532_setLeds(0xFFFF, 0xFFFF);
	Timer0_Wait(200);
	pca9532_setLeds(0x0, 0xFFFF);
}

void sendEmergencyRequest(void) {
	UART_Send(LPC_UART3, (uint8_t *) "*EMERGENCY!*\r\n", 14, BLOCKING);
	pca9532_setBlink0Period(151);
	pca9532_setBlink0Leds(0xFFFF);
}

void sendCemsMessages(void) {
	char str[37] = "";
	sprintf(str, "%03d_-_T%.1f_L%d_AX%d_AY%d_AZ%d\r\n", NNN++,
			temperatureReading / 10.0, lightReading, xReading, yReading,
			zReading);

	if (fireAlert == 1) {
		UART_Send(LPC_UART3, fireMsg, fireMsgLen, BLOCKING);
	}

	if (moveInDarkAlert == 1) {
		UART_Send(LPC_UART3, darknessMsg, darknessMsgLen, BLOCKING);
	}

	UART_Send(LPC_UART3, (uint8_t *) str, strlen(str), BLOCKING);

	sendCemsFlag = 0;
}

/*****************************************************************************/
/********************** Additional Helper Functions ************************/
/*****************************************************************************/
static uint32_t getMsTick(void) {
	return msTicks;
}

static void prepareStableState(void) {
	oled_clearScreen(OLED_COLOR_BLACK);	//Clear the OLED display
	led7seg_setChar(' ', FALSE);		//Blank off the LED 7 Segment display
	segNum = 0; 						//Reset the segnum count to 0
	fireAlert = 0; 						//Clears the "fire alert" flag
	ritInterruptEnabledFlag = 0; 		//Turn off the RGB led
	moveInDarkAlert = 0;				//Clears the "moving in dark" flag
	lightLowWarning = 0;				//Clears the "low light warning" flag
	offBlueLed();
	offRedLed();
	TIM_Cmd(LPC_TIM1, DISABLE); 		//Disables timer for 7Seg
	TIM_ResetCounter(LPC_TIM1 );		//Resets the counter timer for 7seg
	NVIC_DisableIRQ(RIT_IRQn);			//Disables the RGB
	//used for 2nd screen
	oledStatus = STABLE;
	oledUpdatedFlag = 0;			//Clears the "Oled has been updated" flag
	pca9532_setLeds(0x0, 0xFFFF);
}

static void prepareMonitorState(void) {
	UART_Send(LPC_UART3, monitorMsg, monitorMsgLen, BLOCKING);
	sendHelpMsgFlag = 0;
	cancelOptionFlag = 0;
	TIM_Cmd(LPC_TIM1, ENABLE);
	prepareMonitorReadingsOled();
	led7seg_setChar(invertedChars[0], TRUE);
	updateSensors();
	setLightThreshold();
	oledStatus = MONITOR_READINGS;
}

/*****************************************************************************/
/**************************** Initializers ***********************************/
/*****************************************************************************/
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

	// Initialize P0.2
	PinCfg.Funcnum = 0;
	PinCfg.Pinnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1 << 2), 0);
	LPC_GPIOINT ->IO0IntClr |= (1 << 2);
	// Enable GPIO interrupt for temperature
	LPC_GPIOINT->IO0IntEnF |= (1 << 2);
	LPC_GPIOINT->IO0IntEnR |= (1 << 2);

}

static void init_uart(void) {
	UART_CFG_Type uartCfg;
	uartCfg.Baud_rate = 115200;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;
	//pin select for uart3;
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	//supply power & setup working parameters for uart3
	UART_Init(LPC_UART3, &uartCfg);
	//enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);
}

static void initRit(void) {
	RIT_Init(LPC_RIT );
	RIT_TimerClearCmd(LPC_RIT, ENABLE);
}

static void initTimer1Interrupt(void) {
	TIM_MATCHCFG_Type timMatchCfg;
	timMatchCfg.MatchChannel = 0;
	timMatchCfg.IntOnMatch = 1;
	timMatchCfg.StopOnMatch = 0;
	timMatchCfg.ResetOnMatch = 1;
	timMatchCfg.ExtMatchOutputType = 0;
	timMatchCfg.MatchValue = 1000;
	TIM_ConfigMatch(LPC_TIM1, &timMatchCfg);

	TIM_TIMERCFG_Type timTimerCfg;
	timTimerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
	timTimerCfg.PrescaleValue = 1000;
	TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timTimerCfg);

	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_SetPriority(TIMER1_IRQn, NVIC_EncodePriority(5, 0, 0));
	NVIC_EnableIRQ(TIMER1_IRQn);
}

static void initTimer0Interrupt(void) {
	NVIC_SetPriority(TIMER0_IRQn, NVIC_EncodePriority(5, 2, 0));
}

static void initEint0Interrupt(void) {
	LPC_SC ->EXTINT = 1;
	LPC_SC ->EXTMODE = 1;
	LPC_SC ->EXTPOLAR = 0;
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_SetPriority(EINT0_IRQn, NVIC_EncodePriority(5, 1, 1));
	NVIC_EnableIRQ(EINT0_IRQn); // Enable EINT0 interrupt
}

static void initEint3Interrupt(void) {
	NVIC_ClearPendingIRQ(EINT3_IRQn);
	NVIC_SetPriority(EINT3_IRQn, NVIC_EncodePriority(5, 1, 0));
	NVIC_EnableIRQ(EINT3_IRQn); // Enable EINT3 interrupt
}

static void initBoardPosition(void) {
	acc_read(&xReading, &yReading, &zReading);
	xoff = 0 - xReading;
	yoff = 0 - yReading;
	zoff = 64 - zReading;
}

static void initAll(void) {
	init_i2c();
	init_ssp();
	init_GPIO();
	pca9532_init();
	joystick_init();
	acc_init();
	oled_init();
	led7seg_init();
	rgb_init();
	init_uart();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_clearIrqStatus();
	setLightThreshold();
	initRit();
	initTimer1Interrupt();
	initBoardPosition();
	initEint0Interrupt();
	initTimer0Interrupt();
	initEint3Interrupt();
}

/*****************************************************************************/
/******************************* Handlers ************************************/
/*****************************************************************************/
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
	sendHelpMsgFlag = 1;
	LPC_SC ->EXTINT = 1;
	NVIC_ClearPendingIRQ(EINT0_IRQn);
}

void EINT3_IRQHandler(void) {
	if (((LPC_GPIOINT->IO2IntStatF >> 5) & 0x1)) {
		if (lightLowWarning == 0) {
			lightLowWarning = 1;
			light_setLoThreshold(0);
			light_setHiThreshold(51);
		} else if (lightLowWarning == 1) {
			lightLowWarning = 0;
			light_setHiThreshold(3891);
			light_setLoThreshold(50);
		}
		light_clearIrqStatus();
		LPC_GPIOINT ->IO2IntClr |= (1 << 5);
	}

	if (((LPC_GPIOINT->IO0IntStatF >> 2) & 0x1) ||
			((LPC_GPIOINT->IO0IntStatR >> 2) & 0x1)) {
        if (temp_t1 == 0 && temp_t2 == 0) {
            temp_t1 = getMsTick();
        } else if (temp_t1 != 0 && temp_t2 == 0) {
            temp_count++;
            if (temp_count == TEMP_HALF_PERIODS) {
                temp_t2 = getMsTick();
                if (temp_t2 > temp_t1) {
                    temp_t2 = temp_t2 - temp_t1;
                }
                else {
                	temp_t2 = (0xFFFFFFFF - temp_t1 + 1) + temp_t2;
                }
                temperatureReading = ((2*1000*temp_t2) / (TEMP_HALF_PERIODS*TEMP_SCALAR_DIV10) - 2731 );
                temp_t2 = 0;
                temp_t1 = 0;
                temp_count = 0;
            }
        }
        LPC_GPIOINT ->IO0IntClr |= (1 << 2);
    }
//	NVIC_ClearPendingIRQ(EINT3_IRQn);
}

void TIMER1_IRQHandler(void) {
	if (LPC_TIM1 ->IR & (1 << 0)) {
		segNum = (++segNum) % 16;
		led7seg_setChar(invertedChars[segNum], TRUE);
		if (segNum == 5 || segNum == 10 || segNum == 15) {
			updateOledFlag = 1;
			if (segNum == 15) {
				sendCemsFlag = 1;
			}
		}
		TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
		NVIC_ClearPendingIRQ(TIMER1_IRQn);
	}
}

int main(void) {

	SysTick_Config(SystemCoreClock / 1000);  // every 1ms

	uint8_t sw4 = 1;
	uint8_t joystickStatus = 0;
	int sw4HoldStatus = 0;
	int joystickHold = 0;

	initAll();

	while (1) {

		prepareStableState();

		while (cuteStatus == STABLE_STATE) {

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

			if (sw4 == 0 && sw4HoldStatus == 0) {
				cuteStatus = MONITOR_STATE;
				prepareMonitorState();
				sw4HoldStatus = 1;
			}
		}

		while (cuteStatus == MONITOR_STATE) {

				if (updateOledFlag == 1) {
					updateLightSensor();
					updateAccSensor();
					if (oledStatus == MONITOR_READINGS) {
						updateOledReadings();
					}
					oledUpdatedFlag = 1;
					updateOledFlag = 0;
				}

			if (fireAlert == 0 && temperatureReading > TEMPERATURE_THRESHOLD) {
				fireAlert = 1;
				if (ritInterruptEnabledFlag == 0) {
					enableRitRGBinterrupt();
					ritInterruptEnabledFlag = 1;
				}
			}

			if (moveInDarkAlert == 0 && lightLowWarning == 1) {
				updateAccSensor();
				if (checkForMovement()) {
					moveInDarkAlert = 1;
					if (ritInterruptEnabledFlag == 0) {
						enableRitRGBinterrupt();
						ritInterruptEnabledFlag = 1;
					}
				}
			}

			joystickStatus = joystick_read();
			if (joystickStatus == 0) {
				joystickHold = 0;
			}

			if ((joystickHold == 0)
					&& (joystickStatus == JOYSTICK_RIGHT
							|| joystickStatus == JOYSTICK_LEFT)) {
				if (oledStatus == MONITOR_READINGS) {
					switchToMonitorOptions();
					oledStatus = MONITOR_OPTIONS;
				} else if (oledStatus == MONITOR_OPTIONS) {
					switchToMonitorReadings();
					oledStatus = MONITOR_READINGS;
				}
				joystickHold = 1;
			}

			if (oledStatus == MONITOR_OPTIONS) {
				if (joystickHold == 0 && joystickStatus == JOYSTICK_UP) {
					toggleToRequest();
					joystickHold = 1;
				} else if (joystickHold == 0 && joystickStatus == JOYSTICK_DOWN) {
					toggleToCancel();
					joystickHold = 1;
				}
				if (joystickHold == 0 && joystickStatus == JOYSTICK_CENTER) {
					if (cancelOptionFlag == 0) {
						sendHelpRequest();
					} else if (cancelOptionFlag == 1) {
						sendCancelLastRequest();
					}
					joystickHold = 1;
				}

			}

			if (sendHelpMsgFlag == 1) {
				sendEmergencyRequest();
				sendHelpMsgFlag = 0;
			}

			sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;

			if (sw4 == 0 && sw4HoldStatus == 0) {
				cuteStatus = STABLE_STATE;
				sw4HoldStatus = 1;
				RIT_Cmd(LPC_RIT, DISABLE);
			}

			if (sw4 == 1) {
				sw4HoldStatus = 0;
			}

			if (sendCemsFlag == 1) {
				sendCemsMessages();
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
