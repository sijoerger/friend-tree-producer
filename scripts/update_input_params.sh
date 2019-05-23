cd $CMSSW_BASE/src/HiggsAnalysis/friend-tree-producer/data/input_params
read -p "lxplus-username: " USERNMLXP
scp ${USERNMLXP}@lxplus.cern.ch:/eos/home-s/swozniew/friend-tree-producer-input-params/* $CMSSW_BASE/src/HiggsAnalysis/friend-tree-producer/data/input_params/
rm datasets.json
wget https://raw.githubusercontent.com/KIT-CMS/datasets/master/datasets.json
