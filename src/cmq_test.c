
#include <stdio.h>
#include <stdlib.h>

#include "cmq.h"

int test_strings (cmq_t *cmq)
{
   return EXIT_FAILURE;
}

int main (void)
{
   int ret = EXIT_SUCCESS;
   int rc = 0;

   cmq_t *mq = cmq_new ();

   printf ("Testing libcmq [%s]\n", cmq_version);

   if (!mq) {
      CMQ_LOG ("Failed to create new message queue object\n");
      return EXIT_FAILURE;
   }

   CMQ_LOG ("Created message queue\n");

   if ((test_strings (mq))!=EXIT_SUCCESS) {
      CMQ_LOG ("String tests failed...\n");
      ret = EXIT_FAILURE;
   } else {
      CMQ_LOG ("String tests passed.\n");
   }

   cmq_del (mq);

   return ret;
}
