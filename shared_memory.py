#!/usr/bin/env python
# -*- coding: utf-8 -*-

#############################################################################
##
## Copyright (C) 2020 Adrian Böckenkamp
## Copyright (C) 2013 Riverbank Computing Limited
## Copyright (C) 2010 Hans-Peter Jansen <hpj@urpla.net>.
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
##
## This file is part of the examples of PyQt.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial Usage
## Licensees holding valid Qt Commercial licenses may use this file in
## accordance with the Qt Commercial License Agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Nokia.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain additional
## rights.  These rights are described in the Nokia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
## If you have questions regarding the use of this file, please contact
## Nokia at qt-info@nokia.com.
## $QT_END_LICENSE$
##
#############################################################################

# Abstract: this Python app contains the producer logic. Upon selecting an image file to load, it
#           will be displayed in the UI and copied to the shared memory (producing). The consumer
#           can then display (= consume) it as well. Don't (try to) load another image if the
#           previous image hasn't been consumed; otherwise the app will block (until consumed). By
#           slightly changing the code base of both app the behavior can be inverted.

from PyQt5.QtCore import QBuffer, QDataStream, QSharedMemory, QSystemSemaphore, PYQT_VERSION_STR, QT_VERSION_STR
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtWidgets import QApplication, QDialog, QFileDialog
from logzero import logger

from dialog import Ui_Dialog

UNIQUE_SHARED_MEMORY_NAME = "MySharedMemoryDefault"
UNIQUE_SEMAPHORE_EMPTY = "MySemaphoreEmpty"
UNIQUE_SEMAPHORE_FULL = "MySemaphoreFull"
SHARED_MEMORY_KEY_FILE = "shared_memory.key"


class ProducerDialog(QDialog):
    def __init__(self, parent=None):
        super(ProducerDialog, self).__init__(parent)
        logger.info("Using PyQt v" + PYQT_VERSION_STR + " and Qt v" + QT_VERSION_STR)

        key = ProducerDialog.load_key(SHARED_MEMORY_KEY_FILE)
        self.sharedMemory = QSharedMemory(key if key else UNIQUE_SHARED_MEMORY_NAME)
        logger.debug("Creating shared memory with key=\"" + self.sharedMemory.key() + "\" (" +
                     ("loaded from file)" if key else "hardcoded)"))

        self.ui = Ui_Dialog()
        self.ui.setupUi(self)
        # We only allow one image to be put into the shared memory (otherwise we would need to
        # create a dedicated data structure within the SHARED memory and access it from Python and
        # C++ appropriately). Also note that "Open" and "Create" have very different semantics (see
        # docs) and we use "Create" to always create a "fresh" semaphore to not cause undesired
        # blocking/stalls, see also https://doc.qt.io/qt-5/qsystemsemaphore.html
        self.sem_empty = QSystemSemaphore(UNIQUE_SEMAPHORE_EMPTY, 1, QSystemSemaphore.Create)
        self.sem_full = QSystemSemaphore(UNIQUE_SEMAPHORE_FULL, 0, QSystemSemaphore.Create)

        self.ui.loadFromFileButton.clicked.connect(self.load_from_file)
        self.ui.loadFromSharedMemoryButton.clicked.connect(self.load_from_memory)
        # Since we pretend to be the producer (as a demo, to make it clear), disable load from
        # memory (= consuming):
        self.ui.loadFromSharedMemoryButton.setEnabled(False)

        self.setWindowTitle("Shared Memory: Python Example")

    @staticmethod
    def load_key(path="shared_memory.key"):
        # noinspection PyBroadException
        try:
            with open(path) as fp:
                line = fp.readline().strip()
                if fp.readline():
                    logger.warn("Ignoring residual lines in " + path)
                return line
        except:
            pass
        return None

    def load_from_file(self):
        self.ui.label.setText("Select an image file")
        file_name, t = QFileDialog.getOpenFileName(self, None, None, "Images (*.png *.jpg)")
        if not file_name:
            return

        image = QImage()
        if not image.load(file_name):
            self.ui.label.setText("Selected file is not an image, please select another.")
            return

        self.ui.label.setPixmap(QPixmap.fromImage(image))

        if self.sharedMemory.isAttached():
            self.detach()

        # Load into shared memory.
        buf = QBuffer()
        buf.open(QBuffer.ReadWrite)
        out = QDataStream(buf)
        out << image
        size = buf.size()

        # Producer-consumer sync: we are the producer here, so wait for a free slot:
        self.sem_empty.acquire()

        # The following can fail if the app crashed previously being unable to detach from the shared memory:
        if not self.sharedMemory.create(size):
            # Try to recover:
            logger.warn("Shared memory seem to be still existing, unable to create it. Trying to "
                        "recover by gaining ownership and detaching to delete it...")
            self.sharedMemory.attach()
            self.sharedMemory.detach()
            if not self.sharedMemory.create(size):
                # We really still failed:
                self.ui.label.setText("Unable to create or recover shared memory segment: " +
                                      self.sharedMemory.errorString() + "\n\nYou probably need to "
                                      "reboot to fix this.")
            else:
                logger.info("Shared memory successfully created.")
            return

        size = min(self.sharedMemory.size(), size)
        error_str = None

        self.sharedMemory.lock()
        # Copy image data from buf into shared memory area.
        try:
            #self.sharedMemory.data()[:size] = buf.data().data()[:size]
            self.sharedMemory.data()[0] = 'h'
            self.sharedMemory.data()[1] = 'e'
            self.sharedMemory.data()[2] = 'l'
            self.sharedMemory.data()[3] = 'l'
            self.sharedMemory.data()[4] = 'o'
            self.sharedMemory.data()[5] = '\0'
        except Exception as err:
            error_str = str(err)
        self.sharedMemory.unlock()

        if error_str:
            self.ui.label.setText(error_str)

        # We've added an image, so let the consumer (C++ app) know that
        self.sem_full.release()
        # Do not detech here to not let the shared memory be accidentally destroyed (e.g. on Windows).

    def load_from_memory(self):
        if not self.sharedMemory.attach():
            self.ui.label.setText(
                "Unable to attach to shared memory segment.\nLoad an "
                "image first.")
            return

        buf = QBuffer()
        ins = QDataStream(buf)
        image = QImage()

        self.sharedMemory.lock()
        buf.setData(self.sharedMemory.constData())
        buf.open(QBuffer.ReadOnly)
        ins >> image
        self.sharedMemory.unlock()
        self.sharedMemory.detach()

        self.ui.label.setPixmap(QPixmap.fromImage(image))

    def detach(self):
        if not self.sharedMemory.detach():
            self.ui.label.setText("Unable to detach from shared memory.")


if __name__ == '__main__':
    import sys

    app = QApplication(sys.argv)
    dialog = ProducerDialog()
    dialog.show()
    result = 0
    # noinspection PyBroadException
    try:
        result = app.exec_()
    except Exception as e:
        pass
    # VERY IMPORTANT: catch all exceptions inside the lock() to ensure to call unlock() AND
    # detach from the memory if not needed anymore! Otherwise, the shmem is somewhat locked and
    # starting the application again will fail to create / access the shmem.
    dialog.detach()
    sys.exit(result)
