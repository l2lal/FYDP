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
camera_ready = 0

#HTTP SERVER FUNCTIONS
class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        global MUTEX
        global camera_ready 
        self._set_headers()
        MUTEX.acquire()
        self.wfile.write("<html><body><h1>" + str(camera_ready) + "</h1></body></html>")
        camera_ready = 0
        MUTEX.release()

    def do_HEAD(self):
        self._set_headers()

    def do_POST(self):
        # Doesn't do anything with posted data
        self._set_headers()
        self.wfile.write("<html><body><h1>POST!</h1></body></html>")
#END HTTP SERVER FUNCTIONS
    
# THREAD CLASS: Server_Thread -> Used to spawn server thread
class Server_Thread:  
    def __init__(self):
        self._running = True
        self.server_address = ('', 80)
        self.httpd = HTTPServer(self.server_address, S)

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
        self._playback_freq = playback_freq /2
        baud_rate = 9600
        self._ser_AX = serial.Serial("/dev/ttyACM0", baud_rate)
        self._ser_XL = serial.Serial("/dev/ttyACM1", baud_rate)
        self._ser_Light = serial.Serial("/dev/ttyUSB0", baud_rate)
        self._window_size = int(recording_freq * 0.75)
        self._pulse_hysteresis_threshold = 5
        self._pause_points = []
        self._acquire_position = False

    def signal_handler(self):
        print('You pressed Ctrl+C!')
    
    def terminate(self):
        self._ser_AX.close()
        self._ser_XL.close()
        self._ser_Light.close()

    def save_position(self, channel):
        if self._current_state == 1:
            self._acquire_position = True

    def start_recording(self, channel):
        if self._current_state != 1:
            print "now overwriting the previous recording!"
            self._current_state = 1
            self._recording_index = 0
            self._motor_angles = []
            self._pause_points = []
 
    def start_playback(self, channel):
        print self._pause_points
        if self._current_state != 2:
            if len(self._pause_points) == 0:
                self._current_state = 0
                print "Nothing to be played back!"
                return
            self._current_state = 2
            self._playback_index = len(self._pause_points) - 1

    #find when the arm is stalled and update the status on our webserver
    def find_pause_points(self):
        index = self._window_size
        while index < len(self._motor_angles):
            low_index = index - self._window_size
            #TODO the code is a bit too chunky
            if abs(self._motor_angles[index][0] - self._motor_angles[low_index][0]) < self._pulse_hysteresis_threshold and\
               abs(self._motor_angles[index][1] - self._motor_angles[low_index][1]) < self._pulse_hysteresis_threshold and\
               abs(self._motor_angles[index][2] - self._motor_angles[low_index][2]) < self._pulse_hysteresis_threshold and\
               abs(self._motor_angles[index][3] - self._motor_angles[low_index][3]) < self._pulse_hysteresis_threshold:
                average = []
                average.append((self._motor_angles[index][0] + self._motor_angles[low_index][0])/2)
                average.append((self._motor_angles[index][1] + self._motor_angles[low_index][1])/2)
                average.append((self._motor_angles[index][2] + self._motor_angles[low_index][2])/2)
                average.append((self._motor_angles[index][3] + self._motor_angles[low_index][3])/2)

                upper_bound  = [(self._pulse_hysteresis_threshold/2)+x for x in average]
                lower_bound = [x-(self._pulse_hysteresis_threshold/2) for x in average]
                safe = True
                for ii in range(index - self._window_size, index):
                    if self._motor_angles[ii][0] < lower_bound[0] or\
                       self._motor_angles[ii][1] < lower_bound[1] or\
                       self._motor_angles[ii][2] < lower_bound[2] or\
                       self._motor_angles[ii][3] < lower_bound[3]:
                       safe = False
                    if self._motor_angles[ii][0] > upper_bound[0] or\
                       self._motor_angles[ii][1] > upper_bound[1] or\
                       self._motor_angles[ii][2] > upper_bound[2] or\
                       self._motor_angles[ii][3] > upper_bound[3]:
                       safe = False
                if safe:
                    if average not in self._pause_points:
                        self._pause_points.append(average)
                        index += self._window_size/2
            index += 1

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
        #print "Response"
        #print response
        response = response.strip("\r\n")
        response = response.split(",")
        if response[0] != 's' or len(response) <= 2:
            return None
        checksum = 0
        reject = False
        for index in range(1,len(response)):
            try:
                response[index] = int(response[index])
            except ValueError:
                reject = True
                continue
            if response[index] > 1024 or response[index] < 0:
                reject = True
            if index == len(response) - 1:
                checksum = response[index]
        response = response[0:-1]
        received = ",".join(map(str, response))
        received += ','
        calculated = generateChecksum(received)
        if reject:
            return None
    
        if checksum != calculated:
            print "Checksum mismatch!"
            print "received: " + checksum
            print "calculated: " + str(calculated)
            print response
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
        #print "I sent" 
        #print msg

    def recording(self):
        msg = ["2"]
        self.write_to_serial_port(msg, self._ser_AX)
        self.write_to_serial_port(msg, self._ser_XL)
        self._ser_Light.write("1")

        #Get a response from the AX controllers
        AX_resp = self.read_serial_port(self._ser_AX)
        XL_resp = self.read_serial_port(self._ser_XL)

        if AX_resp != None and XL_resp != None:
            AX_resp = AX_resp[1:4]
            #AX_resp = [int(x) for x in AX_resp]
            if len(AX_resp) == 3 and self._acquire_position:
                self._acquire_position = False
                self._pause_points.append(AX_resp[0:3])
                self._pause_points[self._recording_index].append(XL_resp[1])
                self._recording_index += 1
                print "recorded a position\n"
        else :
            print "No Response! trying again"
        time.sleep(1.0/self._recording_freq)

    def take_picture(self, repeat):
        for times in range(0, repeat):
            time.sleep(2)
            MUTEX.acquire()
            camera_ready = 1
            MUTEX.release()
            print "taking picture"
        
    def playing(self):
        global MUTEX
        global camera_ready 
        if (self._playback_index == -1):
            self._current_state = 0
            print "end of playback"
            MUTEX.acquire()
            camera_ready = 2
            MUTEX.release()
            return
        AX_msg = ["1"]
        XL_msg = ["1"]
        AX_msg.append(str(self._pause_points[self._playback_index][0]))
        AX_msg.append(str(self._pause_points[self._playback_index][1]))
        AX_msg.append(str(self._pause_points[self._playback_index][2]))
        XL_msg.append(str(self._pause_points[self._playback_index][3]))
        self.write_to_serial_port(AX_msg, self._ser_AX)
        self.write_to_serial_port(XL_msg, self._ser_XL)
        self._ser_Light.write("1")

        time.sleep(1.0)
        #TODO if the pause point is reached update the boolean, wait for the image to be captured, change the index
        AX_msg = ["3"]
        XL_msg = ["3"]
        self.write_to_serial_port(AX_msg, self._ser_AX)
        self.write_to_serial_port(XL_msg, self._ser_XL)

        AX_resp = self.read_serial_port(self._ser_AX)
        XL_resp = self.read_serial_port(self._ser_XL)

        if AX_resp != None and XL_resp != None:
            if len(AX_resp) >= 1 and AX_resp[1] == 1 and XL_resp[1] == 1:
                self._playback_index -= 1
                self.take_picture(3)
                self._ser_Light.write("2")
                time.sleep(0.4)
            else:
                print "AX"
                print AX_resp[1]
                print XL_resp[1]
        else :
            print "No Response!"
        
    def waiting(self):
        msg = ["0"]
        self.write_to_serial_port(msg, self._ser_AX)
        self.write_to_serial_port(msg, self._ser_XL)
        self._ser_Light.write("0")
        time.sleep(1.0/self._playback_freq)

    def run(self):
        if self._current_state == 0:
            self.waiting()
        elif self._current_state == 1:
            self.recording()
        elif self._current_state == 2:
            self.playing()
        
def generateChecksum(data):
    checksum  = 0
    for char in data:
        checksum += ord(char)
    return (checksum % 256)

def terminate(signum, frame):
    print 'You pressed CTRL+C bro'
    arm.terminate()
    pi_Server.terminate()
    sys.exit(0)
    
recording_freq = 50.0
playback_freq = recording_freq
arm = MotorInterface(recording_freq, playback_freq)  

# Starting webserver thread
#Create Class
pi_Server = Server_Thread()
#Create Thread
pi_ServerThread = Thread(target=pi_Server.run) 
#Start Thread 
pi_ServerThread.start()

if __name__ == '__main__':
    GPIO.setmode(GPIO.BCM)
    # GPIO 23 & 17 set up as inputs, pulled up to avoid false detection.  
    # Both ports are wired to connect to GND on button press.  
    # So we'll be setting up falling edge detection for both  
    GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)  
    GPIO.setup(17, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(23, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    # when a falling edge is detected on port 17, regardless of whatever   
    # else is happening in the program, the function my_callback will be run  
    GPIO.add_event_detect(17, GPIO.FALLING, callback=arm.start_recording, bouncetime=200)  
 
    # when a falling edge is detected on port 23, regardless of whatever   
    # else is happening in the program, the function my_callback2 will be run  
    # 'bouncetime=300' includes the bounce control written into interrupts2a.py  
    GPIO.add_event_detect(22, GPIO.FALLING, callback=arm.start_playback, bouncetime=200)
    GPIO.add_event_detect(23, GPIO.FALLING, callback=arm.save_position, bouncetime=200)
    
    #Signal handler
    signal.signal(signal.SIGINT, terminate)
   
    #print("Starting the loop")
    while True:
        arm.run()
