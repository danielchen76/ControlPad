#include <Bounce2.h>

#define RXLED         17
#define EC11_BUTTON   3

#define ENC_A         2
#define ENC_B         4

#define IR_SENSOR     A7

#define PLUG_IN       8

#define SERIAL      Serial

bool                  button_down = false;
unsigned long         button_down_time;

unsigned long         have_water_report_time;

// Flag byte
unsigned char         ucData;

#define CLICK_MASK    0x10
#define SHORT_CLICK   0x00
#define LONG_CLICK    0x01

#define ROLL_MASK     0x20
#define ROLL_FWD      0x00
#define ROLL_REV      0x02

#define KEY_DOWN_MASK 0x40
#define KEY_DOWN      0x04

#define EMPTY_MASK    0x80
#define EMPTY_TRUE    0x08
#define EMPTY_FALSE   0x00

//TXLED1:Green LED on; TXLED0:Green LED off
//digitalWrite(RXLED, LOW):Red LED on; digitalWrite(RXLED, HIGH):Red LED off

Bounce debouncer = Bounce();
Bounce rotaryA = Bounce();
Bounce rotaryB = Bounce();

unsigned long     tNow;
unsigned long     tDuration;

int               adValue;
bool              bHaveWater = false;

int               plugin;

void setup() {
  // put your setup code here, to run once:
  pinMode(RXLED, OUTPUT);

  pinMode(EC11_BUTTON, INPUT_PULLUP);
  debouncer.attach(EC11_BUTTON);
  debouncer.interval(5);

    /* Setup encoder pins as inputs */
  pinMode(ENC_A, INPUT_PULLUP);
  //digitalWrite(ENC_A, HIGH);
  pinMode(ENC_B, INPUT_PULLUP);
  //digitalWrite(ENC_B, HIGH);
  rotaryA.attach(ENC_A);
  rotaryA.interval(4);
  rotaryB.attach(ENC_B);
  rotaryB.interval(3);

  //plug in detect
  pinMode(PLUG_IN, INPUT_PULLUP);

  SERIAL.begin(115200);
}

void loop() {
  static uint8_t counter = 0;      //this variable will be changed by encoder input
  
  // put your main code here, to run repeatedly:
  ucData = 0;
  
  debouncer.update();
  
  tNow = millis();

  int value = debouncer.read();

    // set LED when long press
  if (button_down)
  {
    if (tNow > button_down_time)
    {
      tDuration = tNow - button_down_time;
    }
    else
    {
      tDuration = UINT32_MAX - button_down_time + tNow;
    }

    if ((tDuration >= 1000) && (tDuration <= 3000))
    {
      // Long click
      TXLED1;
    }
    else if (tDuration > 3000)
    {
      // nothing happend
      TXLED0;
    }
  }

  if (debouncer.fell())
  {
    // remember down time
    button_down_time = millis();
    button_down = true;
  }
  else if (debouncer.rose())
  {
    // Cal the down duration, long press ( >= 1.5s) or short press (< 1s), ignore other( < 1.5s and >= 1s)
    TXLED0;
    button_down = false;
    
    if (tNow > button_down_time)
    {
      tDuration = tNow - button_down_time;
    }
    else
    {
      tDuration = UINT32_MAX - button_down_time + tNow;
    }

    // 
    if (tDuration < 1000)
    {
      // Click
      ucData |= CLICK_MASK;
      ucData |= SHORT_CLICK;
    }
    else if ((tDuration >= 1000) && (tDuration <= 3000))
    {
      // Long click
      ucData |= CLICK_MASK;
      ucData |= LONG_CLICK;
    }
    else
    {
      // nothing happend
    }
  }

  // Rotary Encoder
  rotaryA.update();

  if (rotaryA.rose())
  {
    ucData |= ROLL_MASK;
    
    if (digitalRead(ENC_B))
    {
      // FWD
      ucData |= ROLL_FWD;
    }
    else
    {
      // REV
      ucData |= ROLL_REV;
    }
  }

  plugin = digitalRead(PLUG_IN);
  
  // Check backup RO water IR sensor
  adValue = analogRead(IR_SENSOR);
  if (bHaveWater)
  {
    if ((adValue < 900) || (plugin != LOW))
    {
      // No water
      bHaveWater = false;
      ucData |= EMPTY_MASK | EMPTY_FALSE;
    }
    else
    {
      // Periodly report have water
      if (tNow >= have_water_report_time)
      {
        tDuration = tNow - have_water_report_time;
      }
      else
      {
        tDuration = UINT32_MAX - have_water_report_time + tNow;
      }

      if (tDuration >= 1000)
      {
        // Report
        ucData |= EMPTY_MASK | EMPTY_TRUE;
        have_water_report_time = tNow;
      }
    }
  }
  else
  {
    if ((plugin == LOW) && (adValue > 1000))
    {
      // Have water
      bHaveWater = true;
      ucData |= EMPTY_MASK | EMPTY_TRUE;

      have_water_report_time = tNow;
    }
  }

  if (ucData != 0)
  {
    SERIAL.write(ucData);
  }
}


