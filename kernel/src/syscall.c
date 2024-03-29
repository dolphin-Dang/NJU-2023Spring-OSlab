#include "klib.h"
#include "cte.h"
#include "sysnum.h"
#include "vme.h"
#include "serial.h"
#include "loader.h"
#include "proc.h"
#include "timer.h"
#include "file.h"

typedef int (*syshandle_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern void *syscall_handle[NR_SYS];

void do_syscall(Context *ctx) {
  // TODO: Lab1-5 call specific syscall handle and set ctx register
  int sysnum = ctx->eax;
  uint32_t arg1 = ctx->ebx;
  uint32_t arg2 = ctx->ecx;
  uint32_t arg3 = ctx->edx;
  uint32_t arg4 = ctx->esi;
  uint32_t arg5 = ctx->edi;
  int res;
  if (sysnum < 0 || sysnum >= NR_SYS) {
    res = -1;
  } else {
    res = ((syshandle_t)(syscall_handle[sysnum]))(arg1, arg2, arg3, arg4, arg5);
  }
  ctx->eax = res;
}

int sys_write(int fd, const void *buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  //return serial_write(buf, count);
  proc_t *proc = proc_curr();
  file_t *file = proc_getfile(proc, fd);
  if(!file) return -1;
  //printf("here in sys_write\n");
  return fwrite(file, buf, count);
}

int sys_read(int fd, void *buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  //return serial_read(buf, count);
  proc_t *proc = proc_curr();
  file_t *file = proc_getfile(proc, fd);
  if(!file) return -1;
  return fread(file, buf, count);
}

int sys_brk(void *addr) {
  // TODO: Lab1-5
  //static size_t brk = 0; // use brk of proc instead of this in Lab2-1
  size_t brk = proc_curr()->brk;  //lab2-1
  size_t new_brk = PAGE_UP(addr);
  if (brk == 0) {
    proc_curr()->brk = new_brk;
  } else if (new_brk > brk) {
    //TODO();
    vm_map(vm_curr(), brk, new_brk - brk, 7);
    proc_curr()->brk = new_brk;
  } else if (new_brk < brk) {
    // can just do nothing
    /*
    vm_unmap(vm_curr(), new_brk, brk - new_brk);
    printf("here\n");
    brk = new_brk;
    */
  }
  return 0;
}

void sys_sleep(int ticks) {
  //TODO(); // Lab1-7
  uint32_t tick_start = get_tick();
  while (true) {
    uint32_t tick_now = get_tick();
    if(tick_now - tick_start < ticks){
      //sti(); hlt(); cli();
      proc_yield();
    }else{
      break;
    }
  }
}

int sys_exec(const char *path, char *const argv[]) {
  //TODO(); // Lab1-8, Lab2-1
    //printf("here\n");
  PD *pgdir = vm_alloc();
  Context ctx;
  int ret = load_user(pgdir, &ctx, path, argv);
  if(ret != 0){
    vm_teardown(pgdir);
    return -1;
  }else{
    PD *pgdir_save = vm_curr();
    set_cr3(pgdir);
    vm_teardown(pgdir_save);
    proc_curr()->pgdir = pgdir; //lab2-1
    irq_iret(&ctx);
  }
}

int sys_getpid() {
  //TODO(); // Lab2-1
  return proc_curr()->pid;
}

void sys_yield() {
  proc_yield();
}

int sys_fork() {
  //TODO(); // Lab2-2
  proc_t* childPCB = proc_alloc();
  if(childPCB == NULL) return -1;

  proc_copycurr(childPCB);
  proc_addready(childPCB);
  return childPCB->pid;
}

void sys_exit(int status) {
  //TODO(); // Lab2-3
  // while(1) proc_yield();
  //printf("PID:%d parent:%d become ZOMBIE\n", proc_curr()->pid, proc_curr()->parent->pid);
  proc_makezombie(proc_curr(), status);
  INT(0x81);
}

int sys_wait(int *status) {
  //TODO(); // Lab2-3, Lab2-4
  //sys_sleep(250);
  // return 0;

  proc_t* proc = proc_curr();
  if(proc->child_num == 0) return -1;
  /*
  proc_t* child = proc_findzombie(proc);
  while(child == NULL) {
    proc_yield();
    child = proc_findzombie(proc);
    //printf("in while child == NULL, num = %d, PID:%d\n", proc->child_num, proc->pid);
  }
  */
  sem_p(&(proc->zombie_sem));
  proc_t* child = proc_findzombie(proc);
  if(status != NULL) *status = child->exit_code;
  int pid = child->pid;
  //printf("here in sys_wait\n");
  proc_free(child);
  proc->child_num--;
  return pid;
}

int sys_sem_open(int value) {
  //TODO(); // Lab2-5
  proc_t *procNow = proc_curr();
  int id = proc_allocusem(procNow);
  if(id == -1) return id;
  usem_t *usem = usem_alloc(value);
  if(usem == NULL) return -1;

  procNow->usems[id] = usem;
  return id;
}

int sys_sem_p(int sem_id) {
  //TODO(); // Lab2-5
  proc_t *procNow = proc_curr();
  usem_t *usem = proc_getusem(procNow, sem_id);
  if(usem == NULL) return -1;

  sem_p(&(usem->sem));
  return 0;
}

int sys_sem_v(int sem_id) {
  //TODO(); // Lab2-5
  proc_t *procNow = proc_curr();
  usem_t *usem = proc_getusem(procNow, sem_id);
  if(usem == NULL) return -1;

  sem_v(&(usem->sem));
  return 0;
}

int sys_sem_close(int sem_id) {
  //TODO(); // Lab2-5
  proc_t *procNow = proc_curr();
  usem_t *usem = proc_getusem(procNow, sem_id);
  if(usem == NULL) return -1;

  usem_close(usem);
  procNow->usems[sem_id] = NULL;
  return 0;
}

int sys_open(const char *path, int mode) {
  //TODO(); // Lab3-1
  proc_t *proc = proc_curr();
  //printf("here in open\n");
  int fd = proc_allocfile(proc);
  if(fd == -1) return -1;

  file_t *file = fopen(path, mode);
  if(!file) return -1;

  proc->files[fd] = file;
  return fd;
}

int sys_close(int fd) {
  //TODO(); // Lab3-1
  proc_t *proc = proc_curr();
  file_t *file = proc_getfile(proc, fd);
  if(!file) return -1;
  fclose(file);
  proc->files[fd] = NULL;
  return 0;
}

int sys_dup(int fd) {
  //TODO(); // Lab3-1
  proc_t *proc = proc_curr();
  int fdnew = proc_allocfile(proc);
  if(fdnew == -1)return -1;

  file_t *file = proc_getfile(proc, fd);
  if(!file)return -1;

  proc->files[fdnew] = fdup(file);
  return fdnew;
}

uint32_t sys_lseek(int fd, uint32_t off, int whence) {
  //TODO(); // Lab3-1
  proc_t *proc = proc_curr();
  file_t *file = proc_getfile(proc, fd);
  if(!file) return -1;
  return fseek(file, off, whence);
}

int sys_fstat(int fd, struct stat *st) {
  //TODO(); // Lab3-1
  proc_t *proc = proc_curr();
  file_t *file = proc_getfile(proc, fd);
  if(!file)return -1;

  if(file->type == TYPE_FILE || file->type == TYPE_DIR){
    st->node = ino(file->inode);
    st->size = isize(file->inode);
    st->type = itype(file->inode);
  }else if(file->type == TYPE_DEV){
    st->type = TYPE_DEV;
    st->size = 0;
    st->node = 0;
  }
  return 0;
}

int sys_chdir(const char *path) {
  //TODO(); // Lab3-2
  inode_t * inode = iopen(path, TYPE_NONE);
  if(inode == NULL)return -1;
  if(itype(inode) != TYPE_DIR){
    iclose(inode);
    return -1;
  }else{
    proc_t *proc = proc_curr();
    iclose(proc->cwd);
    proc->cwd = inode;
    return 0;
  }
}

int sys_unlink(const char *path) {
  return iremove(path);
}

// optional syscall

void *sys_mmap() {
  TODO();
}

void sys_munmap(void *addr) {
  TODO();
}

int sys_clone(void (*entry)(void*), void *stack, void *arg) {
  TODO();
}

int sys_kill(int pid) {
  TODO();
}

int sys_cv_open() {
  TODO();
}

int sys_cv_wait(int cv_id, int sem_id) {
  TODO();
}

int sys_cv_sig(int cv_id) {
  TODO();
}

int sys_cv_sigall(int cv_id) {
  TODO();
}

int sys_cv_close(int cv_id) {
  TODO();
}

int sys_pipe(int fd[2]) {
  TODO();
}

int sys_link(const char *oldpath, const char *newpath) {
  TODO();
}

int sys_symlink(const char *oldpath, const char *newpath) {
  TODO();
}

void *syscall_handle[NR_SYS] = {
  [SYS_write] = sys_write,
  [SYS_read] = sys_read,
  [SYS_brk] = sys_brk,
  [SYS_sleep] = sys_sleep,
  [SYS_exec] = sys_exec,
  [SYS_getpid] = sys_getpid,
  [SYS_yield] = sys_yield,
  [SYS_fork] = sys_fork,
  [SYS_exit] = sys_exit,
  [SYS_wait] = sys_wait,
  [SYS_sem_open] = sys_sem_open,
  [SYS_sem_p] = sys_sem_p,
  [SYS_sem_v] = sys_sem_v,
  [SYS_sem_close] = sys_sem_close,
  [SYS_open] = sys_open,
  [SYS_close] = sys_close,
  [SYS_dup] = sys_dup,
  [SYS_lseek] = sys_lseek,
  [SYS_fstat] = sys_fstat,
  [SYS_chdir] = sys_chdir,
  [SYS_unlink] = sys_unlink,
  [SYS_mmap] = sys_mmap,
  [SYS_munmap] = sys_munmap,
  [SYS_clone] = sys_clone,
  [SYS_kill] = sys_kill,
  [SYS_cv_open] = sys_cv_open,
  [SYS_cv_wait] = sys_cv_wait,
  [SYS_cv_sig] = sys_cv_sig,
  [SYS_cv_sigall] = sys_cv_sigall,
  [SYS_cv_close] = sys_cv_close,
  [SYS_pipe] = sys_pipe,
  [SYS_link] = sys_link,
  [SYS_symlink] = sys_symlink};
