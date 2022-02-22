/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */
#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

extern int atoi(char *buff);
static struct task_info *test_tasks[16];

/*struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};
    
struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};



static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task, &strgenerator_task};
static int num_test_tasks = 8;*/

#define SHELL_BEGIN 25

//Compare the Input with Now Commands
void parse_input(char *buf){
    char arg[15][15];       
    int i = 0; int j = 0;
    int num = 0;

    while(buf[j] != '\0' && buf[j] != ' '){
        arg[0][i++] = buf[j++];
    }
    arg[0][i] = '\0';
    for(i = 0; i < 15; i++){
        arg[1][i] = '\0';
    }
    if(buf[j++] == ' '){
        i = 0;
        while(buf[j] != '\0' && buf[j] != ' '){
            arg[1][i++] = buf[j++];
        }
        arg[1][i] = '\0';
        num = atoi(arg[1]);             //Get the Process Pid Which USER Want to Exec and Other
    }

    //Judge commands
    if(!strcmp(arg[0], "ps")){
        sys_process_show();
    }else if(!strcmp(arg[0], "clear")){
        sys_screen_clear();
        sys_move_cursor(1,SHELL_BEGIN);     //Move Cursor to the beginning of the shell
        printf("------------------- COMMAND -------------------\n");
    }else if(!strcmp(arg[0], "spawn")){
        pid_t pid;
        pid = sys_spawn(test_tasks[num], NULL, AUTO_CLEANUP_ON_EXIT);       //Auto Exit
        printf("spawn process[%d].\n",num);
        //sys_yield();
    }else if(!strcmp(arg[0], "kill")){
        int kill_status;
        int num1;
        if(buf[j++] == ' '){
            i = 0;
            while(buf[j] != '\0' && buf[j] != ' '){
                arg[1][i++] = buf[j++];
            }
            arg[1][i] = '\0';
            num1 = atoi(arg[1]);             //Get the Process Pid Which USER Want to Exec and Other
        }

        kill_status = sys_kill(num,num1);
        //Tell User If We Kill the Process Successfully
        if(kill_status == 1){
            printf("kill process[%d] thread[%d].\n",num, num1);
        }else{
            printf("Error: process[%d] thread[%d] is not running.\n",num,num1);
        }
    }else if(!strcmp(arg[0], "exit")){
        sys_exit();
    }else if(!strcmp(arg[0], "waitpid")){
        int wait_status = sys_waitpid(num);
        if(wait_status == 0){
            printf("Error: process[%d] is not running or not existing.",num);
        }else{
            printf("Waiting for process[%d]...", num);
        }
    }else if(!strcmp(arg[0], "exec")){
        int exec_pid = 0;
        int exec_argc = 0;
        char* exec_argv[5];
        int exec_i = 0;
        int l,k;
        k = 1;
        for(l = 2; l < 6; l++){
            if(buf[j++] == '\0'){
                break;
            }
            exec_i = 0;
            while(buf[j] != '\0' && buf[j] != ' '){
                arg[l][exec_i++] = buf[j++];
            }
            arg[l][exec_i] = '\0';
        }
        char *file_name = arg[1];
        exec_argv[0] = file_name;
        
        //printf("%s\n",file_name);
        for(int m = 2; m < l; m++){
            exec_argv[k] = arg[m];
            //printf("%s ",exec_argv[k]);
            k++;
        }
        //printf("\n");
        exec_pid = (int)sys_exec(file_name, l-1, exec_argv, AUTO_CLEANUP_ON_EXIT);
        if(exec_pid != 0){
            printf("exec process[%d]\n",exec_pid);
        }else{
            printf("No such Files\n");
        }
    }else if (!strcmp(arg[0],"ls")){
        if(!strcmp(arg[1], "-l")){
            sys_ls_l();
        }else{
            sys_ls(arg[1]);
        }
    }else if (!strcmp(arg[0],"mkfs"))
    {
        sys_mkfs();
    }else if (!strcmp(arg[0],"statfs"))
    {
        sys_statfs();
    }else if (!strcmp(arg[0],"mkdir"))
    {
        sys_mkdir(arg[1]);
    }else if (!strcmp(arg[0], "rmdir"))
    {
        sys_rmdir(arg[1]);
    }else if (!strcmp(arg[0], "cd")){
        sys_cd(arg[1]);
    }else if (!strcmp(arg[0], "touch"))
    {
        sys_touch(arg[1]);
    }else if(!strcmp(arg[0],"cat")){
        sys_cat(arg[1]);
    }else if (!strcmp(arg[0],"ln")){
        if(buf[j++] == ' '){
            i = 0;
            while(buf[j] != '\0' && buf[j] != ' '){
                arg[2][i++] = buf[j++];
            }
            arg[2][i] = '\0';
        }
        sys_ln(arg[1],arg[2]);
    }else if (!strcmp(arg[0],"rm"))
    {
        sys_rm(arg[1]);
    }
    
    else{
        printf("Unknown Command!\n");
    }
}


int main()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> root@GasaiYuno_OS: ");

    char buf[50];      //The Maximum input
    int i = 0;
    while (1)
    {   
        // TODO: call syscall to read UART port
        char c;
        int n;
        //No input. Spin and Wait
        while((n = sys_get_char()) == -1){
            ;
        }
        c = (char)n;
        if(c == '\r'){
            // TODO: parse input
            printf("\n");
            buf[i] = '\0';
            parse_input(buf);
            printf("> root@GasaiYuno_OS: ");
            i = 0;
        }else if(c == 8 || c == 127){
           // note: backspace maybe 8('\b') or 127(delete)
           i--;
           //printf("%c ",c);     //Exec the last input char with ' '
           printf("%c",c);      //Move the cursor
           //sys_put_char(c);
        }else{
            buf[i++] = c;
            printf("%c",c);
        }
    }
    return 0;
}

