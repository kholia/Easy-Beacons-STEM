// Runs on Raspberry Pi Pico W / Pico

#include <Wire.h>
#include "si5351.h"
#include <Arduino.h>
#include <TimeLib.h>
#include <JTEncode.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"   // interrupts
#include "hardware/pwm.h"   // pwm
#include "hardware/sync.h"  // wait for interrupt

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator);

// GPS stuff
#include <TinyGPS++.h>
TinyGPSPlus gps;

// Si5351 stuff
Si5351 si5351;
int32_t si5351CalibrationFactor = 0;  // This is automatically derived!

long long frequency = 28126010 * 100UL;  // CHANGE THIS PLEASE
uint8_t tones[255];
int toneDelay;
int symbolCount;
int toneSpacing;

// WSPR properties
#define WSPR_TONE_SPACING 146  // ~1.46 Hz
#define WSPR_DELAY 683         // Delay value for WSPR
char call[] = "VU3CER";        // CHANGE THIS PLEASE!
// char loc[] = "MK68";
char loc[8];
char harcoded_loc[] = "MK68";  // CHANGE THIS PLEASE!
uint8_t dbm = 23;
JTEncode jtencode;

// Pins
#define EXTERNAL_LED 6  // GP6 -> Pin 9 on the Pico board
#define FREQUENCY_INPUT_PIN 15
#define SW1 10
int buttonState = 0;
int previousButtonState = -1;

int encoder(char *message, uint8_t *tones, int is_ft4);

void tx(uint8_t *tones) {
  uint8_t i;

  Serial.println("TX!");
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(EXTERNAL_LED, HIGH);
  si5351.set_clock_pwr(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK0, 1);

  for (i = 0; i < symbolCount; i++) {
    si5351.set_freq(frequency + (tones[i] * toneSpacing), SI5351_CLK0);
    delay(toneDelay);
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK0, 0);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(EXTERNAL_LED, LOW);
}

// Automatic calibration
uint32_t f_hi;

void pwm_int() {
  pwm_clear_irq(7);
  f_hi++;
}

void do_calibration() {
  int64_t existing_error = 0;
  int64_t error = 0;
  int count = 3;                         // reverse count
  uint64_t target_freq = 1000000000ULL;  // 10 MHz, in hundredths of hertz

  // Warmup
  Serial.println("Warming up....");
  delay(7000);

  uint32_t f = 0;
  // Frequency counter
  while (true) {
    count = count - 1;
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_RISING);
    pwm_init(7, &cfg, false);
    gpio_set_function(FREQUENCY_INPUT_PIN, GPIO_FUNC_PWM);
    pwm_set_irq_enabled(7, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_int);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    f_hi = 0;
    si5351.set_correction(0, SI5351_PLL_INPUT_XO);  // important - reset calibration
    uint32_t t = time_us_32() + 2;
    while (t > time_us_32())
      ;
    pwm_set_enabled(7, true);
    t += 3000000;  // Gate time (in uSeconds), 3 seconds
    // t += 100000; // Gate time (in uSeconds), 100 ms
    while (t > time_us_32())
      ;
    pwm_set_enabled(7, false);
    f = pwm_get_counter(7);
    f += f_hi << 16;
    Serial.print(f / 3.0);  // Divide by gate time in seconds
    Serial.println(" Hz");
    error = ((f / 3.0) * 100ULL) - target_freq;
    Serial.print("Current calibration correction value is ");
    Serial.printf("%" PRId64 "\n", error);
    Serial.print("Total calibration value is ");
    Serial.println(error + existing_error);
    if (count <= 0) {  // Auto-calibration logic
      Serial.println();
      Serial.print(F("Calibration value is "));
      Serial.println(error);
      Serial.println(F("Setting calibration value automatically"));
      si5351.set_correction(error + existing_error, SI5351_PLL_INPUT_XO);
      existing_error = existing_error + error;
      si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
      Serial.println(F("Resetting target frequency"));
      si5351.set_freq(target_freq, SI5351_CLK2);
      // count = 3;
      break;
    }
  }

  // Let's keep CLK2 on to keep the chip warmed up!
}

// Debug helper
void led_flash() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
}

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator);

void sync_time_with_gps_with_timeout() {
  // digitalWrite(IS_GPS_SYNCED_PIN, LOW);
  bool newData = false;

  Serial.println("GPS Sync Wait...");
  digitalWrite(LED_BUILTIN, LOW);

  Serial1.setTX(16);
  Serial1.setRX(17);
  Serial1.begin(9600);

  for (unsigned long start = millis(); millis() - start < 64000;) {
    while (Serial1.available()) {
      char c = Serial1.read();
      // Serial.write(c); // Debug GPS stuff
      if (gps.encode(c))  // Did a new valid sentence come in?
        newData = true;
    }
    if (newData && gps.time.isUpdated() && gps.date.isUpdated() && (gps.location.isValid() && gps.location.age() < 2000)) {
      byte Year = gps.date.year();
      byte Month = gps.date.month();
      byte Day = gps.date.day();
      byte Hour = gps.time.hour();
      byte Minute = gps.time.minute();
      byte Second = gps.time.second();
      setTime(Hour, Minute, Second, Day, Month, Year);
      Serial.println(gps.location.lng());
      Serial.println(gps.location.lat());
      GPStoLOCATOR(gps.location.lng(), gps.location.lat(), loc);
      Serial.println(loc);
      loc[4] = 0;  // NULL terminate early
      if (!strcmp(loc, "AA00")) {
        strcpy(loc, harcoded_loc);
      }
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.println("GPS Sync Done!");
      return;
    }
  }
  Serial.println("GPS Sync Failed!");
}

void resync_time_with_gps_with_timeout() {
  // digitalWrite(IS_GPS_SYNCED_PIN, LOW);
  bool newData = false;

  Serial.println("GPS Resync Wait...");
  digitalWrite(LED_BUILTIN, LOW);

  for (unsigned long start = millis(); millis() - start < 64000;) {
    while (Serial1.available()) {
      char c = Serial1.read();
      if (gps.encode(c))  // Did a new valid sentence come in?
        newData = true;
    }
    if (newData && gps.time.isUpdated() && gps.date.isUpdated() && (gps.location.isValid() && gps.location.age() < 2000)) {
      byte Year = gps.date.year();
      byte Month = gps.date.month();
      byte Day = gps.date.day();
      byte Hour = gps.time.hour();
      byte Minute = gps.time.minute();
      byte Second = gps.time.second();
      setTime(Hour, Minute, Second, Day, Month, Year);
      GPStoLOCATOR(gps.location.lng(), gps.location.lat(), loc);
      Serial.println(loc);
      loc[4] = 0;  // NULL terminate early
      if (!strcmp(loc, "AA00")) {
        strcpy(loc, harcoded_loc);
      }
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.println("GPS Resync Done!");
      return;
    }
  }
  Serial.println("GPS sync Failed!");
}

void setup() {
  int ret = 0;

  // Setup I/O pins
  gpio_set_function(FREQUENCY_INPUT_PIN, GPIO_FUNC_PWM);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  Serial.begin(115200);
  delay(5000);

  // I2C pins
  Wire.setSDA(12);
  Wire.setSCL(13);
  Wire.begin();

  // Initialize the Si5351
  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 26000000, si5351CalibrationFactor);
  if (ret != true) {
    led_flash();
    watchdog_reboot(0, 0, 1000);
  }
  si5351.set_clock_pwr(SI5351_CLK0, 0);  // Safety first
  si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);

  // Set CLK0 output
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);  // Set for minimum power as we have an external amplifier
  si5351.set_clock_pwr(SI5351_CLK0, 0);                  // Disable the clock initially
  si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);  // Output current 2/4/6/8 mA
  si5351.set_freq(10000000 * 100ULL, SI5351_CLK2);       // Used for calibration
  si5351.output_enable(SI5351_CLK0, 0);
  si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
  si5351.output_enable(SI5351_CLK2, 1);

  // Perform automatic calibration
  // do_calibration();
  si5351.output_enable(SI5351_CLK0, 0);
  si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
  // si5351.set_clock_pwr(SI5351_CLK2, 0);
  // si5351.output_enable(SI5351_CLK2, 0);

  // Sync. time with GPS module
  sync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();

  // Prep for WSPR
  jtencode.wspr_encode(call, loc, dbm, tones);
  toneDelay = WSPR_DELAY;
  toneSpacing = WSPR_TONE_SPACING;
  symbolCount = WSPR_SYMBOL_COUNT;
}

void loop() {
  if (minute() % 2 == 0 && second() % 60 == 0) {
    // empirically calculated start delay
    delay(400);
    tx(tones);
    resync_time_with_gps_with_timeout();
  }

  delay(10);
}
