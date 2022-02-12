/** 
 * Sa se implementeze un server ce ofera sau actualizeaza informatii in timp real de la toti clientii inregistrati pentru:
 *    mersul trenurilor, status plecari, status sosiri, intarzieri si estimare sosire.
 * Serverul citeste datele din fisiere xml si actualizeaza datele(intarzieri si estimare sosire) la cererea clientilor (signal - receive)
 * Toata logica va fi realizata in server, clientul doar cere informatii despre plecari/sosiri si trimite informatii la server despre posible intarzieri si estimare sosire.
 * 
 * Cuvinte cheie : Command design pattern, command queue, threads, sockets
 *
 * Activitati:
 *   trimitere de la server cu informatii despre mersul trenurilor in ziua respectiva
 *   trimitere de la server cu informatii despre plecari in urmatoarea ora ( conform cu planul , in intarziere cu x min) doar la cererea unui client
 *   trimitere de la server cu informatii despre sosiri in urmatoarea ora ( conform cu planul , in intarziere cu x min, cu x min mai devreme) doar la cererea unui client
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/mman.h> //mmap
#include <stdbool.h>
#include <sys/stat.h> //pentru chmod

#include "main_server.h"

static char operations[5][256] = {"update", "all", "arrival", "departure", "login"};
static int contor = 0; /* contor pentru thread-uri sa impart in read_queue in mod egal  */
int nfds;              /* numarul maxim de descriptori */
static pthread_t threads[NR_THREADS];

static void communicate_client(int sd);
static void start_threads();
static void init();

static void proc_tip_request(char *msg_in, char *tip_request);
static void stab_state(char *msg_in, int *state);
bool gaseste_str_in_fis(char *map_addr, char *user, char *psw, char *code, size_t length_map, char *msg_out_login);
bool cauta_in_fis_db(char *cale_fis_db, char *msg_in, char *msg_out_login);
bool parse_first_two(char *str_in, char *user, char *psw);
static void proc_tip_request(char *msg_in, char *tip_request);

static void parse_msg(char *msg, int state);

char catch[6][LENGTH_STR];

int main()
{
  init();
  //!tb sa ma asigur ca totul este initializat (momentan un sleep mic este indeajuns)
  sleep(1);
#ifdef DEBUGG
  printf("Server START\n");
#endif

  struct sockaddr_in server; // structura folosita de server
  fd_set actfds;             /* multimea descriptorilor activi */
  int sd;                    //descriptorul de socket

  /* crearea unui socket */
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  /* completam multimea de descriptori de citire */
  FD_ZERO(&actfds);    /* initial, multimea este vida */
  FD_SET(sd, &actfds); /* includem in multime socketul creat */

  // /* valoarea maxima a descriptorilor folositi */
  // nfds = sd;

  printf("[server]Astept la portul %d...\n", PORT);
  fflush(stdout);

  while (1)
  {
    printf("serverul este gata\n");
    communicate_client(sd);
  } /* while */
} /* main */

static void communicate_client(int sd)
{
  int msglen;
  int length;
  char msg_in[LENGTH_STR];  //mesajul primit de la client
  char msg_tmp[LENGTH_STR]; //mesajul primit de la client
  struct sockaddr_in client;
  int state = 0; //tip-ul mesajului
  bzero(&client, sizeof(client));

  length = sizeof(client);
  bzero(msg_in, LENGTH_STR);
  /* citirea mesajului primit de la client */
  if ((msglen = recvfrom(sd, msg_in, LENGTH_STR, 0, (struct sockaddr *)&client, &length)) <= 0)
  {
    perror("[server]Eroare la recvfrom() de la client.\n");
    return;
  }
  printf("[server]Mesajul rec -> %s\n", msg_in);

  stab_state(msg_in, &state);
  parse_msg(msg_in, state);

  if (state == TYPE_UPDATE)
  { //write

    /* adaug mesajul de update in lista de update-uri */
    pthread_mutex_lock(get_th_upd_list_mutex());
    write_elem_t *tmp = (write_elem_t *)malloc(sizeof(write_elem_t));
    tmp->client = client;
    strcpy(tmp->tip, catch[3]);
    strcpy(tmp->id, catch[4]);
    sscanf(catch[5],"%d",&tmp->delay);
    tmp->sd = sd;

    tmp->next = write_queue->primul;
    write_queue->primul = tmp;

    pthread_mutex_unlock(get_th_upd_list_mutex());

    //trimite msg ca cererea a fost procesata
    static char temp_msg[1024];
    sprintf(temp_msg, "%s", "Cererea a fost procesata");
    printf("tmp->delay");
    if (sendto(sd, temp_msg, sizeof(temp_msg), 0, (struct sockaddr *)&client, length) <= 0)
    {
      perror("[server]Eroare la sendto() catre client.\n");
      return; /* continuam sa ascultam */
    }
  }
  else if (state == TYPE_REQUEST)
  { //read

    /* adaug mesajul intr-una din listele inlatuite */
    pthread_mutex_lock(get_th_read_mutex(contor));
    read_queue_t *tmp = (read_queue_t *)malloc(sizeof(read_queue_t));
    tmp->client = client;
    strcpy(tmp->msg, catch[0]);
    tmp->sd = sd;
    tmp->next = NULL;
    if (read_queue_vec[contor]->urm->ultim == NULL)
    {
      read_queue_vec[contor]->urm->primul = tmp;
      read_queue_vec[contor]->urm->ultim = tmp;
    }
    else
    {
      read_queue_vec[contor]->urm->ultim->next = tmp;
      read_queue_vec[contor]->urm->ultim = tmp;
    }
    pthread_mutex_unlock(get_th_read_mutex(contor));
    /* dau signal catre thread-ul #contor poate tb trezit */
    pthread_cond_signal(get_th_read_cond(contor));
  }
  else if (state == TYPE_LOGIN) //try to login and communicate results
  {

    char msg_out_login[LENGTH_STR];
    if (cauta_in_fis_db(NUME_FIS_LOGIN, msg_in, msg_out_login))
    {
      //am gasit si trimit msg
      int len = strlen(msg_out_login);

      if (sendto(sd, msg_out_login, len, 0, (struct sockaddr *)&client, length) <= 0)
      {
        perror("[client]Eroare la sendto() spre server.\n");
        exit(errno);
      }
    }
    else
    {
      //nu am gasit si trimit msg
      int len = strlen(msg_out_login);

      if (sendto(sd, msg_out_login, len, 0, (struct sockaddr *)&client, length) <= 0)
      {
        perror("[client]Eroare la sendto() spre server.\n");
        exit(errno);
      }
    }
  }
  else //ignore
  {
    printf("mesaj eronat de la client\n");
  }

  ++contor;
  contor %= NR_THREADS;
}

//start la thread-uri
static void start_threads()
{
  /** Initialize and set:
   * thread detached attribute (Portability)*/
  pthread_attr_t attr;
  if (pthread_attr_init(&attr))
  {
    fprintf(stderr, "main: Nu am putut initializa pthread_attr\n");
    perror("Cauza este");
  }
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
  {
    fprintf(stderr, "main: Nu am putut set detach state for thread attribute\n");
    perror("Cauza este");
  }

  /* Pornesc thread-urile de read */
  for (size_t i = 0; i < NR_THREADS; i++)
  {
    if (pthread_create(&threads[i], &attr, read_info, (void *)i))
    {
      fprintf(stderr, "main: Nu s-a creat thread-ul cu nr %ld\n", i);
      perror("Cauza este");
    }
  }

  /* thread pentru timer */
  if (pthread_create(&threads[0], &attr, announcer_write, (void *)NR_THREADS + 1))
  {
    fprintf(stderr, "main: Nu s-a creat thread-ul cu nr %d\n", 0);
    perror("Cauza este");
  }
  /* thread pentru write */
  if (pthread_create(&threads[0], &attr, write_info, (void *)NR_THREADS))
  {
    fprintf(stderr, "main: Nu s-a creat thread-ul cu nr %d\n", 0);
    perror("Cauza este");
  }

  /* destroy attribute */
  if (pthread_attr_destroy(&attr))
  {
    fprintf(stderr, "main: Nu am putut distruge pthread_attr\n");
    perror("Cauza este");
  }
}

void init()
{
  init_var();
  init_mutexes();
  init_queues();
  start_threads();

  //init xml
  free_trenuri();
  init_data_xml();
}

/* 
 * stabilesc state-ul
 */
static void stab_state(char *msg_in, int *state)
{
  char tmp_msg[LENGTH_STR];
  proc_tip_request(msg_in, tmp_msg);

  if (strcmp(tmp_msg, operations[0]) == 0)
  {
    *state = TYPE_UPDATE;
  }
  else if ((strcmp(tmp_msg, operations[1]) == 0) || (strcmp(tmp_msg, operations[2]) == 0) || (strcmp(tmp_msg, operations[3]) == 0))
  {
    *state = TYPE_REQUEST;
  }
  else if (strcmp(tmp_msg, operations[4]) == 0)
  {
    *state = TYPE_LOGIN;
  }
}

/* 
 * parsare mesaj
 */
static void parse_msg(char *msg, int state)
{
  memset(catch, 0, sizeof(catch));
  int c = 0;
  char tmp[LENGTH_STR];
  strcpy(tmp, msg);
  char *token;
  token = strtok(tmp, DELIM);
  while (token != NULL)
  {
    strcpy(catch[c], token);
    c++;
    if (c == 6)
    {
      return;
    }
    token = strtok(NULL, DELIM);
  }
  return;
}

bool cauta_in_fis_db(char *cale_fis_db, char *msg_in, char *msg_out_login)
{
  int fd;
  struct stat st;
  size_t length_map;
  char *map_addr;

  /* Deschiderea fișierului de date. */
  if (-1 == (fd = open(cale_fis_db, O_RDWR)))
  { /* Tratează cazul de eroare la deschidere. */
    perror("Eroare, nu pot deschide fisierul .. deoarece ");
    exit(2);
  }

  /* obtin stats */
  if (fstat(fd, &st) == -1) /* To obtain file size */
  {
    perror("Error at fstat");
    exit(3);
  }

  length_map = st.st_size;

  bool gasit;

  if (length_map > 0)
  {

    /* mapez si caut */
    map_addr = mmap(NULL,                   // Se va crea o mapare începând de la o adresă page-aligned aleasă de kernel (și returnată în map_addr)
                    length_map,             // Lungimea mapării (de fapt, se alocă multiplu de pagini, dar restul din ultima pagină se umple cu zero-uri)
                    PROT_READ | PROT_WRITE, // Tipul de protecție a mapării: paginile mapării vor permite accese în citire și în scriere
                    MAP_PRIVATE,            // Maparea este partajată (altfel, ca mapare privată, nu se salvează nimic în fișierul de pe disc, la final)
                    fd,                     // Descriptorul de fișier, asociat fișierului ce se mapează în memorie
                    0                       // Offset-ul, de la care începe porțiunea de fișier mapată în memorie, este 0, i.e. BOF
    );
    if (map_addr == MAP_FAILED)
    {
      perror("Error at mmap");
      exit(4);
    }

    /* După crearea mapării, descriptorul de fișier poate fi închis imediat, fără a se invalida maparea ! */
    if (-1 == close(fd))
    {
      perror("Error at close");
      exit(5);
    }

    char user[LENGTH_STR], psw[LENGTH_STR], code[LENGTH_STR];
    memset(user, 0, LENGTH_STR);
    memset(psw, 0, LENGTH_STR);
    memset(code, 0, LENGTH_STR);
    if (parse_first_two(msg_in, user, psw) == true)
    {
      gasit = gaseste_str_in_fis(map_addr, user, psw, code, length_map, msg_out_login);
    }

    if (gasit == true)
    {
      sprintf(msg_out_login, "%s%s%s", AUTH, DELIM, code);
    }
    else
    {
      sprintf(msg_out_login, "%s", NOT_AUTH);
    }

    //"șterg/distrug" maparea creată anterior.
    if (-1 == munmap(map_addr, length_map))
    {
      perror("Error at munmap");
      exit(9);
    }
  }
  else
    gasit = false;

  if (gasit)
    return true;
  else
    return false;
}

bool gaseste_str_in_fis(char *map_addr, char *user, char *psw, char *code, size_t length_map, char *msg_out_login)
{
  bool c_found = false;

  //caut username
  unsigned len_str = strlen(user);
  size_t ch2 = 0;
  size_t ch;
  for (ch = 0; ch < length_map; ++ch, ++map_addr)
  {
    if (*map_addr == user[ch2])
    {
      if (((ch2 + 1) == len_str) && ((ch + 1) == length_map || *(map_addr + sizeof(char)) == ' '))
      {
        c_found = true;
        break;
      }
      else if (ch2 < len_str)
        ++ch2;
      else
        ch2 = 0;
    }
    else
    {
      while (*map_addr != '\n' && ch < length_map)
      {
        ++map_addr;
        ++ch;
      }
      ch2 = 0;
    }
  }
  if (c_found == false)
    return false;

  ++map_addr;
  ++map_addr;
  ch += 2;

  int count = 0;
  int length = strlen(psw);
  //vf daca parola este buna
  if (psw != NULL)
  {

    while (ch < length_map && *map_addr != ' ' && count < length)
    {
      if (*map_addr != psw[count])
      {
        return false;
      }
      map_addr++;
      count++;
      ch++;
    }
    if (count != strlen(psw) || *map_addr != ' ')
    {
      return false;
    }
  }
  else
  {
    while (*map_addr != ' ')
      map_addr++;
    ch++;
  }
  map_addr++;
  ch++;
  //intorc codul
  count = 0;
  while (ch < length_map && *map_addr != '\n')
  {
    code[count] = *map_addr;
    count++;
    map_addr++;
    ch++;
  }

  return true;
}

static void proc_tip_request(char *msg_in, char *tip_request)
{
  char tmp_msg[LENGTH_STR];
  strcpy(tmp_msg, msg_in);
  char *token = strtok(tmp_msg, DELIM);
  if (token != NULL)
    strcpy(tip_request, token);
}

bool parse_first_two(char *str_in, char *user, char *psw)
{
  char tmp_str[LENGTH_STR];
  strcpy(tmp_str, str_in);
  const char delim[2] = "|";
  char *token;
  token = strtok(tmp_str, delim);
  if (token == NULL)
    return false;
  token = strtok(NULL, delim);
  if (token != NULL)
  {
    strcpy(user, token);
  }
  else
  {
    return false;
  }

  token = strtok(NULL, delim);

  if (token != NULL)
  {
    strcpy(psw, token);
  }
  else
  {
    return false;
  }

  token = strtok(NULL, delim);

  if (strlen(user) > 0 && strlen(psw) > 0 && token == NULL)
    return true;

  return false;
}