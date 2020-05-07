// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#include "producer_ipc.h"

#include <QDebug>

ProducerIPC::~ProducerIPC() { detach(); }

int ProducerIPC::begin(int desired_size, char **data)
{
  if (transaction_started) {
    if (log) {
      qDebug() << "Already started an transaction, call end() first to start a new one.";
    }
    return -1;
  }
  if (!data) {
    if (log) {
      qDebug() << "Invalid data pointer provided";
    }
    return -1;
  }

  if (shared_memory.isAttached()) {
    detach();
  }
  if (!shared_memory.create(int(desired_size))) {
    if (log) {
      qDebug() << "Shared memory seems to be still existing, unable to create it. Trying to "
                  "recover by gaining ownership and detaching to delete it...";
    }
    shared_memory.attach();
    shared_memory.detach();
    if (!shared_memory.create(int(desired_size))) { // we really still failed
      if (log) {
        qDebug() << "Unable to create or recover shared memory segment: "
                 << shared_memory.errorString() << "\n\nYou probably need to reboot to fix this.";
      }
      return -1;
    }
  }

  if (!sem_empty.acquire()) {
    if (log) {
      qDebug() << "Unable to acquire system semaphore (sem_empty): " << sem_empty.errorString();
    }
    return -1;
  }
  if (!shared_memory.lock()) {
    sem_full.release(); // undo acquire
    if (log) {
      qDebug() << "Unable to lock shared memory: " << shared_memory.errorString();
    }
    return -1;
  }
  *data = static_cast<char*>(shared_memory.data());
  transaction_started = true;
  return qMin(shared_memory.size(), desired_size);
}

void ProducerIPC::end()
{
  if (!transaction_started) {
    if (log) {
      qDebug() << "You must call begin() first before calling end().";
    }
    return;
  }
  shared_memory.unlock();
  sem_full.release();
  transaction_started = false;
}
