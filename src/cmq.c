
/* ****************************************************************************
 *                   Copyright © Lelanthran Manickum, 2021                    *
 *                     Provided under Gnu LGPL 2.1 only.                      *
 * ****************************************************************************/

#include <stdlib.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "cmq.h"

struct cmq_node_t {

   struct cmq_node_t *next;
   struct cmq_node_t *prev;

   void *payload;             // Payload is not a copy. Do not delete it.
   size_t payload_len;

   struct timespec start_time;     // Used to determine how long element was alive
};

static struct cmq_node_t *cmq_node_new (void *payload, size_t payload_len)
{
   struct cmq_node_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   ret->payload = payload;
   ret->payload_len = payload_len;

   // We don't care about errors here - it may throw off the accounting but the
   // queue will still work. Callers getting suspicious lifetime values will have
   // to look into why the clock isn't working.
   clock_gettime (CLOCK_BOOTTIME, &ret->start_time);

   return ret;
}

static void cmq_node_del (struct cmq_node_t *node, struct timespec *lifetime)
{
   if (!node)
      return;

   if (lifetime) {
      struct timespec now = { 0, 0};
      clock_gettime (CLOCK_BOOTTIME, &now);

      int64_t diff_ns = now.tv_nsec - node->start_time.tv_nsec;
      if (diff_ns < 0) {
         now.tv_sec--;
         now.tv_nsec += 1000000000;
         diff_ns = now.tv_nsec - node->start_time.tv_nsec;
      }

      int64_t diff_s = now.tv_sec - node->start_time.tv_sec;
      if (diff_s >= 0) {
         lifetime->tv_sec = diff_s;
         lifetime->tv_nsec = diff_ns;
      } else {
         memset (lifetime, 0, sizeof *lifetime);
      }
   }

   free (node);
}

struct cmq_t {

   struct cmq_node_t *head;
   struct cmq_node_t *tail;

   pthread_mutex_t lock_ptrs;

   sem_t sem;
};

cmq_t *cmq_new (void)
{
   cmq_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   if ((sem_init (&ret->sem, 0, 0))!=0) {
      cmq_del (ret);
      ret = NULL;
   }

   pthread_mutexattr_t attr;

   pthread_mutexattr_init (&attr);
   pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);

   pthread_mutex_init (&ret->lock_ptrs,   &attr);

   pthread_mutexattr_destroy (&attr);

   return ret;
}

void cmq_del (cmq_t *cmq)
{
   if (!cmq)
      return;

   pthread_mutex_lock (&cmq->lock_ptrs);

   sem_destroy (&cmq->sem);

   while (cmq->head) {
      struct cmq_node_t *tmp = cmq->head->next;
      cmq_node_del (cmq->head, NULL);
      cmq->head = tmp;
   }

   pthread_mutex_unlock (&cmq->lock_ptrs);

   pthread_mutex_destroy (&cmq->lock_ptrs);


   free (cmq);
}

bool cmq_resize (cmq_t *cmq, size_t nelems)
{
   // TODO: Unimplemented.
   return false;
}

int cmq_count (cmq_t *cmq)
{
   if (!cmq)
      return 0;

   int semvalue;
   if ((sem_getvalue (&cmq->sem, &semvalue))!=0) {
      return -1;
   }

   return semvalue;
}

bool cmq_post (cmq_t *cmq, void *payload, size_t payload_len)
{
   struct cmq_node_t *newnode = NULL;
   if (!cmq)
      return false;

   if (!(newnode = cmq_node_new (payload, payload_len)))
      return false;

   newnode->prev = NULL;

   pthread_mutex_lock (&cmq->lock_ptrs);

      newnode->next = cmq->head;
      if (cmq->head)
         cmq->head->prev = newnode;

      cmq->head = newnode;

      if (!cmq->tail)
         cmq->tail = newnode;

   pthread_mutex_unlock (&cmq->lock_ptrs);

   sem_post (&cmq->sem);

   return true;
}

bool cmq_wait (cmq_t *cmq, void **payload, size_t *payload_len, size_t timeout_ms,
               struct timespec *lifetime)
{
   struct timespec ts = { 0, 0 };

   if (!cmq)
      return false;

   if (timeout_ms==0) {
      if ((sem_trywait (&cmq->sem))!=0) {
         return false;
      }
   } else {
      if ((clock_gettime (CLOCK_REALTIME, &ts))!=0) {
         return false;
      }
      ts.tv_nsec += timeout_ms / 1000000;
      if ((sem_timedwait (&cmq->sem, &ts))!=0) {
         return false;
      }
   }

   pthread_mutex_lock (&cmq->lock_ptrs);
      if (cmq->head == cmq->tail) {

         if (payload)
            *payload = cmq->tail->payload;

         if (payload_len)
            *payload_len = cmq->tail->payload_len;

         cmq_node_del (cmq->tail, lifetime);

         cmq->head = cmq->tail = NULL;

         pthread_mutex_unlock (&cmq->lock_ptrs);

         return true;
      }

      if (payload)
         *payload = cmq->tail->payload;

      if (payload_len)
         *payload_len = cmq->tail->payload_len;

      struct cmq_node_t *tmp = cmq->tail;
      cmq->tail = cmq->tail->prev;
      cmq->tail->next = NULL;

   pthread_mutex_unlock (&cmq->lock_ptrs);

   cmq_node_del (tmp, lifetime);

   return true;
}

bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq)
      return false;

   pthread_mutex_lock (&cmq->lock_ptrs);
      if (!cmq->tail) {
         pthread_mutex_unlock (&cmq->lock_ptrs);
         return false;
      }

      if (payload)
         *payload = cmq->tail->payload;

      if (payload_len)
         *payload_len = cmq->tail->payload_len;

   pthread_mutex_unlock (&cmq->lock_ptrs);

   return true;
}

