// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#ifndef ABSTRACT_IPC_H
#define ABSTRACT_IPC_H

#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QString>

/**
 * \interface AbstractIPC
 * \brief Abstract base class for the \c ConsumerIPC and \c ProducerIPC classes
 *
 * This class contains all functionality that \c ConsumerIPC and \c ProducerIPC have in common.
 */
class AbstractIPC : public QObject {
Q_OBJECT

  virtual void end() = 0;

protected:
  /**
   * Creates the underlying system ressources.
   * \param [in] id Unique system-wide unique name of shared memory; this name is also used to
   *        create the unique names for the two system-semapores `$id + "_sem_full"` and
   *        `$id + "_sem_empty"`
   * \param [in] key_file_path Optional file path to a file typically named "shared_memory.key"
   *             whose first line is used as the unique ID/name of the shared memory IF this file
   *             exists, can be empty (the default) which then uses \c id (first parameter)
   * \param [in] log_debug \c true to enable logging to \c qDebug() (default), \c false otherwise
   */
  AbstractIPC(const QString &id, const QString &key_file_path = QString(), bool log_debug = true);
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
  bool transaction_started = false; //!< \c true if \c begin() has been called, \c false otherwise
};

#endif // ABSTRACT_IPC_H
