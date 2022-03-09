/*
   Code to detect pulses from the PulseSensor,
   using an interrupt service routine.

   Here is a link to the tutorial\
   https://pulsesensor.com/pages/getting-advanced

   Copyright World Famous Electronics LLC - see LICENSE
   Contributors:
     Joel Murphy, https://pulsesensor.com
     Yury Gitman, https://pulsesensor.com
     Bradford Needham, @bneedhamia, https://bluepapertech.com

   Licensed under the MIT License, a copy of which
   should have been included with this software.

   This software is not intended for medical use.
*/

/*
   Every Sketch that uses the PulseSensor Playground must
   define USE_ARDUINO_INTERRUPTS before including PulseSensorPlayground.h.
   Here, #define USE_ARDUINO_INTERRUPTS true tells the library to use
   interrupts to automatically read and process PulseSensor data.

   See ProcessEverySample.ino for an example of not using interrupts.
*/
#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>
#include <TFT.h>
#include <SPI.h>

/*re going to run
    the Processing Visualizer Sketch.
    See https://github.com/WorldFamous
   The format of our output.

   Set this to PROCESSING_VISUALIZER if you'Electronics/PulseSensor_Amped_Processing_Visualizer

   Set this to SERIAL_PLOTTER if you're going to run
    the Arduino IDE's Serial Plotter.
*/
const int OUTPUT_TYPE = SERIAL_PLOTTER;

/*
   Pinout:
     PULSE_INPUT = Analog Input. Connected to the pulse sensor
      purple (signal) wire.
     PULSE_BLINK = digital Output. Connected to an LED (and 220 ohm resistor)
      that will flash on each detected pulse.
     PULSE_FADE = digital Output. PWM pin onnected to an LED (and resistor)
      that will smoothly fade with each pulse.
      NOTE: PULSE_FADE must be a pin that supports PWM. Do not use
      pin 9 or 10, because those pins' PWM interferes with the sample timer.
*/
const int PULSE_INPUT = A0;
const int PULSE_BLINK = 13;    // Pin 13 is the on-board LED
const int PULSE_FADE = 5;
const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle
char sensorPrintout[3];

#define CS 10
#define DC 8
#define RST 9
#define BKLIGHT 7 // Backlight control pin

#define BRIGHTNESS 250 // Backlight intensity (0-255)
#define RANGE 100 // Vertical size of the graph in pixels.
#define WIDTH 128 // Horizontal size of Display in pixels.
#define HEIGHT 160 // Vertical size of Display in pixels.
#define PERSIST 500 // persistence of the graph 

TFT TFTscreen = TFT(CS, DC, RST);

/*
   All the PulseSensor Playground functions.
*/
PulseSensorPlayground pulseSensor;

int value ;
int last_value;
int x_pos= 0; // cursor position
int offset= 40; // vertical graph displacement in pixels.
int lastBPM = 70;

void setup() {
  /*
     Use 115200 baud because that's what the Processing Sketch expects to read,
     and because that speed provides about 11 bytes per millisecond.

     If we used a slower baud rate, we'd likely write bytes faster than
     they can be transmitted, which would mess up the timing
     of readSensor() calls, which would make the pulse measurement
     not work properly.
  */
  TFTscreen.begin(); // initialize the display
  TFTscreen.setRotation(0);
  TFTscreen.background(0,128,0); // dark green background
  TFTscreen.stroke(255,255,255); // white stroke
  analogWrite(BKLIGHT,BRIGHTNESS); //Set brightness.
  drawFrame();
  clearGraph();
  
  Serial.begin(115200);

  // Configure the PulseSensor manager.

  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.fadeOnPulse(PULSE_FADE);

  pulseSensor.setSerial(Serial);
  pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);

  // Now that everything is ready, start reading the PulseSensor signal.
  if (!pulseSensor.begin()) {
    /*
       PulseSensor initialization failed,
       likely because our particular Arduino platform interrupts
       aren't supported yet.

       If your Sketch hangs here, try PulseSensor_BPM_Alternative.ino,
       which doesn't use interrupts.
    */
    for(;;) {
      // Flash the led to show things didn't work.
      digitalWrite(PULSE_BLINK, LOW);
      delay(50);
      digitalWrite(PULSE_BLINK, HIGH);
      delay(50);
    }
  }
}

void drawFrame(){
TFTscreen.stroke(255,255,255); // white stroke 
TFTscreen.fill( 255, 255, 255 ); // White fill
TFTscreen.rect(0, 0, WIDTH, 21);
TFTscreen.setTextSize(2);
TFTscreen.stroke(0,0,0);
TFTscreen.text("BPM",2,8); // Exanple text
}

void clearGraph(){
TFTscreen.stroke(127,127,127); // grey for the border
TFTscreen.fill( 180, 0, 0 ); // dark Blue for the background
TFTscreen.rect(0, 22, 128, 128);
TFTscreen.line(65, 23, 65, 151); // vertical line
TFTscreen.line(1, 85,127, 85); // horizontal line
TFTscreen.stroke(0,255,255); // yellow stroke for plotting.
}

void plotLine(int last,int actual){
if(x_pos==0)last= actual; // first value is a dot.
TFTscreen.line(x_pos-1, last, x_pos, actual); // draw line.
x_pos++;
if(x_pos > WIDTH){
x_pos=0;
delay(PERSIST);
clearGraph();
}
}

void loop() {
  /*
     Wait a bit.
     We don't output every sample, because our baud rate
     won't support that much I/O.
  */
  delay(20);

  // write the latest sample to Serial.
 pulseSensor.outputSample();
 int myBPM = pulseSensor.getBeatsPerMinute();
 String sensorVal = String(myBPM);
 sensorVal.toCharArray(sensorPrintout, 3);
 last_value= value;
 value= pulseSensor.getLatestSample();
 value= map(value,0,1500,149,23);
 value -= offset;
 
 if(value <= 23) value= 23; //truncate off screen values.
  TFTscreen.stroke(0,255,255);
  plotLine(last_value ,value );
  /*
     If a beat has happened since we last checked,
     write the per-beat information to Serial.
   */
  if (pulseSensor.sawStartOfBeat()) {
   pulseSensor.outputBeat();
  if (abs(myBPM - lastBPM) > 5) {
  TFTscreen.fill( 255, 255, 255 ); // White fill
  TFTscreen.rect(50, 0, WIDTH, 21);
  TFTscreen.stroke(0,0,0);
  TFTscreen.text(sensorPrintout, 50, 8);
  int lastBPM = myBPM;
  }
  }
}
