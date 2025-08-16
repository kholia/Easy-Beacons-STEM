/* ------------------- */
/* conv_fn.cc          */
/* H. M. Stauss, 2001  */
/* ------------------- */

#include <math.h>
#include <ctype.h>
#include <string.h>

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator) {
  /* gps_long and gps_lat are the longitutes (-180 to +180) and   */
  /* latitues (-90 to +90) of the GPS coordinates in degree       */
  /* locator is a string and memory must be allocated for 7 chars */

  int is_err, field_l, field_b, square_l, square_b;
  float ssquare_l, ssquare_b;

  /* Initialization */
  is_err = 0;
  if (gps_long < -180 || gps_long > 180) is_err = 1;
  if (gps_lat  < -90 || gps_lat > 90) is_err = 1;

  /* Mainroutine */
  if (is_err) {
    return (-1);
  }
  else {
    gps_long += 180; gps_lat += 90;
    /* fields */
    field_l = (int)(gps_long / 20.0);
    field_b = (int)(gps_lat / 10.0);
    /* squares */
    square_l = (int)(fmod(gps_long, 20.0) / 2.0);
    square_b = (int)(fmod(gps_lat, 10.0));
    /* subsquares */
    ssquare_l = 12.5 * (gps_long - (field_l * 20) - (square_l * 2));
    ssquare_b = 24.5 * (gps_lat - (field_b * 10) - (square_b));

    strcpy(locator, "ZZZZZZ");
    locator[0] = ((char)field_l + 65); locator[1] = ((char)field_b + 65);
    locator[2] = ((char)square_l + 48); locator[3] = ((char)square_b + 48);
    locator[4] = ((char)(((int)ssquare_l) + 65));
    locator[5] = ((char)(((int)ssquare_b) + 65));
  }
  return (0);
}

int LOCATORtoGPS(char *locator, float *gps_long, float *gps_lat) {
  /* gps_long and gps_lat are the longitutes (-180 to +180) and   */
  /* latitues (-90 to +90) of the GPS coordinates in degree       */

  int i, is_err, field_l, field_b, square_l, square_b;
  float ssquare_l, ssquare_b;

  /* Initialization */
  is_err = 0;
  locator[6] = '\0';
  for (i = 0; i < 2; i++) {
    locator[i] = toupper(locator[i]);
    if (locator[i] < 'A' || locator[i] > 'R') is_err = 1;
  }
  for (i = 4; i < 6; i++) {
    locator[i] = toupper(locator[i]);
    if (locator[i] < 'A' || locator[i] > 'X') is_err = 1;
  }

  /* field */
  field_l = 20 * ((int)locator[0] - 65);
  field_b = 10 * ((int)locator[1] - 65);
  /* square */
  square_l = (int)locator[2] - 48;
  square_b = (int)locator[3] - 48;
  if (square_l < 0 || square_l > 9) is_err = 1;
  if (square_b < 0 || square_b > 9) is_err = 1;
  square_l = 2 * square_l;
  /* subsquare */
  ssquare_l = (int)locator[4] - 65;
  ssquare_b = (int)locator[5] - 65;
  ssquare_l = 2.0 * (float)ssquare_l / 24;
  ssquare_b = 1.0 * (float)ssquare_b / 24;

  if (is_err) {
    return (-1);
  }
  else {
    *gps_long = (float)field_l + (float)square_l + ssquare_l - 180.0;
    *gps_lat = (float)field_b + (float)square_b + ssquare_b - 90.0;
  }
  return (0);
}
