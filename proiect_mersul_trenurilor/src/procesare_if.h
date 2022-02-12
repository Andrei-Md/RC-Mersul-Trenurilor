#ifndef PROCESARE_IF_H
#define PROCESARE_IF_H

#include <unistd.h>

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "procesare_xml.h"

#define NR_THREADS 4
#define TIME_TO_WAIT_WRITE 2 //seconds
/* function to init all mutexes, semaphores and conditional variables */
extern void init_mutexes();

pthread_mutex_t *get_th_read_mutex(int);
//read thread cond var
pthread_cond_t *get_th_read_cond(int);

/* write */
pthread_mutex_t *get_th_upd_list_mutex();

sem_t *get_th_wr_mutex1_sem();
sem_t *get_th_wr_mutex2_sem();


/* sinc write-read */
/* contorizez nr de thread-uri */
pthread_mutex_t *get_th_write_status_mutex(); //mutex pt cond write
pthread_mutex_t *get_th_nothreads_mutex(); //mutex pt modific nr de thread-uri active

pthread_cond_t *get_th_wr_on_cond();
sem_t *get_th_wr_status_sem_();

// size_t get_write_status();
// void set_write_status(size_t);

#endif