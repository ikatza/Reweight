//____________________________________________________________________________
/*
 Copyright (c) 2003-2006, GENIE Neutrino MC Generator Collaboration
 All rights reserved.
 For the licensing terms see $GENIE/USER_LICENSE.

 Author: Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory - May 13, 2005

 For the class documentation see the corresponding header file.

 Important revisions after version 2.0.0 :

*/
//____________________________________________________________________________

#include "EVGModules/RSPPInteractionListGenerator.h"
#include "EVGCore/InteractionList.h"
#include "Interaction/Interaction.h"
#include "Messenger/Messenger.h"

using namespace genie;

//___________________________________________________________________________
RSPPInteractionListGenerator::RSPPInteractionListGenerator() :
InteractionListGeneratorI("genie::RSPPInteractionListGenerator")
{

}
//___________________________________________________________________________
RSPPInteractionListGenerator::RSPPInteractionListGenerator(string config) :
InteractionListGeneratorI("genie::RSPPInteractionListGenerator", config)
{

}
//___________________________________________________________________________
RSPPInteractionListGenerator::~RSPPInteractionListGenerator()
{

}
//___________________________________________________________________________
InteractionList * RSPPInteractionListGenerator::CreateInteractionList(
                                       const InitialState & init_state) const
{
  LOG("InteractionList", pINFO) << "InitialState = " << init_state.AsString();

  // In the thread generating interactions from the list produced here (SPP), 
  // we can have (for free and nuclear targets):
  //
  // neutrino CC:
  //     v p -> l- p pi+
  //     v n -> l- p pi0
  //     v n -> l- n pi+
  // neutrino NC:
  //     v p -> v p pi0
  //     v p -> v n pi+
  //     v n -> v n pi0
  //     v n -> v p pi-
  // anti-neutrino CC:
  //     vb n -> l+ n pi-
  //     vb p -> l+ n pi0
  //     vb p -> l+ p pi-
  // anti-neutrino NC:
  //     vb p -> vb p pi0
  //     vb p -> vb n pi+
  //     vb n -> vb n pi0
  //     vb n -> vb p pi-
  //
  
  const int n_nucc_channels = 3;
  const int n_nunc_channels = 4;

  SppChannel_t nucc_channels[n_nucc_channels] = {kSppNull};
  SppChannel_t nunc_channels[n_nucc_channels] = {kSppNull};

  int nupdg  = init_state.ProbePdg();

  if( pdg::IsNeutrino(nupdg) ) {
    nucc_channels[0] = kSpp_vp_cc_10100;
    nucc_channels[1] = kSpp_vn_cc_10010;
    nucc_channels[2] = kSpp_vn_cc_01100;
    nunc_channels[0] = kSpp_vp_nc_10010;
    nunc_channels[1] = kSpp_vp_nc_01100;
    nunc_channels[2] = kSpp_vn_nc_01010;
    nunc_channels[3] = kSpp_vn_nc_10001;
  } 
  else if ( pdg::IsAntiNeutrino(nupdg) ) {
    nucc_channels[0] = kSpp_vbn_cc_01001;
    nucc_channels[1] = kSpp_vbp_cc_01010;
    nucc_channels[2] = kSpp_vbp_cc_10001;
    nunc_channels[0] = kSpp_vbp_nc_10010;
    nunc_channels[1] = kSpp_vbp_nc_01100;
    nunc_channels[2] = kSpp_vbn_nc_01010;
    nunc_channels[3] = kSpp_vbn_nc_10001;
  } 
  else {
     LOG("InteractionList", pWARN)
       << "Can not handle probe! Returning NULL InteractionList "
                         << "for init-state: " << init_state.AsString();
     return 0;
  }

  Target * inp_target = init_state.TgtPtr();
  bool hasP = (inp_target->Z() > 0);
  bool hasN = (inp_target->N() > 0);

  InteractionList * intlist = new InteractionList;

  if(fIsCC){

    // CC
    for(int i=0; i<n_nucc_channels; i++) {
       int struck_nucleon = SppChannel::InitStateNucleon(nucc_channels[i]);

       if( (struck_nucleon == kPdgProton  && hasP) ||
           (struck_nucleon == kPdgNeutron && hasN) ) {

          ProcessInfo proc_info(kScResonant, kIntWeakCC);
          Interaction * interaction = new Interaction(init_state, proc_info);

          Target * target = interaction->InitStatePtr()->TgtPtr();

          target->SetHitNucPdg(struck_nucleon);
          this->AddFinalStateInfo(interaction, nucc_channels[i]);

          intlist->push_back(interaction);
       }
    }//cc channels
  
  } else if (fIsNC) {

    // NC
    for(int i=0; i<n_nunc_channels; i++) {
       int struck_nucleon = SppChannel::InitStateNucleon(nunc_channels[i]);

       if( (struck_nucleon == kPdgProton  && hasP) ||
           (struck_nucleon == kPdgNeutron && hasN) ) {

          ProcessInfo proc_info(kScResonant, kIntWeakNC);
          Interaction * interaction = new Interaction(init_state, proc_info);

          Target * target = interaction->InitStatePtr()->TgtPtr();

          target->SetHitNucPdg(struck_nucleon);
          this->AddFinalStateInfo(interaction, nunc_channels[i]);

          intlist->push_back(interaction);
       }
    }//nc channels
  }//cc/nc


  if(intlist->size() == 0) {
     LOG("InteractionList", pERROR)
       << "Returning NULL InteractionList for init-state: "
                                                  << init_state.AsString();
     delete intlist;
     return 0;
  }

  return intlist;
}
//___________________________________________________________________________
void RSPPInteractionListGenerator::AddFinalStateInfo(
                       Interaction * interaction, SppChannel_t sppchan) const
{
  int nproton  = 0;
  int nneutron = 0;
  int npiplus  = 0;
  int npi0     = 0;
  int npiminus = 0;

  int nucpdg = SppChannel::FinStateNucleon(sppchan);
  int pipdg  = SppChannel::FinStatePion(sppchan);

  if       ( nucpdg == kPdgProton  ) nproton  = 1;
  else if  ( nucpdg == kPdgNeutron ) nneutron = 1;
  else {
     LOG("InteractionGenerator", pERROR)
          << "Final state nucleon not a proton or a neutron! (pdg="
                                                             << nucpdg <<")";
  }

  if       ( pipdg == kPdgPiPlus  ) npiplus  = 1;
  else if  ( pipdg == kPdgPi0     ) npi0     = 1;
  else if  ( pipdg == kPdgPiMinus ) npiminus = 1;
  else {
     LOG("InteractionGenerator", pERROR)
              << "Final state pion not a pi+/pi-/pi0! (pdg=" << pipdg <<")";
  }

  XclsTag exclusive_tag;

  exclusive_tag.SetNNucleons (nproton, nneutron);
  exclusive_tag.SetNPions    (npiplus, npi0, npiminus);

  interaction->SetExclTag(exclusive_tag);
}
//___________________________________________________________________________
void RSPPInteractionListGenerator::Configure(const Registry & config)
{
  Algorithm::Configure(config);
  this->LoadConfigData();
}
//____________________________________________________________________________
void RSPPInteractionListGenerator::Configure(string config)
{
  Algorithm::Configure(config);
  this->LoadConfigData();
}
//____________________________________________________________________________
void RSPPInteractionListGenerator::LoadConfigData(void)
{
  fIsCC = fConfig->GetBoolDef("is-CC", false);
  fIsNC = fConfig->GetBoolDef("is-NC", false);
}
//____________________________________________________________________________
