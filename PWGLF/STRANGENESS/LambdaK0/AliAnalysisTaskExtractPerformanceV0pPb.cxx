/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Modified version of AliAnalysisTaskCheckCascade.cxx.
// This is a 'hybrid' output version, in that it uses a classic TTree
// ROOT object to store the candidates, plus a couple of histograms filled on
// a per-event basis for storing variables too numerous to put in a tree. 
//
// --- Adapted to look for lambdas as well, using code from 
//        AliAnalysisTaskCheckPerformanceStrange.cxx
//
//  --- Algorithm Description 
//   1. Loop over primaries in stack to acquire generated charged Xi
//   2. Loop over stack to find V0s, fill TH3Fs "PrimRawPt"s for Efficiency
//   3. Perform Physics Selection
//   4. Perform Primary Vertex |z|<10cm selection
//   5. Perform Primary Vertex NoTPCOnly vertexing selection (>0 contrib.)
//   6. Perform Pileup Rejection
//   7. Analysis Loops: 
//    7a. Fill TH3Fs "PrimAnalysisPt" for control purposes only
//    7b. Fill TTree object with V0 information, candidates
//
//  Please Report Any Bugs! 
//
//   --- David Dobrigkeit Chinellato
//        (david.chinellato@gmail.com)
//
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class TTree;
class TParticle;
class TVector3;

//class AliMCEventHandler;
//class AliMCEvent;
//class AliStack;

class AliESDVertex;
class AliAODVertex;
class AliESDv0;
class AliAODv0;

#include <Riostream.h>
#include "TList.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TFile.h"
#include "THnSparse.h"
#include "TVector3.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TLegend.h"
#include "AliLog.h"

#include "AliESDEvent.h"
#include "AliAODEvent.h"
#include "AliV0vertexer.h"
#include "AliCascadeVertexer.h"
#include "AliESDpid.h"
#include "AliESDtrack.h"
#include "AliESDtrackCuts.h"
#include "AliInputEventHandler.h"
#include "AliAnalysisManager.h"
#include "AliMCEventHandler.h"
#include "AliMCEvent.h"
#include "AliStack.h"

#include "AliCFContainer.h"
#include "AliMultiplicity.h"
#include "AliAODMCParticle.h"
#include "AliESDcascade.h"
#include "AliAODcascade.h"
#include "AliESDUtils.h"
#include "AliGenEventHeader.h"
#include "AliAnalysisTaskSE.h"
#include "AliAnalysisUtils.h"
#include "AliAnalysisTaskExtractPerformanceV0pPb.h"
#include "AliHeader.h"
#include "AliGenDPMjetEventHeader.h"

using std::cout;
using std::endl;

ClassImp(AliAnalysisTaskExtractPerformanceV0pPb)

AliAnalysisTaskExtractPerformanceV0pPb::AliAnalysisTaskExtractPerformanceV0pPb()
  : AliAnalysisTaskSE(), fListHistV0(0), fTree(0), fTreeEvents(0), fPIDResponse(0),
  fDiffractiveOnly(kFALSE),

//------------------------------------------------
// Tree Variables 

  fTreeVariablePrimaryStatus(0),
  fTreeVariablePrimaryStatusMother(0),
  fTreeVariableChi2V0(0),
	fTreeVariableDcaV0Daughters(0),
	fTreeVariableDcaV0ToPrimVertex(0),
	fTreeVariableDcaPosToPrimVertex(0),
	fTreeVariableDcaNegToPrimVertex(0),
	fTreeVariableV0CosineOfPointingAngle(0),
	fTreeVariableV0Radius(0),
	fTreeVariablePt(0),
	fTreeVariablePtMC(0),
	fTreeVariableRapK0Short(0),
	fTreeVariableRapLambda(0),
	fTreeVariableRapMC(0),
	fTreeVariableInvMassK0s(0),
	fTreeVariableInvMassLambda(0),
	fTreeVariableInvMassAntiLambda(0),
	fTreeVariableAlphaV0(0),
	fTreeVariablePtArmV0(0),
	fTreeVariableNegTotMomentum(0),
	fTreeVariablePosTotMomentum(0),
	fTreeVariableNegTransvMomentum(0),
	fTreeVariablePosTransvMomentum(0),
	fTreeVariableNegTransvMomentumMC(0),
	fTreeVariablePosTransvMomentumMC(0),
	fTreeVariableNSigmasPosProton(0),
	fTreeVariableNSigmasPosPion(0),
	fTreeVariableNSigmasNegProton(0),
	fTreeVariableNSigmasNegPion(0),
	fTreeVariablePtMother(0),
	fTreeVariableV0CreationRadius(0),
  fTreeVariablePID(0),
  fTreeVariablePIDPositive(0),
  fTreeVariablePIDNegative(0),
  fTreeVariablePIDMother(0),
	fTreeVariableDistOverTotMom(0),
	fTreeVariablePosEta(0),
	fTreeVariableNegEta(0),
	fTreeVariableVertexZ(0),
  fTreeVariableLeastNbrCrossedRows(0),
  fTreeVariableLeastRatioCrossedRowsOverFindable(0),
  fTreeVariableCentrality(0),
  fTreeEventsCentrality(0),

//------------------------------------------------
// HISTOGRAMS
// --- Filled on an Event-by-event basis
//------------------------------------------------
  //V0A Centrality
  fHistCentralityProcessed(0),
  fHistCentralityTrigEvt(0),
  fHistCentralityHasVtx(0),
  fHistCentralityVtxZ(0),

//------------------------------------------------
// PARTICLE HISTOGRAMS
// --- Filled on a Particle-by-Particle basis
//------------------------------------------------

//Standard V0M / multiplicity
  f3dHist_Analysis_PtVsYVsV0A_Lambda(0),
  f3dHist_Analysis_PtVsYVsV0A_AntiLambda(0),
  f3dHist_Analysis_PtVsYVsV0A_K0Short(0),
  f3dHist_Generated_PtVsYVsV0A_Lambda(0),
  f3dHist_Generated_PtVsYVsV0A_AntiLambda(0),
  f3dHist_Generated_PtVsYVsV0A_K0Short(0),
  f3dHist_Analysis_PtVsYVsV0A_KPlus(0),
  f3dHist_Analysis_PtVsYVsV0A_KMinus(0),
  f3dHist_Generated_PtVsYVsV0A_KPlus(0),
  f3dHist_Generated_PtVsYVsV0A_KMinus(0),
  f3dHist_Analysis_PtVsYVsV0A_XiMinus(0), 
  f3dHist_Analysis_PtVsYVsV0A_XiPlus(0), 
  f3dHist_Generated_PtVsYVsV0A_XiMinus(0), 
  f3dHist_Generated_PtVsYVsV0A_XiPlus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_Lambda(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_K0Short(0),
  f3dHist_Generated_PtVsYCMSVsV0A_Lambda(0),
  f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda(0),
  f3dHist_Generated_PtVsYCMSVsV0A_K0Short(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_KPlus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_KMinus(0),
  f3dHist_Generated_PtVsYCMSVsV0A_KPlus(0),
  f3dHist_Generated_PtVsYCMSVsV0A_KMinus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus(0), 
  f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus(0), 
  f3dHist_Generated_PtVsYCMSVsV0A_XiMinus(0), 
  f3dHist_Generated_PtVsYCMSVsV0A_XiPlus(0) 
{
  // Dummy Constructor

}

AliAnalysisTaskExtractPerformanceV0pPb::AliAnalysisTaskExtractPerformanceV0pPb(const char *name) 
  : AliAnalysisTaskSE(name), fListHistV0(0), fTree(0), fTreeEvents(0), fPIDResponse(0), 
  fDiffractiveOnly(kFALSE),

//------------------------------------------------
// Tree Variables 

  fTreeVariablePrimaryStatus(0),
  fTreeVariablePrimaryStatusMother(0),
  fTreeVariableChi2V0(0),
	fTreeVariableDcaV0Daughters(0),
	fTreeVariableDcaV0ToPrimVertex(0),
	fTreeVariableDcaPosToPrimVertex(0),
	fTreeVariableDcaNegToPrimVertex(0),
	fTreeVariableV0CosineOfPointingAngle(0),
	fTreeVariableV0Radius(0),
	fTreeVariablePt(0),
	fTreeVariablePtMC(0),
	fTreeVariableRapK0Short(0),
	fTreeVariableRapLambda(0),
	fTreeVariableRapMC(0),
	fTreeVariableInvMassK0s(0),
	fTreeVariableInvMassLambda(0),
	fTreeVariableInvMassAntiLambda(0),
	fTreeVariableAlphaV0(0),
	fTreeVariablePtArmV0(0),
	fTreeVariableNegTotMomentum(0),
	fTreeVariablePosTotMomentum(0),
	fTreeVariableNegTransvMomentum(0),
	fTreeVariablePosTransvMomentum(0),
	fTreeVariableNegTransvMomentumMC(0),
	fTreeVariablePosTransvMomentumMC(0),
	fTreeVariableNSigmasPosProton(0),
	fTreeVariableNSigmasPosPion(0),
	fTreeVariableNSigmasNegProton(0),
	fTreeVariableNSigmasNegPion(0),
	fTreeVariablePtMother(0),
	fTreeVariableV0CreationRadius(0),
  fTreeVariablePID(0),
  fTreeVariablePIDPositive(0),
  fTreeVariablePIDNegative(0),
  fTreeVariablePIDMother(0),
	fTreeVariableDistOverTotMom(0),
	fTreeVariablePosEta(0),
	fTreeVariableNegEta(0),
	fTreeVariableVertexZ(0),
  fTreeVariableLeastNbrCrossedRows(0),
  fTreeVariableLeastRatioCrossedRowsOverFindable(0),
  fTreeVariableCentrality(0),
  fTreeEventsCentrality(0),

//------------------------------------------------
// HISTOGRAMS
// --- Filled on an Event-by-event basis
//------------------------------------------------
  //V0A Centrality
  fHistCentralityProcessed(0),
  fHistCentralityTrigEvt(0),
  fHistCentralityHasVtx(0),
  fHistCentralityVtxZ(0),

//------------------------------------------------
// PARTICLE HISTOGRAMS
// --- Filled on a Particle-by-Particle basis
//------------------------------------------------

//Standard V0M / multiplicity
  f3dHist_Analysis_PtVsYVsV0A_Lambda(0),
  f3dHist_Analysis_PtVsYVsV0A_AntiLambda(0),
  f3dHist_Analysis_PtVsYVsV0A_K0Short(0),
  f3dHist_Generated_PtVsYVsV0A_Lambda(0),
  f3dHist_Generated_PtVsYVsV0A_AntiLambda(0),
  f3dHist_Generated_PtVsYVsV0A_K0Short(0),
  f3dHist_Analysis_PtVsYVsV0A_KPlus(0),
  f3dHist_Analysis_PtVsYVsV0A_KMinus(0),
  f3dHist_Generated_PtVsYVsV0A_KPlus(0),
  f3dHist_Generated_PtVsYVsV0A_KMinus(0),
  f3dHist_Analysis_PtVsYVsV0A_XiMinus(0), 
  f3dHist_Analysis_PtVsYVsV0A_XiPlus(0), 
  f3dHist_Generated_PtVsYVsV0A_XiMinus(0), 
  f3dHist_Generated_PtVsYVsV0A_XiPlus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_Lambda(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_K0Short(0),
  f3dHist_Generated_PtVsYCMSVsV0A_Lambda(0),
  f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda(0),
  f3dHist_Generated_PtVsYCMSVsV0A_K0Short(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_KPlus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_KMinus(0),
  f3dHist_Generated_PtVsYCMSVsV0A_KPlus(0),
  f3dHist_Generated_PtVsYCMSVsV0A_KMinus(0),
  f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus(0), 
  f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus(0), 
  f3dHist_Generated_PtVsYCMSVsV0A_XiMinus(0), 
  f3dHist_Generated_PtVsYCMSVsV0A_XiPlus(0) 
{
   // Constructor
  // Set Loose cuts or not here...
  DefineOutput(1, TList::Class());
  DefineOutput(2, TTree::Class());
  DefineOutput(3, TTree::Class());
}


AliAnalysisTaskExtractPerformanceV0pPb::~AliAnalysisTaskExtractPerformanceV0pPb()
{
//------------------------------------------------
// DESTRUCTOR
//------------------------------------------------

   if (fListHistV0){
      delete fListHistV0;
      fListHistV0 = 0x0;
   }
   if (fTree){
      delete fTree;
      fTree = 0x0;
   }
   if (fTreeEvents){
      delete fTreeEvents;
      fTreeEvents = 0x0;
   }

}

//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0pPb::UserCreateOutputObjects()
{

   OpenFile(2);	
   // Called once

//------------------------------------------------

   fTree = new TTree("fTree","V0Candidates");

//------------------------------------------------
// fTree Branch definitions - V0 Tree
//------------------------------------------------

//-----------BASIC-INFO---------------------------
/* 1*/   fTree->Branch("fTreeVariablePrimaryStatus",&fTreeVariablePrimaryStatus,"fTreeVariablePrimaryStatus/I");	
/* 2*/   fTree->Branch("fTreeVariablePrimaryStatusMother",&fTreeVariablePrimaryStatusMother,"fTreeVariablePrimaryStatusMother/I");	
/* 2*/   fTree->Branch("fTreeVariableChi2V0",&fTreeVariableChi2V0,"Chi2V0/F");
/* 3*/   fTree->Branch("fTreeVariableDcaV0Daughters",&fTreeVariableDcaV0Daughters,"fTreeVariableDcaV0Daughters/F");
/* 4*/   fTree->Branch("fTreeVariableDcaPosToPrimVertex",&fTreeVariableDcaPosToPrimVertex,"fTreeVariableDcaPosToPrimVertex/F");
/* 5*/   fTree->Branch("fTreeVariableDcaNegToPrimVertex",&fTreeVariableDcaNegToPrimVertex,"fTreeVariableDcaNegToPrimVertex/F");
/* 6*/   fTree->Branch("fTreeVariableV0Radius",&fTreeVariableV0Radius,"fTreeVariableV0Radius/F");
/* 7*/   fTree->Branch("fTreeVariablePt",&fTreeVariablePt,"fTreeVariablePt/F");
/* 7*/   fTree->Branch("fTreeVariablePtMC",&fTreeVariablePtMC,"fTreeVariablePtMC/F");
/* 8*/   fTree->Branch("fTreeVariableRapK0Short",&fTreeVariableRapK0Short,"fTreeVariableRapK0Short/F");
/* 9*/   fTree->Branch("fTreeVariableRapLambda",&fTreeVariableRapLambda,"fTreeVariableRapLambda/F");
/*10*/   fTree->Branch("fTreeVariableRapMC",&fTreeVariableRapMC,"fTreeVariableRapMC/F");
/*11*/   fTree->Branch("fTreeVariableInvMassK0s",&fTreeVariableInvMassK0s,"fTreeVariableInvMassK0s/F");
/*12*/   fTree->Branch("fTreeVariableInvMassLambda",&fTreeVariableInvMassLambda,"fTreeVariableInvMassLambda/F");
/*13*/   fTree->Branch("fTreeVariableInvMassAntiLambda",&fTreeVariableInvMassAntiLambda,"fTreeVariableInvMassAntiLambda/F");
/*14*/   fTree->Branch("fTreeVariableAlphaV0",&fTreeVariableAlphaV0,"fTreeVariableAlphaV0/F");
/*15*/   fTree->Branch("fTreeVariablePtArmV0",&fTreeVariablePtArmV0,"fTreeVariablePtArmV0/F");
/*16*/   fTree->Branch("fTreeVariableNegTransvMomentum",&fTreeVariableNegTransvMomentum,"fTreeVariableNegTransvMomentum/F");
/*17*/   fTree->Branch("fTreeVariablePosTransvMomentum",&fTreeVariablePosTransvMomentum,"fTreeVariablePosTransvMomentum/F");
/*18*/   fTree->Branch("fTreeVariableNegTransvMomentumMC",&fTreeVariableNegTransvMomentumMC,"fTreeVariableNegTransvMomentumMC/F");
/*19*/   fTree->Branch("fTreeVariablePosTransvMomentumMC",&fTreeVariablePosTransvMomentumMC,"fTreeVariablePosTransvMomentumMC/F");
/*20*/   fTree->Branch("fTreeVariableLeastNbrCrossedRows",&fTreeVariableLeastNbrCrossedRows,"fTreeVariableLeastNbrCrossedRows/I");
/*21*/   fTree->Branch("fTreeVariableLeastRatioCrossedRowsOverFindable",&fTreeVariableLeastRatioCrossedRowsOverFindable,"fTreeVariableLeastRatioCrossedRowsOverFindable/F");
/*22*/   fTree->Branch("fTreeVariablePID",&fTreeVariablePID,"fTreeVariablePID/I");
/*23*/   fTree->Branch("fTreeVariablePIDPositive",&fTreeVariablePIDPositive,"fTreeVariablePIDPositive/I");
/*24*/   fTree->Branch("fTreeVariablePIDNegative",&fTreeVariablePIDNegative,"fTreeVariablePIDNegative/I");
/*25*/   fTree->Branch("fTreeVariablePIDMother",&fTreeVariablePIDMother,"fTreeVariablePIDMother/I");
/*26*/   fTree->Branch("fTreeVariablePtXiMother",&fTreeVariablePtMother,"fTreeVariablePtMother/F");
/*27*/   fTree->Branch("fTreeVariableV0CosineOfPointingAngle",&fTreeVariableV0CosineOfPointingAngle,"fTreeVariableV0CosineOfPointingAngle/F");
//-----------MULTIPLICITY-INFO--------------------
/*28*/   fTree->Branch("fTreeVariableCentrality",&fTreeVariableCentrality,"fTreeVariableCentrality/F");
//------------------------------------------------
/*29*/   fTree->Branch("fTreeVariableDistOverTotMom",&fTreeVariableDistOverTotMom,"fTreeVariableDistOverTotMom/F");
/*30*/   fTree->Branch("fTreeVariableNSigmasPosProton",&fTreeVariableNSigmasPosProton,"fTreeVariableNSigmasPosProton/F");
/*31*/   fTree->Branch("fTreeVariableNSigmasPosPion",&fTreeVariableNSigmasPosPion,"fTreeVariableNSigmasPosPion/F");
/*32*/   fTree->Branch("fTreeVariableNSigmasNegProton",&fTreeVariableNSigmasNegProton,"fTreeVariableNSigmasNegProton/F");
/*33*/   fTree->Branch("fTreeVariableNSigmasNegPion",&fTreeVariableNSigmasNegPion,"fTreeVariableNSigmasNegPion/F");
//------------------------------------------------
/*34*/   fTree->Branch("fTreeVariableNegEta",&fTreeVariableNegEta,"fTreeVariableNegEta/F");
/*35*/   fTree->Branch("fTreeVariablePosEta",&fTreeVariablePosEta,"fTreeVariablePosEta/F");
/*36*/   fTree->Branch("fTreeVariableV0CreationRadius",&fTreeVariableV0CreationRadius,"fTreeVariableV0CreationRadius/F");
 
//------------------------------------------------
// fTreeEvents Branch definitions
//------------------------------------------------

   fTreeEvents = new TTree("fTreeEvents","Events");
/*01*/	fTreeEvents->Branch("fTreeEventsCentrality",&fTreeEventsCentrality,"fTreeEventsCentrality/F");


//------------------------------------------------
// Particle Identification Setup
//------------------------------------------------
  
  AliAnalysisManager *man=AliAnalysisManager::GetAnalysisManager();
  AliInputEventHandler* inputHandler = (AliInputEventHandler*) (man->GetInputEventHandler());
  fPIDResponse = inputHandler->GetPIDResponse();
  
//------------------------------------------------
// V0A Centrality Histograms
//------------------------------------------------

   // Create histograms
   OpenFile(1);
   fListHistV0 = new TList();
   fListHistV0->SetOwner();  // See http://root.cern.ch/root/html/TCollection.html#TCollection:SetOwner

  //Default V0A Centrality (if PbPb) 
   if(! fHistCentralityProcessed) {
      fHistCentralityProcessed = new TH1F("fHistCentralityProcessed", 
         "All processed Events;V0A Centrality;Events", 
         200, 0, 100); 		
      fListHistV0->Add(fHistCentralityProcessed);
   }
   if(! fHistCentralityTrigEvt) {
      fHistCentralityTrigEvt = new TH1F("fHistCentralityTrigEvt", 
         "PS selected Events;V0A Centrality;Events", 
         200, 0, 100); 		
      fListHistV0->Add(fHistCentralityTrigEvt);
   }
   if(! fHistCentralityHasVtx) {
      fHistCentralityHasVtx = new TH1F("fHistCentralityHasVtx", 
         "Events having Vertex;V0A Centrality;Events", 
         200, 0, 100); 		
      fListHistV0->Add(fHistCentralityHasVtx);
   }
   if(! fHistCentralityVtxZ) {
      fHistCentralityVtxZ = new TH1F("fHistCentralityVtxZ", 
         "Vertex |z|<10cm;V0A Centrality;Events", 
         200, 0, 100); 		
      fListHistV0->Add(fHistCentralityVtxZ);
   }

//------------------------------------------------
// Track Multiplicity Histograms
//------------------------------------------------

//------------------------------------------------
// Generated Particle Histograms
//------------------------------------------------

   Int_t lCustomNBins = 200; 
   Double_t lCustomPtUpperLimit = 20; 
   Int_t lCustomNBinsMultiplicity = 100;

  Int_t lCustomYNBins = 48; 
  Double_t lCustomMinY = -1.2; 
  Double_t lCustomMaxY = +1.2;

//----------------------------------
// Y in the lab
//----------------------------------

//----------------------------------
// Generated Particle Histos
//----------------------------------

//--- 3D Histo (Pt, Y, Centrality), analysis level

   if(! f3dHist_Analysis_PtVsYVsV0A_Lambda) {
      f3dHist_Analysis_PtVsYVsV0A_Lambda = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_Lambda", "Pt_{lambda} Vs Y_{#Lambda} Vs V0A Centrality; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_Lambda);
   }
   if(! f3dHist_Analysis_PtVsYVsV0A_AntiLambda) {
      f3dHist_Analysis_PtVsYVsV0A_AntiLambda = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_AntiLambda", "Pt_{AntiLambda} Vs Y_{AntiLambda} Vs V0A Centrality; Pt_{AntiLambda} (GeV/c); Y_{AntiLambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_AntiLambda);
   }
   if(! f3dHist_Analysis_PtVsYVsV0A_K0Short) {
      f3dHist_Analysis_PtVsYVsV0A_K0Short = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_K0Short", "Pt_{K0s} Vs Y_{K0s} Vs V0A Centrality; Pt_{K0s} (GeV/c); Y_{K0s} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_K0Short);
   }

//--- 3D Histo (Pt, Y, Centrality), generator level

   if(! f3dHist_Generated_PtVsYVsV0A_Lambda) {
      f3dHist_Generated_PtVsYVsV0A_Lambda = new TH3F( "f3dHist_Generated_PtVsYVsV0A_Lambda", "Pt_{lambda} Vs Y_{#Lambda} Vs V0A Centrality; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_Lambda);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_AntiLambda) {
      f3dHist_Generated_PtVsYVsV0A_AntiLambda = new TH3F( "f3dHist_Generated_PtVsYVsV0A_AntiLambda", "Pt_{AntiLambda} Vs Y_{AntiLambda} Vs V0A Centrality; Pt_{AntiLambda} (GeV/c); Y_{AntiLambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_AntiLambda);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_K0Short) {
      f3dHist_Generated_PtVsYVsV0A_K0Short = new TH3F( "f3dHist_Generated_PtVsYVsV0A_K0Short", "Pt_{K0s} Vs Y_{K0s} Vs V0A Centrality; Pt_{K0s} (GeV/c); Y_{K0s} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_K0Short);
   }

//--------------------------------------------------------------------------------------
// MC Histos for charged Kaons (cross-checking purposes)
//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, V0A Cent) for charged Kaons

   if(! f3dHist_Analysis_PtVsYVsV0A_KPlus) {
      f3dHist_Analysis_PtVsYVsV0A_KPlus = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_KPlus", "Pt_{K+} Vs Y_{K+} Vs Multiplicity; Pt_{K+} (GeV/c); Y_{K+} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_KPlus);
   }
   if(! f3dHist_Analysis_PtVsYVsV0A_KMinus) {
      f3dHist_Analysis_PtVsYVsV0A_KMinus = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_KMinus", "Pt_{K-} Vs Y_{K-} Vs Multiplicity; Pt_{K-} (GeV/c); Y_{K-} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_KMinus);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_KPlus) {
      f3dHist_Generated_PtVsYVsV0A_KPlus = new TH3F( "f3dHist_Generated_PtVsYVsV0A_KPlus", "Pt_{K+} Vs Y_{K+} Vs Multiplicity; Pt_{K+} (GeV/c); Y_{K+} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_KPlus);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_KMinus) {
      f3dHist_Generated_PtVsYVsV0A_KMinus = new TH3F( "f3dHist_Generated_PtVsYVsV0A_KMinus", "Pt_{K-} Vs Y_{K-} Vs Multiplicity; Pt_{K-} (GeV/c); Y_{K-} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_KMinus);
   }

//--------------------------------------------------------------------------------------
// MC Histos for cascades (feeddown)
//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, V0A Cent) 

   if(! f3dHist_Analysis_PtVsYVsV0A_XiMinus) {
      f3dHist_Analysis_PtVsYVsV0A_XiMinus = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_XiMinus", "Pt_{XiMinus} Vs Y_{XiMinus} Vs Multiplicity; Pt_{XiMinus} (GeV/c); Y_{XiMinus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_XiMinus);
   }
   if(! f3dHist_Analysis_PtVsYVsV0A_XiPlus) {
      f3dHist_Analysis_PtVsYVsV0A_XiPlus = new TH3F( "f3dHist_Analysis_PtVsYVsV0A_XiPlus", "Pt_{XiPlus} Vs Y_{XiPlus} Vs Multiplicity; Pt_{XiPlus} (GeV/c); Y_{XiPlus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYVsV0A_XiPlus);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_XiMinus) {
      f3dHist_Generated_PtVsYVsV0A_XiMinus = new TH3F( "f3dHist_Generated_PtVsYVsV0A_XiMinus", "Pt_{XiMinus} Vs Y_{XiMinus} Vs Multiplicity; Pt_{XiMinus} (GeV/c); Y_{XiMinus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_XiMinus);
   }
   if(! f3dHist_Generated_PtVsYVsV0A_XiPlus) {
      f3dHist_Generated_PtVsYVsV0A_XiPlus = new TH3F( "f3dHist_Generated_PtVsYVsV0A_XiPlus", "Pt_{XiPlus} Vs Y_{XiPlus} Vs Multiplicity; Pt_{XiPlus} (GeV/c); Y_{XiPlus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYVsV0A_XiPlus);
   }


//----------------------------------
// Y in the CMS
//----------------------------------

//----------------------------------
// Generated Particle Histos
//----------------------------------

//--- 3D Histo (Pt, Y, Centrality), analysis level

   if(! f3dHist_Analysis_PtVsYCMSVsV0A_Lambda) {
      f3dHist_Analysis_PtVsYCMSVsV0A_Lambda = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_Lambda", "Pt_{lambda} Vs Y_{#Lambda} Vs V0A Centrality; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_Lambda);
   }
   if(! f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda) {
      f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda", "Pt_{AntiLambda} Vs Y_{AntiLambda} Vs V0A Centrality; Pt_{AntiLambda} (GeV/c); Y_{AntiLambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda);
   }
   if(! f3dHist_Analysis_PtVsYCMSVsV0A_K0Short) {
      f3dHist_Analysis_PtVsYCMSVsV0A_K0Short = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_K0Short", "Pt_{K0s} Vs Y_{K0s} Vs V0A Centrality; Pt_{K0s} (GeV/c); Y_{K0s} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_K0Short);
   }

//--- 3D Histo (Pt, Y, Centrality), generator level

   if(! f3dHist_Generated_PtVsYCMSVsV0A_Lambda) {
      f3dHist_Generated_PtVsYCMSVsV0A_Lambda = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_Lambda", "Pt_{lambda} Vs Y_{#Lambda} Vs V0A Centrality; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_Lambda);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda) {
      f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda", "Pt_{AntiLambda} Vs Y_{AntiLambda} Vs V0A Centrality; Pt_{AntiLambda} (GeV/c); Y_{AntiLambda} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_K0Short) {
      f3dHist_Generated_PtVsYCMSVsV0A_K0Short = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_K0Short", "Pt_{K0s} Vs Y_{K0s} Vs V0A Centrality; Pt_{K0s} (GeV/c); Y_{K0s} ; Cent", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_K0Short);
   }

//--------------------------------------------------------------------------------------
// MC Histos for charged Kaons (cross-checking purposes)
//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, V0A Cent) for charged Kaons

   if(! f3dHist_Analysis_PtVsYCMSVsV0A_KPlus) {
      f3dHist_Analysis_PtVsYCMSVsV0A_KPlus = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_KPlus", "Pt_{K+} Vs Y_{K+} Vs Multiplicity; Pt_{K+} (GeV/c); Y_{K+} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_KPlus);
   }
   if(! f3dHist_Analysis_PtVsYCMSVsV0A_KMinus) {
      f3dHist_Analysis_PtVsYCMSVsV0A_KMinus = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_KMinus", "Pt_{K-} Vs Y_{K-} Vs Multiplicity; Pt_{K-} (GeV/c); Y_{K-} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_KMinus);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_KPlus) {
      f3dHist_Generated_PtVsYCMSVsV0A_KPlus = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_KPlus", "Pt_{K+} Vs Y_{K+} Vs Multiplicity; Pt_{K+} (GeV/c); Y_{K+} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_KPlus);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_KMinus) {
      f3dHist_Generated_PtVsYCMSVsV0A_KMinus = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_KMinus", "Pt_{K-} Vs Y_{K-} Vs Multiplicity; Pt_{K-} (GeV/c); Y_{K-} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_KMinus);
   }

//--------------------------------------------------------------------------------------
// MC Histos for cascades (feeddown)
//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, V0A Cent) 

   if(! f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus) {
      f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus", "Pt_{XiMinus} Vs Y_{XiMinus} Vs Multiplicity; Pt_{XiMinus} (GeV/c); Y_{XiMinus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus);
   }
   if(! f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus) {
      f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus = new TH3F( "f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus", "Pt_{XiPlus} Vs Y_{XiPlus} Vs Multiplicity; Pt_{XiPlus} (GeV/c); Y_{XiPlus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_XiMinus) {
      f3dHist_Generated_PtVsYCMSVsV0A_XiMinus = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_XiMinus", "Pt_{XiMinus} Vs Y_{XiMinus} Vs Multiplicity; Pt_{XiMinus} (GeV/c); Y_{XiMinus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_XiMinus);
   }
   if(! f3dHist_Generated_PtVsYCMSVsV0A_XiPlus) {
      f3dHist_Generated_PtVsYCMSVsV0A_XiPlus = new TH3F( "f3dHist_Generated_PtVsYCMSVsV0A_XiPlus", "Pt_{XiPlus} Vs Y_{XiPlus} Vs Multiplicity; Pt_{XiPlus} (GeV/c); Y_{XiPlus} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, lCustomYNBins, lCustomMinY,lCustomMaxY,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHist_Generated_PtVsYCMSVsV0A_XiPlus);
   }


   //List of Histograms: Normal
   PostData(1, fListHistV0);

   //TTree Object: Saved to base directory. Should cache to disk while saving. 
   //(Important to avoid excessive memory usage, particularly when merging)
   PostData(2, fTree);
   PostData(3, fTreeEvents);

}// end UserCreateOutputObjects


//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0pPb::UserExec(Option_t *) 
{
  // Main loop
  // Called for each event

   AliESDEvent *lESDevent = 0x0;
   AliMCEvent  *lMCevent  = 0x0; 
   AliStack    *lMCstack  = 0x0; 

   //Int_t    lNumberOfV0s                      = -1;
   Double_t lTrkgPrimaryVtxPos[3]          = {-100.0, -100.0, -100.0};
   Double_t lBestPrimaryVtxPos[3]          = {-100.0, -100.0, -100.0};
   Double_t lMagneticField                 = -10.;
   Double_t lpARapidityShift = 0.465;
   
  // Connect to the InputEvent   
  // After these lines, we should have an ESD/AOD event + the number of V0s in it.

   // Appropriate for ESD analysis! 
      
   lESDevent = dynamic_cast<AliESDEvent*>( InputEvent() );
   if (!lESDevent) {
      AliWarning("ERROR: lESDevent not available \n");
      return;
   }

   lMCevent = MCEvent();
   if (!lMCevent) {
      Printf("ERROR: Could not retrieve MC event \n");
      cout << "Name of the file with pb :" <<  fInputHandler->GetTree()->GetCurrentFile()->GetName() << endl;   
      return;
   }

   lMCstack = lMCevent->Stack();
   if (!lMCstack) {
      Printf("ERROR: Could not retrieve MC stack \n");
      cout << "Name of the file with pb :" <<  fInputHandler->GetTree()->GetCurrentFile()->GetName() << endl;
      return;
   }
  
   TArrayF mcPrimaryVtx;
   AliGenEventHeader* mcHeader=lMCevent->GenEventHeader();
   if(!mcHeader) return;
   mcHeader->PrimaryVertex(mcPrimaryVtx);
        
  Int_t nPart = 0; 

  //Code Snippet from Alexander for looking at diffractive Events from DPMJet
  if(fDiffractiveOnly){
    AliHeader * header = lMCevent->Header();
    AliGenDPMjetEventHeader* dpmHeader = dynamic_cast<AliGenDPMjetEventHeader*>(header->GenEventHeader());
    if (dpmHeader) nPart = dpmHeader->ProjectileParticipants() + dpmHeader->TargetParticipants();
    //
    Int_t nsdiffrac1 = 0;
    Int_t nsdiffrac2 = 0;
    Int_t nddiffrac  = 0;
    if (dpmHeader) dpmHeader->GetNDiffractive(nsdiffrac1, nsdiffrac2, nddiffrac);
    if (nsdiffrac1 + nsdiffrac2 != nPart) return;
  }

//------------------------------------------------
// Multiplicity Information Acquistion
//------------------------------------------------

   //REVISED multiplicity estimator after 'multiplicity day' (2011)
   Float_t lMultiplicity = -100;
   fTreeVariableCentrality = -100;

   //---> If this is a nuclear collision, then go nuclear on "multiplicity" variable...
   //---> Warning: Experimental
   AliCentrality* centrality;
   centrality = lESDevent->GetCentrality();
   if (centrality->GetQuality()>1) {
     PostData(1, fListHistV0);
     PostData(2, fTree);
     PostData(3, fTreeEvents);
     return;
   }
   lMultiplicity = centrality->GetCentralityPercentile( "V0A" );

   //Set variable for filling tree afterwards!
   //---> Always V0A
  fTreeVariableCentrality = lMultiplicity;
  fTreeEventsCentrality = lMultiplicity;
 
  fHistCentralityProcessed->Fill ( fTreeVariableCentrality );
  
//------------------------------------------------
// MC Information Acquistion
//------------------------------------------------

   Int_t iNumberOfPrimaries = -1;
   iNumberOfPrimaries = lMCstack->GetNprimary();
   if(iNumberOfPrimaries < 1) return; 

//------------------------------------------------
// Variable Definition
//------------------------------------------------

   Int_t lNbMCPrimary        = 0;

   Int_t lPdgcodeCurrentPart = 0;
   Double_t lRapCurrentPart  = 0;
   Double_t lPtCurrentPart   = 0;
  
   //Int_t lComeFromSigma      = 0;

   // current mc particle 's mother
   //Int_t iCurrentMother  = 0;
   lNbMCPrimary = lMCstack->GetNprimary();

//------------------------------------------------
// Pre-Physics Selection
//------------------------------------------------

//----- Loop on primary Xi, Omega --------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0; iCurrentLabelStack < lNbMCPrimary; iCurrentLabelStack++) 
   {// This is the begining of the loop on primaries
      
      TParticle* lCurrentParticlePrimary = 0x0; 
      lCurrentParticlePrimary = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticlePrimary){
         Printf("Cascade loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }
      if ( TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3312 || TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3334 ) { 
         Double_t lRapXiMCPrimary = -100;
         if( (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) != 0 ) { 
           if ( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) !=0 ){
             lRapXiMCPrimary = 0.5*TMath::Log( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) );
           }
         }

         //=================================================================================
         // Xi Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHist_Generated_PtVsYVsV0A_XiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_XiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary+lpARapidityShift, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3312 ){
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHist_Generated_PtVsYVsV0A_XiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_XiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary+lpARapidityShift, lMultiplicity);
         }
      } 
   }
//----- End Loop on primary Xi, Omega ---------------------------------------------------------- 

//----- Loop on Lambda, K0Short ----------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0;  iCurrentLabelStack < (lMCstack->GetNtrack()); iCurrentLabelStack++) 
   {// This is the begining of the loop on tracks
      
      TParticle* lCurrentParticleForLambdaCheck = 0x0; 
      lCurrentParticleForLambdaCheck = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticleForLambdaCheck){
         Printf("V0s loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }

      //=================================================================================
      //Single-Strange checks
      // Keep only K0s, Lambda and AntiLambda:
      lPdgcodeCurrentPart = lCurrentParticleForLambdaCheck->GetPdgCode();	      

      if ( (lCurrentParticleForLambdaCheck->GetPdgCode() == 310   ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == 3122  ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == -3122 ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == 321   ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == -321  ) )
	   {
         lRapCurrentPart   = MyRapidity(lCurrentParticleForLambdaCheck->Energy(),lCurrentParticleForLambdaCheck->Pz());
         lPtCurrentPart    = lCurrentParticleForLambdaCheck->Pt();

         //Use Physical Primaries only for filling PrimRaw Histograms!
         if ( lMCstack->IsPhysicalPrimary(iCurrentLabelStack)!=kTRUE ) continue;

         if( lPdgcodeCurrentPart == 3122 ){
            f3dHist_Generated_PtVsYVsV0A_Lambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_Lambda->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == -3122 ){
            f3dHist_Generated_PtVsYVsV0A_AntiLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_AntiLambda->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == 310 ){
            f3dHist_Generated_PtVsYVsV0A_K0Short->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_K0Short->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == 321 ){
            f3dHist_Generated_PtVsYVsV0A_KPlus->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_KPlus->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == -321 ){
            f3dHist_Generated_PtVsYVsV0A_KMinus->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Generated_PtVsYCMSVsV0A_KMinus->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
      }
   }//End of loop on tracks
//----- End Loop on Lambda, K0Short ------------------------------------------------------------

   lPdgcodeCurrentPart = 0;
   lRapCurrentPart  = 0;
   lPtCurrentPart   = 0;

//------------------------------------------------
// Physics Selection
//------------------------------------------------
  
  // new method
  UInt_t maskIsSelected = ((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected();
  Bool_t isSelected = 0;
  isSelected = (maskIsSelected & AliVEvent::kINT7) == AliVEvent::kINT7;
  
  //Trigger Selection for kINT7
  if ( !isSelected ) {
    PostData(1, fListHistV0);
    PostData(2, fTree);
    PostData(3, fTreeEvents);
    return;
  }
  
//------------------------------------------------
// After Trigger Selection
//------------------------------------------------

  fHistCentralityTrigEvt -> Fill( fTreeVariableCentrality );

//------------------------------------------------
// Getting: Primary Vertex + MagField Info
//------------------------------------------------

   const AliESDVertex *lPrimaryTrackingESDVtx = lESDevent->GetPrimaryVertexTracks();
   // get the vtx stored in ESD found with tracks
   lPrimaryTrackingESDVtx->GetXYZ( lTrkgPrimaryVtxPos );
        
   const AliESDVertex *lPrimaryBestESDVtx = lESDevent->GetPrimaryVertex();	
   // get the best primary vertex available for the event
   // As done in AliCascadeVertexer, we keep the one which is the best one available.
   // between : Tracking vertex > SPD vertex > TPC vertex > default SPD vertex
   // This one will be used for next calculations (DCA essentially)
   lPrimaryBestESDVtx->GetXYZ( lBestPrimaryVtxPos );

   Double_t lPrimaryVtxPosition[3];
   const AliVVertex *primaryVtx = lESDevent->GetPrimaryVertex();
   lPrimaryVtxPosition[0] = primaryVtx->GetX();
   lPrimaryVtxPosition[1] = primaryVtx->GetY();
   lPrimaryVtxPosition[2] = primaryVtx->GetZ();

  //------------------------------------------------
  // Primary Vertex Requirements Section:
  //  ---> pp and PbPb: Only requires |z|<10cm
  //  ---> pPb: all requirements checked at this stage
  //------------------------------------------------
  
  //Roberto's PV selection criteria, implemented 17th April 2013
  
  /* vertex selection */
  Bool_t fHasVertex = kFALSE;
  
  const AliESDVertex *vertex = lESDevent->GetPrimaryVertexTracks();
  if (vertex->GetNContributors() < 1) {
    vertex = lESDevent->GetPrimaryVertexSPD();
    if (vertex->GetNContributors() < 1) fHasVertex = kFALSE;
    else fHasVertex = kTRUE;
    Double_t cov[6]={0};
    vertex->GetCovarianceMatrix(cov);
    Double_t zRes = TMath::Sqrt(cov[5]);
    if (vertex->IsFromVertexerZ() && (zRes>0.25)) fHasVertex = kFALSE;
  }
  else fHasVertex = kTRUE;
  
  //Is First event in chunk rejection: Still present!
  if(fHasVertex == kFALSE) {
    AliWarning("Pb / | PV does not satisfy selection criteria!");
    PostData(1, fListHistV0);
    PostData(2, fTree);
    PostData(3, fTreeEvents);
    return;
  }

  fHistCentralityHasVtx -> Fill ( fTreeVariableCentrality );
  //17 April Fix: Always do primary vertex Z selection, after pA vertex selection from Roberto
  if(TMath::Abs(lBestPrimaryVtxPos[2]) > 10.0) {
    AliWarning("Pb / | pPb case | Z position of Best Prim Vtx | > 10.0 cm ... return !");
    PostData(1, fListHistV0);
    PostData(2, fTree);
    PostData(3, fTreeEvents);
    return;
  }
  
  fHistCentralityVtxZ -> Fill ( fTreeVariableCentrality );  
  lMagneticField = lESDevent->GetMagneticField( );

  //Fill Event Tree: Analysis Selection Level
  fTreeEvents->Fill(); 


//------------------------------------------------
// stack loop starts here
//------------------------------------------------

//---> Loop over ALL PARTICLES
 
   for (Int_t iMc = 0; iMc < (lMCstack->GetNtrack()); iMc++) {  
      TParticle *p0 = lMCstack->Particle(iMc); 
      if (!p0) {
         //Printf("ERROR: particle with label %d not found in lMCstack (mc loop)", iMc);
         continue;
      }
      lPdgcodeCurrentPart = p0->GetPdgCode();

      // Keep only K0s, Lambda and AntiLambda:
      if ( (lPdgcodeCurrentPart != 310 ) && (lPdgcodeCurrentPart != 3122 ) && (lPdgcodeCurrentPart != -3122 ) && (TMath::Abs(lPdgcodeCurrentPart) != 321 ) ) continue;
	
      lRapCurrentPart   = MyRapidity(p0->Energy(),p0->Pz());
      lPtCurrentPart    = p0->Pt();

        //Use Physical Primaries only for filling PrimRaw Histograms!
      if ( lMCstack->IsPhysicalPrimary(iMc)!=kTRUE ) continue;

         if( lPdgcodeCurrentPart == 3122 ){
            f3dHist_Analysis_PtVsYVsV0A_Lambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_Lambda->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == -3122 ){
            f3dHist_Analysis_PtVsYVsV0A_AntiLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_AntiLambda->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == 310 ){
            f3dHist_Analysis_PtVsYVsV0A_K0Short->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_K0Short->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == 321 ){
            f3dHist_Analysis_PtVsYVsV0A_KPlus->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_KPlus->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == -321 ){
            f3dHist_Analysis_PtVsYVsV0A_KMinus->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_KMinus->Fill(lPtCurrentPart, lRapCurrentPart+lpARapidityShift, lMultiplicity);
         }
   }

//----- Loop on primary Xi, Omega --------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0; iCurrentLabelStack < lNbMCPrimary; iCurrentLabelStack++) 
   {// This is the begining of the loop on primaries
      
      TParticle* lCurrentParticlePrimary = 0x0; 
      lCurrentParticlePrimary = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticlePrimary){
         Printf("Cascade loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }
      if ( TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3312 || TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3334 ) { 
         Double_t lRapXiMCPrimary = -100;
         if( (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) != 0 ) { 
           if ( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) !=0 ){
             lRapXiMCPrimary = 0.5*TMath::Log( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) );
           }
         }

         //=================================================================================
         // Xi Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHist_Analysis_PtVsYVsV0A_XiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_XiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary+lpARapidityShift, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3312 ){
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHist_Analysis_PtVsYVsV0A_XiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
            f3dHist_Analysis_PtVsYCMSVsV0A_XiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary+lpARapidityShift, lMultiplicity);
         }
      }
   }
//----- End Loop on primary Xi, Omega ----------------------------------------------------------

//------------------------------------------------
// MAIN LAMBDA LOOP STARTS HERE
//------------------------------------------------

   //Variable definition
   Int_t    lOnFlyStatus = 0;
   Double_t lChi2V0 = 0;
   Double_t lDcaV0Daughters = 0, lDcaV0ToPrimVertex = 0;
   Double_t lDcaPosToPrimVertex = 0, lDcaNegToPrimVertex = 0;
   Double_t lV0CosineOfPointingAngle = 0;
   Double_t lV0Radius = 0, lPt = 0;
   Double_t lRapK0Short = 0, lRapLambda = 0;
   Double_t lInvMassK0s = 0, lInvMassLambda = 0, lInvMassAntiLambda = 0;
   Double_t lAlphaV0 = 0, lPtArmV0 = 0;
   Double_t fMinV0Pt = 0; 
   Double_t fMaxV0Pt = 100; 

   Int_t nv0s = 0;
   nv0s = lESDevent->GetNumberOfV0s();
   
   for (Int_t iV0 = 0; iV0 < nv0s; iV0++) 
	{// This is the begining of the V0 loop
      AliESDv0 *v0 = ((AliESDEvent*)lESDevent)->GetV0(iV0);
      if (!v0) continue;

      Double_t tV0mom[3];
      v0->GetPxPyPz( tV0mom[0],tV0mom[1],tV0mom[2] ); 
      Double_t lV0TotalMomentum = TMath::Sqrt(
         tV0mom[0]*tV0mom[0]+tV0mom[1]*tV0mom[1]+tV0mom[2]*tV0mom[2] );

      Double_t tDecayVertexV0[3]; v0->GetXYZ(tDecayVertexV0[0],tDecayVertexV0[1],tDecayVertexV0[2]); 
      lV0Radius = TMath::Sqrt(tDecayVertexV0[0]*tDecayVertexV0[0]+tDecayVertexV0[1]*tDecayVertexV0[1]);
      lPt = v0->Pt();
      lRapK0Short = v0->RapK0Short();
      lRapLambda  = v0->RapLambda();

      if ((lPt<fMinV0Pt)||(fMaxV0Pt<lPt)) continue;

      UInt_t lKeyPos = (UInt_t)TMath::Abs(v0->GetPindex());
      UInt_t lKeyNeg = (UInt_t)TMath::Abs(v0->GetNindex());

      Double_t lMomPos[3]; v0->GetPPxPyPz(lMomPos[0],lMomPos[1],lMomPos[2]);
      Double_t lMomNeg[3]; v0->GetNPxPyPz(lMomNeg[0],lMomNeg[1],lMomNeg[2]);

      AliESDtrack *pTrack=((AliESDEvent*)lESDevent)->GetTrack(lKeyPos);
      AliESDtrack *nTrack=((AliESDEvent*)lESDevent)->GetTrack(lKeyNeg);
      if (!pTrack || !nTrack) {
         Printf("ERROR: Could not retreive one of the daughter track");
         continue;
      }

      fTreeVariableNegEta = nTrack->Eta();
      fTreeVariablePosEta = pTrack->Eta();

      // Filter like-sign V0 (next: add counter and distribution)
      if ( pTrack->GetSign() == nTrack->GetSign()){
         continue;
      } 

      //________________________________________________________________________
      // Track quality cuts 
      Float_t lPosTrackCrossedRows = pTrack->GetTPCClusterInfo(2,1);
      Float_t lNegTrackCrossedRows = nTrack->GetTPCClusterInfo(2,1);
      fTreeVariableLeastNbrCrossedRows = (Int_t) lPosTrackCrossedRows;
      if( lNegTrackCrossedRows < fTreeVariableLeastNbrCrossedRows )
         fTreeVariableLeastNbrCrossedRows = (Int_t) lNegTrackCrossedRows;

      // TPC refit condition (done during reconstruction for Offline but not for On-the-fly)
      if( !(pTrack->GetStatus() & AliESDtrack::kTPCrefit)) continue;      
      if( !(nTrack->GetStatus() & AliESDtrack::kTPCrefit)) continue;

      //Get status flags
      //fTreeVariablePosTrackStatus = pTrack->GetStatus();
      //fTreeVariableNegTrackStatus = nTrack->GetStatus();
    
      if ( ( ( pTrack->GetTPCClusterInfo(2,1) ) < 70 ) || ( ( nTrack->GetTPCClusterInfo(2,1) ) < 70 ) ) continue;
	
      //GetKinkIndex condition
      if( pTrack->GetKinkIndex(0)>0 || nTrack->GetKinkIndex(0)>0 ) continue;

      //Findable clusters > 0 condition
      if( pTrack->GetTPCNclsF()<=0 || nTrack->GetTPCNclsF()<=0 ) continue;

      //Compute ratio Crossed Rows / Findable clusters
      //Note: above test avoids division by zero! 
      Float_t lPosTrackCrossedRowsOverFindable = -1;
      Float_t lNegTrackCrossedRowsOverFindable = -1;
      if ( ((double)(pTrack->GetTPCNclsF()) ) != 0 ) lPosTrackCrossedRowsOverFindable = lPosTrackCrossedRows / ((double)(pTrack->GetTPCNclsF())); 
      if ( ((double)(nTrack->GetTPCNclsF()) ) != 0 ) lNegTrackCrossedRowsOverFindable = lNegTrackCrossedRows / ((double)(nTrack->GetTPCNclsF())); 

      fTreeVariableLeastRatioCrossedRowsOverFindable = lPosTrackCrossedRowsOverFindable;
      if( lNegTrackCrossedRowsOverFindable < fTreeVariableLeastRatioCrossedRowsOverFindable )
         fTreeVariableLeastRatioCrossedRowsOverFindable = lNegTrackCrossedRowsOverFindable;

      //Lowest Cut Level for Ratio Crossed Rows / Findable = 0.8, set here
      //if ( (fTreeVariableLeastRatioCrossedRowsOverFindable < 0.8)&&(fkTakeAllTracks==kFALSE) ) continue;

      //End track Quality Cuts
      //________________________________________________________________________

      lDcaPosToPrimVertex = TMath::Abs(pTrack->GetD(lPrimaryVtxPosition[0],
							lPrimaryVtxPosition[1],
							lMagneticField) );

      lDcaNegToPrimVertex = TMath::Abs(nTrack->GetD(lPrimaryVtxPosition[0],
							lPrimaryVtxPosition[1],
							lMagneticField) );

      lOnFlyStatus = v0->GetOnFlyStatus();
      lChi2V0 = v0->GetChi2V0();
      lDcaV0Daughters = v0->GetDcaV0Daughters();
      lDcaV0ToPrimVertex = v0->GetD(lPrimaryVtxPosition[0],lPrimaryVtxPosition[1],lPrimaryVtxPosition[2]);
      lV0CosineOfPointingAngle = v0->GetV0CosineOfPointingAngle(lPrimaryVtxPosition[0],lPrimaryVtxPosition[1],lPrimaryVtxPosition[2]);
      fTreeVariableV0CosineOfPointingAngle=lV0CosineOfPointingAngle;

      // Getting invariant mass infos directly from ESD
      v0->ChangeMassHypothesis(310);
      lInvMassK0s = v0->GetEffMass();
      v0->ChangeMassHypothesis(3122);
      lInvMassLambda = v0->GetEffMass();
      v0->ChangeMassHypothesis(-3122);
      lInvMassAntiLambda = v0->GetEffMass();
      lAlphaV0 = v0->AlphaV0();
      lPtArmV0 = v0->PtArmV0();

      //fTreeVariableOnFlyStatus = lOnFlyStatus;
      //fHistV0OnFlyStatus->Fill(lOnFlyStatus);

//===============================================
// Monte Carlo Association starts here
//===============================================

      //---> Set Everything to "I don't know" before starting

      fTreeVariablePIDPositive = 0;
      fTreeVariablePIDNegative = 0;

      //fTreeVariableIndexStatus = 0;
      //fTreeVariableIndexStatusMother = 0;

      fTreeVariablePtMother = -1;
      fTreeVariablePtMC = -1;
      fTreeVariableRapMC = -100;

      fTreeVariablePID = -1; 
      fTreeVariablePIDMother = -1;

      fTreeVariablePrimaryStatus = 0; 
      fTreeVariablePrimaryStatusMother = 0; 
      fTreeVariableV0CreationRadius = -1;
    
      //fTreeVariableNegPhysicalStatus = 0;
      //fTreeVariablePosPhysicalStatus = 0;
    
      Int_t lblPosV0Dghter = (Int_t) TMath::Abs( pTrack->GetLabel() );
      Int_t lblNegV0Dghter = (Int_t) TMath::Abs( nTrack->GetLabel() );
		
      TParticle* mcPosV0Dghter = lMCstack->Particle( lblPosV0Dghter );
      TParticle* mcNegV0Dghter = lMCstack->Particle( lblNegV0Dghter );
	    
      fTreeVariablePosTransvMomentumMC = mcPosV0Dghter->Pt();
      fTreeVariableNegTransvMomentumMC = mcNegV0Dghter->Pt();

      Int_t lPIDPositive = mcPosV0Dghter -> GetPdgCode();
      Int_t lPIDNegative = mcNegV0Dghter -> GetPdgCode();

      fTreeVariablePIDPositive = lPIDPositive;
      fTreeVariablePIDNegative = lPIDNegative;

      Int_t lblMotherPosV0Dghter = mcPosV0Dghter->GetFirstMother() ; 
      Int_t lblMotherNegV0Dghter = mcNegV0Dghter->GetFirstMother();
    
      //if( lMCstack->IsPhysicalPrimary       (lblNegV0Dghter) ) fTreeVariableNegPhysicalStatus = 1; //Is Primary!
      //if( lMCstack->IsSecondaryFromWeakDecay(lblNegV0Dghter) ) fTreeVariableNegPhysicalStatus = 2; //Weak Decay!
      //if( lMCstack->IsSecondaryFromMaterial (lblNegV0Dghter) ) fTreeVariableNegPhysicalStatus = 3; //Material Int!

      //if( lMCstack->IsPhysicalPrimary       (lblPosV0Dghter) ) fTreeVariablePosPhysicalStatus = 1; //Is Primary!
      //if( lMCstack->IsSecondaryFromWeakDecay(lblPosV0Dghter) ) fTreeVariablePosPhysicalStatus = 2; //Weak Decay!
      //if( lMCstack->IsSecondaryFromMaterial (lblPosV0Dghter) ) fTreeVariablePosPhysicalStatus = 3; //Material Int!
    
      if( lblMotherPosV0Dghter == lblMotherNegV0Dghter && lblMotherPosV0Dghter > -1 ){
         //either label is fine, they're equal at this stage
         TParticle* pThisV0 = lMCstack->Particle( lblMotherPosV0Dghter ); 
         //Set tree variables
         fTreeVariablePID   = pThisV0->GetPdgCode(); //PDG Code
         fTreeVariablePtMC  = pThisV0->Pt(); //Perfect Pt

         //Only Interested if it's a Lambda, AntiLambda or K0s 
         //Avoid the Junction Bug! PYTHIA has particles with Px=Py=Pz=E=0 occasionally, 
         //having particle code 88 (unrecognized by PDG), for documentation purposes.
         //Even ROOT's TParticle::Y() is not prepared to deal with that exception!
         //Note that TParticle::Pt() is immune (that would just return 0)...
         //Though granted that that should be extremely rare in this precise condition...
         if( TMath::Abs(fTreeVariablePID) == 3122 || fTreeVariablePID==310 ){
            fTreeVariableRapMC = pThisV0->Y(); //Perfect Y
         }
         fTreeVariableV0CreationRadius = TMath::Sqrt(
          TMath::Power(  ( (mcPrimaryVtx.At(0)) - (pThisV0->Vx()) ) , 2) + 
          TMath::Power(  ( (mcPrimaryVtx.At(1)) - (pThisV0->Vy()) ) , 2) + 
          TMath::Power(  ( (mcPrimaryVtx.At(2)) - (pThisV0->Vz()) ) , 2) 
         );
         //if( lblMotherPosV0Dghter  < lNbMCPrimary ) fTreeVariableIndexStatus = 1; //looks primary
         //if( lblMotherPosV0Dghter >= lNbMCPrimary ) fTreeVariableIndexStatus = 2; //looks secondary
         if( lMCstack->IsPhysicalPrimary       (lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 1; //Is Primary!
         if( lMCstack->IsSecondaryFromWeakDecay(lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 2; //Weak Decay!
         if( lMCstack->IsSecondaryFromMaterial (lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 3; //Material Int!
         
         //Now we try to acquire the V0 parent particle, if possible
         Int_t lblThisV0Parent = pThisV0->GetFirstMother();
         if ( lblThisV0Parent > -1 ){ //if it has a parent, get it and store specs
            TParticle* pThisV0Parent = lMCstack->Particle( lblThisV0Parent );
            fTreeVariablePIDMother   = pThisV0Parent->GetPdgCode(); //V0 Mother PDG
            fTreeVariablePtMother    = pThisV0Parent->Pt();         //V0 Mother Pt
            //Primary Status for the V0 Mother particle 
            //if( lblThisV0Parent  < lNbMCPrimary ) fTreeVariableIndexStatusMother = 1; //looks primary
            //if( lblThisV0Parent >= lNbMCPrimary ) fTreeVariableIndexStatusMother = 2; //looks secondary
            if( lMCstack->IsPhysicalPrimary       (lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 1; //Is Primary!
            if( lMCstack->IsSecondaryFromWeakDecay(lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 2; //Weak Decay!
            if( lMCstack->IsSecondaryFromMaterial (lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 3; //Material Int!
         }
      }

      fTreeVariablePt = v0->Pt();
      fTreeVariableChi2V0 = lChi2V0; 
      fTreeVariableDcaV0ToPrimVertex = lDcaV0ToPrimVertex;
      fTreeVariableDcaV0Daughters = lDcaV0Daughters;
      fTreeVariableV0CosineOfPointingAngle = lV0CosineOfPointingAngle; 
      fTreeVariableV0Radius = lV0Radius;
      fTreeVariableDcaPosToPrimVertex = lDcaPosToPrimVertex;
      fTreeVariableDcaNegToPrimVertex = lDcaNegToPrimVertex;
      fTreeVariableInvMassK0s = lInvMassK0s;
      fTreeVariableInvMassLambda = lInvMassLambda;
      fTreeVariableInvMassAntiLambda = lInvMassAntiLambda;
      fTreeVariableRapK0Short = lRapK0Short;

      fTreeVariableRapLambda = lRapLambda;
      fTreeVariableAlphaV0 = lAlphaV0;
      fTreeVariablePtArmV0 = lPtArmV0;

      //Official means of acquiring N-sigmas
      fTreeVariableNSigmasPosProton = fPIDResponse->NumberOfSigmasTPC( pTrack, AliPID::kProton );
      fTreeVariableNSigmasPosPion   = fPIDResponse->NumberOfSigmasTPC( pTrack, AliPID::kPion );
      fTreeVariableNSigmasNegProton = fPIDResponse->NumberOfSigmasTPC( nTrack, AliPID::kProton );
      fTreeVariableNSigmasNegPion   = fPIDResponse->NumberOfSigmasTPC( nTrack, AliPID::kPion );
    
//tDecayVertexV0[0],tDecayVertexV0[1],tDecayVertexV0[2]
      Double_t lDistanceTravelled = TMath::Sqrt(
						TMath::Power( tDecayVertexV0[0] - lBestPrimaryVtxPos[0] , 2) +
						TMath::Power( tDecayVertexV0[1] - lBestPrimaryVtxPos[1] , 2) +
						TMath::Power( tDecayVertexV0[2] - lBestPrimaryVtxPos[2] , 2)
					);
      fTreeVariableDistOverTotMom = 1e+5;
      if( lV0TotalMomentum + 1e-10 != 0 ) fTreeVariableDistOverTotMom = lDistanceTravelled / (lV0TotalMomentum + 1e-10); //avoid division by zero, to be sure

      Double_t lMomentumPosTemp[3];
      pTrack->GetPxPyPz(lMomentumPosTemp);
      Double_t lPtPosTemporary = sqrt(pow(lMomentumPosTemp[0],2) + pow(lMomentumPosTemp[1],2));

      Double_t lMomentumNegTemp[3];
      nTrack->GetPxPyPz(lMomentumNegTemp);
      Double_t lPtNegTemporary = sqrt(pow(lMomentumNegTemp[0],2) + pow(lMomentumNegTemp[1],2));

      fTreeVariablePosTransvMomentum = lPtPosTemporary;
      fTreeVariableNegTransvMomentum = lPtNegTemporary;


//------------------------------------------------
// Fill Tree! 
//------------------------------------------------

      // The conditionals are meant to decrease excessive
      // memory usage! 

      //Modified version: Keep only OnFlyStatus == 0
      //Keep only if included in a parametric InvMass Region 20 sigmas away from peak

      //First Selection: Reject OnFly
      if(lOnFlyStatus == 0){
         //Second Selection: rough 20-sigma band, parametric. 
         //K0Short: Enough to parametrize peak broadening with linear function.    
         Double_t lUpperLimitK0Short = (5.63707e-01) + (1.14979e-02)*fTreeVariablePt; 
         Double_t lLowerLimitK0Short = (4.30006e-01) - (1.10029e-02)*fTreeVariablePt;
         //Lambda: Linear (for higher pt) plus exponential (for low-pt broadening)
         //[0]+[1]*x+[2]*TMath::Exp(-[3]*x)
         Double_t lUpperLimitLambda = (1.13688e+00) + (5.27838e-03)*fTreeVariablePt + (8.42220e-02)*TMath::Exp(-(3.80595e+00)*fTreeVariablePt); 
         Double_t lLowerLimitLambda = (1.09501e+00) - (5.23272e-03)*fTreeVariablePt - (7.52690e-02)*TMath::Exp(-(3.46339e+00)*fTreeVariablePt);
         //Do Selection      
         if( (fTreeVariableInvMassLambda     < lUpperLimitLambda  && fTreeVariableInvMassLambda     > lLowerLimitLambda     ) || 
             (fTreeVariableInvMassAntiLambda < lUpperLimitLambda  && fTreeVariableInvMassAntiLambda > lLowerLimitLambda     ) || 
             (fTreeVariableInvMassK0s        < lUpperLimitK0Short && fTreeVariableInvMassK0s        > lLowerLimitK0Short    ) ){
             //Pre-selection in case this is AA...
             // ... pre-filter with daughter eta selection only (not TPC)
             if ( TMath::Abs(fTreeVariableNegEta)<0.8 && TMath::Abs(fTreeVariablePosEta)<0.8 ){
               fTree->Fill();
             }
         }
      }

//------------------------------------------------
// Fill tree over.
//------------------------------------------------


   }// This is the end of the V0 loop

//------------------------------------------------

   // Post output data.
   PostData(1, fListHistV0);
   PostData(2, fTree);
   PostData(3, fTreeEvents);
}

//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0pPb::Terminate(Option_t *)
{
   // Draw result to the screen
   // Called once at the end of the query

   TList *cRetrievedList = 0x0;
   cRetrievedList = (TList*)GetOutputData(1);
   if(!cRetrievedList){
      Printf("ERROR - AliAnalysisTaskExtractV0 : ouput data container list not available\n");
      return;
   }	
	
   fHistCentralityProcessed = dynamic_cast<TH1F*> (  cRetrievedList->FindObject("fHistCentralityProcessed")  );
   if (!fHistCentralityProcessed) {
      Printf("ERROR - AliAnalysisTaskExtractV0 : fHistCentralityProcessed not available");
      return;
   }
  
   TCanvas *canCheck = new TCanvas("AliAnalysisTaskExtractV0","V0 Multiplicity",10,10,510,510);
   canCheck->cd(1)->SetLogy();

   fHistCentralityProcessed->SetMarkerStyle(22);
   fHistCentralityProcessed->DrawCopy("E");
}

//----------------------------------------------------------------------------

Double_t AliAnalysisTaskExtractPerformanceV0pPb::MyRapidity(Double_t rE, Double_t rPz) const
{
   // Local calculation for rapidity
   Double_t ReturnValue = -100;
   if( (rE-rPz+1.e-13) != 0 && (rE+rPz) != 0 ){ 
      ReturnValue =  0.5*TMath::Log((rE+rPz)/(rE-rPz+1.e-13));
   }
   return ReturnValue;
}
