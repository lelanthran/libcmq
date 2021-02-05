#include <stdlib.h>

#include <pthread.h>

#include "cmq.h"

struct cmq_node_t {

   struct cmq_node_t *next;
   struct cmq_node_t *prev;

   void *payload;             // Payload is not a copy. Do not delete it.
   size_t payload_len;
};

static struct cmq_node_t *cmq_node_new (void *payload, size_t payload_len)
{
   struct cmq_node_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   ret->payload = payload;
   ret->payload_len = payload_len;

   return ret;
}

static void cmq_node_del (struct cmq_node_t *node)
{
   if (!node)
      return;

   free (node);
}

struct cmq_t {

   struct cmq_node_t *head;
   struct cmq_node_t *tail;

   size_t nelems;

   pthread_mutex_t lock_head;
   pthread_mutex_t lock_tail;
   pthread_mutex_t lock_nelems;
};

cmq_t *cmq_new (void)
{
   cmq_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   pthread_mutexattr_t attr;

   pthread_mutexattr_init (&attr);
   pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);

   pthread_mutex_init (&ret->lock_head,   &attr);
   pthread_mutex_init (&ret->lock_tail,   &attr);
   pthread_mutex_init (&ret->lock_nelems, &attr);

   pthread_mutexattr_destroy (&attr);

   return ret;
}

void cmq_del (cmq_t *cmq)
{
   if (!cmq)
      return;

   pthread_mutex_lock (&cmq->lock_head);
   pthread_mutex_lock (&cmq->lock_tail);
   pthread_mutex_lock (&cmq->lock_nelems);

   while (cmq->head) {
      struct cmq_node_t *tmp = cmq->head->next;
      cmq_node_del (cmq->head);
      cmq->head = tmp;
   }

   pthread_mutex_unlock (&cmq->lock_head);
   pthread_mutex_unlock (&cmq->lock_tail);
   pthread_mutex_unlock (&cmq->lock_nelems);

   pthread_mutex_destroy (&cmq->lock_head);
   pthread_mutex_destroy (&cmq->lock_tail);
   pthread_mutex_destroy (&cmq->lock_nelems);


   free (cmq);
}

size_t cmq_count (cmq_t *cmq)
{
   if (!cmq)
      return 0;

   pthread_mutex_lock (&cmq->lock_nelems);
   size_t ret = cmq->nelems;
   pthread_mutex_unlock (&cmq->lock_nelems);

   return ret;
}

bool cmq_insert (cmq_t *cmq, void *payload, size_t payload_len)
{
   struct cmq_node_t *newnode = NULL;
   if (!cmq)
      return false;

   if (!(newnode = cmq_node_new (payload, payload_len)))
      return false;

   newnode->prev = NULL;

   pthread_mutex_lock (&cmq->lock_head);

      newnode->next = cmq->head;
      if (cmq->head)
         cmq->head->prev = newnode;

      cmq->head = newnode;

   pthread_mutex_unlock (&cmq->lock_head);

   pthread_mutex_lock (&cmq->lock_tail);
   if (!cmq->tail)
      cmq->tail = newnode;
   pthread_mutex_unlock (&cmq->lock_tail);

   pthread_mutex_lock (&cmq->lock_nelems);
   cmq->nelems++;
   pthread_mutex_unlock (&cmq->lock_nelems);

   return true;
}

bool cmq_remove (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq || !cmq->nelems)
      return false;

   pthread_mutex_lock (&cmq->lock_tail);
      if (!cmq->tail) {
         pthread_mutex_unlock (&cmq->lock_tail);
         return false;
      }

      if (payload)
         *payload = cmq->tail->payload;

      if (payload_len)
         *payload_len = cmq->tail->payload_len;

      if (cmq->tail) {
         struct cmq_node_t *tmp = cmq->tail;
         cmq->tail = cmq->tail->prev;
         if (cmq->tail)
            cmq->tail->next = NULL;

         cmq_node_del (tmp);
      }

   pthread_mutex_unlock (&cmq->lock_tail);

   pthread_mutex_lock (&cmq->lock_nelems);
   cmq->nelems--;
   if (cmq->nelems == 0) {
      pthread_mutex_lock (&cmq->lock_head);
      pthread_mutex_lock (&cmq->lock_tail);

      cmq->head = cmq->tail = NULL;

      pthread_mutex_unlock (&cmq->lock_head);
      pthread_mutex_unlock (&cmq->lock_tail);

      pthread_mutex_lock (&cmq->lock_nelems);

      return true;
   }

   pthread_mutex_unlock (&cmq->lock_nelems);

   return true;
}

bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq)
      return false;

   pthread_mutex_lock (&cmq->lock_tail);
      if (!cmq->tail) {
         pthread_mutex_unlock (&cmq->lock_tail);
         return false;
      }

      if (payload)
         *payload = cmq->tail->payload;

      if (payload_len)
         *payload_len = cmq->tail->payload_len;

   pthread_mutex_unlock (&cmq->lock_tail);

   return true;
}

