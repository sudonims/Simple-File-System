/*Operating System Project
  Run this file with parameters*/



#include "browse.c"
  
int main(int argc,char *argv[])
{
    char *name;
    name=argv[1];
    int fs_handle=open(name,O_RDWR);
    read(fs_handle,&sb,sizeof(super_t));
    if(sb.magic==MAGIC)
        browse(fs_handle);
    else
        printf("FIle system invalid,Cannot access.\n");
}
