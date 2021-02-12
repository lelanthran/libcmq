
/* ****************************************************************************
 *                   Copyright © Lelanthran Manickum, 2021                    *
 *                     Provided under Gnu LGPL 2.1 only.                      *
 * ****************************************************************************/

#ifndef H_CMQ
#define H_CMQ

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* *****************************************************************************
 * Implementation of a thread-safe FIFO message-queue using POSIX threads.
 *
 * Messages are added to the queue with cmq_post() and removed with cmq_wait(). Messages
 * are added to the head of the queue and removed from the tail of the queue; this means
 * that the oldest messages are removed first.
 *
 * The caller will always get the messages in a First-In-First-Out order (FIFO).
 */
typedef struct cmq_t cmq_t;

#define CMQ_LOG(...)        do {\
   printf ("[%s:%i] ", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

   //    Create a new queue. Queue must be destroyed with cmq_del(). Returns NULL on
   // error.
   cmq_t *cmq_new (void);
   //    Destroys a queue, discarding all the elements contained within it.
   void cmq_del (cmq_t *cmq);

   //    Returns the number of elements in the queue, or <0 on error.
   int cmq_count (cmq_t *cmq);

   // Insert the element into the queue, returns true on success and false on error.
   bool cmq_post (cmq_t *cmq, void *payload, size_t payload_len);

   //    Removes an element from the queue and places the element's pointer and length
   // into the buffers provided. If the queue is empty, this function will wait a maximum
   // of timeout seconds for the queue to be populated and will return the oldest element
   // in the queue.
   //
   //    If payload is NULL it is ignored.
   //    If payload_len is NULL it is ignored.
   //    If timeout // is 0 then this function will not wait for arrivale of an element in
   // the queue and // will instead return immediately, returning true for a successful
   // element removal and // false if no element was removed. If timeout is non-zero,
   // then this function will // wait *at least* the specified number of seconds for a
   // message to arrive.
   //    If lifetime is NULL it is ignored.
   //
   //    Returns true if a message was removed and places the message into payload and
   // payload_len and the lifetime of the message into lifetime.
   //    Returns false if no message was removed, in which case payload, payload_len and
   // lifetime remain unchanged.
   bool cmq_wait (cmq_t *cmq, void **payload, size_t *payload_len, size_t timeout_ms,
                  struct timespec *lifetime);

   //    Returns a message without removing it from the queue. Returns true if a message
   // was found, in which case payload and payload_len contains the message.
   //    Returns false if no message was found, in which case payload and payload_len
   // remain unchanged.
   bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len);

#ifdef __cplusplus
};
#endif


#endif
