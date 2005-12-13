//____________________________________________________________________________
/*!

\class    genie::Target

\brief    A Neutrino Interaction Target. Is a transparent encapsulation of
          quite different physical systems such as a nuclear target, a
          'spectator' nuclear target with a struck nucleon, a free nucleon or
          a free particle (eg a e- target in the inverse muon decay reaction)

\author   Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
          CCLRC, Rutherford Appleton Laboratory

\created  May 03, 2004

*/
//____________________________________________________________________________

#include <sstream>

#include <TParticlePDG.h>

#include "Conventions/Constants.h"
#include "Interaction/Target.h"
#include "Messenger/Messenger.h"
#include "PDG/PDGLibrary.h"
#include "PDG/PDGCodes.h"
#include "PDG/PDGUtils.h"
#include "Utils/PrintUtils.h"

using std::endl;
using std::ostringstream;

using namespace genie;
using namespace genie::constants;

ClassImp(Target)

//____________________________________________________________________________
namespace genie {
 ostream & operator << (ostream & stream, const Target & target)
 {
   target.Print(stream);
   return stream;
 }
}
//___________________________________________________________________________
Target::Target()
{
  this->Init();
}
//___________________________________________________________________________
Target::Target(int pdgc)
{
  this->Init();

  fTgtPDG = pdgc;

  if( pdg::IsIon(pdgc) ) {

     int Z = pdg::IonPdgCodeToZ(pdgc);
     int A = pdg::IonPdgCodeToA(pdgc);

     // set Z,A & fix struck nucleon PDG if tgt = free nucleon
     this->SetZA(Z,A);
  }
}
//___________________________________________________________________________
Target::Target(int Z, int A)
{
  this->Init();

  // set Z,A & fix struck nucleon PDG if tgt = free nucleon

  fTgtPDG = pdg::IonPdgCode(A,Z);

  this->SetZA(Z,A);
}
//___________________________________________________________________________
Target::Target(int Z, int A, int struck_nucleon_pdgc)
{
  this->Init();

  fZ = Z;
  fA = A;

  fTgtPDG = pdg::IonPdgCode(A,Z);

  this->ForceNucleusValidity(); // search for this nucleus at the PDG Ions
  this->SetStruckNucleonPDGCode(struck_nucleon_pdgc);
}
//___________________________________________________________________________
Target::Target(const Target & tgt)
{
  this->Copy(tgt);
}
//___________________________________________________________________________
Target::~Target()
{
  if(fStruckNucP4) delete fStruckNucP4;
}
//___________________________________________________________________________
string Target::AsString(void) const
{
  ostringstream s;

  s << this->PDGCode();
  if(this->StruckNucleonIsSet())
     s << "[N=" << this->StruckNucleonPDGCode() << "]";
  if(this->StruckQuarkIsSet()) {
     s << "[q=" << this->StruckQuarkPDGCode();
     s << (this->StruckQuarkIsFromSea() ? "(s)" : "(v)");
     s << "]";
  }

  return s.str();
}
//___________________________________________________________________________
void Target::Copy(const Target & tgt)
{
  this->Init();

  fTgtPDG = tgt.fTgtPDG;

  if( pdg::IsIon(fTgtPDG) ) {

     fZ = tgt.fZ; // copy A,Z
     fA = tgt.fA;

     fStruckNucPDG   = tgt.fStruckNucPDG;   // struck nucleon PDG
     fStruckQuarkPDG = tgt.fStruckQuarkPDG; // struck quark PDG
     fStruckSeaQuark = tgt.fStruckSeaQuark; // struck quark is from sea?

     if(tgt.fStruckNucP4) {
        fStruckNucP4 = new TLorentzVector(*tgt.fStruckNucP4);
     }
     this->ForceNucleusValidity(); // look it up at the isotopes chart
     this->ForceStruckNucleonValidity(); // must be p or n
  }
}
//___________________________________________________________________________
int Target::Z(void) const
{
  return fZ;
}
//___________________________________________________________________________
int Target::N(void) const
{
  return (fA-fZ);
}
//___________________________________________________________________________
int Target::A(void) const
{
  return fA;
}
//___________________________________________________________________________
int Target::PDGCode(void) const
{
  return fTgtPDG;
}
//___________________________________________________________________________
bool Target::IsFreeNucleon(void) const
{
  return ( fA == 1 && (fZ == 0 || fZ == 1) );
}
//___________________________________________________________________________
bool Target::IsProton(void) const
{
  return ( fA == 1 && fZ == 1 );
}
//___________________________________________________________________________
bool Target::IsNeutron(void) const
{
  return ( fA == 1 && fZ == 0 );
}
//___________________________________________________________________________
bool Target::IsNucleus(void) const
{
  return ( fA > 1 ); // IsValidNucleus() was ensured when A,Z were set
}
//___________________________________________________________________________
bool Target::IsParticle(void) const
{
  TParticlePDG * p = PDGLibrary::Instance()->Find(fTgtPDG);

  return (p && fA==0 && fZ==0);
}
//___________________________________________________________________________
bool Target::StruckNucleonIsSet(void) const
{
  return pdg::IsNeutronOrProton(fStruckNucPDG);
}
//___________________________________________________________________________
bool Target::StruckQuarkIsSet(void) const
{
  return (
     pdg::IsQuark(fStruckQuarkPDG) || pdg::IsAntiQuark(fStruckQuarkPDG)
  );
}
//___________________________________________________________________________
bool Target::StruckQuarkIsFromSea(void) const
{
  return fStruckSeaQuark;
}
//___________________________________________________________________________
void Target::SetZA(int Z, int A)
{
  fZ = Z;
  fA = A;

  this->ForceNucleusValidity(); // search at the isotopes chart

  // if the target is a free nucleon, then the struck nucleon pdg code is
  // automaticaly set

  if( this->IsFreeNucleon() ) {
    if( this->IsProton() ) this->SetStruckNucleonPDGCode(kPdgProton);
    else                   this->SetStruckNucleonPDGCode(kPdgNeutron);
  }
}
//___________________________________________________________________________
void Target::SetStruckNucleonPDGCode(int nucl_pdgc)
{
  fStruckNucPDG = nucl_pdgc;
  bool is_valid = this->ForceStruckNucleonValidity();  // must be p or n

  // If it is a valid struck nucleon pdg code, initialize its 4P:
  // at-rest + on-mass-shell
  if( is_valid ) {
    double M = PDGLibrary::Instance()->Find(nucl_pdgc)->Mass();
    TLorentzVector p4(0,0,0,M);
    this->SetStruckNucleonP4(p4);
  }
}
//___________________________________________________________________________
void Target::SetStruckQuarkPDGCode(int pdgc)
{
  if(pdg::IsQuark(pdgc) || pdg::IsAntiQuark(pdgc)) fStruckQuarkPDG = pdgc;
}
//___________________________________________________________________________
void Target::SetStruckNucleonP4(const TLorentzVector & p4)
{
  if(fStruckNucP4) delete fStruckNucP4;
  fStruckNucP4 = new TLorentzVector(p4);
}
//___________________________________________________________________________
void Target::SetStruckSeaQuark(bool tf)
{
  fStruckSeaQuark = tf;
}
//___________________________________________________________________________
int Target::StruckNucleonPDGCode(void) const
{
  return fStruckNucPDG;
}
//___________________________________________________________________________
int Target::StruckQuarkPDGCode(void) const
{
  return fStruckQuarkPDG;
}
//___________________________________________________________________________
TLorentzVector * Target::StruckNucleonP4(void) const
{
  if(!fStruckNucP4) {
    LOG("Target", pWARN) << "Returning NULL struck nucleon 4-momentum";
    return 0;
  }

  return fStruckNucP4;
}
//___________________________________________________________________________
double Target::Mass(void) const
{
// Shortcut for commonly used code for extracting the nucleus mass from PDG
//
  TParticlePDG * p = PDGLibrary::Instance()->Find(fTgtPDG);
  if(p) return p->Mass(); // in GeV
  return 0.;
}
//___________________________________________________________________________
double Target::Charge(void) const
{
// Shortcut for commonly used code for extracting the nucleus charge from PDG
//
  TParticlePDG * p = PDGLibrary::Instance()->Find(fTgtPDG);
  if(p) return p->Charge() / 3.; // in +e
  return 0;
}
//___________________________________________________________________________
double Target::StruckNucleonMass(void) const
{
  if(!fStruckNucPDG) {
    LOG("Target", pWARN) << "Returning struck nucleon mass = 0";
    return 0;
  }
  return PDGLibrary::Instance()->Find(fStruckNucPDG)->Mass();
}
//___________________________________________________________________________
void Target::Init(void)
{
  fZ = 0;
  fA = 0;

  fTgtPDG         = 0;
  fStruckNucPDG   = 0;
  fStruckQuarkPDG = 0;
  fStruckSeaQuark = false;

  fStruckNucP4  = new TLorentzVector(0,0,0,kNucleonMass);
}
//___________________________________________________________________________
bool Target::ForceStruckNucleonValidity(void)
{
// resets the struck nucleon pdg-code if it found not to be a valid one

  bool valid = pdg::IsProton(fStruckNucPDG) || pdg::IsNeutron(fStruckNucPDG);

  if( ! valid ) {
    LOG("Target", pDEBUG) << "Reseting to struck nucleon to 'Rootino'";
    fStruckNucPDG = 0;
  }
  return valid;
}
//___________________________________________________________________________
void Target::ForceNucleusValidity(void)
{
  if( ! this->IsValidNucleus() ) {
    LOG("Target", pWARN) << "Invalid target -- Reseting to Z = 0, A = 0";
    fZ = 0;
    fA = 0;
  }
}
//___________________________________________________________________________
bool Target::IsValidNucleus(void) const
{
  //-- it is valid if it is a free nucleon...
  if(this->IsFreeNucleon()) return true;

  //-- ... or a nucleus that can be found in the MINOS ion PDG extensions
  int pdg_code = pdg::IonPdgCode(fA, fZ);
  TParticlePDG * p = PDGLibrary::Instance()->Find(pdg_code);

  if(p) return true;
  return false;
}
//___________________________________________________________________________
bool Target::IsEvenEven(void) const
{
  if( this->IsNucleus() ) {

    int N = this->N();
    int Z = this->Z();

    if( N % 2 == 0 && Z % 2 == 0 ) return true;
  }
  return false;
}
//___________________________________________________________________________
bool Target::IsEvenOdd(void) const
{
  if( this->IsNucleus() ) {
      if(! this->IsEvenEven() && ! this->IsOddOdd() ) return true;
  }
  return false;
}
//___________________________________________________________________________
bool Target::IsOddOdd(void) const
{
  if( this->IsNucleus() ) {
    int N = this->N();
    int Z = this->Z();
    if( N % 2 == 1 && Z % 2 == 1 ) return true;
  }
  return false;
}
//___________________________________________________________________________
void Target::Print(ostream & stream) const
{
  stream << " target PDG code = " << fTgtPDG << endl;

  if( this->IsNucleus() || this->IsFreeNucleon() ) {
      stream << " Z = " << fZ << ", A = " << fA << endl;
  }

  if( this->StruckNucleonIsSet() ) {

    TParticlePDG * p = PDGLibrary::Instance()->Find(fStruckNucPDG);

    stream << " struck nucleon = " << p->GetName()
              << ", P4 = " << utils::print::P4AsString(fStruckNucP4) << endl;
  }
}
//___________________________________________________________________________


