#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <kern/limits.h>
#include <synch.h>
#include <mips/trapframe.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include "opt-A2.h"

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

#if OPT_A2
void add_pid(pid_t curpid){
  pid_t *pid_pointer = kmalloc(sizeof(pid_t));
  *pid_pointer = curpid;
  array_add(ava_pid, ((void*) pid_pointer), NULL);
}

#endif /* OPT_A2 */



void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  //DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);

  #if OPT_A2

  lock_acquire(proc_lock);
  int nproc = array_num(allp_relation);
  pid_t curpid = p->p_pid;

      //DEBUG (DB_SYSCALL, "start loop sys_exit \n");
  struct p_relation *temp;

  for(int i = 0; i < nproc; i++){
    temp = ((struct p_relation *) array_get(allp_relation, i));

    if(temp->child_pid == curpid){ //current proc is childern
        //DEBUG (DB_SYSCALL, "curproc is a child proc \n");
      temp->child_alive = false;
      if(temp->parent_alive == false){ // parent of curproc is dead, the pid of curproc is available now
          //DEBUG(DB_SYSCALL, "parent of curproc exited, pid of curproc can be reuse \n");
        lock_acquire(ava_pid_lock);
        
        add_pid(curpid);
        lock_release(ava_pid_lock);

        array_remove(allp_relation, i); //curproc have be killed
        //kfree(pid_pointer);
        i = i -1;
        nproc = nproc - 1; //whole arry be moved froward one position
      }else if (temp->parent_alive == true){ // temp->parent_alive == true
          //DEBUG(DB_SYSCALL, "parent of curproc is still running \n");
        temp->ex_code = _MKWAIT_EXIT(exitcode);
        cv_broadcast(proc_cv, proc_lock);
      }


    }else if(temp->parent_pid == curpid){ //current proc is parent
        //DEBUG (DB_SYSCALL, "curproc is a parent proc \n");

      temp->parent_alive = false;
      if(temp->child_alive == false){ // child has been killed, it's pid is available now
          //DEBUG (DB_SYSCALL, "curproc's child died, it's pid can be reuse \n");
        lock_acquire(ava_pid_lock);
        add_pid(temp->child_pid);
        lock_release(ava_pid_lock);

        array_remove(allp_relation, i);
        //kfree(pid_pointer);

        i = i-1;
        nproc = nproc -1;
      }else if(temp->child_alive == true){
          //DEBUG (DB_SYSCALL, "do nothing \n");
        continue;
      }
    }
  }
  lock_release(proc_lock);
  #endif /* OPT_A2 */

  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");


}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;

  #if OPT_A2
  *retval = curproc->p_pid;

  #endif /* OPT_A2 */
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  #if OPT_A2

  exitstatus = 0;
  struct proc *p = curproc;
  lock_acquire(proc_lock);
  int numporc = array_num(allp_relation);


      //DEBUG(DB_SYSCALL, "start loop \n");
  struct p_relation *temp;
  for(int i = 0; i < numporc; i++){
    
    temp = ((struct p_relation *) array_get(allp_relation, i));
    if(p->p_pid == temp->parent_pid && temp->child_pid == pid){ // proc with (procid == pid) is a child of curproc
          //DEBUG(DB_SYSCALL, "find a child proc \n");
      if(temp->child_alive == true){ // child is still alive
          //DEBUG(DB_SYSCALL, "child proc is running \n");
        while(temp->child_alive == true){
          cv_wait(proc_cv, proc_lock); // wiat until child proc exit
          //DEBUG(DB_SYSCALL, "wait child proc exit \n");
        }
      }if(temp->child_alive == false){ // child is died
        exitstatus = temp->ex_code; //set the exitcode
          //DEBUG(DB_SYSCALL, "child has exited \n");
      }
      break;
    }else if(p->p_pid != temp->parent_pid && temp->child_pid == pid){ //do not find a child of curproc with (procid == pid)
      lock_release(proc_lock);
      return(0); // wairpid() fail
    }
  }
  lock_release(proc_lock);

     //DEBUG(DB_SYSCALL, "waitpid success\n");

  #endif /* OPT_A2 */

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 exitstatus = 0;*/
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
int sys_fork(struct trapframe *tf, pid_t *retval){
  struct proc *p = curproc;
  struct proc *child_proc = proc_create_runprogram(p->p_name); // pid has already assigned !!!

  if(child_proc == NULL){
    return(ENOMEM); //Sufficient virtual memory for the new process was not available.
  }

  if(child_proc->p_pid >= 32767){ // if pid excess max value
    lock_acquire(ava_pid_lock);
    pidc = pidc - 1;
    lock_release(ava_pid_lock);
  }

       //DEBUG(DB_SYSCALL, "check max value of pid \n");

  struct addrspace * child_space; 

  if(as_copy(p->p_addrspace, &child_space) == ENOMEM){
    return(ENOMEM); //Sufficient virtual memory for the new process was not available.
  }


  spinlock_acquire(&child_proc->p_lock);//give child process an address space
  child_proc->p_addrspace = child_space;
  spinlock_release(&child_proc->p_lock);
       //DEBUG(DB_SYSCALL, "assigning address space \n");

       //DEBUG(DB_SYSCALL, "creat c/p relationship \n");

  

       //DEBUG(DB_SYSCALL, "before acquire proc_lock \n");
  lock_acquire(proc_lock); // create child/patrent relationship
       //DEBUG(DB_SYSCALL, "after acquire proc_lock \n");

  struct p_relation *new_relation = p_relation_create(p->p_pid, child_proc->p_pid);


  array_add(allp_relation, new_relation, NULL);
  lock_release(proc_lock);

       //DEBUG(DB_SYSCALL, "build child/parent relationship \n");

  struct trapframe * ntf;
  ntf = kmalloc(sizeof(*ntf));

  *ntf = *tf;

      //DEBUG(DB_SYSCALL, "before set trapframe to child process \n");

  int success = thread_fork("child_proc", child_proc, (void*) enter_forked_process, (void*) ntf, (unsigned long) 0);

       //DEBUG(DB_SYSCALL, "after set trapframe to child process \n");

  if(success){ // if error
    return success;
  }

  *retval = child_proc->p_pid;

       //DEBUG(DB_SYSCALL, "assign return value \n");

  return(0);
}

void cleanup(char **arr, int length);
void cleanup(char **arr, int length){
  for(long i=0; i<length; i++){
    kfree(arr[i]);
  }
  kfree(arr);
}


int sys_execv(char* program, char** args){ 
  int numagrs = 0;
  char** temp;
  int result;

  //kprintf("sys_execv start\n");

  //kprintf("111sys_execv before copy\n");
  result = copyin((const_userptr_t)args, &temp, sizeof(char**)); 
  if(result){
    return result;
  }

  //kprintf("111sys_execv after copyin\n");

  //kprintf("count the number of arguments\n");

  for(int i = 0; ((char**)args)[i] != NULL; i++){ // count the number of arguments
    numagrs++;

  }
  numagrs++; //Null terminator

  /*//kprintf("sys_execv before copy\n");
  result = copyin(args, dest, numagrs * sizeof(char*)); 
  if(result){
    return result;
  }

  //kprintf("sys_execv after copyin\n");*/

  //kprintf("copy arguments into kernel\n");
  //copy arguments into kernel
  char** dest = kmalloc(numagrs * 4);
  for(int i = 0; i < numagrs-1; i++){
    int length = strlen(((char**)args)[i]) + 1;
    dest[i] = kmalloc(length);
    result = copyinstr((const_userptr_t)args[i], dest[i], length, NULL);

    if(result){
      cleanup(dest, i);
      return result;
    }
  }
  dest[numagrs-1] = NULL;

  //kprintf("copy the program path into kernel\n");

  //copy the program path into kernel
  char* dest2 = kmalloc(strlen(program) + 1);
  result = copyin((const_userptr_t)program, dest2, strlen(program)+1);
  if(result){
    cleanup(dest, numagrs-1);
    kfree(dest2);
    return result;
  }


  struct addrspace *as;
  struct addrspace *old_as;
  struct vnode *v;
  vaddr_t entrypoint, stackptr;
  
  //kprintf("open the file\n");

  // Open the file
  result = vfs_open(dest2, O_RDONLY, 0, &v);
  if (result) {
    cleanup(dest, numagrs-1);
    kfree(dest2);
    return result;
  }

//kprintf("Create a new address space\n");
  // Create a new address space.
  as = as_create();
  if (as ==NULL) {
    vfs_close(v);
    cleanup(dest, numagrs-1);
    kfree(dest2);
    return ENOMEM;
  }

  // set process to the new address space, and activate it
  old_as = curproc_setas(as);
  as_activate();

  // Load the executable.
  result = load_elf(v, &entrypoint);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    vfs_close(v);
    cleanup(dest, numagrs-1);
    kfree(dest2);
    return result;
  }

  vfs_close(v);

  // Define the user stack in the address space 
  result = as_define_stack(as, &stackptr);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    cleanup(dest, numagrs-1);
    kfree(dest2);
    return result;
  }

//kprintf("copy strings onto user stack\n");

  //copy strings onto user stack
  userptr_t source[numagrs];
  for(int i = 0; i < numagrs - 1; i++){
    int length = strlen(dest[i]) + 1;
    int length_rp = ROUNDUP(length, 8);  //round size
    stackptr = stackptr - length_rp; //get the stack address
    
    source[i] = (userptr_t) stackptr;
    result = copyoutstr(dest[i], source[i], length * sizeof(char), NULL);
    if(result){
      return result;
    }
  }

  source[numagrs-1] = NULL; //set NULL terminator
  
//kprintf("copy array onto stack stack\n");

  //copy array onto stack stack
  int size = ROUNDUP(numagrs*sizeof(char*), 8);
  stackptr = stackptr - size;
  result = copyout(source, (userptr_t)stackptr, (size_t)size);
  if(result){
    return result;
  }

  //delete old address space
  as_destroy(old_as);


//kprintf("sys_execv before enter new process\n");

  enter_new_process(numagrs-1  /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
        stackptr, entrypoint);

//kprintf("sys_execv after enter new process\n");

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");
  return EINVAL;
}

#endif /* OPT_A2 */
















