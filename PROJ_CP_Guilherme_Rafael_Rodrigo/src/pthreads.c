#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void) {

   int thread_count;

   thread_count = 1;

  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );


   printf("%d-%s%d-%s%d-%s%d-%s", thread_count, asctime (timeinfo), thread_count, asctime (timeinfo), thread_count, asctime (timeinfo), thread_count, asctime (timeinfo));

   return 0;
}
