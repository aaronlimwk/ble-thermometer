#include <SoftwareSerial.h>
#include <OneButton.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>
#include <RunningMedian.h>

// Baud Rate
#define baud 9600

// Pin Definitions
#define BUT_L     2
#define BUT_MID   3
#define BUT_R     4
#define BUZZ      5
#define PWR_IN    6
#define PWR_OUT   7
#define LED       10
#define VTH       14
#define VR        15
#define BLE_STATE 16


// Bluetooth Message Handler + Setup
char message[100];
bool message_incoming = false;
SoftwareSerial mySerial(9, 8); // RX, TX
int ble_conn_count = 0;
bool ble_connected = false;

//Sleep Sequence
#define SLEEP_SEQUENCE  4

// OLED Definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 'Menu Bar Icon', 20x20px
const unsigned char menu_bmp [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x1f, 0xff, 0x80, 0x18, 0x01, 0x80, 0x30,
  0x00, 0xc0, 0x33, 0xfc, 0xc0, 0x33, 0xfc, 0xc0, 0x30, 0x00, 0xc0, 0x33, 0xfc, 0xc0, 0x33, 0xfc,
  0xc0, 0x30, 0x00, 0xc0, 0x33, 0xfc, 0xc0, 0x33, 0xfc, 0xc0, 0x30, 0x00, 0xc0, 0x18, 0x01, 0x80,
  0x1f, 0xff, 0x80, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'Bluetooth Disabled Icon', 20x20px
const unsigned char ble_off_bmp [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x08, 0x70, 0x00, 0x1c, 0x78, 0x00, 0x0e,
  0x7c, 0x00, 0x07, 0x2e, 0x00, 0x03, 0x9c, 0x00, 0x01, 0xc8, 0x00, 0x00, 0xe0, 0x00, 0x00, 0xf0,
  0x00, 0x01, 0xf8, 0x00, 0x03, 0xfc, 0x00, 0x07, 0x6e, 0x00, 0x0e, 0x6f, 0x00, 0x04, 0x7b, 0x80,
  0x00, 0x71, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'Bluetooth Enabled Icon', 20x20px
const unsigned char ble_on_bmp [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x70, 0x00, 0x04, 0x78, 0x00, 0x0e,
  0x7c, 0x00, 0x07, 0x6e, 0x00, 0x03, 0xfc, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
  0x00, 0x01, 0xf8, 0x00, 0x03, 0xfc, 0x00, 0x07, 0x6e, 0x00, 0x0e, 0x7c, 0x00, 0x04, 0x78, 0x00,
  0x00, 0x70, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'Sleeping Mode Icon', 20x20px
const unsigned char sleep_bmp [] PROGMEM = {
  0xff, 0xc0, 0x00, 0x7f, 0xc0, 0x00, 0x01, 0x80, 0x00, 0x03, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0c,
  0x00, 0x00, 0x18, 0x0f, 0xf0, 0x30, 0x00, 0x60, 0x7f, 0xc0, 0xc0, 0xff, 0xc1, 0x80, 0x00, 0x03,
  0x00, 0x00, 0x06, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0xc0, 0x00,
  0x01, 0x80, 0x00, 0x03, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x0f, 0xe0, 0x00
};

const char string_off[] PROGMEM = "OFF";
const char string_tmp[] PROGMEM = "TMP";
const char string_bth[] PROGMEM = "BLE";
const char string_clf[] PROGMEM = "CLF";
const char string_deg[] PROGMEM = "DEG";

const char *const string_table[] PROGMEM = {string_off, string_tmp, string_bth, string_clf, string_deg};

char buffer[20];

// Classifier Declarations
#define MAX_QUESTION      8

const char question_1[] PROGMEM = "Feeling tired?"; // (24, 6)
const char question_2[] PROGMEM = "Fever?";         // (50, 6)
const char question_3[] PROGMEM = "Chills?";        // (46, 6)
const char question_4[] PROGMEM = "Sore throat?";   // (28, 6)
const char question_5[] PROGMEM = "Coughing?";      // (40, 6)
const char question_6[] PROGMEM = "Headache?";      // (40, 6)
const char question_7[] PROGMEM = "Muscle pain?";   // (28, 6)
const char question_8[] PROGMEM = "Sneezing?";      // (40, 6)

int question_num = 0;

const char *const questions[] PROGMEM = {question_1, question_2, question_3, question_4, question_5, question_6, question_7, question_8};
const int position_x[8] = {24, 50, 47, 29, 40, 40, 29, 40};

#define FLU       0.001
const float flu_table[] = {0.8, 0.9, 0.9, 0.55, 0.9, 0.85, 0.675, 0.25};
float prob_flu = log(FLU);

#define COLD      0.999
const float cold_table[] = {0.225, 0.005, 0.1, 0.5, 0.4, 0.25, 0.1, 0.9};
float prob_cold = log(COLD);

// State Definitions
#define SLEEP_STATE       -1
#define MIN_STATE         0
#define DEFAULT_STATE     1
#define CLF_STATE         3
#define MAX_STATE         4

#define TEMP_REC_STATE    10

#define CLF_NO_STATE      30
#define CLF_YES_STATE     31

#define CELSIUS_STATE     40
#define FAHRENHEIT_STATE  41

#define MAX_TIME        30000
#define SLEEP_TIME      45000

#define SAMPLE_SIZE       40
#define ADC_RES           1024
#define V_REF             5
#define OFFSET            2.5
#define GAIN              1977
#define MV                1000
#define CAL_CONST         0.7
RunningMedian temp_samples = RunningMedian(SAMPLE_SIZE);

double temp_c = 0;
double temp_f = 0;

bool deg_celsius = true;

int state = DEFAULT_STATE;
unsigned long last_millis = 0;
unsigned long last_interrupt_millis = 0;

// Button Setup
OneButton but_left = OneButton(BUT_L, false, false);
OneButton but_middle = OneButton(BUT_MID, false, false);
OneButton but_right = OneButton(BUT_R, false, false);

// Power Button
bool power_on = false;
OneButton pwr_in = OneButton(PWR_IN, true, true);

//Temperature Chart
const float temp_chart[9][54] PROGMEM = {
  //20 degrees Celsius
  { 1.247, 1.256, 1.265, 1.274, 1.283, 1.292, 1.301, 1.31, 1.319, 1.328, 1.337, 1.346, 1.355, 1.364, 1.373, 1.382, 1.391, 1.4, 1.409, 1.418, 1.427, 1.437, 1.446, 1.455, 1.464, 1.473, 1.482,
    1.491, 1.501, 1.51, 1.519, 1.528, 1.537, 1.547, 1.556, 1.565, 1.574, 1.584, 1.593, 1.602, 1.612, 1.621, 1.63, 1.639, 1.649, 1.658, 1.668, 1.677, 1.686, 1.696, 1.705, 1.714, 1.724, 1.733
  },
  //21 degrees Celsius
  { 1.169, 1.178, 1.187, 1.196, 1.205, 1.214, 1.223, 1.232, 1.241, 1.25, 1.259, 1.268, 1.277, 1.286, 1.295, 1.305, 1.314, 1.323, 1.332, 1.341, 1.35, 1.359, 1.368, 1.377, 1.387, 1.396, 1.405,
    1.414, 1.423, 1.432, 1.442, 1.451, 1.46, 1.469, 1.479, 1.488, 1.497, 1.506, 1.516, 1.525, 1.534, 1.543, 1.553, 1.562, 1.571, 1.581, 1.59, 1.599, 1.609, 1.618, 1.628, 1.637, 1.646, 1.656
  },
  //22 degrees Celsius
  { 1.091, 1.1, 1.109, 1.118, 1.127, 1.136, 1.145, 1.154, 1.163, 1.172, 1.181, 1.19, 1.199, 1.208, 1.217, 1.226, 1.235, 1.245, 1.254, 1.263, 1.272, 1.281, 1.29, 1.299, 1.308, 1.318, 1.327,
    1.336, 1.345, 1.354, 1.363, 1.373, 1.382, 1.391, 1.4, 1.41, 1.419, 1.428, 1.437, 1.447, 1.456, 1.465, 1.475, 1.484, 1.493, 1.503, 1.512, 1.521, 1.531, 1.54, 1.549, 1.559, 1.568, 1.578
  },
  //23 degrees Celsius
  { 1.012, 1.021, 1.03, 1.039, 1.048, 1.057, 1.066, 1.075, 1.084, 1.093, 1.102, 1.111, 1.12, 1.129, 1.138, 1.147, 1.156, 1.166, 1.175, 1.184, 1.193, 1.202, 1.211, 1.22, 1.229, 1.239, 1.248,
    1.257, 1.266, 1.275, 1.284, 1.294, 1.303, 1.312, 1.321, 1.331, 1.34, 1.349, 1.358, 1.368, 1.377, 1.386, 1.396, 1.405, 1.414, 1.424, 1.433, 1.442, 1.452, 1.461, 1.47, 1.48, 1.489, 1.499
  },
  //24 degrees Celsius
  { 0.932, 0.941, 0.95, 0.959, 0.968, 0.977, 0.986, 0.995, 1.004, 1.013, 1.022, 1.031, 1.04, 1.049, 1.058, 1.068, 1.077, 1.086, 1.095, 1.104, 1.113, 1.122, 1.131, 1.14, 1.15, 1.159, 1.168,
    1.177, 1.186, 1.195, 1.205, 1.214, 1.223, 1.232, 1.242, 1.251, 1.26, 1.269, 1.279, 1.288, 1.297, 1.306, 1.316, 1.325, 1.334, 1.344, 1.353, 1.362, 1.372, 1.381, 1.391, 1.4, 1.409, 1.419
  },
  //25 degrees Celsius
  { 0.852, 0.861, 0.87, 0.879, 0.888, 0.897, 0.906, 0.915, 0.924, 0.933, 0.942, 0.951, 0.96, 0.969, 0.978, 0.987, 0.996, 1.005, 1.014, 1.023, 1.032, 1.042, 1.051, 1.06, 1.069, 1.078, 1.087,
    1.096, 1.106, 1.115, 1.124, 1.133, 1.142, 1.152, 1.161, 1.17, 1.179, 1.189, 1.198, 1.207, 1.217, 1.226, 1.235, 1.245, 1.254, 1.263, 1.273, 1.282, 1.291, 1.301, 1.31, 1.319, 1.329, 1.338
  },
  //26 degrees Celsius
  { 0.77, 0.779, 0.788, 0.797, 0.806, 0.815, 0.824, 0.833, 0.842, 0.851, 0.86, 0.869, 0.878, 0.887, 0.896, 0.906, 0.915, 0.924, 0.933, 0.942, 0.951, 0.96, 0.969, 0.978, 0.988, 0.997, 1.006,
    1.015, 1.024, 1.033, 1.043, 1.052, 1.061, 1.07, 1.08, 1.089, 1.098, 1.107, 1.117, 1.126, 1.135, 1.144, 1.154, 1.163, 1.172, 1.182, 1.191, 1.2, 1.21, 1.219, 1.229, 1.238, 1.247, 1.257
  },
  //27 degrees Celsius
  { 0.688, 0.697, 0.706, 0.715, 0.724, 0.733, 0.742, 0.751, 0.76, 0.769, 0.778, 0.787, 0.796, 0.805, 0.814, 0.823, 0.832, 0.841, 0.851, 0.86, 0.869, 0.878, 0.887, 0.896, 0.905, 0.914, 0.924,
    0.933, 0.942, 0.951, 0.96, 0.97, 0.979, 0.988, 0.997, 1.007, 1.016, 1.025, 1.034, 1.044, 1.053, 1.062, 1.072, 1.081, 1.09, 1.1, 1.109, 1.118, 1.128, 1.137, 1.147, 1.156, 1.165, 1.175
  },
  //28 degrees Celsius
  { 0.605, 0.614, 0.623, 0.632, 0.641, 0.65, 0.659, 0.668, 0.677, 0.686, 0.695, 0.704, 0.713, 0.722, 0.731, 0.74, 0.749, 0.758, 0.767, 0.777, 0.786, 0.795, 0.804, 0.813, 0.822, 0.831, 0.841,
    0.85, 0.859, 0.868, 0.877, 0.887, 0.896, 0.905, 0.914, 0.923, 0.933, 0.942, 0.951, 0.961, 0.97, 0.979, 0.988, 0.998, 1.007, 1.016, 1.026, 1.035, 1.045, 1.054, 1.063, 1.073, 1.082, 1.091
  }
};

//Ambient Temperature Chart
float ambient_chart[9] = {2.78, 2.72, 2.67, 2.61, 2.55, 2.5, 2.45, 2.39, 2.34};

void setup() {
  pinMode(BLE_STATE, INPUT);
  pinMode(VTH, INPUT);
  pinMode(PWR_OUT, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  //Turn off LED
  digitalWrite(LED, LOW);

  //Begin Serial Communication for HM-11 and Board
  //mySerial.begin(baud);

  //Button Click - Function Link
  but_left.attachClick(leftClick);
  but_left.attachLongPressStart(holdLeft);
  but_right.attachClick(rightClick);
  but_middle.attachClick(middleClick);
  but_middle.attachLongPressStart(holdClick);
  pwr_in.attachClick(powerON);
  pwr_in.attachLongPressStart(holdRecord);

  // OLED Setup
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1750); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Default Font
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

}

void loop() {
  digitalWrite(LED, HIGH);

  temp_samples.clear();

  temp_f = 0;
  temp_c = 0;

  double temp_object = 0;
  double temp_ambient = 0;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    temp_samples.add(analogRead(VTH));
    temp_ambient += analogRead(VR);
    delay(30);
  }
  //digitalWrite(BUZZ, LOW);
  temp_object = temp_samples.getAverage();
  temp_ambient /= SAMPLE_SIZE;
  getTemperature(temp_object, temp_ambient);
  //    if (ble_connected) {
  //      mySerial.print(temp_median);
  //    }
  digitalWrite(LED, LOW);

}

void leftClick() {
  if (state == FAHRENHEIT_STATE) {
    last_millis = millis();
    state = CELSIUS_STATE;
    reset();
  } else if (state > MIN_STATE && state <= MAX_STATE) {
    last_millis = millis();
    state--;
  } else if (state == CLF_NO_STATE) {
    last_millis = millis();
    state = CLF_YES_STATE;
  }
}

void rightClick() {
  if (state == CELSIUS_STATE) {
    last_millis = millis();
    state = FAHRENHEIT_STATE;
  } else if (state >= MIN_STATE && state < MAX_STATE) {
    last_millis = millis();
    state++;
  } else if (state == CLF_YES_STATE) {
    last_millis = millis();
    state = CLF_NO_STATE;
  }
}

void middleClick() {
  last_millis = millis();
  if (state == SLEEP_STATE) {
    state = DEFAULT_STATE;
  } else if (state == CLF_STATE) {
    state = CLF_NO_STATE;
  } else if (state == MAX_STATE) {
    state = CELSIUS_STATE;
  } else if (state == CLF_YES_STATE || state == CLF_NO_STATE) {
    if (state == CLF_YES_STATE) {
      prob_flu += log(flu_table[question_num]);
      prob_cold += log(cold_table[question_num]);
    } else {
      prob_flu += log(1 - flu_table[question_num]);
      prob_cold += log(1 - cold_table[question_num]);
    }
    if (question_num + 1 <= MAX_QUESTION) {
      question_num++;
      state = CLF_NO_STATE;
      if (question_num == MAX_QUESTION) {
        probSequence();
        state = DEFAULT_STATE;
        reset();
      }
    }
  } else if (state == DEFAULT_STATE) {
    if (power_on == false) {
      powerON();
    }
    state = TEMP_REC_STATE;
  }
}

void holdLeft() {
  if (state == CELSIUS_STATE || state == FAHRENHEIT_STATE) {
    last_millis = millis();
    state = MAX_STATE;
  } else if (state == CLF_NO_STATE || state == CLF_YES_STATE) {
    last_millis = millis();
    question_num = 0;
    state = CLF_STATE;
  } else if (state == TEMP_REC_STATE) {
    last_millis = millis();
    temp_f = 0.0;
    temp_c = 0.0;
    state = DEFAULT_STATE;
    powerOFF();
  }
}

void holdClick() {
  if (state == MIN_STATE) {
    sleepSequence();
    state = SLEEP_STATE;
    display.setTextSize(2);
  } else if (state == CELSIUS_STATE || state == FAHRENHEIT_STATE) {
    last_millis = millis();
    selectSequence();
    delay(2000);
    state = MAX_STATE;
  }
}

void holdRecord() {
  if (state == TEMP_REC_STATE) {
    last_millis = millis();
    //digitalWrite(BUZZ, HIGH);
    digitalWrite(LED, HIGH);

    temp_samples.clear();

    temp_f = 0;
    temp_c = 0;

    double temp_object = 0;
    double temp_ambient = 0;

    for (int i = 0; i < SAMPLE_SIZE; i++) {
      temp_samples.add(analogRead(VTH));
      temp_ambient += analogRead(VR);
      delay(30);
    }
    //digitalWrite(BUZZ, LOW);
    temp_object = temp_samples.getAverage();
    temp_ambient /= SAMPLE_SIZE;
    getTemperature(temp_object, temp_ambient);
    //    if (ble_connected) {
    //      mySerial.print(temp_median);
    //    }
    digitalWrite(LED, LOW);
  }
}

void getTemperature(double object, double ambient) {
  //Convert ADC reading to voltage
  ambient = ambient / ADC_RES * V_REF;
  object  = object  / ADC_RES * V_REF;

  //subtract the 2.5V offset that was placed on the pin
  object -= OFFSET;

  //Find the object voltage in mV by removing the gain factor
  object = object / GAIN * MV / CAL_CONST;

  //Find Ambient Temperature based on voltage
  int ambient_index = 0;
  double ratio = 0;
  for (int i = 0; i < 8; i++) {
    if (ambient <= ambient_chart[i] && ambient >= ambient_chart[i + 1]) {
      double diff = ambient_chart[i] - ambient_chart[i + 1];
      ambient = ambient - ambient_chart[i + 1];

      ratio = ambient / diff;

      if (ratio >= 0.5) {
        ambient_index = i;
      } else {
        ambient_index = i + 1;
      }
    }
  }

  for (int i = 0; i < 53; i++) {
    float temp1 = pgm_read_float_near(temp_chart[ambient_index]+i);
    float temp2 = pgm_read_float_near(temp_chart[ambient_index]+i+1);
    
    if (object >= temp1 && object <= temp2) {
      if (object - temp1 > temp2 - object) {
        temp_c = (i + 1) * 0.1 + 35;
      } else {
        temp_c = i * 0.1 + 35;
      }
    }
  }
  
  temp_f = temp_c * 1.8 + 32;

  display.setCursor(25,9);
  display.print(temp_c);
  refresh(1000);
}

void drawIcon() {
  display.drawBitmap(4, 6, menu_bmp, 20, 20, SSD1306_WHITE);
  if (ble_connected) {
    display.drawBitmap(104, 6, ble_on_bmp, 20, 20, SSD1306_WHITE);
  } else {
    display.drawBitmap(104, 6, ble_off_bmp, 20, 20, SSD1306_WHITE);
  }
}

void refresh(int displayDelay) {
  display.display();
  delay(displayDelay);
  display.clearDisplay();
}


void sleepSequence() {
  char str_sleep[15] = "Turning Off";
  display.setTextSize(1);

  for (uint8_t i = 0; i < SLEEP_SEQUENCE; i++) {
    display.clearDisplay();
    display.drawBitmap(4, 6, sleep_bmp, 20, 20, SSD1306_WHITE);
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.setCursor(32, 12);

    if (i != 0) {
      display.print(strcat(str_sleep, "."));
    } else {
      display.print(str_sleep);
    }

    display.display();
    delay(300);
  }
}

void selectSequence() {
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 12);
  display.write(248);

  if (state == CELSIUS_STATE) {
    display.print("C ");
    deg_celsius = true;
  } else {
    display.print("F ");
    deg_celsius = false;
  }

  display.print("selected!");
  display.display();
  display.setTextSize(2);
}

void reset() {
  question_num = 0;
  for (int i = 0; i < MAX_QUESTION; i++) {
    prob_flu = log(FLU);
    prob_cold = log(COLD);
  }
  digitalWrite(LED, LOW);
  digitalWrite(PWR_OUT, LOW);
  power_on = false;
  temp_f = 0.0;
  temp_c = 0.0;
}

void probSequence() {
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  display.setCursor(20, 8);
  display.print("You most likely");
  if (prob_flu > prob_cold) {
    display.setCursor(26, 18);
    display.print("have the flu.");
  } else {
    display.setCursor(24, 18);
    display.print("have the cold.");
  }
  refresh(2500);
}

void recordTemperature() {
  state = TEMP_REC_STATE;
}

void powerON() {
  digitalWrite(PWR_OUT, HIGH);
  power_on = true;
  state = TEMP_REC_STATE;
}

void powerOFF() {
  digitalWrite(LED, LOW);
  digitalWrite(PWR_OUT, LOW);
  power_on = false;
}
