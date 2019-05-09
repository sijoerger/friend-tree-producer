#include "TauAnalysis/ClassicSVfit/interface/ClassicSVfit.h"
#include "TauAnalysis/ClassicSVfit/interface/MeasuredTauLepton.h"
#include "TauAnalysis/ClassicSVfit/interface/svFitHistogramAdapter.h"
#include "TauAnalysis/ClassicSVfit/interface/FastMTT.h"

#include "TH1F.h"
#include "TVector2.h"

#include <boost/algorithm/string.hpp>
#include "boost/algorithm/string/predicate.hpp"
#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace classic_svFit;
using boost::starts_with;
namespace po = boost::program_options;

std::string folder_to_channel(std::string foldername)
{
    std::vector<std::string> foldername_split;
    boost::split(foldername_split,foldername, [](char c){return c == '_';});
    return foldername_split.at(0);
}

float folder_to_kappa_parameter(std::string foldername)
{
    std::string channel = folder_to_channel(foldername);
    if(channel == "em")      return 3.0;
    else if(channel == "et") return 4.0;
    else if(channel == "mt") return 4.0;
    else if(channel == "tt") return 5.0;
    else {std::cout << "Channel determined in a wrong way. Exiting"; exit(1);}
    return -1.0;
}

std::pair<MeasuredTauLepton::kDecayType,MeasuredTauLepton::kDecayType> folder_to_ditaudecay(std::string foldername)
{
    std::string channel = folder_to_channel(foldername);
    if(channel == "em")      return std::make_pair(MeasuredTauLepton::kTauToElecDecay,MeasuredTauLepton::kTauToMuDecay);
    else if(channel == "et") return std::make_pair(MeasuredTauLepton::kTauToElecDecay,MeasuredTauLepton::kTauToHadDecay);
    else if(channel == "mt") return std::make_pair(MeasuredTauLepton::kTauToMuDecay,MeasuredTauLepton::kTauToHadDecay);
    else if(channel == "tt") return std::make_pair(MeasuredTauLepton::kTauToHadDecay,MeasuredTauLepton::kTauToHadDecay);
    else {std::cout << "Channel determined in a wrong way. Exiting"; exit(1);}
    return std::make_pair(MeasuredTauLepton::kUndefinedDecayType,MeasuredTauLepton::kUndefinedDecayType);
}


int main(int argc, char** argv)
{
  std::string input = "output.root";
  std::string folder = "mt_nominal";
  std::string tree = "ntuple";
  unsigned int first_entry = 0;
  unsigned int last_entry = 9;
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
  Float_t pt_1,eta_1,phi_1,m_1;
  Int_t decayMode_1;
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
  Float_t pt_2,eta_2,phi_2,m_2;
  Int_t decayMode_2;
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

  // Initialize output file
  std::string outputname = "happytreefriend.root"; //TODO create name from input,folder,first_entry,last_entry
  TFile* out = TFile::Open(outputname.c_str(), "recreate");
  out->mkdir(folder.c_str());
  out->cd(folder.c_str());

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

  // Initialize SVFit settings
  float kappa_parameter = folder_to_kappa_parameter(folder); // fully-leptonic: 3.0, semi-leptonic: 4.0; fully-hadronic: 5.0
  std::pair<MeasuredTauLepton::kDecayType,MeasuredTauLepton::kDecayType> ditaudecay = folder_to_ditaudecay(folder);

  float default_float = -10.0;

  // Initialize ClassicSVFit
  ClassicSVfit svFitAlgo(0);
  svFitAlgo.addLogM_fixed(true, kappa_parameter);

  // Initialize FastMTT
  FastMTT aFastMTTAlgo;

  // Loop over desired events of the input tree & compute outputs
  for(unsigned int i=first_entry; i <= last_entry; i++)
  {
        std::cout << "Entry: " << i << std::endl;
        inputtree->GetEntry(i);

        // define MET;
        TVector2 metVec;
        metVec.SetMagPhi(met,metphi);

        // define MET covariance
        TMatrixD covMET(2, 2);
        covMET[0][0] = metcov00;
        covMET[1][0] = metcov10;
        covMET[0][1] = metcov01;
        covMET[1][1] = metcov11;

        // define lepton four vectors
        std::vector<MeasuredTauLepton> measuredTauLeptons;
        measuredTauLeptons.push_back(MeasuredTauLepton(ditaudecay.first, pt_1, eta_1, phi_1, m_1, decayMode_1 >= 0 ? decayMode_1 : -1));
        measuredTauLeptons.push_back(MeasuredTauLepton(ditaudecay.second,  pt_2, eta_2, phi_2, m_2, decayMode_2 >= 0 ? decayMode_2 : -1));

        /*
           tauDecayModes:  0 one-prong without neutral pions
                           1 one-prong with neutral pions
              10 three-prong without neutral pions
        */

        // Run ClassicSVFit
        svFitAlgo.integrate(measuredTauLeptons, metVec.X(), metVec.Y(), covMET);
        bool isValidSolution = svFitAlgo.isValidSolution();

        if ( isValidSolution ) {
            pt_sv = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getPt();
            eta_sv = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getEta();
            phi_sv = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getPhi();
            m_sv = static_cast<DiTauSystemHistogramAdapter*>(svFitAlgo.getHistogramAdapter())->getMass();
        } else {
            pt_sv = default_float;
            eta_sv = default_float;
            phi_sv = default_float;
            m_sv = default_float;
        }

        // Run FastMTT
        aFastMTTAlgo.run(measuredTauLeptons,  metVec.X(), metVec.Y(), covMET);
        LorentzVector ttP4 = aFastMTTAlgo.getBestP4();
        pt_fastmtt = ttP4.Pt();
        eta_fastmtt = ttP4.Eta();
        phi_fastmtt = ttP4.Phi();
        m_fastmtt = ttP4.M();

        // Fill output tree
        svfitfriend->Fill();
  }

  // Fill output file
  svfitfriend->Write("",TObject::kOverwrite);
  out->Close();
  in->Close();

  return 0;
}
