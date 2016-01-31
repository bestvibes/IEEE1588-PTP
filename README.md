# IEEE1588-PTP
An IEEE1588 Precision Time Protocol Time Synchronization Algorithm implementation with a goal to minimize latency and network jitter.

Implementations
================

1. **Python**: Probably the highest level implementation possible using standard sockets - **Status: Done**

2. **C**: Highest level implementation possible using C with standard socket system calls - **Status: Done**

3. **C with kernel timestamping**: One level lower using socket options to add kernel timestamping to reduce the non-deterministic delay from the time when the packet reaches the NIC to the time the userland records the time  - **Status: Icebox, too kernel/hardware specific, not enough support**

4. **C with kernel bypass using netmap**: Bypassing the kernel completely using netmap and talking directly to the NIC using mmap, ioctl, and ring buffers. Should result in the most accurate measurements and fastest execution  - **Status: WIP**

Structure
==========

**slave/** contains the implementation for the slave, which is a server that will take requests to sync from the master.

**master/** contains the implementation for the master which will initiate the sync protocol with the slave.

Running
===========

Run the slave on the designated slave machine first, then run the master from the designated master.

**Make sure you configure the slave IP address and the number of times to run the protocol at the top of `libs/common.h`. This implementation assumes that the master is a 64-bit machine, but the slave can be 32-bit.**

For instructions on building, run "make" in the root directory. The makefiles in the individual server and pi directories are for debugging purposes.


References
===========
Netmap: http://info.iet.unipi.it/~luigi/netmap/

IEEE1588 explanation: http://www.nist.gov/el/isd/ieee/upload/tutorial-basic.pdf
