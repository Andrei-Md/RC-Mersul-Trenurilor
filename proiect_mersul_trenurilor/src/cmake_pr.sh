#!/bin/bash -x
# gcc -DDEBUG -g process_data.c -o process_data.bin -g

gcc -g -c -o main_server.o main_server.c 
gcc -g -c -o procesare_info.o procesare_info.c 
gcc -g -c -o procesare_if.o procesare_if.c 
gcc -g -c -o procesare_xml.o `xml2-config --cflags` procesare_xml.c `xml2-config --libs`
gcc -g -o server.bin `xml2-config --cflags` main_server.o procesare_info.o procesare_if.o procesare_xml.o -pthread `xml2-config --libs`

gcc -DDEBUG -g main_client.c -o main_client.bin -g

gcc -g -I/usr/include/libxml2 -o xml_write.bin `xml2-config --cflags` xml_write.c `xml2-config --libs`

