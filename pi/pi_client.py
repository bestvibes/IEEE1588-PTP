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
ADDRESS = "192.168.137.2"
#ADDRESS = "127.0.0.1"
PORT = 2468
NUM_OF_TIMES = 100

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

  sync_clock()
  #ping()

def ping():
  print "\nPinging"
  send("ping")
  t, resp = recv()
  print resp
  
def sync_clock():
  print "\nSyncing time..."
  #send("sync")
  #time.sleep(1)
  send("sync")
  t, resp = recv()
  if(resp == "ready"):
    send(str(NUM_OF_TIMES))

  t, resp = recv()

  if(resp == "ready"):
    time.sleep(1) #to allow for server to get ready
    for i in range(NUM_OF_TIMES):
      sync_packet()
      delay_packet()
      recv()
      #if(i % 100 == 0):
      #  print i
    #t, resp = recv()
    #print "\n\n" + resp
    print "Done!"
  else:
    print "Error syncing times, received: " + resp

def sync_packet():
  send(get_time(datetime.now()))

def delay_packet():
  t4, delay_string = recv()
  send(t4)


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