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
    if (firstWord.compare("chprompt") == 0)
        return new ChpromptCommand(cmd_line, &this->prompt, &this->jobsList);
    else if (firstWord.compare("pwd") == 0)
        return new GetCurrDirCommand(cmd_line, &this->jobsList);
    else if (firstWord.compare("showpid") == 0)
        return new ShowPidCommand(cmd_line, &this->shell_pid, &this->jobsList);
    else if  (firstWord.compare("cd") == 0)
        return new ChangeDirCommand(cmd_line, &this->old_pwd, &this->curr_pwd, &this->jobsList);
    else if (firstWord.compare("jobs") == 0)
        return new JobsCommand(cmd_line, &this->jobsList);

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
    }
    else// father process
    {
        if(this->is_background)
        {
            this->pjobsList->addJob(this, Background);
        }
        else
        {
            this->pjobsList->pi_fg=pid_ex;
            this->pjobsList->job_fg= new JobsList::JobEntry(this,Foreground);
            waitpid(this->pid_ex,NULL, WUNTRACED);
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

void JobsCommand::execute() {
    this->pjobsList->printJobsList();
}

JobsList::JobEntry::JobEntry(Command *cmd, JobStatus status) {
    this->cmd = cmd;
    this->status = status;

}
void JobsList::addJob(Command *cmd, JobStatus status) {
    this->removeFinishedJobs();
    JobEntry* job = new JobEntry(cmd, status);
    job->id = this->id_to_insert;
    job->command_type = cmd->command_args[0];
    job->time_inserted = time(nullptr);
    this->jobs.insert(std::pair<jobid, JobEntry*>(job->id, job));
    this->id_to_insert++;
}

void JobsList::removeFinishedJobs() {
    for (auto riter = this->jobs.rbegin(); riter!=jobs.rend();)
    {
        if (riter->second!= nullptr)
        {
            pid_t pid = riter->second->cmd->pid_ex;
            if(waitpid(pid,NULL, WNOHANG) > 0)
            {
                if (id_to_insert == riter->second->id + 1)
                    id_to_insert--;
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