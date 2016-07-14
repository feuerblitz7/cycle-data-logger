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

//static int myFd ;

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

//double[] getData (int argc, char *argv [])
//int logADCData (FILE *file)
void logADCData(FILE *file)
{
    float data[] = {0, 0};

    int loadSpi=FALSE;
    int analogChannel=0;
    int spiChannel=0;
    int channelConfig=CHAN_CONFIG_SINGLE;
    /*if (argc < 2)
    {
        printf("%s\n", usage) ;
        return 1 ;
    }
    if((strcasecmp (argv [1], "all") == 0) )
        argv[1] = "0";
    if ( (sscanf (argv[1], "%i", &analogChannel)!=1) || analogChannel < 0 || analogChannel > 8 )
    {
        printf ("%s\n",  usage) ;
        return 1 ;
    }
    int i;
    for(i=2; i<argc; i++)
    {
        if (strcasecmp (argv [i], "-l") == 0 || strcasecmp (argv [i], "-load") == 0)
            loadSpi=TRUE;
        else if (strcasecmp (argv [i], "-ce1") == 0)
            spiChannel=1;
        else if (strcasecmp (argv [i], "-d") == 0 || strcasecmp (argv [i], "-diff") == 0)
            channelConfig=CHAN_CONFIG_DIFF;
    }*/
    //
    
    if(loadSpi==TRUE)
        loadSpiDriver();
    //wiringPiSetup () ;
    //spiSetup(spiChannel);
    
    //
    /*if(analogChannel>0)
    {
        printf("MCP3008(CE%d,%s): analogChannel %d = %d\n",spiChannel,(channelConfig==CHAN_CONFIG_SINGLE)
               ?"single-ended":"differential",analogChannel,myAnalogRead(spiChannel,channelConfig,analogChannel-1));
    }
    else
    {
        for(i=0; i<8; i++)
        {
            printf("MCP3008(CE%d,%s): analogChannel %d = %d\n",spiChannel,(channelConfig==CHAN_CONFIG_SINGLE)
                   ?"single-ended":"differential",i+1,myAnalogRead(spiChannel,channelConfig,i));
			
		}
    }*/
	data[0] = 280.0f * ((float)myAnalogRead(spiChannel, channelConfig, 0) / 1023.0f);
	data[1] = 3.0f * ((float)myAnalogRead(spiChannel, channelConfig, 1) / 1023.0f);

	fprintf(file, "%f,%f,", data[0], data[1]);

    //close (myFd) ;
    //return output;
}
