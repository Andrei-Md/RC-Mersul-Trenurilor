#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#include "procesare_info.h"

#define STUPID_TIME

static read_queue_t tmp_read_elem;
static write_queue_t *tmp_write_queue;
static void rasp_client(int thread_id);
static void get_client(int thread_id);
static void write_in_db(write_queue_t *tmp_write_queue);

size_t no_threads;
size_t write_status;

extern void init_var()
{
  no_threads = NR_THREADS;
  write_status = 0;
}

extern void init_queues()
{
  /* read queue */
  for (size_t i = 0; i < NR_THREADS; i++)
  {
    read_queue_next_t *temp = (read_queue_next_t *)malloc(sizeof(read_queue_next_t));
    read_queue_vec[i] = temp;
    read_queue_prim_t *tmp = (read_queue_prim_t *)malloc(sizeof(read_queue_prim_t));
    tmp->primul = NULL;
    tmp->ultim = NULL;
    read_queue_vec[i]->urm = tmp;
  }

  /* write queue */
  write_queue_t *tmp = (write_queue_t *)malloc(sizeof(write_queue_t));
  write_queue = tmp;
  write_queue->primul = NULL;
}

static void get_client(int thread_id)
{
  while (read_queue_vec[thread_id]->urm->primul == NULL)
  {
#ifdef DEBUGG
    printf("nr_th: %ld\n", no_threads);
    printf("aici %d\n", thread_id);
#endif
    pthread_mutex_lock(get_th_nothreads_mutex());
    no_threads--;
    if (no_threads == 0)
    {
#ifdef DEBUGG
      printf(">>>>>>>>il trezesc\n");
#endif
      pthread_cond_signal(get_th_wr_on_cond()); //semnal la writer at cand nu mai sunt thread-uri de read
    }
    pthread_mutex_unlock(get_th_nothreads_mutex());
    pthread_cond_wait(get_th_read_cond(thread_id), get_th_read_mutex(thread_id));

    pthread_mutex_lock(get_th_nothreads_mutex());
    no_threads++;
    pthread_mutex_unlock(get_th_nothreads_mutex());
  }

  /* wait to write in db */
  pthread_mutex_lock(get_th_write_status_mutex());
  if (write_status == 1)
  {
    pthread_mutex_lock(get_th_nothreads_mutex());
    no_threads--;
    pthread_mutex_unlock(get_th_nothreads_mutex());

    if (no_threads == 0)
    {
#ifdef DEBUGG
      printf(">>>>>>>>>>>>>>>>>il trezesc\n");
#endif
      pthread_cond_signal(get_th_wr_on_cond()); //semnal la writer at cand nu mai sunt thread-uri de read
    }
    pthread_mutex_unlock(get_th_write_status_mutex());
    sem_wait(get_th_wr_status_sem_());
    sem_post(get_th_wr_status_sem_());
    pthread_mutex_lock(get_th_nothreads_mutex());
    no_threads++;
    pthread_mutex_unlock(get_th_nothreads_mutex());
  }
  else
  {
    pthread_mutex_unlock(get_th_write_status_mutex());
  }
#ifdef DEBUGG
  printf("write status:>>%ld", write_status);
#endif

  /* copii elem citit*/
  strcpy(tmp_read_elem.msg, read_queue_vec[thread_id]->urm->primul->msg);
  tmp_read_elem.sd = read_queue_vec[thread_id]->urm->primul->sd;
  tmp_read_elem.client = read_queue_vec[thread_id]->urm->primul->client;

  /* sterg elem din lista */
  if (read_queue_vec[thread_id]->urm->primul->next == NULL)
  {
    free(read_queue_vec[thread_id]->urm->primul);
    read_queue_vec[thread_id]->urm->primul = NULL;
    read_queue_vec[thread_id]->urm->ultim = NULL;
  }
  else
  {
    read_queue_t *tmp = read_queue_vec[thread_id]->urm->primul;
    read_queue_vec[thread_id]->urm->primul = read_queue_vec[thread_id]->urm->primul->next;
    free(tmp);
  }
  pthread_mutex_unlock(get_th_read_mutex(thread_id));
}

static void rasp_client(int thread_id)
{
  /* daca nu are mesaje de trimis thread-ul intra in wait */
  pthread_mutex_lock(get_th_read_mutex(thread_id));

  char *msgrasp; //mesaj de raspuns pentru client

  /*pregatesc mesajul de raspuns */
  if (strcmp(tmp_read_elem.msg, "all") == 0)
  {
    msgrasp = get_tbl_zi_cur();
    if (msgrasp == NULL)
    {
      msgrasp = strdup("Nici un tren nu pleaca astazi!");
    }
  }
  else if (strcmp(tmp_read_elem.msg, REQ_ARRIVAL) == 0)
  {
//!init xml daca nu functioneaza MKTIME corect (doar primele interogari scot timp aiurea) (depinde de statie)
#ifdef STUPID_TIME
    free_trenuri();
    init_data_xml();
#endif
    //
    msgrasp = get_tbl_1h(REQ_ARRIVAL);
    if (msgrasp == NULL)
    {
      msgrasp = strdup("Nici un tren nu soseste in urmatoarea ora!");
    }
  }
  else if (strcmp(tmp_read_elem.msg, REQ_DEPARTURE) == 0)
  {
    //!init xml daca nu functioneaza MKTIME corect (doar primele interogari scot timp aiurea) (depinde de statie)
    //
#ifdef STUPID_TIME
    free_trenuri();
    init_data_xml();
#endif
    //
    msgrasp = get_tbl_1h(REQ_DEPARTURE);
    if (msgrasp == NULL)
    {
      msgrasp = strdup("Nici un tren nu pleaca in urmatoarea ora!");
    }
  }

  // bzero(msgrasp, 100);
  // strcat(msgrasp, "trenul-> ");
  // strcat(msgrasp, tmp_read_elem.msg);
  // sprintf(msgrasp + strlen(msgrasp), "-> %d", thread_id);

#ifdef DEBUGG
  printf("[server]Trimitem mesajul inapoi...%s\n", msgrasp);
#endif

  printf("mesaj returnat<<-%s\n", msgrasp);
  /* returnez mesajul clientului */
  int length = sizeof(tmp_read_elem.client);
  if (sendto(tmp_read_elem.sd, msgrasp, strlen(msgrasp), 0, (struct sockaddr *)&tmp_read_elem.client, length) <= 0)
  {
    perror("[server]Eroare la sendto() catre client.\n");
    return;
  }
  else
  {
    printf("[server]Mesajul a fost transmis cu succes.\n");
  }
  // free(msgrasp);
  return;
}

extern void *read_info(void *th_id)
{
  long thread_id = (long)th_id;

#ifdef DEBUGG
  printf("read thread init -> %ld\n", thread_id);
#endif
  while (1)
  {
    get_client(thread_id);
    rasp_client(thread_id);
  }

  pthread_exit(th_id);
}

extern void *write_info(void *th_id)
{
  long thread_id = (long)th_id;
#ifdef DEBUGG
  printf("write thread init -> %ld\n", thread_id);
#endif
  //!
  while (1)
  {
    /* sem ciclic */
    sem_wait(get_th_wr_mutex2_sem());
#ifdef DEBUGG
    printf("here\n");
#endif

    /* sinc cu th de read */
    /* semnalez ca vreau sa scriu */
    pthread_mutex_lock(get_th_write_status_mutex());
    write_status = 1;
#ifdef DEBUGG
    printf("write_status -> %ld", write_status);
#endif
    /* blochez th de read la sem */
    sem_wait(get_th_wr_status_sem_());

    /* astept semnal ca toate thread-urile s-au oprit */
    /* vf intai daca nu cumva sunt toate oprite */
    pthread_mutex_lock(get_th_nothreads_mutex());
    if (no_threads != 0)
    {
      pthread_mutex_unlock(get_th_nothreads_mutex());
      pthread_cond_wait(get_th_wr_on_cond(), get_th_write_status_mutex());
    }
    else
      pthread_mutex_unlock(get_th_nothreads_mutex());

    /* copii lista de write-uri */
    pthread_mutex_lock(get_th_upd_list_mutex());
    tmp_write_queue = (write_queue_t *)malloc(sizeof(write_queue_t));
    tmp_write_queue->primul = write_queue->primul;
    write_queue->primul = NULL;

    /* am terminat de copiat */
    pthread_mutex_unlock(get_th_upd_list_mutex());

    if (tmp_write_queue->primul != NULL)
    {
      //sleep pentru debug
      // sleep(3);
      write_in_db(tmp_write_queue);
    }
    else //ma intorc la semafor
    {
      free(tmp_write_queue);
#ifdef DEBUGG
      printf("nu am printat nimic in db\n\n");
#endif
    }
    /* semnalez th read */
    write_status = 0; //modific statusul la write
    pthread_mutex_unlock(get_th_write_status_mutex());
    sem_post(get_th_wr_status_sem_());
    /* semnalez th ciclic */
    sem_post(get_th_wr_mutex1_sem());
  }
  pthread_exit(th_id);
}

/* print mesajele si free memory */
static void write_in_db(write_queue_t *tmp_write_queue)
{
  static char temp_msg[1024];
  temp_msg[0] = '\0';

  while (tmp_write_queue->primul != NULL)
  {

    //! setez in tabel
    set_tbl_delay(tmp_write_queue->primul->tip, tmp_write_queue->primul->id, tmp_write_queue->primul->delay);

    /* free mem */
    write_elem_t *tmp = tmp_write_queue->primul;
    tmp_write_queue->primul = tmp_write_queue->primul->next;
    free(tmp);
  }
  //! write in DB
  xml_write();
  //! init database
  free_trenuri();
  init_data_xml();
  free(tmp_write_queue);

  printf("am modificat in db:\n%s\n", temp_msg);
#ifdef DEBUGG
#endif
  return;
}

/* trigger writing info in db */
extern void *announcer_write(void *th_id)
{
  long thread_id = (long)th_id;
#ifdef DEBUGG
  printf("announcer_write thread init -> %ld\n", thread_id);
#endif
  while (1)
  {
    sem_wait(get_th_wr_mutex1_sem());

    //todo cond var
    nanosleep((const struct timespec[]){{TIME_TO_WAIT_WRITE, 0L}}, NULL);

    sem_post(get_th_wr_mutex2_sem());
  }
  pthread_exit(th_id);
}
