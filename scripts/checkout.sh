#!/bin/bash


### Setup of CMSSW release
CMSSW=CMSSW_10_2_14

if [ "$1" == "" ]; then
  echo "$0: Explicit CMSSW version is not provided. Checking out as default $CMSSW"
fi

if [ ! "$1" == "" ]; then
  CMSSW=$1
  echo "$0: Checking out $CMSSW"
fi


export SCRAM_ARCH=slc6_amd64_gcc700
export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch
source $VO_CMS_SW_DIR/cmsset_default.sh

scramv1 project $CMSSW; pushd $CMSSW/src
eval `scramv1 runtime -sh`

### Checkout of external software

# SVfit and fastMTT
git clone https://github.com/SVfit/ClassicSVfit TauAnalysis/ClassicSVfit -b fastMTT_19_02_2019
git clone https://github.com/SVfit/SVfitTF TauAnalysis/SVfitTF

# MELA
git clone https://github.com/cms-analysis/HiggsAnalysis-ZZMatrixElement HiggsAnalysis/ZZMatrixElement -b v2.2.0

# TODO NN mass

# TODO FF weights

# TODO NN MET

# TODO NN max score 

# TODO single-tau HLT

### Checkout of friend tree producer setup
git clone https://github.com/KIT-CMS/friend-tree-producer.git HiggsAnalysis/friend-tree-producer

### Compiling under CMSSW
CORES=10
scram b -j $CORES
popd
