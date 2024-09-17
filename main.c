#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "nfd.h"
#include <dirent.h>
#include <fcntl.h>

#define PARTIAL 1
#define TOTAL 0

int _running;
char** me_a; int me_c; // extensions to move
char** de_a; int de_c; // extensions to delete
char** mf_a; int mf_c; // files to move
char** ig_a; int ig_c; // files to ignore
char** df_a; int df_c; // files to delete

void printHelp(){
    printf("%s\n","Usage: watchdrop [-filters] [scan dir] [command (optional)] [ ... command args (optional)]");
    printf("%s\n"," Continuously monitors a folder and handles files according to given criteria.");
    printf("%s\n"," -me   Moves the file into an user specified by gui folder");
    printf("%s\n","       if its name consist partialy with value\n");

    printf("%s\n"," -mf   Moves the file into an user specified by gui folder");
    printf("%s\n","       if its name is equal than value\n");

    printf("%s\n"," -de   Deletes the file if its name consist partially with value");
    printf("%s\n"," -df   Deletes the file if its name is equal than value");
    printf("%s\n"," -ig   Ignores a file even if it fits other params criteria, only if its name");
    printf("%s\n","       is equal than value\n");


}

int moveFile(char* inPath, char* fileName){
    NFD_Init();
    int returnCode;
    nfdchar_t* outPath;
    nfdresult_t result = NFD_SaveDialogN(&outPath,NULL,0,NULL,fileName);

    switch(result) {
        case(NFD_OKAY):
            if(rename(inPath,outPath) == -1){
                int from = open(inPath,O_RDONLY);
                int to = open(outPath,O_WRONLY | O_CREAT | 0644);
                if(from == -1 || to == -1) returnCode=-1; goto end;
                

                size_t from_s = lseek(from,0,SEEK_END);
                if(from_s == -1){
                    close(from);
                    close(to);
                    returnCode=-2; goto end;
                }
                
                size_t bytesT = 0;
                while(bytesT < from_s){
                    int result = sendfile(to,from,NULL,from_s - bytesT);
                    if(result == -1){
                        close(from);
                        close(to);
                        returnCode=-3; goto end;
                    }

                    bytesT += result;
                }

                close(from);
                close(to);
                if(remove(inPath) == -1){
                    returnCode=-4; goto end;
                }
            }
            returnCode=NFD_OKAY; goto end;

        case(NFD_CANCEL):
            if(remove(inPath) == -1){
                returnCode=-5; goto end;
            }
            returnCode=NFD_CANCEL; goto end;

        case(NFD_ERROR):
            printf("%s\n",NFD_GetError());
            returnCode=NFD_ERROR; goto end;
    }

    end:
    if(result == NFD_OKAY) NFD_FreePathN(outPath);
    NFD_Quit();
    return returnCode;
}

int isInCategory(char* fileName,char** category, int partial){
    int i=0;
    while(*(category+i) != NULL){
        char* item = *(category+i);
        if(strcasestr(fileName,item) != NULL && partial) return i+1;
        if(strcmp(item,fileName) == 0) return i+1;
        i++;
    }

    return 0;
}

int isAFlag(char* argn, char* flag, int* i, int* c){
    if(strcmp(argn,flag) == 0){
        *i += 1;
        *c = *i;
        return 0;
    }
    return 1;
}

int sortFlags(int argsc ,int flagsArg_c, char** args, char* conf_n, int* conf_c, char*** conf){
    for(int i=1; i<flagsArg_c; i++){
        char* arg = *(args+i);
        if(strcmp(conf_n,arg) == 0){
            *conf_c += 1;
        }
    }

    *conf = (char**) malloc(*conf_c * sizeof(char*));
    int conf_index = 0;
    for(int i=1; i<flagsArg_c; i++){
        char* arg = *(args+i);
        if(strcmp(conf_n,arg) == 0 && i+1<argsc){
            *(*conf + conf_index) = *(args+i+1);
            conf_index++;
        }
        
    }

    return 0;
}

int main(int argCount, char** args){
    if(argCount == 1){
        printHelp();
        return 1;
    }

    int flagC = 0;
    for(int i=1; i<argCount; i++){
        char* arg = *(args+i);
        if(isAFlag(arg,"-me",&i,&flagC) == 0) continue;
        if(isAFlag(arg,"-de",&i,&flagC) == 0) continue;
        if(isAFlag(arg,"-mf",&i,&flagC) == 0) continue;
        if(isAFlag(arg,"-ig",&i,&flagC) == 0) continue;
        if(isAFlag(arg,"-df",&i,&flagC) == 0) continue;
    }

    sortFlags(argCount, flagC+1, args, "-me", &me_c, &me_a);
    sortFlags(argCount, flagC+1, args, "-de", &de_c, &de_a);
    sortFlags(argCount, flagC+1, args, "-mf", &mf_c, &mf_a);
    sortFlags(argCount, flagC+1, args, "-ig", &ig_c, &ig_a);
    sortFlags(argCount, flagC+1, args, "-df", &df_c, &df_a);

    if(argCount == flagC+1){ // there is no folder to monitor
        printHelp();
        return 1;
    }

    char* mdir = *(args+flagC+1);
    char* command = NULL;
    char** command_args = NULL;
    pid_t childProcess;

    // if there's a command
    if(argCount > flagC+1){
        command = *(args + flagC + 2);
        
        // if there are args
        if(argCount > flagC+2){
            int command_argsQ = argCount-(flagC+2);
            command_args = (char**) malloc((command_argsQ+1) * sizeof(char*));
            *command_args = command;

            int o=1;
            for(int i=flagC+3; i<argCount;  i++){
                *(command_args+o) = *(args + i);
                o++;
            }

            *(command_args+command_argsQ) = NULL;
        }
        

        childProcess = fork();
        if(childProcess == 0){
            if(execvp(command,command_args) == -1){
                fprintf(stderr,"child process failed due to: %s\n",strerror(errno));
                return 1;
            }
        }

    }

    DIR* cwd_stream = opendir(mdir);
    if(cwd_stream == NULL){
        fprintf(stderr,"monitor dir could not be opened due to: %s\n",strerror(errno));
        return 1;
    }

    _running = 1;
    while(_running){
        rewinddir(cwd_stream);
        struct dirent* cwd_entry;
        
        while(cwd_entry = readdir(cwd_stream), cwd_entry != NULL){
            int c=2;
            if(*(mdir+(strlen(mdir)-1)) == '/') c--;
            char* fileName = (char*) malloc(strlen(mdir) + strlen(cwd_entry->d_name) + c);
            if(c==2) sprintf(fileName,"%s/%s",mdir,cwd_entry->d_name);
            else sprintf(fileName,"%s%s",mdir,cwd_entry->d_name);

            if((isInCategory(fileName,de_a,PARTIAL) ||
                isInCategory(fileName,df_a,TOTAL) ) &&
                !isInCategory(fileName,ig_a,TOTAL)){
                    if(remove(fileName) != 0){
                        fprintf(stderr,"%s could not be removed due to : %s\n", fileName, strerror(errno));
                    }
                    else continue;
            } 

            else if((isInCategory(fileName,me_a,PARTIAL) ||
                isInCategory(fileName,mf_a,TOTAL) ) && 
                !isInCategory(fileName,ig_a,TOTAL)){
                    while(1){
                        nfdresult_t result = moveFile(fileName, cwd_entry->d_name);
                        if(result == NFD_OKAY || result == NFD_CANCEL) break;
                        fprintf(stderr,"%s\n",NFD_GetError());
                    }
                    
                    continue;
            }

            free(fileName);
        }

        if(command != NULL){
            int status = 0;
            int result = waitpid(childProcess,&status,WNOHANG);
            if(result != 0) return 0;
        }

        sleep(1);
    }

    closedir(cwd_stream);
    free(me_a);
    free(de_a);
    free(mf_a);
    free(ig_a);
    free(df_a);
    free(command_args);

    return 0;
}