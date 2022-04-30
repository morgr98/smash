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

void _removeBackgroundSign(char* cmd_line) {
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
    if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0)
        return new ChpromptCommand(cmd_line, &this->prompt, &this->jobsList);
    else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0)
        return new GetCurrDirCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0)
        return new ShowPidCommand(cmd_line, &this->shell_pid, &this->jobsList);
    else if  (firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0)
        return new ChangeDirCommand(cmd_line, &this->old_pwd, &this->curr_pwd, &this->jobsList);
    else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0)
        return new JobsCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("kill") == 0 || firstWord.compare("kill&") == 0)
        return new KillCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0)
        return new ForegroundCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("bg") == 0|| firstWord.compare("bg&") == 0)
        return new BackgroundCommand(cmd_line, &this->jobsList);
    else if(firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0)
        return new QuitCommand(cmd_line, &this->jobsList);

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
    _removeBackgroundSign(this->cmd_line);
    if (this->num_args>0)
        _removeBackgroundSign(this->command_args[0]);
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
        //if command not found?
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
        perror("smash error: get_current_dir_name failed");
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
    if (arg.compare("-") == 0)
    {
        if (chdir((this->plastPwd)->c_str()) == -1)
            perror("smash error: chdir failed");
        std::swap(*(this->pcurrPwd),*(this->plastPwd));
    }
    else
    {
        if (chdir(this->command_args[1]) == -1)
            perror("smash error: chdir failed");
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
            cout << "sending SIGKILL signal to " << this->pjobsList->jobs.size() - 1 << " jobs:" << endl;
            std::string message;
            this->pjobsList->killAllJobs(nullptr, &message);
            cout << message;
        }
    }
    exit(0);
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