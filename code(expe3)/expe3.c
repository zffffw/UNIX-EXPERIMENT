#include <apue.h>
#include <sys/types.h>
#include <myerror.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#define MAX_PAR 100
#define MAX_L 256

int split_paramaters(char* buf, char** argv) {
    int i = 0;
    int subi = 0;
    int para_num = 0;
    while(buf[i]) {
        if (buf[i] != ' ' && buf[i] != '\t' && buf[i]) {
            subi = 0;
            char buff[MAX_L];
            while(buf[i] && buf[i] != ' ' && buf[i] != '\t' ) {
                buff[subi++] = buf[i++];
            }
            *(argv + para_num) = (char *) malloc(sizeof(char)*(strlen(buff) + 1));
            memset(*(argv + para_num), 0, sizeof(char)*(strlen(buff) + 1));
            for(int j = 0; j < strlen(buff); ++j) {
                argv[para_num][j] = buff[j];
            }
            argv[para_num][subi] = 0;
            para_num += 1;
        }
        i++;
    }
    return para_num - 1;
}

char* search_file(char* argv1) {
    char env_path[3][MAX_L] = {"/usr/bin", "/usr/local/bin", "/bin"}; //模拟环境变量
    for (int i = 0; i < 3; ++i) {
        // printf("%s\n", env_path[i]);
        DIR *dir;
        struct dirent *dirp;
        if ((dir = opendir(env_path[i])) == NULL) {
            printf("cannot open dir:%s\n", env_path[i]);
        }
        env_path[i][strlen(env_path[i])] = '/';
        env_path[i][strlen(env_path[i]) + 1] = 0;
        while ((dirp = readdir(dir)) != NULL) {
            if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
                    continue;		/* ignore dot and dot-dot */
            // printf("%s\n", dirp->d_name);
            if (strcmp(dirp->d_name, argv1) == 0) {
                char* res_path;
                strcpy(&env_path[i][strlen(env_path[i])], dirp->d_name);	/* append name after "/" */
                res_path = (char *)malloc(sizeof(char)*(strlen(env_path[i]) + 1));
                strcpy(res_path, env_path[i]);
                return res_path;
            }
            
        }
	}
    return argv1;
}

int main() {
    char buf[MAXLINE] = {0};
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    char current_dir[MAXLINE] = {0};
    getcwd(current_dir, MAXLINE);
    pid_t pid;
    int status;
    printf("%s:%s$ ", pwd->pw_name, current_dir);
    while(fgets(buf, MAXLINE, stdin) != NULL) {
        /* 符号\ 可以换行， 模拟更真实的shell */
        while(buf[strlen(buf) - 2] == '\\') {
            printf("> ");
            fgets(buf + strlen(buf) - 2, MAXLINE, stdin);
        }
        buf[strlen(buf) - 1] = 0;
        char* argv[MAX_L];
        int para_num = split_paramaters(buf, argv);
        argv[para_num + 1] = NULL;
        char* respath = search_file(argv[0]);
        argv[0] =respath;
        // printf("%s\n", respath);
        if ( (pid = fork()) < 0) {
            err_sys("fork error");
        } else if (pid == 0) {
            execve(argv[0], argv, NULL);
            err_ret("couldn't execute: %s", buf);
            exit(127);
        }
        if ( (pid = waitpid(pid, &status, 0)) < 0) {
            err_sys("wait pid error");
        } 
        /* 对切换工作文件夹的命令进行真实切换 */
        if(strcmp(argv[0], "/usr/bin/cd") == 0) {
            chdir(argv[1]);
        }
        pwd = getpwuid(getuid());
        getcwd(current_dir, MAXLINE);
        printf("%s:%s$ ", pwd->pw_name, current_dir);
    }
    exit(0);
}