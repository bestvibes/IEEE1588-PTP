#-------------------------------------------------------------------------------
# Name:        Raspberry Pi Socket Server
# Purpose:
#
# Author:      Vaibhav
#
# Created:     19/08/2014
# Copyright:   (c) Vaibhav 2014
# Licence:     <your licence>
#-------------------------------------------------------------------------------

import socket
import sys
from datetime import datetime

server_socket = None
ADDRESS = ""
PORT = 2468
OFFSETS = []
DELAYS = []

def main():
  global server_socket
  global OFFSETS
  global DELAYS
  try:
    print "Creating socket..."
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  except socket.error as e:
    print "Error creating socket: " + str(e) + ". Exitting..."
    server_socket.close();
    sys.exit(-1)

  try:
    print "Binding to socket... " + str(ADDRESS) + ":" + str(PORT)
    server_socket.bind((ADDRESS, PORT))
  except socket.error as e:
    print "Error binding to socket: " + str(e) + ". Exitting..."
    server_socket.close()
    sys.exit(-1)

##  try:
##    print "Starting listen on socket..."
##    server_socket.listen(5);
##    print "Now listening on socket..."
##  except socket.error as e:
##    print "Error listening: " + str(e) + ". Exitting..."
##    server_socket.close();
##    sys.exit(-1)

  try:
    while(1):
      print "\nReady to receive requests"
      data, addr = server_socket.recvfrom(4096)
      print "Request from " + addr[0]
      if("sync" == data):
        server_socket.sendto("ready", addr)
        num_of_times, addr = server_socket.recvfrom(4096)
        server_socket.sendto("ready", addr)
        for i in range(int(num_of_times)):
          sync_clock();
        print "\n\nAVG OFFSET: %s" % str(sum(OFFSETS) / len(OFFSETS)) + "\nAVG DELAY: %s"% str(sum(DELAYS) / len(DELAYS))
        print "\n\nMIN OFFSET: %s" % str(min(OFFSETS)) + "\nMIN DELAY: %s"% str(min(DELAYS))
        print "\n\nMAX OFFSET: %s" % str(max(OFFSETS)) + "\nMAX DELAY: %s"% str(max(DELAYS))
        #send(output, addr)
        OFFSETS = []
        DELAYS = []
      else:
        server_socket.sendto("Hello World!", addr)
  except socket.error as e:
    print "Error while handling requests: " + str(e)
    server_socket.close()
    sys.exit(-1)

def sync_clock():
  ms_diff, addr = sync_packet();
  sm_diff = delay_packet(addr)
  #print "ms_diff = " + str(ms_diff)
  #print "sm_diff = " + str(sm_diff)
  offset = (ms_diff - sm_diff)/2
  OFFSETS.append(offset)
  delay = (ms_diff + sm_diff)/2
  DELAYS.append(delay)
  send("ready", addr)

def sync_packet():
  t2, (t1, addr) = recv()
  ms_diff = long(t2) - long(t1)
  return (ms_diff, addr)

def delay_packet(addr):
  t3 = get_time(datetime.now())
  send("delay_req", addr)
  t, (t4, addr) = recv()
  sm_diff = long(t4) - long(t3)
  return sm_diff


def recv():
  try:
    request = server_socket.recvfrom(4096)
    t = get_time(datetime.now())
    #print "Request from " + str(request[1][0])
    return (t, request)
  except socket.error as e:
    print "Error while receiving request: " + str(e)
    server_socket.close()
    sys.exit(-1)

def send(data, addr):
  try:
    server_socket.sendto(str(data), addr)
    #print "Sent to " + addr[0]
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
