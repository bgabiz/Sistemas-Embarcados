#include <stdio.h>
#include <stdlib.h>
#include "gpio_sysfs.h"
#include <unistd.h>
#include "gpio_dev_mem.h"
#include <bcm2835.h>
#include <time.h>


// Input on RPi pin GPIO 15
#define PIN RPI_GPIO_P1_15
#define MAXSTR 64
#define VALID_PINS 0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25



int setGPIO_In(int pin)
{	
  /* This is a function to export a GPIO pin - and set it's direction to input... */

  /* Start with a test to see if we built in debug...
     ...if we did, then show some more verbose descriptions of what we're doing. */
#ifdef debug
  printf("Exporting pin %d for input\n", pin);
#endif

  int valid_pins[]={VALID_PINS};
  int c;
  int valid = 0;

  /* Before we do anyting else, we need to check to see if the pin number passed in
   * is a valid pin or not - we'll compare it against the pins defined in the list of
   * valid pins */
  for (c=0;c<(sizeof(valid_pins) / sizeof(int));c++)
    {
      if(pin == valid_pins[c])
	valid = 1;
    }

  if(!valid)
    {
      fprintf(stderr, "ERROR: Invalid pin!\nPin %d is not a GPIO pin...\n", pin);
      return -1;
    }

  FILE *sysfs_handle = NULL;

  /* Now we define a file handle, for the 'gpio/export' sysfs file & try to open it
   * Note that we open for writing, and as a binary file*/
  if ((sysfs_handle = fopen("/sys/class/gpio/export", "w")) == NULL)
    {
      fprintf(stderr, "ERROR: Cannot open GPIO export...\n(Is this program running as root?)\n");
      return 1;
    }

  /* Next we need to convert our int pin value to a string... The safest way is to use snprintf
   * Note that the length is n+1 as C strings are nul-terminated - so for a two-digit value - we
   * need to specify 3 chars to be used. */
  char str_pin[3];
  snprintf(str_pin, (3*sizeof(char)), "%d", pin);	

  /* To actually export the pin - we simply write the string value of the pin number to the
   * sysfs gpio/export file */
  if (fwrite(&str_pin, sizeof(char), 3, sysfs_handle)!=3)
    {
      fprintf(stderr, "ERROR: Unable to export GPIO pin %d\n", pin);
      return 2;
    }
  fclose(sysfs_handle);

  /* If we got to here, then we've been able to export the pin - so now we need to set the direction */
  /* We open the direction file for the pin...*/
  char str_direction_file[MAXSTR];
  snprintf(str_direction_file, (MAXSTR*sizeof(char)), "/sys/class/gpio/gpio%d/direction", pin);
  if ((sysfs_handle = fopen(str_direction_file, "w")) == NULL)
    {
      fprintf(stderr, "ERROR: Cannot open direction file...\n");
      return 3;
    }

  /* ...and then we'll write "out" to the direction file.*/
  if (fwrite("in", sizeof(char), 3, sysfs_handle) != 3)
    {

      fprintf(stderr, "ERROR: Unable to write direction for GPIO%d\n", pin);
      return 4;
    }
  fclose(sysfs_handle);

  // If everything worked, we'll return 0 - an non-zero return value signifies something went wrong
  return 0;
}

int GPIO_Read(int pin) {
  /* This function will read a value (0 or 1) from the selected GPIO pin */

  /* Once again, if we built in debug mode - then show verbose output*/
#ifdef debug
  printf("Reading value from GPIO pin %d\n", pin);
#endif

  /* Now we'll try to open the sysfs value file for the pin
   * We'll start by building the path - using snprintf... */
  FILE *sysfs_handle = NULL;
  char str_value_file[MAXSTR];

  snprintf (str_value_file, (MAXSTR*sizeof(char)), "/sys/class/gpio/gpio%d/value", pin);

  if ((sysfs_handle = fopen(str_value_file, "r")) == NULL)
    {
      fprintf(stderr, "ERROR: Cannot open value file for pin %d...\n(Has the pin been exported?)\n", pin);
      return -1;
    }

  /* If the file is good - then we read the value. */

  // character string to read the file contents into
  char str_val[2];

  // copy the file into the buffer:
  if ((fread (str_val, sizeof(char), 2, sysfs_handle)) != 2) {
    fprintf(stderr, "Reading error\n");
    return -1;
  }

  fclose(sysfs_handle);
  return atoi(str_val);
}

int unsetGPIO(int pin)
{
  /* This function "turns off" the specified pin - and unexports it. */

  /* If we built in debug - then produce verbose output */
#ifdef debug
  printf("Unxporting pin %d\n", pin);
#endif

  /* We start by building the string for the sysfs value file for the pin - to set it to 0*/
  FILE *sysfs_handle = NULL;
  char str_pin[3];
  char str_value_file[MAXSTR];

  snprintf (str_pin, (3*sizeof(char)), "%d", pin);
  snprintf (str_value_file, (MAXSTR*sizeof(char)), "/sys/class/gpio/gpio%d/value", pin);

  if ((sysfs_handle = fopen(str_value_file, "w")) == NULL)
    {
      fprintf(stderr, "ERROR: Cannot open value file for pin %d...\n", pin);
      return 1;
    }

  if(fwrite("0", sizeof(char), 2, sysfs_handle) != 2)
    {
      fprintf(stderr, "ERROR: Cannot write to GPIO pin %d\n", pin);
      return 2;
    }
  fclose(sysfs_handle);

  /* Once we've done that - the last step is to open the gpio/unexport file - and write the pin
   * number to it - to unexport the pin. */
  if ((sysfs_handle = fopen("/sys/class/gpio/unexport", "w")) == NULL)
    {
      fprintf(stderr, "ERROR: Cannot open GPIO unexport...\n");
      return 1;
    }

  if (fwrite(&str_pin, sizeof(char), 3, sysfs_handle)!=3)
    {
      fprintf(stderr, "ERROR: Unable to unexport GPIO pin %d\n", pin);
      return 2;
    }
  fclose(sysfs_handle);
  return 0;
}
