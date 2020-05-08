// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#include "consumer_ipc.h"

#include <QtConcurrent>
#include <QDebug>

ConsumerIPC::ConsumerIPC(const QString &id, const QString &key_file_path, bool log_debug)
  : AbstractIPC(id, key_file_path, log_debug), data_acquired(false)
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
    // Passively wait until a data is ready:
    if (log) {
      qDebug() << "Update thread: Waiting for data...";
    }
    if (!sem_full.acquire()) {
      if (log) {
        qDebug() << "Unable to acquire system semaphore (sem_full): " << sem_full.errorString();
      }
    }
    data_acquired = true;

    if (!terminate) {
      if (log) {
        qDebug() << "Producer signaled that data is ready! Triggering UI thread...";
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
  // Check to ensure that its allowed to call this since the thread was really signaled...this way,
  // it could happen that sem_empty.release() is called in end() although sem_full.acquire() in
  // updateThread() was never triggered:
  if (!data_acquired) {
    if (log) {
      qDebug() << "Data was not acquired yet. Wait until the signal ConsumerIPC::available() is "
                  "emitted and call this method in a slot upon being notified.";
    }
    return -1;
  }
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

  // Now, consume it:
  if (!shared_memory.attach()) {
    if (log) {
      qDebug() << "Unable to attach to shared memory segment: " << shared_memory.errorString();
    }
    sem_empty.release(); // undo (data may be lost)
    return -1;
  }

  if (!shared_memory.lock()) {
    if (log) {
      qDebug() << "Unable to lock shared memory segment: " << shared_memory.errorString();
    }
    shared_memory.detach(); // dito
    sem_empty.release();
    return -1;
  }
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
  data_acquired = false;
  shared_memory.unlock();
  shared_memory.detach();

  if (log) {
    qDebug() << "Signaling that the produced data was consumed...";
  }
  if (!sem_empty.release()) {
    if (log) {
      qDebug() << "Unable to release system semaphore (sem_empty): " << sem_empty.errorString();
    }
  }
  transaction_started = false;
}
