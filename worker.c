/*

O Manager epikoinwnei me tous workers mesw named pipes.
Gia kathe onoma arxeiou pou lamvanei o manager apo ton 
listener tha eidopoiei h tha dhmiourgei mia diergasia 
worker, h opoia energei panw sto sugkekrimeno arxeio. O
kathe worker analamvanei na epexergastei ena arxeio th fora.

Skopos tou worker: na anoixei to arxeio kai na anazhthsei urls mesw low-level I/O. Ta arxeia keimenou mporei na 
periexoun aplo keimeno kai urls. H anazhthsh periorizetai sta urls pou xrhsimopoioun to prwtokollo http, dhl ths
morfhs http://... .Pio sugkekrimena, to kathe url xekinaeime http:// kai teleiwnei me enan keno xarakthra.
Kata th diarkeia ths anagnwshs tou arxeiou o worker dhmiourgei ena neo arxeio sto opoio katagrafei ola ta locations pou
anixneuse mazi me ton arithmo emfanishs tous. Px an sto arxeio  pou prostethike emfanizontai 3 urlsme location "di.uoa.gr"
to arxeio exodou tou worker tha periexei th grammh "di.uoa.gr" 3 kai antistoixa mia grammh gia kathe allo location
An to arxeio pou diavase o worker exei onoma <filename>, to arxeio pou dhmiourgei exei onoma <filename>.out

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "include/ADTList.h"

#define PERMS 0666 
#define BUFFSIZE 16384

int main(int argc, char* argv[]){
    int fd, fd2, fd3, line, counter, readfd;
    ssize_t bytes;
    char buffer[BUFFSIZE];
    char filename[BUFFSIZE];
    List list;

    char* pipeName = argv[1];

    while(1){
        list = list_create(free);  // list for urls
        printf("Worker opening named pipe %s \n", pipeName);

        /* Open named pipe to get filename */
        if((readfd = open(pipeName, O_RDONLY)) < 0){
            perror("Worker cant open read fifo");
            exit(1);
        }
        if((bytes=read(readfd, filename, BUFFSIZE)) <= 0){
            perror("Worker: filename read error");
            exit(2);
        }
        filename[bytes] = '\0';

        close(readfd);
        
        if((fd = open(filename, O_RDONLY, PERMS)) == -1){
            perror("opening");
            exit(1);
        }

        int count = 0;
        printf("Worker %d starting to read file %s\n", getpid(), filename);
        while((bytes = read(fd, buffer, BUFFSIZE)) > 0){
            char* p = buffer;
            char* p2 = malloc(sizeof(char)*strlen(buffer)+1);
            char* url;
            char* location = malloc(sizeof(char)*strlen(buffer)+1);
            char* p3 = malloc(sizeof(char)*strlen(location)+1);
            int index;
            char* c;
            
            /* FINDING THE URL's */
            while((p = strstr(p, "http://")) != NULL){
                count++;    // just a counter for urls in this file

                /* Get string that starts with "http://" and then remove the "http://"-part */
                strcpy(p2, p);
                url = strtok(p2, " ");  // we now have our url 
                // printf("url is %s \n", url);
                strcpy(location, p2);
                memmove(location, location + 7, strlen(location) - 6);

                /* Check if string starts with "www.". If yes copy string without the first 4 bytes ("www.") */
                if(strstr(location, "www.") == location){
                    memmove(location, location + 4, strlen(location)-3);
                }

                /* Find if our string has "/" in it. If yes then replace it with end of string, else we got our location */
                if((c = strchr(location, '/')) != NULL){
                    index = (int)(c - location);
                    location[index] = '\0';
                }
                // printf("with location: %s \n", location);
                
                /* Inserting the url in list */
                char* newloc = malloc(strlen(location)+1);
                strcpy(newloc, location);
                list_insert_next(list, LIST_BOF, newloc);

                p++;
            }
            free(p2); free(p3);
            free(location);
            
            // printf("urls found in this file %d \n", count);
        }
        close(fd);
        
        /* Remove path from filename */
        /* Find the last occurence of / in filename */
        char* ptr = strrchr(filename, '/');
        int len = strlen(filename);
        int pos = len-strlen(ptr);
        memmove(filename, filename + pos + 1, len - pos + 1);
        
        /* Find the .txt in filename and replace it with .out so we have our <filename>.out file */
        char* dot;
        char* out = ".out";
        int ind;
        if((dot = strchr(filename, '.')) != NULL){
            ind = (int)(dot - filename);
            filename[ind] = '\0';    
        }
        strcat(filename, out);
        printf("Output filename is %s \n", filename);

        /* Output files are created in folder output/ */
        char* path = malloc(sizeof(char)*(strlen(filename)+strlen("output/")+1));
        strcpy(path, "output/");
        strcat(path, filename);
        //printf("opening -> %s\n", path);
        if((fd2 = open(path, O_CREAT|O_WRONLY|O_APPEND, PERMS)) == -1){
            perror("open");
            exit(1);
        }
        
        int i=0;
        int length, byte;
        char* value;
        char* string_to_write;
        char* cnt;
        for (ListNode node = list->dummy->next; node != NULL; node = node->next){
            /* get value from node */
            value = malloc(strlen((char*)node->value));
            strcpy(value, node->value);
            
            /* convert integer to string */
            length = snprintf(NULL, 0, "%d", node->counter);
            cnt = malloc(length+1);
            snprintf(cnt, length+1, "%d", node->counter);
            
            /* Combine all the strings and write them into file */
            string_to_write = malloc(strlen(value)+strlen(cnt)+3); // +3 for string terminator and for space and change line characters
            strcpy(string_to_write, value);
            strcat(string_to_write, " ");
            strcat(string_to_write, cnt);
            strcat(string_to_write, "\n");

            byte = write(fd2, string_to_write, strlen(string_to_write));

            free(string_to_write);
            free(value);
        }
        free(cnt);
        free(list);
        free(path);
        
        close(fd2);
        close(readfd);
        /* Empty buffer */
        memset(buffer, '\0', BUFFSIZE);
        
        /* Send signal to stop - tell parent that u are ready for another file */
        printf("Worker %d raised(SIGSTOP)\n", getpid());
        raise(SIGSTOP);
        printf("Worker %d continues \n", getpid());
    }
    
    return 0;
}