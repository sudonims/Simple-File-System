#include "browse.c"

int main(int argc,char *argv[])
{
    char *name;
    name=argv[1];
    int fs_handle=open(name,O_RDWR);
    read(fs_handle,&sb,sizeof(super_t));
    browse(fs_handle);
}