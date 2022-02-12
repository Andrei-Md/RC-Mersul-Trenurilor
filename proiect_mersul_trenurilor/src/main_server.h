#ifndef MAIN_SERVER_H
#define MAIN_SERVER_H

/* portul folosit */
#define PORT 2700
#define DELIM "|"


#define TYPE_UPDATE 1
#define TYPE_REQUEST 2
#define TYPE_LOGIN 3

#define NUME_FIS_LOGIN "auth.txt"
#define AUTH "auth"
#define NOT_AUTH "not_auth"



#include "procesare_info.h"
/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct{
  char request[LENGTH_STR];
  char user[LENGTH_STR];
  char psw[LENGTH_STR];
  char tip_tren[LENGTH_STR];
  char id_tren[LENGTH_STR];
  char intarziere[LENGTH_STR];
}catch_t;

#endif
