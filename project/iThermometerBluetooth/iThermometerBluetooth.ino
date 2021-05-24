#include <OneButton.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>

#define baud 9600

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial mySerial(9,8); // RX, TX

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

char buffer[3];

#define BUT_L     2
#define BUT_MID   3
#define BUT_R     4

OneButton but_left = OneButton(BUT_L, false, false);
OneButton but_middle = OneButton(BUT_MID, false, false);
OneButton but_right = OneButton(BUT_R, false, false);

#define SLEEP_STATE       -1
#define MIN_STATE         0
#define DEFAULT_STATE     1
#define MAX_STATE         4

#define CELSIUS_STATE     40
#define FAHRENHEIT_STATE  41

bool deg_celsius = true;

#define MAX_TIME        30000
#define SLEEP_TIME      45000

int state = DEFAULT_STATE;
unsigned long last_millis = 0;
unsigned long last_interrupt_millis = 0;

char message[100];
bool message_incoming = false;

void setup() {
  // Serial.begin(9600);
  mySerial.begin(baud);
  
  but_left.attachClick(leftClick);
  but_right.attachClick(rightClick);
  but_middle.attachClick(middleClick);
  but_middle.attachLongPressStart(holdClick);
  but_middle.attachDoubleClick(doubleClick);

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

  at("AT"); // check if working, always returns OK
  at("AT+NAMEtrump2024");
  at("AT+ROLE0"); // select master = central
  at("AT+IMME0"); // "work immediately", not sure what this does
  at("AT+RESET"); // actually more a restart than a reset .. needed after ROLE
  at("AT"); // check if working, always returns OK
  delay(1000);
}

void loop() {
  but_left.tick();
  but_right.tick();
  but_middle.tick();
  display.clearDisplay();

  if (state != SLEEP_STATE) {
    if (millis() >= last_millis + MAX_TIME) {
      state =  DEFAULT_STATE;
    }
  
    if (millis() >= last_millis + SLEEP_TIME) {
      state = SLEEP_STATE;
    } 
  }

  if (state == CELSIUS_STATE || state == FAHRENHEIT_STATE) {
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
      if(a == '*'){
        message_incoming = true;
        Serial.println("Message Coming");
      }
    }
    else{
      if(a == '#'){
        message_incoming = false;
        Serial.println("Message Finished");
        Serial.println(message);
        memset(&message[0],0,sizeof(message));
        //process message here
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
  while(!mySerial.find("OK")) Serial.print(".");
  Serial.println(" .. OK");
}

void leftClick() {
  if (state == FAHRENHEIT_STATE) {
    last_millis = millis();
    state = CELSIUS_STATE;
  } else if (state > MIN_STATE && state <= MAX_STATE) {
    last_millis = millis();
    state--;
  }
}

void rightClick() {
  if (state == CELSIUS_STATE) {
    last_millis = millis();
    state = FAHRENHEIT_STATE;
  } else if (state >= MIN_STATE && state < MAX_STATE) {
    last_millis = millis();
    state++;
  }
}

void middleClick() {
  if (state == SLEEP_STATE) {
    last_millis = millis();
    state = DEFAULT_STATE;
  } else if (state == MAX_STATE) {
    last_millis = millis();
    state = CELSIUS_STATE;
  }
}

void doubleClick() {
  if (state == CELSIUS_STATE || state == FAHRENHEIT_STATE) {
    last_millis = millis();
    state = MAX_STATE;
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

void drawIcon() {
  display.drawBitmap(4, 6, menu_bmp, 20, 20, SSD1306_WHITE);
  display.drawBitmap(104, 6, ble_off_bmp, 20, 20, SSD1306_WHITE);
}

void refresh(int displayDelay) {
  display.display();
  delay(displayDelay);
  display.clearDisplay();
}

#define SLEEP_SEQUENCE  4

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
