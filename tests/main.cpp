#include <stdio.h>

#include <math.h>
#include <ctype.h>
#include <string.h>

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator);

int main()
{
  char loc[8];

  GPStoLOCATOR(73.95, 18.49, loc);

  puts(loc);

  return 0;
}
