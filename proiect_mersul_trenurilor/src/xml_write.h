#ifndef _XMLWRITE_H
#define _XMLWRITE_H

#include <stdio.h>

#ifndef String
typedef char string[128];
#endif

typedef struct data
{
  int an;
  int luna;
  int zi;
  int ora;
  int min;
} data_t;

typedef struct info_data
{
  data_t data_plecare;
  data_t data_sosire;
  data_t data_sosire_ef;
} info_data_t;

typedef struct tren
{
  int tren_id;
  string tren_tip;
  info_data_t info_date[10]; //todo make this dynamic (maybe directly from xml)
} tren_t;
#endif