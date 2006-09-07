//____________________________________________________________________________
/*
 Copyright (c) 2003-2006, GENIE Neutrino MC Generator Collaboration
 All rights reserved.
 For the licensing terms see $GENIE/USER_LICENSE.

 Author: Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory - December 05, 2004

 For the class documentation see the corresponding header file.

 Important revisions after version 2.0.0 :

*/
//____________________________________________________________________________

#include <TLorentzVector.h>

#include "EVGCore/ToyInteractionSelector.h"
#include "EVGCore/EventRecord.h"
#include "EVGCore/InteractionList.h"
#include "EVGCore/InteractionGeneratorMap.h"
#include "Interaction/Interaction.h"
#include "Messenger/Messenger.h"
#include "Numerical/RandomGen.h"
#include "PDG/PDGCodes.h"
#include "PDG/PDGUtils.h"

using namespace genie;

//___________________________________________________________________________
ToyInteractionSelector::ToyInteractionSelector() :
InteractionSelectorI("genie::ToyInteractionSelector")
{

}
//___________________________________________________________________________
ToyInteractionSelector::ToyInteractionSelector(string config) :
InteractionSelectorI("genie::ToyInteractionSelector", config)
{

}
//___________________________________________________________________________
ToyInteractionSelector::~ToyInteractionSelector()
{

}
//___________________________________________________________________________
EventRecord * ToyInteractionSelector::SelectInteraction
     (const InteractionGeneratorMap * igmap, const TLorentzVector & p4) const
{
  if(!igmap) {
     LOG("InteractionSelector", pERROR)
           << "\n*** NULL InteractionGeneratorMap! Can't select interaction";
     return 0;
  }
  if(igmap->size() <= 0) {
     LOG("InteractionSelector", pERROR)
          << "\n*** Empty InteractionGeneratorMap! Can't select interaction";
     return 0;
  }

  // select a random event generator
  RandomGen * rnd = RandomGen::Instance();

  const InteractionList & ilst = igmap->GetInteractionList();

  unsigned int nint = ilst.size();
  unsigned int iint = (unsigned int) rnd->RndISel().Integer(nint);

  Interaction * interaction = ilst[iint];

  // clone interaction
  Interaction * selected_interaction = new Interaction( *interaction );
  selected_interaction->InitStatePtr()->SetProbeP4(p4);
  LOG("InteractionSelector", pINFO)
                   << "Interaction to generate: \n" << *selected_interaction;

  // bootstrap the event record
  EventRecord * evrec = new EventRecord;
  evrec->AttachSummary(selected_interaction);

  return evrec;
}
//___________________________________________________________________________
