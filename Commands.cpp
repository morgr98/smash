#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <algorithm>
#include <fcntl.h>

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line, bool leave_space = true) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
    if (!leave_space)
        cmd_line[str.find_last_not_of(WHITESPACE, idx + 1)] = 0;
}

bool isNumber(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// todo: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
// TODO: add your implementation
    shell_pid = getpid();
    char* temp = get_current_dir_name();
    curr_pwd = temp;
    old_pwd = "";
    free(temp);
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    // For example:

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord.compare("") == 0)
        return nullptr;
    if(cmd_s.size() != firstWord.size()) {
        string cut_cmd= cmd_s.substr(firstWord.size()+1);
        string secondWord = cut_cmd.substr(0, cut_cmd.find_first_of(" \n"));
        if (secondWord.compare(">") == 0 || secondWord.compare(">>") == 0) {
            return new RedirectionCommand(cmd_line, &this->jobsList);
        }
    }
    if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0) {
        return new ChpromptCommand(cmd_line, &this->prompt, &this->jobsList);
    }
    else if(cmd_s.find('|') != string::npos || cmd_s.find("|&") != string::npos )
        return new PipeCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0)
        return new GetCurrDirCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0)
        return new ShowPidCommand(cmd_line, &this->shell_pid, &this->jobsList);
    else if  (firstWord.compare("cd") == 0)// || firstWord.compare("cd&") == 0)
        return new ChangeDirCommand(cmd_line, &this->old_pwd, &this->curr_pwd, &this->jobsList);
    else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0)
        return new JobsCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("kill") == 0) //|| firstWord.compare("kill&") == 0)
        return new KillCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0)
        return new ForegroundCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("bg") == 0|| firstWord.compare("bg&") == 0)
        return new BackgroundCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0)
        return new QuitCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("tail") == 0)
        return new TailCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("touch") == 0)
        return new TouchCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("timeout") == 0)
        return new TouchCommand(cmd_line, &this->jobsList);

/*
  else if ...
  .....
  */
    else {
        return new ExternalCommand(cmd_line, &this->jobsList);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    //for example:
    Command* cmd = CreateCommand(cmd_line);
    if (cmd == nullptr)
        return;
    cmd->pjobsList->removeFinishedJobs();
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char *cmd_line, JobsList* pjobsList)  {
    //this->command_args = (char**)malloc(sizeof(char*)*COMMAND_MAX_ARGS);
    this->command_args = new char*[COMMAND_MAX_ARGS];
    for (int i=0;i<COMMAND_MAX_ARGS;i++)
        command_args[i] = nullptr;
    this->num_args = _parseCommandLine(cmd_line, command_args);
    this->cmd_line = (char*)malloc(sizeof(char)*(strlen(cmd_line) +1));
    strcpy(this->cmd_line, cmd_line);
    this->pjobsList = pjobsList;
}

Command::~Command() noexcept {
    for (int i=0;i<COMMAND_MAX_ARGS;i++)
    {
        if (this->command_args[i]!= nullptr)
            free(command_args[i]);
        else
            break;
    }
    delete(this->command_args);
    free(cmd_line);
}

BuiltInCommand::BuiltInCommand(const char* cmd_line, JobsList* shellsjobList):Command(cmd_line, shellsjobList){
    if (_isBackgroundComamnd(cmd_line))
    {
        std::string last_arg = this->command_args[this->num_args-1];
        if (last_arg.compare("&")==0)
            this->num_args--;
        else
        {
            _removeBackgroundSign(this->command_args[this->num_args-1], false);
        }

    }
}

ExternalCommand::ExternalCommand(const char *cmd_line, JobsList* pjobslist) : Command(cmd_line, pjobslist){
    if(_isBackgroundComamnd(cmd_line))
    {
        this->is_background= true;
    }
    else
        this->is_background=false;
    this->cmd_ex=string(this->cmd_line);
    _removeBackgroundSign((char*)this->cmd_ex.c_str());

}

ExternalCommand::~ExternalCommand() {

}

void ExternalCommand::execute() {
    this->pid_ex= fork();
    if(this->pid_ex==-1)
    {
        perror("smash error: fork failed");
        return;
    }
    if(this->pid_ex ==0)//child process
    {
        setpgrp();
        char * args_commend []={(char *)"/bin/bash",(char *)"-c",(char *)this->cmd_ex.c_str(),(char *)"/0"};
        execv(args_commend[0],args_commend);
        //for morg - i think you should do it here that if command not found you perror(smash error: ........) and than return;
    }
    else// father process
    {
        if (this->is_background) {
            this->pjobsList->addJob(this, Background);
        } else {
            this->pjobsList->pi_fg = pid_ex;
            this->pjobsList->cmd_line_fg = this->command_args[0];
            this->pjobsList->cmd_fg = this;
            //this->pjobsList->job_fg= new JobsList::JobEntry(this,Foreground);
            //this->pjobsList->job_fg->command_type=this->command_args[1];
            waitpid(this->pid_ex, NULL, WUNTRACED);
        }
    }
}

void ChpromptCommand::execute() {
    if (this->num_args > 1)
        *(this->prompt) = std::string(this->command_args[1]) + "> ";
    else
        *(this->prompt) = "smash> ";
}

void ShowPidCommand::execute() {
    //can pid change while shell is working?
    cout << "smash pid is " << *(this->shell_pid) << endl;
}

void GetCurrDirCommand::execute() {
    char* curr_dir = get_current_dir_name();
    if (curr_dir == nullptr)
    {
        perror("smash error: get_current_dir_name failed");
        return;
    }
    cout << curr_dir << endl;
    free(curr_dir);
}

void ChangeDirCommand::execute() {
    if (this->num_args > 2)
    {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    else if (this->num_args == 1)
    {
        cerr << "smash error: cd: not enough arguments" << endl;
        return;
    }
    std::string arg = this->command_args[1];
    int idx = arg.find_last_not_of(WHITESPACE);
    if (arg[idx]=='-')
    {
        if (this->plastPwd->compare("") == 0)
        {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        else if (chdir((this->plastPwd)->c_str()) == -1)
        {
            perror("smash error: chdir failed");
            return;
        }
        std::swap(*(this->pcurrPwd),*(this->plastPwd));
    }
    else
    {
        if (chdir(this->command_args[1]) == -1)
        {
            perror("smash error: chdir failed");
            return;
        }
        *plastPwd = *pcurrPwd;
        *(this->pcurrPwd)=std::string(this->command_args[1]);
    }
}

void KillCommand::execute() {
    if (this->num_args > 3 || this->num_args < 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    std::string sig_num_str = this->command_args[1];
    std::string jobid = this->command_args[2];
    if (sig_num_str.at(0) != ('-') || !isNumber(sig_num_str.substr(1)) || !isNumber(jobid)) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig_num = stoi(sig_num_str.substr(1));
    if (sig_num < 1 || sig_num > 31)
    {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    JobsList::JobEntry* job = this->pjobsList->getJobById(stoi(jobid));
    if (job== nullptr)
    {
        cerr << "smash error: kill: job-id " << jobid << " does not exist" << endl;
        return;
    }
    pid_t job_pid = job->cmd->pid_ex;
    if (kill(job_pid, sig_num)!=0)
    {
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << sig_num << " was sent to pid " << job_pid << endl;
    this->pjobsList->removeFinishedJobs();
}

void JobsCommand::execute() {
    this->pjobsList->printJobsList();
}

void ForegroundCommand::execute() {
    if (this->num_args > 2)
    {
        cerr <<"smash error: fg: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* job_to_fg;
    if (this->num_args == 2)
    {
        std::string jobid_string = std::string(this->command_args[1]);
        if (!(isNumber(jobid_string)))
        {
            cerr <<"smash error: fg: invalid arguments" << endl;
            return;
        }
        jobid jid = stoi(jobid_string);
        job_to_fg = this->pjobsList->getJobById(jid);
        if (job_to_fg == nullptr)
        {
            cerr << "smash error: fg: job-id "<< jid << " does not exist" << endl;
            return;
        }
    }
    else
    {
        job_to_fg = this->pjobsList->findMaxId(nullptr);
        if (job_to_fg == nullptr)
        {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    Command* cmd = job_to_fg->cmd;
    pid_t pid = cmd->pid_ex;
    cout << cmd->cmd_line << " : " << cmd->pid_ex << endl;
    if(kill(pid, SIGCONT)!=0)
    {
        perror("smash error: kill failed");
        return;
    }
    this->pjobsList->removeJobById(job_to_fg->id);
    this->pjobsList->pi_fg = pid;
    this->pjobsList->cmd_line_fg = cmd->cmd_line;
    this->pjobsList->cmd_fg = cmd;
    this->pjobsList->jid_fg = job_to_fg->id;
    waitpid(this->pid_ex, NULL, WUNTRACED);
}

void BackgroundCommand::execute() {
    if (this->num_args > 2)
    {
        cerr <<"smash error: bg: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* job_to_fg;
    if (this->num_args == 2)
    {
        std::string jobid_string = std::string(this->command_args[1]);
        if (!(isNumber(jobid_string)))
        {
            cerr <<"smash error: bg: invalid arguments" << endl;
            return;
        }
        jobid jid = stoi(jobid_string);
        job_to_fg = this->pjobsList->getJobById(jid);
        if (job_to_fg == nullptr)
        {
            cerr << "smash error: bg: job-id "<< jid << " does not exist" << endl;
            return;
        }
        if (job_to_fg->status!=Stopped)
        {
            cerr << "smash error: bg: job-id " << jid << "is already running in the background" << endl;
            return;
        }
    }
    else
    {
        job_to_fg = this->pjobsList->getLastStoppedJob(nullptr);
        if (job_to_fg == nullptr)
        {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
    job_to_fg->status = Background;
    Command* cmd = job_to_fg->cmd;
    cout << cmd->cmd_line << " : " << cmd->pid_ex << endl;
    if (kill(pid_ex, SIGCONT) != 0)
    {
        job_to_fg->status = Stopped;
        perror("smash error: kill failed");
        return;
    }
}

void QuitCommand::execute() {
    if (num_args >1)
    {
        std::string first_arg = this->command_args[1];
        if(first_arg.compare("kill") == 0 || first_arg.compare("kill&") == 0) //kill will surely be the first?
        {
            cout << "smash sending SIGKILL signal to " << this->pjobsList->jobs.size() - 1 << " jobs:" << endl;
            std::string message;
            this->pjobsList->killAllJobs(nullptr, &message);
            cout << message;
        }
    }
    exit(0);
}

void RedirectionCommand::execute() {
    if(this->num_args != 3)
    {
        return;
    }
    Command* cmd= SmallShell::getInstance().CreateCommand(this->command_args[0]);
    int fd;
    int new_fd_montior= dup(1);
    close(1);
    if(string(this->command_args[1]).compare(">") == 0) {
        fd = open(this->command_args[2], O_TRUNC | O_WRONLY);
        if (fd == -1) {
            perror("smash error: open failed");
            dup2(new_fd_montior,1);
            close(new_fd_montior);
            return;
        }
    }
    else {
        fd = open(this->command_args[2], O_APPEND | O_WRONLY);
        if (fd == -1) {
            perror("smash error: open failed");
            dup2(new_fd_montior,1);
            close(new_fd_montior);
            return;
        }
    }
    cmd->execute();
    close(fd);
    dup2(new_fd_montior,1);
    close(new_fd_montior);
}

void PipeCommand::execute() {
    //cout<<this->cmd_line<<endl;
    std::string cmd_trim = _trim(string(this->cmd_line));
   // cout<<cmd_trim<<endl;
    std::size_t pos = cmd_trim.find("|");
    std::string cmd_line_1 = cmd_trim.substr(0, pos);
    std::string cmd_line_2;
    int write_change;
   if(cmd_trim.find("|&") == string::npos) {
       cmd_line_2 = cmd_trim.substr(pos+1);
       write_change=1;
   }
   else
   {
       cmd_line_2 = cmd_trim.substr(pos+2);
       write_change=2;
   }
    cmd_line_2=_trim(cmd_line_2);
    Command* cmd1= SmallShell::getInstance().CreateCommand(cmd_line_1.c_str());
    Command* cmd2= SmallShell::getInstance().CreateCommand(cmd_line_2.c_str());
    //cout<<cmd_line_2<<endl;
    //cout<<cmd_line_1<<endl;
    int my_pipe[2];
    pipe(my_pipe);
    pid_t pid_1, pid_2;
    pid_1= fork();
    if(pid_1 == -1)
    {
        perror("smash error: fork failed");
        close(my_pipe[0]);
        close(my_pipe[1]);
        return;
    }
    if(pid_1 == 0)// first son process
    {
        setpgrp();
        close(write_change);
        dup2(my_pipe[1],write_change);
        close(my_pipe[0]);
        close(my_pipe[1]);
        cmd1->execute();
        exit(0);

    }
    else// father process
    {
        pid_2=fork();
        if(pid_2 == -1)
        {
            kill(pid_1,9);
            close(my_pipe[0]);
            close(my_pipe[1]);
            perror("smash error: fork failed");
            return;
        }
        if(pid_2 == 0) // second son process
        {
            setpgrp();
            close(0);
            dup2(my_pipe[0], 0);
            close(my_pipe[0]);
            close(my_pipe[1]);
            cmd2->execute();
            exit(0);
        }
    }
    close(my_pipe[0]);
    close(my_pipe[1]);
    waitpid(pid_1,NULL, WUNTRACED);
    waitpid(pid_2,NULL, WUNTRACED);
}

void TailCommand::execute() {
    if (this->num_args > 3 || this->num_args <= 1) {
        cerr << "smash error: tail: invalid arguments" << endl;
        return;
    }
    int fd;
    int N = 10;
    std::string arg_2 = this->command_args[1];
    if (isNumber(arg_2)) {
        N = stoi(arg_2);
        fd = open(this->command_args[2], O_RDONLY);
        if (fd == -1) {
            perror("smash error: open failed");
            return;
        }
    } else {
        fd = open(this->command_args[1], O_RDONLY);
        if (fd == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    off64_t end =  lseek(fd, 0, SEEK_END);
    if (end==-1)
    {
        perror("smash error: lseek failed");
        return;
    }
    char buffer;
    int count = 0;
    off64_t i = 0;
    while (i < end - 1)
    {
        if (read(fd, &buffer, 1) == -1)
        {
            perror("smash error: read failed");
            return;
        }
        if (buffer == '\n')
        {
            count++;
        }
        if (count == N)
        {
            i+=2;
            break;
        }
        i++;
        if(lseek(fd, -2, SEEK_CUR)==-1)
        {
            perror("smash error: lseek failed");
            return;
        }
    }
    if (count < N)
        i+=1;
    off64_t bytes_to_read = i;
    char* to_print = (char*)malloc(sizeof(char)*(bytes_to_read + 1));
    if (read(fd, to_print, bytes_to_read) == -1) {
        perror("smash error: read failed");
        return;
    }
    cout << to_print;
    free(to_print);
}

void TouchCommand::execute() {
    //cout<<this->num_args<< endl;
    if(this->num_args != 3)
    {
        cerr << "smash error: touch: invalid arguments" << endl;
        return;
    }
    char* file_name= this->command_args[1];
    string timestamp= string(this->command_args[2]);
    timestamp.erase(std::remove(timestamp.begin(), timestamp.end(), ':'), timestamp.end());
    tm time_info={0};

    if(strptime(timestamp.c_str(),"%S%M%H%d%m%Y", &time_info) == nullptr)
    {
        return;
    }
    /*
    time_info.tm_sec = stoi(timestamp.substr(0,2));
    cout<<time_info.tm_sec<< endl;
    time_info.tm_min = stoi(timestamp.substr(3,5));
    cout<<time_info.tm_min<< endl;
    time_info.tm_hour = stoi(timestamp.substr(6,8));
    cout<<time_info.tm_hour<< endl;
    time_info.tm_mday = stoi(timestamp.substr(9,11));
    cout<<time_info.tm_mday<< endl;
    time_info.tm_mon = stoi(timestamp.substr(12,14));
    cout<<time_info.tm_mon<< endl;
    time_info.tm_year = stoi(timestamp.substr(15,19));
    cout<<time_info.tm_year<< endl;
*/
    time_t update_time =mktime(&time_info);
    utimbuf new_time;
    new_time.actime= update_time;
    new_time.modtime= update_time;
    utime(file_name,&new_time);
}

void TimeoutCommand::execute() {/*
    string duration1= string(this->command_args[1]);
    int pos= string(this->cmd_line).find(duration1);
    string cmd_l = string(this->cmd_line).substr(pos);
    cmd_l= _trim(cmd_l);
    int duration = stoi(duration1);
    alarm(duration);
    Command* cmd= SmallShell::getInstance().CreateCommand(cmd_l.c_str());
    struct sigaction siga;
    siga.sa_flags= SA_RESTART;
    //siga.sa_handler = cmd->execute;
    */

}

JobsList::JobEntry::JobEntry(Command *cmd, JobStatus status) {
    this->cmd = cmd;
    this->status = status;

}
void JobsList::addJob(Command *cmd, JobStatus status) {
    this->removeFinishedJobs();
    findMaxId(&max_id);
    JobEntry* job = new JobEntry(cmd, status);
    job->id = this->max_id+1;
    job->command_type = cmd->command_args[0];
    job->time_inserted = time(nullptr);
    this->jobs.insert(std::pair<jobid, JobEntry*>(job->id, job));
    this->max_id=job->id;
}

void JobsList::removeFinishedJobs() {
    for (auto riter = this->jobs.rbegin(); riter!=jobs.rend();)
    {
        if (riter->second!= nullptr)
        {
            pid_t pid = riter->second->cmd->pid_ex;
            if(waitpid(pid,NULL, WNOHANG) > 0)
            {
                if (this->max_id == riter->second->id)
                {
                    findMaxId(&max_id);
                }

                delete riter->second;
                riter = decltype(riter){ this->jobs.erase(std::next(riter).base()) };
                continue;
            }
        }
        riter++;
    }
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    for (auto iter = this->jobs.begin(); iter!=this->jobs.end(); iter++)
    {
        if(iter->second== nullptr)
            continue;
        double time_elapsed = difftime(time(nullptr), iter->second->time_inserted);
        cout << "[" << iter->second->id << "] " << iter->second->cmd->cmd_line << " : " << iter->second->cmd->pid_ex << " " << time_elapsed << " ";
        if (iter->second->status == Stopped)
            cout << "(stopped)";
        cout << endl;
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    auto it = this->jobs.find(jobId);
    if (it == this->jobs.end())
        return nullptr;
    return it->second;
}
JobsList::JobEntry* JobsList::findMaxId(jobid* jobId) {
    for (auto riter = this->jobs.rbegin(); riter!=this->jobs.rend(); riter++)
    {
        if (riter->second!=nullptr)
        {
            if (jobId!=nullptr)
                *jobId = riter->second->id;
            this->max_id = riter->second->id;
            return riter->second;
        }
    }
    if (jobId!=nullptr)
        *jobId = 0;
    this->max_id = 0;
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
    for (auto riter = this->jobs.rbegin(); riter!=this->jobs.rend(); riter++)
    {
        if (riter->second!=nullptr)
        {
            if (riter->second->status==Stopped)
            {
                if (jobId!=nullptr)
                    *jobId = riter->second->id;
                return riter->second;
            }
        }
    }
    if (jobId!=nullptr)
        *jobId = 0;
    return nullptr;
}
void JobsList::removeJobById(int jobId) {
    this->jobs.erase(jobId);
}

void JobsList::killAllJobs(int* num_killed, std::string* message) {
    for (auto it = this->jobs.begin(); it!=this->jobs.end(); it++)
    {
        if (it->second!=nullptr)
        {
            pid_t pid = it->second->cmd->pid_ex;
            if (kill(pid, SIGKILL) != 0)
            {
                perror("smash error: kill failed"); //what do we do then?
            }
            if (num_killed!=nullptr)
                (*num_killed)++;
            if (message!=nullptr)
                *message += (std::to_string(pid) + ": " + it->second->cmd->cmd_line + "\n");
        }
    }
}