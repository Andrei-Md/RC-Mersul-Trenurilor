#!/bin/bash -x
gcc -g -I/usr/include/libxml2 -o xml_write.bin `xml2-config --cflags` xml_write.c `xml2-config --libs`