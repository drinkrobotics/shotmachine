// Bounce.pde
// -*- mode: C++ -*-
//
// Make a single stepper bounce from one limit to another
//
// Copyright (C) 2012 Mike McCauley
// $Id: Random.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

#include <AccelStepper.h>

#define pinEnable 12
#define pinStep 7
#define pinDir 6

int speed_delay_in_microseconds = 5000;

// Define a stepper and the pins it will use
AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, pinStep, pinDir);

void setup()
{  

  
  // Change these to suit your stepper if you want
  stepper.setPinsInverted(false, false, true);
  stepper.setEnablePin(pinEnable);
  stepper.setMaxSpeed(10000);
  stepper.setAcceleration(1000);
  stepper.moveTo(10000);
}

void loop()
{
  
    if(speed_delay_in_microseconds > 200){

    speed_delay_in_microseconds--;
    }
  
    digitalWrite(pinEnable, LOW);

    digitalWrite(pinStep, HIGH);
    delayMicroseconds(speed_delay_in_microseconds);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(speed_delay_in_microseconds);

    digitalWrite(pinEnable, HIGH);
    
    //stepper.run();
    

}

