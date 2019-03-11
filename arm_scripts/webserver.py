#!/usr/bin/env python
"""
Very simple HTTP server in python.
Usage::
    ./dummy-web-server.py [<port>]
Send a GET request::
    curl http://localhost
Send a HEAD request::
    curl -I http://localhost
Send a POST request::
    curl -d "foo=bar&bin=baz" http://localhost
"""
from threading import Thread
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import time

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

class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        self.wfile.write("<html><body><h1>hi!</h1></body></html>")

    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        # Doesn't do anything with posted data
        self._set_headers()
        self.wfile.write("<html><body><h1>POST!</h1></body></html>")
        

if __name__ == "__main__":
    #Create Class
    pi_Server = Server_Thread()
    #Create Thread
    pi_ServerThread = Thread(target=pi_Server.run) 
    #Start Thread 
    pi_ServerThread.start()

    #When to end? 
    time.sleep(15)
    print 'goodbye...'
    pi_Server.terminate()