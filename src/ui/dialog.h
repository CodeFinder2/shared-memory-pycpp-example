/****************************************************************************
**
** Copyright (C) 2020 Adrian Böckenkamp
** Copyright (C) 2016 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CONSUMER_DIALOG_H
#define CONSUMER_DIALOG_H

// Abstract: this C++ application contains the consumer logic, i.e., it waits until the Python app
//           has created (aka produced) an image and put that into a shared memory segment. This app
//           then displays (aka consumes) the image. By changing CONSUMER in the code base of both
//           apps, the behavior can be inverted. It surely also works when using only C++ or only
//           only Python apps (one consumer, one producer of course).

#define CONSUMER 1 // 1: this is the consumer, 0: this is the producer

// (System-wide) Unique name of the shared memory; must be equal in all apps using the memory:
#define UNIQUE_SHARED_MEMORY_NAME  "MySharedMemoryDefault"
// Hardcoded file name of file to be used for specifying an alternative name of the shared memory.
// If that file exists, its first line is ALWAYS used as the shared memory's name:
#define SHARED_MEMORY_KEY_FILE     "shared_memory.key"

#include <QDialog>
#include <QString>

#include "ui_dialog.h"
#if CONSUMER == 1
#include <prodcon_ipc/consumer_ipc.h>
#elif CONSUMER == 0
#include <prodcon_ipc/producer_ipc.h>
#else
  #error Invalid value for <CONSUMER> define, should be 0 (= producer) or 1 (= consumer).
#endif

class ConsumerDialog : public QDialog {
  Q_OBJECT

public:
  ConsumerDialog(QWidget *parent = nullptr);

public slots:
  void loadFromFile();
  void loadFromMemory();

private:
  Ui::Dialog ui;
#if CONSUMER == 1
  ConsumerIPC cipc;
#else
  ProducerIPC pipc;
#endif
};

#endif // CONSUMER_DIALOG_H
