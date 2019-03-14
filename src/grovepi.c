// GrovePi C library
// v0.1
//
// This library provides the basic functions for using the GrovePi in C
//
// The GrovePi connects the Raspberry Pi and Grove sensors.  You can learn more about GrovePi here:  http://www.dexterindustries.com/GrovePi
//
// Have a question about this example?  Ask on the forums here: http://forum.dexterindustries.com/c/grovepi
//
// 	History
// 	----------------------------------------------------------------------------------------
// 	Author		Date      		Comments
//	Karan		28 Dec 15		Initial Authoring
//  Niklas Rose 13 Mar 18       Added DHT, LCD, dust-sensor, updated readBlock function

/*
License
The MIT License (MIT)
GrovePi for the Raspberry Pi: an open source platform for connecting Grove Sensors to the Raspberry Pi.
Copyright (C) 2017  Dexter Industries
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "grovepi.h"

int fd;
char *fileName = "/dev/i2c-1";
int  address = 0x04;
unsigned char w_buf[5],ptr,r_buf[32];
unsigned long reg_addr=0;

int RBUFFER_SIZE = 32;

#define dbg 0
int init(void)
{
	if ((fd = open(fileName, O_RDWR)) < 0)
	{					// Open port for reading and writing
		printf("Failed to open i2c port\n");
		return -1;
	}

	if (ioctl(fd, I2C_SLAVE, address) < 0)
	{					// Set the port options and set the address of the device
		printf("Unable to get bus access to talk to slave\n");
		return -1;
	}
	return 1;
}
//Write a register
int write_block(char cmd,char v1,char v2,char v3)
{
	int dg;
	w_buf[0]=cmd;
    w_buf[1]=v1;
    w_buf[2]=v2;
    w_buf[3]=v3;

    dg=i2c_smbus_write_i2c_block_data(fd,1,4,w_buf);

	if (dbg)
		printf("wbk: %d\n",dg);

    // if (i2c_smbus_write_i2c_block_data(fd,1,4,w_buf) != 5)
    // {
        // printf("Error writing to GrovePi\n");
        // return -1;
    // }
    return 1;
}

//write a byte to the GrovePi
int write_byte(char b)
{
    w_buf[0]=b;
    if ((write(fd, w_buf, 1)) != 1)
    {
        printf("Error writing to GrovePi\n");
        return -1;
    }
    return 1;
}

//Read 1 byte of data
char read_byte(void)
{
	r_buf[0]=i2c_smbus_read_byte(fd);
	if (dbg)
		printf("rbt: %d\n",r_buf[0]);
	// if (read(fd, r_buf, reg_size) != reg_size) {
		// printf("Unable to read from GrovePi\n");
		// //exit(1);
        // return -1;
	// }

    return r_buf[0];
}

//NEW READ_BLOCK
uint8_t readBlock(uint8_t *data_block)
{
	int current_retry = 0;
	int output_code = 0;

	// repeat until it reads the data
	// or until it fails sending it
	while(output_code == 0 && current_retry < 5)
	{
		output_code = i2c_smbus_read_i2c_block_data(fd, 1, RBUFFER_SIZE, data_block);
		current_retry += 1;
	}

	// if the error persisted
	// after retrying for [max_i2c_retries] retries
	// then throw exception
	if(output_code == 0)
		printf("[I2CError reading block: max retries reached]\n");

	return output_code;
}


//Read a 32 byte block of data from the GrovePi
char read_block(void)
{
    int ret;
    ret=i2c_smbus_read_i2c_block_data(fd,1,32,&r_buf[0]);
	//&r_buf[0]=&ptr;
	if(dbg)
		printf("rbk: %d\n",ret);
	// if (read(fd, r_buf, reg_size) != reg_size) {
		// printf("Unable to read from GrovePi\n");
		// //exit(1);
        // return -1;
	// }

    return 1;
}


void pi_sleep(int t)
{
	usleep(t*1000);
}

// Read analog value from port
int analogRead(int port)
{
	int data;
	write_block(aRead_cmd,port,0,0);
	read_byte();
	read_block();
	data=r_buf[1]* 256 + r_buf[2];
	if (data==65535)
		return -1;
	return data;
}

//Write a digital value to a port
int digitalWrite(int port,int value)
{
	return write_block(dWrite_cmd,port,value,0);
}

//Set the mode of a port
//mode
//	1: 	output
//	0:	input
int pinMode(int port,int mode)
{
	return write_block(pMode_cmd,port,mode,0);
}

//Read a digital value from a port
int digitalRead(int port)
{
	write_block(dRead_cmd,port,0,0);
	usleep(10000);
	return read_byte();
}

//Write a PWM value to a port
int analogWrite(int port,int value)
{
	return write_block(aWrite_cmd,port,value,0);
}
// Read a Ultrasonic distance value from a port
int ultrasonicRead(int port)
{
	int data;
	write_block(uRead_cmd,port,0,0);
	usleep(60);
	read_byte();
	read_block();
	data=r_buf[1]* 256 + r_buf[2];
	if (data==65535)
		return -1;
	return data;
}

/**
 * determines the revision of the raspberry hardware
 * @return revision number
 */
static uint8_t gpioHardwareRevision()
{
	int revision = 0;
	FILE * filp = fopen("/proc/cpuinfo", "r");
	char buffer[512];
	char term;

	if(filp != NULL)
	{
		while(fgets(buffer,sizeof(buffer),filp) != NULL)
		{
			if(!strncasecmp("revision\t", buffer, 9))
			{
				if(sscanf(buffer + strlen(buffer) - 5, "%x%c", &revision, &term) == 2)
				{
					if(term == '\n')
						break;
					revision = 0;
				}
			}
		}
		fclose(filp);
	}
	return revision;
}

/**
 * determines wheter I2C is found at "/dev/i2c-0" or "/dev/i2c-1"
 * depending on the raspberry model
 *
 * @param smbus_name string to hold the filename
 *
 * hw_rev    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 * Type.1    X  X  -  -  X  -  -  X  X  X  X  X  -  -  X  X
 * Type.2    -  -  X  X  X  -  -  X  X  X  X  X  -  -  X  X
 * Type.3          X  X  X  X  X  X  X  X  X  X  X  X  X  X
 *
 * hw_rev    16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
 * Type.1    -  X  X  -  -  X  X  X  X  X  -  -  -  -  -  -
 * Type.2    -  X  X  -  -  -  X  X  X  X  -  X  X  X  X  X
 * Type.3    X  X  X  X  X  X  X  X  X  X  X  X  -  -  -  -
 *
 */
void SMBusName(char *smbus_name)
{
	unsigned int hw_revision = gpioHardwareRevision();
	unsigned int smbus_rev;

	if(hw_revision < 4)
		// type 1
		smbus_rev = 1;
	else if(hw_revision < 16)
		// type 2
		smbus_rev = 2;
	else
		// type 3
		smbus_rev = 3;

	if(smbus_rev == 2 || smbus_rev == 3)
		strcpy(smbus_name, "/dev/i2c-1");
	else
		strcpy(smbus_name, "/dev/i2c-0");
}


/**
 * for setting the address of an I2C device
 * @param address 7-bit address of the slave device
 */
int initDevice(uint8_t address)
{
	char filename[11];           // enough to hold "/dev/i2c-x"
	int current_retry = 0;
	int max_i2c_retries = 5;
	int i2c_file_device;
	SMBusName(filename);

	// try to connect for a number of times
	while(current_retry < max_i2c_retries)
	{
		// increase the counter
		current_retry += 1;

		// open port for read/write operation
		if((i2c_file_device = open(filename, O_RDWR)) < 0)
		{
			printf("[failed to open i2c port]\n");
			// try in the next loop to connect
			continue;
		}
		// setting up port options and address of the device
		if(ioctl(i2c_file_device, I2C_SLAVE, address) < 0)
		{
			printf("[unable to get bus access to talk to slave]\n");
			// try in the next loop to connect
			continue;
		}

		// if it got connected, then exit
		break;
	}

	// if connection couldn't be established
	// throw exception
	if(current_retry == max_i2c_retries)
		printf("[I2CError on opening port]\n");

	return i2c_file_device;
}

/**
 * returns via its parameters the temperature and humidity
 * this function is NaN-proof
 * it always gives "accepted" values
 *
 * if bad values are read, then it will retry reading them
 * and check if they are okay for a number of [MAX_RETRIES] times
 * before throwing a [runtime_error] exception
 *
 * @param temp     in Celsius degrees
 * @param humidity in percentage values
 */
void getSafeTemperatureAndHumidityData(float *temp, float *humidity, int port)
{
	int current_retry  = 0;
	getUnsafeTemperatureAndHumidityData(temp, humidity, port); // read data from GrovePi once

	// while values got are not okay / accepteed
	while((isnan(*temp) || isnan(*humidity) || !areTemperatureAndHumidityGoodReadings(*temp, *humidity))
	      && current_retry < MAX_RETRIES)
	{
		// reread them again
		current_retry += 1;
		getUnsafeTemperatureAndHumidityData(temp, humidity, port);
	}

	// if even after [MAX_RETRIES] attempts at getting good values
	// nothing good came, then throw one of the bottom exceptions

	if(isnan(*temp) || isnan(*humidity))
		printf("[GroveDHT NaN readings - check analog port]\n");

	if(!areTemperatureAndHumidityGoodReadings(*temp, *humidity))
		printf("[GroveDHT bad readings - check analog port]\n");
}

/**
 * function for returning via its arguments the temperature & humidity
 * it's not recommended to use this function since it might throw
 * some NaN or out-of-interval values
 *
 * use it if you come with your own implementation
 * or if you need it for some debugging
 *
 * @param temp     in Celsius degrees
 * @param humidity in percentage values
 */
void getUnsafeTemperatureAndHumidityData(float *temp, float *humidity, int port)
{
	write_block(DHT_TEMP_CMD, port, 0, 0);
	read_byte();

	uint8_t data_block[33];
	readBlock(data_block);

	*temp = fourBytesToFloat(data_block + 1);
	*humidity = fourBytesToFloat(data_block + 5);
}

void getSafeTemperatureData(float *temp, int port){
    int current_retry  = 0;
	getUnsafeTemperatureData(temp, port); // read data from GrovePi once

	// while values got are not okay / accepteed
	while((isnan(*temp) || !areTemperatureGoodReadings(*temp)) && current_retry < MAX_RETRIES)
	{
		// reread them again
		current_retry += 1;
		getUnsafeTemperatureData(temp, port);
	}

	// if even after [MAX_RETRIES] attempts at getting good values
	// nothing good came, then throw one of the bottom exceptions

	if(isnan(*temp))
		printf("[GroveDHT - Temperature NaN readings - check analog port]\n");

	if(!areTemperatureGoodReadings(*temp))
		printf("[GroveDHT - Temperature bad readings - check analog port]\n");
}
void getSafeHumidityData(float *humidity, int port){
    int current_retry  = 0;
	getUnsafeHumidityData(humidity, port); // read data from GrovePi once

	// while values got are not okay / accepteed
	while((isnan(*humidity) || !areHumidityGoodReadings(*humidity))
	      && current_retry < MAX_RETRIES)
	{
		// reread them again
		current_retry += 1;
		getUnsafeHumidityData(humidity, port);
	}

	// if even after [MAX_RETRIES] attempts at getting good values
	// nothing good came, then throw one of the bottom exceptions

	if(isnan(*humidity))
		printf("[GroveDHT - Humidity NaN readings - check port]\n");

	if(!areHumidityGoodReadings(*humidity))
		printf("[GroveDHT - Humidity bad readings - check port]\n");
}

void getUnsafeTemperatureData(float *temp, int port){
    write_block(DHT_TEMP_CMD, port, 0, 0);
	read_byte();

	uint8_t data_block[33];
	readBlock(data_block);

	*temp = fourBytesToFloat(data_block + 1);
}

void getUnsafeHumidityData(float *humidity, int port){
    write_block(DHT_TEMP_CMD, port, 0, 0);
	read_byte();

	uint8_t data_block[33];
	readBlock(data_block);

	*humidity = fourBytesToFloat(data_block + 5);
}

void getTemperature(float *temp, int port){
        getSafeTemperatureData(temp, port);
}

void getHumidity(float *humidity, int port){
    getSafeHumidityData(humidity, port);
}

void getTemperatureAndHumidity(float *temp, float *humidity, int port){
    getSafeTemperatureAndHumidityData(temp, humidity, port);
}

/**
 * function for converting 4 unsigned bytes of data
 * into a single float
 * @param  byte_data array to hold the 4 data sets
 * @return           the float converted data
 */
const float fourBytesToFloat(uint8_t *byte_data)
{
	float output;

	// so we take the 1st address &byte_data[0],
	// the 5th address &byte_data[4] (the end address)
	// and translate the output into a unsigned byte type
	memcpy(&output, byte_data, sizeof(float));

	return output;
}
/**
* function for checking the read data from the DHT-sensor.
* This means, that it checks the range of the measured data and returns true or false back
* @param measured temperature and humidity
* @return true or false if the measured data is in "normal" range
**/
const bool areTemperatureAndHumidityGoodReadings(int temp, int humidity)
{
	return (temp > -100.0 && temp < 150.0 && humidity >= 0.0 && humidity <= 100.0);
}

const bool areTemperatureGoodReadings(int temp){
    return (temp > -100.0 && temp < 150.0);
}

const bool areHumidityGoodReadings(int humidity){
    return (humidity >= 0.0 && humidity <= 100.0);
}

/**
*   RGB-LCD Functions
**/

void connectLCD()
{
	// initializing with a random address
	// it's just important to get the device file
	DEVICE_FILE = initDevice(DISPLAY_TEXT_ADDR);
}

/**
 * set rgb color
 * if there are writes errors, then it throws exception
 * @param red   8-bit
 * @param green 8-bit
 * @param blue  8-bit
 */
void setRGB(uint8_t red, uint8_t green, uint8_t blue)
{
	selectSlave(DISPLAY_RGB_ADDR);

	sendCommand(0x00, 0x00);
	sendCommand(0x01, 0x00);
	sendCommand(0x08, 0xaa);
	sendCommand(0x04, red);
	sendCommand(0x03, green);
	sendCommand(0x02, blue);
}

/**
 *  Grove RGB LCD
 *
 *     |                  Column
 * ------------------------------------------------------
 * Row | 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
 * ------------------------------------------------------
 * 1   | x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x
 * 2   | x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x
 * ------------------------------------------------------
 *
 * Whatever text is sent to the LCD via the [str] variable
 * Gets printed on the screen.
 * The limit of text is determined by the amount of available
 * Characters on the LCD screen.
 *
 * Every newline char ['\n'] takes the cursor onto the next line
 * (aka on the row no. 2) and uses that space
 * This means that if we have a text such "Hello\n World!",
 * The first 5 characters of the word "Hello" get printed on the first line,
 * whilst the rest of the string gets printed onto the 2nd line.
 * So, if you use the newline char ['\n'] before the row ends,
 * You will end up with less space to put the rest of the characters (aka the last line)
 *
 * If the text your putting occupies more than the LCD is capable of showing,
 * Than the LCD will display only the first 16x2 characters.
 *
 * @param string of maximum 32 characters excluding the NULL character.
 */
void setText(const char *str)
{
	selectSlave(DISPLAY_TEXT_ADDR);

	sendCommand(PROGRAM_MODE, CLEAR_DISPLAY);
	pi_sleep(50);
	sendCommand(PROGRAM_MODE, DISPLAY_ON | NO_CURSOR);
	sendCommand(PROGRAM_MODE, ENABLE_2ROWS);

	pi_sleep(50);

	int length = strlen(str);
	bool already_had_newline = false;
	for(int i = 0; i < length && i < MAX_NO_CHARS; i++)
	{
		if(i == 16 || str[i] == '\n')
		{
			if(!already_had_newline)
			{
				already_had_newline = true;
				sendCommand(PROGRAM_MODE, NEW_ROW);
				if(str[i] == '\n')
					continue;
			}
			else if(str[i] == '\n')
				break;
		}

		sendCommand(DISPLAY_CHAR, (uint8_t)str[i]);
	}
}

/**
 * function for sending data to GrovePi RGB LCD
 * @param mode    see the constants defined up top
 * @param command see the constants defined up top
 */
void sendCommand(uint8_t mode, uint8_t command)
{
	int error = i2c_smbus_write_byte_data(DEVICE_FILE, mode, command);

	if(error == -1)
		printf("[I2CError writing data: check LCD wirings]\n");
}

/**
 * the LCD has 2 slaves
 * 1 for the RGB backlight color
 * 1 for the actual text
 * therefore there are 2 adresses
 * @param slave 7-bit address
 */
void selectSlave(uint8_t slave)
{
	int error = ioctl(DEVICE_FILE, I2C_SLAVE, slave);
	if(error == -1)
		printf("[I2CError selecting LCD address]\n");
}
