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
import time

server_socket = None
ADDRESS = ""
PORT = 2468

def main():
  global server_socket
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

  try:
    while(1):
      print "\nReady to receive requests on port " + str(PORT) + "..."
      data, addr = server_socket.recvfrom(4096)
      print "Request from " + addr[0]
      if("sync" == data):
        server_socket.sendto("ready", addr)
        num_of_times, addr = server_socket.recvfrom(4096)
        server_socket.sendto("ready", addr)
        for i in range(int(num_of_times)):
          sync_clock();
        print "Done!"
      else:
        server_socket.sendto("Hello World!", addr)
  except socket.error as e:
    print "Error while handling requests: " + str(e)
    server_socket.close()
    sys.exit(-1)

def sync_clock():
  addr = sync_packet();
  delay_packet(addr)
  recv()

def sync_packet():
  t2, (t1, addr) = recv()
  send(t2, addr)
  return addr

def delay_packet(addr):
  recv()
  send(get_time(), addr)


def recv():
  try:
    request = server_socket.recvfrom(4096)
    t = get_time()
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

def get_time():
  #hours = time.hour * 3600000000
  #minutes = time.minute * 60000000
  #seconds = time.second * 1000000
  #milliseconds = time.millisecond * 1000
  #microseconds = time.microsecond
  #return minutes + seconds + microseconds
  return time.time()

if __name__ == '__main__':
    main()
