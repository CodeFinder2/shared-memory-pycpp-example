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

from __future__ import with_statement

# Abstract: this Python app contains the producer logic. Upon selecting an image file to load, it
#           will be displayed in the UI and copied to the shared memory (producing). The consumer
#           can then display (= consume) it as well. Don't (try to) load another image if the
#           previous image hasn't been consumed; otherwise the app will block (until consumed). By
#           changing CONSUMER of both apps, the behavior (consumer/producer) can be inverted.

CONSUMER = 0

UNIQUE_SHARED_MEMORY_NAME = "MySharedMemoryDefault"
SHARED_MEMORY_KEY_FILE = "shared_memory.key"

# See https://github.com/karkason/cppystruct and https://docs.python.org/2/library/struct.html
SHARED_STRUCT = 1  # cppystruct/struct Python/C++ communication only enabled if 1 (0: disabled)
STRUCT_FORMAT = "<I?30s"  # format of struct data, see https://docs.python.org/2/library/struct.html#format-characters

from PyQt5.QtCore import QBuffer, QDataStream, QSharedMemory
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtWidgets import QApplication, QDialog, QFileDialog
from dialog import Ui_Dialog
import logzero
import sys
import os.path
if SHARED_STRUCT == 1:
    import struct


class ProducerDialog(QDialog):
    def __init__(self, parent=None):
        super(ProducerDialog, self).__init__(parent)

        self.ui = Ui_Dialog()
        self.ui.setupUi(self)

        # We only allow one image to be put into the shared memory (otherwise we would need to
        # create a dedicated data structure within the SHARED memory and access it from Python and
        # C++ appropriately). Also note that "Open" and "Create" have very different semantics (see
        # docs) and we use "Create" to always create a "fresh" semaphore to not cause undesired
        # blocking/stalls, see also https://doc.qt.io/qt-5/qsystemsemaphore.html

        if CONSUMER == 0:
            self.ui.loadFromFileButton.clicked.connect(self.load_from_file)
            self.ui.loadFromSharedMemoryButton.setEnabled(False)
            self.setWindowTitle("Shared Memory Producer: Python Example")
            from prodcon_ipc.producer_ipc import ProducerIPC
            self.producer_ipc = ProducerIPC(UNIQUE_SHARED_MEMORY_NAME, SHARED_MEMORY_KEY_FILE)
        else:
            self.ui.loadFromSharedMemoryButton.clicked.connect(self.load_from_memory)
            self.ui.loadFromFileButton.setEnabled(False)
            self.setWindowTitle("Shared Memory Consumer: Python Example")
            from prodcon_ipc.consumer_ipc import ConsumerIPC
            self.consumer_ipc = ConsumerIPC(UNIQUE_SHARED_MEMORY_NAME, SHARED_MEMORY_KEY_FILE)

        if SHARED_STRUCT == 1:
            self.shmem_config = QSharedMemory('shared_struct_test')
            if self.shmem_config.isAttached() or self.shmem_config.attach():
                if self.shmem_config.lock():
                    counter, stop_flag, file_name = struct.unpack(STRUCT_FORMAT, self.shmem_config.constData())
                    logzero.logger.debug("Shared memory struct read: counter=" + str(counter) + ", stop_flag=" + str(stop_flag) + ", file_name=" + file_name)
                    self.shmem_config.unlock()
                else:
                    logzero.logger.error("unable to lock " + self.shmem_config.key())
                # Note: if both processes detach from the memory, it gets deleted so that attach() fails. That's why we
                #       simply never detach (HERE). Depending on the app design, there may be a better solution.
                #self.shmem_config.detach()
            else:
                logzero.logger.error("unable to attach " + self.shmem_config.key())

    def load_from_file(self):  # producer slot
        self.ui.label.setText("Select an image file")
        file_name, t = QFileDialog.getOpenFileName(self, None, None, "Images (*.png *.jpg)")
        if not file_name:
            return

        image = QImage()
        if not image.load(file_name):
            self.ui.label.setText("Selected file is not an image, please select another.")
            return

        self.ui.label.setPixmap(QPixmap.fromImage(image))

        # Get the image data:
        buf = QBuffer()
        buf.open(QBuffer.ReadWrite)
        out = QDataStream(buf)
        out << image

        try:
            from prodcon_ipc.producer_ipc import ScopedProducer
            with ScopedProducer(self.producer_ipc, buf.size()) as sp:
                # Copy image data from buf into shared memory area:
                sp.data()[:sp.size()] = buf.data().data()[:sp.size()]
        except Exception as err:
            self.ui.label.setText(str(err))

        if SHARED_STRUCT == 1:
            # Read from shared memory, increase value and write it back:
            if self.shmem_config.isAttached() or self.shmem_config.attach():
                if self.shmem_config.lock():
                    counter, stop_flag, _ = struct.unpack(STRUCT_FORMAT, self.shmem_config.constData())
                    data = struct.pack(STRUCT_FORMAT, counter + 1, stop_flag, str(os.path.basename(file_name)[:30]))
                    size = min(struct.calcsize(STRUCT_FORMAT), self.shmem_config.size())

                    self.shmem_config.data()[:size] = data[:size]
                    self.shmem_config.unlock()
                    if stop_flag:  # stop producing?
                        logzero.logger.info("Consumer requested to stop the production.")
                        sys.exit(0)
                else:
                    logzero.logger.error("unable to lock " + self.shmem_config.key())
                #self.shmem_config.detach()
            else:
                logzero.logger.error("unable to attach " + self.shmem_config.key())

    def load_from_memory(self):  # consumer slot
        buf = QBuffer()
        ins = QDataStream(buf)
        image = QImage()

        if True:  # first variant (much simpler / shorter, more robust)
            try:
                from prodcon_ipc.consumer_ipc import ScopedConsumer
                with ScopedConsumer(self.consumer_ipc) as sc:
                    buf.setData(sc.data())
                    buf.open(QBuffer.ReadOnly)
                    ins >> image
            except Exception as err:
                self.ui.label.setText(str(err))

        else:  # second variant, using begin()...end() manually
            try:
                data = self.consumer_ipc.begin()
            except RuntimeError as err:
                self.ui.label.setText(str(err))
                return

            # Read from the shared memory:
            try:
                buf.setData(data)
                buf.open(QBuffer.ReadOnly)
                ins >> image
            except Exception as err:
                logzero.logger.error(str(err))

            try:
                self.consumer_ipc.end()
            except RuntimeError as err:
                self.ui.label.setText(str(err))
                return

        if not image.isNull():
            self.ui.label.setPixmap(QPixmap.fromImage(image))
        else:
            logzero.logger.error("Image data was corrupted.")


def producer_repetitive_scope_test(path, repetitions=1, delay=0):  # producer with repetitive writes to shared memory
    import time
    from PyQt5.QtGui import QPainter, QPixmap, QColor, QFont, QPen
    from PyQt5.QtCore import Qt
    from prodcon_ipc.producer_ipc import ProducerIPC

    # FIXME: this is still buggy because when the images are not read sufficiently fast from the consumer (which
    #       only (?) happens in the non-async Python consumer), image data gets corrupted (may be overwritten by the
    #       producer?! but this SHOULD be prevented by the system semaphores and the producer/consumer sync...)

    producer_ipc = ProducerIPC(UNIQUE_SHARED_MEMORY_NAME, SHARED_MEMORY_KEY_FILE)
    for i in range(repetitions):
        image = QImage()
        if not image.load(path):
            logzero.logger.error("Unable to load image " + path)
            sys.exit(1)
        else:
            pm = QPixmap.fromImage(image)  # .convertToFormat(QImage.Format_RGB32)

            p = QPainter()
            p.begin(pm)
            p.setPen(QPen(Qt.yellow))
            p.setFont(QFont("Times", 20, QFont.Bold))
            p.drawText(pm.rect(), Qt.AlignCenter, str(i+1) + " of " + str(repetitions))
            p.end()

            image = pm.toImage()

            # Load the image:
            buf = QBuffer()
            buf.open(QBuffer.ReadWrite)
            out = QDataStream(buf)
            out << image

            try:
                avail_size, mem_data = producer_ipc.begin(buf.size())
                if avail_size < buf.size():
                    logzero.logger.warn("Couldn't get enough memory!")
            except RuntimeError as err:
                logzero.logger.error(str(err))
                sys.exit(2)

            # Copy image data from buf into shared memory area:
            error_str = None
            try:
                mem_data[:avail_size] = buf.data().data()[:avail_size]
            except Exception as err:
                logzero.logger.error(str(err))

            try:
                producer_ipc.end()
            except RuntimeError as err:
                logzero.logger.error(str(err))
                sys.exit(3)
            if delay > 0:
                time.sleep(delay)
        logzero.logger.debug("Iteration " + str(i + 1) + " of " + str(repetitions) + " completed.")

    logzero.logger.info("All okay")
    # IMPORTANT: The object should NOT go out of scope until the consumer has read the data, so (also) dont call:
    # del producer_ipc
    # ...or let the app exit right away. That's why we do here:
    try:
        input("Press Enter to exit but let the consumer read the image first...")
    except:
        sys.stdout.write("\n")
        pass
    sys.exit(0)


if __name__ == '__main__':
    app = QApplication(sys.argv)

    if 'consumer' in sys.argv:
        CONSUMER = 1
    elif 'producer' in sys.argv:
        CONSUMER = 0
    elif len(sys.argv) > 1 and os.path.exists(sys.argv[1]) and (sys.argv[1].endswith('.png') or
                                                                sys.argv[1].endswith('.jpg')):
        producer_repetitive_scope_test(sys.argv[1], 5, 0)

    logzero.logger.debug("I am the " + ('consumer.' if CONSUMER == 1 else 'producer.'))

    dialog = ProducerDialog()
    dialog.show()
    result = 0
    # noinspection PyBroadException
    try:
        result = app.exec_()
    except Exception as e:
        pass
    sys.exit(result)
