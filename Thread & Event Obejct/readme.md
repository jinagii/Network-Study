Thread & Event Object study

- Thread is kind of unit object that handles logics in a process.
- Every process has at least one thread (= main thread)
- Each thread in single process has individual thread stack.
- They share a parent's process memory area.

Functions for Thread

WaitForSingleObject()
WaitForMulipleObject()
_beginthreadex()
SuspendThread()
ResumeThread()
GetExitCodeThread()
etc.

or we can use STL Thread
<thread>
thread ThreadFunction(proc, 1);
~~
ThreadFunction.join();
  
  
Functions for Event

CreateEvent()
SetEvent()
RestEvent()
CloseHandle()
etc.
