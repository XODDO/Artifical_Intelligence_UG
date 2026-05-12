  //  Serial Read from CAM
  //  Serial Write to MP3 player
  //  i2c write to OLED

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
void update_display(char message_printable[150]);
void beep(uint8_t times);

#include "HardwareSerial.h"   //  w/o this, get verbose nastygrams (orange)
#include "DFRobotDFPlayerMini.h"

// Use pins 2 and 3 to communicate with DFPlayer Mini
const byte RXD2 = 16; // Connects to module's RX 
const byte TXD2 = 17; // Connects to module's TX 

HardwareSerial FPSerial(2); // Use UART channel 1

DFRobotDFPlayerMini audio;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C LED(U8G2_R0); 

int indicator = 2;
int buzzer = 4;

int file_index = 0;
bool busy = false;


bool now_playing = false;
bool anything_found = false;
char received_buffer[500] = ""; // the entire message: e.g. object: book, certainty: 98.3%

char object[18][20] = { "cup", "plate", "orange", "bottle", "photo",
                        "onion", "book", "plant", "pineapple", "mango",
                        "phone","banana", "shoe", "screw_driver", "avocado"
                        "box", "phone", "irish"}; 


uint8_t item_index = 0;
uint8_t items_listed = 0;

void setup() { delay(500);
    Serial.begin(115200); Serial.println("Booting...");
    //Serial2.begin(9600);

    LED.begin(); Serial.println("Initializing LED Display");

    pinMode(buzzer, OUTPUT); 


    LED.clearBuffer();          // clear the internal memory
    LED.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
    LED.drawStr(0,29,"SANDRA");  // write something to the internal memory
    LED.sendBuffer();         // transfer internal memory to the display
    delay(2000); Serial.println("SANDRA");

    LED.clearBuffer();          // clear the internal memory
    LED.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
    LED.drawStr(0,29,"EDMOND");  // write something to the internal memory
    LED.sendBuffer();         // transfer internal memory to the display
    delay(2000); Serial.println("EDMUND");

    LED.clearBuffer();         // clear the internal memory
    LED.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
    LED.drawStr(1,31,"GOGGLES");  // write something to the internal memory
    LED.sendBuffer();         // transfer internal memory to the display
    delay(2000); Serial.println("GOGGLES");


//init.. mp3
 #if (defined ESP32)
      FPSerial.begin(9600, SERIAL_8N1, RXD2, TXD2); //, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);
    #else
      FPSerial.begin(9600);
    #endif


  Serial.println(F("Initializing AUDIO (May take 3~5 seconds)"));
  
  if (!audio.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
      Serial.println(F("Unable to initialize MP3 Player!"));
      Serial.println(F("1.Please recheck the connection! or"));
      Serial.println(F("2.Please insert the SD card!"));
  
  }

  else {
      Serial.println(F("MP3 MODULE INITIALIZED!"));
      
      audio.volume(27);  //Set volume value. From 0 to 30
     // audio.play(1);  //Play the first mp3 FILE

////Play the first mp3 FILE
      while(file_index <= 12){ // assuming we have 12 audio files

         if(!busy){ // tied to the busy pin of the MP3 plater
              file_index++;
              audio.play(file_index); beep(1); Serial.print("Playing Audio File: "); Serial.print(file_index); Serial.print("  "); Serial.println(object[file_index]);
              update_display(object[file_index]);
              
              
              delay(3000); // the longest file is up to 3 seconda
         }
      }
   

  }

  beep(4);
  update_display("Started!");

    pinMode(indicator, OUTPUT); digitalWrite(indicator, HIGH); delay(1000);
 
             items_listed = sizeof(object)/sizeof(object[0]); // should yield number of items in the set

    Serial.print(items_listed); Serial.println(" items listed.");
    Serial.println("Done Booting!");
  
}


bool match_made = false;
char seen_object[100] = "...";
char printabo[20] = "...";

char packet = 'A'; 
uint64_t now_now = 0, prev = 0;
uint64_t time_check = 0;
uint16_t ticker_timer = 0;

void loop() {
   now_now = esp_timer_get_time()/1000ULL;

     // if((now_now - prev) >= 100){ // 10Hz cycle ... 10X every second
        if((now_now - prev) >= 500){ // 2Hz cycle ... 2X every second


            // anything_found = false;
               match_made = false;
            
            strcpy(received_buffer, ""); //Serial.print("Receiving Buffer: "); Serial.println(received_buffer);
            
            while(Serial.available()){ anything_found = true;
                    packet = Serial.read(); 
                    char stringified_packet[2] = {packet, '\0'}; 
                    strcat(received_buffer, stringified_packet); 
                } 

                
        
            if(anything_found){  // if  ANYTHING SEEN, NO MATTER WHAT

                  //  Serial.println(); Serial.print("Number of Items: "); Serial.println(items_listed); 

                    for(int i=0; i<items_listed; i++) {
                               if(strstr(received_buffer, object[i]) != NULL){ // if the received buffer has anything that looks a bit like what's databased
                                      match_made = true; item_index = (i+1);  strcpy(seen_object, received_buffer); //full formated text
                                      strcpy(printabo, object[i]);
                                 }
                             // else Serial.println("Object Mismatch!!!!!"); 
                        } 

                           if(match_made){ Serial.println("Match Identified!"); 
                                 digitalWrite(indicator, HIGH); digitalWrite(buzzer, HIGH); delay(50);
                                 digitalWrite(indicator, LOW);digitalWrite(buzzer, LOW);
                                 
                                 Serial.print("Item Location: "); Serial.println(item_index);
                                 Serial.print("Received BUFF: => ");    Serial.print(seen_object);         //Serial.println("]");
                                 update_display(printabo); delay(100);

                          //   if(!now_playing)   { audio.play(item_index); now_playing = true; delay(2000); } // play the associated audio... separation between media play instances

                              audio.play(item_index); delay(3500); // play the associated audio... separation between media play instances

                           }

                           
                          
                
               
            } else  digitalWrite(indicator, LOW); digitalWrite(buzzer, LOW);  match_made = false;
            
        prev = now_now; time_check = prev/1000;
        anything_found = false; ticker_timer++;

        if(ticker_timer%100 == 0){ // EVERY TEN SECONDS
            Serial.print("Time Check: "); Serial.println(time_check);
        }
      

    }
  
}


void update_display(char message_printable[150]){
      // show what has been seen
                            LED.clearBuffer();
                            LED.setFont(u8g2_font_logisoso16_tr);  // choose a large font 
                            LED.drawStr(24, 21, message_printable);  // write something to the debugging 128x32p disp
                            LED.sendBuffer(); 
}



void beep(uint8_t times){

      for(int xy=0; xy<times; xy++){
        digitalWrite(buzzer, HIGH);
        
        delay(50);
        
        digitalWrite(buzzer, LOW);
        if(times > 1) delay(50);
      }
  
  }

