// Copyright Dexter Industries, 2016
// http://dexterindustries.com/grovepi

#ifndef GROVEPI_H
#define GROVEPI_H

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>


extern int fd;
extern char *fileName;
extern int  address;
extern unsigned char w_buf[5],r_buf[32];
extern unsigned long reg_addr;

#define INPUT 0
#define OUTPUT 1

#define dRead_cmd 	1
#define dWrite_cmd 	2
#define aRead_cmd 	3
#define aWrite_cmd 	4
#define pMode_cmd	5
#define uRead_cmd   7
#define dustSensorRead_cmd 10
#define dustEn_cmd  14
#define dustDis_cmd 15
#define acc_xyz_cmd 20

//Initialize the GrovePi
int init(void);
//Write a register
int write_block(char cmd,char v1,char v2,char v3);
//Read 1 byte of data
char read_byte(void);
uint8_t readBlock(uint8_t *data_block);
char read_block();
//RaspberyPi / GrovePi sleeps x milliseconds
void pi_sleep(int);
//read Data from an anolog port (Relais)
int analogRead(int port);
//read Data from a port which is connected with a ultrasonic Range Sensor
int ultrasonicRead(int port);
//write Data to a digital pin (LED)
int digitalWrite(int port,int value);
//specify the pin-mode which is used (at port x, INPUT / OUTPUT)
int pinMode(int port,int mode);
//read Data from a digital port (Button)
int digitalRead(int port);
//write Data to a anlaog port (motor)
int analogWrite(int port,int value);
//some functions for the DHT-sensor for connection
void SMBusName(char *smbus_name);
int initDevice(uint8_t address);

const static uint8_t DHT_TEMP_CMD = 40; // command for reaching DTH sensor on the GrovePi
const static int MAX_RETRIES = 3;       //Max retries for connecting via I2C

void getSafeTemperatureAndHumidityData(float *temp, float *humidity, int port); //get checked Data from Temperature / Humidity sensor
void getUnsafeTemperatureAndHumidityData(float *temp, float *humidity, int port); //get data from from Temperature / Humidity sensor

void getSafeTemperatureData(float *temp, int port);
void getSafeHumidityData(float *humidity, int port);
void getUnsafeTemperatureData(float *temp, int port);
void getUnsafeHumidityData(float *humidity, int port);

//new Temp and Hum functions
void getTemperature(float *temp, int port);
void getHumidity(float *humidity, int port);
void getTemperatureAndHumidity(float *temp, float *humidity, int port);


// converts the first 4 bytes of the array into a float
static const float fourBytesToFloat(uint8_t *data); // converts the first 4 bytes of the array into a float
static const bool areTemperatureAndHumidityGoodReadings(int temp, int humidity); // check the data if they are correct for a "normal" surroundings

static const bool areTemperatureGoodReadings(int temp);
static const bool areHumidityGoodReadings(int humidity);

//all functions for RGB-LCD
void connectLCD(); // connects the LCD via I2C

void setRGB(uint8_t red, uint8_t green, uint8_t blue); //sets the color of the background
void setText(const char *str);                          //sets the Text (multiline if wanted)
void sendCommand(uint8_t mode, uint8_t command);        // sends the command
void selectSlave(uint8_t slave);

uint8_t DEVICE_FILE;
bool connected;

#define DISPLAY_RGB_ADDR 0x62
#define DISPLAY_TEXT_ADDR 0x3e

#define CLEAR_DISPLAY 0x01
#define DISPLAY_ON 0x08
#define NO_CURSOR 0x04
#define ENABLE_2ROWS 0x28
#define PROGRAM_MODE 0x80
#define NEW_ROW 0xc0
#define DISPLAY_CHAR 0x40
#define MAX_NO_CHARS 32


#endif /*GROVEPI_H */
