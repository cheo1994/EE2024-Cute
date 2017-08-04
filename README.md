# Project CUTE - Care Unit for The Elderly
EE2024: Programming for Computer Interfaces

### Team
Teh Chee Yeo & Kelvin Ng

### Project Objective 
Expose students to programming for embedded devices.

### Description 
In this project, we were exposed to programming hardware components such as several push buttons, light sensors, a temperature sensor, an accelerometer, UART, 7-segment display and a LED display. 

### Tools used
* LPCXpresso Base Board
* LPC1769
* C Language (Main language used with minor exposure to Assembly language)
* GitHub
* Source Tree
* Eclipse Mars



| Device Components  | Requirements							     							|
| -------------------|:--------------------------------------------------------------------:|	
| Light sensor       | Detect low light environment below 50 lux     						|
| Temperature sensor | Detect temperatures above 35 degrees celcius  						|
| UART               | Send an alert via UART terminal if temperature or light alarm reached|
| Accelerometer      | Detect if user has fallen down 										|
| Push buttons		 | Enable user to turn on and off device/components 					|
| OLED     			 | Display light, temperature & accelerometer readings 					| 
| 7-segment led 	 | Timer from 0 to 15 													|
| 16 mini leds 		 | Indicate to completion of an action 									|


### Learning Outcomes
We learnt that using interrupts made the system more _sensitive_ to changes and more _responsive_. However, this responsive systems came with a trade off of _reliability_ when the system is subjected to rigourous conditions which triggers several interrupts at once. 
Doing a periodic polling to check for a interrupt makes the system more reliable and consistent but may miss cases where the system should have responded.
