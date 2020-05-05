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
#           changing CONSUMER of both apps, the behavior (consumer/producer) can be inverted.

CONSUMER = 0

from PyQt5.QtCore import QBuffer, QDataStream
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtWidgets import QApplication, QDialog, QFileDialog
from dialog import Ui_Dialog


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
            self.producer_ipc = ProducerIPC()
        else:
            self.ui.loadFromSharedMemoryButton.clicked.connect(self.load_from_memory)
            self.ui.loadFromFileButton.setEnabled(False)
            self.setWindowTitle("Shared Memory Consumer: Python Example")
            from prodcon_ipc.consumer_ipc import ConsumerIPC
            self.consumer_ipc = ConsumerIPC()

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

        # Load the image:
        buf = QBuffer()
        buf.open(QBuffer.ReadWrite)
        out = QDataStream(buf)
        out << image

        avail_size, mem_data = self.producer_ipc.begin(buf.size())

        # Copy image data from buf into shared memory area:
        error_str = None
        try:
            mem_data[:avail_size] = buf.data().data()[:avail_size]
        except Exception as err:
            error_str = str(err)

        self.producer_ipc.end()

        if error_str:
            self.ui.label.setText(error_str)

    def load_from_memory(self):  # consumer slot
        buf = QBuffer()
        ins = QDataStream(buf)
        image = QImage()

        data = self.consumer_ipc.begin()

        # Read from the shared memory:
        buf.setData(data)
        buf.open(QBuffer.ReadOnly)
        ins >> image

        self.consumer_ipc.end()

        self.ui.label.setPixmap(QPixmap.fromImage(image))


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
    sys.exit(result)
