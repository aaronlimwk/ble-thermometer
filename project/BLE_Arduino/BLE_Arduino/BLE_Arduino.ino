#include <SoftwareSerial.h>

#define baud 9600

char message[100];
bool message_incoming = false;

SoftwareSerial mySerial(9,8); // RX, TX
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(baud);
  mySerial.begin(baud);
  
  at("AT"); // check if working, always returns OK
  at("AT+ROLE0"); // select master = central
  at("AT+IMME0"); // "work immediately", not sure what this does
  at("AT+RESET"); // actually more a restart than a reset .. needed after ROLE
  at("AT"); // check if working, always returns OK
  
  delay(1000); // wait a bit, NECESSARY!!
//  Serial.print("Waiting for connection");
//  while(!mySerial.find("OK+CONN")) Serial.print(".");
//  Serial.println();
//  Serial.println("Connection Established");
}

void at(char* cmd) {
  mySerial.write(cmd);
  Serial.print(cmd);
  while(!mySerial.find("OK")) Serial.print(".");
  
  Serial.println(" .. OK");
}


void loop() // run over and over
{
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
  
  if (Serial.available())
    mySerial.write(Serial.read());
}
