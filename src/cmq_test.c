
/* ****************************************************************************
 *                   Copyright © Lelanthran Manickum, 2021                    *
 *                     Provided under Gnu LGPL 2.1 only.                      *
 * ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <pthread.h>

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
      if (!(cmq_post (cmq, tmp, g_strings[i].len))) {
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

   while ((more = cmq_wait (cmq, (void *)&output, &output_len, 0))==true) {
      CMQ_LOG ("Removed [%s:%zu] (%i remaining)\n", output, output_len, cmq_count (cmq));
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

   size_t limit = cmq_count (cmq) * 4;
   size_t i = 0;

   CMQ_LOG ("Maximum loop limit [%zu]\n", limit);
   while (i++ < limit) {
      char test_string[30];
      snprintf (test_string, sizeof test_string, "String [%zu]", i);
      if (!(cmq_post (cmq, lstrdup (test_string), strlen (test_string) + 1))) {
         CMQ_LOG ("Failed to insert [%s]\n", test_string);
         ret = EXIT_FAILURE;
      }
      CMQ_LOG ("Added [%s]\n", test_string);

      char *tmp = NULL;

      if (!(cmq_wait (cmq, (void **)&tmp, NULL, 0))) {
         CMQ_LOG ("No more elements to remove\n");
         break;
      }
      CMQ_LOG ("Removed [%s]\n", tmp);
      free (tmp);
      if (!(cmq_wait (cmq, (void **)&tmp, NULL, 0))) {
         CMQ_LOG ("No more elements to remove\n");
         break;
      }
      CMQ_LOG ("Removed [%s]\n", tmp);
      free (tmp);
   }
   CMQ_LOG ("Looped [%zu] times\n", i);

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

pthread_mutex_t g_lock;

void *worker_test (void *vptr_cmq)
{

#ifdef PLATFORM_Windows
#define PTHREAD_SPEC    "%llu"
#else
#define PTHREAD_SPEC    "%lu"
#endif

#define THRD_LOG(...)        do {\
   pthread_mutex_lock (&g_lock);\
   printf (PTHREAD_SPEC ": ", id);\
   printf (__VA_ARGS__);\
   pthread_mutex_unlock (&g_lock);\
} while (0)

   cmq_t *cmq = vptr_cmq;
   pthread_t id = pthread_self ();
   THRD_LOG ("Starting thread [" PTHREAD_SPEC "]\n", id);

   for (size_t i=0; i<200; i++) {
      int toss = rand () % 2;
      char *tmp = NULL;
      if (toss) {
         char tmpstring[20];
         snprintf (tmpstring, sizeof tmpstring, "" PTHREAD_SPEC ": [%zu]", id, i);
         tmp = lstrdup (tmpstring);
         if (!(cmq_post (cmq, tmp, strlen (tmp) + 1))) {
            THRD_LOG ("Error: unable to insert into cmq [%i elements]\n", cmq_count (cmq));
            free (tmp);
            return NULL;
         }
         THRD_LOG ("Insert [%s] into cmq [%i elements]\n", tmp, cmq_count (cmq));
      } else {
         if (!(cmq_wait (cmq, (void **)&tmp, NULL, 1000))) {
            THRD_LOG ("Queue appears to be empty, unable to remove\n");
            continue;
         }
         THRD_LOG ("Removed [%s] from cmq [%i elements]\n", tmp, cmq_count (cmq));
         free (tmp);
      }
      struct timespec tv = { 0, 0};

      tv.tv_nsec = 1000000 * (rand () % 9);

      nanosleep (&tv, NULL);

   }
   THRD_LOG ("Ending thread\n");

   return NULL;
}

int test_threaded (cmq_t *cmq)
{
   int ret = EXIT_SUCCESS;

   pthread_mutexattr_t attr;
   pthread_mutexattr_init (&attr);
   pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init (&g_lock, &attr);
   pthread_mutexattr_destroy (&attr);

   if (!(populate_strings (cmq, true))) {
      CMQ_LOG ("Failed to populate the strings table\n");
      return EXIT_FAILURE;
   }

   pthread_t threads[20];
   for (size_t i=0; i<sizeof threads/sizeof threads[0]; i++) {
      if ((pthread_create (&threads[i], NULL, worker_test, cmq))!=0) {
         CMQ_LOG ("Failed to start new thread %zu, aborting\n", i);
         ret = EXIT_FAILURE;
      }
   }

   for (size_t i=0; i<sizeof threads/sizeof threads[0]; i++) {
      pthread_join (threads[i], NULL);
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

   if ((test_threaded (mq))!=EXIT_SUCCESS) {
      CMQ_LOG ("gradual-depletion test failed...\n");
      ret = EXIT_FAILURE;
   } else {
      CMQ_LOG ("gradual-depletion test passed.\n");
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
