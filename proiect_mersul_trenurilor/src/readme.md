# Mersul Trenurilor

Protocolul de comunicatie utilizat la nivel transport este UDP 

## Comenzi:
 - trimitere de la server cu informatii despre mersul trenurilor in ziua respectiva
 - trimitere de la server cu informatii despre plecari in urmatoarea ora (conform cu planul, in intarziere cu x min) doar la cererea unui client
 - trimitere de la server cu informatii despre sosiri in urmatoarea ora (conform cu planul, in intarziere cu x min, cu x min mai devreme) doar la cererea unui client
 - actualizarea datelor la cerere (intarzierea): e.g. 
 - (q)uit


## Compilare 
use `cmake.sh`
OR
```
gcc -g -c -o main_server.o main_server.c 
gcc -g -c -o procesare_info.o procesare_info.c 
gcc -g -c -o procesare_if.o procesare_if.c 
gcc -g -c -o procesare_xml.o `xml2-config --cflags` procesare_xml.c `xml2-config --libs`
gcc -g -o server.bin `xml2-config --cflags` main_server.o procesare_info.o procesare_if.o procesare_xml.o -pthread `xml2-config --libs`

gcc -DDEBUG -g main_client.c -o main_client.bin -g

gcc -g -I/usr/include/libxml2 -o xml_write.bin `xml2-config --cflags` xml_write.c `xml2-config --libs`
```

## Rulare
### Server:
./server.bin (asteptati 1 secunda pentru a se initializa)

### Client:
running:
  ./main_client.bin <adresa_server> <port>
eg:
  ./main_client.bin 127.0.0.1 2801

login -> scrie un credential din fisierul auth.txt (nume parola)
      e.g.: andrei psw
1. mersul trenurilor pentru astazi
2. informatii plecari trenuri in urmatoarea ora
3. informatii sosiri trenuri in urmatoarea ora
4. actualizare informatii referitoare la trenuri in urmatoarea ora 
   - Introduceti tipul si id-ul trenului urmat de intarziere 
     `<TREN TIP> <TREN ID> <INTARZIERE>` e.g.: `R 2 30`


quit -> apasa q oricand pentru a te deloga. Daca esti logat vei fi delogat avand posibilitate sa te autentifici cu alt username.

### XML
Se poate utiliza xml_write.bin pentru a genera un nou xml cu datele din program. Acestea pot fi modificate mai usor decat direct in xml.
