#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
#include<linux/fs.h>
#include "structure.h"

#define INODE_RATIO 0.10

super_t sb={
    .magic=MAGIC,
    .blk_size=FS_BLOCK_SIZE,
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
	long long c = 0;
	lseek(sfs_handle,sb->part_size * sb->blk_size - 1, SEEK_SET);
	write(sfs_handle,&c, 1);
}

int main(int argc, char *argv[])
{
	int sfs_handle;
	size_t size;
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <file_name>",argv[0]);
		return 1;
	}

	sfs_handle=open(argv[1],O_RDWR);
	if(sfs_handle==-1)
	{
		fprintf(stderr,"Error formatting the device.\n");
		return 2;
	}

	ioctl(sfs_handle,BLKGETSIZE64,&size);
	sb.part_size =size/FS_BLOCK_SIZE;
	sb.inode_table_size = sb.part_size * INODE_RATIO;
	sb.inode_count = sb.inode_table_size * sb.blk_size / sb.inode_size;
	sb.data_blk_start = 1 + sb.inode_table_size;

	// sfs_handle = creat(argv[2],S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	// if (sfs_handle == -1)
	// {
	// 	perror("No permissions to format");
	// 	return 2;
	// }

	fflush(stdout);
	write_super_block(sfs_handle,&sb);
	clear_file_entries(sfs_handle, &sb);
	mark_data_blocks(sfs_handle, &sb);
	close(sfs_handle);
	printf("Flashing success.\n");
	return 0;
}