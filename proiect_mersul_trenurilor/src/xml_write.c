/* In this module will process data
 * storage solution: xml (imposed)
*/
#include "xml_write.h"

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


#define nr_trenuri 14
#define nr_plec_sos 1
#define DOC_FILENAME "mersul_trenurilor.xml"

void print_func(tren_t *);
void xml_write(const tren_t *);
void write_info_data(const data_t *data_tg, xmlNodePtr info_data_node, char *name);

tren_t* trenuri_noi;

int main()
{
 

  // tren_t trenuri[nr_trenuri] = {
  //     {11234, "IR", {{{2021, 01, 11, 00, 20}, {2021, 01, 11, 00, 30}, {2021, 01, 11, 00, 40}},
  //                    {{2021, 01, 11, 00, 10}, {2021, 01, 11, 00, 20}, {2021, 01, 11, 00, 50}}, 
  //                    {{2021, 01, 11, 00, 30}, {2021, 01, 11, 00, 50}, {2021, 01, 11, 00, 40}}}},
  //     {2, "R", {{{2020, 11, 26, 02, 20}, {2020, 11, 26, 03, 50}, {2020, 11, 26, 03, 40}},
  //               {{2020, 11, 26, 03, 40}, {2020, 11, 26, 04, 30}, {2020, 11, 26, 05, 10}},
  //               {{2020, 11, 26, 06, 40}, {2020, 11, 26, 07, 20}, {2020, 11, 26, 07, 20}}}},
  //     {3, "R", {{{2021, 01, 10, 02, 50}, {2021, 01, 10, 03, 50}, {2021, 01, 10, 03, 40}},
  //               {{2021, 01, 10, 03, 50}, {2021, 01, 10, 04, 40}, {2021, 01, 10, 05, 10}},
  //               {{2021, 01, 10, 04, 50}, {2021, 01, 10, 06, 10}, {2021, 01, 10, 06, 10}}}},
  //     {1, "R", {{{2021, 01, 11, 02, 50}, {2021, 01, 02, 03, 50}, {2021, 01, 11, 00, 40}},
  //               {{2020, 01, 02, 03, 50}, {2021, 01, 02, 04, 40}, {2021, 01, 02, 05, 10}},
  //               {{2020, 01, 02, 04, 50}, {2021, 01, 02, 06, 10}, {2021, 01, 02, 06, 10}}}}
  //     };
   /*
  * declare dummy data
  */
  tren_t trenuri[nr_trenuri] = {
      {3, "IR", {{{2021, 01, 11, 12, 40}, {2021, 01, 11, 13, 20}, {2021, 01, 11, 13, 20}}}},
      {2, "R", {{{2021, 01, 13, 23, 20}, {2021, 01, 13, 03, 50}, {2021, 01, 13, 03, 40}}}},
      {3, "R", {{{2021, 01, 13, 23, 40}, {2021, 01, 13, 03, 50}, {2021, 01, 13, 04, 40}}}},
      {4, "R", {{{2021, 01, 13, 02, 50}, {2021, 01, 13, 03, 50}, {2021, 01, 13, 03, 40}}}},
      {5, "R", {{{2021, 01, 13, 12, 50}, {2021, 01, 13, 12, 50}, {2021, 01, 13, 12, 40}}}},
      {6, "R", {{{2021, 01, 13, 13, 50}, {2021, 01, 13, 13, 50}, {2021, 01, 13, 13, 40}}}},
      {7, "R", {{{2021, 01, 13, 14, 50}, {2021, 01, 13, 14, 50}, {2021, 01, 13, 14, 50}}}},
      {32, "IC", {{{2021, 01, 13, 10, 05}, {2021, 01, 13, 12, 10}, {2021, 01, 13, 13, 55}}}},
      {31, "IC", {{{2021, 01, 13, 10, 40}, {2021, 01, 13, 12, 10}, {2021, 01, 13, 13, 55}}}},
      {30, "IC", {{{2021, 01, 13, 10, 30}, {2021, 01, 13, 12, 10}, {2021, 01, 13, 13, 55}}}},
      {8, "R", {{{2021, 01, 13, 12, 30}, {2021, 01, 13, 13, 10}, {2021, 01, 13, 13, 45}}}},
      {9, "R", {{{2021, 01, 13, 13, 00}, {2021, 01, 13, 13, 20}, {2021, 01, 13, 13, 30}}}},
      {2, "IR", {{{2021, 01, 13, 22, 45}, {2021, 01, 12, 23, 00}, {2021, 01, 12, 23, 10}}}},
      {1, "IR", {{{2021, 01, 13, 22, 45}, {2021, 01, 12, 23, 00}, {2021, 01, 12, 22, 50}}}}
};
  xml_write(trenuri);


  print_func(trenuri);
  return 0;
}


void xml_write(const tren_t *trenuri)
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
  for (size_t i = 0; i < nr_trenuri; i++)
  {
    /**
     * Create child node <tren>
     */
    tren_node = xmlNewChild(root_node, NULL, BAD_CAST "tren", NULL);

    sprintf(tmp_buff, "%d", trenuri[i].tren_id);
    xmlNewChild(tren_node, NULL, BAD_CAST "trenID", BAD_CAST tmp_buff);
    xmlNewChild(tren_node, NULL, BAD_CAST "trenTip", BAD_CAST trenuri[i].tren_tip);

    for (size_t j = 0; j < nr_plec_sos; j++)
    {
      info_data_node = xmlNewChild(tren_node, NULL, BAD_CAST "infoData", NULL);
      write_info_data(&trenuri[i].info_date[j].data_plecare, info_data_node, "dataPlecare");
      write_info_data(&trenuri[i].info_date[j].data_sosire, info_data_node, "dataSosire");
      write_info_data(&trenuri[i].info_date[j].data_sosire_ef, info_data_node, "dataSosireEfectiva");
    }
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

void print_func(tren_t *trenuri)
{
  int intarziere;
  printf(" Mersul Trenurilor \n");
  printf("______________________________________________________________\n");
  printf("|Tip Tren | ID Tren | Data plecare | Data sosire | Intarziere|\n");
  for (size_t i = 0; i < nr_trenuri; i++)
    for (size_t j = 0; j < nr_plec_sos; j++)
    {
      intarziere = (trenuri[i].info_date[j].data_sosire_ef.ora - trenuri[i].info_date[j].data_sosire.ora) * 60 +
                   (trenuri[i].info_date[j].data_sosire_ef.min - trenuri[i].info_date[j].data_sosire.min);
      printf("| %-5s   |  %-5d  |    %02d:%02d     |    %02d:%02d    |    %-3d    |\n",
             trenuri[i].tren_tip, trenuri[i].tren_id,
             trenuri[i].info_date[j].data_plecare.ora, trenuri[i].info_date[j].data_plecare.min,
             trenuri[i].info_date[j].data_sosire_ef.ora, trenuri[i].info_date[j].data_sosire_ef.min,
             intarziere);
    }
  printf("--------------------------------------------------------------\n");
}