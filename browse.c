#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include "structure.h"

/* WORK TO DO

 * fix bugs if any
 * go back in directory (cd -b)....Is it really needed?
 * directory structure...seems fine though
 * 
 */

super_t sb;
char directory[16];
char par_dir[16];

uint_t *used;
uint_t block[FS_BLOCK_SIZE];

void init(int fs_handle)
{
    int i,j;
    inode_t inode;
    used=(uint_t*)calloc(sb.part_size,sizeof(uint_t));
    if(!used)
    {
        printf("Memory full.\n");
        exit(1);
    }
    for(i=0;i<sb.data_blk_start;i++)
    {
        used[i]=1;
    }
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,&inode,sizeof(inode_t));
        if(!inode.name[0]) continue;
        for(j=0;j<BLOCK_COUNT;j++)
        {
            if(inode.blocks[j]==0) break;
            used[inode.blocks[j]]=1;
        }
    }
}
void shut(){
    free(used);
}

int get_free_block()
{
    int i;
    for(i=sb.data_blk_start;i<sb.part_size;i++)
    {
        if(used[i]==0)
        {
            used[i]=1;
            return i;
        }
    }
    return -1;
}

void reset_data_block(int i)
{
    used[i]=0;
}


inode_t find(char*nm,int fs_handle)
{
    inode_t inode,inode1={.type=-1};
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(int i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,&inode,sizeof(inode_t));
        if(!inode.name[0]){continue;}
        if(strcmp(inode.name,nm)==0 && strcmp(inode.dir,directory)==0)                  //BUG #1 changes directory even if not in right pwd
        {                                                                               //BUG #2 reference to inode doesn't work. inode_t*find(...)
            return inode;
        }
    }
    return inode1;
}

int lookup(int fs_handle,char *nm,inode_t *inode)
{
    int i;
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size,SEEK_SET);
    for(i=0;i<sb.inode_count;i++)
    {
        read(fs_handle,inode,sizeof(inode_t));
        if(!inode->name[0]) continue;
        if(strcmp(inode->name,nm)==0) return i;
    }
    return -1;
}

void change_directory(char *nm,int fs_handle)
{
    if(find(nm,fs_handle).type==2 || strcmp(nm,"root")==0)
    {
        strcpy(par_dir,directory);
        strcpy(directory,nm);
    }
    else if(find(nm,fs_handle).type==1)
    {
        printf("It is a file not a directory.\n");
    }
    else if(find(nm,fs_handle).type==-1)
    {
        printf("Directory not found\n");
    }
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
        if(strcmp(inode.name,nm)==0 && strcmp(inode.dir,directory)==0)
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
    inode_t inode;
    int i=lookup(fs_handle,nm,&inode),j;
    
    if(i==-1){
        printf("File doesn't exist.\n");
        return;
    }
    if(strcmp(inode.dir,directory)!=0)
    {
        printf("%s %s",inode.dir,directory);
        printf("File not found in current directory.Try changing directory\n");        //BUG #3 Prints this even if file is in directory sometimes..(makes no sense)
        return;
    }

    for(j=0;j<BLOCK_COUNT;j++)
    {
        if(!inode.blocks[j])
        {
            break;
        }
        reset_data_block(inode.blocks[j]);
    }
    memset(&inode,0,sizeof(inode_t));
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size + i*sb.inode_size,SEEK_SET);
    write(fs_handle,&inode,sizeof(inode_t));
}

void update(int fs_handle,char *nm,int *size,int *permissions)
{
    inode_t inode;
    int i=lookup(fs_handle,nm,&inode);
    if(i==-1){
        printf("File doesn't exist.\n");
        return;
    }

    if(size) inode.size=*size;
    if(permissions && (*permissions<=7)) inode.permissions=*permissions;
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size + i*sb.inode_size,SEEK_SET);
    write(fs_handle,&inode,sizeof(inode_t));
}

void fs_read(int fs_handle,char *nm)
{
    inode_t inode;
    int i=lookup(fs_handle,nm,&inode),j,done_read=0,rem_read,to_read;
    if(i==-1){
        printf("File doesn't exist.\n");
        return;
    }
    if(!(inode.permissions & 04))
    {
        printf("File doen't have read permissions.\n");
        return;
    }

    rem_read=inode.size;
    for(j=0;j<BLOCK_COUNT;j++)
    {
        if(!inode.blocks[j]) break;
        to_read=(rem_read>=sb.blk_size) ? sb.blk_size : rem_read;
        lseek(fs_handle,inode.blocks[j]*sb.blk_size,SEEK_SET);
        read(fs_handle,block,to_read);
        write(1,block,to_read);
        done_read+=to_read;
        rem_read-=to_read;
        if(!rem_read) break;
    }
}

void fs_write(int fs_handle,char *nm)
{
    inode_t inode;
    int i=lookup(fs_handle,nm,&inode);
    
    if(i==-1){
        printf("File doesn't exist.\n");
        return;
    }

    if(!(inode.permissions & 02))
    {
        printf("The file doesn't have write permissions.\n");
        return;
    }
    int cur_read,cur_read_i,to_read,total_size,j,free_block;

    for(j=0;j<BLOCK_COUNT;j++)
    {
        if(!inode.blocks[j])
        {
            break;
        }
        reset_data_block(inode.blocks[j]);
    }

    cur_read_i=0;
    to_read=sb.blk_size;
    total_size=0;
    j=0;

    while((cur_read=read(0,block+cur_read_i,to_read))>0)
    {
        if(cur_read==to_read)
        {
            if(j=BLOCK_COUNT) break;
            if((free_block=get_free_block())==-1) break;
            lseek(fs_handle,free_block*sb.blk_size,SEEK_SET);
            write(fs_handle,block,sb.blk_size);
            inode.blocks[j]=free_block;
            j++;
            total_size+= sb.blk_size;

            cur_read_i=0;
            to_read=sb.blk_size;
        }
        else
        {
            cur_read_i+=cur_read;
            to_read-=cur_read;
        }
    }

    if((cur_read<=0) && (cur_read_i))
    {
        if(j!=BLOCK_COUNT && ((inode.blocks[j]=get_free_block())!=-1))
        {
            lseek(fs_handle,inode.blocks[j]*sb.blk_size,SEEK_SET);
            write(fs_handle,block,cur_read_i);
            total_size+=cur_read_i;
        }
    }

    inode.size=total_size;
    lseek(fs_handle,sb.inode_tbl_blk_start*sb.blk_size+i*sb.inode_size,SEEK_SET);
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
                    ctime((time_t *)&inode.timestamp));                                     //BUG #4 time and date is wrong.
        }
    }
}

void browse(int fs_handle)
{
    int flag=0;
    char cmd[256],*fn;
    strcpy(directory,"root");
    strcpy(par_dir,"null");
    init(fs_handle);
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
                if(*fn=='-'&&*(fn+1)=='r')
                {
                    fn+=2;
                    while(*fn==' ') fn++;
                    remove_file(fs_handle,fn);
                }
                if(find(fn,fs_handle).type==2)
                    printf("A directory.Check command\n");
                else if(find(fn,fs_handle).type==1)
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
                change_directory(fn,fs_handle);
            }
        } else if(strncmp(cmd,"mkdir",5)==0){ //creates directory
            if(cmd[5]==' ')
            {
                fn=cmd+5;
                while(*fn==' '){ fn++; }
                create_file(fs_handle,fn,2);
            }
        } else if(strncmp(cmd,"read",4)==0){
            if(cmd[4]==' ')
            {
                fn=cmd+4;
                while(*fn==' '){ fn++; }
                fs_read(fs_handle,fn);
            }
        } else if(strncmp(cmd,"write",5)==0){
            if(cmd[5]==' ')
            {
                fn=cmd+5;
                while(*fn==' '){ fn++; }
                fs_write(fs_handle,fn);
            }
        } else if(strncmp(cmd,"chmod",5)==0){
            if(cmd[5]==' ')
            {
                fn=cmd+5;
                while(*fn==' '){ fn++; }
                char a[15];
                int i=0;
                while(*fn!=' ')
                {
                    a[i++]=*(fn++);
                }
                printf("%s %s",a,fn);
                int p=atoi(fn);
                update(fs_handle,a,NULL,&p);
            }
        }
        else {
            printf("Incorrect command. Try 'help'\n");
        }
    }
}
