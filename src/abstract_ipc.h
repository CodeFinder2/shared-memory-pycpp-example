// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#ifndef ABSTRACT_IPC_H
#define ABSTRACT_IPC_H

#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QString>

// (System-wide) Unique name of the shared memory; must be equal in all apps using the memory:
#define UNIQUE_SHARED_MEMORY_NAME  "MySharedMemoryDefault"
// Hardcoded file name of file to be used for specifying an alternative name of the shared memory.
// If that file exists, its first line is ALWAYS used as the shared memory's name:
#define SHARED_MEMORY_KEY_FILE     "shared_memory.key"
// (System-wide) Unique names of (system) semaphores to be used for synchronizing producer/consumer:
#define UNIQUE_SEMAPHORE_EMPTY     "MySemaphoreEmpty"
#define UNIQUE_SEMAPHORE_FULL      "MySemaphoreFull"

class AbstractIPC : public QObject {
Q_OBJECT

  virtual void end() = 0;

protected:
  /**
   * Creates the underlying system ressources.
   * \param [in] log_debug \c true to enable logging to \c qDebug(), \c false otherwise
   */
  AbstractIPC(bool log_debug = true);
  /// Does nothing yet (but required due to a pure virtual function above).
  virtual ~AbstractIPC();
  /**
   * Alternatively loads the shared memory's name from a file named \c SHARED_MEMORY_KEY_FILE.
   * \param path Path to file whose first name contains the unique name; the rest is ignored
   * \return Unique name of shared memory or an empty string if such a file did not exist
   */
  QString loadKey(const QString path) const;

  /// Detaches from the shared memory.
  void detach();

  bool log; //!< \c true to show log output using \c qDebug() (useful for debugging)
  QSharedMemory shared_memory; //!< Instance of the shared memory reference
  QSystemSemaphore sem_empty; //!< System-wide semaphore to indicate the #(free slots)
  QSystemSemaphore sem_full; //!< System-wide semaphore to indicate the #(full slots)
  QString file_key; //!< Unique name of shared memory loaded from file (may be empty)
  bool transaction_started = false;
};

#endif // ABSTRACT_IPC_H
