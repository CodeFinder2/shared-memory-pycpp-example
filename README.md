# Overview
This example demonstrates how to use shared memory (via [`QSharedMemory`](https://doc.qt.io/qt-5/qsharedmemory.html)) and a system-wide semaphore (via [`QSystemSemaphore`](https://doc.qt.io/qt-5/qsystemsemaphore.html)) between a C++ application (the consumer) and a Python application (the producer). The producer "produces" images by loading them from a file from disk (when the user triggers it) and the consumer "consumes" theses images by displaying them in the UI. The C++ application can either be configured in a "threaded waiting" fashion (the default) so that it spawns a separate thread which waits for the system semaphore to be signaled to not block the UI thread. Alternatively (and just for demonstration purposes), the app might also block in its main thread (effectively blocking the UI) by `ENABLE_THREADED_WAITING` is *not* defined. In the latter case, the user needs to click the button at the bottom of the UI to activate the waiting (blocking).

Both apps print some logging information to the standard output.

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
