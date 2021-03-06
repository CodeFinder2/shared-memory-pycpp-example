// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#ifndef ABSTRACT_IPC_CPP
#define ABSTRACT_IPC_CPP

#include "abstract_ipc.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

AbstractIPC::AbstractIPC(const QString &id, const QString &key_file_path, bool log_debug) : log(log_debug),
  shared_memory(id),
  sem_empty(id + "_sem_empty", 1, QSystemSemaphore::AccessMode::Create),
  sem_full(id + "_sem_full", 0, QSystemSemaphore::AccessMode::Create)
{
  // Set unique name (key) of shared memory handle, use file if exiting and otherwise hardcoded
  // name:
  file_key = loadKey(key_file_path);
  if (!file_key.isEmpty()) {
    shared_memory.setKey(file_key);
  }
}

AbstractIPC::~AbstractIPC() { }

QString AbstractIPC::loadKey(const QString path) const
{
  if (path.isEmpty()) {
    return QString();
  }
  QFile file(path);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream in_str(&file);
    if (!in_str.atEnd()) {
      QString line = in_str.readLine().trimmed();
      in_str.readLine();
      if (!in_str.atEnd() && log) {
        qDebug() << "Ignoring residual lines in " << path;
      }
      return line;
    }
    file.close();
  }
  return QString();
}

void AbstractIPC::detach()
{
  // See also: https://doc.qt.io/qt-5/qsharedmemory.html#details
  if (!shared_memory.detach() && log) {
    qDebug() << "Unable to detach from shared memory.";
  }
}

#endif // ABSTRACT_IPC_CPP
