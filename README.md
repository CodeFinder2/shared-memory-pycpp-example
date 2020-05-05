# Overview
This example demonstrates how to use shared memory (via [`QSharedMemory`](https://doc.qt.io/qt-5/qsharedmemory.html)) and a system-wide semaphore (via [`QSystemSemaphore`](https://doc.qt.io/qt-5/qsystemsemaphore.html)) between a C++ application (the consumer on default) and a Python application (the producer on default) in Qt5. The behavior (producer vs. consumer) can be controlled in both sources via the `CONSUMER` define/variable: 0 means producer, 1 means consumer. Both apps print some logging information to the standard output.

Both for Python (see `prodcon_ipc` package) and C++ (see `*_ipc.{h,cpp}`files in `src/` directory), the relevent functionality is encapsulated in dedicated classes to ease re-usability.

In the accompanying example applications, the producer "produces" an image by loading them from a file from disk (when the user triggers it) and the consumer "consumes" these images by displaying them in the UI. The C++ application is designed in a threaded fashion so that it spawns a separate thread which waits for the system semaphore to be signaled to not block the UI thread. The Python consumer however (still) blocks when the user clicks the button at the bottom of the UI to activate the waiting (blocking). (It blocks until an image was "produced".)

# Installation
Install Qt5, PyQt5, CMake and a compiler (gcc or clang) on your system. Either compile the sources manually or use the provided `build.sh` script to compile and run it. The compiled C++ application will be placed in `build/shared_memory_cpp`. The Python application is `shared_memory.py`.

Tested with PyQt v5.10.1, Qt v5.9.5 and GCC v7.5.0 on Ubuntu 18.04.4 LTS.

# Specialities
There are some things to note:
- The system-semaphores to be used for the [producer-consumer problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem) are always reset to their defaults which allows for starting the apps in any order. However, this also has the consequence that when, for instance, the Python app was started first and already "produced" an image, it will be removed/lost if the C++ app is started afterwards. This can be avoided by adjusting the semaphore creation (using `QSystemSemaphore::Open` instead of `QSystemSemaphore::Create`, see docs) and depends on the actual interaction (design) of the applications.
- Shared memory is tricky in general because if an app crashes, the shared memory might not be removed properly. If that's the case, restarting an app might cause it to fail creating the shared memory (as its already existing). To avoid this, the Python app tries to re-attach the memory if this happens (see `load_from_file()`). It also tries to avoid exceptions/errors in when the shared memory is used (again, see `load_from_file()`).
- As an addition to the previous problem, a `shared_memory.key` file option has been added, allowing one to also (alternatively) specify the unique name of the shared memory in that file. It is used only if the file exists (in the app directory) and allows one to temporarily change the shared memory's name.
- Finally, take note about how shared memory is [released differently](https://doc.qt.io/qt-5/qsharedmemory.html#details) across operating systems.

# Credits
Based on:
- https://code.qt.io/cgit/qt/qtbase.git/tree/examples/corelib/ipc/sharedmemory?h=5.14
- https://github.com/baoboa/pyqt5/tree/master/examples/ipc/sharedmemory
