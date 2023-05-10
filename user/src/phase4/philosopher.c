#include "philosopher.h"

// TODO: define some sem if you need
int chops[PHI_NUM];
int numsATime;

void init() {
  // init some sem if you need
  //TODO();
  for(int i=0; i<PHI_NUM; i++){
    chops[i] = sem_open(1);
  }
  numsATime = sem_open(4);
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  while (1) {
    //TODO();
    P(numsATime);
    P(chops[id]);
    P(chops[(id+1)%5]);
    eat(id);
    V(chops[(id+1)%5]);
    V(chops[id]);
    V(numsATime);
    think(id);
  }
}
