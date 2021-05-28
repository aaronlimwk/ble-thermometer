#include <SoftwareSerial.h>
#include <OneButton.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>

// Baud Rate
#define baud 9600

// Pin Definitions
#define BUT_L     2
#define BUT_MID   3
#define BUT_R     4
#define PWR_IN    6
#define PWR_OUT   7
#define LED       10
#define VTH       14
#define BLE_STATE 16

// OLED Definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Bluetooth Message Handler + Setup
char message[100];
bool message_incoming = false;
SoftwareSerial mySerial(9,8); // RX, TX
int ble_conn_count = 0;
bool ble_connected = false;

// Power Button
bool power_on = false;
OneButton pwr_in = OneButton(PWR_IN, true, true);

void setup()  
{
  // Pin Setup
  pinMode(BLE_STATE, INPUT);
  pinMode(VTH,INPUT);
  pinMode(PWR_OUT, OUTPUT);
  pinMode(LED, OUTPUT);
  
  pwr_in.attachClick(switchLED);
  digitalWrite(LED, LOW);
  
  mySerial.begin(baud);
//  Serial.begin(baud);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  
  // Default Font
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  
  at("AT"); // check if working, always returns OK
  at("AT+NAMEtrump2024"); //change name
  at("AT+ROLE0"); // select master = central
  at("AT+IMME0"); // "work immediately", not sure what this does
  at("AT+RESET"); // actually more a restart than a reset .. needed after ROLE
  
  delay(1000); // wait a bit, NECESSARY!!
}

void at(char* cmd) {
  mySerial.write(cmd);
//  Serial.print(cmd);
  while(!mySerial.find("OK"));
//  Serial.print(".");
  
}


void loop() // run over and over
{
  
  pwr_in.tick();

  if(ble_connected == false){
    if(digitalRead(BLE_STATE) == 1){
      if(ble_conn_count == 20){
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

  
  if(mySerial.available()){
    char a = char(mySerial.read());
    if(!message_incoming){
      if(a == '<'){
        message_incoming = true;
//        Serial.println("Message Coming");
      }
    }
    else{
      if(a == '>'){
        message_incoming = false;
//        Serial.println("Message Finished");
//        Serial.println(message);
        //process message here
        display.clearDisplay();
        display.setCursor(25, 9);
        display.print(message);

        if(strcmp("Record Temperature",message) == 0){
          mySerial.write("Temperature: Too Hot Bro");
          //Call Temperature Record Function Here
        }
        //reset message buffer
        memset(&message[0],0,sizeof(message));
      }
      else{
        strncat(message,&a,1);
      }
    }
  }
  refresh(10);
  
//  if (Serial.available())
//    mySerial.write(Serial.read());
}

void refresh(int displayDelay) {
  display.display();
  delay(displayDelay);
  display.clearDisplay();
}

void switchLED() {
  if(power_on == false) {
    digitalWrite(LED, HIGH);
    digitalWrite(PWR_OUT, HIGH);
    power_on = true;
  }
  else{
    digitalWrite(LED, LOW);
    digitalWrite(PWR_OUT, LOW);
    at("AT+SLEEP");
    power_on = false;
  }
}
