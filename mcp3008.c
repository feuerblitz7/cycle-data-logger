/*
 *  readMcp3008.c:
 *  read value from ADC MCP3008
 *
 * Requires: wiringPi (http://wiringpi.com)
 * Copyright (c) 2015 http://shaunsbennett.com/piblog
 ***********************************************************************
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#define	TRUE	            (1==1)
#define	FALSE	            (!TRUE)
#define CHAN_CONFIG_SINGLE  8
#define CHAN_CONFIG_DIFF    0


char *usage = "Usage: mcp3008 all|analogChannel[1-8] [-l] [-ce1] [-d]";
// -l   = load SPI driver,  default: do not load
// -ce1  = spi analogChannel 1, default:  0
// -d   = differential analogChannel input, default: single ended

void loadSpiDriver()
{
    if (system("gpio load spi") == -1)
    {
        printf ("Can't load the SPI driver: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}

void spiSetup (int spiChannel)
{
    if ((wiringPiSPISetup (spiChannel, 1000000)) < 0)
    {
        printf ("Can't open the SPI bus: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}

int myAnalogRead(int spiChannel,int channelConfig,int analogChannel)
{
    if(analogChannel<0 || analogChannel>7)
        return -1;
    unsigned char buffer[3] = {1}; // start bit
    buffer[1] = (channelConfig+analogChannel) << 4;
    wiringPiSPIDataRW(spiChannel, buffer, 3);
    return ( (buffer[1] & 3 ) << 8 ) + buffer[2]; // get last 10 bits
}

void logADCData(FILE *file)
{
    float data[] = {0, 0};

    int loadSpi = FALSE;
    int analogChannel = 0;
    int spiChannel = 0;
    int channelConfig = CHAN_CONFIG_SINGLE;
    
    if(loadSpi == TRUE)
        loadSpiDriver();

	//data[0] = 280.0f * ((float)myAnalogRead(spiChannel, channelConfig, 0) / 1023.0f);
	//data[1] = 3.0f * ((float)myAnalogRead(spiChannel, channelConfig, 1) / 1023.0f);

	//fprintf(file, "%f,%f,", data[0], data[1]);
	fprintf(file, "%d,%d,", myAnalogRead(spiChannel, channelConfig, 0), myAnalogRead(spiChannel, channelConfig, 1));
}
