// Header
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <phidget21.h>
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

// Timing
#define DELAY_MS		8	// Time to wait after sampling
#define DELAY_IDLE		100
#define DELAY_BTN		1000

// Variables
int state, prev_state, line_index;
int bike = BIKE_ETRIKE;
int file_transfer_done = FALSE;

struct timeval tval_before, tval_after, tval_result;

FILE *file;

CPhidgetHandle spatial;
int initSpatial = FALSE;

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
					delay(DELAY_BTN);
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
					delay(DELAY_IDLE);
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
					delay(DELAY_BTN);
				}

				delay(DELAY_MS);
				break;

			case STATE_FILETRANSFER:
				// Turn on also red LED
				digitalWrite(PIN_LED_RED, HIGH);
				
				printf("File transfer start.\n");
				
				// Copy/Move folder from /home/pi/trike/data/ to /media/usb/data
				moveDataToUSB();
				
				printf("File transfer done.\n");
				
				// Turn off red LED
				digitalWrite(PIN_LED_RED, LOW);

				// Mark transfer as done
				file_transfer_done = TRUE;

				// Change back to idle
				state = STATE_IDLE;
				break;

			case STATE_SHUTDOWN:
				// Blink 3 times as sign for shutdown
				
				printf("Going to shutdown...\n");
				
				digitalWrite(PIN_LED_RED, HIGH);
				delay(250);
				digitalWrite(PIN_LED_RED, LOW);
				delay(250);
				digitalWrite(PIN_LED_RED, HIGH);
				delay(250);
				digitalWrite(PIN_LED_RED, LOW);
				delay(250);
				digitalWrite(PIN_LED_RED, HIGH);
				
				closeSpatialIMU();

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
	fprintf(file, "Time [s],Steer-angle [LSB],Speed [LSB],Acc-X [g],Acc-Y [g],Acc-Z [g],Rollrate-X [deg/s],Rollrate-Y [deg/s],Rollrate-Z [deg/s]\n");

	// Reset line index
	line_index = 0;

	// Setup ADC
	spiSetup(0);

	// Setup IMU
	if(!initSpatial)
	{
		initSpatialIMU();
		initSpatial = TRUE;
	}
}

int endLogFile(void)
{
	// Close spatial IMU
	//closeSpatialIMU();

	// Close csv file
	fclose(file);
}

int setupGPIO()
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
	// Count number of files in data
	int file_count = 0;
	DIR *dirp;
	struct dirent *entry;
	dirp = opendir("/home/pi/data");
	while((entry = readdir(dirp)) != NULL)
	{
		if(entry->d_type == DT_REG)
		{
			file_count++;
		}
	}

	// Check if files available for transfer
	if(file_count > 0)
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
		
		printf("%d files have been moved.\n", file_count);
	}

	return 0;
}


//callback that will run if the Spatial is attached to the computer*/
int CCONV AttachHandler(CPhidgetHandle spatial, void *userptr)
{
	int serialNo;
	CPhidget_getSerialNumber(spatial, &serialNo);
	printf("Spatial %10d attached!", serialNo);

	return 0;
}

//callback that will run if the Spatial is detached from the computer
int CCONV DetachHandler(CPhidgetHandle spatial, void *userptr)
{
	int serialNo;
	CPhidget_getSerialNumber(spatial, &serialNo);
	printf("Spatial %10d detached! \n", serialNo);

	return 0;
}

//callback that will run if the Spatial generates an error
int CCONV ErrorHandler(CPhidgetHandle spatial, void *userptr, int ErrorCode, const char *unknown)
{
	printf("Error handled. %d - %s \n", ErrorCode, unknown);
	return 0;
}

int CCONV SpatialDataHandler(CPhidgetSpatialHandle spatial, void *userptr, CPhidgetSpatial_SpatialEventDataHandle *data, int count)
{
	if(state == STATE_IDLE)
	{
		return 0;
	}

	
	// Log time [ms], jump over first samples
	if(line_index == 10)
	{
		// First entry is 0.000000 [s]
		fprintf(file, "0.000000,");

		// Start time
		gettimeofday(&tval_before, NULL);
	}
	else if(line_index > 10)
	{
		// Measure new time and calculate elapsed time
		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_result);

		fprintf(file, "%ld.%06ld,", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
	}

	if(line_index >= 10)
	{
		// Log data from ADC
		logADCData(file);

		// Log data from IMU
		int i;
		for(i = 0; i < count; i++)
		{
			fprintf(file, "%6f,%6f,%6f,%6f,%6f,%6f\n", data[i]->acceleration[0], data[i]->acceleration[1], data[i]->acceleration[2], data[i]->angularRate[0], data[i]->angularRate[1], data[i]->angularRate[2]);
		}
	}

	// Increment line index
	line_index++;

	return 0;
}

int initSpatialIMU()
{
	int result;
	const char *err;

	//Declare a spatial handle
	CPhidgetSpatialHandle spatial = 0;

	//create the spatial object
	CPhidgetSpatial_create(&spatial);

	//Set the handlers to be run when the device is plugged in or opened from software, unplugged or closed from software, or generates an error.
	CPhidget_set_OnAttach_Handler((CPhidgetHandle)spatial, AttachHandler, NULL);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)spatial, DetachHandler, NULL);
	CPhidget_set_OnError_Handler((CPhidgetHandle)spatial, ErrorHandler, NULL);

	//Registers a callback that will run according to the set data rate that will return the spatial data changes
	//Requires the handle for the Spatial, the callback handler function that will be called, 
	//and an arbitrary pointer that will be supplied to the callback function (may be NULL)
	CPhidgetSpatial_set_OnSpatialData_Handler(spatial, SpatialDataHandler, NULL);

	//open the spatial object for device connections
	CPhidget_open((CPhidgetHandle)spatial, -1);

	//get the program to wait for a spatial device to be attached
	printf("Waiting for spatial to be attached.... \n");
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)spatial, 10000)))
	{
		CPhidget_getErrorDescription(result, &err);
		printf("Problem waiting for attachment: %s\n", err);
		return 0;
	}
	
	//read spatial event data
	printf("Reading.....\n");

	//Set the data rate for the spatial events
	CPhidgetSpatial_setDataRate(spatial, 8);

	//run until user input is read
	printf("Press any key to end\n");
	//getchar();

	//since user input has been read, this is a signal to terminate the program so we will close the phidget and delete the object we c
	return 0;
}

int closeSpatialIMU()
{
	CPhidget_close((CPhidgetHandle)spatial);
	CPhidget_delete((CPhidgetHandle)spatial);

	return 0;
}

