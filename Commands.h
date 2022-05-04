#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <map>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class JobsList;
typedef int jobid;
enum JobStatus {Stopped, Background, Foreground};

class Command {
// TODO: Add your data members
public:
    int num_args;
    char** command_args;
    char* cmd_line;
    pid_t pid_ex=-1;
    JobsList* pjobsList;
    Command(const char* cmd_line, JobsList* pjobslist);
    // create constructor
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, JobsList* shellsjobList);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    bool is_background;
    std::string cmd_ex;
    ExternalCommand(const char* cmd_line, JobsList* pjobsList);
    virtual ~ExternalCommand() ;
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    explicit PipeCommand(const char* cmd_line, JobsList* pjobslist) : Command(cmd_line, pjobslist) {};
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members

public:
    explicit RedirectionCommand(const char* cmd_line, JobsList* pjobslist) : Command(cmd_line, pjobslist) {};
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    std::string *plastPwd;
    std::string *pcurrPwd;
    ChangeDirCommand(const char* cmd_line, std::string *old_pwd, std::string *curr_pwd, JobsList* pjobslist): BuiltInCommand(cmd_line, pjobslist), plastPwd(old_pwd), pcurrPwd(curr_pwd){};
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, JobsList* pjobslist): BuiltInCommand(cmd_line, pjobslist){};
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
public:
    std::string *prompt;
    ChpromptCommand(const char* cmd_line, std::string *prompt, JobsList* pjobslist) : BuiltInCommand(cmd_line, pjobslist), prompt(prompt){};
    virtual ~ChpromptCommand() {}
    void execute() override;

};

class ShowPidCommand : public BuiltInCommand {
public:
    pid_t* shell_pid;
    ShowPidCommand(const char* cmd_line, pid_t* shell_pid, JobsList* pjobslist): BuiltInCommand(cmd_line, pjobslist), shell_pid(shell_pid){};
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line, jobs){};
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
    public:
        jobid id;
        std::string command_type;
        Command* cmd;
        time_t time_inserted;
        JobStatus status;

        JobEntry(Command* cmd, JobStatus status);
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    std::map<jobid, JobEntry*> jobs;
    jobid max_id;
    JobEntry* job_fg;
   char* cmd_line_fg;
   Command* cmd_fg;
    pid_t pi_fg=-1;
    jobid jid_fg=-1;
    JobsList(): max_id(0) , job_fg(nullptr), cmd_fg(nullptr){
        jobs.insert(std::pair<jobid, JobEntry*>(0,nullptr));
    };
    ~JobsList(){
        for (auto it = jobs.begin(); it!=jobs.end();it++)
        {
            if (it->second!=nullptr)
            {
                delete it->second;
            }
        }
    };
    void addJob(Command* cmd, JobStatus status = Background);
    void printJobsList();
    void killAllJobs(int* num_killed, std::string* message);
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry *getLastStoppedJob(int *jobId);
    JobEntry *findMaxId(jobid *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char* cmd_line, JobsList* shellsjobslist): BuiltInCommand(cmd_line, shellsjobslist){};
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line, jobs){};
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line, jobs){};
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line, jobs){};
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line, jobs){};
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char* cmd_line);
    virtual ~TouchCommand() {}
    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
public:
    std::string prompt = "smash> ";
    pid_t shell_pid;
    std::string old_pwd;
    std::string curr_pwd;
    JobsList jobsList;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
