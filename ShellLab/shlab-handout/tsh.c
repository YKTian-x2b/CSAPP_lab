/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

// kai mod
#define _XOPEN_SOURCE

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

volatile sig_atomic_t flag_pid;
volatile sig_atomic_t job_cnt = 0;
volatile sig_atomic_t stop_fg = 0;
volatile sig_atomic_t int_fg = 0;

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(const char s[]){
    int i = 0;
    while (s[i] != '\0')
        ++i;
    return i;
}
 /* Put string */
ssize_t sio_puts(const char s[]){
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}

/* Put error message and exit */
void sio_error(char s[]){
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}

ssize_t Sio_puts(const char s[]){
    ssize_t n;
    if ((n = sio_puts(s)) < 0)
	    sio_error("Sio_puts error");
    return n;
}

ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

pid_t Fork(void) {
    pid_t pid;
    if ((pid = fork()) < 0)
	    unix_error("Fork error");
    return pid;
}

void Execve(const char *filename, char *const argv[], char *const envp[]) {
    if (execve(filename, argv, envp) < 0){
        Sio_puts(filename);
        Sio_puts(": Command not found\n");
        _exit(1);
    }
}

pid_t Waitpid(pid_t pid, int *iptr, int options) {
    pid_t retpid;
    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	    unix_error("Waitpid error");
    return retpid;
}

void Kill(pid_t pid, int signum) 
{    
    int rc;
    if ((rc = kill(pid, signum)) < 0)
	    unix_error("Kill error");
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset){
    if (sigprocmask(how, set, oldset) < 0)
	    unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set){
    if (sigemptyset(set) < 0)
	    unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set){ 
    if (sigfillset(set) < 0)
	    unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum){
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

// 信号安全strcmp
int kai_strcmp(const char *s1, const char *s2)
{
    assert(NULL != s1);
	assert(NULL != s2);       // 防御性编程
    const char * str1 = s1;
    const char * str2 = s2;
	while (*str1 == *str2){  
        if(*str1 == '\0')
        {
            return 0;
		}
        str1++; 
		str2++;
	}
	return *str1 - *str2;
}



void eval(char *cmdline) 
{
    char * argv[MAXARGS];
    int bg = parseline(cmdline, argv);
    // 没命令则直接返回
    if(argv[0] == NULL)         return;

    pid_t pid;
    flag_pid = 0;
    // 如果是内置命令就在builtin_cmd()里立即执行，然后返回，它就是tsh进程
    if(!builtin_cmd(argv)){
        sigset_t mask_all, mask_child, mask_none;
        Sigfillset(&mask_all);
        Sigemptyset(&mask_child);
        Sigemptyset(&mask_none);
        Sigaddset(&mask_child, SIGCHLD);
        // 简单屏蔽SIGCHLD 就能避免在addjob前deljob
        Sigprocmask(SIG_BLOCK, &mask_child, NULL);
        // 否则在子进程上下文中执行命令
        if((pid = Fork()) == 0){
            // 子进程应该有 独立的不同于当前tsl进程组的PGID
            setpgid(0, 0);
            // 子进程里恢复阻塞位向量的状态 避免子进程不能接收信号
            Sigprocmask(SIG_SETMASK, &mask_none, NULL);
            Execve(argv[0], argv, environ);
        }
        int status = bg == 1 ? BG : FG;
        // 阻塞所有信号 直到waitfg临时解除阻塞并接收各种信号，以维持前台进程的唯一性和同步执行
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        // 所有jobs相关的工作都要阻塞所有信号，以避免handler和tsh的竞争
        addjob(jobs, pid, status, cmdline);
        job_cnt++;
        // 这里是父进程tsh 处理非内置命令子进程组
        if(bg){
            // 后台子进程组任务完成时会自动发送SIGCHLD通知shell，再收割
            printf("[%d] (%d) %s", job_cnt, pid, cmdline);  
        }
        else{
            // 等待子进程组前台任务终止
            waitfg(pid);
        }
    }
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';              // 它把源地址空间的每个argv都用'\0'划分出来，这样就能以字符串的形式区分且不用复制 精彩！
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if(!kai_strcmp(argv[0], "quit")){
        _exit(0);
    }
    else if(!kai_strcmp(argv[0], "jobs")){
        sigset_t mask, prev_mask;
        Sigfillset(&mask);
        Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
        listjobs(jobs);
        return 1;
    }
    else if(!kai_strcmp(argv[0], "bg") || !kai_strcmp(argv[0], "fg")){
        do_bgfg(argv);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *curr_job = NULL;
    pid_t pid = 0;
    int jid = 0;
    sigset_t mask, prev_mask;
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    // 命令行不规范检测 不涉及jobs的访问可以放到 阻塞信号函数Sigprocmask 外
    if(!argv[1]) {
        printf("%s: command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    if(argv[1][0] == '%'){
        jid = atoi(&argv[1][1]);
        if(jid == 0){
            printf("%%%d: No such job\n", jid);
            return;
        }
        curr_job = getjobjid(jobs, jid);
        if(!curr_job){
            printf("%%%d: No such job\n", jid);
            return;
        }
        pid = curr_job->pid;
    }
    else{
        pid = atoi(argv[1]);
        if(pid == 0){
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }
        if(pid > 32767){
            printf("(%d): No such process\n", pid);
            return;
        }
        jid = pid2jid(pid);
        curr_job = getjobjid(jobs, jid);
        if(!curr_job){
            printf("%%%d: No such job\n", jid);
            return;
        }
    }
    if(!kai_strcmp(argv[0], "bg")){
        // The bg <job> command restarts <job> by sending it a SIGCONT signal, 
        // and then runs it in the background. 
        // The <job> argument can be either a PID or a JID.
        curr_job->state = BG;
        Kill(-pid, SIGCONT); 
        printf("[%d] (%d) %s", job_cnt, pid, curr_job->cmdline);  
    }
    else if(!kai_strcmp(argv[0], "fg")){
        // The fg <job> command restarts <job> by sending it a SIGCONT signal, 
        // and then runs it in the foreground. 
        // The <job> argument can be either a PID or a JID.
        curr_job->state = FG;
        Kill(-pid, SIGCONT);
        waitfg(pid);
    }
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    sigset_t mask;
    Sigemptyset(&mask);
    // tsh进程等待 子进程 暂停/终止/自然退出 时发出的信号被sigchld_handler捕获，并破坏该循环
    // 分别对应 stop_fg/int_fg/flag_pid 
    while(!flag_pid){
        Sigsuspend(&mask);
        // 尽可能在tsh进程内处理复杂事务，降低handler复杂性
        if(int_fg){
            int_fg = 0;
            deletejob(jobs, pid);
            job_cnt--;
            break; 
        }
        if(stop_fg){
            stop_fg = 0;
            for (int i = 0; i < MAXJOBS; i++){
                if (jobs[i].pid == pid){
                    jobs[i].state = ST;
                }
            }
            break; 
        }
    }
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
// 保持handler简单： 只做必要的工作，比如只做全局标志的设置，信号的接收处理工作都在主程序执行
// 使用volatile声明全局变量 使用原子变量sig_atomic_t

// 为了确保 控制台和其他进程发送给子进程的信号 都能被tsh进程获知
// 我们把对信号的事务处理，延迟到子进程状态改变时，而不是在接收到信号时
// 因为子进程状态改变 tsh进程可以获悉。而其他进程对子进程的信号发送，tsh进程不知道
void sigchld_handler(int sig) 
{
    // 保存和恢复errno
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask, prev_mask;
    Sigfillset(&mask);
    // waitpid要用的讲究一点
    // 立即返回 如果集合中的子进程都没有被停止或终止则返回0，如果有一个停止或终止，则返回该进程的pid
    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
        Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
        // 未被捕获的信号导致子进程终止
        if(WIFSIGNALED(status)){
            int_fg = 1;
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, SIGINT);
        }
        // 子进程暂停
        if(WIFSTOPPED(status)){
            stop_fg = 1;
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, SIGTSTP);
        }
        // 前台进程死亡
        flag_pid = pid == fgpid(jobs) ? 1:0;
        // 如果是正常退出(如后台进程结束) 则在这儿处理
        if(WIFEXITED(status)){
            deletejob(jobs, pid);
            job_cnt--;
        }
        Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }
    errno = olderrno;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask, prev_mask;
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    pid_t pid = fgpid(jobs);
    if(pid == 0)    return;
    Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    // 只是转发信号，不做业务处理
    Kill(-pid, SIGINT);
    errno = olderrno;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask, prev_mask;
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    pid_t pid = fgpid(jobs);
    if(pid == 0)    return;
    Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    // 只是转发信号，不做业务处理
    Kill(-pid, SIGTSTP);
    errno = olderrno;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	    unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



// ./sdriver.pl -t trace_kai.txt -s ./tsh -a "-p"