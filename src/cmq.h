
#ifndef H_CMQ
#define H_CMQ

#include <stdbool.h>
#include <stdlib.h>

typedef struct cmq_t cmq_t;

#define CMQ_LOG(...)        do {\
   printf ("[%s:%i] ", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

   cmq_t *cmq_new (void);
   void cmq_del (cmq_t *cmq);

   bool cmq_insert (cmq_t *cmq, void *payload, size_t payload_len);
   bool cmq_remove (cmq_t *cmq, void **payload, size_t *payload_len);
   bool cmq_peek (cmq_t *cmq, void **payload, size_t *payload_len);

#ifdef __cplusplus
};
#endif


#endif
