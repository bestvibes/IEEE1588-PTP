import socket
import sys
from datetime import datetime
import time

server_socket = None
ADDRESS = "127.0.0.1"
PORT = 2468
NUM_OF_TIMES = 1000


def main():
    try:
        global server_socket
        print("Creating socket...")
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    except socket.error as e:
        print("Error creating socket: " + str(e) + ". Exitting...")
        server_socket.close()
        sys.exit(-1)

    try:
        print("Connecting to socket... " + str(ADDRESS) + ":" + str(PORT))
        server_socket.connect((ADDRESS, PORT))
    except socket.error as e:
        print("Error connecting to socket: " + e + ". Exitting...")
        server_socket.close()
        sys.exit(-1)

    send(255)
    # send(get_time(datetime.now()))


def recv():
    try:
        request = server_socket.recv(4096)
        t = get_time(datetime.now())
        # print "Response: " + str(request)
        return (t, request)
    except socket.error as e:
        print("Error while receiving request: " + str(e))
        server_socket.close()
        sys.exit(-1)


def send(data):
    try:
        server_socket.sendall(data)
        # print "Sent:" + str(data)
    except socket.error as e:
        print("Error while sending request: " + str(e))
        print("Tried to send: " + data)
        server_socket.close()
        sys.exit(-1)


def get_time():
    return time.time()


if __name__ == '__main__':
    main()
