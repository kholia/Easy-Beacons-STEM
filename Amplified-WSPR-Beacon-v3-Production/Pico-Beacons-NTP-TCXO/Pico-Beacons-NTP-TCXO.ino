// Runs on Raspberry Pi Pico W

#include <Wire.h>
#include <si5351.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <JTEncode.h>

// WiFI
#include <WiFi.h>
#include "credentials.h"
WiFiMulti multi;

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm
#include "hardware/sync.h" // wait for interrupt

// GPS stuff
#include <TinyGPS++.h>
TinyGPSPlus gps;

// Si5351 stuff
Si5351 si5351;
int32_t si5351CalibrationFactor = 0; // This is automatically derived!

long long frequency = 28126175 * 100UL; // CHANGE THIS PLEASE
// long long frequency = 50295000 * 100UL; // 6m WSPR - YAY!
uint8_t tones[255];
int toneDelay;
int symbolCount;
int toneSpacing;

// WSPR properties
#define WSPR_TONE_SPACING 146 // ~1.46 Hz
#define WSPR_DELAY 683    // Delay value for WSPR
char call[] = "VU3CER"; // CHANGE THIS PLEASE!
char loc[] = "MK68";
uint8_t dbm = 23;
JTEncode jtencode;

// Pins
#define EXTERNAL_LED 6 // GP6 -> Pin 9 on the Pico board
#define FREQUENCY_INPUT_PIN 15
#define SW1 10
int buttonState = 0;
int previousButtonState = -1;

int encoder(char *message, uint8_t *tones, int is_ft4);

void tx(uint8_t *tones)
{
  uint8_t i;

  Serial.println("TX!");
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(EXTERNAL_LED, HIGH);
  si5351.set_clock_pwr(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK0, 1);

  for (i = 0; i < symbolCount; i++)
  {
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

void do_calibration()
{
  int64_t existing_error = 0;
  int64_t error = 0;
  int count = 3; // reverse count
  uint64_t target_freq = 1000000000ULL; // 10 MHz, in hundredths of hertz

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
    si5351.set_correction(0, SI5351_PLL_INPUT_XO); // important - reset calibration
    uint32_t t = time_us_32() + 2;
    while (t > time_us_32());
    pwm_set_enabled(7, true);
    t += 3000000; // Gate time (in uSeconds), 3 seconds
    // t += 100000; // Gate time (in uSeconds), 100 ms
    while (t > time_us_32());
    pwm_set_enabled(7, false);
    f = pwm_get_counter(7);
    f += f_hi << 16;
    Serial.print(f / 3.0); // Divide by gate time in seconds
    Serial.println(" Hz");
    error = ((f / 3.0) * 100ULL) - target_freq;
    Serial.print("Current calibration correction value is ");
    Serial.printf("%" PRId64 "\n", error);
    Serial.print("Total calibration value is ");
    Serial.println(error + existing_error);
    if (count <= 0) { // Auto-calibration logic
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
}

// Debug helper
void led_flash()
{
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

void setClock() {
  NTP.begin("pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  // We just need the minute and the second fields to be accurate
  // setTime(hr, min, sec, day, mnth, yr);
  // https://cplusplus.com/reference/ctime/tm/
  // https://github.com/PaulStoffregen/Time/blob/master/TimeLib.h#L34
  setTime(now);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  Serial.println(timeStatus());
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
  si5351.set_clock_pwr(SI5351_CLK0, 0); // safety first

  // WiFI + NTP (https://arduino-pico.readthedocs.io/en/latest/wifintp.html)
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  multi.addAP(ssid, password);
  if (multi.run() != WL_CONNECTED) {
    Serial.println("Unable to connect to network, rebooting in 10 seconds...");
    delay(10000);
    rp2040.reboot();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  setClock();

  // Set CLK0 output
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set for minimum power as we have an external amplifier
  si5351.set_clock_pwr(SI5351_CLK0, 0); // Disable the clock initially
  si5351.output_enable(SI5351_CLK0, 0);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA); // Output current 2/4/6/8 mA
  si5351.set_freq(10000000 * 100ULL, SI5351_CLK2); // used for calibration
  si5351.output_enable(SI5351_CLK0, 0);

  // Perform automatic calibration
  do_calibration();
  si5351.output_enable(SI5351_CLK0, 0);
  // si5351.set_clock_pwr(SI5351_CLK2, 0);
  // si5351.output_enable(SI5351_CLK2, 0);

  // Prep for WSPR
  jtencode.wspr_encode(call, loc, dbm, tones);
  toneDelay = WSPR_DELAY;
  toneSpacing = WSPR_TONE_SPACING;
  symbolCount = WSPR_SYMBOL_COUNT;
}

void loop() {
  if (minute () % 2 == 0 && second() % 60 == 0)  {
    // empirically calculated start delay
    tx(tones);
  }

  delay(10);
}
