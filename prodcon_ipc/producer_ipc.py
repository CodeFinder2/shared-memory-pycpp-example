# -*- coding: utf-8 -*-

# Copyright (C) 2020 Adrian Böckenkamp
# This code is licensed under the BSD 3-Clause license (see LICENSE for details).

import abstract_ipc
import logzero


class ProducerIPC(abstract_ipc.AbstractIPC):
    """
    Given a system-wide producer-consumer problem, this class encapsulates the Inter-Process-Communication (IPC) for the
    producer. It allows simplified access to the underlying shared memory.

    Basically, to put data into the shared memory, call `begin()` with the amount of memory your data needs. You can
    then store the data under the provided `data` attribute. Afterwards, call `end()` to complete the transaction.
    """
    def __init__(self, id, key_file_path=None, log=True):
        """
        Creates the shared memory reference and the internal semaphores.

        :param id: Unique system-wide unique name (str) of shared memory; this name is also used to create the unique
        names for the two system-semapores `$id + "_sem_full"` and `$id + "_sem_empty"`
        :param key_file_path: Optional file path to a file typically named "shared_memory.key" whose first line is used
        as the unique ID/name of the shared memory IF this file exists, can be empty (the default) which then uses `id`
        (first parameter)
        :param log: `True` to enable logging using `logzero.logger`, `False` otherwise
        """
        abstract_ipc.AbstractIPC.__init__(self, id, key_file_path, log)

    def __del__(self):
        # VERY IMPORTANT: ensure to call unlock() if not needed anymore AND to
        # detach from the memory before exiting. Otherwise, the shmem is somewhat locked and
        # starting the application again will fail to create / access the shmem.
        self._shared_memory.detach()

    def begin(self, desired_memory_size):
        """
        Begins "producing" data for the memory, i.e., starts a transaction on the shared memory block. After writing to
        the memory, call `end()` but only do this if `begin()` succeeded previously.

        :param desired_memory_size: Amount of desired memory in bytes
        :return: a tuple (size, data) whereby `size` denotes the available shared memory size in bytes and `data` is
                the allocated memory; `size` MAY be <= `desired_memory_size` so ensure to only write up to `size` bytes
        :except: `RuntimeError` when the shared memory cannot be created / accessed (don't call `end()` then) or if a
                call to `end()` is missing
        """
        if self._transaction_started:
            raise RuntimeError("You must call end() first, cannot start a second transaction.")
        if self._shared_memory.isAttached():
            self._shared_memory.detach()

        # The following can fail if the app crashed previously being unable to detach from the shared memory:
        if not self._shared_memory.create(desired_memory_size):
            # Try to recover:
            if self._log:
                logzero.logger.warn("Shared memory seems to be still existing, unable to create it. Trying to "
                                    "recover by gaining ownership and detaching to delete it...")
            self._shared_memory.attach()
            self._shared_memory.detach()
            if not self._shared_memory.create(desired_memory_size):
                # We really still failed:
                raise RuntimeError("Unable to create or recover shared memory segment: " +
                                   self._shared_memory.errorString() + "\n\nYou probably need to reboot to fix this.")
            elif self._log:
                logzero.logger.info("Shared memory successfully created.")

        # Producer-consumer sync: we are the producer here, so wait for a free slot:
        if not self._sem_empty.acquire():
            raise RuntimeError("Unable to acquire the system semaphore (_sem_empty): " + self._shared_memory.errorString())

        if self._shared_memory.lock():
            self._transaction_started = True
            return min(self._shared_memory.size(), desired_memory_size), self._shared_memory.data()
        else:
            raise RuntimeError("Unable to lock the shared memory block: " + self._shared_memory.errorString())

    def end(self):
        """
        Ends a transaction on the shared memory. You must call this method when finished dealing with the memory
        referenced by `data`, returned by `begin()`.

        :return: None
        """
        if not self._transaction_started:
            raise RuntimeError("You must call begin() first.")
        if not self._shared_memory.unlock() and self.log:
            raise RuntimeError("Unlocking the shared memory failed: " + self._shared_memory.errorString())

        # We've written data, so let the consumer know that
        if not self._sem_full.release() and self.log:
            raise RuntimeError("Releasing the system semaphore failed: " + self._sem_full.errorString())
        self._transaction_started = False
        # Do not detech here to not let the shared memory be accidentally destroyed (e.g. on Windows).


class ScopedProducer(object):
    """
    Allows to use an object of ProducerIPC in combination with Python's "with" statement conveniently.
    """
    def __init__(self, producer_ipc, desired_memory_size):
        assert isinstance(producer_ipc, ProducerIPC)
        self.__prod_ipc = producer_ipc
        self.__size = desired_memory_size
        self.__success = False
        self.__data = None
        self.__avail_size = -1

    def __enter__(self):
        try:
            self.__avail_size, self.__data = self.__prod_ipc.begin(self.__size)
            self.__success = True
        except RuntimeError:
            self.__success = False
            raise
        return self

    def data(self):
        """
        Returns the shared memory to allow writing data to it

        :return: Bytes of the shared memory
        """
        return self.__data if self.__success else None

    def size(self):
        """
        Returns the available bytes in the shared memory block.

        :return: Size in bytes, maybe < desired_memory_size provided in the constructor
        """
        return self.__avail_size if self.__success else None

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.__success:
            self.__prod_ipc.end()
