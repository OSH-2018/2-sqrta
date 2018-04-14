#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#define END 1
#define CONTINUE 0
#define ERROR -1
#define LENGTH 50
#define MAX_INPUT 256
#define MAX_ORDERS_NUM 16
#define MAX_LENGTH 64

typedef struct cmd_order{ //文件重定向结构体
    char direct[5];
    char path[MAX_LENGTH];
}order;

int execute(char *args[]);
int run_cmd(int x);
int exe_cmd(char *cmd,int x);

int num,direct_num;//num为被管道符隔开的命令数，direct_num为重定向命令数
char cmds[MAX_ORDERS_NUM][MAX_LENGTH]={};//每一行分别装入被管道符隔开一条命令
order orders[4*MAX_ORDERS_NUM]={};//储存所有文件重定向命令
int cmd_redirect[MAX_ORDERS_NUM][2]={};//分别记录每条命令对应的文件重定向命令在数组orders中的位置，比如第0条命令有2个文件重定向，那么
                                    //cmd_redirect[0][0]=１,cmd_redirect[0][1]=2

int main(){
    char input[MAX_INPUT];

    int cnt=0;
    int i,j;
    while(1){
        memset(&cmd_redirect[0][0],0,sizeof(&cmd_redirect[0][0]));
        memset(&orders[0],0,sizeof(orders));
        printf("# ");
        fflush(stdin);
        /* 输入的命令行 */
        for (i=0;i<num;i++)
            for (j=0;j<64;j++)
                cmds[i][j]=0;
        fgets(input,MAX_INPUT,stdin);
        int flag=1;//输入是否合法
        int REDIRECT=0;
        for (i = 0; input[i]!='\n' && input[i];i++);
        input[i]='\0';
        for (num=j=i=0,direct_num=1;input[i];i++){
            //printf("i:%d  %d %c\n",i,input[i],input[i]);
            if (input[i]=='|'){
                if (input[i-1]!=' ' || input[i+1]!=' '){
                    flag=0;
                    break;
                }
                //if(num==0) printf("%d\n",direct_num);
                cmd_redirect[num][1]=direct_num;
                input[i-1]='\0';
                strcpy(cmds[num],input+j);
                i++;
                j=i+1;
                num++;
            }

            else if (input[i]=='>' || input[i]=='<'){

                int m=0;
                while (input[i-m]!=' '){
                    m++;
                }
                input[i-m]='\0';
                strcpy(cmds[num],input+j);
                if (!cmd_redirect[num][0]){
                    cmd_redirect[num][0]=direct_num;
                }
                m=0;

                if (input[i-1]=='0' || input[i-1]=='1' || input[i-1]=='2'){
                    orders[direct_num].direct[m]=input[i-1];
                    m++;
                }

                orders[direct_num].direct[m]=input[i];
                m++;

                if (input[i+1]==input[i]){
                    orders[direct_num].direct[m]=input[i+1];
                    i++;
                }
                i++;
                while(input[i]==' ') i++;

                for (m=0;input[i]!=' ' && input[i]!='\0' && input[i]!='\n';m++,i++){
                    orders[direct_num].path[m]=input[i];

                }
                direct_num++;

            }
        }
        cmd_redirect[num][1]=direct_num;
        if (direct_num<=1) strcpy(cmds[num],input+j);
        /*
        for (i=0;i<=num;i++){
            puts(cmds[i]);
            printf("%d %d\n",cmd_redirect[i][0],cmd_redirect[i][1]);
            for (j=cmd_redirect[i][0];j<cmd_redirect[i][1];j++)
                printf("%s %s\n",orders[j].direct,orders[j].path);
        }
*/

        if (!flag) continue;

        if (run_cmd(num) == END) break;

    }
}

int run_cmd(int x){
/*
run_cmd执行一条命令和在其之前的所有命令，
迭代实现管道，子进程做管道符之前的所有，父进程做当前一条
*/
    if (x<0) _exit(0);
    if (x==0 && cmd_redirect[0][0]==0){

        return exe_cmd(cmds[0],x);
    }
    int fd[2];
    int main_fd[2];
    pipe(main_fd);
    pid_t mpid=fork();
    if (mpid==0){
    /*
    创建两次进程是为了专门创建一个子进程来执行命令，
    由于执行过程中会关闭stdin，用子进程执行，
    父进程正常返回。否则会出现返回后stdin被关闭
    无法读取的问题。
    */
        if (pipe(fd)==0){
            pid_t pid = fork();
            if (pid == 0){
                close(fd[0]);
                close(fileno(stdout));
                dup2(fd[1],fileno(stdout));
                close(fd[1]);
                run_cmd(x-1);
                _exit(0);
            }
            else{


                close(fd[1]);
                //close(fileno(stdin));
                dup2(fd[0],fileno(stdin));
                //close(fd[0]);
                waitpid(pid, NULL, 0);
                exe_cmd(cmds[x],x);
                 _exit(0);
        }

    }
    }
    else{
        wait(NULL);
    }

    return CONTINUE;
}

int exe_cmd(char *cmd,int x) {
        //printf("%d %d\n",cmd_redirect[x][0],cmd_redirect[x][1]);
        /*根据每条命令的重定向命令修改输入输出方向*/
        if (cmd_redirect[x][0]){
            int j;
            for (j=cmd_redirect[x][0];j<cmd_redirect[x][1];j++){
                //printf("%s %s\n",orders[j].direct,orders[j].path);
                if (strcmp(orders[j].direct,">")==0){
                    freopen(orders[j].path,"w",stdout);
                }
                else if (strcmp(orders[j].direct,">>")==0){
                    freopen(orders[j].path,"a+",stdout);
                }
                else if (strcmp(orders[j].direct,"<")==0 || strcmp(orders[j].direct,"<<")==0){
                    freopen(orders[j].path,"r",stdin);
                }
            }
        }

    /* 命令行拆解成的各部分，以空指针结尾 */

    char *args[128];

        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n' && cmd[i];i++);

        cmd[i] = '\0';
        while(cmd[0]==' ' && cmd[0]){
            cmd++;
        }
        //printf("break\n");
        /* 拆解命令行 */

        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    while (*args[i+1] == ' '){
                        *args[i+1] = '\0';
                        args[i+1]++;
                    }

                    break;
                }
        args[i] = NULL;
        /* 没有输入命令 */

        if (!args[0])
            return CONTINUE;

        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1])
                chdir(args[1]);
            return CONTINUE;
        }
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            return CONTINUE;
        }

        if (strcmp(args[0],"export")==0){
            char *a=args[1];
            /* */
            while(*a!='=') a++;
            *a='\0';
            a=a+1;
            setenv(args[1],a,1);
            return CONTINUE;
        }



        if (strcmp(args[0], "exit") == 0)
            return END;

        /* 外部命令 */
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            execvp(args[0], args);
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        wait(NULL);
        return CONTINUE;
}
