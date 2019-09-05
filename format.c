#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "structure.h"

#define INODE_RATIO 0.10

super_t sb={
    .magic=MAGIC,
    .blk_size=BLOCK_SIZE,
    .inode_size=INODE_SIZE,
    .inode_tbl_blk_start=1
};

inode_t inode;

void write_super_block(int sfs_handle, super_t *sb)
{
	write(sfs_handle, sb, sizeof(super_t));
}

void clear_file_entries(int sfs_handle, super_t *sb)
{
	int i;
	for (i = 0; i < sb->inode_count; i++)
	{
		write(sfs_handle, &inode, sizeof(inode));
	}
}

void mark_data_blocks(int sfs_handle, super_t *sb)
{
	char c = 0;
	lseek(sfs_handle,sb->part_size * sb->blk_size - 1, SEEK_SET);
	write(sfs_handle, &c, 1);
}

int main(int argc, char *argv[])
{
	int sfs_handle;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <partition_size> <file_name>",argv[0]);
		return 1;
	}
	sb.part_size = atoi(argv[1]);
	sb.inode_table_size = sb.part_size * INODE_RATIO;
	sb.inode_count = sb.inode_table_size * sb.blk_size / sb.inode_size;
	sb.data_blk_start = 1 + sb.inode_table_size;

	sfs_handle = creat(argv[2],S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (sfs_handle == -1)
	{
		perror("No permissions to format");
		return 2;
	}
	write_super_block(sfs_handle, &sb);
	clear_file_entries(sfs_handle, &sb);
	mark_data_blocks(sfs_handle, &sb);
	close(sfs_handle);
	return 0;
}