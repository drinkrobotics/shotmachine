#!/usr/bin/env python
# -*- coding: utf-8 -*-

# import required modules
import time
import RPi.GPIO as GPIO
import smbus
import pygame
from array import *

UltrasonicSensorSRF02Address = 0x71
UltrasonicSensorSRF08Address = 0x70

#output GPIO pins e.g. relais
GPIOOutput1 = 11  #motor
GPIOOutput2 = 7 #pump
GPIOOutput3 = 9  #direction of motor
GPIOOutput4 = 8 #direction of pump

GPIOInput1 = 18  #end reached switch
# variables

#The min time (s) between two output switch events
DeadTime = 1

#Trigger distance
#If the detected distance is smaller than this value (cm) the output switch is opened
TriggerDistance = 30

#The time the output switch is open
#1.5 shot
OpenTime = 0.5

pwmDutyCycle = 100




# main function
def main():

  initAudio()

  DeadTimeStart = 0
  
  shotWasPoured = False
  
  x = 1

  while x is 1:
    if MeasureDistanceSRF02() < 20:
      try:
        while True:
          #Check if end contact was activated
          if(not GPIO.input(GPIOInput1)):
            GPIO.output(GPIOOutput3, GPIO.HIGH)
            time.sleep(0.435)
            pwm.stop()
            GPIO.output(GPIOOutput1, GPIO.LOW)
            GPIO.output(GPIOOutput3, GPIO.LOW)
            break
          

          #MeasureDistanceSRF02()
          Distance = MeasureDistanceSRF08()

          #If a shot has been poured do nothing until the sensor detects nothing (0)
          #if shotWasPoured and Distance >= 1:
          #  continue
          #else:
          #  shotWasPoured = False
              

          if Distance <= 1 or Distance >= 4:
            #turn on motor
            pwm.start(pwmDutyCycle)
          else:
            #turn off motor
            pwm.stop()
            GPIO.output(GPIOOutput1, GPIO.LOW)
            MeasureDistanceSRF02()
            
            #pour
            GPIO.output(GPIOOutput2, GPIO.HIGH)
            ml = 20.0
            mlPerSec = 3*220/60 #225 bei 24Volt
            time.sleep(ml/mlPerSec)
            GPIO.output(GPIOOutput2, GPIO.LOW)
            time.sleep(0.1)
            #shotWasPoured = True
            
            #turn on motor
            pwm.start(pwmDutyCycle)
            
          	#goto end of glass
            while Distance == 3 and GPIO.input(GPIOInput1):
              Distance = MeasureDistanceSRF08()

      # reset GPIO settings if user pressed Ctrl+C
      except KeyboardInterrupt:
        print("Measurement stopped by user")
        GPIO.cleanup()
      except IOError:
        print("IO Error")
        pwm.stop()
        GPIO.output(GPIOOutput1, GPIO.LOW)
        GPIO.output(GPIOOutput2, GPIO.LOW)
        GPIO.output(GPIOOutput3, GPIO.LOW)
        #GPIO.cleanup()
      x = 0

      

"""
def MeasureDistance():
  #Simple measurement - result in cm
  i2c.write_byte_data(UltrasonicSensorSRF08Address, 0, 81)
  time.sleep(0.1)
  return i2c.read_word_data(UltrasonicSensorSRF08Address, 2) / 255
"""

def MeasureDistanceSRF02():
  return 10
  
  #Simple measurement - result in cm
  i2c.write_byte_data(UltrasonicSensorSRF02Address, 0, 81)
  time.sleep(0.1)
 
  high = i2c.read_word_data(UltrasonicSensorSRF02Address, 3)
  print "Dist activte " + str(high)
  return high


def MeasureDistanceSRF08():

  #set aplification to 94 (see datasheet page 4 http://www.roboter-teile.de/datasheets/srf08.pdf)
  i2c.write_byte_data(UltrasonicSensorSRF08Address, 1, 1)


  #set range to 9cm (doc p. 3)
  i2c.write_byte_data(UltrasonicSensorSRF08Address, 2, 1)

  #Simple measurement - result in cm
  i2c.write_byte_data(UltrasonicSensorSRF08Address, 0, 81)
  
 
   
  #time.sleep(0.0045)
  time.sleep(0.005)
  
  #version = 0xff
  #while version is 0xFF:
    #version = i2c.read_word_data(UltrasonicSensorSRF08Address, 0)
    #print version
 
  
  lowByte = i2c.read_word_data(UltrasonicSensorSRF08Address, 3)
  #print lowByte
  return lowByte
  
  
  
  #read_registers.append(highByte/256)
  #read_registers.append(lowByte | (highByte<<8))

  #print read_registers




def initAudio():
  pygame.mixer.init()
  pygame.mixer.music.load("arrr.mp3")

def playSound():
  pygame.mixer.music.play()

def changeI2CAddress(oldAddress, newAddress):
  i2c.write_byte_data(oldAddress,0,0xA0)
  i2c.write_byte_data(oldAddress,0,0xAA)
  i2c.write_byte_data(oldAddress,0,0xA5)
  i2c.write_byte_data(oldAddress,0,newAddress)

if __name__ == '__main__':
  # use GPIO pin numbering convention (not the actual pin numbers)
  GPIO.setmode(GPIO.BCM)

  # set up GPIO pins
  GPIO.setup(GPIOOutput1, GPIO.OUT)
  GPIO.setup(GPIOOutput2, GPIO.OUT)
  GPIO.setup(GPIOOutput3, GPIO.OUT)
  GPIO.setup(GPIOOutput4, GPIO.OUT)
  GPIO.setup(GPIOInput1, GPIO.IN)
  
  pwm = GPIO.PWM(GPIOOutput1, 450)

  pwm.stop()
  GPIO.output(GPIOOutput1, GPIO.LOW)
  GPIO.output(GPIOOutput2, GPIO.LOW)
  GPIO.output(GPIOOutput3, GPIO.LOW)
  GPIO.output(GPIOOutput4, GPIO.HIGH)


  #init smbus 1
  i2c = smbus.SMBus(1)

  time.sleep(0.5)

  # call main function
  main()

# function to measure the distance
"""
def MeasureDistance():
  # set trigger to high
  GPIO.output(GPIOTrigger, GPIO.HIGH)

  # set trigger after 10µs to low
  time.sleep(0.00001)
  GPIO.output(GPIOTrigger, GPIO.LOW)

  # store initial start time
  StartTime = time.time()

  # store start time
  while GPIO.input(GPIOEcho) == 0:
    StartTime = time.time()

  # store stop time
  
  while GPIO.input(GPIOEcho) == 1:
    StopTime = time.time()

  # calculate distance
  TimeElapsed = StopTime - StartTime
  Distance = (TimeElapsed * 34300) / 2

  return Distance
"""
