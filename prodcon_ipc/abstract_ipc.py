# -*- coding: utf-8 -*-

# Copyright (C) 2020 Adrian Böckenkamp
# This code is licensed under the BSD 3-Clause license (see LICENSE for details).

import logzero
import PyQt5.QtCore


class AbstractIPC(object):
    """
    Encapsulates code that both the producer and the consumer requires.
    """
    def __init__(self, id, key_file_path=None, log=True):
        """
        Creates the underlying system resources.

        :param id: Unique system-wide unique name (str) of shared memory; this name is also used to create the unique
        names for the two system-semapores `$id + "_sem_full"` and `$id + "_sem_empty"`
        :param key_file_path: Optional file path to a file typically named "shared_memory.key" whose first line is used
        as the unique ID/name of the shared memory IF this file exists, can be empty (the default) which then uses `id`
        (first parameter)
        :param log: `True` to enable logging using `logzero.logger`, `False` otherwise
        """
        self._log = log
        self._transaction_started = False
        if self._log:
            logzero.logger.info("Using PyQt v" + PyQt5.QtCore.PYQT_VERSION_STR + " and Qt v" +
                                PyQt5.QtCore.QT_VERSION_STR)

        self._file_key = self._load_key(key_file_path)
        self._shared_memory = PyQt5.QtCore.QSharedMemory(self._file_key if self._file_key else str(id))
        if self._log:
            logzero.logger.debug("Creating shared memory with key=\"" + self._shared_memory.key() + "\" (" +
                                 ("loaded from file)" if self._file_key else "hardcoded)"))
        self._sem_empty = PyQt5.QtCore.QSystemSemaphore(str(id) + "_sem_empty", 1, PyQt5.QtCore.QSystemSemaphore.Create)
        self._sem_full = PyQt5.QtCore.QSystemSemaphore(str(id) + "_sem_full", 0, PyQt5.QtCore.QSystemSemaphore.Create)

    def _load_key(self, path):
        """
        Alternatively loads the shared memory's name from a file named `SHARED_MEMORY_KEY_FILE`.

        :param path: Path to file whose first name contains the unique name; the rest is ignored
        :return: Unique name of shared memory or None if such a file did not exist
        """
        if path is None:
            return None
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
