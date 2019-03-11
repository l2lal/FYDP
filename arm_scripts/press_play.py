#! /usr/bin/env python
from threading import Thread, Lock
import RPi.GPIO as GPIO
import serial
import signal
import sys
import time
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer

MUTEX = Lock()
steady_bool = False

# THREAD CLASS: Server_Thread -> Used to spawn server thread
class Server_Thread:  
    def __init__(self):
        self._running = True
        self.server_address = ('', 80)
        self.httpd = HTTPServer(self.server_address, MotorInterface)

    def terminate(self):
        print('closing server')
        self.httpd.shutdown()
        self._running = False  

    def run(self):
        print 'Starting httpd...'
        self.httpd.serve_forever()
 #END SERVER THREAD CLASS
    
class MotorInterface(object):
    def __init__(self, recording_freq, playback_freq):
        self._states = ["waiting", "recording", "playback"]
        self._current_state = 0
        self._motor_angles = []
        self._recording_index = 0
        self._playback_index = 0
        self._recording_freq = recording_freq
        self._playback_freq = playback_freq
        baud_rate = 9600
        self._ser_AX = serial.Serial("/dev/ttyACM0", baud_rate)
        self._ser_XL = serial.Serial("/dev/ttyACM1", baud_rate)
    
    def terminate(self):
        self._ser_AX.close()
        self._ser_XL.close()

    def start_recording(self, channel):
        if self._current_state != 1:
            print "now overwriting the previous recording!"
            self._current_state = 1
            self._recording_index = 0
            self._motor_angles = []
 
    def start_playback(self, channel):  
        if self._current_state != 2:
            if len(self._motor_angles) == 0:
                self._current_state = 0
                print "Nothing to be played back!"
                return
            self._current_state = 2
            self._playback_index = self._recording_index - 1

    def readBuffer(self, ser):
        data = ser.read()
        if data == len(data):
            print "reopening port"
            ser.close()
            ser.open()
        n = ser.inWaiting()
        for num in range(n):
            data += ser.read()
        return str(data)

    def read_serial_port(self, ser):
        response  = self.readBuffer(ser)
        response = response.strip("\r\n")
        response = response.split(",")
        checksum = response[-1]
        response = response[0:-1]
        received = ",".join(map(str, response))
        received += ','
        calculated = generateChecksum(received)
        reject = False
        for index in range(1,len(response)):
            try:
                int(response[index])
            except ValueError:
                reject = True
                continue
            if int(response[index]) > 1024 or response[index] < 0:
                reject = True
        if reject:
            response = []
    
        if checksum != str(calculated):
            print "Checksum mismatch!"
            print "received: " + checksum
            print "calculated: " + str(calculated)
            return None
        return response

    def write_to_serial_port(self, data, ser):
        #Send a message to request the motor positions
        # START_BIT, MODE, CHECKSUM
        msg = "s,"
        msg += ",".join(data)
        msg += ","
        msg += str(generateChecksum(msg))
        msg += "\n"
        ser.write(msg)

    def recording(self):
        msg = ["2"]
        self.write_to_serial_port(msg, self._ser_AX)
        self.write_to_serial_port(msg, self._ser_XL)

        #Get a response from the AX controllers
        AX_resp = self.read_serial_port(self._ser_AX)
        XL_resp = self.read_serial_port(self._ser_XL)

        if AX_resp != None and XL_resp != None:
            AX_resp = AX_resp[1:4]
            if len(AX_resp) == 3:
                self._motor_angles.append(AX_resp[0:3])
                #print self._motor_angles
                self._motor_angles[self._recording_index] += XL_resp[1]
                self._recording_index += 1
        else :
            print "No Response!"
        time.sleep(1.0/recording_freq)

    def playing(self):
        if (self._playback_index == 0):
            self._current_state = 0
            print "end of playback"
        AX_msg = ["1"]
        XL_msg = AX_msg
        AX_msg += self._motor_angles[self._playback_index][0:3]
        XL_msg += self._motor_angles[self._playback_index][3]
        self._playback_index -= 1;
        self.write_to_serial_port(AX_msg, self._ser_AX)
        self.write_to_serial_port(XL_msg, self._ser_XL)

        time.sleep(1.0/self._playback_freq)

    def waiting(self):
        msg = ["0"]
        self.write_to_serial_port(msg, self._ser_AX)
        self.write_to_serial_port(msg, self._ser_XL)
        time.sleep(1.0/self._playback_freq)

    def run(self):
        if self._current_state == 0:
            self.waiting()
        elif self._current_state == 1:
            self.recording()
        elif self._current_state == 2:
            self.playing()
    
    #HTTP SERVER FUNCTIONS
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        self.steady_mutex.acquire()
        self.wfile.write("<html><body><h1>hi!</h1></body></html>")
        self.stead_mutex.release()

    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        # Doesn't do anything with posted data
        self._set_headers()
        self.wfile.write("<html><body><h1>POST!</h1></body></html>")
    #END HTTP SERVER FUNCTIONS
        
def generateChecksum(data):
    checksum  = 0
    for char in data:
        checksum += ord(char)
    return (checksum % 256)

def terminate(arm, thread):
    print 'You pressed CTRL+C bro'
    arm.terminate()
    thread.terminate()
    sys.exit(0)
   
if __name__ == '__main__':
    recording_freq = 30.0
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
    GPIO.add_event_detect(17, GPIO.FALLING, callback=arm.start_recording, bouncetime=200)  
 
    # when a falling edge is detected on port 23, regardless of whatever   
    # else is happening in the program, the function my_callback2 will be run  
    # 'bouncetime=300' includes the bounce control written into interrupts2a.py  
    GPIO.add_event_detect(22, GPIO.FALLING, callback=arm.start_playback, bouncetime=200)
    
    # Starting webserver thread
    #Create Class
    pi_Server = Server_Thread()
    #Create Thread
    pi_ServerThread = Thread(target=pi_Server.run) 
    #Start Thread 
    pi_ServerThread.start()
    
    #Signal handler
    signal.signal(signal.SIGINT, terminate(arm, pi_Server))
   
    #print("Starting the loop")
    while True:
        arm.run()
