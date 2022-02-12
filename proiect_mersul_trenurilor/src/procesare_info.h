#ifndef PROCESARE_INFO_H
#define PROCESARE_INFO_H

#define FIRST_ELEM "start"

#include "procesare_if.h"
#define LENGTH_STR 1024
#define LENGTH_OUT 8192
// typedef struct read_queue_ read_queue_t;

typedef struct read_queue_
{
  struct sockaddr_in client;
  char msg[256];
  int sd;
  struct read_queue_ *next;
} read_queue_t;

typedef struct
{
  read_queue_t *primul;
  read_queue_t *ultim;
} read_queue_prim_t;

typedef struct
{
  read_queue_prim_t *urm;
} read_queue_next_t;

typedef struct write_elem_
{
  struct sockaddr_in client;
  char tip[256];
  char id[256];
  int delay;
  int sd;
  struct write_elem_ *next;
} write_elem_t;

typedef struct
{
  write_elem_t *primul;
} write_queue_t;

write_queue_t *write_queue;
read_queue_next_t *read_queue_vec[NR_THREADS];

extern void *read_info(void *);
extern void *write_info(void *);
extern void *timer_info(void *);
extern void *announcer_write(void *);
extern void init_var();
extern void init_queues();

#endif
