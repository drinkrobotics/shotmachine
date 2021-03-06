#include <Wire.h>
#include <SparkFun_VL6180X.h>

//blynk
//#include <SoftwareSerial.h>
//SoftwareSerial SwSerial(2, 3); // RX, TX
//#define BLYNK_PRINT SwSerial
//#include <BlynkSimpleSerial.h>

#define VL6180X_ADDRESS 0x29
#define pinEnable 12
#define pinStep 7
#define pinModeSwitch 8
#define pinDir 6
#define pinEndSwitch 2
#define pinStartSwitch 3
#define pinLEDStartButton 11
#define pinRedStatusLED 13
#define pinPump1 4
#define pinPump2 5

#define RIGHT 1
#define LEFT 0

#define NUMBER_OF_SHOTS 8
#define NUMBER_OF_BULLETS_IN_RUSSIAN_ROULETTE 1
#define STEPS_TO_FIRST_SHOT 1420
#define STEPS_BETWEEN_SHOTS 840

//proximity sensor
#define TO_NEAR_DISTANCE_THRESHOLD 10
#define GLASS_AVAILABLE_THRESHOLD 80

//2cl
//#define TIME_TO_FILL_MS 3500

//time to fill 1cl ?
#define TIME_TO_FILL_MS_PUMP_1 1750
#define TIME_TO_FILL_MS_PUMP_2 1750

#define SIM_MODE false

bool checkForGlassEnabled = true;

int speed_delay_in_microseconds = 70; //works with acceleration
//int speed_delay_in_microseconds = 200;

///////////////////////////////////////////////////////////////////////
//don't change the following values as they will be overwritten///////
///////////////////////////////////////////////////////////////////////

bool proximity_sensor_available = true;

bool calibrated = false;

bool eStopEnabled = false;

volatile int endSwitchState = HIGH;

int number_of_shots = NUMBER_OF_SHOTS;


//blynk: auth key
//char auth[] = "f82e3a734586434487598f6968122ef7";

//blynk: set virtual pin 2 as terminal pin
//WidgetTerminal terminal(V2);

VL6180xIdentification identification;
VL6180x sensor(VL6180X_ADDRESS);


void setup() {

  //outputs
  pinMode(pinEnable, OUTPUT);
  pinMode(pinDir, OUTPUT);
  pinMode(pinStep, OUTPUT);
  pinMode(pinLEDStartButton, OUTPUT);
  pinMode(pinPump1, OUTPUT);
  pinMode(pinPump2, OUTPUT);


  //inputs
  pinMode(pinStartSwitch, INPUT);
  pinMode(pinEndSwitch, INPUT);
  pinMode(pinModeSwitch, INPUT);

  //enable pull ups
  digitalWrite(pinStartSwitch, HIGH);
  digitalWrite(pinEndSwitch, HIGH);
  digitalWrite(pinModeSwitch, HIGH);

  //set default value
  digitalWrite(pinLEDStartButton, LOW);
  digitalWrite(pinEnable, HIGH);
  digitalWrite(pinPump1, HIGH);
  digitalWrite(pinPump2, HIGH);

  attachInterrupt(digitalPinToInterrupt(pinEndSwitch), endSwitchInterrupt, FALLING);

  init_VL6180();

  randomSeed(666);

  //SwSerial.begin(9600);
  //Blynk.begin(auth);

  Serial.begin(9600);


  if (SIM_MODE) {
    Serial.println("Warning Sim mode enabled. Pumps deactivated!");
  }
}


void loop() {

  //Blynk.run();

  /*
  if (checkForGlassEnabled) {
    int distance = sensor.getDistance();
    Serial.println(distance);
    Blynk.virtualWrite(V5, distance);
    delay(500);
  }
  */

  int distance = sensor.getDistance();
  Serial.print("Distance measured (mm) = ");
  Serial.println(distance);

  if (digitalRead(pinModeSwitch) == LOW) {
    if (digitalRead(pinStartSwitch) == LOW) {
       make_shots(number_of_shots);
    }
  }else{
    if (digitalRead(pinStartSwitch) == LOW) {
      start_all_pumps();
    }else{
      stop_all_pumps();
    }
       
  }

}

void start_all_pumps(){
      digitalWrite(pinPump1, LOW);
      digitalWrite(pinPump2, LOW);    
}

void stop_all_pumps(){
      digitalWrite(pinPump2, HIGH);
      digitalWrite(pinPump1, HIGH);
}
void make_shots(int number) {

  if (!calibrated || eStopEnabled) {
    eStopEnabled = false;
    move_right_until_endswitch_detected();
  }

  digitalWrite(pinLEDStartButton, LOW);

  Serial.println("Making shots...");

  int bullet_positions_array[number];
  bool where_is_a_glass_array[number];


  //set all glasses to available
  for (int pos = 0; pos < number; pos++) {

    where_is_a_glass_array[pos] = true;

  }

    //russian roulette mode: enable pump 2 (strong schnaps)
    //randomly NUMBER_OF_BULLETS_IN_RUSSIAN_ROULETTE times.

    generate_bullets_array(number, where_is_a_glass_array, bullet_positions_array);

  //scan for glasses while driving left
  for (int pos = 0; pos < number; pos++) {

    if (pos == 0) {
      //longer drive for first shot
      move_steps(STEPS_TO_FIRST_SHOT, LEFT);
    } else {
      move_steps(STEPS_BETWEEN_SHOTS, LEFT);
    }

    //is there a glass?
    if (checkForGlassEnabled) {

      where_is_a_glass_array[pos] = is_there_a_glass();

      generate_bullets_array(number, where_is_a_glass_array, bullet_positions_array);

    } else {
      //if checkForGLasses is disabled directly fill in the glasses while going left
      fill_glass(bullet_positions_array[pos]);
    }
  }


  //we are now on the left side and drive to the right to fill the glasses

  if (checkForGlassEnabled) {

    Serial.println("fill the glasses on the way back to start");

    int distance_to_drive = 0;
    for (int pos = number - 1; pos >= 0; pos--) {

      if (where_is_a_glass_array[pos]) {
        move_steps(distance_to_drive, RIGHT);
        fill_glass(bullet_positions_array[pos]);
        distance_to_drive = 0;
      }

      if (pos == 0) {
        //longer drive for first shot
        distance_to_drive += STEPS_TO_FIRST_SHOT;
        move_steps(distance_to_drive, RIGHT);
      } else {
        distance_to_drive += STEPS_BETWEEN_SHOTS;
      }
    }

  } else {
    //return to start
    move_steps((number - 1)*STEPS_BETWEEN_SHOTS + STEPS_TO_FIRST_SHOT, RIGHT);
  }

  digitalWrite(pinLEDStartButton, HIGH);

  Serial.println("Finished");
}


void init_VL6180() {

  Wire.begin(); //Start I2C library
  delay(100); // delay .1s

  if (sensor.VL6180xInit() != 0) {
    Serial.println("FAILED TO INITALIZE VL6180, Disabled Check for Glass"); //Initialize device and check for errors
    proximity_sensor_available = false;
    checkForGlassEnabled = false;
  };

  sensor.VL6180xDefautSettings(); //Load default settings to get started.

  delay(1000); // delay 1s
}


void move_right_until_endswitch_detected() {

  Serial.println("Move right until endswitch detected");
  digitalWrite(pinEnable, LOW);

  digitalWrite(pinDir, RIGHT);

  int position = 0;

  for (position = 0; position < 20000; position++) {
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(1000);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(1000);

    if (endSwitchState == LOW) {
      endSwitchState = HIGH;


      //move to initial position
      move_steps(70, LEFT);

      calibrated = true;
      Serial.println("Successfully calibrated!");
      return;
    }
  }


  Serial.println("Error: Timed out but no endswitch reached!");

  digitalWrite(pinEnable, HIGH);

}


void endSwitchInterrupt() {
  Serial.println("End Switch Interrupt");

  endSwitchState = LOW;

  detachInterrupt(digitalPinToInterrupt(pinEndSwitch));
  if (calibrated) {
    //eStopEnabled = true;
  }
}


void move_steps(int steps, int dir) {

  int ramp_start_speed = 700;
  //int ramp_start_speed = 200;
  int acceleration = 300; //in how many steps should be accelerated

  if (acceleration >= steps / 2) {
    acceleration = steps / 2;
  }

  Serial.print("Move Steps: ");
  Serial.print(steps);
  Serial.print(" ");

  if(dir){
    Serial.println("right");
  }else{
    Serial.println("left");
  }

  int position = 0;

  digitalWrite(pinEnable, LOW);

  digitalWrite(pinDir, dir);
  //digitalWrite(pinStep, LOW);


  for (position = 0; position < steps; position++) {
    if (eStopEnabled) {
      Serial.println("E Stop enabled, press start to disable. This happened because end switch was pressed and it was already calibrated.");
      break;
    }

    int wait_delay = 0;

    if (position < acceleration) {
      wait_delay = speed_delay_in_microseconds + (acceleration - (position + 1)) * ((ramp_start_speed - speed_delay_in_microseconds) / acceleration);


    } else {
      wait_delay = speed_delay_in_microseconds;
    }

    int steps_before_end = steps - position;
    if (steps_before_end < acceleration) {
      wait_delay = speed_delay_in_microseconds + (acceleration - (steps_before_end)) * ((ramp_start_speed - speed_delay_in_microseconds) / acceleration);
    }

    //Serial.println("Delay");
    //Serial.println(wait_delay);

    digitalWrite(pinStep, HIGH);
    delayMicroseconds(wait_delay);

    digitalWrite(pinStep, LOW);
    delayMicroseconds(wait_delay);

  }

  digitalWrite(pinEnable, HIGH);
}


void generate_bullets_array(int number, bool *where_is_a_glass_array, int *bullet_positions_array) {

  //Init bullet positions array with 0, no bullet
  for (int pos = 0; pos < number; pos++) {
    bullet_positions_array[pos] = 0;
  }

  //check how many glasses there are in total
  int glass_count = 0;
  for (int pos = 0; pos < number; pos++) {

    if(where_is_a_glass_array[pos]){
      glass_count++;
    }
  }

  if(glass_count == 0){
    Serial.println("No glasses found going back home.");
    return;  
  }

  Serial.println("Number of available glasses: ");
  Serial.println(glass_count);
  Serial.println("\n");

  //generate the bullets at random locations
  Serial.println("Number of bullets in game: ");
  Serial.println(NUMBER_OF_BULLETS_IN_RUSSIAN_ROULETTE);
  Serial.println("\n");

  for (int bullet_number = 0; bullet_number < NUMBER_OF_BULLETS_IN_RUSSIAN_ROULETTE; bullet_number++) {

    int random_number = random(number);

    //find a space in the array with no bullet
    while (bullet_positions_array[random_number] == 1 || where_is_a_glass_array[random_number] == false) {
      Serial.println("Position not suitable for bullet because either there was already a bullet or no glass at this position");
      Serial.println(random_number);
      Serial.println("\n");
      random_number = random(number);
    }

    Serial.println("Bullet at Position: ");
    Serial.println(random_number);
    Serial.println("\n");

    bullet_positions_array[random_number] = 1;

  }
}


bool is_there_a_glass() {

  if (checkForGlassEnabled) {
    int distance = sensor.getDistance();
    delay(100);
    distance = sensor.getDistance();
    Serial.print("Distance measured (mm) = ");
    Serial.println(distance);

   // Blynk.virtualWrite(V5, distance);

    if (distance < TO_NEAR_DISTANCE_THRESHOLD) {

      Serial.print("Distance measured was smaller than TO_NEAR_DISTANCE_THRESHOLD (mm) = ");
      Serial.println(distance);
      Serial.println("Assume there is no glass to avoid spilling if there is no glass and the sensor is dirty");
      return false;
    }

    if (distance > GLASS_AVAILABLE_THRESHOLD) {
      return false;
    } else {
      return true;
    }
  }

}


void fill_glass(int pump) {

  if (SIM_MODE) {

    Serial.print("Start pump ");
    Serial.println(pump);

  } else {

    if (!is_there_a_glass()) {
      return;
    }

    Serial.print("Start pump ");
    Serial.println(pump);

    if (pump == 0) {
      digitalWrite(pinPump1, LOW);
      delay(TIME_TO_FILL_MS_PUMP_1);
      digitalWrite(pinPump1, HIGH);
    } else {
      digitalWrite(pinPump2, LOW);
      delay(TIME_TO_FILL_MS_PUMP_2);
      digitalWrite(pinPump2, HIGH);
    }

    Serial.print("Stop pump.");
    Serial.println(pump);

    //delay for drops
    delay(400);
  }
}


// This function will be called every time
// when App writes value to Virtual Pin 1
/*BLYNK_WRITE(V1)
{
  Serial.println("Got message V1 virtual start button");

  int i = param.asInt();

  if (i) {
    make_shots(number_of_shots);
  }
}

BLYNK_WRITE(V3)
{
  int _number_of_shots = param.asInt();

  if (_number_of_shots > 8) {
    number_of_shots = NUMBER_OF_SHOTS;
  } else if (_number_of_shots < 1) {
    number_of_shots = 0;
  } else {
    number_of_shots = _number_of_shots;
  }

  Serial.print("Got message V3 Number of Shots set to: ");
  Serial.println(number_of_shots);
}


BLYNK_WRITE(V4)
{
  if (proximity_sensor_available) {
    checkForGlassEnabled = param.asInt();
    Serial.print("Setting check for glasses to: ");
    Serial.println(checkForGlassEnabled);
  } else {
    Serial.println("Proximity Sensor not connected/available. Cannot enable or disable.");
  }
}
*/

