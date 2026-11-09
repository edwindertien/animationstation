/********************************************************************************
  LoopStation using
  - AKAI APCmini
  - Arduino MEGA ADK

  This source controls a valvecontrolboard over RS485 (dynamixel protocol) using
  solenoids as exciters.

  Faders control 8 velocity channels, the buttons the beat pattern per channel.

  Other example used is the USB-MIDI to Legacy Serial MIDI converter
  Copyright 2012-2013 Yuuichi Akagawa using an idea from LPK25 USB-MIDI
  to Serial MIDI converter by Collin Cunningham - makezine.com, narbotic.com

  for use with USB Host Shield 2.0 from Circuitsathome.com
  https://github.com/felis/USB_Host_Shield_2.0

  The MIDI mapping for the AKAI APCmini

  Pads: from left to right and lowest to highest row.
  These are notes:
  Row1: 0-7
  Row2: 8-15
  Row3: 16-23
  Row4: 24-31
  Row5: 32-39
  Row6: 40-47
  Row7: 48-55
  Row8: 56-63

  The velocity values for the different lightings are:
  00:off
  01:green
  02:green blink
  03:red
  04:red blink
  05:amber
  06: amber blink
  07-127: green

  The extra button row on the bottom left to right:
  64-71
  the extra button column on the right: top to bottom
  82-89
  The velocity values for the different lightings are:
  00:off
  01:red
  02:red blink
  03-127: red

  The shift button is note 98. I could not get it to light up.

  The buttons always send a velocity of 127.
  When pressed a note on message and when released a note off message.
  The Midi channel is channel 1
*/
#define DEBUG (1)
#include <Usb.h>
#include <usbh_midi.h>
USB  Usb;
MIDI  Midi(&Usb); //(sometimes USBH_MIDI
byte sendBuf[4]; // as global

#define LOOPDELAY 50  // determines the loop frequency in ms. 50 ms for 20 Hz, 100 ms for 10Hz, etc
unsigned long looptime;

// buffers for the panel (and track)
byte buttons[64];
byte track[64];
volatile byte inputBuffer[8];  // for the chanel sliders
byte tempoSlider;     // for the tempo

void setup() {
  Serial.begin(115200);
  Serial2.begin(1000000); // RS485 bus to robot

  pinMode(22, OUTPUT); //s-r

  //Workaround for non UHS2.0 Shield
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
  for (int n = 0; n < 64; n++) {
    buttons[n] = 0;
  }
  for (int n = 0; n < 8; n++) {
    inputBuffer[n] = 1;
  }
  tempoSlider = 32; // first value
}
int step, counter;
void loop() {
  Usb.Task(); // do the USB thing
  if (millis() > looptime + LOOPDELAY - 1) {
    looptime = millis();
    counter += (tempoSlider);
    if (counter > 120) {
      step++;
      counter = 0;
      if (step > 7) step = 0;
      for (int n = 0; n < 8; n++) {
        if (track[n + step * 8] > 0) {
          buttons[n + step * 8] = 1;
          //          Serial.print(n);
          //          Serial.print('\t');
          //          Serial.println(track[n + step * 8]);
          DimmerBoardWrite(0, 7-n, max(inputBuffer[n] / 4, 5));
        }
        else buttons[n + step * 8] = 5;
        if (step > 0) {
          if (track[n + (step - 1) * 8] > 0) buttons[n + (step - 1) * 8] = 3;
          else buttons[n + (step - 1) * 8] = 0;
        }
        if (step == 0) {
          if (track[n + (7) * 8] > 0) buttons[n + (7) * 8] = 3;
          else buttons[n + (7) * 8] = 0;
        }
      }
    }

    // do buttons
    for (int n = 0; n < 64; n++) {
      sendBuf[0] = 0x90;
      sendBuf[1] = n;
      sendBuf[2] = buttons[n];
      Midi.SendData(sendBuf, 0);
    }
  }
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
    MIDI_poll();
  }
  //delay(1);
}


void MIDI_poll() {  // Poll USB MIDI Controler and store the data
  uint8_t size;
  byte outBuf[ 3 ];
  do {
    if ( (size = Midi.RcvData(outBuf)) > 0 ) {
      #ifdef DEBUG
      Serial.print(outBuf[0]);
           Serial.print('\t');
           Serial.print(outBuf[1]);
           Serial.print('\t');
           Serial.println(outBuf[2]); // For debugging print the channel as serial output
#endif
      if (outBuf[0] == 144) {
        //Serial.println("note on");
        for (int n = 0; n < 8 ; n++) {
          for (int k = 0; k < 8; k++) {
            if (outBuf[1] == n + k * 8) {
              if (track[n + k * 8] > 0) track[n + k * 8] = 0;
              else track[n + k * 8] = inputBuffer[n] + 1;
              if (buttons[n + k * 8] == 3) buttons[n + k * 8] = 0;
              else if (buttons[n + k * 8] == 0) buttons[n + k * 8] = 3;
            }
          }
        }
        for (int z = 64; z<72; z++){
          if(outBuf[1]==z) DimmerBoardWrite(0, 71-z, 5);
        }
      }
      if (outBuf[0] == 176) {
        for (int i = 48; i < 56; i++) {            // faders
          if (outBuf[1] == i) {
            inputBuffer[i - 48] = outBuf[2];
          }
        }
        if (outBuf[1] == 56) {
          tempoSlider = outBuf[2];
        }
      }
    }
  } while (size > 0);
}
void DimmerBoardWrite(int board, int channel, int value)
{
  byte checksum = ~( board + 0x04 + 0x03 + channel + value);
  unsigned char buffer[8] = {
    0xFF, 0xFF, // Header
    board, // Slave ID
    0x04, // data length (=number of parameter bytes + 2)
    0x03, // Write command
    channel, // Parameter 1
    value, // Parameter 2
    checksum
  };
  digitalWrite(22, HIGH);
  Serial2.write(buffer, 8);
  Serial2.flush();
  digitalWrite(22, LOW);
}
