#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.jobsList.pi_fg != -1) {
        cout << "smash: got ctrl-Z" << endl;
        kill((smash.jobsList.pi_fg),7);
        cout << "smash: process " << smash.jobsList.pi_fg << " was stopped" << endl;
       // JobsList::JobEntry job_stop= smash.jobsList.jobs.in
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.jobsList.pi_fg != -1) {
        cout << "smash: got ctrl-C" << endl;
        kill(smash.jobsList.pi_fg, 2);
        cout << "smash: process " << smash.jobsList.pi_fg << " was killed" << endl;
        smash.jobsList.pi_fg = -1;
    }
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

