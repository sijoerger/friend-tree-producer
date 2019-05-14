#include "lwtnn/LightweightGraph.hh"
#include "lwtnn/parse_json.hh"
#include <fstream>

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TVector2.h"
#include "Math/LorentzVector.h"
#include "Math/PtEtaPhiM4D.h"
#include "Math/PxPyPzM4D.h"
#include "Math/Vector4Dfwd.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <iostream>

using boost::starts_with;
namespace po = boost::program_options;

std::string folder_to_channel(std::string foldername) {
  std::vector<std::string> foldername_split;
  boost::split(foldername_split, foldername, [](char c) { return c == '_'; });
  return foldername_split.at(0);
}

std::string filename_from_inputpath(std::string input) {
  std::vector<std::string> path_split;
  boost::split(path_split, input, [](char c) { return c == '/'; });
  std::string filename = path_split.at(path_split.size() - 1);
  boost::replace_all(filename, ".root", "");
  return filename;
}

std::string outputname_from_settings(std::string input, std::string folder,
                                     unsigned int first_entry,
                                     unsigned int last_entry) {
  std::string filename = filename_from_inputpath(input);
  std::string outputname = filename + "/" + filename + "_" + folder + "_" +
                           std::to_string(first_entry) + "_" +
                           std::to_string(last_entry) + ".root";
  return outputname;
}

int main(int argc, char **argv) {
  std::string input = "output.root";
  std::string folder = "mt_nominal";
  std::string tree = "ntuple";
  std::string lwtnn_config = "model.json";
  unsigned int first_entry = 0;
  unsigned int last_entry = 9;
  po::variables_map vm;
  po::options_description config("configuration");
  config.add_options()("input",
                       po::value<std::string>(&input)->default_value(input))(
      "folder", po::value<std::string>(&folder)->default_value(folder))(
      "tree", po::value<std::string>(&tree)->default_value(tree))(
      "first_entry",
      po::value<unsigned int>(&first_entry)->default_value(first_entry))(
      "last_entry",
      po::value<unsigned int>(&last_entry)->default_value(last_entry))(
      "lwtnn_config", po::value<std::string>(&lwtnn_config)->default_value(lwtnn_config));
  po::store(po::command_line_parser(argc, argv).options(config).run(), vm);
  po::notify(vm);

  // Access input file and tree
  auto in = TFile::Open(input.c_str(), "read");
  auto dir = (TDirectoryFile *)in->Get(folder.c_str());
  auto inputtree = (TTree *)dir->Get(tree.c_str());

  // Quantities of first lepton
  Float_t pt_1, eta_1, phi_1, m_1;
  inputtree->SetBranchAddress("pt_1", &pt_1);
  inputtree->SetBranchAddress("eta_1", &eta_1);
  inputtree->SetBranchAddress("phi_1", &phi_1);
  inputtree->SetBranchAddress("m_1", &m_1);

  // Quantities of second lepton
  Float_t pt_2, eta_2, phi_2, m_2;
  inputtree->SetBranchAddress("pt_2", &pt_2);
  inputtree->SetBranchAddress("eta_2", &eta_2);
  inputtree->SetBranchAddress("phi_2", &phi_2);
  inputtree->SetBranchAddress("m_2", &m_2);

  // MET
  float met, metphi;
  inputtree->SetBranchAddress("met", &met);
  inputtree->SetBranchAddress("metphi", &metphi);

  // Initialize output file
  auto outputname =
      outputname_from_settings(input, folder, first_entry, last_entry);
  boost::filesystem::create_directories(filename_from_inputpath(input));
  auto out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());

  // Create output tree
  auto nnfriend = new TTree("ntuple", "NN mass friend tree");

  // NN outputs
  float m_nn, pt_nn, eta_nn, phi_nn;
  float m_1_nn, pt_1_nn, eta_1_nn, phi_1_nn;
  float m_2_nn, pt_2_nn, eta_2_nn, phi_2_nn;
  nnfriend->Branch("m_nn", &m_nn, "m_nn/F");
  nnfriend->Branch("pt_nn", &pt_nn, "pt_nn/F");
  nnfriend->Branch("eta_nn", &eta_nn, "eta_nn/F");
  nnfriend->Branch("phi_nn", &phi_nn, "phi_nn/F");
  nnfriend->Branch("m_1_nn", &m_1_nn, "m_1_nn/F");
  nnfriend->Branch("pt_1_nn", &pt_1_nn, "pt_1_nn/F");
  nnfriend->Branch("eta_1_nn", &eta_1_nn, "eta_1_nn/F");
  nnfriend->Branch("phi_1_nn", &phi_1_nn, "phi_1_nn/F");
  nnfriend->Branch("m_2_nn", &m_2_nn, "m_2_nn/F");
  nnfriend->Branch("pt_2_nn", &pt_2_nn, "pt_2_nn/F");
  nnfriend->Branch("eta_2_nn", &eta_2_nn, "eta_2_nn/F");
  nnfriend->Branch("phi_2_nn", &phi_2_nn, "phi_2_nn/F");

  // Set up lwtnn
  if (!boost::filesystem::exists(lwtnn_config)) {
      throw std::runtime_error("LWTNN config file does not exist.");
  }
  std::ifstream config_file(lwtnn_config);
  auto nnconfig = lwt::parse_json_graph(config_file);
  lwt::LightweightGraph model(nnconfig, "out_0");
    std::map<std::string, std::map<std::string, double> > inputs;

  // Loop over desired events of the input tree & compute outputs
  typedef ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double> > PtEtaPhiMVector;
  typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzM4D<double> > PxPyPzMVector;
  for (unsigned int i = first_entry; i <= last_entry; i++) {
    // Get entry
    inputtree->GetEntry(i);

    // Create four-vectors of reco taus and reco met
    PtEtaPhiMVector rectau1(pt_1, eta_1, phi_1, m_1);
    PtEtaPhiMVector rectau2(pt_2, eta_2, phi_2, m_2);
    PtEtaPhiMVector recmet(met, 0.0, metphi, 0.0);

    // Fill input map
    inputs["in_0"]["t1_rec_px"] = rectau1.Px();
    inputs["in_0"]["t1_rec_py"] = rectau1.Py();
    inputs["in_0"]["t1_rec_pz"] = rectau1.Pz();
    inputs["in_0"]["t1_rec_e"] = rectau1.E();
    inputs["in_0"]["t2_rec_px"] = rectau2.Px();
    inputs["in_0"]["t2_rec_py"] = rectau2.Py();
    inputs["in_0"]["t2_rec_pz"] = rectau2.Pz();
    inputs["in_0"]["t2_rec_e"] = rectau2.E();
    inputs["in_0"]["met_rec_px"] = recmet.Px();
    inputs["in_0"]["met_rec_py"] = recmet.Py();

    // Run computation
    auto outputs = model.compute(inputs);

    // Compute output taus four-vectors
    const double tau_mass = 1.776;
    const auto px_1 = outputs["t1_gen_px"];
    const auto py_1 = outputs["t1_gen_py"];
    const auto pz_1 = outputs["t1_gen_pz"];
    const auto px_2 = outputs["t2_gen_px"];
    const auto py_2 = outputs["t2_gen_py"];
    const auto pz_2 = outputs["t2_gen_pz"];

    PxPyPzMVector gentau1(px_1, py_1, pz_1, tau_mass);
    PxPyPzMVector gentau2(px_2, py_2, pz_2, tau_mass);

    // Compute Higgs four-vector
    const auto higgs = gentau1 + gentau2;

    // Set outputs
    m_nn = higgs.mass();
    pt_nn = higgs.pt();
    eta_nn = higgs.eta();
    phi_nn = higgs.phi();

    m_1_nn = gentau1.mass();
    pt_1_nn = gentau1.pt();
    eta_1_nn = gentau1.eta();
    phi_1_nn = gentau1.phi();

    m_2_nn = gentau2.mass();
    pt_2_nn = gentau2.pt();
    eta_2_nn = gentau2.eta();
    phi_2_nn = gentau2.phi();

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
