/*
 *      dht22.c:
 *	Simple test program to test the wiringPi functions
 *	Based on the existing dht11.c
 *	Amended by technion@lolware.net
 *  Extensions by oliverschneider+sweetpi@gmail.com
 */

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "locking.h"

#define MAXTIMINGS 85
static int dht22_dat[5] = {0,0,0,0,0};

static uint8_t sizecvt(const int read)
{
  /* digitalRead() and friends from wiringpi are defined as returning a value
  < 256. However, they are returned as int() types. This is a safety function */

  if (read > 255 || read < 0)
  {
    printf("Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }
  return (uint8_t)read;
}

static int read_dht22_dat(const int pin)
{
  uint8_t laststate = HIGH;
  uint8_t counter = 0;
  uint8_t j = 0, i;

  dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

  // pull pin down for 18 milliseconds
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(18);
  // then pull it up for 40 microseconds
  digitalWrite(pin, HIGH);
  delayMicroseconds(40); 
  // prepare to read the pin
  pinMode(pin, INPUT);

  // detect change and read data
  for ( i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while (sizecvt(digitalRead(pin)) == laststate) {
      counter++;
      delayMicroseconds(1);
      if (counter == 255) {
        break;
      }
    }
    laststate = sizecvt(digitalRead(pin));

    if (counter == 255) break;

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      dht22_dat[j/8] <<= 1;
      if (counter > 16)
        dht22_dat[j/8] |= 1;
      j++;
    }
  }

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
  if ((j >= 40) && 
      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float t, h;
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t /= 10.0;
        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;


    printf("Pin = %i, Humidity = %.1f %%, Temperature = %.1f °C\n", pin, h, t );
    return 1;
  }
  else
  {
    printf("Data not good, skip\n");
    return 0;
  }
}

int main (int argc, char** argv)
{
  int lockfd;

  if(argc == 1) {
    printf("usage rpi_dht pin1 [pin2...]\n");
    exit(EXIT_FAILURE) ;
  }

  int pins[argc-1];
  for(int i = 0; i < argc-1; i++)
  {
    int pin = atoi(argv[i+1]);
    if(pin < 0 || pin > 20)
    {
      printf("Invalid pin given: %i\n", pin);
      exit(EXIT_FAILURE);
    }
    pins[i] = pin;
  }

  lockfd = open_lockfile(LOCKFILE);

  if (wiringPiSetup () == -1)
    exit(EXIT_FAILURE) ;
	
  if (setuid(getuid()) < 0)
  {
    perror("Dropping privileges failed\n");
    exit(EXIT_FAILURE);
  }

  for(int i = 0; i < sizeof(pins)/sizeof(pins[0]); i++)
  {
    while (read_dht22_dat(pins[i]) == 0) 
    {
       delay(1000); // wait 1sec to refresh
    }
  }


  delay(1500);
  close_lockfile(lockfd);

  return 0 ;
}
