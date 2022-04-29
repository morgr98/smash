#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.jobsList.pi_fg != -1) {
        cout << "smash: got ctrl-Z" << endl;
        kill((smash.jobsList.pi_fg),SIGSTOP);
        cout << "smash: process " << smash.jobsList.pi_fg << " was stopped" << endl;
        /*
       smash.jobsList.job_fg->status= Stopped;
       smash.jobsList.job_fg->id=smash.jobsList.id_to_insert;
       smash.jobsList.job_fg->time_inserted= time(nullptr);
         */
        smash.jobsList.removeFinishedJobs(); //added because we didnt use addJob here
        Command* cmd_to_stop = smash.jobsList.cmd_fg;
       JobsList::JobEntry* job_stop= new JobsList::JobEntry(cmd_to_stop,JobStatus::Stopped);
       job_stop->time_inserted=time(nullptr);
       job_stop->command_type= smash.jobsList.cmd_line_fg;
       if (smash.jobsList.jid_fg!=-1)
       {
           job_stop->id = smash.jobsList.jid_fg;
           smash.jobsList.jobs.insert(std::pair<jobid, JobsList::JobEntry*>(smash.jobsList.jid_fg, job_stop));
           smash.jobsList.pi_fg=-1;
           smash.jobsList.jid_fg=-1;
       }

       else
       {
           job_stop->id=smash.jobsList.max_id +1;
           smash.jobsList.max_id++; //i think all of the above (from job_stop) are not necessary - happen in addJob anyway
           smash.jobsList.jobs.insert(std::pair<jobid, JobsList::JobEntry*>(job_stop->id, job_stop));
           smash.jobsList.pi_fg=-1;
       }
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.jobsList.pi_fg != -1) {
        cout << "smash: got ctrl-C" << endl;
        kill(smash.jobsList.pi_fg, SIGKILL);
        cout << "smash: process " << smash.jobsList.pi_fg << " was killed" << endl;
        smash.jobsList.pi_fg = -1;
        smash.jobsList.jid_fg = -1;
        //smash.jobsList.cmd_line_fg="";
       // delete smash.jobsList.job_fg;
    }
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
}

