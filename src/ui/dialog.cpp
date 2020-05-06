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
#include <QThread>

ConsumerDialog::ConsumerDialog(QWidget *parent) : QDialog(parent)
{
  ui.setupUi(this);
#if CONSUMER == 1
  qDebug() << "I am the consumer.";
  connect(ui.loadFromSharedMemoryButton, &QPushButton::clicked, [this](){
          this->ui.label->setText(tr("Please wait until an image was produced from the Python app "
                                     "(load an image therefrom); it will be shown here "
                                     "automatically."));
  });
  setWindowTitle(tr("Shared Memory Consumer: C++ Example"));
  // Since we pretend to be the consumer (as a demo, to make it clear), disable load from
  // file (= producing):
  ui.loadFromFileButton->setEnabled(false);

  connect(&this->cipc, &ConsumerIPC::available, this, &ConsumerDialog::loadFromMemory);
#else
  qDebug() << "I am the producer.";
  connect(ui.loadFromFileButton, &QPushButton::clicked, this, &ConsumerDialog::loadFromFile);
  setWindowTitle(tr("Shared Memory Producer: C++ Example"));
  // Since we pretend to be the producer (as a demo, to make it clear), disable load from
  // memory (= consuming):
  ui.loadFromSharedMemoryButton->setEnabled(false);
#endif
}

void ConsumerDialog::loadFromFile() // producer logic (not active on default, see ctor)
{
#if CONSUMER == 0
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

  // Load into shared memory
  QBuffer buffer;
  buffer.open(QBuffer::ReadWrite);
  QDataStream out(&buffer);
  out << image;

  try {
    ScopedProducer sp(pipc, int(buffer.size()));
    memcpy(sp.data(), buffer.data().data(), size_t(sp.size()));
  } catch (std::runtime_error &e) {
    ui.label->setText(e.what());
  }
#endif
}

void ConsumerDialog::loadFromMemory() // consumer logic, active on default
{
#if CONSUMER == 1
  QBuffer buffer;
  QDataStream in(&buffer);
  QImage image;

  {
    ScopedConsumer sc(cipc);
    if (sc.size() <= 0) {
      ui.label->setText(tr("Unable to attach to shared memory segment.\nLoad an image first."));
      return;
    }

    buffer.setData(sc.data(), sc.size());
    buffer.open(QBuffer::ReadOnly);
    in >> image;
  }

  if (image.isNull()) {
    ui.label->setText(tr("Image data is corrupted!"));
  } else {
    ui.label->setPixmap(QPixmap::fromImage(image));
  }
#endif
}
