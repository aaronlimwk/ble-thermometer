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
#define BLE_STATE 16


// Bluetooth Message Handler + Setup
char message[100];
bool message_incoming = false;
SoftwareSerial mySerial(9,8); // RX, TX
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
RunningMedian temp_samples = RunningMedian(SAMPLE_SIZE);
double temp_median = 0;
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

void setup() {
  //analogReference(EXTERNAL);
  // Pin Setup
  pinMode(BLE_STATE, INPUT);
  pinMode(VTH,INPUT);
  pinMode(PWR_OUT, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  //Turn off LED
  digitalWrite(LED, LOW);

  //Begin Serial Communication for HM-11 and Board
  mySerial.begin(baud);

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
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
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

  // BLE Setup
  at("AT+NAMEtrump2024");
  at("AT+ROLE0"); // peripheral
  at("AT+IMME0"); // "work immediately", not sure what this does
  at("AT+RESET"); // actually more a restart than a reset .. needed after ROLE
  delay(1000); 
}

void loop() {
  but_left.tick();
  but_right.tick();
  but_middle.tick();
  pwr_in.tick();
  
  display.clearDisplay();
  display.setTextSize(2);

  if(ble_connected == false){
    if(digitalRead(BLE_STATE) == 1){
      if(ble_conn_count == 20){
        //SHOW BLE CONNECTED HERE U DUMBASS
        ble_connected = true;
        ble_conn_count = 0;
      }
      else { 
        ble_conn_count++;
      }
      
    }
    else {
      ble_conn_count = 0;
    }
  }
  
  if(ble_connected == true){
    if(digitalRead(BLE_STATE) == 0){
      ble_connected = false;
    }
  }

  if (state != SLEEP_STATE) {
    if (millis() >= last_millis + MAX_TIME) {
      state = DEFAULT_STATE;
      reset();
    } else if (millis() >= last_millis + SLEEP_TIME) {
      state = SLEEP_STATE;
    } 
  }

  if (state == CLF_NO_STATE || state == CLF_YES_STATE) {
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.setTextSize(1);

    if (state == CLF_NO_STATE) {
      display.setCursor(25, 20);
      display.print("YES");
      display.setCursor(85, 20);
      display.print(">NO");
    } else {
      display.setCursor(19, 20);
      display.print(">YES");
      display.setCursor(91, 20);
      display.print("NO");
    }

    if (question_num < MAX_QUESTION) {
      display.setCursor(position_x[question_num], 6);
      strcpy_P(buffer, (char *)pgm_read_word(&(questions[question_num])));
      display.print(buffer);
      
      display.setCursor(57, 20);
      display.print(question_num + 1);
      display.print("/8");
    }
    
  } else if (state == CELSIUS_STATE || state == FAHRENHEIT_STATE) {
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.setCursor(25, 9);
    display.print("Unit:");
    display.setTextSize(1);
    display.write(167);
    display.setTextSize(2);

    if (state == CELSIUS_STATE) {
      display.print("C");
      display.setCursor(108, 9);
      display.write(16);
    } else {
      display.print("F");
      display.setCursor(8, 9);
      display.write(17);
    }
  } else if (state == TEMP_REC_STATE) {
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.setCursor(25, 9);
    display.print(temp_median); 
  } else if (state != SLEEP_STATE) {
    drawIcon();
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    if (state == MIN_STATE) {
      display.setCursor(90, 9);
      display.write(16);
    } else if (state == MAX_STATE) {
      display.setCursor(28, 9);
      display.write(17);
    } else {
      display.setCursor(28, 9);
      display.write(17);
      display.setCursor(90, 9);
      display.write(16);
    }
    
    display.setCursor(47, 9);
    strcpy_P(buffer, (char *)pgm_read_word(&(string_table[state])));
    display.print(buffer);   
  }

  if(mySerial.available()){
    char a = char(mySerial.read());
    if(!message_incoming){
      if(a == '<'){
        message_incoming = true;
      }
    }
    else{
      if(a == '>'){
        message_incoming = false;
        if(strcmp("Record Temperature",message) == 0) {
          if(power_on == false) {
            powerON();
            delay(500);
            temp_samples.clear();
            for(int i = 0; i < SAMPLE_SIZE; i++) {
              temp_samples.add(analogRead(VTH));
              delay(30);
            }
            temp_median = temp_samples.getMedian();
            mySerial.print(temp_median);
            powerOFF();
          }
          else {
            mySerial.print("Try Again");
          }
        }
        memset(&message[0],0,sizeof(message));
      }
      else{
        strncat(message,&a,1);
      }
    }
  }
  
  refresh(10);
}

void at(char* cmd) {
  mySerial.write(cmd);
//  Serial.print(cmd);
  while(!mySerial.find("OK"));
//  Serial.print(".");
  
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
    temp_median = 0.0;
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
    for(int i = 0; i < SAMPLE_SIZE; i++) {
      temp_samples.add(analogRead(VTH));
      delay(30);
    }
    //digitalWrite(BUZZ, LOW);
    temp_median = temp_samples.getAverage();
    if (ble_connected) {
      mySerial.print(temp_median);
    }
    digitalWrite(LED, LOW);
  }
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
  temp_median = 0.0;
}

void probSequence() {
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  display.setCursor(20,8);
  display.print("You most likely");
  if (prob_flu > prob_cold) {
    display.setCursor(26,18);
    display.print("have the flu.");
  } else {
    display.setCursor(24,18);
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
