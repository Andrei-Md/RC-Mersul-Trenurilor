#include "procesare_if.h"

static pthread_mutex_t th_read_m[NR_THREADS];
static pthread_mutex_t th_upd_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t th_write_status = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t th_nothreads_status = PTHREAD_MUTEX_INITIALIZER;
static sem_t th_sem_wr_mutex1;
static sem_t th_sem_wr_mutex2;
static sem_t th_sem_wr_status;

static pthread_cond_t th_read_cond[NR_THREADS];
static pthread_cond_t th_cond_wr_on;

extern void init_mutexes()
{
  /* read mutexes init */
  for (size_t i = 0; i < NR_THREADS; i++)
  {
    pthread_mutex_init(&th_read_m[i], NULL);
  }

  /* read thread conditional init */
  pthread_cond_init(&th_cond_wr_on, NULL);
  for (size_t i = 0; i < NR_THREADS; i++)
  {
    pthread_cond_init(&th_read_cond[i], NULL);
  }

  /* write sem init */
  sem_init(get_th_wr_mutex1_sem(), 0, 1);
  sem_init(get_th_wr_mutex2_sem(), 0, 0);
  sem_init(get_th_wr_status_sem_(), 0, 1);
}

pthread_mutex_t *get_th_read_mutex(int thread_no)
{
  return &th_read_m[thread_no];
}

pthread_cond_t *get_th_read_cond(int thread_no)
{
  return &th_read_cond[thread_no];
}

pthread_mutex_t *get_th_upd_list_mutex()
{
  return &th_upd_list_mutex;
}

sem_t *get_th_wr_mutex1_sem()
{
  return &th_sem_wr_mutex1;
}
sem_t *get_th_wr_mutex2_sem()
{
  return &th_sem_wr_mutex2;
}

pthread_mutex_t *get_th_write_status_mutex()
{
  return &th_write_status;
}

pthread_mutex_t *get_th_nothreads_mutex()
{
  return &th_nothreads_status;
}

pthread_cond_t *get_th_wr_on_cond()
{
  return &th_cond_wr_on;
}

sem_t *get_th_wr_status_sem_()
{
  return &th_sem_wr_status;
}
