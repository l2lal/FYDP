#! /usr/bin/env python
import threading
import RPi.GPIO as GPIO
import serial
import crcmod
import signal
import sys

class MotorInterface(object):
    def __init__(self, recording_freq, playback_freq):
        self._states = ["waiting", "recording", "playback"]
        self._current_state = 0
        self._motor_angles = []
        self._recording_index = 0
        self._playback_index = 0
        self._crc32 = crcmod.Crc(0x104c11db7, initCrc=0, xorOut=0xFFFFFFFF)
        self._recording_freq = recording_freq
        self._playback_freq = playback_freq
        baud_rate = 9600
        self._ser_AX = serial.Serial("/dev/ttyACM0", baud_rate)
        #self._ser_XL = serial.Serial("/dev/ttyACM0", baud_rate)

    def signal_handler(self, sig, frame):
        print('You pressed Ctrl+C!')
        self._ser_AX.close()
        sys.exit(0)

    def start_recording(self, channel):
        if self._current_state != 1:
            print "now overwriting the previous recording!"
            self._current_state = 1
            self._recording_index = 0
            self._motor_angles = []
            threading.Timer(1.0/self._recording_freq, self.recording()).start()
 
    def start_playback(self, channel):  
        if self._current_state != 2:
            if len(motor_angles) == 0:
                self._current_state = 0
                print "Nothing to be played back!"
                return
            self._current_state = 2
            self._playback_index = self._record_index
            threading.Timer(1.0/self._playback_freq, self.playing()).start()

    def read_serial_port(self, ser):
        response  = ""
        if ser.inWaiting():
            n = ser.inWaiting()
            for num in range(n):
                response += ser.read()
            print response
            response = response.strip("\r\n")
            response = response.split(",")
            checksum = response[-1]
            response = response[0:-1]
            self._crc32.update(",".join(map(str, response)))

            #if checksum != str(self._crc32.crcValue):
                #print "Checksum mismatch!"
                #print "received: " + checksum
                #print "calculated: " + str(self._crc32.crcValue)
                #return None
            return response
        else:
            print "No data!"

    def write_to_serial_port(self, data, ser):
        #Send a message to request the motor positions
        # START_BIT, MODE, CHECKSUM
        msg = "s,"
        msg += ",".join(data)
        msg += ","
        self._crc32.update(msg)
        msg += str(self._crc32.crcValue)
        msg += "\n"
        ser.write(msg)

    def recording(self):
        msg = ["2"]
        self.write_to_serial_port(msg, self._ser_AX)
        #self.write_to_serial_port(msg, self._ser_XL)

        #Get a response from the AX controllers
        AX_resp = self.read_serial_port(self._ser_AX)
        #XL_resp = self.read_serial_port(self._ser_XL)

        if AX_resp != None: # and XL_resp != None:
            if len(AX_resp) == 3:
                self._motor_angles.append(AX_resp[1:4])
                print self._motor_angles
                #motor_angles[self._recording_index] += XL_resp[1]
                self._recording_index += 1
        if self._current_state == 1:
            threading.Timer(1.0/recording_freq, self.recording()).start()

    def playing(self):
        if (self._playback_index == 0):
            self._current_state = 0
        AX_msg = ["1"]
        AX_msg += motor_angles[self._playback_index]
        self.playback_index -= 1;
        self.write_to_serial_port(AX_msg, self._ser_AX)
        #self.write_to_serial_port(msg, self._ser_XL)

        if self._current_state == 2:
            threading.Timer(1.0/playback_freq, self.playing()).start() 

if __name__ == '__main__':
    recording_freq = 80.0
    playback_freq = recording_freq
    arm = MotorInterface(recording_freq, playback_freq)

    GPIO.setmode(GPIO.BCM)
    # GPIO 23 & 17 set up as inputs, pulled up to avoid false detection.  
    # Both ports are wired to connect to GND on button press.  
    # So we'll be setting up falling edge detection for both  
    GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)  
    GPIO.setup(17, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    # when a falling edge is detected on port 17, regardless of whatever   
    # else is happening in the program, the function my_callback will be run  
    GPIO.add_event_detect(17, GPIO.FALLING, callback=arm.start_recording, bouncetime=300)  
 
    # when a falling edge is detected on port 23, regardless of whatever   
    # else is happening in the program, the function my_callback2 will be run  
    # 'bouncetime=300' includes the bounce control written into interrupts2a.py  
    GPIO.add_event_detect(22, GPIO.FALLING, callback=arm.start_playback, bouncetime=300)
    signal.signal(signal.SIGINT, arm.signal_handler)

    print("Starting the loop")
    while True:
        pass
