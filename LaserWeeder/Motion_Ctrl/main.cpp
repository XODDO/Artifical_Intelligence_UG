#include <math.h>
/*  XODDOCODE 1ST May 2025
 *  SMART REMOTE CONTROL
 *  ENABLE WIRELESS MOTION CONTROL
 *  SHOW POSITION OF THE OBJECT
 *  PROJECT IMAGE OF THE SUBJECT INFRONT OF THE VEHICLE

 */  

//   WITH TEXT BOX MODEL IMPLEMENTED vs ALL SCREEN UPDATE

#include <esp_now.h>
#include <WiFi.h>

//       uint8_t TractorEngine[] = {0xF0, 0xF5, 0xBD, 0x43, 0x6C, 0x18};  // Tractor Motor Controller ... ESP32 S3
 //      uint8_t TractorEngine[] = {0xD0, 0xEF, 0x76, 0x45, 0xCB, 0xF8};  // Tractor Motor Controller ... ESP32 
         uint8_t TractorEngine[] = {0x88, 0x13, 0xBF, 0x0C, 0x1F, 0xD8};
      // Peer info 
       esp_now_peer_info_t peerInfo;  
  

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"


uint8_t sending = 12, moving = 14;
uint8_t camera_shutter = 26, wirelessLED = 27;
//uint8_t wirelessLED = 25; 

uint8_t vibrator = 17;
uint8_t buzzer   = 25; // 16;

uint8_t backlight_ = 4;



// For the Adafruit shield, these are the default.
uint8_t TFT_DC = 15;
uint8_t TFT_CS = 5; //39; //5;//39;
uint8_t RST = 13;      //int8_t RST = 4;

uint8_t TFT_MOSI = 23;
uint8_t TFT_MISO = 19;
uint8_t TFT_CLK = 18;


 //  If using the breakout, change pins as desired
 //  Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, RST, TFT_MISO); //12X slower
     Adafruit_ILI9341 LCD(TFT_CS, TFT_DC, RST); // HARDWARE SPI

uint8_t currentScreen = 1;

typedef struct drive{
  
  uint8_t nav_direction;
  
  char timestamp[20];


  drive(){ //constructor to initializing the variables!
    strcpy(timestamp, "8:00:10");

    nav_direction = 0;
  
  }
  
} drive; 

drive senderObj;


 uint8_t y_axis = 34;
 uint8_t x_axis = 35;

 uint8_t forward = 32, reverse = 33;
 uint16_t forward_reverse = 0; bool going_forward = false, going_reverse = false;

 uint16_t right_left = 0;      bool going_left = false, going_right = false;


 unsigned long duration  = 0; int64_t now_now = 0, boot_start = 0; float boot_duration = 0.00;


void setup() { boot_start = esp_timer_get_time();
          Serial.begin(115200);
        
          pinMode(sending, OUTPUT); digitalWrite(sending, HIGH);
          pinMode(moving, OUTPUT);    digitalWrite(moving, HIGH);
          pinMode(camera_shutter, OUTPUT); digitalWrite(camera_shutter, HIGH);
         // pinMode(wifi_on,  OUTPUT); digitalWrite(wifi_on, HIGH);

          pinMode(wirelessLED, OUTPUT); digitalWrite(wirelessLED, HIGH);

        // BACK LIGHT
          pinMode(backlight_, OUTPUT); digitalWrite(backlight_, HIGH);

          LCD.begin();
          LCD.setRotation(3); // for the 2.4" screen

          LCD.fillScreen(ILI9341_BLACK);
          delay(200);

      
      
     // Initilize ESP-NOW
        WiFi.mode(WIFI_STA);

 
      if (esp_now_init() != ESP_OK) {
            Serial.println("Error initializing ESP-NOW");
            //return;
      }

 // Register callback function
  esp_err_t result = esp_now_register_send_cb(OnDataSent); // Serial.print("CALLBACK: ");Serial.println(result);
    if(result == ESP_OK) Serial.println("Call Back successfully set!"); 
 

  // Register peer
  memcpy(peerInfo.peer_addr, TractorEngine, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
    //return;
  }

   
      LCD.fillScreen(ILI9341_YELLOW);
      delay(200);
      
      LCD.fillScreen(ILI9341_RED);
      
      delay(500); 
      
      Serial.println("Done Booting!"); Serial.println();
      now_now = esp_timer_get_time();
      boot_duration = (now_now - boot_start)/1000000.0;
      Serial.print("TOTAL Boot Duration: "); Serial.print(boot_duration, 2); Serial.println(" seconds");
 
  pinMode(x_axis, INPUT); pinMode(y_axis, INPUT);
  delay(100);




  
    pinMode(vibrator, OUTPUT); digitalWrite(vibrator, HIGH); delay(150); digitalWrite(vibrator, LOW); 

    pinMode(buzzer, OUTPUT); digitalWrite(buzzer, HIGH); delay(100);digitalWrite(buzzer, LOW);

 
 
    Serial.println("Done Booting!"); currentScreen = 1; updateDisplay();

    digitalWrite(wirelessLED, LOW); digitalWrite(camera_shutter, LOW); digitalWrite(sending, LOW); digitalWrite(moving, LOW);
 
}


int sec = 0, min_ = 0, hr = 0;

uint32_t refresh_rate = 0;
uint32_t ticker_timer = 0;
uint64_t last_tapped = 0;

uint8_t direction = 0;

bool wipe = false;
bool can_send = false;
bool drive_command_made = false;

char WhereTo[25] = "";

bool tractor_reached = false;
bool controller_toggled = false;

void loop(void) {
  now_now = esp_timer_get_time()/1000ULL;
 
 //READ DIRECTION TWICE EVERY SECOND
    if(now_now - last_tapped >= 100){ // 100X every second

        drive_command_made =   joysticks();
        if(drive_command_made) { //if(wipe) wipe = false;
           calculate_navigation();
           drive_tractor();
           updateDisplay();
        }  
   //     if(!wipe) updateDisplay(); // Clear back to empty nav buttons indicators
    
          ticker_timer++;
          last_tapped = now_now;

      if(ticker_timer % 10 == 0){ sec++;  updateDisplay(); if(drive_command_made) { Serial.print("DRIVE COMMAND: "); Serial.println(drive_command_made); } }
    }
        //refresh_rate = (esp_timer_get_time() - now_now)/1000; Serial.print("sec = "); Serial.println(sec);
    //Serial.print("Screen Update Duration: "); Serial.print(refresh_rate); Serial.println(" milliseconds");


}




uint16_t orig_tractor_x_pos = 296; uint16_t current_tractor_x = 296;
uint16_t orig_tractor_y_pos = 195; uint16_t current_tractor_y = 195;
   

bool direction_changed = false;

void calculate_navigation(){ 

           if(forward_reverse > 3500){  direction = 1; direction_changed = true;
             // can_send = true;
             // Serial.println(); Serial.print("Y => "); Serial.println(forward_reverse);
      }

      else if(forward_reverse < 1000){  direction = 2; direction_changed = true;
             // can_send = true;
             //Serial.println(); Serial.print("Y => "); Serial.println(forward_reverse);
      }

      else  direction_changed = false; //toggled = false; 
      

          if(right_left < 1000){  direction = 4; direction_changed = true;
            // can_send = true;
           //  Serial.println(); Serial.print("X => "); Serial.println(right_left);
     }

      else if(right_left > 3500){  direction = 3; direction_changed = true;
         //  can_send = true;
          //  Serial.println(); Serial.print("X => "); Serial.println(right_left);
      }


    //  else { direction_changed = false;  }
     
        


  //    Serial.println(toggled?"User input received!":"NO COMMAND SEEN");
     // Serial.println(toggled?"User input received!":"");
    //  if(toggled) Serial.println("User input received!");
    
    //  Serial.print("Going: "); Serial.println(WhereTo);

}



  bool lock_press = false, passed_yet = false;
  unsigned long start_counting = 0;

uint16_t buttoncount = 0;
bool locked = false; bool long_locked = false; 
bool pressed = false; bool firstPass = false;

bool joysticks(){ bool toggled = false;
      forward_reverse = analogRead(y_axis); delay(5); 
      right_left      = analogRead(x_axis); delay(5);

              if(forward_reverse > 3500 || forward_reverse < 1000 || right_left > 3500 || right_left < 1000) { if(!firstPass) {pressed = true; start_counting = now_now; firstPass = true;  }} //start_counting = now_now;
            else { pressed = false;  firstPass = false; start_counting = now_now; buttoncount = 0; locked = false; long_locked = false;  }

               if(pressed){       
                    if((now_now - start_counting) >= 50 && (now_now - start_counting) < 300) { // short press
                             if(!locked){ locked = true;  // wipe = false; 
                                  Serial.print("\tPressed: IN Short Press ");  Serial.println(buttoncount); 
                                  toggled = true;
                             }
                    }

                    if((now_now - start_counting) >= 1000){ // if u kulemezaamu
                        toggled = true;
                    }
               }

        return toggled;
}

uint32_t sendings = 0; char camera_CMD[500]; //shutter, videostream etc
void drive_tractor(){ //work o best possible averages n send the result 

    senderObj.nav_direction = direction;

    esp_err_t result = esp_now_send(TractorEngine, (uint8_t*) &senderObj, sizeof(senderObj));
    
    if(result == ESP_OK)  {  Serial.println(" Sent Successfully");
    sendings++; //Sending Round
      digitalWrite(sending, HIGH); 
          delay(50);
      digitalWrite(sending, LOW);

  }

      Serial.print("Direction: "); 
    if(senderObj.nav_direction == 1) strcpy(WhereTo, "FORWARD");
    if(senderObj.nav_direction == 2) strcpy(WhereTo, "REVERSE");
    if(senderObj.nav_direction == 3) strcpy(WhereTo, "RIGHT");
    if(senderObj.nav_direction == 4) strcpy(WhereTo, "LEFT");

Serial.println(WhereTo); 
}    


char SystemTime[32] = "08:56:25";

  void updateDisplay(){
      
        if(currentScreen == 1) homeScreen(); 
   else if(currentScreen == 2) cameraScreen();  
}
   
char where_is_it[100] = "";
char direction_of_motion[100] = "";

bool now_home = false;
char pad_timer[15] = ".";
void homeScreen(){

  if(!now_home){ 
    //HEADER
      header();

      sub_header();

           
    //BODY      
       
     
   
    now_home = true;  
    }

//DYNAMIC DATA

    running_timer();  //Serial.println(pad_timer);
    LCD.fillRect(260, 0, 320, 25, ILI9341_RED); // 320p x 240p display
    LCD.setCursor(270, 10); LCD.setTextSize(1); LCD.setTextColor(ILI9341_WHITE);  LCD.print(pad_timer);

       
   
          if(drive_command_made)  {     
            
            if(tractor_reached) {

              switch(direction){
                case 1: strcpy(direction_of_motion, "Rover Moving Forward");
                 break;
                case 2: strcpy(direction_of_motion, "Rover Moving Backward");
                 break;
                
                case 3: strcpy(direction_of_motion, "Rover Steering Right");
                 break;
                case 4: strcpy(direction_of_motion, "Rover Steering Left");
                 break;

            
              }

                 //           LCD.fillRect(50, 100, 210, 140, ILI9341_BLACK); //one text box all the four arrows

                   LCD.fillRect(50, 92, 260, 30, ILI9341_BLACK); //one text box all the four arrows
                  
             LCD.setCursor(50, 100); LCD.setTextColor(ILI9341_GREEN); LCD.setTextSize(2);
                   LCD.print(direction_of_motion);

                  LCD.fillRoundRect(70, 125, 170, 75, 5, ILI9341_GREENYELLOW);
                //  LCD.fillCircle(120, 160, 30, ILI9341_BLACK);
                //    LCD.fillCircle(120, 140, 5, ILI9341_BLACK);  LCD.fillCircle(120, 160, 5, ILI9341_BLACK);  LCD.fillCircle(120, 180, 5, ILI9341_BLACK); // VERTICAL CIRCLES
                  LCD.fillCircle(100, 160, 5, ILI9341_BLACK);  LCD.fillCircle(120, 160, 5, ILI9341_BLACK);  LCD.fillCircle(140, 160, 5, ILI9341_BLACK);
                  LCD.drawCircle(160, 160, 5, ILI9341_BLACK); LCD.drawCircle(180, 160, 5, ILI9341_BLACK);   LCD.drawCircle(200, 160, 5, ILI9341_BLACK);
                       //tractor(296, 195);
                if(current_tractor_y > orig_tractor_y_pos) { LCD.fillRect(current_tractor_x-4, current_tractor_y-1, 16, 16, ILI9341_BLACK);}

                //THE TRACK
                  track(300, 200); //continuous track from down upwards
            
                //THE MOVER
                  tractor(current_tractor_x, current_tractor_y); 

                  
                  strcpy(where_is_it, "Analysing Crops...");
                  //  LCD.setCursor(40, 210); LCD.setTextSize(2); 
                  LCD.setTextColor(ILI9341_GREEN);
             }

            if(!tractor_reached){ // SHOW CAMERA
                 
            //     LCD.fillRect(50, 100, 210, 140, ILI9341_BLACK); //one text box all the four arrows
                LCD.fillRect(50, 90, 300, 200, ILI9341_BLACK);
                  ctrl_symboloz_empty();
                  ctrl_symboloz();
                
                //THE MOVER
                  tractor(current_tractor_x, current_tractor_y); 

                  LCD.fillRoundRect(40, 205, 240, 27, 2, ILI9341_WHITE);
                  strcpy(where_is_it, "Rover Out of Reach!");
  
                  LCD.setCursor(295, (current_tractor_y-30)); LCD.setTextSize(2); LCD.setTextColor(ILI9341_ORANGE);                   
                  LCD.print("?");

                   LCD.setTextColor(ILI9341_RED);

              }

                         LCD.setCursor(50, 210); LCD.setTextSize(2); LCD.print(where_is_it);

          
          }

         
         if(!drive_command_made) {
              
              strcpy(direction_of_motion, "Rover is not moving!"); 
              
            }





}



void running_timer(){
      char temp_holding[9] = "";  char temp_sec[4] = ""; char temp_min[4] = "00"; char temp_hr[4] = "00";
      
     // ltoa(now_now/1000, temp_holding, 10);


      if(sec >= 60) { sec = 0; min_++; }
      if(min_ >= 60) { hr++; }

      itoa(sec, temp_sec, 10); itoa(min_, temp_min, 10); if(hr > 0) { itoa(hr, temp_hr, 10);}


      strcpy(pad_timer, temp_hr); strcat(pad_timer, ":"); 
      if(min_ < 10) strcat(pad_timer, "0");  strcat(pad_timer, temp_min); strcat(pad_timer, ":"); 
      if(sec < 10) strcat(pad_timer, "0");  strcat(pad_timer, temp_sec);

}



 void track(uint16_t xo, uint16_t yo){
              LCD.drawFastVLine(xo, yo, -50, ILI9341_YELLOW);
              LCD.drawFastHLine(xo, (yo-50), -30, ILI9341_YELLOW);
              LCD.drawFastVLine((xo-31), (yo-50), -60, ILI9341_YELLOW);
   }

void ctrl_symboloz_empty(){ //one text box all the four arrows

      LCD.fillRect(50, 100, 210, 140, ILI9341_BLACK); 
           
               LCD.drawTriangle(140, 140, 160, 100, 180, 140, ILI9341_RED);    // UP
               LCD.drawTriangle(140, 200, 160, 240, 180, 200, ILI9341_RED); //  DOWN
               LCD.drawTriangle(200, 150, 240, 170, 200, 190, ILI9341_RED);  //   RIGHT
               LCD.drawTriangle(120, 150, 80, 170, 120, 190,  ILI9341_RED); //    LEFT
}


void ctrl_symboloz(){
    if(direction_changed){ 
          //TEXT BOX MODEL --- navigation cleaner
            LCD.fillRect(50, 100, 210, 140, ILI9341_BLACK); //one text box all the four arrows

          //  MOTION CONTROLS    

                if(direction == 1) up(); 
            else if(direction == 2) down();
            else if(direction == 3) right();
            else if(direction == 4) left();

            else direction = 0;

       
 //    direction++;
    }  else { 
  
            if(!drive_command_made){ //EMPTY THE BOXES --- navigation cleanser

                        if(!wipe){ ctrl_symboloz_empty(); wipe = true; Serial.println("Quantitative Cleansing");
                
                        }
                }
          
    
    
    }
}

void header(){
              LCD.fillRect(0, 0, 320, 25, ILI9341_RED); // 320p x 240p display
              LCD.setTextColor(ILI9341_WHITE);  LCD.setTextSize(1);
           //   LCD.setCursor(1, 10);        LCD.print("IntelliSys"); 
              LCD.setCursor(100, 6); LCD.setTextSize(2); LCD.print("FREDMARKIAN"); 
          //    LCD.setCursor(270, 10); LCD.setTextSize(1); LCD.print(now_now/1000);
} 


void  sub_header(){      
            //LCD.fillRect(0, 20, 480, 239, ILI9341_DARKGREEN);
              LCD.fillRect(0, 25, 320, 200, ILI9341_WHITE); //UPPER MIDDLE CLASS
              LCD.fillRect(0, 90, 320, 150, ILI9341_BLACK);//MIDDLE CLASS
              
              LCD.setTextColor(ILI9341_BLACK);

              LCD.setTextSize(3); //LCD.setFont(ArialBlack_8_data);
              LCD.setCursor(35, 35); LCD.print("CONTROLLER PAD");
              
              LCD.setTextSize(2);
              LCD.setCursor(50, 65); LCD.print("Crop Health Monitor");
}


   void tractor(uint16_t xx, uint16_t yy){
              LCD.fillRect(xx, yy, 8, 15, ILI9341_CYAN);   //Nose
              LCD.fillRect(xx-4, yy+12, 16, 12, ILI9341_CYAN); // Body
              LCD.fillRect(xx-4, yy+2, 2, 7, ILI9341_CYAN);  //  FRONT WHEEL 1
              LCD.fillRect(xx+10, yy+2, 2, 7, ILI9341_CYAN); //   FRONT WHEEN 2
              LCD.drawFastHLine(xx-3, yy+5, 15, ILI9341_CYAN);//    CONNECTING SHAFT
   }  


void textBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour){
  LCD.drawRect(x, y, w, h, colour);
}

void up(){
               LCD.fillTriangle(140, 140, 160, 100, 180, 140, ILI9341_GREEN);    // UP
               LCD.drawTriangle(140, 200, 160, 240, 180, 200, ILI9341_RED); //  DOWN
               LCD.drawTriangle(200, 150, 240, 170, 200, 190, ILI9341_RED);  //   RIGHT
               LCD.drawTriangle(120, 150, 80, 170, 120, 190,  ILI9341_RED); //    LEFT

}


void down(){
               LCD.drawTriangle(140, 140, 160, 100, 180, 140, ILI9341_RED);    // UP
               LCD.fillTriangle(140, 200, 160, 240, 180, 200, ILI9341_GREEN); //  DOWN
               LCD.drawTriangle(200, 150, 240, 170, 200, 190, ILI9341_RED);  //   RIGHT
               LCD.drawTriangle(120, 150, 80,  170, 120, 190, ILI9341_RED); //    LEFT

}

void right(){
               LCD.drawTriangle(140, 140, 160, 100, 180, 140, ILI9341_RED);    // UP
               LCD.drawTriangle(140, 200, 160, 240, 180, 200, ILI9341_RED); //  DOWN
               LCD.fillTriangle(200, 150, 240, 170, 200, 190, ILI9341_GREEN);  //   RIGHT
               LCD.drawTriangle(120, 150, 80,  170, 120, 190, ILI9341_RED); //    LEFT

}

void left(){
               LCD.drawTriangle(140, 140, 160, 100, 180, 140, ILI9341_RED);    // UP
               LCD.drawTriangle(140, 200, 160, 240, 180, 200, ILI9341_RED); //  DOWN
               LCD.drawTriangle(200, 150, 240, 170, 200, 190, ILI9341_RED);  //   RIGHT
               LCD.fillTriangle(120, 150, 80,  170, 120, 190, ILI9341_GREEN); //    LEFT

}


uint32_t delivereds = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  //Serial.print("\r\nLast Packet Send Status:\t");
 // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Successfully Delivered!" : "Failed 2 Delivery!");
    Serial.println();
    if(status == ESP_NOW_SEND_SUCCESS){ tractor_reached = true;    digitalWrite(moving, HIGH); digitalWrite(buzzer, HIGH); delay(50); digitalWrite(buzzer, LOW); digitalWrite(moving, LOW); delivereds++; }
    if(status != ESP_NOW_SEND_SUCCESS) { tractor_reached = false;  digitalWrite(vibrator, HIGH); delay(50); digitalWrite(vibrator, LOW); }

  Serial.println(tractor_reached?"[---REACHED---]":"[---ROVER FWAAA---]");
}


void cameraScreen(){

}



/*
void updateClock(){
      second += 1;
      if(second == 60){ minute++; second = 0; }
      if(minute == 60){hour++; minute = 0;}
    
      //ONLY WRITE CLOCK TO EEPROM, once 3v3ry 30mins:
    if(second == 0){  
        if(minute == 0 || minute == 30){ // 6X/HR::thus 347 - 600 days
            EEPROM.write(EEPROM_ADDRESS, hour);
            EEPROM.write(EEPROM_ADDRESS+1, minute);
            EEPROM.write(EEPROM_ADDRESS+2, second);
            EEPROM.commit();
          }
        //ONLY WRITE CLOCK TO EEPROM ONCE A DAY CHANGES: 100,000 DAYS:
        if(hour == 24){day++; hour = 0;
            EEPROM.write(EEPROM_ADDRESS+3, day);
            EEPROM.write(EEPROM_ADDRESS+6, month);
            EEPROM.write(EEPROM_ADDRESS+7, year);
            EEPROM.commit();
        }

        if(day != 0 && day%30 == 0)  EEPROM_ADDRESS+=5; //every 30 days, shift the location
    }
  
 }



*/


 
