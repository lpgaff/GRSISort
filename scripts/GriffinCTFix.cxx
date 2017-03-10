#include <iostream>

#include "TFile.h"
#include "TH2.h"
#include "TF1.h"
#include "TCutG.h"
#include "TGraphErrors.h"
#include "TString.h"
#include "TObjString.h"
#include "TObjArray.h"
#include "TProfile.h"

#include "TChannel.h"
#include "TGriffin.h"

double CrossTalkFit(double* x, double* par){
   double k0 = par[0]/(1.-par[0]);
   double k1 = par[1]/(1.-par[1]);
   double energy = par[2];

   double slope = -(1.+k0)/(1.+k1);
   double intercept = energy*(1.+k0/(1.+k1));

   return x[0]*slope + intercept;

}

double *CrossTalkFix(int det, double energy, TFile* in_file) {
   //The outfile is implicit since it was the last file that was open.
   static double largest_correction = 0.0;

   //Clear all of the CT calibrations for this detector.
   for(int i = 0; i<4; ++i){
      TChannel* chan = TChannel::FindChannelByName(Form("GRG%02d%sN00A",det,TGriffin::GetColorFromNumber(i)));
      if(!chan){
         std::cout << DRED << "Couldn't find a channel for " << Form("GRG%02d%sN00A",det,TGriffin::GetColorFromNumber(i)) << RESET_COLOR << std::endl;
         continue;
      }
      chan->DestroyCTCal();
   }

   static int largest_det = -1;
   static int largest_crystal1=-1;
   static int largest_crystal2=-1;

   std::string namebase = Form("det_%d",det);
   std::vector<TH2*> mats;
   for(int i=0;i<4;i++) {
      for(int j=i+1;j<4;j++) {
         std::string name = Form("%s_%d_%d",namebase.c_str(),i,j);
         TH2 *m = (TH2*)in_file->Get(name.c_str());
         if(!m) {
            std::cout << "can not find:  " << name << std::endl;
            return 0;
         }
         mats.push_back((TH2*)in_file->Get(name.c_str()));     
      }
   }
  
   double low_cut  = energy - 15;
   double high_cut = energy + 15;

   double xpts[5] = {low_cut,   0,   0,high_cut,low_cut};
   double ypts[5] = {   0,low_cut,high_cut,   0,   0};
   TCutG cut("cut",5,xpts,ypts);

   double *d = new double[16];//[4][4];
   double *e_d = new double[16];//[4][4];

   for(size_t x=0;x<mats.size();x++) {
      TH2 *mat = mats.at(x);
      std::cout << mat->GetName() << std::endl;
      //TH2 *cmat = (TH2*)(mat->DrawCopy("colz [cut]"));
      int xbins = mat->GetNbinsX();
      int ybins = mat->GetNbinsY();
   
      TH2 *cmat = (TH2*)mat->Clone(Form("%s_clone",mat->GetName()));
      cmat->Reset();
   
      TGraphErrors *fitGraph = new TGraphErrors;
       fitGraph->SetNameTitle(Form("%s_graph",mat->GetName()),"Graph");

      for(int i=1;i<=xbins;i++) {
         bool inside_yet = false;
         int low_bin_in_cut = 0;
         int high_bin_in_cut = 0;
         for(int j=1;j<=ybins;j++) {
            double xc = mat->GetXaxis()->GetBinCenter(i);
            double yc = mat->GetYaxis()->GetBinCenter(j);
            if(cut.IsInside(xc,yc)) {
               if(!inside_yet){
                  low_bin_in_cut = j;
                  inside_yet = true;
               }
               cmat->Fill(xc,yc,mat->GetBinContent(i,j));
            } 
            else if(inside_yet){
               high_bin_in_cut = j;
            }
         }
         cmat->GetXaxis()->SetRange(i,i);
         if(cmat->Integral() > 4){
            fitGraph->SetPoint(fitGraph->GetN(),cmat->GetYaxis()->GetBinCenter(i),cmat->GetMean(2));
            fitGraph->SetPointError(fitGraph->GetN()-1,cmat->GetXaxis()->GetBinWidth(i),cmat->GetMeanError(2));
         }
         cmat->GetXaxis()->UnZoom();
      }
      cmat->Write();
    
      TString name = mat->GetName();
      TObjArray *strings = name.Tokenize("_"); 
      int xind = ((TObjString*)(strings->At(strings->GetEntries()-1)))->String().Atoi();
      int yind = ((TObjString*)(strings->At(strings->GetEntries()-2)))->String().Atoi();
    

      TF1 *fpx = new TF1(Form("pxfit_%i_%i",yind,xind),CrossTalkFit,6,1167,3);
      fpx->SetParameter(0,0.0001); fpx->SetParameter(1,0.0001); 
      fpx->SetParameter(2,energy); fpx->FixParameter(2,energy);
      fitGraph->Fit(fpx);
      fitGraph->Write();
      TProfile *px = cmat->ProfileX();

      px->Write();
      //Make a residuals plot
      TH1 *residual_plot = new TH1D(Form("%s_resid",fpx->GetName()),"residuals",2000,0,2000);
      for(int i=0; i<residual_plot->GetNbinsX(); ++i){
         if(px->GetBinContent(i) != 0)
         residual_plot->SetBinContent(i,px->GetBinContent(i) - fpx->Eval(residual_plot->GetBinCenter(i)));
         residual_plot->SetBinError(i,px->GetBinError(i));
      }
      residual_plot->Write();

      std::cout << "=====================" << std::endl;
      std::cout << mat->GetName() << std::endl;
      std::cout << "d" << xind << yind << " at zero   " << (fpx->Eval(energy))/energy  << std::endl;
      std::cout << "=====================" << std::endl;
    
      d[xind*4+yind] = fpx->GetParameter(0);
      d[yind*4+xind] = fpx->GetParameter(1);
      e_d[xind*4+yind] = fpx->GetParError(0);
      e_d[yind*4+xind] = fpx->GetParError(1);

      if(fpx->GetParameter(0) > largest_correction){
         largest_correction = fpx->GetParameter(0);
         largest_det = det;
         largest_crystal1 = xind;
         largest_crystal2 = yind;
      }
      if(fpx->GetParameter(1) > largest_correction){
         largest_correction = fpx->GetParameter(1);
         largest_det = det;
         largest_crystal1 = xind;
         largest_crystal2 = yind;
      }

   }

   std::cout << " -------------------- " << std::endl;
   std::cout << " -------------------- " << std::endl;
   std::cout << std::endl;

   for(int i=0;i<4;i++) {
      for(int j=0;j<4;j++) {
         if(i==j){ 
            d[i*4+j] = 0.0000;
            e_d[i*4+j] = 0.0000;
         }
         std::cout << d[j*4+i] << "\t";// << "+/-" << e_d[i*4+j]<<"\t";
         
         //Time to find the proper channels and build the corrections xind/i is row number
       //  std::cout << "Looking for: " <<Form("GRG%02d%sN00A",det,TGriffin::GetColorFromNumber(j)) << std::endl;
         TChannel* chan = TChannel::FindChannelByName(Form("GRG%02d%sN00A",det,TGriffin::GetColorFromNumber(j)));
         if(!chan){
            std::cout << DRED << "Couldn't find a channel for " << Form("GRG%02d%sN00A",det,TGriffin::GetColorFromNumber(j)) << RESET_COLOR << std::endl;
            continue;
         }
         //chan->AddCTCoefficient(d[j*4+i]);
         chan->AddCTCoefficient(d[i*4+j]);
      }
      std::cout << std::endl;
   }

   std::cout << std::endl;
   std::cout << "Largest correction = " << largest_correction << " Shift = " << largest_correction*energy << std::endl;
   std::cout << "Largest combo, det = " << largest_det << " Crystals = " << largest_crystal1 << ", " << largest_crystal2 << std::endl;
   std::cout << " -------------------- " << std::endl;
   std::cout << " -------------------- " << std::endl;
   return d;
}

void FixAll(TFile* in_file, TFile* out_file) {
   double energies[2] = {1173.228, 1332.492};
   for(int d=1;d<=16;d++) {
      CrossTalkFix(d,energies[1],in_file);
   }
}

#ifndef __CINT__ 
int main(int argc, char **argv) {
   //Do basic file checks
   if(argc != 3) {
      printf("try again (usage: %s <matrix file> <cal file>\n",argv[0]);
      return 0;
   }

   if(TChannel::ReadCalFile(argv[2]) < 0){
      std::cout << "Aborting" << std::endl;
      exit(1);
   }

   TFile* in_file = new TFile(argv[1]);

   if(in_file == nullptr) {
      printf("Failed to open file '%s'!\n",argv[1]);
      return 1;
   }
   if(!in_file->IsOpen()) {
      printf("Failed to open file '%s'!\n",argv[1]);
      return 1;
   }
   
   //Create and output file.
   TFile * out_file = new TFile(Form("ct_%s",in_file->GetName()),"RECREATE");
   
   FixAll(in_file,out_file);

   TChannel::WriteCTCorrections("ct_correction.cal");

}   
#endif
