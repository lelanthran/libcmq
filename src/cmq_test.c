
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmq.h"

static char *lstrdup (const char *src)
{
   char *ret = malloc (strlen (src) + 1);
   if (ret)
      strcpy (ret, src);
   return ret;
}

static struct {
   char *string;
   size_t len;
} g_strings [] = {
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
static size_t g_nstrings = sizeof g_strings / sizeof g_strings[0];

bool populate_strings (cmq_t *cmq, bool allocate)
{
   bool ret = true;
   for (size_t i=0; i<g_nstrings; i++) {
      g_strings[i].len = strlen (g_strings[i].string) + 1;
      char *tmp = allocate ? lstrdup (g_strings[i].string) : g_strings[i].string;
      if (!(cmq_insert (cmq, tmp, g_strings[i].len))) {
         CMQ_LOG ("Failed to insert [%s:%zu]\n", g_strings[i].string, g_strings[i].len);
         ret = false;
      }
   }
   return ret;
}

int test_simple (cmq_t *cmq)
{
   int ret = EXIT_SUCCESS;

   bool more = true;
   const char *output = NULL;
   size_t output_len = 0;

   if (!(populate_strings (cmq, false))) {
      CMQ_LOG ("Not all strings populated\n");
      ret = EXIT_FAILURE;
   }

   while ((more = cmq_remove (cmq, (void *)&output, &output_len))==true) {
      CMQ_LOG ("Removed [%s:%zu] (%zu remaining)\n", output, output_len, cmq_count (cmq));
   }

   return ret;
}

int test_gradual_depletion (cmq_t *cmq)
{
   int ret = EXIT_SUCCESS;

   if (!(populate_strings (cmq, true))) {
      CMQ_LOG ("Not all strings populated\n");
      ret = EXIT_FAILURE;
   }

   // TODO: Add 1 string, remove 2, in a loop until the queue is empty.
   size_t limit = cmq_count (cmq) * 4;
   size_t i = 0;

   CMQ_LOG ("Maximum loop limit [%zu]\n", limit);
   while (i++ < limit) {
      char test_string[30];
      snprintf (test_string, sizeof test_string, "String [%zu]", i);
      if (!(cmq_insert (cmq, lstrdup (test_string), strlen (test_string) + 1))) {
         CMQ_LOG ("Failed to insert [%s]\n", test_string);
         ret = EXIT_FAILURE;
      }
      CMQ_LOG ("Added [%s]\n", test_string);

      char *tmp = NULL;

      if (!(cmq_remove (cmq, (void **)&tmp, NULL))) {
         CMQ_LOG ("No more elements to remove\n");
         break;
      }
      CMQ_LOG ("Removed [%s]\n", tmp);
      free (tmp);
      if (!(cmq_remove (cmq, (void **)&tmp, NULL))) {
         CMQ_LOG ("No more elements to remove\n");
         break;
      }
      CMQ_LOG ("Removed [%s]\n", tmp);
      free (tmp);
   }
   CMQ_LOG ("Looped [%zu]\n", i);

   return ret;
}

int test_delqueue (cmq_t *cmq)
{
   int ret = EXIT_SUCCESS;

   if (!(populate_strings (cmq, false))) {
      CMQ_LOG ("Not all strings populated\n");
      ret = EXIT_FAILURE;
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

   if ((test_simple (mq))!=EXIT_SUCCESS) {
      CMQ_LOG ("Simple test failed...\n");
      ret = EXIT_FAILURE;
   } else {
      CMQ_LOG ("Simple test passed.\n");
   }

   if ((test_gradual_depletion (mq))!=EXIT_SUCCESS) {
      CMQ_LOG ("gradual-depletion test failed...\n");
      ret = EXIT_FAILURE;
   } else {
      CMQ_LOG ("gradual-depletion test passed.\n");
   }

   if ((test_delqueue (mq))!=EXIT_SUCCESS) {
      CMQ_LOG ("delete-queue test failed...\n");
      ret = EXIT_FAILURE;
   } else {
      CMQ_LOG ("delete-queue test passed.\n");
   }

   cmq_del (mq);

   return ret;
}
