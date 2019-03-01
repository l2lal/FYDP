#! /usr/bin/env python
import threading
import RPi.GPIO as GPIO
import serial

baud_rate = 9600
ser_AX = serial.Serial("/dev/ttyACM0", baud_rate)
ser_XL = serial.Serial("/dev/ttyACM0", baud_rate)

#TODO Remove these?
global states = ["waiting", "recording", "playback"]
global current_state = 0
global motor_angles = []
recording_index = 0
playback_index = 0
crc32 = crcmod.Crc(0x104c11db7, initCrc=0, xorOut=0xFFFFFFFF)

def start_recording(channel):
    if state == 2:
        display("now overwriting the previous recording!")
    state = 1
    recording_index = 0
    motor_angles = []
    threading.Timer(1/recording_freq, recording(recording_freq)).start()
  
def start_playback(channel):  
    if state == and len(motor_angles) != 0:
        state = 2
    elif len(motor_angles) == 0:
        display("Nothing to be played back!")
        return
    playback_index = 0
    threading.Timer(1/playback_freq, playing(playback_freq)).start()

def read_serial_port(ser):
    ser.open()
    n = ser.inWaiting()
    response  = ""
    for num in range(n):
        response += ser.read()
    ser.close()
    response = response.strip("\r\n")
    response = response.split(",")
    checksum = response[-1]
    response = response[0:-1]
    crc32.update(",".join(map(str, response)))

    if checksum != crc32.hexdigest()
        display("Checksum mismatch!")
        display("received: " + checksum)
        display("calculated: " + crc32.hexdigest())
        return NONE
    return response

def write_to_serial_port(data, ser):
    #Send a message to request the motor positions
    # START_BIT, MODE, CHECKSUM
    msg = "s,"
    msg += ",".join(data)
    crc32.update(msg)
    msg += ","
    msg += crc32.hexdigest()
    msg += "\n"
    ser.write(msg)

def recording(recording_freq):
    msg = "" #TODO please define this
    write_to_serial_port(msg, ser_AX)
    write_to_serial_port(msg, ser_XL)
    #Get a response from the AX controllers
    AX_resp = read_serial_port(ser_AX)
    XL_resp = read_serial_port(ser_XL)

    if AX_resp != NONE and XL_resp != NONE:
        motor_angles[recording_index] = AX_resp[1:4]
        motor_angles[recording_index] += XL_resp[1]
        recording_index += 1
    if state == 1:
        threading.Timer(1/recording_freq, recording(recording_freq)).start()


def playing(playback_freq):
    if (playback_index == len(motor_angles)):
        state = 0
    msg = "" #TODO please define this
    write_to_serial_port(msg, ser_AX)
    write_to_serial_port(msg, ser_XL)
    if state == 2:
        threading.Timer(1/playback_freq, playing(playback_freq)).start() 


if __name__ == '__main__':
    recording_freq = 40
    playback_freq = recording_freq
      
    # when a falling edge is detected on port 17, regardless of whatever   
    # else is happening in the program, the function my_callback will be run  
    GPIO.add_event_detect(17, GPIO.FALLING, callback=start_recording, bouncetime=300)  
    
    # when a falling edge is detected on port 23, regardless of whatever   
    # else is happening in the program, the function my_callback2 will be run  
    # 'bouncetime=300' includes the bounce control written into interrupts2a.py  
    GPIO.add_event_detect(22, GPIO.FALLING, callback=start_playback, bouncetime=300)

    while true:
        pass
