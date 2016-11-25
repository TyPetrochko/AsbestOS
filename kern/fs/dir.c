#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include "inode.h"
#include "dir.h"

// Directories

int
dir_namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

/**
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 */
struct inode*
dir_lookup(struct inode *dp, char *name, uint32_t *poff)
{
  uint32_t off, inum;
  struct dirent de;
  int read_value;
  int dir_entry_size = sizeof(de);

  if(dp->type != T_DIR)
    KERN_PANIC("dir_lookup not DIR");

  //TODO
  for (off = 0; off < dp->size; dp += dir_entry_size) {
    read_value = inode_read(dp, (char *) &de, off, dir_entry_size);
    if (read_value != dir_entry_size) {
      KERN_PANIC("Bad inode_read in dir_lookup");
    } else if (de.inum == 0) {
      //not allocated
      continue;
    } else if (dir_namecmp(name, de.name) == 0) {
      *poff = off;
      inum = de.inum;
      return inode_get(dp->dev, inum);
    }
  }
  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int
dir_link(struct inode *dp, char *name, uint32_t inum)
{
  struct inode * node;
  uint32_t * poff;
  uint32_t off;
  struct dirent de;
  int read_value, write_value;
  int dir_entry_size = sizeof(de);
  // TODO: Check that name is not present.
  node = dir_lookup(dp, name, poff);
  if (node) {
    inode_put(node);
    return -1;
  }

  // TODO: Look for an empty dirent.
  for (off = 0; off < dp->size; off += dir_entry_size) {
    read_value = inode_read(dp, (char *) &de, off, dir_entry_size);
    if (read_value != dir_entry_size) {
      KERN_PANIC("Bad inode_read in dir_link");
    } else if (de.inum == 0) {
      break;
    }
  }

  if (de.inum != 0) {
    KERN_PANIC("No unallocated blocks in dir_link");
  }
  //found block, copy in info
  de.inum = inum;
  strncpy(de.name, name, DIRSIZ);
  write_value = inode_write(dp, (char *) &de, off, dir_entry_size);
  if (write_value != dir_entry_size) {
    KERN_PANIC("Bad inode_write in dir_link");
  }
  
  return 0;
}
