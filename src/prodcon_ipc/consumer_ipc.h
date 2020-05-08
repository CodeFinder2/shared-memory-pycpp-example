// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#ifndef CONSUMER_IPC_H
#define CONSUMER_IPC_H

#include <atomic>
#include <QFuture>

#include "abstract_ipc.h"
/**
 * \class ConsumerIPC
 * \brief Provides simplified access to shared memory as a "consumer"
 *
 * Given a system-wide producer-consumer problem, this class encapsulates the
 * Inter-Process-Communication (IPC) for the consumer. It allows simplified access to the underlying
 * shared memory.
 *
 * Basically, the signal \c available() is emitted once data was produced and put into the shared
 * memory. Once triggered, use \c begin() ... \c end() to access the data.
 */
class ConsumerIPC : public AbstractIPC {
  Q_OBJECT

public:
  /// \copydoc AbstractIPC::AbstractIPC()
  ConsumerIPC(const QString &id, const QString &key_file_path = QString(), bool log_debug = true);
  /// Terminates the updater thread.
  ~ConsumerIPC();

  /**
   * Begins "consuming" the memory, i.e., starts a transaction on the shared memory block. Don't
   * call \c end() if this returns with errors!
   * \param [out] data Pointer to shared memory block
   * \return number of bytes that \c data addresses, may be <= 0 in case of errors
   * \see end()
   */
  int begin(const char **data);
  /**
   * Ends a transaction on the shared memory. You must call this method when finished dealing with
   * the memory referenced by \c data, returned by \c begin().
   * \see begin()
   */
  void end();

signals:
  /// Emitted when new data was put (produced) in the shared memory. Use \c begin() ... \c end() to
  /// retrieve the data.
  void available();

private:

  /// Thread function to wait for an updated shared memory block
  void updateThread();
  std::atomic_bool terminate; //!< \c to indicate \c updateThread() to terminate
  QFuture<void> future; //!< Instance used to spawn the update thread
  std::atomic_bool data_acquired; //!< Indicates if data was received (set in \c updateThread())
};

/**
 * \class ScopedConsumer
 * \brief Simplifies using \c ConsumerIPC by RAII style
 *
 * You can use this class to pass a \c ConsumerIPC object to the constructor to let it automatically
 * call \c begin() upon construction and \c end() upon deletion. This ensures to never forget to
 * call \c end(), e.g., in case of errors. This is similar to a \c std::scope_lock.
 *
 * You should \b not create objects of this class on the heap (new).
 */
class ScopedConsumer {
public:
  /**
   * Calls \c begin() of the provided object.
   * \param obj Object whose shared memory should be consumed
   * \see ConsumerIPC::begin()
   * \see ~ScopedConsumer()
   */
  ScopedConsumer(ConsumerIPC &obj) : that(obj)
  {
    that_size = that.begin(&that_data);
  }
  /**
   * Returns the number of bytes stored in the shared memory.
   * \return data size in bytes, may be <= 0 in case of errors
   */
  int size() const { return that_size; }
  /**
   * \brief Returns a pointer to the data of the shared memory.
   * \return pointer to data of shared memory
   */
  const char *data() const { return that_data; }
  /**
   * Ends the transaction of "consuming" the data of the shared memory.
   * \see ConsumerIPC::end()
   */
  ~ScopedConsumer()
  {
    that.end();
  }
private:
  int that_size; //!< Number of bytes stored in the shared memory as returned by \c begin()
  const char *that_data; //!< Pointer to data in shared memory as returned by \c begin()
  ConsumerIPC &that; //!< Instance passed to constructor (kept for dtor call)
};

#endif // CONSUMER_IPC_H
