#include "ZZMatrixElement/MELA/interface/Mela.h"
#include "ZZMatrixElement/MELA/interface/TUtil.hh"

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TVector2.h"

#include <math.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <iostream>

#include "HiggsAnalysis/friend-tree-producer/interface/HelperFunctions.h"

using boost::starts_with;
namespace po = boost::program_options;

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
  // 1. Matrix element variables for different hypotheses (VBF Higgs, ggH + 2 jets, Z + 2 jets)
  float ME_vbf, ME_ggh, ME_z2j_1, ME_z2j_2;
  melafriend->Branch("ME_ggh", &ME_ggh, "ME_ggh/F");
  melafriend->Branch("ME_vbf", &ME_vbf, "ME_vbf/F");
  melafriend->Branch("ME_z2j_1", &ME_z2j_1, "ME_z2j_1/F");
  melafriend->Branch("ME_z2j_2", &ME_z2j_2, "ME_z2j_2/F");

  // 2. Energy transfer (Q^2) variables
  float ME_q2v1, ME_q2v2;
  melafriend->Branch("ME_q2v1", &ME_q2v1, "ME_q2v1/F");
  melafriend->Branch("ME_q2v2", &ME_q2v2, "ME_q2v2/F");

  // 3. Angle variables
  float ME_costheta1, ME_costheta2, ME_phi, ME_costhetastar, ME_phi1;
  melafriend->Branch("ME_costheta1", &ME_costheta1, "ME_costheta1/F");
  melafriend->Branch("ME_costheta2", &ME_costheta2, "ME_costheta2/F");
  melafriend->Branch("ME_phi", &ME_phi, "ME_phi/F");
  melafriend->Branch("ME_costhetastar", &ME_costhetastar, "ME_costhetastar/F");
  melafriend->Branch("ME_phi1", &ME_phi1, "ME_phi1/F");

 // 4. Main BG vs. Higgs discriminators
  float ME_vbf_vs_Z, ME_ggh_vs_Z, ME_vbf_vs_ggh;
  melafriend->Branch("ME_vbf_vs_Z", &ME_vbf_vs_Z, "ME_vbf_vs_Z/F");
  melafriend->Branch("ME_ggh_vs_Z", &ME_ggh_vs_Z, "ME_ggh_vs_Z/F");
  melafriend->Branch("ME_vbf_vs_ggh", &ME_vbf_vs_ggh, "ME_vbf_vs_ggh/F");

  // Set up MELA
  const int erg_tev = 13;
  const float mPOLE = 125.6;
  TVar::VerbosityLevel verbosity = TVar::SILENT;
  Mela mela(erg_tev, mPOLE, verbosity);

  // Loop over desired events of the input tree & compute outputs
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
      ME_z2j_1 = default_float;
      ME_z2j_2 = default_float;
      ME_vbf_vs_Z = default_float;
      ME_ggh_vs_Z = default_float;

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

    SimpleParticleCollection_t associated2;
    associated2.push_back(SimpleParticle_t(0, jet2));
    associated2.push_back(SimpleParticle_t(0, jet1));

    mela.resetInputEvent();
    mela.setCandidateDecayMode(TVar::CandidateDecay_ff);
    mela.setInputEvent(&daughters, &associated, (SimpleParticleCollection_t *)0, false);

    // Hypothesis: SM VBF Higgs
    mela.setProcess(TVar::HSMHiggs, TVar::JHUGen, TVar::JJVBF);
    mela.computeProdP(ME_vbf, false);
    mela.computeVBFAngles(ME_q2v1, ME_q2v2, ME_costheta1, ME_costheta2, ME_phi, ME_costhetastar, ME_phi1);

    // Hypothesis ggH + 2 jets
    mela.setProcess(TVar::SelfDefine_spin0, TVar::JHUGen, TVar::JJQCD);
    mela.selfDHggcoupl[0][gHIGGS_GG_2][0] = 1;
    mela.computeProdP(ME_ggh, false);

    // Hypothesis: Z + 2 jets
    // Compute the Hypothesis with flipped jets and sum them up for the discriminator.
    mela.setProcess(TVar::bkgZJets, TVar::MCFM, TVar::JJQCD);
    mela.computeProdP(ME_z2j_1, false);

    mela.resetInputEvent();
    mela.setInputEvent(&daughters, &associated2, (SimpleParticleCollection_t *)0, false);
    mela.computeProdP(ME_z2j_2, false);

    // Compute discriminator for VBF vs Z
    if ((ME_vbf + ME_z2j_1 + ME_z2j_2) != 0.0)
    {
        ME_vbf_vs_Z = ME_vbf / (ME_vbf + ME_z2j_1 + ME_z2j_2);
    }
    else
    {
        std::cout << "WARNING: ME_vbf_vs_Z = X / 0. Setting it to default " << default_float << std::endl;
        ME_vbf_vs_Z = default_float;
    }

    // Compute discriminator for ggH vs Z
    if ((ME_ggh + ME_z2j_1 + ME_z2j_2) != 0.0)
    {
        ME_ggh_vs_Z = ME_ggh / (ME_ggh + ME_z2j_1 + ME_z2j_2);
    }
    else
    {
        std::cout << "WARNING: ME_ggh_vs_Z = X / 0. Setting it to default " << default_float << std::endl;
        ME_ggh_vs_Z = default_float;
    }


    // Compute discriminator for VBF vs ggH
    if ((ME_vbf + ME_ggh) != 0.0)
    {
        ME_vbf_vs_ggh = ME_vbf / (ME_vbf + ME_ggh);
    }
    else
    {
        std::cout << "WARNING: ME_vbf_vs_ggh = X / 0. Setting it to default " << default_float << std::endl;
        ME_vbf_vs_ggh = default_float;
    }

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
