// Header
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "mcp3008.h"

// Define standards
#define TRUE	(1==1)
#define FALSE	(!TRUE)
#define UNDEF	-1

// Bike specific things
#define BIKE_ETRIKE 	0
#define BIKE_EBIKE 		1
#define BIKE_CONVBIKE	2

// Define states
#define	STATE_IDLE 			0
#define	STATE_LOG			1
#define STATE_FILETRANSFER	2
#define STATE_SHUTDOWN		3

// Define GPIO pins (BCM)
#define PIN_LED_GREEN		13
#define PIN_LED_RED			19
#define PIN_LED_BTN_LOG		6
#define PIN_BTN_LOG			12
#define PIN_BTN_SHUTDOWN	16
#define PIN_ADC_CE0_IN		8
#define PIN_ADC_MOSI		10
#define PIN_ADC_MISO		9
#define PIN_ADC_SCLK		11

// Define ADC channels
#define ADC_CHAN_POT		0
#define ADC_CHAN_DCM		1

// Variables
int state, prev_state, line_index;
int bike = BIKE_ETRIKE;
int file_transfer_done = FALSE;
struct timeval tval_before, tval_after, tval_result;
FILE *file;

// Main method
int main(void)
{
	// Start in idle state
	prev_state = UNDEF;
	state = STATE_IDLE;
	
	// Setup the Pi GPIO
	setupGPIO();
	
	// Main loop
	for(;;)
	{
		
		// State machine
		switch(state)
		{
			case STATE_IDLE:
printf("In Idle state.\n");
				// At state change
				if(state != prev_state)
				{
					// Turn off red LED and turn on green LED
					digitalWrite(PIN_LED_RED, LOW);
					digitalWrite(PIN_LED_GREEN, HIGH);
					
					// Reset previous state
					prev_state = state;
				}
				// Check if data log button is pressed
				else if(digitalRead(PIN_BTN_LOG) == 0)
				{
					// Change to logging state
					state = STATE_LOG;
printf("Switching to logging state!\n");					
					// Wait 2 s for user to release button
					delay(2000);
				}
				// Check if shutdown button is pressed
				else if(digitalRead(PIN_BTN_SHUTDOWN) == 0)
				{
					state = STATE_SHUTDOWN;
				}
				// Check if USB stick is plugged in and data is yet unsaved
				else if(opendir("/media/usb/data") && !file_transfer_done)
				{
					state = STATE_FILETRANSFER;
				}
				else
				{
					// By default, delay 100 ms
					delay(100);
				}
				
				break;
			
			case STATE_LOG:
				// At state change
				if(state != prev_state)
				{
printf("Logging just started...\n");
					// Turn off red LED and turn on green LED
					digitalWrite(PIN_LED_GREEN, LOW);
					digitalWrite(PIN_LED_RED, HIGH);
					
					// Init logging
					createLogFile();
printf("Log file created!\n");					
					// Set previous state
					prev_state = state;
				}
				// Check if log button was pressed again
				else if(digitalRead(PIN_BTN_LOG) == 0)
				{
					// End log file
					endLogFile();
					
					// Change back to idle state
					state = STATE_IDLE;
					
					// Mark file as unsafed
					file_transfer_done = FALSE;
					
					// Wait 2 s for user to release button
					delay(2000);
				}
				
				// Log data
				dataLog();
				
				// Delay 10 ms to achieve 100 Hz sampling frequency -> TO IMPROVE!
				delay(10);
				break;
			
			case STATE_FILETRANSFER:
				// Turn on also red LED
				digitalWrite(PIN_LED_RED, HIGH);
printf("File transfer start!\n");
				// Copy/Move folder from /home/pi/trike/data/ to /media/usb/data
				moveDataToUSB();
printf("File transfer done!\n");
				// Turn off red LED
				digitalWrite(PIN_LED_RED, LOW);

				// Mark transfer as done
				file_transfer_done = TRUE;
				
				// Change back to idle
				state = STATE_IDLE;
				break;
			
			case STATE_SHUTDOWN:
printf("Going to shutdown!\n");
digitalWrite(PIN_LED_RED, HIGH);
delay(250);
digitalWrite(PIN_LED_RED, LOW);
delay(250);
digitalWrite(PIN_LED_RED, HIGH);
delay(250);
digitalWrite(PIN_LED_RED, LOW);
delay(250);
digitalWrite(PIN_LED_RED, HIGH);

				// Shutdown Pi
				system("sudo shutdown -h now");
				break;
				
			default:
				break;
		}
	}
}

int createLogFile(void)
{
	// Create filename /home/pi/data/YYYY-MM-DD HH:mm.csv
	char filename[64];
	time_t now;
	time(&now);
	struct tm* now_tm;
	now_tm = localtime(&now);

	strftime(filename, 64, "/home/pi/data/%Y-%m-%d_%H-%M.csv", now_tm);
	
	// Open file (create if not existing, otherwise truncate)
	file = fopen(filename, "w");
	
	// Write csv header
	fprintf(file, "Time [s],Steer-angle [deg],Speed [V],Acc-X [g],Acc-Y [g],Acc-Z [g],Rollrate-X [deg/s],Rollrate-Y [deg/s],Rollrate-Z [deg/s]\n");

	// Reset line index
	line_index = 0;
	// Setup ADC
	spiSetup(0);
}
int endLogFile(void)
{
	// Close csv file
	fclose(file);
}

int dataLog(void)
{
	// Log time [ms]
	if(line_index == 0)
	{
		// First entry is 0.000000 [s]
		fprintf(file, "0.000000,");

		// Start time
		gettimeofday(&tval_before, NULL);
	}
	else
	{
		// Measure new time and calculate elapsed time
		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_result);

		fprintf(file, "%ld.%06ld,", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
	}

	// Log data from ADC
	logADCData(file);

	// Log data from IMU
	fprintf(file, "0,0,0,0,0,0\n");

	// Increment line index
	line_index++;
}

int setupGPIO(int gpioPin)
{
	// Setup wiringPi GPIO
	wiringPiSetupGpio();
	
	// Assign output pins
	pinMode(PIN_LED_RED, OUTPUT);
	pinMode(PIN_LED_GREEN, OUTPUT);
	pinMode(PIN_LED_BTN_LOG, OUTPUT);
	
	// Assign input pins 
	pinMode(PIN_BTN_LOG, INPUT);
	pullUpDnControl(PIN_BTN_LOG, PUD_UP);
	pinMode(PIN_BTN_SHUTDOWN, INPUT);
	pullUpDnControl(PIN_BTN_SHUTDOWN, PUD_UP);

	return 0;
}

int moveDataToUSB()
{
	switch(bike)
	{
		case BIKE_ETRIKE:
			system("sudo mv /home/pi/data/*.csv /media/usb/data/etrike/");
			break;
			
		case BIKE_EBIKE:
			system("sudo mv /home/pi/data/*.csv /media/usb/data/ebike/");
			break;
			
		case BIKE_CONVBIKE:
			system("sudo mv /home/pi/data/*.csv /media/usb/data/convbike/");
			break;
			
		default:
			break;
	}

	return 0;
}
