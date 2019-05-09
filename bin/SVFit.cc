#include "TauAnalysis/ClassicSVfit/interface/ClassicSVfit.h"
#include "TauAnalysis/ClassicSVfit/interface/MeasuredTauLepton.h"
#include "TauAnalysis/ClassicSVfit/interface/svFitHistogramAdapter.h"
#include "TauAnalysis/ClassicSVfit/interface/FastMTT.h"

#include "TH1F.h"

#include "boost/algorithm/string/predicate.hpp"
#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace classic_svFit;
using boost::starts_with;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
  std::string input = "output.root";
  std::string folder = "mt_nominal";
  std::string tree = "ntuple";
  unsigned int first_entry = 0;
  unsigned int last_entry = 99;
  po::variables_map vm;
  po::options_description config("configuration");
  config.add_options()
    ("input", po::value<std::string>(&input)->default_value(input))
    ("folder", po::value<std::string>(&folder)->default_value(folder))
    ("tree", po::value<std::string>(&tree)->default_value(tree))
    ("first_entry", po::value<unsigned int>(&first_entry)->default_value(first_entry))
    ("last_entry", po::value<unsigned int>(&last_entry)->default_value(last_entry));
  po::store(po::command_line_parser(argc, argv).options(config).run(), vm);
  po::notify(vm);

  // Access input file and tree
  TFile* in = TFile::Open(input.c_str(), "read");
  TDirectoryFile* dir = (TDirectoryFile*) in->Get(folder.c_str());
  TTree* inputtree = (TTree*) dir->Get(tree.c_str());

  // Restrict input tree to needed branches
  inputtree->SetBranchStatus("*",0);

  // Quantities of first lepton
  inputtree->SetBranchStatus("pt_1",1);
  inputtree->SetBranchStatus("eta_1",1);
  inputtree->SetBranchStatus("phi_1",1);
  inputtree->SetBranchStatus("m_1",1);
  inputtree->SetBranchStatus("decayMode_1",1);
  Float_t pt_1,eta_1,phi_1,m_1,decayMode_1;
  inputtree->SetBranchAddress("pt_1",&pt_1);
  inputtree->SetBranchAddress("eta_1",&eta_1);
  inputtree->SetBranchAddress("phi_1",&phi_1);
  inputtree->SetBranchAddress("m_1",&m_1);
  inputtree->SetBranchAddress("decayMode_1",&decayMode_1);

  // Quantities of second lepton
  inputtree->SetBranchStatus("pt_2",1);
  inputtree->SetBranchStatus("eta_2",1);
  inputtree->SetBranchStatus("phi_2",1);
  inputtree->SetBranchStatus("m_2",1);
  inputtree->SetBranchStatus("decayMode_2",1);
  Float_t pt_2,eta_2,phi_2,m_2,decayMode_2;
  inputtree->SetBranchAddress("pt_2",&pt_2);
  inputtree->SetBranchAddress("eta_2",&eta_2);
  inputtree->SetBranchAddress("phi_2",&phi_2);
  inputtree->SetBranchAddress("m_2",&m_2);
  inputtree->SetBranchAddress("decayMode_2",&decayMode_2);

  // Quantities of MET
  inputtree->SetBranchStatus("met",1);
  inputtree->SetBranchStatus("metcov00",1);
  inputtree->SetBranchStatus("metcov01",1);
  inputtree->SetBranchStatus("metcov10",1);
  inputtree->SetBranchStatus("metcov11",1);
  inputtree->SetBranchStatus("metphi",1);
  Float_t met,metcov00,metcov01,metcov10,metcov11,metphi;
  inputtree->SetBranchAddress("met",&met);
  inputtree->SetBranchAddress("metcov00",&metcov00);
  inputtree->SetBranchAddress("metcov01",&metcov01);
  inputtree->SetBranchAddress("metcov10",&metcov10);
  inputtree->SetBranchAddress("metcov11",&metcov11);
  inputtree->SetBranchAddress("metphi",&metphi);

  // Create output tree
  TTree* svfitfriend = new TTree("ntuple","svfit friend tree");

  // ClassicSVFit outputs
  Float_t pt_sv,eta_sv,phi_sv,m_sv;
  svfitfriend->Branch("pt_sv",&pt_sv,"pt_sv/F");
  svfitfriend->Branch("eta_sv",&eta_sv,"eta_sv/F");
  svfitfriend->Branch("phi_sv",&phi_sv,"phi_sv/F");
  svfitfriend->Branch("m_sv",&m_sv,"m_sv/F");

  // FastMTT outputs
  Float_t pt_fastmtt,eta_fastmtt,phi_fastmtt,m_fastmtt;
  svfitfriend->Branch("pt_fastmtt",&pt_fastmtt,"pt_fastmtt/F");
  svfitfriend->Branch("eta_fastmtt",&eta_fastmtt,"eta_fastmtt/F");
  svfitfriend->Branch("phi_fastmtt",&phi_fastmtt,"phi_fastmtt/F");
  svfitfriend->Branch("m_fastmtt",&m_fastmtt,"m_fastmtt/F");

  // Initialize SVFit settings TODO

  // Loop over desired events of the input tree & compute outputs TODO
  for(unsigned int i=first_entry; i <= last_entry; i++)
  {
        std::cout << "Entry: " << i << std::endl;
        inputtree->GetEntry(i);
  }

  // define MET
  double measuredMETx =  11.7491;
  double measuredMETy = -51.9172;

  // define MET covariance
  TMatrixD covMET(2, 2);
  covMET[0][0] =  787.352;
  covMET[1][0] = -178.63;
  covMET[0][1] = -178.63;
  covMET[1][1] =  179.545;

  // define lepton four vectors
  std::vector<MeasuredTauLepton> measuredTauLeptons;
  measuredTauLeptons.push_back(MeasuredTauLepton(MeasuredTauLepton::kTauToElecDecay, 33.7393, 0.9409,  -0.541458, 0.51100e-3)); // tau -> electron decay (Pt, eta, phi, mass)
  measuredTauLeptons.push_back(MeasuredTauLepton(MeasuredTauLepton::kTauToHadDecay,  25.7322, 0.618228, 2.79362,  0.13957, 0)); // tau -> 1prong0pi0 hadronic decay (Pt, eta, phi, mass)
  /*
     tauDecayModes:  0 one-prong without neutral pions
                     1 one-prong with neutral pions
        10 three-prong without neutral pions
  */

  int verbosity = 1;
  ClassicSVfit svFitAlgo(verbosity);

  svFitAlgo.addLogM_fixed(true, 6.);
  svFitAlgo.setLikelihoodFileName("testClassicSVfit.root");

  svFitAlgo.integrate(measuredTauLeptons, measuredMETx, measuredMETy, covMET);
  bool isValidSolution = svFitAlgo.isValidSolution();

  double mass = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getMass();
  double massErr = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getMassErr();
  double transverseMass = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getTransverseMass();
  double transverseMassErr = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getTransverseMassErr();

  if ( isValidSolution ) {
    std::cout << "found valid solution: mass = " << mass << " +/- " << massErr << " (expected value = 115.746 +/- 92.5252),"
              << " transverse mass = " << transverseMass << " +/- " << transverseMassErr << " (expected value = 114.242 +/- 91.2066)" << std::endl;
  } else {
    std::cout << "sorry, failed to find valid solution !!" << std::endl;
  }
  
  // re-run with mass constraint
  double massContraint = 125.06;
  std::cout << "\n\nTesting integration with di tau mass constraint set to " << massContraint << std::endl;
  svFitAlgo.setLikelihoodFileName("testClassicSVfit_withMassContraint.root");
  svFitAlgo.setDiTauMassConstraint(massContraint);
  svFitAlgo.integrate(measuredTauLeptons, measuredMETx, measuredMETy, covMET);
  isValidSolution = svFitAlgo.isValidSolution();
  mass = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getMass();
  massErr = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getMassErr();
  transverseMass = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getTransverseMass();
  transverseMassErr = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getTransverseMassErr();

  if ( isValidSolution ) {
    std::cout << "found valid solution: mass = " << mass << " +/- " << massErr << " (expected value = 124.646 +/- 1.23027),"
              << " transverse mass = " << transverseMass << " +/- " << transverseMassErr << " (expected value = 123.026 +/- 1.1574)" << std::endl;
  } else {
    std::cout << "sorry, failed to find valid solution !!" << std::endl;
  }
  
  // re-run with classic_svFit::TauTauHistogramAdapter
  std::cout << "\n\nTesting integration with classic_svFit::TauTauHistogramAdapter" << std::endl;
  ClassicSVfit svFitAlgo2(verbosity);
  svFitAlgo2.setHistogramAdapter(new classic_svFit::TauTauHistogramAdapter());
  svFitAlgo2.addLogM_fixed(true, 6.);
  svFitAlgo2.setLikelihoodFileName("testClassicSVfit_TauTauHistogramAdapter.root");
  svFitAlgo2.integrate(measuredTauLeptons, measuredMETx, measuredMETy, covMET);
  isValidSolution = svFitAlgo2.isValidSolution();
  classic_svFit::LorentzVector tau1P4 = static_cast<classic_svFit::TauTauHistogramAdapter*>(svFitAlgo2.getHistogramAdapter())->GetFittedTau1LV();
  classic_svFit::LorentzVector tau2P4 = static_cast<classic_svFit::TauTauHistogramAdapter*>(svFitAlgo2.getHistogramAdapter())->GetFittedTau2LV();

  if ( isValidSolution ) {
    std::cout << "found valid solution: pT(tau1) = " << tau1P4.Pt() << " (expected value = 102.508),"
              << "                      pT(tau2) = " << tau2P4.Pt() << " (expected value = 27.019)" << std::endl;
  } else {
    std::cout << "sorry, failed to find valid solution !!" << std::endl;
  }

  //Run FastMTT
  FastMTT aFastMTTAlgo;
  aFastMTTAlgo.run(measuredTauLeptons, measuredMETx, measuredMETy, covMET);
  LorentzVector ttP4 = aFastMTTAlgo.getBestP4();
  std::cout<<std::endl;
  std::cout << "FastMTT found best p4 with mass = " << ttP4.M()
	    << " (expected value = 108.991),"
	    <<std::endl;
  std::cout<<"Real Time =   "<<aFastMTTAlgo.getRealTime("scan")<<" seconds "
	   <<" Cpu Time =   "<<aFastMTTAlgo.getCpuTime("scan")<<" seconds"<<std::endl;

  // Initialize output
  std::string outputname = "friend.root"; //TODO create name from input,folder,first_entry,last_entry
  TFile* out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());
  svfitfriend->Write();
  out->Close();

  return 0;
}
