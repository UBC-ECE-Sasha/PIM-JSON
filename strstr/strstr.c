#include <barrier.h>
#include <defs.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mutex.h>
#include <perfcounter.h>
// #include <math.h>

BARRIER_INIT(my_barrier, NR_TASKLETS);
MUTEX_INIT(my_mutex);
char coefficients[300];

int found = 0;

/* Computes the sum of coefficients within a tasklet specific range. */
int compute_checksum() {
  int i, checksum = 0;
  for (i = 0; i < 32; i++)
    checksum += coefficients[(me() << 5) + i];
  return checksum;
}

void strstr_dpu() {
//  int num = ((int)me())<<6+100;
  mutex_id_t mutex_id = MUTEX_GET(my_mutex);
  int num = me();
  num= num==0 ? 0:(num<<6) -5;
  // char* result = strstr(coefficients+num, "trump");
  // windows CRT
  char*s1,*s2;
  char*cp = (char*)coefficients+num;

  for (int i =0; i< 70; i++) {
    s1= cp;
    s2 = (char*)"trump";
    while(*s1 &&*s2&&!(*s1-*s2)){
      s1++; 
      s2++;
    }

    if(!*s2) {
      mutex_lock(mutex_id);
      found = 1;
      mutex_unlock(mutex_id);
      return;
    } 

    cp++;
  }
}

/* Initializes the coefficient table. */
void setup_coefficients() {
  int i;
  for (i = 0; i < 300; i++) {
    if(i %20 ==0) {
      coefficients[i] = 't';
    }
    else {
      coefficients[i] = i;
    }
  }

  int needle = 230;
  strncpy(coefficients+needle, "trump", 5);
  // printf("found %s\n", coefficients);
  
}

/* The main thread initializes the table and joins the barrier to wake up the
 * other tasklets. */
int master() {
  barrier_id_t barrier = BARRIER_GET(my_barrier);
  setup_coefficients();
  barrier_wait(barrier);
  strstr_dpu();
  printf("result is %d\n", found);
  return 0;
}

/* Tasklets wait for the initialization to complete, then compute their
 * checksum. */
int slave() {
  barrier_id_t barrier = BARRIER_GET(my_barrier);
  barrier_wait(barrier);
  strstr_dpu();
  // return result;
  return 0;
}

int main() {
  int tasklet_id = me();
  if (tasklet_id == 0)
    perfcounter_config(COUNT_CYCLES, true);
  
  tasklet_id == 0 ? master(): slave();

  uint32_t cycles = (uint32_t)perfcounter_get();  
  printf("total cycle is %d\n", cycles);
  return 0;
  // return tasklet_id == 0 ? master(): slave();
}