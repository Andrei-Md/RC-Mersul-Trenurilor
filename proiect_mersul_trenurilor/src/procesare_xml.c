
/* 
 * vf local time
 * time_t rawtime;
 * time(&rawtime);
 * time_t seconds;
 * seconds = difftime(tmp_time,time(&rawtime));
 */

/* In this module will process data
 * storage solution: xml (imposed)
 * lib: libxml (code: http://xmlsoft.org/)
*/
#include "procesare_xml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

// #define nr_trenuri 3
// #define nr_plec_sos 3
#define DOC_FILENAME "mersul_trenurilor.xml"
#define NR_TRENURI 30

#undef DEBUGG
// #define DEBUGG
char tren_types[3][256] = {"IR", "R", "IC"};

static void write_info_data(const data_t *data_tg, xmlNodePtr info_data_node, char *name);

/* read xml build cache structure */
static void xml_read(trenuri_t *trenuri);
static xmlDoc *get_dom(char *doc_name);
static void build_tree(xmlNode *root_element, trenuri_t *trenuri);

static void set_data(xmlNode *tmp_node, info_data_t *data, char *str);
static void set_time(xmlNode *tmp_node, data_t *data, char *str);

static void insert_data_sosplec(data_time_t *tren_data_sosire_ef, time_t tmp_time, info_data_t *tmp_info_dta);
static void sortare_vect_time(data_time_t *tren_data);
static void sortare_vect_name(data_name_t *tren_data);

static void create_data_sosplec_struct(trenuri_t *trenuri, data_time_t *tren_data_sosire_ef, data_time_t *tren_data_plecare);
static void create_data_tren_name(trenuri_t *trenuri, data_name_t *tren_data_name);

//primul elem mem nr de elem din vector
data_time_t tren_data_sosire_ef[NR_TRENURI] = {0};
data_time_t tren_data_plecare[NR_TRENURI] = {0};
data_name_t tren_data_name[NR_TRENURI] = {0};
trenuri_t trenuri = {NULL};


/* 
 * Write in xml Database
 */
void xml_write()
{
  xmlDocPtr doc = NULL;                                                                         /* document pointer */
  xmlNodePtr root_node = NULL, tren_node = NULL, info_data_node = NULL, info_data_node2 = NULL; /* node pointers */
  char tmp_buff[256];
  data_t *tmp_data;
  int i, j;

  /** 
   * Creates a new document, a node and set it as a root node
   */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "DateTrenuri");
  xmlDocSetRootElement(doc, root_node);

  /*
     * Creates a DTD (Document Type Definition) declaration. Isn't mandatory. 
     */
  xmlCreateIntSubset(doc, BAD_CAST "DateTrenuri", NULL, BAD_CAST "DateTrenuri.dtd");

  /**
  * Create tree
  */
  tren_t *tmp_tren = trenuri.first;

  while (tmp_tren != NULL)
  {
    /**
     * Create child node <tren>
     */
    tren_node = xmlNewChild(root_node, NULL, BAD_CAST "tren", NULL);

    sprintf(tmp_buff, "%s", tmp_tren->tren_id);
    xmlNewChild(tren_node, NULL, BAD_CAST "trenID", BAD_CAST tmp_buff);
    xmlNewChild(tren_node, NULL, BAD_CAST "trenTip", BAD_CAST tmp_tren->tren_tip);

    info_data_t *tmp_info_data = tmp_tren->info_data_first;
    while (tmp_info_data != NULL)
    {
      info_data_node = xmlNewChild(tren_node, NULL, BAD_CAST "infoData", NULL);
      write_info_data(&tmp_info_data->data_plecare, info_data_node, "dataPlecare");
      write_info_data(&tmp_info_data->data_sosire, info_data_node, "dataSosire");
      write_info_data(&tmp_info_data->data_sosire_ef, info_data_node, "dataSosireEfectiva");
      tmp_info_data = tmp_info_data->next;
    }
    tmp_tren = tmp_tren->next_tren;
  }

  /** 
   * Dumping document to stdio or file
   */
  char xml_file[30] = DOC_FILENAME;
  xmlSaveFormatFileEnc(DOC_FILENAME, doc, "UTF-8", 1);

  /*free the document */
  xmlFreeDoc(doc);

  /**
   * Free the global variables that may
   * have been allocated by the parser.
   */
  xmlCleanupParser();

  /**
   * this is to debug memory for regression tests
   */
  xmlMemoryDump();
}


/* 
 * functio to update tree for each info data
 */
void write_info_data(const data_t *data_tg, xmlNodePtr info_data_node, char *name)
{
  xmlNodePtr info_data_tg_node;
  info_data_tg_node = xmlNewChild(info_data_node, NULL, BAD_CAST name, NULL);

  char tmp_buff[256];
  sprintf(tmp_buff, "%d", data_tg->an);
  xmlNewChild(info_data_tg_node, NULL, BAD_CAST "an", BAD_CAST tmp_buff);
  sprintf(tmp_buff, "%02d", data_tg->luna);
  xmlNewChild(info_data_tg_node, NULL, BAD_CAST "luna", BAD_CAST tmp_buff);
  sprintf(tmp_buff, "%02d", data_tg->zi);
  xmlNewChild(info_data_tg_node, NULL, BAD_CAST "zi", BAD_CAST tmp_buff);
  sprintf(tmp_buff, "%02d", data_tg->ora);
  xmlNewChild(info_data_tg_node, NULL, BAD_CAST "ora", BAD_CAST tmp_buff);
  sprintf(tmp_buff, "%02d", data_tg->min);
  xmlNewChild(info_data_tg_node, NULL, BAD_CAST "minut", BAD_CAST tmp_buff);
}

/* 
 * function to create all data structures from xml file
 */
void init_data_xml()
{
  /* 
   * parse xml and fill trenuri structure
   */
  xml_read(&trenuri);

  create_data_sosplec_struct(&trenuri, tren_data_sosire_ef, tren_data_plecare);
  create_data_tren_name(&trenuri, tren_data_name);
}

/* 
 * set an element with new delay given by user
 */
void set_tbl_delay(char *elem_tip, char *elem_id, int delay)
{

  //traduc elem tip in cod-ul meu
  char new_tip;
  for (int i = 0; i <= NO_TYPES; i++)
  {
    if (strcmp(elem_tip, tren_types[i]) == 0)
    {
      new_tip = i + '0';
      break;
    }
    if (i == NO_TYPES) //nu am gasit nici un tren
      return;
  }

  //caut elem
  string str_to_find;
  memset(str_to_find, 0, LENGTH_S);
  sprintf(str_to_find, "%c%s", new_tip, elem_id);
  int prim = 1;
  int ultim = tren_data_name[0].cnt;
  int middle = (prim + ultim) / 2;
  int pozitie_match = 0;
  while (prim <= ultim)
  {
    if (strcmp(tren_data_name[middle].tip_id, str_to_find) == 0)
    {
      pozitie_match = middle;
      break;
    }
    else if (strcmp(tren_data_name[middle].tip_id, str_to_find) < 0)
    {
      prim = middle + 1;
    }
    else
    {
      ultim = middle - 1;
    }
    middle = (prim + ultim) / 2;
  }
  if (pozitie_match == 0) //Nu am gasit nimic (probabil id-ul nu era bun)
  {
    return;
  }

  //modific valoarea
  //transform in unix time
  struct tm str_timp_temp;
  time_t timp_temp;

  str_timp_temp.tm_mday = tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.zi;
  str_timp_temp.tm_mon = tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.luna;
  str_timp_temp.tm_year = tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.an;
  str_timp_temp.tm_sec = 0;
  str_timp_temp.tm_min = tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.min;
  str_timp_temp.tm_hour = tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.ora;

  //adaug delay
  timp_temp = mktime(&str_timp_temp);
  timp_temp += 60 * delay + 3600 * 2; //adaug 2 ore diferenta de GMT(3600) iar timpul adaugat este convertit in secunde

  //time to data
  string temp_nr;
  struct tm *upd_time = gmtime(&timp_temp);

  tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.zi = upd_time->tm_mday;
  tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.luna = upd_time->tm_mon;
  tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.an = upd_time->tm_year;
  tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.min = upd_time->tm_min;
  tren_data_name[pozitie_match].tren_info->info_data_first->data_sosire_ef.ora = upd_time->tm_hour;
}

/* 
   * make additional structure
   * tren_data_name - sort by TIP+ID
   *    tip - cu prefixe (IR-1, R-2)
   */
void create_data_tren_name(trenuri_t *trenuri, data_name_t *tren_data_name)
{
  tren_data_name[0].cnt = 0;
  tren_t *tmp_tren = trenuri->first;
  string tmp_nume;
  char new_tip;
  while (tmp_tren != NULL)
  {
    //inserez nodul in vector
    tren_data_name[0].cnt++;
    for (int i = 0; i <= NO_TYPES; i++)
    {
      if (strcmp(tmp_tren->tren_tip, tren_types[i]) == 0)
      {
        new_tip = i + '0';
        break;
      }
    }

    sprintf(tren_data_name[tren_data_name[0].cnt].tip_id, "%c%s", new_tip, tmp_tren->tren_id);
    tren_data_name[tren_data_name[0].cnt].tren_info = tmp_tren;

    tmp_tren = tmp_tren->next_tren;
  }
  sortare_vect_name(tren_data_name);
}

/* 
 * sortare vectori dupa data (int)
 * folosit pentru data_plecare, data_sosire
 */
void sortare_vect_name(data_name_t *tren_data)
{
  tren_t *swap_ptr;
  string swap_id;
  for (int i = 1; i < tren_data[0].cnt; i++)
  {
    for (int j = 1; j < tren_data[0].cnt - i + 1; j++)
    {
      if (strcmp(tren_data[j].tip_id, tren_data[j + 1].tip_id) > 0)
      {
        //swap
        strcpy(swap_id, tren_data[j].tip_id);
        swap_ptr = tren_data[j].tren_info;
        strcpy(tren_data[j].tip_id, tren_data[j + 1].tip_id);
        tren_data[j].tren_info = tren_data[j + 1].tren_info;
        strcpy(tren_data[j + 1].tip_id, swap_id);
        tren_data[j + 1].tren_info = swap_ptr;
      }
    }
  }
}

/* 
   * make additional structure
   * tren_data_sosire - sort by data_sosire_ef asc
   * tren_data_plecare - sort by data_plecare asc
   */
void create_data_sosplec_struct(trenuri_t *trenuri, data_time_t *tren_data_sos_ef, data_time_t *tren_data_plec)
{
  memset(tren_data_sos_ef, '\0', sizeof(data_time_t) * NR_TRENURI);
  memset(tren_data_plec, '\0', sizeof(data_time_t) * NR_TRENURI);
  tren_t *tmp_tren = trenuri->first;
  struct tm time_str;
  time_t tmp_time;

  while (tmp_tren != NULL)
  {
    info_data_t *tmp_info_dta = tmp_tren->info_data_first;
    while (tmp_info_dta != NULL)
    {
      //creez structura pentru time_str pentru data sosire efectiva
      time_str.tm_sec = 0;                                       /* Seconds.	[0-60] (1 leap second) */
      time_str.tm_min = tmp_info_dta->data_sosire_ef.min;        /* Minutes.	[0-59] */
      time_str.tm_hour = tmp_info_dta->data_sosire_ef.ora;       /* Hours.	[0-23] */
      time_str.tm_mday = tmp_info_dta->data_sosire_ef.zi;        /* Day.		[1-31] */
      time_str.tm_mon = tmp_info_dta->data_sosire_ef.luna - 1;   /* Month.	[0-11] */
      time_str.tm_year = tmp_info_dta->data_sosire_ef.an - 1900; /* Year	- 1900.  */
      tmp_time = mktime(&time_str);

      //inserez nodul in vector
      insert_data_sosplec(tren_data_sos_ef, tmp_time, tmp_info_dta);

      //creez structura pentru time_str pentru data plecare
      time_str.tm_sec = 0;                                     /* Seconds.	[0-60] (1 leap second) */
      time_str.tm_min = tmp_info_dta->data_plecare.min;        /* Minutes.	[0-59] */
      time_str.tm_hour = tmp_info_dta->data_plecare.ora;       /* Hours.	[0-23] */
      time_str.tm_mday = tmp_info_dta->data_plecare.zi;        /* Day.		[1-31] */
      time_str.tm_mon = tmp_info_dta->data_plecare.luna - 1;   /* Month.	[0-11] */
      time_str.tm_year = tmp_info_dta->data_plecare.an - 1900; /* Year	- 1900.  */
      tmp_time = mktime(&time_str);

      //inserez nodul in vector
      insert_data_sosplec(tren_data_plec, tmp_time, tmp_info_dta);

      tmp_info_dta = tmp_info_dta->next;
    }
    tmp_tren = tmp_tren->next_tren;
  }
  sortare_vect_time(tren_data_sos_ef);
  sortare_vect_time(tren_data_plec);
  return;
}

/* 
 * Inseare nod in vector pentru data_sosire, data_plecare
 */

void insert_data_sosplec(data_time_t *tren_data, time_t tmp_time, info_data_t *tmp_info_dta)
{
  tren_data[0].data++;
  tren_data[tren_data[0].data].data = tmp_time;
  tren_data[tren_data[0].data].tren_info_data = tmp_info_dta;
}

/* 
 * sortare vectori dupa data (int)
 * folosit pentru data_plecare, data_sosire
 */
void sortare_vect_time(data_time_t *tren_data)
{
  info_data_t *swap_ptr;
  time_t swap_time;
  for (int i = 1; i < tren_data[0].data; i++)
  {
    for (int j = 1; j < tren_data[0].data - i + 1; j++)
    {
      if (tren_data[j].data > tren_data[j + 1].data)
      {
        //swap
        swap_time = tren_data[j].data;
        swap_ptr = tren_data[j].tren_info_data;
        tren_data[j].data = tren_data[j + 1].data;
        tren_data[j].tren_info_data = tren_data[j + 1].tren_info_data;
        tren_data[j + 1].data = swap_time;
        tren_data[j + 1].tren_info_data = swap_ptr;
      }
    }
  }
}

/* 
 * functie starter citire doc xml
 */
void xml_read(trenuri_t *trenuri)
{
  xmlNode *root_element = NULL;
  xmlDoc *xml_doc = NULL;

  xml_doc = get_dom(DOC_FILENAME);

  /* Get the root element node */
  root_element = xmlDocGetRootElement(xml_doc);

  build_tree(root_element, trenuri);

  /*free the document */
  xmlFreeDoc(xml_doc);

  /*
   *Free the global variables that may
   *have been allocated by the parser.
   */
  xmlCleanupParser();
}

/* 
 * get Document Object Model
 */
xmlDoc *get_dom(char *doc_name)
{
  xmlDoc *xml_dom = NULL;
  xml_dom = xmlReadFile(DOC_FILENAME, NULL, 0);

  if (xml_dom == NULL)
  {
    perror("[server]Eroare la socket().\n");
    exit(1);
  }

  return xml_dom;
}

/* 
 * Construiesc arborele in care stochez datele din xml
 */
void build_tree(xmlNode *xml_node, trenuri_t *trenuri)
{
  xmlNode *tmp_node = NULL;
  tmp_node = xml_node;
  while (tmp_node != NULL)
  {
    if (tmp_node->type == XML_ELEMENT_NODE)
    {
#ifdef DEBUGG
      printf("name-%s: -", tmp_node->name);
#endif
      if (strcmp(tmp_node->name, "tren") == 0)
      {
        //adaug un tren nou
        tren_t *tren_nou = (tren_t *)malloc(sizeof(tren_t));
        tren_nou->next_tren = NULL;
        tren_nou->info_data_first = NULL;
        tren_nou->info_data_last = NULL;
        if (trenuri->last == NULL)
        {
          trenuri->first = tren_nou;
        }
        else
        {
          trenuri->last->next_tren = tren_nou;
        }
        trenuri->last = tren_nou;
#ifdef DEBUGG
        printf(">>>>>>>tren<<<\n");
#endif
      }
      else if (strcmp(tmp_node->name, "trenID") == 0)
      {
        strcpy(trenuri->last->tren_id, tmp_node->children->content);
#ifdef DEBUGG
        printf("trenID\n");
#endif
      }
      else if (strcmp(tmp_node->name, "trenTip") == 0)
      {
        strcpy(trenuri->last->tren_tip, tmp_node->children->content);
#ifdef DEBUGG
        printf("trenTip\n");
#endif
      }
      else if (strcmp(tmp_node->name, "infoData") == 0)
      {
        //adaug infodata noua
        info_data_t *info_data_nou = (info_data_t *)malloc(sizeof(info_data_t));
        info_data_nou->next = NULL;
        info_data_nou->tren_parinte = trenuri->last;
        if (trenuri->last->info_data_last == NULL)
        {
          trenuri->last->info_data_first = info_data_nou;
        }
        else
        {
          trenuri->last->info_data_last->next = info_data_nou;
        }
        trenuri->last->info_data_last = info_data_nou;
#ifdef DEBUGG
        printf("infoData\n");
#endif
      }
      else if (strcmp(tmp_node->name, "dataPlecare") == 0)
      {
#ifdef DEBUGG
        printf("dataPlecare\n");
#endif
      }
      else if (strcmp(tmp_node->name, AN) == 0)
      {
        set_data(tmp_node, trenuri->last->info_data_last, AN);
#ifdef DEBUGG
        printf("an\n");
#endif
      }
      else if (strcmp(tmp_node->name, LUNA) == 0)
      {
        set_data(tmp_node, trenuri->last->info_data_last, LUNA);
#ifdef DEBUGG
        printf("luna\n");
#endif
      }
      else if (strcmp(tmp_node->name, ZI) == 0)
      {
        set_data(tmp_node, trenuri->last->info_data_last, ZI);
#ifdef DEBUGG
        printf("zi\n");
#endif
      }
      else if (strcmp(tmp_node->name, ORA) == 0)
      {
        set_data(tmp_node, trenuri->last->info_data_last, ORA);
#ifdef DEBUGG
        printf("ora\n");
#endif
      }
      else if (strcmp(tmp_node->name, MINUT) == 0)
      {
        set_data(tmp_node, trenuri->last->info_data_last, MINUT);
#ifdef DEBUGG
        printf("minut\n");
#endif
      }
      else
      {
#ifdef DEBUGG
        printf("gol\n");
#endif
      }
#ifdef DEBUGG
      printf("node content: %s\n", tmp_node->children->content);
#endif
    }
    build_tree(tmp_node->children, trenuri);
    tmp_node = tmp_node->next;
  }
}

/* 
 * set info_data nodes
 */
void set_data(xmlNode *tmp_node, info_data_t *data, char *str)
{
  if (strcmp(tmp_node->parent->name, "dataPlecare") == 0)
  {
    set_time(tmp_node, &(data->data_plecare), str);
  }
  else if (strcmp(tmp_node->parent->name, "dataSosire") == 0)
  {
    set_time(tmp_node, &(data->data_sosire), str);
  }
  else if (strcmp(tmp_node->parent->name, "dataSosireEfectiva") == 0)
  {
    set_time(tmp_node, &(data->data_sosire_ef), str);
  }
  return;
}

/* 
 * set time in info_data according to str
 */
void set_time(xmlNode *tmp_node, data_t *data, char *str)
{
  if (strcmp(str, AN) == 0)
  {
    data->an = atoi(tmp_node->children->content);
  }
  else if (strcmp(str, LUNA) == 0)
  {
    data->luna = atoi(tmp_node->children->content);
  }
  else if (strcmp(str, ZI) == 0)
  {
    data->zi = atoi(tmp_node->children->content);
  }
  else if (strcmp(str, ORA) == 0)
  {
    data->ora = atoi(tmp_node->children->content);
  }
  else if (strcmp(str, MINUT) == 0)
  {
    data->min = atoi(tmp_node->children->content);
  }
  return;
}

/* 
 * functia de free memory
 */
void free_trenuri()
{
  while (trenuri.first != NULL)
  {
    while (trenuri.first->info_data_first != NULL)
    {
      info_data_t *tmp_infodta;
      tmp_infodta = trenuri.first->info_data_first;
      trenuri.first->info_data_first = trenuri.first->info_data_first->next;
      free(tmp_infodta);
    }
    tren_t *tmp_tren = trenuri.first;
    trenuri.first = trenuri.first->next_tren;
    free(tmp_tren);
  }
  trenuri.first = NULL;
  trenuri.last = NULL;
  return;
}

/* 
 * functie pentru returnarea tabelei cutrenurile din ziua respesctiva
 */
char *get_tbl_zi_cur()
{
  data_time_t *tren_data = tren_data_plecare;
  char *tbl_zi_cur;
  char tmp_str[1024] = {0};

  //local time in this moment
  time_t local = time(NULL);
  if (local == -1)
  {
    perror("time function fail.\n");
    exit(1);
  }
  //put local time in struct
  struct tm *local_time = localtime(&local);

  time_t first_time, second_time;
  struct tm first, second;
  first.tm_mday = local_time->tm_mday; /* Day.		[1-31] */
  first.tm_mon = local_time->tm_mon;   /* Month.	[0-11] */
  first.tm_year = local_time->tm_year; /* Year	- 1900.  */
  first.tm_sec = 0;
  first.tm_min = 0;
  first.tm_hour = 1;

  first_time = mktime(&first);

  second.tm_mday = local_time->tm_mday + 1;
  second.tm_mon = local_time->tm_mon;
  second.tm_year = local_time->tm_year;
  second.tm_sec = 0;
  second.tm_min = 0;
  second.tm_hour = 1;

  second_time = mktime(&second);

  struct tm str_timp_sos, str_timp_sosef;
  time_t timp_sos, timp_sosef;

  int intarziere;
  int prim = 1;
  int ultim = tren_data[0].data;
  int middle = (prim + ultim) / 2;
  int pozitie_first_match = 0;
  while (prim <= ultim)
  {
    if (tren_data[middle].data >= first_time && tren_data[middle].data <= second_time)
    {
      pozitie_first_match = middle;
      break;
    }
    else if (tren_data[middle].data < first_time)
    {
      prim = middle + 1;
    }
    else
    {
      ultim = middle - 1;
    }
    middle = (prim + ultim) / 2;
  }

  if (pozitie_first_match != 0)
  {
    while (tren_data[pozitie_first_match].data >= first_time)
    {
      pozitie_first_match--;
    }
  }
  else
  { //Nu am gasit nimic
    return NULL;
  }

  int len, len2;
  pozitie_first_match++;
  len = strlen(tmp_str);
  sprintf(tmp_str + len, "%s %d.%02d.%d", " Mersul Trenurilor - program astazi: ", local_time->tm_mday, (local_time->tm_mon + 1), (local_time->tm_year + 1900));
  len = strlen(tmp_str);
  sprintf(tmp_str + len, "%s", "\n_______________________________________________________________\n");
  len = strlen(tmp_str);
  tbl_zi_cur = (char *)calloc(len + 2, sizeof(char));
  strcpy(tbl_zi_cur, tmp_str);

  sprintf(tmp_str, "%s", "|Tip Tren | ID Tren | Data plecare | Data sosire | Intarziere |\n");
  len = strlen(tmp_str);
  len2 = strlen(tbl_zi_cur);
  tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2 + 1) * sizeof(char));
  strcat(tbl_zi_cur, tmp_str);

  for (size_t i = pozitie_first_match; i <= tren_data[0].data && tren_data[i].data <= second_time; i++)
  {
    str_timp_sos.tm_mday = tren_data[i].tren_info_data->data_sosire.zi;
    str_timp_sos.tm_mon = tren_data[i].tren_info_data->data_sosire.luna;
    str_timp_sos.tm_year = tren_data[i].tren_info_data->data_sosire.an;
    str_timp_sos.tm_sec = 0;
    str_timp_sos.tm_min = tren_data[i].tren_info_data->data_sosire.min;
    str_timp_sos.tm_hour = tren_data[i].tren_info_data->data_sosire.ora;

    str_timp_sosef.tm_mday = tren_data[i].tren_info_data->data_sosire_ef.zi;
    str_timp_sosef.tm_mon = tren_data[i].tren_info_data->data_sosire_ef.luna;
    str_timp_sosef.tm_year = tren_data[i].tren_info_data->data_sosire_ef.an;
    str_timp_sosef.tm_sec = 0;
    str_timp_sosef.tm_min = tren_data[i].tren_info_data->data_sosire_ef.min;
    str_timp_sosef.tm_hour = tren_data[i].tren_info_data->data_sosire_ef.ora;

    timp_sos = mktime(&str_timp_sos);
    timp_sosef = mktime(&str_timp_sosef);
    intarziere = (timp_sosef - timp_sos) / 60;

    sprintf(tmp_str, "| %5s   |  %5s  |    %02d:%02d     |    %02d:%02d    |%7d     |\n",
            tren_data[i].tren_info_data->tren_parinte->tren_tip, tren_data[i].tren_info_data->tren_parinte->tren_id,
            tren_data[i].tren_info_data->data_plecare.ora, tren_data[i].tren_info_data->data_plecare.min,
            tren_data[i].tren_info_data->data_sosire_ef.ora, tren_data[i].tren_info_data->data_sosire_ef.min,
            intarziere);
    len = strlen(tmp_str);
    len2 = strlen(tbl_zi_cur);
    tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2 + 1) * sizeof(char));
    strcat(tbl_zi_cur, tmp_str);
  }
  sprintf(tmp_str, "%s", "---------------------------------------------------------------\n");
  len = strlen(tmp_str);
  len2 = strlen(tbl_zi_cur);
  tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2) * sizeof(char));
  strcat(tbl_zi_cur, tmp_str);
  return tbl_zi_cur;
}

/* 
 * functie ce returneaza sosirile sau plecarile din urmatoarea ora in functie de tren_data pasat
 */
char *get_tbl_1h(char *str)
{
  char str_tip[LENGTH_S];
  data_time_t *tren_data;
  if (strcmp(str, REQ_ARRIVAL) == 0)
  {
    tren_data = tren_data_sosire_ef;
    strcpy(str_tip, "Sosire");
  }
  else if (strcmp(str, REQ_DEPARTURE) == 0)
  {
    tren_data = tren_data_plecare;
    strcpy(str_tip, "Plecare");
  }
  else
  {
#ifdef DEBUGG
    printf("aici nu e bun tren_data\n");
#endif
    return NULL;
  }

  char *tbl_zi_cur;
  char tmp_str[1024] = {0};

  //local time in this moment
  time_t local = time(NULL);
  if (local == -1)
  {
    perror("time function fail.\n");
    return NULL;
  }
  //put local time in struct
  struct tm *local_time = localtime(&local);
#ifdef DEBUGG
  printf("local-> %ld\n", local);
#endif

  time_t first_time;
  time_t second_time;
  struct tm first_str, second_str;
  first_str.tm_mday = local_time->tm_mday; /* Day.		[1-31] */
  first_str.tm_mon = local_time->tm_mon;   /* Month.	[0-11] */
  first_str.tm_year = local_time->tm_year; /* Year	- 1900.  */
  first_str.tm_sec = 0;
  first_str.tm_min = local_time->tm_min;
  first_str.tm_hour = local_time->tm_hour + 1;

  second_str.tm_mday = local_time->tm_mday;
  second_str.tm_mon = local_time->tm_mon;
  second_str.tm_year = local_time->tm_year;
  second_str.tm_sec = 0;
  second_str.tm_min = local_time->tm_min;
  second_str.tm_hour = local_time->tm_hour + 1;

  first_time = mktime(&first_str);
  second_time = first_time + 60 * 60; //mktime(&second_str);

  struct tm str_timp_sos, str_timp_sosef;
  time_t timp_sos, timp_sosef;

  int intarziere;
  int prim = 1;
  int ultim = tren_data[0].data;
  int middle = (prim + ultim) / 2;
  int pozitie_first_match = 0;
#ifdef DEBUGG
  printf("f-> %ld, s-> %ld\n", first_time, second_time);
  printf("f1-> %ld, s1-> %ld\n", tren_data[1].data, tren_data[tren_data[0].data].data);
#endif
  while (prim <= ultim)
  {
    if (tren_data[middle].data >= first_time && tren_data[middle].data <= second_time)
    {
      pozitie_first_match = middle;
      break;
    }
    else if (tren_data[middle].data < first_time)
    {
      prim = middle + 1;
    }
    else
    {
      ultim = middle - 1;
    }
    middle = (prim + ultim) / 2;
  }

  if (pozitie_first_match != 0)
  {
    while (tren_data[pozitie_first_match].data >= first_time)
    {
      pozitie_first_match--;
    }
  }
  else
  { //Nu am gasit nimic
#ifdef DEBUGG
    printf("nu am gasit nimic??\n");
#endif
    return NULL;
  }

  pozitie_first_match++;
  int len, len2;
  len = strlen(tmp_str);
  sprintf(tmp_str + len, "%s - %s intre ora: %02d:%02d si %02d:%02d", " Mersul Trenurilor ", str_tip, first_str.tm_hour, first_str.tm_min, second_str.tm_hour, second_str.tm_min);
  len = strlen(tmp_str);
  sprintf(tmp_str + len, "%s", "\n_______________________________________________________________\n");
  len = strlen(tmp_str);
  tbl_zi_cur = (char *)calloc(len + 10, sizeof(char));
  strcpy(tbl_zi_cur, tmp_str);

  sprintf(tmp_str, "%s", "|Tip Tren | ID Tren | Data plecare | Data sosire | Intarziere |\n");
  len = strlen(tmp_str);
  len2 = strlen(tbl_zi_cur);
  tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2 + 1) * sizeof(char));
  strcat(tbl_zi_cur, tmp_str);

  for (size_t i = pozitie_first_match; i <= tren_data[0].data && tren_data[i].data <= second_time; i++)
  {
    str_timp_sos.tm_mday = tren_data[i].tren_info_data->data_sosire.zi;
    str_timp_sos.tm_mon = tren_data[i].tren_info_data->data_sosire.luna;
    str_timp_sos.tm_year = tren_data[i].tren_info_data->data_sosire.an;
    str_timp_sos.tm_sec = 0;
    str_timp_sos.tm_min = tren_data[i].tren_info_data->data_sosire.min;
    str_timp_sos.tm_hour = tren_data[i].tren_info_data->data_sosire.ora;

    str_timp_sosef.tm_mday = tren_data[i].tren_info_data->data_sosire_ef.zi;
    str_timp_sosef.tm_mon = tren_data[i].tren_info_data->data_sosire_ef.luna;
    str_timp_sosef.tm_year = tren_data[i].tren_info_data->data_sosire_ef.an;
    str_timp_sosef.tm_sec = 0;
    str_timp_sosef.tm_min = tren_data[i].tren_info_data->data_sosire_ef.min;
    str_timp_sosef.tm_hour = tren_data[i].tren_info_data->data_sosire_ef.ora;

    timp_sos = mktime(&str_timp_sos);
    timp_sosef = mktime(&str_timp_sosef);
    intarziere = (timp_sosef - timp_sos) / 60;

    sprintf(tmp_str, "| %5s   |  %5s  |    %02d:%02d     |    %02d:%02d    |%7d     |\n",
            tren_data[i].tren_info_data->tren_parinte->tren_tip, tren_data[i].tren_info_data->tren_parinte->tren_id,
            tren_data[i].tren_info_data->data_plecare.ora, tren_data[i].tren_info_data->data_plecare.min,
            tren_data[i].tren_info_data->data_sosire_ef.ora, tren_data[i].tren_info_data->data_sosire_ef.min,
            intarziere);
    len = strlen(tmp_str);
    len2 = strlen(tbl_zi_cur);
    tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2 + 1) * sizeof(char));
    strcat(tbl_zi_cur, tmp_str);
  }
  sprintf(tmp_str, "%s", "---------------------------------------------------------------\n");
  len = strlen(tmp_str);
  len2 = strlen(tbl_zi_cur);
  tbl_zi_cur = (char *)realloc(tbl_zi_cur, (len + len2 + 2) * sizeof(char));
  strcat(tbl_zi_cur, tmp_str);
#ifdef DEBUGG
  printf("%s\n", tbl_zi_cur);
#endif
  return tbl_zi_cur;
}

/* 
 * functie de print pentru debug
 */
void print_func(trenuri_t trenuri)
{
  tren_t *tmp_tren = trenuri.first;

  int intarziere;
  printf(" Mersul Trenurilor \n");
  printf("_______________________________________________________________\n");
  printf("|Tip Tren | ID Tren | Data plecare | Data sosire | Intarziere |\n");
  struct tm str_timp_sos, str_timp_sosef;
  time_t timp_sos, timp_sosef;
  while (tmp_tren != NULL)
  {
    info_data_t *tmp_info_dta = tmp_tren->info_data_first;
    while (tmp_info_dta != NULL)
    {
      str_timp_sos.tm_mday = tmp_info_dta->data_sosire.zi;
      str_timp_sos.tm_mon = tmp_info_dta->data_sosire.luna;
      str_timp_sos.tm_year = tmp_info_dta->data_sosire.an;
      str_timp_sos.tm_sec = 0;
      str_timp_sos.tm_min = tmp_info_dta->data_sosire.min;
      str_timp_sos.tm_hour = tmp_info_dta->data_sosire.ora;

      str_timp_sosef.tm_mday = tmp_info_dta->data_sosire_ef.zi;
      str_timp_sosef.tm_mon = tmp_info_dta->data_sosire_ef.luna;
      str_timp_sosef.tm_year = tmp_info_dta->data_sosire_ef.an;
      str_timp_sosef.tm_sec = 0;
      str_timp_sosef.tm_min = tmp_info_dta->data_sosire_ef.min;
      str_timp_sosef.tm_hour = tmp_info_dta->data_sosire_ef.ora;

      timp_sos = mktime(&str_timp_sos);
      timp_sosef = mktime(&str_timp_sosef);
      intarziere = (timp_sosef - timp_sos) / 60;

      printf("| %5s   |  %5s  |    %02d:%02d     |    %02d:%02d    |%7d     |\n",
             tmp_info_dta->tren_parinte->tren_tip, tmp_info_dta->tren_parinte->tren_id,
             tmp_info_dta->data_plecare.ora, tmp_info_dta->data_plecare.min,
             tmp_info_dta->data_sosire_ef.ora, tmp_info_dta->data_sosire_ef.min,
             intarziere);
      tmp_info_dta = tmp_info_dta->next;
    }
    tmp_tren = tmp_tren->next_tren;
  }
  printf("---------------------------------------------------------------\n");
}