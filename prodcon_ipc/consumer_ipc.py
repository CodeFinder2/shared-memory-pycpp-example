# -*- coding: utf-8 -*-

# Copyright (C) 2020 Adrian Böckenkamp
# This code is licensed under the BSD 3-Clause license (see LICENSE for details).

import abstract_ipc
import logzero


class ConsumerIPC(abstract_ipc.AbstractIPC):
    """
    Given a system-wide producer-consumer problem, this class encapsulates the Inter-Process-Communication (IPC) for the
    consumer. It allows simplified access to the underlying shared memory.

    Basically, the signal  available() is emitted once data was produced and put into the shared
 * memory. Once triggered, use \c begin() ... \c end() to access the data.
    """
    def __init__(self, log=True):
        """
        Creates the shared memory reference and the internal semaphores.

        :param log: `True` to enable logging using `logzero.logger`, `False` otherwise
        """
        abstract_ipc.AbstractIPC.__init__(self, log)

    def begin(self):
        """
        Starts reading from the shared memory. Blocks if not available.

        :return: Data in shared memory
        """
        if self._transaction_started:
            raise RuntimeError("You must call end() first, cannot start a another transaction.")

        if not self._sem_full.acquire():
            raise RuntimeError("Unable to acquire system semaphore (_sem_full): " + self._sem_full.errorString())

        if not self._shared_memory.attach():
            self._sem_full.release()  # undo
            raise RuntimeError("Unable to attach to shared memory segment: " + self._shared_memory.errorString())

        if not self._shared_memory.lock():
            err_str = self._shared_memory.errorString()  # may be overwritten by errors in detach()
            self._sem_full.release()  # dito
            self._shared_memory.detach()  # dito
            raise RuntimeError("Unable to attach to shared memory segment: " + err_str)
        self._transaction_started = True
        return self._shared_memory.constData()

    def end(self):
        """
        Stops reading from shared memory.
        """
        if not self._transaction_started:
            raise RuntimeError("You must call begin() first.")
        if not self._shared_memory.unlock():
            raise RuntimeError("Unable to unlock shared memory segment: " + self._shared_memory.errorString())
        if not self._shared_memory.detach() and self.log:
            logzero.logger.error("Unable to detach shared memory: " + self._shared_memory.errorString())
        if not self._sem_empty.release():
            raise RuntimeError("Unable to release system semaphore (_sem_empty): " + self._sem_empty.errorString())
        self._transaction_started = False
