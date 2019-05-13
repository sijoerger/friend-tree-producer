# friend-tree-producer
Software setup to produce additional quantities on top of ntuples in sync format with batch-system support

## Setting up the software
To setup the environment, please download the checkout and execute the checkout script as follows:

```bash
wget https://raw.githubusercontent.com/KIT-CMS/friend-tree-producer/master/scripts/checkout.sh
bash checkout.sh
```
## General setup of friend tree executables
Several external software relies on `C++` or `python` executables to create friend trees.
The `C++` executables should be placed into the [bin](https://github.com/KIT-CMS/friend-tree-producer/tree/master/bin)
folder with corresponding entry in the [BuildFile.xml](https://github.com/KIT-CMS/friend-tree-producer/tree/master/bin/BuildFile.xml),
whereas the `python` executables should be placed into the [scripts](https://github.com/KIT-CMS/friend-tree-producer/tree/master/scripts) folder.
After compilation within CMSSW, these executables should be accessible within the command line.

A common set of command line options should be used for these executables in order to process them equally through the batch system job management.
This set of command line options should be:

 * `--input`: path to the input ntuple to be used
 * `--folder`: systematics folder to be accessed within the file defined with the option above
 * `--tree`: tree name to be accessed within the folder defined with the option above
 * `--first_entry`: first entry to be processed within the tree
 * `--last_entry`: last entry to be processed within the tree

Asuming the absolute path to the input file is `/path/to/the/<input>.root`, the path to the output file starting from the current directory should read:
`<input>/<input>_<folder>_<first_entry>_<last_entry>.root`.

## Job management for condor batch systems
The main script to submit jobs is [job_management.py](https://github.com/KIT-CMS/friend-tree-producer/blob/master/scripts/job_management.py). Following options are available:

 * `--executable`: Executable to be used for friend tree creation ob the batch system. Currently, only the choice for `SVFit` and `MELA` available.
 * `--batch_cluster`: Batch system cluster to be used. Currently available choices: `naf` and `etp`. The templates for the `.jdl` files can be found in the [data](https://github.com/KIT-CMS/friend-tree-producer/tree/master/data) folder.
 * `--command`: Command to be done by the job manager. The `submit` command preprares a submission `.jdl` file for the desired condor batch system. The `collect` command merges the produced outputs to a single output file.
 * `--input_ntuples_directory`: Directory where the input files can be found. The file structure in the directory should match `*/*.root` wildcard.
 * `--events_per_job`: Event to be processed by each job.
 * `--walltime`: This option should be only set, if it is required by the batch cluster you are using. Currently, for the `etp` cluster.
 * `--cores`: Number of cores to be used for the collect command.

Please use also the `--help` command for this script to see the details of its execution.
