#include<ctype.h>
#include<string.h>
#include<float.h>
#include<stdio.h>

void fill_col(char *buf, int already_filled, int rest) {
  while ( already_filled < rest) {
    strcat(buf, " ");
    already_filled++;
  }
}

void to_upper(char *buf) {
  int i = 0;
  while( buf[i] ) {
    buf[i] = toupper(buf[i]);
    i++;
  }
}

int first_digit(char *buf) {
  // move j to the first digit
  int j = 0;
  while( !isdigit(buf[j]) && buf[j] ) {
    j++;
  }
  return j;
}

// parse string to double
double parse_sensor_data(char *buf) {
  char sensorbuf[512];
  double sensordata;

  int fd = first_digit(buf);

  // void
  if (fd == strlen(buf)) return DBL_MIN;

  memcpy(sensorbuf,&buf[fd], strlen(buf)-fd);
  sscanf(sensorbuf,"%lf",&sensordata);

  return sensordata;
}

