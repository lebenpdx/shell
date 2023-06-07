#define maxhistory 15
#define maxarg 256
#define maxcmdlen 100
#define promptlen 255
#define PROMPT_STR "PSUsh"
#define maxpipe 64

void handle_signal(int);
void execmd(char *cmd);
void options(int argc, char *argv[]);
