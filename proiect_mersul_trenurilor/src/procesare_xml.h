#ifndef _PROCESAREXML_H
#define _PROCESAREXML_H

#include <stdio.h>
#include <time.h>
#define AN "an"
#define LUNA "luna"
#define ZI "zi"
#define ORA "ora"
#define MINUT "minut"
#define LENGTH_S 256

#define REQ_ARRIVAL "arrival"
#define REQ_DEPARTURE "departure"

#ifndef String
typedef char string[LENGTH_S];
#endif

#define NO_TYPES 3

typedef struct info_data info_data_t;
typedef struct tren tren_t;

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
  info_data_t* next; 
  tren_t* tren_parinte;
} info_data_t;

typedef struct tren
{
  string tren_tip;
  string tren_id;
  info_data_t* info_data_first; 
  info_data_t* info_data_last; 
  tren_t* next_tren; 
} tren_t;

typedef struct trenuri
{
  tren_t* first;
  tren_t* last;
} trenuri_t;


typedef struct
{
  time_t data;
  info_data_t* tren_info_data;
}data_time_t;

typedef struct
{
  string tip_id;
  int cnt;
  tren_t* tren_info;
}data_name_t;

extern char *get_tbl_zi_cur();
extern char *get_tbl_1h(char *str);
extern void set_tbl_delay(char *elem_tip, char *elem_id, int delay);
extern void init_data_xml();
extern void xml_write();
extern void free_trenuri();


#endif