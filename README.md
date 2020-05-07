*TL;DR: To get started quickly on Debian/Ubuntu like systems, simply run `./run.sh` which will install all missing dependencies, compile the sources and run both demo applications side-by-side. To simply re-use these wrappers, take a look at `prodcon_ipc/`(for Python) or `src/prodcon_ipc/` (for C++).*

# Table of Contents  
* [Overview](#overview)
* [Installation](#install)
* [Specialities](#special)
* [Credits](#credits)

# Overview <a name="overview"/>
This example demonstrates how to use shared memory (via [`QSharedMemory`](https://doc.qt.io/qt-5/qsharedmemory.html)) and a system-wide semaphore (via [`QSystemSemaphore`](https://doc.qt.io/qt-5/qsystemsemaphore.html)) between a C++ application (the consumer on default) and a Python application (the producer on default) in (Py)Qt5. The behavior (producer vs. consumer) can be controlled in both sources via the `CONSUMER` define/variable: 0 means producer, 1 means consumer. The Python app also supports the command arguments "`consumer`" or "`producer`" to specify its behavior, and, additionally, you can pass a path to an image to the Python example (which is then a producer) to just (possibly repetitively) load (aka produce) that image. Both apps print some logging information to the standard output to simplify what is happening in the background.

Both for Python (see `prodcon_ipc` package) and C++ (see `*_ipc.{h,cpp}`files in `src/` directory), the **relevent functionality is encapsulated in dedicated classes to ease re-usability** in Qt applications.

In the accompanying example applications, the producer "produces" an image by loading them from a file from disk (when the user triggers it) and the consumer "consumes" these images by displaying them in the UI. The C++ application is designed in a threaded fashion so that it spawns a separate thread which waits for the system semaphore to be signaled to not block the UI thread. The Python consumer however (still) blocks (because [threads in Python are a topic on its own](https://realpython.com/python-gil/#the-impact-on-multi-threaded-python-programs)) when the user clicks the button at the bottom of the UI to activate the waiting (blocking). (It blocks until an image was "produced".)

# Installation <a name="install"/>
Install Qt5, PyQt5, CMake and a compiler (gcc or clang) on your system. Either compile the sources manually or use the provided `run.sh` script to compile and run it. The compiled C++ application will be placed in `build/shared_memory_cpp`. The Python application is `shared_memory.py`.

Tested with PyQt v5.10.1, Qt v5.9.5 and GCC v7.5.0 on Ubuntu 18.04.4 LTS.

# Specialities <a name="special"/>
There are some things to note:
- The system-semaphores to be used for the [producer-consumer problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem) are **always reset to their defaults which allows for starting the apps in any order**. However, this also has the consequence that when, for instance, the Python app was started first and already "produced" an image, it will be removed/lost if the C++ app is started afterwards. This can be avoided by adjusting the semaphore creation (using `QSystemSemaphore::Open` instead of `QSystemSemaphore::Create`, see [docs](https://doc.qt.io/qt-5/qsystemsemaphore.html#AccessMode-enum)) and depends on the actual interaction (design) of the applications.
- **Shared memory can be tricky** in general because if an app crashes, the [shared memory might not be removed properly](https://stackoverflow.com/questions/42549904/qsharedmemory-is-not-getting-deleted-on-application-crash). If that's the case, restarting an app might cause it to fail creating the shared memory (as its already existing). To avoid this, the apps try to re-attach the memory if this happens (see e.g. `ProducerIPC::begin()`). It also tries to avoid exceptions/errors when the shared memory is used.
- As an addition to the previous problem, a `shared_memory.key` file option has been added, allowing one to also (alternatively) **specify the unique name of the shared memory in that file**. It is used only if the file exists (in the app directory) and allows one to temporarily change the shared memory's name.
- If multiple data/images should be stored in shared memory, the number of free slots in the semaphore needs to be increased (see `sem_empty`in `abstract_ipc.{cpp,py}`.
- Finally, take note about how **shared memory is [released differently](https://doc.qt.io/qt-5/qsharedmemory.html#details)** across operating systems.

# Credits <a name="credits"/>
Based on:
- https://code.qt.io/cgit/qt/qtbase.git/tree/examples/corelib/ipc/sharedmemory?h=5.14
- https://github.com/baoboa/pyqt5/tree/master/examples/ipc/sharedmemory
