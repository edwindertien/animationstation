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

#include <USBHost_t36.h>

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);
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

  // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
  // use too much power, Teensy at least completes USB enumeration, which
  // makes isolating the power issue easier.
  delay(1500);
  Serial.println("USB Host InputFunctions example");
  delay(10);
  myusb.begin();
  Serial2.begin(1000000); // RS485 bus to robot

  pinMode(22, OUTPUT); //s-r
  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleAfterTouchPoly(myAfterTouchPoly);
  midi1.setHandleControlChange(myControlChange);
  midi1.setHandleProgramChange(myProgramChange);
  midi1.setHandleAfterTouchChannel(myAfterTouchChannel);
  midi1.setHandlePitchChange(myPitchChange);
  // Only one of these System Exclusive handlers will actually be
  // used.  See the comments below for the difference between them.
  midi1.setHandleSystemExclusive(mySystemExclusiveChunk);
  midi1.setHandleSystemExclusive(mySystemExclusive);
  midi1.setHandleTimeCodeQuarterFrame(myTimeCodeQuarterFrame);
  midi1.setHandleSongPosition(mySongPosition);
  midi1.setHandleSongSelect(mySongSelect);
  midi1.setHandleTuneRequest(myTuneRequest);
  midi1.setHandleClock(myClock);
  midi1.setHandleStart(myStart);
  midi1.setHandleContinue(myContinue);
  midi1.setHandleStop(myStop);
  midi1.setHandleActiveSensing(myActiveSensing);
  midi1.setHandleSystemReset(mySystemReset);
  // This generic System Real Time handler is only used if the
  // more specific ones are not set.
  midi1.setHandleRealTimeSystem(myRealTimeSystem);

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
  myusb.Task();
  midi1.read();
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
          DimmerBoardWrite(0, 7 - n, max(inputBuffer[n] / 4, 5));
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
 //     Midi.SendData(sendBuf, 0);
//myusb.send(
//
midi1.sendNoteOn(n,buttons[n],1);
    }
  }

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


void myNoteOn(byte channel, byte note, byte velocity) {
  // When a USB device with multiple virtual cables is used,
  // midi1.getCable() can be used to read which of the virtual
  // MIDI cables received this message.
  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);

  //Serial.println("note on");
  for (int n = 0; n < 8 ; n++) {
    for (int k = 0; k < 8; k++) {
      if (note == n + k * 8) {
        if (track[n + k * 8] > 0) track[n + k * 8] = 0;
        else track[n + k * 8] = inputBuffer[n] + 1;
        if (buttons[n + k * 8] == 3) buttons[n + k * 8] = 0;
        else if (buttons[n + k * 8] == 0) buttons[n + k * 8] = 3;
      }
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {
  Serial.print("Note Off, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
}

void myAfterTouchPoly(byte channel, byte note, byte velocity) {
  Serial.print("AfterTouch Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
}

void myControlChange(byte channel, byte control, byte value) {
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.println(value, DEC);

  for (int i = 48; i < 56; i++) {            // faders
    if (control == i) {
      inputBuffer[i - 48] = value;
    }
  }
  if (control == 56) {
    tempoSlider = value;
  }
}

void myProgramChange(byte channel, byte program) {
  Serial.print("Program Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", program=");
  Serial.println(program, DEC);
}

void myAfterTouchChannel(byte channel, byte pressure) {
  Serial.print("After Touch, ch=");
  Serial.print(channel, DEC);
  Serial.print(", pressure=");
  Serial.println(pressure, DEC);
}

void myPitchChange(byte channel, int pitch) {
  Serial.print("Pitch Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", pitch=");
  Serial.println(pitch, DEC);
}


// This 3-input System Exclusive function is more complex, but allows you to
// process very large messages which do not fully fit within the midi1's
// internal buffer.  Large messages are given to you in chunks, with the
// 3rd parameter to tell you which is the last chunk.  This function is
// a Teensy extension, not available in the Arduino MIDI library.
//
void mySystemExclusiveChunk(const byte *data, uint16_t length, bool last) {
  Serial.print("SysEx Message: ");
  printBytes(data, length);
  if (last) {
    Serial.println(" (end)");
  } else {
    Serial.println(" (to be continued)");
  }
}

// This simpler 2-input System Exclusive function can only receive messages
// up to the size of the internal buffer.  Larger messages are truncated, with
// no way to receive the data which did not fit in the buffer.  If both types
// of SysEx functions are set, the 3-input version will be called by midi1.
//
void mySystemExclusive(byte *data, unsigned int length) {
  Serial.print("SysEx Message: ");
  printBytes(data, length);
  Serial.println();
}

void myTimeCodeQuarterFrame(byte data) {
  static char SMPTE[8] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  static byte fps = 0;
  byte index = data >> 4;
  byte number = data & 15;
  if (index == 7) {
    fps = (number >> 1) & 3;
    number = number & 1;
  }
  if (index < 8 || number < 10) {
    SMPTE[index] = number + '0';
    Serial.print("TimeCode: ");  // perhaps only print when index == 7
    Serial.print(SMPTE[7]);
    Serial.print(SMPTE[6]);
    Serial.print(':');
    Serial.print(SMPTE[5]);
    Serial.print(SMPTE[4]);
    Serial.print(':');
    Serial.print(SMPTE[3]);
    Serial.print(SMPTE[2]);
    Serial.print('.');
    Serial.print(SMPTE[1]);  // perhaps add 2 to compensate for MIDI latency?
    Serial.print(SMPTE[0]);
    switch (fps) {
      case 0: Serial.println(" 24 fps"); break;
      case 1: Serial.println(" 25 fps"); break;
      case 2: Serial.println(" 29.97 fps"); break;
      case 3: Serial.println(" 30 fps"); break;
    }
  } else {
    Serial.print("TimeCode: invalid data = ");
    Serial.println(data, HEX);
  }
}

void mySongPosition(uint16_t beats) {
  Serial.print("Song Position, beat=");
  Serial.println(beats);
}

void mySongSelect(byte songNumber) {
  Serial.print("Song Select, song=");
  Serial.println(songNumber, DEC);
}

void myTuneRequest() {
  Serial.println("Tune Request");
}

void myClock() {
  Serial.println("Clock");
}

void myStart() {
  Serial.println("Start");
}

void myContinue() {
  Serial.println("Continue");
}

void myStop() {
  Serial.println("Stop");
}

void myActiveSensing() {
  Serial.println("Actvice Sensing");
}

void mySystemReset() {
  Serial.println("System Reset");
}

void myRealTimeSystem(uint8_t realtimebyte) {
  Serial.print("Real Time Message, code=");
  Serial.println(realtimebyte, HEX);
}



void printBytes(const byte *data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
}
