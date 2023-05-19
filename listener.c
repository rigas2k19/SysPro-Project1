/*
H entolh inotifywait stelnei eidopoihseis gia tis allages
sta periexomena enos katalogou tou file system. Me xrhsh 
ths inotifywait tha kanete monitoring twn allagwn sta arxeia
enos directory. H inotifywait tha ektelestei (me klhsh ths
oikogeneias exec) mesa se mia dikh sas diergasia listener.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void listener(char* args[]){
    execvp(args[0], args);
}