// Copyright (C) 2020 Adrian Böckenkamp
// This code is licensed under the BSD 3-Clause license (see LICENSE for details).

#ifndef PRODUCER_IPC_H
#define PRODUCER_IPC_H

#include "abstract_ipc.h"

/**
 * \class ProducerIPC
 * \brief Provides simplified access to shared memory as a "producer"
 *
 * Given a system-wide producer-consumer problem, this class encapsulates the
 * Inter-Process-Communication (IPC) for the producer. It allows simplified access to the underlying
 * shared memory.
 *
 * Basically, to put data into the shared memory, call \c begin() with the amount of memory your
 * data needs. A pointer to the allocated shared memory is then provided in \c data. Afterwards,
 * call end to complete the transaction. Alternatively, use \c ScopedProducer for a even simpler
 * use.
 *
 * \todo add a signal `void consumed()` to allow using this class asynchronously but keep this sync
 *       variant since in most cases, "production" requires more time is done in a dedicated thread
 *       anyway so blocking that thread isn't a problem (and often desired)
 */
class ProducerIPC : public AbstractIPC {
  Q_OBJECT

public:
  /**
   * Creates the shared memory reference and the internal semaphores.
   * \param [in] log_debug \c true to enable logging to \c qDebug(), \c false to disable logging
   */
  ProducerIPC(bool log_debug = true) : AbstractIPC(log_debug) { }
  /// Detaches from the shared memory.
  ~ProducerIPC();
  /**
   * Begins "producing" data for the memory, i.e., starts a transaction on the shared memory block.
   * After writing to the memory, call \c end() but only do this if \c begin() succeeded previously.
   * \param [in] desired_size Number of bytes you wish to store in the shared memory
   * \param [out] data Writable pointer to shared memory block
   * \return number of bytes that \c data addresses, may be <= 0 in case of errors
   * \note The returned size may be smaller than \c desired_size so ensure to not write more bytes
   *       than what is returned by this method.
   * \see end()
   */
  int begin(int desired_size, char **data);
  /**
   * Ends a transaction on the shared memory. You must call this method when finished dealing with
   * the memory referenced by \c data, returned by \c begin().
   * \see begin()
   */
  void end();
};

/**
 * \class ScopedProducer
 * \brief Simplifies using \c ProducerIPC by RAII style
 *
 * You can use this class to pass a \c ProducerIPC object to the constructor to let it automatically
 * call \c begin() upon construction and \c end() upon deletion. This ensures to never forget to
 * call \c end(), e.g., in case of errors. This is similar to a \c std::scope_lock.
 *
 * You should \b not create objects of this class on the heap (new).
 */
class ScopedProducer {
public:
  /**
   * Calls \c begin() of the provided object.
   * \param obj Object whose shared memory should be filled
   * \see ProducerIPC::begin()
   * \see ~ScopedProducer()
   */
  ScopedProducer(ProducerIPC &obj, int desired_size) : that(obj)
  {
    that_size = that.begin(desired_size, &that_data);
    if (that_size <= 0) {
      throw std::runtime_error("Unable to begin transaction.");
    }
  }
  /**
   * Returns the number of bytes available in the shared memory.
   * \return data size in bytes, may be <= 0 in case of errors
   */
  int size() const { return that_size; }
  /**
   * \brief Returns a writable pointer to the data of the shared memory.
   * \return pointer to data of shared memory
   */
  char *data() const { return that_data; }
  /**
   * Ends the transaction of "producing" data to be put into the shared memory.
   * \see ProducerIPC::end()
   */
  ~ScopedProducer()
  {
    if (that_size > 0) {
      that.end();
    }
  }
private:
  int that_size; //!< Number of bytes available in the shared memory as returned by \c begin()
  char *that_data; //!< Pointer to data in shared memory as returned by \c begin()
  ProducerIPC &that; //!< Instance passed to constructor (kept for dtor call)
};

#endif // PRODUCER_IPC_H
