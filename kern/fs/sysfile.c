//
// File-system system calls.
//

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include <kern/thread/PTCBIntro/export.h>
#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/trap/TSyscallArg/export.h>
#include <kern/lib/spinlock.h>

#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"

#define MAX_BUF 10000

char k_buff[MAX_BUF];
spinlock_t k_buff_lock; 

/**
 * This function is not a system call handler, but an auxiliary function
 * used by sys_open.
 * Allocate a file descriptor for the given file.
 * You should scan the list of open files for the current thread
 * and find the first file descriptor that is available.
 * Return the found descriptor or -1 if none of them are free.
 */
static int
fdalloc(struct file *f)
{
  int pid, i;
  struct file **open_files;

  pid = get_curid();
  open_files = tcb_get_openfiles(pid);

  for(i = 0; i < NOFILE; i++){
    if(open_files[i] == 0){ // a "free" file descriptor is represented by a null pointer
      tcb_set_openfiles(pid, i, f);
      //open_files[i] = f;
      return i;
    }
  }
  return -1;
}


/**
 * From the file indexed by the given file descriptor, read n bytes and save them
 * into the buffer in the user. As explained in the assignment specification,
 * you should first write to a kernel buffer then copy the data into user buffer
 * with pt_copyout.
 * Return Value: Upon successful completion, read() shall return a non-negative
 * integer indicating the number of bytes actually read. Otherwise, the
 * functions shall return -1 and set errno E_BADF to indicate the error.
 */
void sys_read(tf_t *tf)
{
  int fd;
  unsigned int u_buff;
  size_t n, read; // bytes to read; bytes read
  struct file *f;

  // get args
  fd = syscall_get_arg2(tf);
  u_buff = syscall_get_arg3(tf);
  n = syscall_get_arg4(tf);

  // basic error check
  if(fd < 0 || fd >= NOFILE || !u_buff || n < 0 || n > MAX_BUF)
    goto bad;
  else if (VM_USERLO > u_buff || u_buff + n > VM_USERHI || n > sizeof(k_buff))
    goto bad;

	spinlock_acquire(&k_buff_lock);
  
  // "clear" buffer
  k_buff[0] = '\0';

  // disk --> kernel memory
  f = tcb_get_openfiles(get_curid())[fd];
  if (f == 0)
    goto bad;
  
  read = file_read(f, k_buff, n);
  if (read < 0)
    goto bad;
   
  // kernel memory --> user memory
  if (read > 0) {
    if (pt_copyout(k_buff, get_curid(), u_buff, read) != read) 
      goto bad;
  } 

	spinlock_release(&k_buff_lock);
	syscall_set_errno(tf, E_SUCC);
	syscall_set_retval1(tf, read);
  return;
bad:
	spinlock_release(&k_buff_lock);
  syscall_set_errno(tf, E_BADF);
	syscall_set_retval1(tf, -1);
  return ;
}

/**
 * Write n bytes of data in the user's buffer into the file indexed by the file descriptor.
 * You should first copy the data info an in-kernel buffer with pt_copyin and then
 * pass this buffer to appropriate file manipulation function.
 * Upon successful completion, write() shall return the number of bytes actually
 * written to the file associated with f. This number shall never be greater
 * than nbyte. Otherwise, -1 shall be returned and errno E_BADF set to indicate the
 * error.
 */
void sys_write(tf_t *tf)
{
  int fd;
  unsigned int u_buff;
  size_t n, written; // bytes to write; bytes written
  struct file *f;

  // get args
  fd = syscall_get_arg2(tf);
  u_buff = syscall_get_arg3(tf);
  n = syscall_get_arg4(tf);

  // basic error check
  if(fd < 0 || fd >= NOFILE || !u_buff || n < 0 || n > MAX_BUF)
    goto bad;
  else if (!(VM_USERLO <= u_buff && u_buff + n <= VM_USERHI))
    goto bad;

	spinlock_acquire(&k_buff_lock);

  // user memory --> kernel memory
  if(pt_copyin(get_curid(), u_buff, k_buff, n) < n)
    goto bad;

  // kernel memory --> disk
  f = tcb_get_openfiles(get_curid())[fd];
  if((written = file_write(f, k_buff, n)) > n)
    goto bad;

	spinlock_release(&k_buff_lock);
	syscall_set_errno(tf, E_SUCC);
	syscall_set_retval1(tf, written);
  return ;
bad:
	spinlock_release(&k_buff_lock);
  syscall_set_errno(tf, E_BADF);
	syscall_set_retval1(tf, -1);
  return ;
}


/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_close(tf_t *tf)
{
  struct file *f;
	int fd;
	unsigned int pid;
	pid = get_curid();

	fd = syscall_get_arg2(tf);
	if (fd < 0 || fd >= NOFILE) 
		goto bad;

  f = tcb_get_openfiles(pid)[fd];
	if (f == 0)
		goto bad;

  file_close(f);
	tcb_set_openfiles(pid, fd, 0);
	syscall_set_errno(tf, E_SUCC);
	syscall_set_retval1(tf, 0);
	return;

bad:
	syscall_set_errno(tf, E_BADF);
	syscall_set_retval1(tf, -1);
	return;
}

/**
 * Return Value: Upon successful completion, 0 shall be returned. Otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_fstat(tf_t *tf)
{
  struct file *f;
	int fd;
	struct file_stat *user;
	struct file_stat kern;
	unsigned int pid;
	int stat_size = sizeof(struct file_stat);

	fd = syscall_get_arg2(tf);
	if (fd < 0 || fd >= NOFILE)
		goto bad;

	user = syscall_get_arg3(tf);
	if ((unsigned int) user < VM_USERLO || (unsigned int) user > VM_USERHI - stat_size)
		goto bad;

	f = tcb_get_openfiles(pid)[fd];
	if (f == 0)
		goto bad;

	pt_copyin(pid, user, &kern, stat_size);
	if (file_stat(f, &kern) != 0)
		goto bad;

	syscall_set_errno(tf, E_SUCC);
	syscall_set_retval1(tf, 0);
	return;

bad:
  syscall_set_errno(tf, E_BADF);
	syscall_set_retval1(tf, -1);
	return;
}

/**
 * Create the path new as a link to the same inode as old.
 */
void sys_link(tf_t *tf)
{
  int old_len, new_len;
  char name[DIRSIZ], new[128], old[128];
  struct inode *dp, *ip;

  old_len = syscall_get_arg4(tf);
  new_len = syscall_get_arg5(tf);

  // +1 for terminating null char
  if(old_len + 1 > 128 || new_len + 1 > 128)
    KERN_PANIC("sys_link length greater than 128 chars");
  
  pt_copyin(get_curid(), syscall_get_arg2(tf), old, old_len + 1);
  pt_copyin(get_curid(), syscall_get_arg3(tf), new, new_len + 1);

  if((ip = namei(old)) == 0){
    syscall_set_errno(tf, E_NEXIST);
    return;
  }
  
  begin_trans();

  inode_lock(ip);
  if(ip->type == T_DIR){
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }

  ip->nlink++;
  inode_update(ip);
  inode_unlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  inode_lock(dp);
  if(   dp->dev != ip->dev
     || dir_link(dp, name, ip->inum) < 0){
    inode_unlockput(dp);
    goto bad;
  }
  inode_unlockput(dp);
  inode_put(ip);

  commit_trans();

  syscall_set_errno(tf, E_SUCC);
  return;

bad:
  inode_lock(ip);
  ip->nlink--;
  inode_update(ip);
  inode_unlockput(ip);
  commit_trans();
  syscall_set_errno(tf, E_DISK_OP);
  return;
}

/**
 * Is the directory dp empty except for "." and ".." ?
 */
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(inode_read(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      KERN_PANIC("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

void sys_unlink(tf_t *tf)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], path[128];
  uint32_t off;
  int len;

  len = syscall_get_arg3(tf);
  if(len + 1 > 128)
    KERN_PANIC("sys_unlink path length too long");

  pt_copyin(get_curid(), syscall_get_arg2(tf), path, len + 1);

  if((dp = nameiparent(path, name)) == 0){
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }
  
  begin_trans();

  inode_lock(dp);

  // Cannot unlink "." or "..".
  if(   dir_namecmp(name, ".") == 0
     || dir_namecmp(name, "..") == 0)
    goto bad;

  if((ip = dir_lookup(dp, name, &off)) == 0)
    goto bad;
  inode_lock(ip);

  if(ip->nlink < 1)
    KERN_PANIC("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    inode_unlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(inode_write(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    KERN_PANIC("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    inode_update(dp);
  }
  inode_unlockput(dp);

  ip->nlink--;
  inode_update(ip);
  inode_unlockput(ip);

  commit_trans();

  syscall_set_errno(tf, E_SUCC);
  return;
  
bad:
  inode_unlockput(dp);
  commit_trans();
  syscall_set_errno(tf, E_DISK_OP);
  return;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  uint32_t off;
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  inode_lock(dp);
  if((ip = dir_lookup(dp, name, &off)) != 0){
    inode_unlockput(dp);
    inode_lock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    inode_unlockput(ip);
    return 0;
  }
  
  if((ip = inode_alloc(dp->dev, type)) == 0)
    KERN_PANIC("create: ialloc");

  inode_lock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  inode_update(ip);
  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    inode_update(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(   dir_link(ip, ".", ip->inum) < 0
       || dir_link(ip, "..", dp->inum) < 0)
      KERN_PANIC("create dots");
  }
  if(dir_link(dp, name, ip->inum) < 0)
    KERN_PANIC("create: dir_link");

  inode_unlockput(dp);

  return ip;
}

void sys_open(tf_t *tf)
{
  char path[128];
  int fd, omode, len;
  struct file *f;
  struct inode *ip;

  static int first = TRUE;
  if (first) {
    first = FALSE;
    log_init();
  }

  len = syscall_get_arg4(tf);
  if(len + 1 > 128)
    KERN_PANIC("sys_open length greater than 128 chars [%d]", len + 1);
  
  pt_copyin(get_curid(), syscall_get_arg2(tf), path, len + 1);
  omode = syscall_get_arg3(tf);

  if (!path)
    KERN_PANIC("sys_open: no path");
  
  if(omode & O_CREATE){
    begin_trans();
    ip = create(path, T_FILE, 0, 0);
    commit_trans();
    if(ip == 0){
      syscall_set_retval1(tf, -1);
      syscall_set_errno(tf, E_CREATE);
      return;
    }
  } else {
    if((ip = namei(path)) == 0){
      syscall_set_retval1(tf, -1);
      syscall_set_errno(tf, E_NEXIST);
      return;
    }
    inode_lock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      inode_unlockput(ip);
      syscall_set_retval1(tf, -1);
      syscall_set_errno(tf, E_DISK_OP);
      return;
    }
  }

  if((f = file_alloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      file_close(f);
    inode_unlockput(ip);
    syscall_set_retval1(tf, -1);
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }
  inode_unlock(ip);

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  syscall_set_retval1(tf, fd);
  syscall_set_errno(tf, E_SUCC);
}

void sys_mkdir(tf_t *tf)
{
  char path[128];
  struct inode *ip;
  int len;

  len = syscall_get_arg3(tf);
  if(len + 1 > 128)
    KERN_PANIC("sys_mkdir path length greater than 128");

  pt_copyin(get_curid(), syscall_get_arg2(tf), path, len + 1);
  begin_trans();
  if((ip = (struct inode*)create(path, T_DIR, 0, 0)) == 0){
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }
  inode_unlockput(ip);
  commit_trans();
  syscall_set_errno(tf, E_SUCC);
}

void sys_chdir(tf_t *tf)
{
  char path[128];
  struct inode *ip;
  int pid, len;

  len = syscall_get_arg3(tf);
  pid = get_curid();

  if(len + 1 > 128)
    KERN_PANIC("sys_chdir path length greater than 128");

  pt_copyin(get_curid(), syscall_get_arg2(tf), path, len + 1);

  if((ip = namei(path)) == 0){
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }
  inode_lock(ip);
  if(ip->type != T_DIR){
    inode_unlockput(ip);
    syscall_set_errno(tf, E_DISK_OP);
    return;
  }
  inode_unlock(ip);
  inode_put(tcb_get_cwd(pid));
  tcb_set_cwd(pid, ip);
  syscall_set_errno(tf, E_SUCC);
}

void sys_ls(tf_t *tf){
  unsigned int u_buff;
  size_t n, written; // bytes to read; bytes read
  struct inode *cwd;

  // get args/init
  u_buff = syscall_get_arg2(tf);
  n = syscall_get_arg3(tf);
  cwd = tcb_get_cwd(get_curid());
  written = 0;

  // basic error check
  if(!u_buff || n < 0 || n > MAX_BUF)
    goto bad;
  else if (VM_USERLO > u_buff || u_buff + n > VM_USERHI || n > sizeof(k_buff))
    goto bad;

	spinlock_acquire(&k_buff_lock);

  // "clear" buffer
  k_buff[0] = '\0';

  // disk --> kernel memory
  struct dirent de;
  int r, off;
  for (off = 0; off < cwd->size; off += sizeof(de)) {
    r = inode_read(cwd, (char *) &de, off, sizeof(de));
    if (r != sizeof(de)) {
      KERN_PANIC("Bad inode_read in dir_link");
    }
    if(!strcmp(de.name, ".") || !strcmp(de.name, ".."))
      continue;
    if (de.inum == 0 || strnlen(de.name, MAX_BUF) + written >= MAX_BUF) {
      continue;
    }
    // copy in a space
    if(written > 0){
      k_buff[written] = ' ';
      written ++;
    }
    //KERN_DEBUG("name: %s\n", de.name);
    // copy into ls buffer
    strncpy(k_buff + written, de.name, MAX_BUF);
    written += strnlen(de.name, MAX_BUF);
  }

  // null terminate buffer
  k_buff[written + 1] = '\0';
  written++;

  // kernel memory --> user memory
  if (written > 0) {
    if (pt_copyout(k_buff, get_curid(), u_buff, written) != written) 
      goto bad;
  } 

	spinlock_release(&k_buff_lock);
	syscall_set_errno(tf, E_SUCC);
	syscall_set_retval1(tf, written);
  return;
bad:
	spinlock_release(&k_buff_lock);
  syscall_set_errno(tf, E_BADF);
	syscall_set_retval1(tf, -1);
  return ;
}

