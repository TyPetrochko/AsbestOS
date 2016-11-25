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

  if(dp->type != T_DIR)
    KERN_PANIC("dir_lookup not DIR");

  //TODO
  for (off = 0; off < dp->size; dp += sizeof(de)) {
    read_value = inode_read(dp, (char *) de, off, dp->size);
    if (read_value != sizeof(de)) {
      KERN_PANIC("Bad lookup!");
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
  // TODO: Check that name is not present.

  // TODO: Look for an empty dirent.
  
  return 0;
}
