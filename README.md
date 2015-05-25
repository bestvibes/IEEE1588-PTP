# rPi-IEEE1588
An IEEE1588 Precision Time Protocol Time Synchronization Algorithm implementation with a goal to minimize latency and network jitter. The slave is a server so it has to be run first.

Originally designed in Python, then implemented in C, C using kernel timestamping, and kernel bypass to reduce delay and jitter.

For instructions on building, run "make" in root directory. The makefiles in the individual server and pi directories are for debugging.
