# -*- coding: utf-8 -*-

# Copyright (C) 2020 Adrian Böckenkamp
# This code is licensed under the BSD 3-Clause license (see LICENSE for details).

import logzero
import PyQt5.QtCore


class AbstractIPC(object):
    """
    Encapsulates code that both the producer and the consumer requires.
    """
    UNIQUE_SHARED_MEMORY_NAME = "MySharedMemoryDefault"
    UNIQUE_SEMAPHORE_EMPTY = "MySemaphoreEmpty"
    UNIQUE_SEMAPHORE_FULL = "MySemaphoreFull"
    SHARED_MEMORY_KEY_FILE = "shared_memory.key"

    def __init__(self, log=True):
        """
        Creates the underlying system ressources.

        :param log: `True` to enable logging using `logzero.logger`, `False` otherwise
        """
        self._log = log
        self._transaction_started = False
        if self._log:
            logzero.logger.info("Using PyQt v" + PyQt5.QtCore.PYQT_VERSION_STR + " and Qt v" +
                                PyQt5.QtCore.QT_VERSION_STR)

        self._file_key = self._load_key(AbstractIPC.SHARED_MEMORY_KEY_FILE)
        self._shared_memory = PyQt5.QtCore.QSharedMemory(self._file_key if self._file_key else
                                                         AbstractIPC.UNIQUE_SHARED_MEMORY_NAME)
        if self._log:
            logzero.logger.debug("Creating shared memory with key=\"" + self._shared_memory.key() + "\" (" +
                                 ("loaded from file)" if self._file_key else "hardcoded)"))
        self._sem_empty = PyQt5.QtCore.QSystemSemaphore(AbstractIPC.UNIQUE_SEMAPHORE_EMPTY, 1,
                                                        PyQt5.QtCore.QSystemSemaphore.Create)
        self._sem_full = PyQt5.QtCore.QSystemSemaphore(AbstractIPC.UNIQUE_SEMAPHORE_FULL, 0,
                                                       PyQt5.QtCore.QSystemSemaphore.Create)

    def _load_key(self, path=SHARED_MEMORY_KEY_FILE):
        """
        Alternatively loads the shared memory's name from a file named `SHARED_MEMORY_KEY_FILE`.

        :param path: Path to file whose first name contains the unique name; the rest is ignored
        :return: Unique name of shared memory or None if such a file did not exist
        """
        # noinspection PyBroadException
        try:
            with open(path) as fp:
                line = fp.readline().strip()
                if fp.readline() and self._log:
                    logzero.logger.warn("Ignoring residual lines in " + path)
                return line
        except:
            pass
        return None
