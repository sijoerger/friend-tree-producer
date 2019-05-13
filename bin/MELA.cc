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
  float q_1, q_2;
  inputtree->SetBranchAddress("pt_1", &pt_1);
  inputtree->SetBranchAddress("eta_1", &eta_1);
  inputtree->SetBranchAddress("phi_1", &phi_1);
  inputtree->SetBranchAddress("m_1", &m_1);
  inputtree->SetBranchAddress("q_1", &q_1);

  // Quantities of second lepton
  Float_t pt_2, eta_2, phi_2, m_2;
  inputtree->SetBranchAddress("pt_2", &pt_2);
  inputtree->SetBranchAddress("eta_2", &eta_2);
  inputtree->SetBranchAddress("phi_2", &phi_2);
  inputtree->SetBranchAddress("m_2", &m_2);
  inputtree->SetBranchAddress("q_2", &q_2);

  // Quantities of first jet
  Float_t jpt_1, jeta_1, jphi_1, jm_1;
  int njets;
  inputtree->SetBranchAddress("njets", &njets);
  inputtree->SetBranchAddress("jpt_1", &jpt_1);
  inputtree->SetBranchAddress("jeta_1", &jeta_1);
  inputtree->SetBranchAddress("jphi_1", &jphi_1);
  inputtree->SetBranchAddress("jm_1", &jm_1);

  // Quantities of second jet
  Float_t jpt_2, jeta_2, jphi_2, jm_2;
  inputtree->SetBranchAddress("jpt_2", &jpt_2);
  inputtree->SetBranchAddress("jeta_2", &jeta_2);
  inputtree->SetBranchAddress("jphi_2", &jphi_2);
  inputtree->SetBranchAddress("jm_2", &jm_2);

  // Initialize output file
  auto outputname =
      outputname_from_settings(input, folder, first_entry, last_entry);
  boost::filesystem::create_directories(filename_from_inputpath(input));
  auto out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());

  // Create output tree
  auto melafriend = new TTree("ntuple", "MELA friend tree");

  // MELA outputs
  float ME_vbf, ME_q2v1, ME_q2v2, ME_costheta1, ME_costheta2, ME_phi, ME_costhetastar, ME_phi1;
  melafriend->Branch("ME_vbf", &ME_vbf, "ME_vbf/F");
  melafriend->Branch("ME_q2v1", &ME_q2v1, "ME_q2v1/F");
  melafriend->Branch("ME_q2v2", &ME_q2v2, "ME_q2v2/F");
  melafriend->Branch("ME_costheta1", &ME_costheta1, "ME_costheta1/F");
  melafriend->Branch("ME_costheta2", &ME_costheta2, "ME_costheta2/F");
  melafriend->Branch("ME_phi", &ME_phi, "ME_phi/F");
  melafriend->Branch("ME_costhetastar", &ME_costhetastar, "ME_costhetastar/F");
  melafriend->Branch("ME_phi1", &ME_phi1, "ME_phi1/F");
  // float ME_z2j;
  //melafriend->Branch("ME_z2j", &ME_z2j, "ME_z2j/F");

  // Set up MELA
  const int erg_tev = 13;
  const float mPOLE = 125.6;
  TVar::VerbosityLevel verbosity = TVar::SILENT;
  Mela mela(erg_tev, mPOLE, verbosity);

  // Loop over desired events of the input tree & compute outputs
  const auto default_float = -10.f;
  for (unsigned int i = first_entry; i <= last_entry; i++) {
    // Get entry
    inputtree->GetEntry(i);

    // Fill defaults for events without two jets
    if (njets < 2) {
      ME_vbf= default_float;
      ME_q2v1 = default_float;
      ME_q2v2 = default_float;
      ME_costheta1 = default_float;
      ME_costheta2 = default_float;
      ME_phi = default_float;
      ME_costhetastar = default_float;
      ME_phi1 = default_float;
      //ME_z2j= default_float;
      melafriend->Fill();
      continue;
    }

    // Sanitize charge for application on same-sign events
    if (q_1 * q_2 > 0) {
      q_2 = -q_1;
    }

    // Build four-vectors
    TLorentzVector tau1, tau2;
    tau1.SetPtEtaPhiM(pt_1, eta_1, phi_1, m_1);
    tau2.SetPtEtaPhiM(pt_2, eta_2, phi_2, m_2);

    // FIXME: TODO: Why do we not use the jet mass here?
    TLorentzVector jet1, jet2;
    jet1.SetPtEtaPhiM(jpt_1, jeta_1, jphi_1, 0);
    jet2.SetPtEtaPhiM(jpt_2, jeta_2, jphi_2, 0);

    // Run MELA
    SimpleParticleCollection_t daughters;
    daughters.push_back(SimpleParticle_t(15 * q_1, tau1));
    daughters.push_back(SimpleParticle_t(15 * q_2, tau2));

    SimpleParticleCollection_t associated;
    associated.push_back(SimpleParticle_t(0, jet1));
    associated.push_back(SimpleParticle_t(0, jet2));

    mela.resetInputEvent();
    mela.setCandidateDecayMode(TVar::CandidateDecay_ff);
    mela.setInputEvent(&daughters, &associated, (SimpleParticleCollection_t *)0,
                       false);

    // Hypothesis: SM Higgs
    mela.setProcess(TVar::HSMHiggs, TVar::JHUGen, TVar::JJVBF);
    mela.computeProdP(ME_vbf, false);
    mela.computeVBFAngles(ME_q2v1, ME_q2v2, ME_costheta1, ME_costheta2, ME_phi, ME_costhetastar, ME_phi1);

    // Hypothesis: Z + 2 jets
    // Following hypothesis fails due to following error:
    // MYLHE format not implemented for    634894960
    /*
    mela.setProcess(TVar::bkgZJets, TVar::MCFM, TVar::JJQCD);
    mela.computeProdP(ME_z2j, false);
    */

    // Fill output tree
    melafriend->Fill();
  }

  // Fill output file
  out->cd(folder.c_str());
  melafriend->Write("", TObject::kOverwrite);
  out->Close();
  in->Close();

  return 0;
}
