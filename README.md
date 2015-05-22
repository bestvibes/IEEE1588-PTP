# rPi-IEEE1588
An IEEE1588 Precision Time Protocol Time Synchronization Algorithm implementation with a goal to minimize latency and network jitter. The server is the slave and the client is the master.

Originally designed in Python, then implemented in C, C using kernel timestamping, and kernel bypass to reduce delay and jitter.

For instructions on building, run "make" in root directory. The makefiles in the individual server and pi directories are for debugging.
