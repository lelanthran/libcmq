#include <stdlib.h>

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

};

cmq_t *cmq_new (void)
{
   cmq_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;
   // TODO
   return ret;
}

void cmq_del (cmq_t *cmq)
{
   if (!cmq)
      return;
   // Traverse head = head->next until head=>next = NULL, free each node
   free (cmq);
}

bool cmq_insert (cmq_t *cmq, void *payload, size_t payload_len)
{
   struct cmq_node_t *newnode = NULL;
   if (!cmq)
      return false;

   if (!(newnode = cmq_node_new (payload, payload_len)))
      return false;

   newnode->prev = NULL;
   newnode->next = cmq->head;
   if (cmq->head)
      cmq->head->prev = newnode;

   cmq->head = newnode;
   if (!cmq->tail)
      cmq->tail = newnode;

   cmq->nelems++;

   return true;
}

bool cmq_remove (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!(cmq_peek (cmq, payload, payload_len)))
      return false;

   if (cmq->tail) {
      struct cmq_node_t *tmp = cmq->tail;
      cmq->tail = cmq->tail->prev;
      if (cmq->tail)
         cmq->tail->next = NULL;

      cmq_node_del (tmp);
   }

   cmq->nelems--;

   return true;
}

bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len)
{
   if (!cmq || !cmq->tail)
      return false;

   if (payload)
      *payload = cmq->tail->payload;

   if (payload_len)
      *payload_len = cmq->tail->payload_len;

   return true;
}

