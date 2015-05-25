#-------------------------------------------------------------------------------
# Name:        Raspberry Pi Socket Client
# Purpose:
#
# Author:      Vaibhav
#
# Created:     20/08/2014
# Copyright:   (c) Vaibhav 2014
# Licence:     <your licence>
#-------------------------------------------------------------------------------

import socket
import sys
from datetime import datetime
import time

server_socket = None
ADDRESS = "10.0.0.19"
#ADDRESS = "192.168.137.2"
#ADDRESS = "127.0.0.1"
PORT = 2468
NUM_OF_TIMES = 1000

def main():
  try:
    global server_socket
    print "Creating socket..."
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  except socket.error as e:
    print "Error creating socket: " + str(e) + ". Exitting..."
    server_socket.close();
    sys.exit(-1)

  try:
    print "Connecting to socket... " + str(ADDRESS) + ":" + str(PORT)
    server_socket.connect((ADDRESS, PORT))
  except socket.error as e:
    print "Error connecting to socket: " + e + ". Exitting..."
    server_socket.close()
    sys.exit(-1)

  send("hello world")
  #send(get_time(datetime.now()))

def recv():
  try:
    request = server_socket.recv(4096)
    t = get_time(datetime.now())
    #print "Response: " + str(request)
    return (t, request)
  except socket.error as e:
    print "Error while receiving request: " + str(e)
    server_socket.close()
    sys.exit(-1)

def send(data):
  try:
    server_socket.sendall(str(data))
    #print "Sent:" + str(data)
  except socket.error as e:
    print "Error while sending request: " + str(e)
    print "Tried to send: " + data
    server_socket.close()
    sys.exit(-1)
    
def get_time(time):
  #hours = time.hour * 3600000000
  minutes = time.minute * 60000000
  seconds = time.second * 1000000
  #milliseconds = time.millisecond * 1000
  microseconds = time.microsecond
  return minutes + seconds + microseconds

if __name__ == '__main__':
    main()