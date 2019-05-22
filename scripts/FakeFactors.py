#!/usr/bin/env python
# -*- coding: utf-8 -*-

pkg = __import__("HiggsAnalysis.fake-factor-application.calculate_fake_factors")
ff_tools=getattr(pkg, "fake-factor-application").calculate_fake_factors

import os
import json

import argparse
import logging
logger = logging.getLogger()


def setup_logging(output_file, level=logging.DEBUG):
    logger.setLevel(level)
    formatter = logging.Formatter("%(name)s - %(levelname)s - %(message)s")

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    file_handler = logging.FileHandler(output_file, "w")
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Calculate fake factors and create friend trees.")
    parser.add_argument(
        "--input",
        required=True,
        type=str,
        help="Input file.")
    parser.add_argument(
        "--input-friends",
        nargs='+',
        type=str,
        default=[],
        help="List of files with friend trees.")
    parser.add_argument(
        "--folder",
        required=True,
        type=str,
        help=
        "Directory within rootfile."
    )
    parser.add_argument(
        "--tree",
        default="ntuple",
        type=str,
        help=
        "Name of the root tree."
    )
    parser.add_argument(
        "--first-entry",
        required=True,
        type=int,
        help=
        "Index of first event to process."
    )
    parser.add_argument(
        "--last-entry",
        required=True,
        type=int,
        help=
        "Index of last event to process."
    )
    return parser.parse_args()


def main(args):

    nickname = os.path.basename(args.input).replace(".root","")
    
    # Get path to cmssw and fakefactordatabase
    cmsswbase = os.environ['CMSSW_BASE']
    fakefactordatabase = {
        "2016" : {
            "et" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2016/SM2016_ML/tight/et/fakeFactors_tight.root",
            "mt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2016/SM2016_ML/tight/mt/fakeFactors_tight.root",
            "tt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2016/SM2016_ML/tight/tt/fakeFactors_tight.root"
                },
        "2017" : {
            "et" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2017/SM2017/tight/vloose/et/fakeFactors.root",
            "mt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2017/SM2017/tight/vloose/mt/fakeFactors.root",
            "tt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2017/SM2017/tight/vloose/tt/fakeFactors.root"
                },
        "2018" : {
            "et" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2018/SM2018/tight/vloose/et/fakeFactors.root",
            "mt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2018/SM2018/tight/vloose/mt/fakeFactors.root",
            "tt" : cmsswbase+"/src/HTTutilities/Jet2TauFakes/data_2018/SM2018/tight/vloose/tt/fakeFactors.root"
                }
        }

    # Determine era
    with open(cmsswbase+"/src/HiggsAnalysis/friend-tree-producer/data/input_params/datasets.json") as json_file:  
        datasets = json.load(json_file)
    era = str(datasets[nickname]["year"])

    # Load fractions
    fractions = ff_tools.load_fractions("", "", True, "%s/src/HiggsAnalysis/friend-tree-producer/data/input_params/htt_ff_fractions_%s.root"%(cmsswbase, era), "", era)
    
    # Create friend tree
    os.mkdir(nickname)
    ff_tools.apply_fake_factors(
        datafile = args.input,
        friendfilelists = { # proper input friend files according to the channel are provided by job manager. Other channels won't be used.
            "et" : args.input_friends,
            "mt" : args.input_friends,
            "tt" : args.input_friends
            },
        outputfile = os.path.join(nickname, "%s_%s_%s_%s.root"%(nickname, args.folder, args.first_entry, args.last_entry)),
        category_mode = "inclusive",
        fakefactordirectories = fakefactordatabase[era],
        fractions = fractions,
        use_fractions_from_worspace = True,
        configpath = cmsswbase+"/src/HiggsAnalysis/fake-factor-application/config.yaml",
        expression = "njets_mvis",
        era = era,
        pipeline_selection = args.folder,
        treename = args.tree,
        eventrange = [args.first_entry, args.last_entry]
        )


if __name__ == "__main__":
    args = parse_arguments()
    setup_logging("FakeFactors_%s_%s_%s_%s.log"%(os.path.basename(args.input).replace(".root",""), args.folder, args.first_entry, args.last_entry),
                  logging.INFO)
    main(args)