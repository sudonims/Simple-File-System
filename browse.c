#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include "structure.h"

super_t sb;
char directory[16];
char par_dir[16];

void change_directory(char *nm)
{
    strcpy(par_dir,directory);
    strcpy(directory,nm);
}

void create_file(int fs_handle,char *nm,int type)
{
    int i;
    inode_t inode;
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,&inode,sizeof(inode_t));
        if(!inode.name[0]){ break; }
        if(strcmp(inode.name,nm)==0)
        {
            printf("File already exist\n");
            return ;
        }
    }
    if(i==sb.inode_count)
    {
        printf("FUll\n");
        return;
    }

    lseek(fs_handle,-(off_t)(sb.inode_size),SEEK_CUR);
    strncpy(inode.name,nm,15);
    inode.name[15]=0;
    strcpy(inode.dir,directory);
    inode.type=type;  //1->file 2->directory
    inode.size=0;
    inode.permissions=07;
    inode.timestamp=time(NULL);
    for(i=0;i<BLOCK_COUNT;i++)
    {
        inode.blocks[i]=0;
    }
    write(fs_handle,&inode,sizeof(inode_t));
}

void remove_file(int fs_handle,char *nm)
{
    int i;
    inode_t inode;
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,&inode,sizeof(inode_t));
        if(inode.name==0)
            continue;
        if(strcmp(nm,inode.name)==0)
            break;
    }
    if(i==sb.inode_count)
    {
        printf("File not found\n");
        return ;
    }
    if(strcmp(inode.dir,directory)!=0)
    {
        printf("File not found in current directory.Try changing directory\n");
        return;
    }
    lseek(fs_handle,-(off_t)(sb.inode_size),SEEK_CUR);
    memset(&inode,0,sizeof(inode_t));
    write(fs_handle,&inode,sizeof(inode_t));
}

void list(int fs_handle)
{
    int i;
    inode_t inode;

    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,&inode,sizeof(inode_t));
        if(!inode.name[0])
        {
            continue;
        }
        if(strcmp(inode.dir,directory)==0)
        {
            printf("%-15s   %10d bytes  %d  %c%c%c  %s",inode.name,inode.size,inode.type,
                    inode.permissions&04 ? 'r' : '-',
                    inode.permissions&02 ? 'w' : '-',
                    inode.permissions&01 ? 'x' : '-',
                    ctime((time_t *)&inode.timestamp));
        }
    }
}

void browse(int fs_handle)
{
    int flag=0;
    char cmd[256],*fn;
    strcpy(directory,"root");
    strcpy(par_dir,"null");
    while (!flag)
    {
        printf("[ root@fs ]$ "); //Shell drop
        scanf(" %[^\n]s",cmd);
        if(strcmp(cmd,"exit")==0){
            flag=1;          //loop break condition
        } else if(strncmp(cmd,"cr",2)==0) {
            if(cmd[2]==' ')
            {
                fn=cmd+2; //skips the spaces bet cmd and args
                while(*fn==' ')
                    fn++;
                create_file(fs_handle,fn,1);
            }
        } else if(strncmp(cmd,"rm",2)==0) {
            if(cmd[2]==' ')
            {
                fn=cmd+2; //skips spaces
                while(*fn==' ') fn++;
                remove_file(fs_handle,fn);
            }
        } else if(strcmp(cmd,"ls")==0) {
            list(fs_handle);
        } else if(strcmp(cmd,"help")==0) {  //help window..
            printf("help: help,opens this page.\n");
            printf("ls: ls , list all the files\n");
            printf("mkdir: mkdir <dir_name> , creates new directory\n");
            printf("cd: cd <dir_name> , changes the working directory\n");
            printf("cr: cr <file_name> , creates a file with given name\n");
            printf("rm: rm <file_name> , removes the given file\n");
            printf("exit: exit, quits the fs shell.\n");
        } else if(strncmp(cmd,"cd",2)==0) {  //changes directory
            if(cmd[2]==' ')
            {
                fn=cmd+2; //skips spaces 
                while(*fn==' ') { fn++ ;}
                change_directory(fn);
            }
        } else if(strncmp(cmd,"mkdir",5)==0){ //creates directory
            if(cmd[5]==' ')
            {
                fn=cmd+5;
                while(*fn==' '){ fn++; }
                create_file(fs_handle,fn,2);
            }
        }
    }
}
