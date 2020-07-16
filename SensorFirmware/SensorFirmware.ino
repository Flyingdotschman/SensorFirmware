#include <TimerThree.h>
#include <MillisTimer.h>

// Konstanten

// Anschluesse PI

const uint8_t signal_length = 5; // Hight time signal nach Raspberry PI
const uint32_t time_to_free_up = 1000;

const int pinPersOutData = 2;
const int pinPersInData = 3;

// Anschluesse Sensoren
const int leftforward = A2;
const int leftbackward =  A3;
const int rightforward = A4;
const int rightbackward = A5;

const int led_lf = 8;
const int led_lb = 9;
const int led_rf = 4;
const int led_rb = 5;

const int led_state_left = 7;
const int led_state_right = 6;

const int buzzer = 10;

const int poti_left = A1;
const int poti_right = A0;


// Moeglicher Status der Detection
enum STATES  {WAITING, DETECTED_FRONT_FIRST, DETECTED_BACK_FIRST, DETECT_IN, DETECT_OUT };
enum SENSOR_STATE { FREE, DETECT_ACTIV, DETECT_PAST};
// Status der Linken bzw Rechten Lichtschranken
STATES state_left;
STATES state_right;

SENSOR_STATE sensor_lf;
SENSOR_STATE sensor_lb;
SENSOR_STATE sensor_rf;
SENSOR_STATE sensor_rb;



// Lauf Variablen

// Kalibrationswerte
uint16_t cal_lf = 0;
uint16_t cal_lb = 0;
uint16_t cal_rf = 0;
uint16_t cal_rb = 0;

uint16_t offset_left = 0;
uint16_t offset_right = 0;

// Gelesene Werte der Sensoren
int32_t val_lf = 0;
int32_t val_lb = 0;
int32_t val_rf = 0;
int32_t val_rb = 0;

String outp;


// Timer

MillisTimer timer_left = MillisTimer(time_to_free_up);
MillisTimer timer_right = MillisTimer(time_to_free_up);

// **************************** BEGINN SETUP **************************************
void setup() {
  Serial.begin(9600);
  ////Serial1.begin(11520);
  // Pins Sensoren
  pinMode(leftforward, INPUT);
  pinMode(leftbackward, INPUT);

  pinMode(rightforward, INPUT);
  pinMode(rightbackward, INPUT);


  pinMode(poti_left, INPUT);
  pinMode(poti_right, INPUT);

  pinMode(buzzer, OUTPUT);

  pinMode(led_lf, OUTPUT);
  digitalWrite(led_lf, HIGH);
  pinMode(led_lb, OUTPUT);
  digitalWrite(led_lb, HIGH);

  pinMode(led_rf, OUTPUT);
  digitalWrite(led_rf, HIGH);
  pinMode(led_rb, OUTPUT);
  digitalWrite(led_rb, HIGH);

  pinMode(led_state_left, OUTPUT);
  digitalWrite(led_state_left, LOW);
  pinMode(led_state_right, OUTPUT);
  digitalWrite(led_state_right, LOW);

  // Pins PI

  pinMode(pinPersOutData, OUTPUT);
  pinMode(pinPersInData, OUTPUT);


  delay(500);
  cal_lf = calibrate(leftforward);
  cal_lb = calibrate(leftbackward);
  cal_rf = calibrate(rightforward);
  cal_rb = calibrate(rightbackward);
  // calibrate(rightforward);
  // calibrate(rightbackward);

  delay(500);


  digitalWrite(pinPersOutData, LOW);
  digitalWrite(pinPersInData, LOW);

  delay(1000);
  state_left = WAITING;
  sensor_lf = FREE;
  sensor_lb = FREE;
  tone(buzzer, 500, 500);
  delay(500);
  tone(buzzer, 1000, 500);
  delay(1500);
}


// **************************** BEGINN LOOP ******************************************
void loop() {
  delay(17);

  while (Serial.available() > 0) {
    int xxx = Serial.read();
    if (xxx == 48) {
      personOUT();
      Serial.println("person Out USB");
    }
    if (xxx == 49) {
      personIN();
      Serial.println("person In USB");

    }    
  }

  if (state_left == WAITING) {
    digitalWrite(led_state_left, LOW);
  } else {
    digitalWrite(led_state_left, HIGH);
  }
  if (state_right == WAITING) {
    digitalWrite(led_state_right, LOW);
  } else {
    digitalWrite(led_state_right, HIGH);
  }

  timer_left.run();
  timer_right.run();

  offset_left = analogRead(poti_left);
  offset_right = analogRead(poti_right);

  offset_left = int(float(offset_left / 1024.0) * 200.0);
  offset_right = int(float(offset_right / 1024.0 ) * 200.0);


  // SMOOTHING PART NEEDS CHECKING IN PRACTICE *************************************************************************************
  //delay(10);
  val_lf = val_lf * 0.5 + 0.5 * analogRead(leftforward);
  val_lb = val_lb * 0.5 + 0.5 * analogRead(leftbackward);
  val_rf = val_rf * 0.5 + 0.5 * analogRead(rightforward);
  val_rb = val_rb * 0.5 + 0.5 * analogRead(rightbackward);
  //val_rf=analogRead(rightforward);
  //val_rb=analogRead(rightbackward);
  //
  //String outp = String(val_lf - cal_lf) + " ; " + String(val_lb - cal_lb)  + " ; " + String(offset_left);// + " ; " + String(val_rb);

  //String outp = String(val_lf-cal_lf);
  // Serial.println(outp);
  //Serial1.println(outp);

  // ***********************LEFT SIDE**************************LEFT SIDE *******************************************
  //  SENSOR LEFT FRONT ***********************************************
  if ((val_lf - cal_lf - offset_left) > 0) {
    sensor_lf = DETECT_ACTIV;
    digitalWrite(led_lf, LOW);
    if (sensor_lb == FREE) {
      if (state_left == WAITING) {
        state_left = DETECTED_FRONT_FIRST;
        abbortLeftTimer();
        startLeftTimer();
      }
    }
  }


  if (((val_lf - cal_lf - offset_left) < 0) && sensor_lf == DETECT_ACTIV) {
    sensor_lf = DETECT_PAST;
    digitalWrite(led_lf, HIGH);
  }



  // SENSOR LEFT BACK **************************************************
  if ((val_lb - cal_lb - offset_left) > 0) {
    sensor_lb = DETECT_ACTIV;
    digitalWrite(led_lb, LOW);
    if (sensor_lf == FREE) {
      if (state_left == WAITING) {
        state_left = DETECTED_BACK_FIRST;
        abbortLeftTimer();
        startLeftTimer();
      }
    }
  }

  if (((val_lb - cal_lb - offset_left) < 0) && sensor_lb == DETECT_ACTIV) {
    sensor_lb = DETECT_PAST;
    digitalWrite(led_lb, HIGH);
  }

  // SENSOR LEFT DESCISION *********************************************
  if ((sensor_lf == DETECT_PAST) && (sensor_lb == DETECT_PAST)) {
    if (state_left == DETECTED_FRONT_FIRST) {
      state_left = DETECT_IN;
    }
    if (state_left == DETECTED_BACK_FIRST) {
      state_left = DETECT_OUT;
    }
  }

  // LEFT IN ****************************************************
  if (state_left == DETECT_IN) {
    personIN();
    resetStateLeft();
  }
  // LEFT OUT **********************************************
  if (state_left == DETECT_OUT) {
    personOUT();
    resetStateLeft();
  }

  //outp = "LEFT FRONT: " + String(sensor_lf) + "   LEFT BACK: " + String(sensor_lb) + "   LEFT STATE: " + String(state_left) + "  Werte:    " + String(val_lf - cal_lf) + " ; " + String(val_lb - cal_lb)  + " ; " + String(offset_left);
  outp = "  Werte Links:    " + String(val_lf - cal_lf) + " ; " + String(val_lb - cal_lb)  + " ; " + String(offset_left) + "  Werte Rechts:    " + String(val_rf - cal_rf) + " ; " + String(val_rb - cal_rb)  + " ; " + String(offset_right);
  Serial.println(outp);
  //Serial1.println(outp);

  // ***********************RIGHT SIDE**************************RIGHT SIDE *******************************************
  //  SENSOR RIGHT FRONT ***********************************************
  if ((val_rf - cal_rf - offset_right) > 0) {
    sensor_rf = DETECT_ACTIV;
    digitalWrite(led_rf, LOW);
    if (sensor_rb == FREE) {
      if (state_right == WAITING) {
        state_right = DETECTED_FRONT_FIRST;
        abbortRightTimer();
        startRightTimer();
      }
    }
  }


  if (((val_rf - cal_rf - offset_right) < 0) && sensor_rf == DETECT_ACTIV) {
    sensor_rf = DETECT_PAST;
    digitalWrite(led_rf, HIGH);
  }



  // SENSOR RIGHT BACK **************************************************
  if ((val_rb - cal_rb - offset_right) > 0) {
    sensor_rb = DETECT_ACTIV;
    digitalWrite(led_rb, LOW);
    if (sensor_rf == FREE) {
      if (state_right == WAITING) {
        state_right = DETECTED_BACK_FIRST;
        abbortRightTimer();
        startRightTimer();
      }
    }
  }

  if (((val_rb - cal_rb - offset_right) < 0) && sensor_rb == DETECT_ACTIV) {
    sensor_rb = DETECT_PAST;
    digitalWrite(led_rb, HIGH);
  }

  // SENSOR RIGHT DESCISION *********************************************
  if ((sensor_rf == DETECT_PAST) && (sensor_rb == DETECT_PAST)) {
    if (state_right == DETECTED_FRONT_FIRST) {
      state_right = DETECT_IN;
    }
    if (state_right == DETECTED_BACK_FIRST) {
      state_right = DETECT_OUT;
    }
  }

  // RIGHT IN ****************************************************
  if (state_right == DETECT_IN) {
    personIN();
    resetStateRight();
  }
  // RIGHT OUT **********************************************
  if (state_right == DETECT_OUT) {
    personOUT();
    resetStateRight();
  }

  //outp = "RIGHT FRONT: " + String(sensor_rf) + "   RIGHT BACK: " + String(sensor_rb) + "   RIGHT STATE: " + String(state_right) + "  Werte:    " + String(val_rf - cal_rf) + " ; " + String(val_rb - cal_rb)  + " ; " + String(offset_right);
  //Serial.println(outp);
  //Serial1.println(outp);
}





// ********************* FUNCTIONS ************************ FUNCTIONS **********************************************
uint16_t calibrate(int pin) {

  // digitalWrite(pinPersOut, HIGH);
  //digitalWrite(pinPersIn, HIGH);
  long temp = 0;
  for (uint16_t i = 0; i < 50; i++) {
    delay(17);
    temp = temp + analogRead(pin);
  }


  // Serial.println("Calibration:");
  //Serial.println("**");
  // Serial.println(temp / 50);
  /*   String outp = "CalibrationValue:  " + String(temp/100);
    Serial.println(outp);

  */

  // digitalWrite(pinPersOut, LOW);
  // digitalWrite(pinPersIn, LOW);

  return ((temp / 50));
}

// ******************** PERSON IN FUCNTION ***************************************
void personIN() {
  tone(buzzer, 500, 250);

  digitalWrite(pinPersInData, HIGH);
  delay(signal_length);

  digitalWrite(pinPersInData, LOW);
  //Serial1.println("IN");
  /*
    if (sensor_rf != DETECT_ACTIV) {
      cal_rf = calibrate(rightforward);
    }
    if (sensor_rb != DETECT_ACTIV) {
      cal_rb = calibrate(rightbackward);
    }*/
}


// ************************** PERSON OUT FUNCTION ********************************
void personOUT() {
  tone(buzzer, 1000, 250);

  digitalWrite(pinPersOutData, HIGH);
  delay(signal_length);

  digitalWrite(pinPersOutData, LOW);

  //Serial1.println("OUT");
  /*
    if (sensor_rf != DETECT_ACTIV) {
      cal_rf = calibrate(rightforward);
    }
    if (sensor_rb != DETECT_ACTIV) {
      cal_rb = calibrate(rightbackward);
    }*/
}

void resetStateLeft() {
  state_left = WAITING;
  sensor_lf = FREE;
  sensor_lb = FREE;

  if (timer_left.isRunning()) {
    abbortLeftTimer();
  }
}

void resetStateRight() {
  state_right = WAITING;
  sensor_rf = FREE;
  sensor_rb = FREE;

  if (timer_right.isRunning()) {
    abbortRightTimer();
  }
}

void startLeftTimer() {
  timer_left.setInterval(time_to_free_up);
  timer_left.expiredHandler(resetStateLeft);
  timer_left.setRepeats(1);
  timer_left.start();
}

void startRightTimer() {
  timer_right.setInterval(time_to_free_up);
  timer_right.expiredHandler(resetStateRight);
  timer_right.setRepeats(1);
  timer_right.start();
}

void abbortLeftTimer() {
  timer_left.stop();
  timer_left.reset();
}

void abbortRightTimer() {
  timer_right.stop();
  timer_right.reset();
}
