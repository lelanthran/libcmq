
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmq.h"

int test_strings (cmq_t *cmq)
{
   static struct {
      char *string;
      size_t len;
   } strings [] = {
      {  "one",         0 },
      {  "two",         0 },
      {  "three",       0 },
      {  "four",        0 },
      {  "five",        0 },
      {  "six",         0 },
      {  "seven",       0 },
      {  "eight",       0 },
      {  "nine",        0 },
      {  "ten",         0 },
   };
   static size_t nstrings = sizeof strings / sizeof strings[0];

   int ret = EXIT_SUCCESS;

   for (size_t i=0; i<nstrings; i++) {
      strings[i].len = strlen (strings[i].string) + 1;
      if (!(cmq_insert (cmq, strings[i].string, strings[i].len))) {
         CMQ_LOG ("Failed to insert [%s:%zu]\n", strings[i].string, strings[i].len);
         ret = EXIT_FAILURE;
      }
   }

   bool more = true;
   const char *output = NULL;
   size_t output_len = 0;

   while ((more = cmq_remove (cmq, (void *)&output, &output_len))==true) {
      CMQ_LOG ("Removed [%s:%zu]\n", output, output_len);
   }

   return ret;
}

int main (void)
{
   int ret = EXIT_SUCCESS;

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
