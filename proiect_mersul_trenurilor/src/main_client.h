#ifndef _MAIN_CLIENT_H
#define _MAIN_CLIENT_H

#define QUIT 1
#define INIT_MENU 2
#define LOGGEDIN 4

#define INIT_LOG 1
#define SEC_LOG 2
#define TRY_LOG 3

#define REQUEST1 1
#define REQUEST2 2
#define REQUEST3 3
#define REQUEST4 4
#define REQUEST_ANY 0

#define OK_REQ 0
#define TRY_REQ 1

#define LOGARE "login"
#define LOG_AUTH "auth"
#define LOG_NOT_AUTH "not_auth"
#define DELIM "|"

#define REQ_ALL_DAY "all"
#define REQ_ARRIVAL "arrival"
#define REQ_DEPARTURE "departure"
#define REQ_UPDATE "update"

#define LENGTH 8192

typedef struct 
{
  int menu_state;
  int login_state;
  int req_state;
  int fail_state;
}states_t;


#endif