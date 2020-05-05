// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#include "consumer_ipc.h"

#include <QtConcurrent>
#include <QDebug>

ConsumerIPC::ConsumerIPC(bool log_debug) : AbstractIPC(log_debug)
{
  if (log) {
    qDebug() << (QString("Compiled with Qt v") + QT_VERSION_STR).toStdString().c_str();
    qDebug() << "Creating shared memory with key=" << shared_memory.key()
             << (file_key.isEmpty() ? " (hardcoded)" : " (loaded from file)");
  }

  terminate = false;
  future = QtConcurrent::run(this, &ConsumerIPC::updateThread);
}

ConsumerIPC::~ConsumerIPC()
{
  // Stop waiting in the thread and wait for its termination:
  if (log) {
    qDebug() << "Requesting update thread to terminate...";
  }
  terminate = true;
  sem_full.release(); // fakes data to unblock the thread
  future.waitForFinished();
  if (log) {
    qDebug() << "Update thread has terminated successfully.";
  }
}

void ConsumerIPC::updateThread()
{
  while (!terminate) {
    // Passively wait until an image is ready to be displayed:
    if (log) {
      qDebug() << "Update thread: Waiting for an(other) image...";
    }
    sem_full.acquire();

    if (!terminate) {
      if (log) {
        qDebug() << "Producer signaled that an image is ready! Triggering UI thread...";
      }
      // Signal the GUI thread to display it:
      emit available();
    }
  }
  if (log) {
    qDebug() << "Update thread about to terminate...";
  }
}

int ConsumerIPC::begin(const char **data)
{
  if (transaction_started) {
    if (log) {
      qDebug() << "Already started an transaction, call end() first to start a new one.";
    }
    return -1;
  }
  if (!data) { // invalid pointer?
    if (log) {
      qDebug() << "Invalid pointer \'data\' provided.";
    }
    return -1;
  }

  // Now, consume it (= display the image):
  if (!shared_memory.attach()) {
    if (log) {
      qDebug() << "Unable to attach to shared memory segment.\nLoad an image first.";
    }
    return -1;
  }

  shared_memory.lock();
  *data = static_cast<const char*>(shared_memory.constData());
  transaction_started = true;
  return shared_memory.size();
}

void ConsumerIPC::end()
{
  if (!transaction_started) {
    if (log) {
      qDebug() << "You must call begin() first before calling end().";
    }
    return;
  }
  shared_memory.unlock();
  shared_memory.detach();

  if (log) {
    qDebug() << "Signaling that the produced image was consumed (displayed)...";
  }
  sem_empty.release();
  transaction_started = false;
}
