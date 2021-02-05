#include <stdlib.h>
#include <threads.h>

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

   mtx_t lock_head;
   mtx_t lock_tail;
   mtx_t lock_nelems;
};

cmq_t *cmq_new (void)
{
   cmq_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   int lock_type = mtx_plain | mtx_recursive;

   int lock_head_rc    = mtx_init (&ret->lock_head, lock_type);
   int lock_tail_rc    = mtx_init (&ret->lock_tail, lock_type);
   int lock_nelems_rc  = mtx_init (&ret->lock_nelems, lock_type);

   if (lock_head_rc == thrd_error ||
       lock_tail_rc == thrd_error ||
       lock_nelems_rc == thrd_error) {
      cmq_del (ret);
      ret = NULL;
   }
   return ret;
}

void cmq_del (cmq_t *cmq)
{
   if (!cmq)
      return;

   mtx_lock (&cmq->lock_head);
   mtx_lock (&cmq->lock_tail);
   mtx_lock (&cmq->lock_nelems);

   while (cmq->head) {
      struct cmq_node_t *tmp = cmq->head->next;
      cmq_node_del (cmq->head);
      cmq->head = tmp;
   }

   mtx_unlock (&cmq->lock_head);
   mtx_unlock (&cmq->lock_tail);
   mtx_unlock (&cmq->lock_nelems);

   mtx_destroy (&cmq->lock_head);
   mtx_destroy (&cmq->lock_tail);
   mtx_destroy (&cmq->lock_nelems);


   free (cmq);
}

size_t cmq_count (cmq_t *cmq)
{
   if (!cmq)
      return 0;

   mtx_lock (&cmq->lock_nelems);
   size_t ret = cmq->nelems;
   mtx_unlock (&cmq->lock_nelems);

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

   mtx_lock (&cmq->lock_head);

      newnode->next = cmq->head;
      if (cmq->head)
         cmq->head->prev = newnode;

      cmq->head = newnode;

   mtx_unlock (&cmq->lock_head);

   mtx_lock (&cmq->lock_tail);
   if (!cmq->tail)
      cmq->tail = newnode;
   mtx_unlock (&cmq->lock_tail);

   mtx_lock (&cmq->lock_nelems);
   cmq->nelems++;
   mtx_unlock (&cmq->lock_nelems);

   return true;
}

bool cmq_remove (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq || !cmq->nelems)
      return false;

   mtx_lock (&cmq->lock_tail);
      if (!cmq->tail) {
         mtx_unlock (&cmq->lock_tail);
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

   mtx_unlock (&cmq->lock_tail);

   mtx_lock (&cmq->lock_nelems);
   cmq->nelems--;
   if (cmq->nelems == 0) {
      mtx_lock (&cmq->lock_head);
      mtx_lock (&cmq->lock_tail);

      cmq->head = cmq->tail = NULL;

      mtx_unlock (&cmq->lock_head);
      mtx_unlock (&cmq->lock_tail);
      return true;
   }

   mtx_unlock (&cmq->lock_nelems);

   return true;
}

bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq)
      return false;

   mtx_lock (&cmq->lock_tail);
      if (!cmq->tail) {
         mtx_unlock (&cmq->lock_tail);
         return false;
      }

      if (payload)
         *payload = cmq->tail->payload;

      if (payload_len)
         *payload_len = cmq->tail->payload_len;

   mtx_unlock (&cmq->lock_tail);

   return true;
}

