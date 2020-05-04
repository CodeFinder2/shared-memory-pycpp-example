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
//           then displays (aka consumes) the image. By slightly changing the code base of both app
//           the behavior can be inverted.

// (System-wide) Unique name of the shared memory; must be equal in all apps using the memory:
#define UNIQUE_SHARED_MEMORY_NAME  "MySharedMemoryDefault"
// Hardcoded file name of file to be used for specifying an alternative name of the shared memory.
// If that file exists, its first line is ALWAYS used as the shared memory's name:
#define SHARED_MEMORY_KEY_FILE     "shared_memory.key"
// (System-wide) Unique names of (system) semaphores to be used for synchronizing producer/consumer:
#define UNIQUE_SEMAPHORE_EMPTY     "MySemaphoreEmpty"
#define UNIQUE_SEMAPHORE_FULL      "MySemaphoreFull"

// If defined, the application uses a dedicated thread for (passively) wait until an image was
// produced. Uncomment to disable threading, making the app BLOCK after clicking "" until an image was produced.
#define ENABLE_THREADED_WAITING

#include <QDialog>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QString>
#include <atomic>

#include "ui_dialog.h"

#ifdef ENABLE_THREADED_WAITING
#include <QFuture>
#endif

class ConsumerDialog : public QDialog {
  Q_OBJECT

public:
  ConsumerDialog(QWidget *parent = nullptr);
  ~ConsumerDialog();

public slots:
  void loadFromFile();
  void loadFromMemory();

private:
  void detach();

#ifdef ENABLE_THREADED_WAITING
  void updateThread();
  std::atomic_bool terminate;
#endif

  static QString loadKey(const QString path);

  Ui::Dialog ui;
  QSharedMemory sharedMemory;
  QSystemSemaphore sem_empty;
  QSystemSemaphore sem_full;
#ifdef ENABLE_THREADED_WAITING
  QFuture<void> future;
#endif

signals:
#ifdef ENABLE_THREADED_WAITING
  void available();
#endif
};

#endif // CONSUMER_DIALOG_H
