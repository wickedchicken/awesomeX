#include <stdio.h>
#include <stdlib.h>
#include "print.h"

int main(int argc, char *argv[]){
  if(argc < 2){
    fprintf(stderr, "must specify an argument!\n");
    return 1;
  }

  char *p;
  int input = strtol(argv[1],&p,10);
  if(!*argv[1] || (*p)){
    fprintf(stderr, "must specify an integer!\n");
    return 1;
  }

  int len = print(input);

  printf("this should read 15: %d\n", len);
  return 0;
}
