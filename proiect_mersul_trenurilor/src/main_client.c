/* 
 * running:
 *   ./main_client.bin <adresa_server> <port>
 * e.g.:
 *   ./main_client.bin 127.0.0.1 2801
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "main_client.h"

#undef DEBUGG
// #define DEBUGG

#define nr_tst 10

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;



void client_server(char *mesaj_out, char *mesaj_in, states_t *state);

void procesare_intrare_user(char *str);
void menu();
bool parse_first_two(char *string, char *str1, char *str2, char *delim);
void viz_menu(states_t *state, char *str_transfer, char *msg_out);

int msglen = 0, length = 0;
int sd;                    // descriptorul de socket
struct sockaddr_in server; // structura folosita pentru conectare
char msg_header[LENGTH];

int main(int argc, char *argv[])
{

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul */
  port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  struct timeval read_timeout;
  read_timeout.tv_sec = 2;
  read_timeout.tv_usec = 0;
  //! TODO (tb sa-i dau drumul)
  setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  /* umplem structura folosita pentru realizarea dialogului cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  char str_in[LENGTH];  //input user
  char msg_out[LENGTH]; //msg trimis
  char msg_in[LENGTH];  //msg primit
  char user[LENGTH];    //user
  char psw[LENGTH];     //password
  char code[LENGTH];    //code
  states_t state;
  state.menu_state = INIT_MENU;
  state.login_state = INIT_LOG;
  state.req_state = REQUEST_ANY;
  state.fail_state = OK_REQ;

  while (1)
  {
    //afisare
    viz_menu(&state, str_in, msg_in);
    //incerc sa ma loghez
    procesare_intrare_user(str_in);

    if (state.menu_state == INIT_MENU) //INITIAL MENU
    {
      state.login_state = SEC_LOG;
      if (strcmp(str_in, "q") == 0) //QUIT
      {
        break;
      }
      else
      {
        //vf daca am 2 cuvinte (user si parola)
        if (parse_first_two(str_in, user, psw, " ") == true)
        { //incerc sa trimit
          memset(msg_header, 0, LENGTH);
          strcpy(msg_header, LOGARE);
          memset(msg_out, 0, LENGTH);
          sprintf(msg_out, "%s|%s|%s", msg_header, user, psw);
#ifdef DEBUGG
          printf("header->%s\n", msg_out);
#endif
          client_server(msg_out, msg_in, &state);

          //TODO primire confimare de logate server
          // client_server(mesaj_out, mesaj_in);
          char auth[LENGTH];
          if (parse_first_two(msg_in, auth, code, DELIM) == true)
          {
            if (strcmp(auth, LOG_AUTH) == 0)
            {
              state.menu_state = LOGGEDIN;
            }
            else if (strcmp(auth, LOG_NOT_AUTH) == 0)
              printf("not auth\n");
          }
        }
        else
        { //o sa incerc sa ma loghez dinnou sau sa dau quit
        }
      }
    }
    //TODO
    else if (state.menu_state == LOGGEDIN) //LOGGEDIN
    {                                      //m-am logat
      if (strcmp(str_in, "q") == 0)
      {
        state.menu_state = INIT_MENU;
      }
      else
      {
        if (strcmp(str_in, "1") == 0) //mersul trenurilor pentru toata ziua
        {
          state.req_state = REQUEST1;
          //setez header
          memset(msg_header, 0, LENGTH);
          sprintf(msg_header, "%s|%s", user, code);
          memset(msg_out, 0, LENGTH);
          sprintf(msg_out, "%s|%s", REQ_ALL_DAY, msg_header);

          //todo get state from server in client_server
          client_server(msg_out, msg_in, &state);

          if (state.fail_state == TRY_REQ)
          {
            memset(msg_in, 0, LENGTH);
            sprintf(msg_in, "cererea nu a putut fi procesata, mai incercati\n");
          }
#ifdef DEBUGG
          printf(msg_out, "mersul trenurilor pt azi\n");
#endif
        }
        else if (strcmp(str_in, "2") == 0)
        { //mersul trenurilor plecari pentru urmatoare ora
          state.req_state = REQUEST2;
          memset(msg_header, 0, LENGTH);
          memset(msg_out, 0, LENGTH);
          sprintf(msg_header, "%s|%s", user, code);
          sprintf(msg_out, "%s|%s", REQ_DEPARTURE, msg_header);

          client_server(msg_out, msg_in, &state);

          if (state.fail_state == TRY_REQ)
          {
            memset(msg_in, 0, LENGTH);
            sprintf(msg_in, "cererea nu a putut fi procesata, mai incercati\n");
          }
#ifdef DEBUGG
          printf(msg_out, "plecari 1h\n");
#endif
        }
        else if (strcmp(str_in, "3") == 0)
        { //mersul trenurilor sosiri pentru urmatoarea ora
          state.req_state = REQUEST3;
          memset(msg_header, 0, LENGTH);
          memset(msg_out, 0, LENGTH);
          sprintf(msg_header, "%s|%s", user, code);
          sprintf(msg_out, "%s|%s", REQ_ARRIVAL, msg_header);

          client_server(msg_out, msg_in, &state);

          if (state.fail_state == TRY_REQ)
          {
            memset(msg_in, 0, LENGTH);
            sprintf(msg_in, "cererea nu a putut fi procesata, mai incercati\n");
          }
#ifdef DEBUGG
          printf(msg_out, "sosiri 1h\n");
#endif
        }
        else if (strcmp(str_in, "4") == 0)
        { //actualizare info tren
          state.req_state = REQUEST4;
          char input_user[256];
          char c;
          int cnt = 0;
          memset(input_user, 0, sizeof(input_user));
          printf("Introduceti tipul si id-ul trenului urmat de intarziere \n");
          printf("<TREN TIP> <TREN ID> <INTARZIERE> e.g.: R 2 30)\n");
          while ((c = getchar()) != '\n' && c != EOF)
          {
            input_user[cnt] = c;
            cnt++;
          }
          input_user[cnt] = '\0';
          cnt = 0;
          char mesaj_modif[3][256];
          memset(mesaj_modif, 0, sizeof(mesaj_modif));
          char *token;
          token = strtok(input_user, " ");
          while (token != NULL)
          {
            strcpy(mesaj_modif[cnt], token);
            cnt++;
            if (cnt == 3)
            {
              break;
            }
            token = strtok(NULL, " ");
          }

          memset(msg_header, 0, LENGTH);
          memset(msg_out, 0, LENGTH);
          sprintf(msg_header, "%s|%s", user, code);
          sprintf(msg_out, "%s|%s", REQ_UPDATE, msg_header);
          sprintf(msg_out + strlen(msg_out), "|%s|%s|%s", mesaj_modif[0], mesaj_modif[1], mesaj_modif[2]);

          client_server(msg_out, msg_in, &state);

          if (state.fail_state == TRY_REQ)
          {
            memset(msg_in, 0, LENGTH);
            sprintf(msg_in, "cererea nu a putut fi procesata, mai incercati\n");
          }
#ifdef DEBUGG
          printf(msg_out, "modificare cu success\n");
#endif
        }
        else
        {
          state.req_state = REQUEST_ANY;
        }
      }

      // printf("%s", str_transfer);
      // printf("LOGGED IN main\n");
    }
  }

  /* inchidem socket-ul, am terminat */
  close(sd);
}

bool parse_first_two(char *string, char *str1, char *str2, char *delim)
{
  char tmp[LENGTH];
  strcpy(tmp, string);
  memset(str1, 0, LENGTH);
  memset(str2, 0, LENGTH);
  // const char delim[2] = delim;
  char *token;
  token = strtok(tmp, delim);
  if (token != NULL)
  {
    strcpy(str1, token);
  }
  else
  {
    return false;
  }

  token = strtok(NULL, delim);

  if (token != NULL)
  {
    strcpy(str2, token);
  }
  else
  {
    return false;
  }

  token = strtok(NULL, delim);

  if (strlen(str1) > 0 && strlen(str2) > 0 && token == NULL)
    return true;

  // for (size_t i = 0; i < strlen(str_in); i++)
  // {
  //   if (str_in[i] == ' ')
  //     return false;
  // }

  return false;
}

void client_server(char *mesaj_out, char *mesaj_in, states_t *state)
{
  state->fail_state = OK_REQ;
  memset(mesaj_in, 0, LENGTH);
#ifdef DEBUGG
  printf("[client]Mesajul trimis este: \n%s\n", mesaj_out);
#endif
  /* trimiterea mesajului la server */
  length = sizeof(server);
  int len = strlen(mesaj_out);
  if (sendto(sd, mesaj_out, len, 0, (struct sockaddr *)&server, length) <= 0)
  {
    perror("[client]Eroare la sendto() spre server.\n");
    exit(errno);
  }

  /* citirea raspunsului dat de server 
   (apel blocant pina cind serverul raspunde) */
  if ((msglen = recvfrom(sd, mesaj_in, LENGTH, 0, (struct sockaddr *)&server, &length)) < 0)
  {
    if (errno == EWOULDBLOCK)
    {
      state->login_state = TRY_LOG;
      state->fail_state = TRY_REQ;
      return;
    }
    else
    {
      perror("[client]Eroare la recvfrom() de la server.\n");
      exit(errno);
    }
  }
/* afisam mesajul primit */
#ifdef DEBUGG
  printf("[client]Mesajul primit este: \n%s\n", mesaj_in);
#endif

//todo scot asta
#ifdef DEBUGG
  nanosleep((const struct timespec[]){{0, 200000000L}}, NULL); //0.2s
#endif
  // sleep(1);
}

void procesare_intrare_user(char *str)
{
  int ch;
  int c = 0;
  char *tmp_str;
  /* read user input */
  while ((ch = getchar()) != '\n' && ch != EOF)
  {
    str[c] = ch;
    c++;
  }
  str[c] = '\0';
  char no[10];
  int len = strlen(str);
}

void viz_menu(states_t *state, char *str_transfer, char *msg_in)
{
  //clear screen (ANSI escape)
  printf("\e[2J\n");

#ifdef DEBUGG
  printf("%s\n", str_transfer);
#endif

  switch (state->menu_state)
  {
  case LOGGEDIN:
  {
    // printf("LOGGEDIN\n");
    if (state->login_state == SEC_LOG)
    {
      printf("Autentificare reusita\n\n");
      state->login_state = INIT_LOG;
    }
    else if (state->req_state == REQUEST1) //mersul trenurilor pentru toata ziua
    {
      printf("%s\n", msg_in);
    }
    else if (state->req_state == REQUEST2) //mersul trenurilor pentru toata ziua
    {
      printf("%s\n", msg_in);
    }
    else if (state->req_state == REQUEST3) //mersul trenurilor pentru toata ziua
    {
      printf("%s\n", msg_in);
    }
    else if (state->req_state == REQUEST4) //mersul trenurilor pentru toata ziua
    {
      printf("%s\n", msg_in);
    }
    else
    {
#ifdef DEBUGG
      printf("loggin else\n");
#endif
    }

    menu();
    break;
  }

  case QUIT:
    // printf("QUIT\n");
    break;

  case INIT_MENU:
    // printf("INITMENU\n");
    if (state->login_state == INIT_LOG) //prima logare
      printf("Introduceti username si parola sau quit (q)\n");
    else if (state->login_state == SEC_LOG) //a doua logare
    {
      printf("username-ul si parola nu corespund, mai incearca\n");
      printf("Introduceti username si parola sau quit (q)\n");
    }
    else if (state->login_state == TRY_LOG)
    {
      printf("Din pacate serverul nu poate fi accesat\n");
      printf("Introduceti username si parola sau quit (q)\n");
    }
    break;
  default:
    printf("DEFAULT\n");
    break;
  }
}

void menu()
{
  //clear screen (ANSI escape)
  // printf("\e[2J");
  printf("Introduceti optiunea dorita:\n");
  printf("1. mersul trenurilor pentru astazi:\n");
  printf("2. informatii plecari trenuri in urmatoarea ora:\n");
  printf("3. informatii sosiri trenuri in urmatoarea ora:\n");
  printf("4. actualizare informatii referitoare la trenuri in urmatoarea ora:\n");
}