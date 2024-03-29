#include "shell.h"

typedef struct proc {
  pid_t pid;    /* process identifier */
  int state;    /* RUNNING or STOPPED or FINISHED */
  int exitcode; /* -1 if exit status not yet received */
} proc_t;

typedef struct job {
  pid_t pgid;            /* 0 if slot is free */
  proc_t *proc;          /* array of processes running in as a job */
  struct termios tmodes; /* saved terminal modes */
  int nproc;             /* number of processes */
  int state;             /* changes when live processes have same state */
  char *command;         /* textual representation of command line */
} job_t;

static job_t *jobs = NULL;          /* array of all jobs */
static int njobmax = 1;             /* number of slots in jobs array */
static int tty_fd = -1;             /* controlling terminal file descriptor */
static struct termios shell_tmodes; /* saved shell terminal modes */

static void sigchld_handler(int sig) {
  int old_errno = errno;
  pid_t pid;
  int status;
  /* TODO: Change state (FINISHED, RUNNING, STOPPED) of processes and jobs.
   * Bury all children that finished saving their status in jobs. */

  bool change = true;
  bool exit_for_loop = false;
  int curr_status, new_status;
  int i, j = 0;

  while ( (pid = waitpid(-1, &status, WNOHANG | WCONTINUED | WUNTRACED)) > 0) {
    // szukam indeksu odpowiedniego job_t (i) i proc_t (j)
    exit_for_loop = false;
      for(i = 0; i< njobmax; i++){
        for (j = 0; j < jobs[i].nproc; j++){
          if(pid == jobs[i].proc[j].pid){
            exit_for_loop = true;
            break;
          }
        }
        if(exit_for_loop)
          break; 
      }
      if(WIFSTOPPED(status)){
        jobs[i].proc[j].state = STOPPED;
      } else if (WIFEXITED(status) || WIFSIGNALED(status)){
        jobs[i].proc[j].exitcode = status;
        jobs[i].proc[j].state = FINISHED;
      } else if (WIFCONTINUED(status)){
        jobs[i].proc[j].state = RUNNING;
      }
    curr_status = jobs[i].state;
    change = true;
    new_status = FINISHED;
    for (j = 0; j < jobs[i].nproc; j++){
      status = jobs[i].proc[j].state;
      if(curr_status == RUNNING){
         // zmień status tylko gdy wszystkie procesy zostały wstrzymane lub zakończone
         if(status == RUNNING){
           change = false;
         }else{
           // jeśli chociaż jeden proces jest stopped spośród innych finished, to zmień na stopped
           if(status == STOPPED)
              new_status = STOPPED;
         }
         // gdy zadanie się zakończyło (wszystkie procesy na finished) to nie ma zmiany z powrotem na stopped czy running
         // curr_status == STOPPED
      } else {
         // zmień status tylko gdy wszystkie procesy zostały wznowione lub zakończone
          if(status == STOPPED){
           change = false;
          }else{
           // jeśli chociaż jeden proces jest running spośród innych finished, to zmień na running
           if(status == RUNNING)
              new_status = RUNNING;
          }
      }   
    }
     if(change)
        jobs[i].state = new_status;  
  }

  errno = old_errno;
}

/* When pipeline is done, its exitcode is fetched from the last process. */
static int exitcode(job_t *job) {
  return job->proc[job->nproc - 1].exitcode;
}

static int allocjob(void) {
  /* Find empty slot for background job. */
  for (int j = BG; j < njobmax; j++)
    if (jobs[j].pgid == 0)
      return j;

  /* If none found, allocate new one. */
  jobs = realloc(jobs, sizeof(job_t) * (njobmax + 1));
  memset(&jobs[njobmax], 0, sizeof(job_t));
  return njobmax++;
}

static int allocproc(int j) {
  job_t *job = &jobs[j];
  job->proc = realloc(job->proc, sizeof(proc_t) * (job->nproc + 1));
  return job->nproc++;
}

int addjob(pid_t pgid, int bg) {
  int j = bg ? allocjob() : FG;
  job_t *job = &jobs[j];
  /* Initial state of a job. */
  job->pgid = pgid;
  job->state = RUNNING;
  job->command = NULL;
  job->proc = NULL;
  job->nproc = 0;
  job->tmodes = shell_tmodes;
  return j;
}

static void deljob(job_t *job) {
  assert(job->state == FINISHED);
  free(job->command);
  free(job->proc);
  job->pgid = 0;
  job->command = NULL;
  job->proc = NULL;
  job->nproc = 0;
}

static void movejob(int from, int to) {
  assert(jobs[to].pgid == 0);
  memcpy(&jobs[to], &jobs[from], sizeof(job_t));
  memset(&jobs[from], 0, sizeof(job_t));
}

static void mkcommand(char **cmdp, char **argv) {
  if (*cmdp)
    strapp(cmdp, " | ");

  for (strapp(cmdp, *argv++); *argv; argv++) {
    strapp(cmdp, " ");
    strapp(cmdp, *argv);
  }
}

void addproc(int j, pid_t pid, char **argv) {
  assert(j < njobmax);
  job_t *job = &jobs[j];

  int p = allocproc(j);
  proc_t *proc = &job->proc[p];
  /* Initial state of a process. */
  proc->pid = pid;
  proc->state = RUNNING;
  proc->exitcode = -1;
  mkcommand(&job->command, argv);
}

/* Returns job's state.
 * If it's finished, delete it and return exitcode through statusp. */
int jobstate(int j, int *statusp) {
  assert(j < njobmax);
  job_t *job = &jobs[j];
  int state = job->state;

  /* TODO: Handle case where job has finished. */

  if(state == FINISHED){
      *statusp = exitcode(job);
      deljob(job);
  }

  return state;
}

char *jobcmd(int j) {
  assert(j < njobmax);
  job_t *job = &jobs[j];
  return job->command;
}

/* Continues a job that has been stopped. If move to foreground was requested,
 * then move the job to foreground and start monitoring it. */
bool resumejob(int j, int bg, sigset_t *mask) {
  if (j < 0) {
    for (j = njobmax - 1; j > 0 && jobs[j].state == FINISHED; j--)
      continue;
  }

  if (j >= njobmax || jobs[j].state == FINISHED)
    return false;

  /* TODO: Continue stopped job. Possibly move job to foreground slot. */

   if(!bg){
    if(jobs[j].state == STOPPED)
      printf("[%d] continue '%s'\n",j,jobs[j].command);
    movejob(j, FG);
    killpg(jobs[0].pgid, SIGCONT);
    monitorjob(mask);
  }else{
    if(jobs[j].state == STOPPED)
      printf("[%d] continue '%s'\n",j,jobs[j].command);
    watchjobs(FINISHED);
    killpg(jobs[j].pgid, SIGCONT);
  }
    

  return true;
}

/* Kill the job by sending it a SIGTERM. */
bool killjob(int j) {
  if (j >= njobmax || jobs[j].state == FINISHED)
    return false;
  debug("[%d] killing '%s'\n", j, jobs[j].command);

  /* TODO: I love the smell of napalm in the morning. */

  killpg(jobs[j].pgid,SIGTERM);
  killpg(jobs[j].pgid,SIGCONT);

  return true;
}

/* Report state of requested background jobs. Clean up finished jobs. */
void watchjobs(int which) {
  char *n = malloc(sizeof(char) * 10);
  char *command = malloc(sizeof(char) * 256);
  int state, exitcode;
  for (int j = BG; j < njobmax; j++) {
    if (jobs[j].pgid == 0)
      continue;

    /* TODO: Report job number, state, command and exit code or signal. */

    exitcode = -1;
    strcpy(command, jobs[j].command);
    state = jobstate(j, &exitcode);
    if(which == ALL){ 
      if(state == RUNNING)
        strcpy(n, "running");
      else if (state == STOPPED)
        strcpy(n, "suspended");
      else{
        if (WIFSIGNALED(exitcode))
          strcpy(n, "killed");
        else
          strcpy(n, "exited");
      }
        

      if(state == RUNNING || state == STOPPED)
        printf("[%d] %s '%s'\n",j, n, command);
      else{
        if (WIFSIGNALED(exitcode))
          printf("[%d] %s '%s' by signal %d\n",j, n, command, exitcode);
        else
          printf("[%d] %s '%s', status=%d\n",j, n, command, WEXITSTATUS(exitcode));
      }
        
    }else{
      if(which == RUNNING && state != RUNNING)
        continue;
      if(which == STOPPED && state != STOPPED)
        continue;
      if(which == FINISHED && state != FINISHED)
        continue;

      if(state == RUNNING)
        strcpy(n, "running");
      else if (state == STOPPED)
        strcpy(n, "suspended");
      else{
        if (WIFSIGNALED(exitcode))
          strcpy(n, "killed");
        else
          strcpy(n, "exited");
      }

      if(state == RUNNING || state == STOPPED)
        printf("[%d] %s '%s'\n",j, n, command);
      else{
        if (WIFSIGNALED(exitcode))
          printf("[%d] %s '%s' by signal %d\n",j, n, command, exitcode);
        else
          printf("[%d] %s '%s', status=%d\n",j, n, command, WEXITSTATUS(exitcode));
      }
    }
  }

  free(n);
  free(command);
}

/* Monitor job execution. If it gets stopped move it to background.
 * When a job has finished or has been stopped move shell to foreground. */
int monitorjob(sigset_t *mask) {
  int exitcode, state;

  /* TODO: Following code requires use of Tcsetpgrp of tty_fd. */

// ustaw atrybuty terminala, jakie ustawiło dane zadanie
  Tcsetattr(tty_fd, TCSAFLUSH, &jobs[0].tmodes);
  Tcsetpgrp(tty_fd, jobs[0].pgid);

  state = jobstate(FG,&exitcode);
  // przypadek gdy proces dostaje SIGCONT, po otrzymaniu SIGTTOU lub SIGTTIN i przydzieleniu terminala
  if(state != RUNNING){
    while(state != RUNNING){
      Sigsuspend(mask);
      state = jobstate(FG,&exitcode);
    }
  }

  while(state == RUNNING){
    Sigsuspend(mask);
    state = jobstate(FG,&exitcode);
  }

// zapisz atrybuty terminala, jakie ustawiło dane zadanie
  Tcgetattr(tty_fd, &jobs[0].tmodes);
  Tcsetattr(tty_fd, TCSAFLUSH, &shell_tmodes);
  
  if(state == STOPPED){
    int j = addjob(0,true);
    movejob(FG, j);
  }

  Tcsetpgrp(tty_fd, getpgrp());

  watchjobs(FINISHED);

  return exitcode;
}

/* Called just at the beginning of shell's life. */
void initjobs(void) {
  Signal(SIGCHLD, sigchld_handler);
  jobs = calloc(sizeof(job_t), 1);

  /* Assume we're running in interactive mode, so move us to foreground.
   * Duplicate terminal fd, but do not leak it to subprocesses that execve. */
  assert(isatty(STDIN_FILENO));
  tty_fd = Dup(STDIN_FILENO);
  fcntl(tty_fd, F_SETFD, FD_CLOEXEC);

  /* Take control of the terminal. */
  Tcsetpgrp(tty_fd, getpgrp());

  /* Save default terminal attributes for the shell. */
  Tcgetattr(tty_fd, &shell_tmodes);
}

/* Called just before the shell finishes. */
void shutdownjobs(void) {
  sigset_t mask;
  Sigprocmask(SIG_BLOCK, &sigchld_mask, &mask);

  /* TODO: Kill remaining jobs and wait for them to finish. */

  for(int j = BG; j < njobmax; j++){
    if(jobs[j].state == FINISHED)
      continue;
      
    // dzięki temu shell nie zawiesi się, gdy zadanie zostanie zatrzymane przez otrzymanie SIGTTOU lub SIGTTIN
    Tcsetattr(tty_fd, TCSAFLUSH, &jobs[j].tmodes);
    Tcsetpgrp(tty_fd, jobs[j].pgid);
    killjob(j);
    while(jobs[j].state != FINISHED){
      Sigsuspend(&mask);
    }
  }

  Tcsetattr(tty_fd, TCSAFLUSH, &shell_tmodes);
  Tcsetpgrp(tty_fd, getpgrp());

  watchjobs(FINISHED);

  Sigprocmask(SIG_SETMASK, &mask, NULL);

  Close(tty_fd);
}
