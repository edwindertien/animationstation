/********************************************************************************
  Servo Recorder using 
  - Korg nanoKONTROL2 panel
  - Adafruit 16-channel PWM driver
  - Arduino MEGA ADK
  
  The fader panel should be configured to have the lights controlled externally
  using the korg utility.
  
  Faders control 8 servo channels, the knobs the offset per channel. 
  Fader positions are recorded, the knobs are -always- in use for adjustment
  
  All 'normal' functions are implemented: record, play, stop, step forward/step backward
  Play will play a recorded track from begin till end. With 'cycle' pressed it 
  will loop at the end. 
  
  tracks have to be 'armed' for record using the R-buttons to make sure they are recorded
  
  Currently a track of 30 seconds will be recorded. Longer tracks are possible by increasing 
  the amount of steps and perhaps reducing the sample rate
  
  The Adafruit 16-channel PWM & Servo driver has been used with the library from 
  https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
  and uses I2C to communicate, 2 pins are required to
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4
  Written by Limor Fried/Ladyada for Adafruit Industries. BSD license

  Other example used is the USB-MIDI to Legacy Serial MIDI converter 
  Copyright 2012-2013 Yuuichi Akagawa using an idea from LPK25 USB-MIDI 
  to Serial MIDI converter by Collin Cunningham - makezine.com, narbotic.com
  
  for use with USB Host Shield 2.0 from Circuitsathome.com
  https://github.com/felis/USB_Host_Shield_2.0
*/

#include <Usb.h>
#include <usbh_midi.h>

#include <Braccio.h>
#include <Servo.h>

Servo base;
Servo shoulder;
Servo elbow;
Servo wrist_rot;
Servo wrist_ver;
Servo gripper;

   /*
   Step Delay: a milliseconds delay between the movement of each servo.  Allowed values from 10 to 30 msec.
   M1=base degrees. Allowed values from 0 to 180 degrees
   M2=shoulder degrees. Allowed values from 15 to 165 degrees
   M3=elbow degrees. Allowed values from 0 to 180 degrees
   M4=wrist vertical degrees. Allowed values from 0 to 180 degrees
   M5=wrist rotation degrees. Allowed values from 0 to 180 degrees
   M6=gripper degrees. Allowed values from 10 to 73 degrees. 10: the toungue is open, 73: the gripper is closed.
  */

int SERVOMIN [] = {0,15,0,0,0,10};
int SERVOMAX [] = {180,165,180,180,180,73};
int sendValue[8];

#define MAX_STEPS 500 // this is the amount of steps that can be recorded in memory, reserved as globals * 10 byte!
#define LOOPDELAY 50  // determines the loop frequency in ms. 50 ms for 20 Hz, 100 ms for 10Hz, etc
#define TIMEOUT 10    // time in 'loops' of inactivity (not direct control/play by hand/recording) before all servo's are turned off

typedef struct {
  unsigned int time;
  byte value[8];
} midiAction;

midiAction set[MAX_STEPS];  // this eats dynamic memory!

USB  Usb;
MIDI Midi(&Usb);

// globals for storing panel information
byte inputBuffer[8];
byte offsetBuffer[8];
byte lastOffsetBuffer[8];
byte lastInputBuffer[8];
byte recordEnable[8];
byte muteEnable[8];
byte soloChannel;
byte lastSoloChannel;
boolean cycle, blinkstate;

#define RECORD 2
#define STOP 0
#define PLAY 1
int mode;
int memoryCounter;
unsigned long looptime, starttime, blinktimer, idletimer;

void setup(){
  Serial.begin(115200);
  //Workaround for non UHS2.0 Shield
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  //All the servo motors will be positioned in the "safety" position:
  //Base (M1):90 degrees
  //Shoulder (M2): 45 degrees
  //Elbow (M3): 180 degrees
  //Wrist vertical (M4): 180 degrees
  //Wrist rotation (M5): 90 degrees
  //gripper (M6): 10 degrees
  Braccio.begin();
 //base.attach(11);
 //
  
  delay( 200 );
}
void loop() {
  Usb.Task(); // do the USB thing
  if (millis() > looptime + LOOPDELAY - 1) {
    looptime = millis();
    int changed = 0; // only output to screen when input has changed
    for (int i = 0; i < 8; i++) {
      if (inputBuffer[i] != lastInputBuffer[i]) changed++;
      else if (offsetBuffer[i] != lastOffsetBuffer[i]) changed++;
      
      lastInputBuffer[i] = inputBuffer[i];
      lastOffsetBuffer[i] = offsetBuffer[i];
    }
    if (changed > 0 && mode == STOP) {
      directaction(); // direct control in stop mode
      idletimer=0;
    }
   else if (mode == RECORD) {
      set[memoryCounter].time = (int)((millis() - starttime)) / LOOPDELAY;
      idletimer=0;
      for (int i = 0; i < 8; i++) {
        if (recordEnable[i]) set[memoryCounter].value[i] = inputBuffer[i];
      }
      //////// OUTPUT ///////////
      printaction(memoryCounter);
      ///////////////////////////
      memoryCounter += 1;
      if (memoryCounter > MAX_STEPS - 5 * (1000 / LOOPDELAY)) {
        if (millis() > blinktimer) {
          if (blinkstate) blinkstate = 0; else blinkstate = 1;
          byte sendBuf[] = {0xb0, 45, (byte)blinkstate * 127};
          Midi.SendData(sendBuf, 0);
          blinktimer = millis() + 500;
        }
      }
      if (memoryCounter > MAX_STEPS - 1) {
        stop();
      }
    }
    else if (mode == PLAY) {
      idletimer=0;
      if (set[memoryCounter].time < ((millis() - starttime) / LOOPDELAY)) {
        //////// OUTPUT ///////////
        printaction(memoryCounter);
        ///////////////////////////
        memoryCounter++;
        if (set[memoryCounter].time == 0 || memoryCounter > MAX_STEPS - 1) {
          if (!cycle) stop(); else start();
        }
      }
    }
    else idletimer++;
    if((idletimer)>TIMEOUT) {stop(); idletimer=0;}
  }
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
    MIDI_poll();
  }
  delay(1);
}
void directaction() {
  Serial.print("s");
  Serial.print("\t");
  Serial.print("t");
  Serial.print("\t");

   
  
  for (int i = 0; i < 8; i++) {
    if (muteEnable[i]) Serial.print("-");
    else {     
      sendValue[i] = map(inputBuffer[i] - offsetBuffer[i], -127, 127, SERVOMIN[i], SERVOMAX[i]);
      Serial.print(sendValue[i]);
    }
    Serial.print("\t");
  }
  Serial.println("<");
  Braccio.ServoMovement(1,sendValue[0],sendValue[1],sendValue[2],sendValue[3],sendValue[4],sendValue[5]);
}
void printaction(int step) {
  Serial.print("s ");
  Serial.print(step);
  Serial.print("\t");
  Serial.print("t ");
  Serial.print(set[step].time);
  Serial.print("\t");
  if (soloChannel > 0) { // only one channel for output
//    Serial.print(set[step].value[soloChannel - 1] - offsetBuffer[soloChannel - 1]);
//    pwm.setPWM(soloChannel - 1, 0, map(set[step].value[soloChannel - 1] - offsetBuffer[soloChannel - 1], -127, 127, SERVOMIN, SERVOMAX));
 
      sendValue[soloChannel-1] =  map(set[step].value[soloChannel-1] - offsetBuffer[soloChannel-1], -127, 127, SERVOMIN[soloChannel-1], SERVOMAX[soloChannel-1]);
      Serial.print(sendValue[soloChannel-1]);
  }
  else {
    for (int i = 0; i < 8; i++) {
      if (muteEnable[i]) Serial.print("-");   // and do nothing..
      else {                                  // or do the output to the PWM shield
        //Serial.print(set[step].value[i] - offsetBuffer[i]);
    //    pwm.setPWM(i, 0, map(set[step].value[i] - offsetBuffer[i], -127, 127, SERVOMIN, SERVOMAX));
      sendValue[i] = map(set[step].value[i] - offsetBuffer[i], -127, 127, SERVOMIN[i], SERVOMAX[i]);
      Serial.print(sendValue[i]);
    
      }
      Serial.print("\t");
    }
  }
  Serial.println(" <");
  Braccio.ServoMovement(1,sendValue[0],sendValue[1],sendValue[2],sendValue[3],sendValue[4],sendValue[5]);

}
void stop() {
  mode = STOP;

  byte sendBuf[] = {0xb0, 45, 0};
  Midi.SendData(sendBuf, 0);
  sendBuf[1] = 41;
  Midi.SendData(sendBuf, 0);
  Serial.print("stop at step: ");
  Serial.println(memoryCounter);
}
void start() {
  mode = PLAY;
  starttime = millis();
  memoryCounter = 0;
  byte sendBuf[] = {0xb0, 41, 127};
  Midi.SendData(sendBuf, 0);
}
void record() {
  mode = RECORD;
  byte sendBuf[] = {0xb0, 45, 127};
  Midi.SendData(sendBuf, 0);
  starttime = millis();
  memoryCounter = 0;
}
void MIDI_poll() {  // Poll USB MIDI Controler and store the data
  uint8_t size;
  byte outBuf[ 3 ];
  do {
    if ( (size = Midi.RcvData(outBuf)) > 0 ) {
      // This bit is specific for the KORG nanoKONTROL2 panel
      // It uses the factory settings for the panel, with one change: button mode (LED's) is 
      // set to EXTERNAL so our control box can light them... 
      // Serial.println(outBuf[1]); // For debugging print the channel as serial output
      for (int i = 0; i < 8; i++) {              // faders
        if (outBuf[1] == i) {
          inputBuffer[i] = outBuf[2];
        }
      }
      for (int i = 16; i < 24; i++) {            // knobs
        if (outBuf[1] == i) {
          offsetBuffer[i - 16] = outBuf[2];
        }
      }
      for (int i = 48; i < 56; i++) {            // mute buttons
        if (outBuf[1] == i && outBuf[2] == 127) {
          if (muteEnable[i - 48] == 0) {
            muteEnable[i - 48] = 1;
            byte sendBuf[] = {0xb0, i, 127};
            Midi.SendData(sendBuf, 0);
          }
          else {
            muteEnable[i - 48] = 0;
            byte sendBuf[] = {0xb0, i, 0};
            Midi.SendData(sendBuf, 0);
          }
        }
      }
      for (int i = 32; i < 40; i++) {            // solo buttons
        if (outBuf[1] == i && outBuf[2] == 127) {
          soloChannel = i - 31;
          if (lastSoloChannel == soloChannel && lastSoloChannel > 0) {
            soloChannel = 0;
          }
          byte sendBuf[] = {0xb0, i, 127};
          Midi.SendData(sendBuf, 0);
          sendBuf[1] = lastSoloChannel + 31;
          sendBuf[2] = 0;
          Midi.SendData(sendBuf, 0);
          lastSoloChannel = soloChannel;
        }
      }
      for (int i = 64; i < 72; i++) {            // record buttons
        if (outBuf[1] == i && outBuf[2] == 127) {
          if (recordEnable[i - 64] == 0) {
            recordEnable[i - 64] = 1;
            byte sendBuf[] = {0xb0, i, 127};
            Midi.SendData(sendBuf, 0);
          }
          else {
            recordEnable[i - 64] = 0;
            byte sendBuf[] = {0xb0, i, 0};
            Midi.SendData(sendBuf, 0);
          }
        }
      }
      if (outBuf[1] == 45 && outBuf[2] == 127) {  // record
        record();
      }
      if (outBuf[1] == 41 && outBuf[2] == 127) {  // play
        start();
      }
      if (outBuf[1] == 42 && outBuf[2] == 127) {  // stop 
        stop();
      }
      if (outBuf[1] == 43 && outBuf[2] == 127) {  // step back
        stop();
        if (memoryCounter > 0) memoryCounter --;
        printaction(memoryCounter);
      }
      if (outBuf[1] == 44 && outBuf[2] == 127) {  // step forward
        stop();
        if (memoryCounter < MAX_STEPS) memoryCounter++;
        printaction(memoryCounter);
      }
      if (outBuf[1] == 46 && outBuf[2] == 127) {  // toggle cycle (continuous play) 
        if (cycle == 0) {
          cycle = 1;
          byte sendBuf[] = {0xb0, 46, 127};
          Midi.SendData(sendBuf, 0);
        } else {
          cycle = 0;
          byte sendBuf[] = {0xb0, 46, 0};
          Midi.SendData(sendBuf, 0);
        }
      }
    }
  } while (size > 0);
}


