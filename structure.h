#ifndef STRUCTURE_H
#define STRUCTURE_H

#define MAGIC 0x1304ABCD
#define BLOCK_SIZE 512 //bytes
#define INODE_SIZE 128 //bytes
#define BLOCK_COUNT ((INODE_SIZE - (48+4*4)) / 4) 

typedef unsigned int uint_t;
typedef unsigned char uchar_t;

typedef struct superblock
{
  uint_t magic;
  uint_t part_size;
  uint_t blk_size;
  uint_t inode_size;
  uint_t inode_table_size;
  uint_t inode_tbl_blk_start;
  uint_t inode_count;
  uint_t data_blk_start;
  uint_t reserved[BLOCK_SIZE/4 -8];
}super_t;

typedef struct inode
{
  char name[16];
  char parent_dir[16];
  char dir[16];
  uint_t type;
  uint_t size;
  uint_t timestamp;
  uint_t permissions;
  uint_t blocks[BLOCK_COUNT];
}inode_t;


#endif
