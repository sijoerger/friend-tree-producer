#include "lwtnn/LightweightNeuralNetwork.hh"
#include "lwtnn/parse_json.hh"
#include <fstream>

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TVector2.h"
#include "Math/LorentzVector.h"
#include "Math/PtEtaPhiM4D.h"
#include "Math/PxPyPzM4D.h"
#include "Math/Vector4Dfwd.h"
#include "Math/Vector2Dfwd.h"
#include "Math/Vector2D.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>

#include "HiggsAnalysis/friend-tree-producer/interface/HelperFunctions.h"

using boost::starts_with;
namespace po = boost::program_options;

int main(int argc, char **argv) {
  std::string input = "output.root";
  std::vector<std::string> input_friends  = {};
  std::string folder = "mt_nominal";
  std::string tree = "ntuple";
  std::string lwtnn_config = std::string(std::getenv("CMSSW_BASE"))+"/src/HiggsAnalysis/friend-tree-producer/data/inputs_lwtnn/";
  unsigned int first_entry = 0;
  unsigned int last_entry = 9;
  po::variables_map vm;
  po::options_description config("configuration");
  config.add_options()
     ("input",         po::value<std::string>(&input)->default_value(input))
     ("input_friends", po::value<std::vector<std::string>>(&input_friends)->multitoken())
     ("folder",        po::value<std::string>(&folder)->default_value(folder))
     ("tree",          po::value<std::string>(&tree)->default_value(tree))
     ("first_entry",   po::value<unsigned int>(&first_entry)->default_value(first_entry))
     ("last_entry",    po::value<unsigned int>(&last_entry)->default_value(last_entry))
     ("lwtnn_config",  po::value<std::string>(&lwtnn_config)->default_value(lwtnn_config));
  po::store(po::command_line_parser(argc, argv).options(config).run(), vm);
  po::notify(vm);
  // Add additional info inferred from options above
  std::vector<std::string> folder_split;
  boost::split(folder_split, folder, boost::is_any_of("_"));
  std::string channel = folder_split.at(0);
  std::vector<std::string> input_split;
  boost::split(input_split, input, boost::is_any_of("/"));
  std::string nick = input_split.end()[-2];

  // Access input file and tree
  auto in = TFile::Open(input.c_str(), "read");
  auto dir = (TDirectoryFile *)in->Get(folder.c_str());
  auto inputtree = (TTree *)dir->Get(tree.c_str());
  for(std::vector<std::string>::const_iterator s = input_friends.begin(); s != input_friends.end(); ++s)
  {
    inputtree->AddFriend((folder+"/"+tree).c_str(), (&(*s))->c_str());
  }

  // Set up lwtnn
  if (!boost::filesystem::exists(lwtnn_config)) {
      throw std::runtime_error("LWTNN config file does not exist.");
  }

  std::ifstream config_file(lwtnn_config+"/NNrecoil/NNrecoil_lwtnn.json");
  auto nnconfig = lwt::parse_json(config_file);
  auto model = new lwt::LightweightNeuralNetwork(nnconfig.inputs, nnconfig.layers, nnconfig.outputs);

  // Initialize inputs

  // MET inputs
  std::vector<std::string> met_definitions = {"", "track", "nopu", "pucor", "pu", "puppi"};
  std::vector<std::string> met_quantities = {"met", "metphi", "metsumet"};
  std::map<std::string, Float_t> metinputs;
  for(auto metdef : met_definitions)
  {
    for(auto quantity : met_quantities)
    {
      std::string metname = metdef+quantity;
      inputtree->SetBranchAddress((metdef+quantity).c_str(), &(metinputs.find(metname)->second));
    }
  }

  // NPV
  ULong64_t npv;
  inputtree->SetBranchAddress("npv", &npv);

  // Lepton inputs
  Float_t pt_1, pt_2, phi_1, phi_2;
  inputtree->SetBranchAddress("npv", &npv);
  inputtree->SetBranchAddress("pt_1", &pt_1);
  inputtree->SetBranchAddress("pt_2", &pt_2);
  inputtree->SetBranchAddress("phi_1", &phi_1);
  inputtree->SetBranchAddress("phi_2", &phi_2);

  // Initialize output file
  auto outputname =
      outputname_from_settings(input, folder, first_entry, last_entry);
  boost::filesystem::create_directories(filename_from_inputpath(input));
  auto out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());

  // Create output tree
  auto nnfriend = new TTree("ntuple", "NN score friend tree");

  // Initialize outputs for the tree
  std::map<std::string, Float_t> outputs;
  for(size_t n=0; n < nnconfig.outputs.size(); n++)
  {
    outputs[nnconfig.outputs.at(n)] = 0.0;
    nnfriend->Branch((channel+"_"+nnconfig.outputs.at(n)).c_str(), &(outputs.find(nnconfig.outputs.at(n))->second), (channel+"_"+nnconfig.outputs.at(n)+"/F").c_str());
  }
  // TODO: add additional branches NNrecoil_pt, NNrecoil_phi, nnmetpt, nnmetphi

  // Loop over desired events of the input tree & compute outputs
  for (unsigned int i = first_entry; i <= last_entry; i++) {
    // Get entry
    inputtree->GetEntry(i);

    // Convert the inputs from Float_t to double
    std::map<std::string, double> model_inputs;
    for(unsigned int metindex = 0; metindex < met_definitions.size(); ++metindex)
    {
      std::string metdef = met_definitions.at(metindex);
      auto metvec = ROOT::Math::Polar2DVector(metinputs[metdef+met_quantities.at(0)], metinputs[metdef+met_quantities.at(1)]);
      Float_t sumet = metinputs[metdef+met_quantities.at(2)];
      // Subtract di-tau leptons in case of charged met definitions from PV
      if(metindex != 4)
      {
        sumet -= pt_1 + pt_2;
        auto lep1 = ROOT::Math::Polar2DVector(pt_1, phi_1);
        auto lep2 = ROOT::Math::Polar2DVector(pt_2, phi_2);
        metvec -= lep1 + lep2;
      }
      model_inputs[metdef+"metpx"] = metvec.X();
      model_inputs[metdef+"metpy"] = metvec.Y();
      model_inputs[metdef+met_quantities.at(2)] = sumet;
    }
    model_inputs["npv"] = npv;
   
    // Apply model on inputs
    auto model_outputs = model->compute(model_inputs);

    // Fill output map
    for(size_t index=0; index < nnconfig.outputs.size(); index++)
    {
      auto output_value = model_outputs[nnconfig.outputs.at(index)];
      outputs[nnconfig.outputs.at(index)] = output_value;
    }

    // Fill output tree
    nnfriend->Fill();
  }

  // Fill output file
  out->cd(folder.c_str());
  nnfriend->Write("", TObject::kOverwrite);
  out->Close();
  in->Close();

  return 0;
}
