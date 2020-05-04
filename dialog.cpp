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

#include "dialog.h"

#include <QFileDialog>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#ifdef ENABLE_THREADED_WAITING
  #include <QThread>
  #include <QtConcurrent>
#endif

QString ConsumerDialog::loadKey(const QString path)
{
  QFile file(path);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream in_str(&file);
    if (!in_str.atEnd()) {
      QString line = in_str.readLine().trimmed();
      in_str.readLine();
      if (!in_str.atEnd()) {
        qDebug() << "Ignoring residual lines in " << path;
      }
      return line;
    }
    file.close();
  }
  return QString();
}

ConsumerDialog::ConsumerDialog(QWidget *parent) : QDialog(parent),
    sharedMemory(UNIQUE_SHARED_MEMORY_NAME),
    sem_empty(UNIQUE_SEMAPHORE_EMPTY, 1, QSystemSemaphore::AccessMode::Create),
    sem_full(UNIQUE_SEMAPHORE_FULL, 0, QSystemSemaphore::AccessMode::Create)
{
  qDebug() << (QString("Compiled with Qt v") + QT_VERSION_STR).toStdString().c_str();

  ui.setupUi(this);
  connect(ui.loadFromFileButton, &QPushButton::clicked, this, &ConsumerDialog::loadFromFile);
#ifndef ENABLE_THREADED_WAITING
  connect(ui.loadFromSharedMemoryButton, &QPushButton::clicked, this, &Dialog::loadFromMemory);
#else
  connect(ui.loadFromSharedMemoryButton, &QPushButton::clicked, [this](){
          this->ui.label->setText(tr("Please wait until an image was produced from the Python app "
                                     "(load an image therefrom); it will be shown here "
                                     "automatically."));
  });
#endif
  setWindowTitle(tr("Shared Memory: C++ Example"));
  // Since we pretend to be the consumer (as a demo, to make it clear), disable load from
  // file (= producing):
  ui.loadFromFileButton->setEnabled(false);

  // Set unique name (key) of shared memory handle, use file if exiting and otherwise hardcoded
  // name:
  QString key = loadKey(SHARED_MEMORY_KEY_FILE);
  if (!key.isEmpty()) {
    sharedMemory.setKey(key);
  }
  qDebug() << "Creating shared memory with key=" << sharedMemory.key()
           << (key.isEmpty() ? " (hardcoded)" : " (loaded from file)");

#ifdef ENABLE_THREADED_WAITING
  terminate = false;
  connect(this, &ConsumerDialog::available, this, &ConsumerDialog::loadFromMemory);
  future = QtConcurrent::run(this, &ConsumerDialog::updateThread);
#endif
}

ConsumerDialog::~ConsumerDialog()
{
#ifdef ENABLE_THREADED_WAITING
  // Stop waiting in the thread and wait for its termination:
  qDebug() << "Requesting update thread to terminate...";
  terminate = true;
  sem_full.release(); // fakes an image not unblock the thread
  future.waitForFinished();
  qDebug() << "Update thread has terminated successfully.";
#endif
}

#ifdef ENABLE_THREADED_WAITING
void ConsumerDialog::updateThread()
{
  while (!terminate) {
    // Passively wait until an image is ready to be displayed:
    qDebug() << "Update thread: Waiting for an(other) image...";
    sem_full.acquire();

    if (!terminate) {
      qDebug() << "Producer signaled that an image is ready! Trigger UI thread...";
      // Signal the GUI thread to display it:
      emit available();
    }
  }
  qDebug() << "Update thread about to terminate...";
}
#endif

void ConsumerDialog::loadFromFile() // producer logic (not active on default, see ctor)
{
  ui.label->setText(tr("Select an image file"));
  QString fileName = QFileDialog::getOpenFileName(nullptr, QString(), QString(),
                                                  tr("Images (*.png *.xpm *.jpg)"));
  if (fileName.isEmpty()) {
    return;
  }

  QImage image;
  if (!image.load(fileName)) {
    ui.label->setText(tr("Selected file is not an image, please select another."));
    return;
  }
  ui.label->setPixmap(QPixmap::fromImage(image));

  if (sharedMemory.isAttached()) {
    detach();
  }

  // load into shared memory
  QBuffer buffer;
  buffer.open(QBuffer::ReadWrite);
  QDataStream out(&buffer);
  out << image;
  qint64 size = buffer.size();

  if (!sharedMemory.create(int(size))) {
    ui.label->setText(tr("Unable to create shared memory segment."));
    return;
  }
  sharedMemory.lock();
  char *to = static_cast<char*>(sharedMemory.data());
  const char *from = buffer.data().data();
  memcpy(to, from, qMin(size_t(sharedMemory.size()), size_t(size)));
  sharedMemory.unlock();
}

void ConsumerDialog::loadFromMemory() // consumer logic, active on default
{
#ifndef ENABLE_THREADED_WAITING
  qDebug() << "Passively waiting (blocking) until the next image is in the shared memory...";
  // Wait until something was produced:
  sem_full.acquire(); // WARNING: this blocks (the GUI thread...) until something was produced!
#endif

  // Now, consume it (= display the image):
  if (!sharedMemory.attach()) {
    ui.label->setText(tr("Unable to attach to shared memory segment.\nLoad an image first."));
    return;
  }

  QBuffer buffer;
  QDataStream in(&buffer);
  QImage image;

  sharedMemory.lock();
  buffer.setData(static_cast<const char*>(sharedMemory.constData()), sharedMemory.size());
  buffer.open(QBuffer::ReadOnly);
  in >> image;
  sharedMemory.unlock();
  sharedMemory.detach();

  if (image.isNull()) {
    ui.label->setText(tr("Image data is corrupted!"));
  } else {
    ui.label->setPixmap(QPixmap::fromImage(image));
  }

  qDebug() << "Signaling that the produced image was consumed (displayed)...";
  sem_empty.release();
}

void ConsumerDialog::detach()
{
  // See also: https://doc.qt.io/qt-5/qsharedmemory.html#details
  if (!sharedMemory.detach()) {
    ui.label->setText(tr("Unable to detach from shared memory."));
  }
}
