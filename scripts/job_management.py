#!/usr/bin/env python

import ROOT as r
import glob
import argparse
import json
import os
import numpy as np
import stat

shellscript_template = '''#!/bin/sh
ulimit -s unlimited
set -e

cd {TASKDIR}

{COMMANDS}

'''

command_template = '''
if [ $1 -eq {JOBNUMBER} ]; then
    {COMMAND}
fi
'''

def prepare_jobs(input_ntuples_list, events_per_job, batch_cluster, executable):
    ntuple_database = {}
    for f in input_ntuples_list:
        nick = os.path.basename(f).strip(".root")
        ntuple_database[nick] = {}
        ntuple_database[nick]["path"] = f
        F = r.TFile.Open(f,"read")
        pipelines = [k.GetName() for k in F.GetListOfKeys()]
        ntuple_database[nick]["pipelines"] = {}
        for p in pipelines:
            ntuple_database[nick]["pipelines"][p] = F.Get(p).Get("ntuple").GetEntries()
    job_database = {}
    job_number = 0
    for nick in ntuple_database:
        for p in ntuple_database[nick]["pipelines"]:
            n_entries = ntuple_database[nick]["pipelines"][p]
            entry_list = np.append(np.arange(0,n_entries,events_per_job),[n_entries -1])
            first_entries = entry_list[:-1]
            last_entries = entry_list[1:] -1
            last_entries[-1] += 1
            for first,last in zip(first_entries, last_entries):
                job_database[job_number] = {}
                job_database[job_number]["input"] = ntuple_database[nick]["path"]
                job_database[job_number]["folder"] = p
                job_database[job_number]["tree"] = "ntuple"
                job_database[job_number]["first_entry"] = first
                job_database[job_number]["last_entry"] = last
                job_number +=1
    workdir_path = os.path.join(os.environ["CMSSW_BASE"],"src",executable+"_workdir")
    if not os.path.exists(workdir_path):
        os.mkdir(workdir_path)
    if not os.path.exists(os.path.join(workdir_path,"logging")):
        os.mkdir(os.path.join(workdir_path,"logging"))
    commandlist = []
    for jobnumber in job_database:
        options = " ".join(["--"+k+" "+str(v) for (k,v) in job_database[jobnumber].items()])
        commandline = "{EXEC} {OPTIONS}".format(EXEC=executable, OPTIONS=options)
        command = command_template.format(JOBNUMBER=str(jobnumber), COMMAND=commandline)
        commandlist.append(command)
    commands = "\n".join(commandlist)
    shellscript_content = shellscript_template.format(COMMANDS=commands,TASKDIR=workdir_path)
    executable_path = os.path.join(workdir_path,"condor_"+executable+".sh")
    condorjdl_path = os.path.join(workdir_path,"condor_"+executable+".jdl")
    jobdb_path = os.path.join(workdir_path,"condor_"+executable+".json")
    with open(executable_path,"w") as shellscript:
        shellscript.write(shellscript_content)
        os.chmod(executable_path, os.stat(executable_path).st_mode | stat.S_IEXEC)
        shellscript.close()
    condorjdl_template_path = os.path.join(os.environ["CMSSW_BASE"],"src/HiggsAnalysis/friend-tree-producer/data/submit_condor_%s.jdl"%batch_cluster)
    condorjdl_template_file = open(condorjdl_template_path,"r")
    condorjdl_template = condorjdl_template_file.read()
    njobs = str(job_number)
    condorjdl_content = condorjdl_template.format(TASKDIR=workdir_path,EXECUTABLE=executable_path,NJOBS=njobs)
    with open(condorjdl_path,"w") as condorjdl:
        condorjdl.write(condorjdl_content)
        condorjdl.close()
    print "To run the condor submission, execute the following:"
    print
    print "cd {TASKDIR}; condor_submit {CONDORJDL}".format(TASKDIR=workdir_path, CONDORJDL=condorjdl_path)
    with open(jobdb_path,"w") as db:
        db.write(json.dumps(job_database, sort_keys=True, indent=2))
        db.close()
    

def collect_outputs():
    pass

def main():
    parser = argparse.ArgumentParser(description='Script to manage condor batch system jobs for the executables and their outputs.')
    parser.add_argument('--executable',required=True, help='Executable to be used for friend tree creation ob the batch system.')
    parser.add_argument('--batch_cluster',required=True, choices=['naf'], help='Batch system cluster to be used.')
    parser.add_argument('--command',required=True, choices=['submit','collect'], help='Command to be done by the job manager.')
    parser.add_argument('--input_ntuples_directory',required=True, help='Batch system cluster to be used.')
    parser.add_argument('--events_per_job',required=True, type=int, help='Event to be processed by each job')
    args = parser.parse_args()

    input_ntuples_list = glob.glob(os.path.join(args.input_ntuples_directory,"*","*.root"))
    if args.command == "submit":
        prepare_jobs(input_ntuples_list, args.events_per_job, args.batch_cluster, args.executable)
    elif args.command == "collect":
        collect_outputs(executable)

if __name__ == "__main__":
    main()
