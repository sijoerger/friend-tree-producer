#include "ZZMatrixElement/MELA/interface/Mela.h"
#include "ZZMatrixElement/MELA/interface/TUtil.hh"

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TVector2.h"

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
      po::value<unsigned int>(&last_entry)->default_value(last_entry));
  po::store(po::command_line_parser(argc, argv).options(config).run(), vm);
  po::notify(vm);

  // Access input file and tree
  auto in = TFile::Open(input.c_str(), "read");
  auto dir = (TDirectoryFile *)in->Get(folder.c_str());
  auto inputtree = (TTree *)dir->Get(tree.c_str());

  // Quantities of first lepton
  Float_t pt_1, eta_1, phi_1, m_1;
  Int_t decayMode_1;
  inputtree->SetBranchAddress("pt_1", &pt_1);
  inputtree->SetBranchAddress("eta_1", &eta_1);
  inputtree->SetBranchAddress("phi_1", &phi_1);
  inputtree->SetBranchAddress("m_1", &m_1);
  inputtree->SetBranchAddress("decayMode_1", &decayMode_1);

  // Quantities of second lepton
  Float_t pt_2, eta_2, phi_2, m_2;
  Int_t decayMode_2;
  inputtree->SetBranchAddress("pt_2", &pt_2);
  inputtree->SetBranchAddress("eta_2", &eta_2);
  inputtree->SetBranchAddress("phi_2", &phi_2);
  inputtree->SetBranchAddress("m_2", &m_2);
  inputtree->SetBranchAddress("decayMode_2", &decayMode_2);

  // Quantities of MET
  Float_t met, metcov00, metcov01, metcov10, metcov11, metphi;
  inputtree->SetBranchAddress("met", &met);
  inputtree->SetBranchAddress("metcov00", &metcov00);
  inputtree->SetBranchAddress("metcov01", &metcov01);
  inputtree->SetBranchAddress("metcov10", &metcov10);
  inputtree->SetBranchAddress("metcov11", &metcov11);
  inputtree->SetBranchAddress("metphi", &metphi);

  // Initialize output file
  auto outputname =
      outputname_from_settings(input, folder, first_entry, last_entry);
  boost::filesystem::create_directories(filename_from_inputpath(input));
  auto out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());

  // Create output tree
  TTree *melafriend = new TTree("ntuple", "MELA friend tree");

  // MELA outputs
  Float_t pt_sv, eta_sv, phi_sv, m_sv;
  melafriend->Branch("pt_sv", &pt_sv, "pt_sv/F");
  melafriend->Branch("eta_sv", &eta_sv, "eta_sv/F");
  melafriend->Branch("phi_sv", &phi_sv, "phi_sv/F");
  melafriend->Branch("m_sv", &m_sv, "m_sv/F");

  // Set up MELA
  const auto default_float = -10.f;

  // Loop over desired events of the input tree & compute outputs
  for (unsigned int i = first_entry; i <= last_entry; i++) {
    // Get entry
    inputtree->GetEntry(i);

    // Run MELA

    // Fill output tree
    melafriend->Fill();
  }

  // Fill output file
  melafriend->Write("", TObject::kOverwrite);
  out->Close();
  in->Close();

  return 0;
}
