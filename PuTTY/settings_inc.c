#include <winsock2.h>
#include <windows.h>
const char * appname="PuTTY";
int be_default_protocol= PROT_SSH;
extern Backend ssh_backend;
Backend * backends[]={&ssh_backend,NULL};

