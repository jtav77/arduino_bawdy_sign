#include <SevSeg.h>

// The code in this file controls the Bawdy Storytelling
// timing sign.  Basically, there are two modes that an
// operator can use - preview mode and story mode.
//
// To get into preview mode, the operator presses the round
// push button while not in story mode.  Subsequent pushes to 
// this button moves the sign into the next stage - "4 min", 
// "2 min", "0 min", "0 min" and slow blink, and finally "0 min"
// with fast blink.  Pressing the push button while in story mode
// will not start preview mode.  To prevent moving through stages
// faster than intended or restarting preview mode unintentionally,
// the code waits previewInterval seconds before checking for
// another button press to advance the stage.
//
// To get into story mode, the operator slides the switch to the
// left while not in preview mode.  At that point, the clock
// starts counting up.  When there are 4 minutes left in a 10
// minute story, the first message of "4 min" glows on the sign
// and stays on displayInterval number of seconds.  At 2 minutes
// left, the second message of "2 min" glows for displayInterval
// seconds.  At ten minutes, the red "0 min" message glows.  At
// 10 minutes and slowBlinkStartTime seconds, this message begins
// a slow blink.  At 11 minutes, this message starts a fast blink
// and does not turn off until the operator slides the switch back
// to the right.  Whenever the switch is moved to the right, the
// clock resets to 00:00; there is no pausing the clock.  Note: the 
// operator must advance through preview mode before starting story
// mode.
//
// For both story mode and preview mode, the speed of the slow and
// fast blink can be adjusted with the configurable constants 
// slowBlinkInterval and fastBlinkInterval.

// configurable constants

// how long the sign stays lit, in seconds
static const int displayInterval = 20;
// minimum seconds each stage previewed to prevent skipping a stage
static const int previewInterval = 3;
// seconds after 10 min to start slow blink
static const int slowBlinkStartTime = 30;
static const int slowBlinkInterval = 500; // milliseconds
static const int fastBlinkInterval = 200; // milliseconds

// non-configurable constants
static const byte switchPin = A5;
static const byte previewPin = A4;
static const byte fourPin = 6;
static const byte twoPin = 5; 
static const byte zeroPin = 4;
static const byte minPin = 7;
static const int msPerSecond = 1000;

static SevSeg sevseg;
static boolean blink = false;
static boolean slowBlink = false;
static boolean fastBlink = false;
static unsigned long blinkTimer = millis();

void setup()
{
  byte numDigits = 4;   
  byte digitPins[] = {
    2, 3, 8, 9            };
  byte segmentPins[] = {
    13, 11, A2, A1, A0, 12, A3, 10            };

  sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(20); // does not seem to make a difference

  pinMode(previewPin, INPUT);
  pinMode(switchPin, INPUT);

  // initialize EL pins, cannot be changed from pins 4 - 7
  for(int i = 4; i < 8; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

// set number and "min" in EL wire
//
// input:  pin - expects zeroPin, twoPin, or fourPin
//         state - expects LOW or HIGH
void setEL(byte pin, byte state)
{
  digitalWrite(pin, state);
  digitalWrite(minPin, state);
}

// Through repeated presses of the push button, this routine
// allows the operator to cycle through the 3 different messages
// shown to the storyteller, as well as 2 stages of blinking.
void preview()
{
  bool delayDone = false;
  byte previousStage = -1;
  byte stage = 0;
  const byte lastStage = 4;
  unsigned long delayTimer;

  blink = false;
  slowBlink = false;
  fastBlink = false;

  while (stage <= lastStage) {
    switch (stage) {
    case 0:
      setEL(fourPin, HIGH);
      break;
    case 1:
      setEL(fourPin, LOW);
      setEL(twoPin, HIGH);
      break;
    case 2:
      setEL(twoPin, LOW);
      setEL(zeroPin, HIGH);
      break;
    case 3:
      setEL(zeroPin, LOW);

      slowBlink = true;

      if (blink) {
        setEL(zeroPin, HIGH);
      } 
      else {
        setEL(zeroPin, LOW);   
      }
      break;
    case 4:
      setEL(zeroPin, LOW);

      slowBlink = false;
      fastBlink = true;

      if (blink) {
        setEL(zeroPin, HIGH);    
      } 
      else {
        setEL(zeroPin, LOW);   
      }
      break;
    default:
      // NOP
      break;
    }

    if (previousStage != stage) {
      previousStage = stage;
      delayDone = false;
      delayTimer = millis();
      blinkTimer = millis();
    }

    if ((digitalRead(previewPin) == LOW) && delayDone) {
      stage++;
    }

    if (!delayDone) {
      // delay several seconds so a single button press
      // does not skip a stage
      if (millis() >= (delayTimer +
        (previewInterval * msPerSecond))) {
        delayDone = true;
        delayTimer += (previewInterval * msPerSecond);
      }
    }

    if (slowBlink) {
      if (millis() >= (blinkTimer + slowBlinkInterval)) {
        blink = !blink;
        blinkTimer += slowBlinkInterval;
      }
    } 
    else if (fastBlink) {
      if (millis() >= (blinkTimer + fastBlinkInterval)) {
        blink = !blink;
        blinkTimer += fastBlinkInterval;
      }
    }
  }

  setEL(zeroPin, LOW);

  // prevent getting stuck repeating preview mode
  delay(previewInterval * msPerSecond);
}

void loop()
{
  static unsigned long now;
  static unsigned long clock = millis();
  static int seconds = 0;
  static int minutes = 0;
  static boolean count = false;

  if (digitalRead(switchPin) == LOW) {
    count = true;
  } 
  else {
    // no pausing the clock
    // turning the switch off resets the time
    count = false;
    seconds = minutes = 0;
    digitalWrite(zeroPin, LOW);
    digitalWrite(twoPin, LOW);
    digitalWrite(fourPin, LOW);
    digitalWrite(minPin, LOW); 
  }

  // prevent accidentally getting into preview mode
  // during a story by checking count is false
  if ((digitalRead(previewPin) == LOW) && !count) {
    preview();
  }

  // count up per second on the 4 x 7-segment display
  // during a story
  if ((millis() >= (clock + msPerSecond)) && count) {
    seconds++;
    clock += msPerSecond; 
    if (seconds == 60) {
      minutes++;
      seconds = 0;
    }
  } 
  else if (!count) {
    clock = millis();
  }

  // calculate if and how fast to blink the "0 min" message
  if (slowBlink) {
    if (millis() >= (blinkTimer + slowBlinkInterval)) {
      blink = !blink;
      blinkTimer += slowBlinkInterval;
    }
  } 
  else if (fastBlink) {
    if (millis() >= (blinkTimer + fastBlinkInterval)) {
      blink = !blink;
      blinkTimer += fastBlinkInterval;
    }
  }

  // determine when and which EL wires should be turned on  
  if ((minutes == 6) && (seconds >= displayInterval)) {
    setEL(fourPin, LOW);
  } 
  else if (minutes == 6) {
    setEL(fourPin, HIGH);
  } 
  else if ((minutes == 8) && (seconds >= displayInterval)) {
    setEL(twoPin, LOW);
  } 
  else if (minutes == 8) {
    setEL(twoPin, HIGH);
  } 
  else if ((minutes == 10) && (seconds >= slowBlinkStartTime)) {
    slowBlink = true;
    if (blink) {
      setEL(zeroPin, HIGH);
    } 
    else {
      setEL(zeroPin, LOW);
    }
  } 
  else if (minutes == 10) {
    setEL(zeroPin, HIGH);
  } 
  else if (minutes >= 11) {
    slowBlink = false;
    fastBlink = true;
    if (blink) {
      setEL(zeroPin, HIGH);
    } 
    else {
      setEL(zeroPin, LOW);
    }
  }

  sevseg.setNumber(minutes * 100 + seconds, 2);
  sevseg.refreshDisplay(); // Must run repeatedly
}








