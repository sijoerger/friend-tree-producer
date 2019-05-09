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
