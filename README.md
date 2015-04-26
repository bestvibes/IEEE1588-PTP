# rPi-IEEE1588
An IEEE1588 Precision Time Protocol Time Synchronization Algorithm implementation for the Raspberry Pi with a goal to minimize latency and network jitter. The server is the slave and the client is the master.

Originally designed in Python, then implemented in C, and planning to use kernel bypass to reduce delay and jitter.

To build, run "make" in root directory. The makefiles in the individual server and pi directories are for debugging.
