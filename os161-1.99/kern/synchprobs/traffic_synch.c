#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
static struct lock *intersectionlock;
static int count[12];
static struct cv *allcv[12];
char *path_name[12] = ["SW", "SN", "SE", "ES", "EW", "EN", "NE", "NS", "NW", "WN", "WE", "WS"];
                      //0      1     2     3     4     5     6    7      8     9    10    11   

static void count_changer(Direction origin, Direction destination, char sign);
static void wake_up_all(int n);
static void set_wait_count(Direction origin, Direction destination, int *num_count, struct cv ** cur_cv);
static void add_wait_count(Direction origin, Direction destination);
static void sub_wait_count(Direction origin, Direction destination);



/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  intersectionlock = lock_create("intersectionlock");

  for(int i = 0; i < 12; i++){
    count[i] = 0;
    allcv[i] = cv_create(path_name[i]);
  }

  /* replace this default implementation with your own implementation */
  if (intersectionSem == NULL) {
    panic("could not create intersection semaphore");
  }

  for(int i = 0; i < 12; i++){
    if(allcv[i] == NULL){
       panic("could not create intersection semaphore");
    }
  }
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersectionSem != NULL);
  for(int i = 0; i < 12; i++){
    KASSERT(allcv[i]);
  }
  for(int i = 0; i < 12; i++){
    cv_destory(allcv[i]);
  }
  lock_destroy(intersectionlock);
}

static void count_changer(Direction origin, Direction destination, char sign){
  int i;
  if (sign == '-'){
    i = -1;
  }if(sign == '+'){
    i = 1;
  }if(origin == south && destination == west){
    num_count[7]+i;
    num_count[10]+i;
    num_count[4]+i;
    num_count[6]+i;
    num_count[8]+i;
    num_count[9]+i;
    num_count[3]+i;
  }if(origin == south && destination == north){
    num_count[10]+i;
    num_count[4]+i;
    num_count[6]+i;
    num_count[9]+i;
    num_count[3]+i;
    num_count[5]+i;
  }if(origin == south && destination == east){
    num_count[4]+i;
    num_count[6]+i;
  }if(origin == east && destination == south){
    num_count[1]+i;
    num_count[7]+i;
    num_count[10]+i;
    num_count[0]+i;
    num_count[6]+i;
    num_count[11]+i;
    num_count[9]+i;
  }if(origin == east && destination == west){
    num_count[1]+i;
    num_count[7]+i;
    num_count[0]+i;
    num_count[6]+i;
    num_count[8]+i;
    num_count[9]+i;
  }if(origin == east && destination == north){
    num_count[1]+i;
    num_count[9]+i;
  }if(origin == north && destination == east){
    num_count[1]+i;
    num_count[10]+i;
    num_count[4]+i;
    num_count[2]+i;
    num_count[0]+i;
    num_count[9]+i;
    num_count[3]+i;
  }if(origin == north && destination == south){
    num_count[10]+i;
    num_count[4]+i;
    num_count[0]+i;
    num_count[11]+i;
    num_count[9]+i;
    num_count[3]+i;
  }if(origin == north && destination == west){
    num_count[1]+i;
    num_count[10]+i;
    num_count[4]+i;
    num_count[2]+i;
    num_count[0]+i;
    num_count[9]+i;
    num_count[3]+i;
  }if(origin == west && destination == north){
    num_count[1]+i;
    num_count[7]+i;
    num_count[4]+i;
    num_count[0]+i;
    num_count[6]+i;
    num_count[3]+i;
    num_count[5]+i;
  }if(origin == west && destination == east){
    num_count[1]+i;
    num_count[7]+i;
    num_count[2]+i;
    num_count[0]+i;
    num_count[6]+i;
    num_count[3]+i;
  }if(origin == west && destination == south){
    num_count[3]+i;
    num_count[7]+i;
  }
}


static void set_wait_count (Direction origin, Direction destination, int *num_count, struct cv ** cur_cv){
  if(origin == south && destination == west){
    *cur_cv = allcv[0];
    *num_count = count[0];
  }if(origin == south && destination == north){
    *cur_cv = allcv[1];
    *num_count = count[1];
  }if(origin == south && destination == east){
    *cur_cv = allcv[2];
    *num_count = count[2];
  }if(origin == east && destination == south){
    *cur_cv = allcv[3];
    *num_count = count[3];
  }if(origin == east && destination == west){
    *cur_cv = allcv[4];
    *num_count = count[4];
  }if(origin == east && destination == north){
    *cur_cv = allcv[5];
    *num_count = count[5];
  }if(origin == north && destination == east){
    *cur_cv = allcv[6];
    *num_count = count[6];
  }if(origin == north && destination == south){
    *cur_cv = allcv[7];
    *num_count = count[7];
  }if(origin == north && destination == west){
    *cur_cv = allcv[8];
    *num_count = count[8];
  }if(origin == west && destination == north){
    *cur_cv = allcv[9];
    *num_count = count[9];
  }if(origin == west && destination == east){
    *cur_cv = allcv[10];
    *num_count = count[10];
  }if(origin == west && destination == south){
    *cur_cv = allcv[11];
    *num_count = count[11];
  }

}

static void add_wait_count(Direction origin, Direction destination){
  count_changer(origin, destination, '+');
}

static void wake_up_all(int n){
  if(num_count[n] == 0){
    cv_broadcast(allcv[n], intersectionlock);
  }
}

static void sub_wait_count(Direction origin, Direction destination){
  if(origin == south && destination == west){
    count_changer(origin, destination, '-');
    wake_up_all(7);
    wake_up_all(10);
    wake_up_all(4);
    wake_up_all(6);
    wake_up_all(8);
    wake_up_all(9);
    wake_up_all(3);
  }else if(origin == south && destination == north){
    count_changer(origin, destination, '-');
    wake_up_all(10);
    wake_up_all(4);
    wake_up_all(6);
    wake_up_all(9);
    wake_up_all(3);
    wake_up_all(5);
  }else if(origin == south && destination == east){
    count_changer(origin, destination, '-');
    wake_up_all(4);
    wake_up_all(6);
  }else if(origin == east && destination == south){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(7);
    wake_up_all(10);
    wake_up_all(0);
    wake_up_all(6);
    wake_up_all(11);
    wake_up_all(9);
  }else if(origin == east && destination == west){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(7);
    wake_up_all(0);
    wake_up_all(6);
    wake_up_all(8);
    wake_up_all(9);
  }else if(origin == east && destination == north){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(9);
  }else if(origin == north && destination == east){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(10);
    wake_up_all(4);
    wake_up_all(2);
    wake_up_all(0);
    wake_up_all(9);
    wake_up_all(3);
  }else if(origin == north && destination == south){
    count_changer(origin, destination, '-');
    wake_up_all(10);
    wake_up_all(4);
    wake_up_all(0);
    wake_up_all(11);
    wake_up_all(9);
    wake_up_all(3);
  }else if(origin == north && destination == west){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(10);
    wake_up_all(4);
    wake_up_all(2);
    wake_up_all(0);
    wake_up_all(9);
    wake_up_all(3);
  }else if(origin == west && destination == north){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(7);
    wake_up_all(4);
    wake_up_all(0);
    wake_up_all(6);
    wake_up_all(3);
    wake_up_all(5);
  }else if(origin == west && destination == east){
    count_changer(origin, destination, '-');
    wake_up_all(1);
    wake_up_all(7);
    wake_up_all(2);
    wake_up_all(0);
    wake_up_all(6);
    wake_up_all(3);
  }else if(origin == west && destination == south){
    count_changer(origin, destination, '-');
    wake_up_all(3);
    wake_up_all(7);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersectionSem != NULL);
  int *num_count;
  struct cv *cur_cv;
  lock_acquire(intersectionlock);
  set_wait_count(origin, destination, num_count, &cur_cv);

  while(true){
    if (*num_count == 0){
      add_wait_count(origin, destination);
    }else{
      cv_wait(cur_cv, intersectionlock);
      set_wait_count(origin, destination, num_count, &cur_cv);
    }
  }

  lock_release(intersectionlock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersectionSem != NULL);
  lock_acquire(intersectionlock);
  sub_wait_count(origin, destination);
  lock_release(intersectionlock);
}
