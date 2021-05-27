#include <OneButton.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>

#define PWR_IN 6
#define PWR_OUT 7
#define VTH 14
#define LED 10

#define SE 0.02722
#define ADC2V 0.0049
#define GAIN 496.49
#define AMBIENT 70
#define EXP 4

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool power_on = false;

double temp = 0;

OneButton pwr_in = OneButton(PWR_IN, true, true);

void setup() {
  // put your setup code here, to run once:
  pinMode(VTH,INPUT);
  pinMode(PWR_OUT, OUTPUT);
  pinMode(LED, OUTPUT);

  pwr_in.attachClick(switchLED);
  
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

  digitalWrite(LED, LOW);
}

void loop() {

  display.clearDisplay();
  display.setCursor(25, 9);
  temp = analogRead(VTH)*ADC2V-2.5;
  temp /= SE;
  temp += pow(AMBIENT,EXP);
  temp = pow(temp, 1/EXP);
  display.print(temp);
  display.display();
  delay(10);

  pwr_in.tick();

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
    power_on = false;
  }
}
