#ifndef TSCALER_H
#define TSCALER_H

/*
 * Author:  V. Bildstein, <vbildste@uoguelph.ca>
 * 
 * Based on the TPPG class
 * 
 * Please indicate changes with your initials.
 *
 */


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// TScaler                                                             //
//                                                                     //
// The TScaler is designed to hold all of the information about the    //
// scaler status.                                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <map>

#include "TObject.h"
#include "Globals.h"
#include "TCollection.h"
#include "TTree.h"
#include "TH1D.h"

#include "TPPG.h"

class TScalerData : public TObject {
 public:
	TScalerData();
	TScalerData(const TScalerData&);
	~TScalerData(){};
	
	void Copy(TObject& rhs) const;
	
	void SetAddress(UInt_t address) { fAddress = address; }
	void SetNetworkPacketId(UInt_t network_id) { fNetworkPacketId = network_id; }
	void SetLowTimeStamp(UInt_t low_time) { fLowTimeStamp = low_time; SetTimeStamp(); }
	void SetHighTimeStamp(UInt_t high_time) { fHighTimeStamp = high_time; SetTimeStamp();}
	void SetScaler(size_t index, UInt_t scaler) { 
		if(index < fScaler.size()) 
		 fScaler[index] = scaler;
		else
		 std::cout<<"Failed to set scaler "<<scaler<<", index "<<index<<" is out of range 0 - "<<fScaler.size()<<std::endl;
	}

	void SetTimeStamp();

	UInt_t GetAddress() const { return fAddress; }
	UInt_t GetNetworkPacketId() const { return fNetworkPacketId; }
	UInt_t GetLowTimeStamp() const { return fLowTimeStamp; }
	UInt_t GetHighTimeStamp() const { return fHighTimeStamp; }
	std::vector<UInt_t> GetScaler() const { return fScaler; }
	UInt_t GetScaler(size_t index) const { 
		if(index < fScaler.size()) 
		 return fScaler[index]; 
		else 
		 return 0; 
	}

	ULong64_t GetTimeStamp() const { return fTimeStamp; }

	void Print(Option_t *opt = "") const;
	void Clear(Option_t *opt = "");

 private:
	ULong64_t fTimeStamp;
	UInt_t fNetworkPacketId;
	UInt_t fAddress;
	std::vector<UInt_t> fScaler;
	UInt_t fLowTimeStamp;
	UInt_t fHighTimeStamp;

	ClassDef(TScalerData,2) //Contains scaler data information
};

class TScaler : public TObject {
 public:
	TScaler(bool loadIntoMap = false);
	TScaler(TTree*, bool loadIntoMap = false);
	TScaler(const TScaler&);
	virtual ~TScaler(); 

	void Copy(TObject& rhs) const;
 public: 
	std::vector<UInt_t> GetScaler(UInt_t address, ULong64_t time) const;
	UInt_t GetScaler(UInt_t address, ULong64_t time, size_t index) const;
	UInt_t GetScalerDifference(UInt_t address, ULong64_t time, size_t index) const;
	ULong64_t NumberOfScalerReadouts() const { return fEntries; };
   
	ULong64_t GetTimePeriod(UInt_t address);

   std::map<UInt_t, ULong64_t> GetTimePeriodMap() { return fTimePeriod; }
   std::map<UInt_t,std::map<ULong64_t, int> > GetNumberOfTimePeriods() { return fNumberOfTimePeriods; }

	void Clear(Option_t *opt = "");
	TH1D* Draw(UInt_t address, size_t index = 0, Option_t *opt = "");

 private:
	TTree* fTree;
	TScalerData* fScalerData;
	Long64_t fEntries;
	std::map<UInt_t, std::map<ULong64_t, std::vector<UInt_t> > > fScalerMap; //! an address-map of timestamp mapped scaler values
	std::map<UInt_t, ULong64_t> fTimePeriod; //! a map between addresses and time differences (used to calculate the time period)
	std::map<UInt_t,std::map<ULong64_t, int> > fNumberOfTimePeriods;//!
	ULong64_t fTotalTimePeriod;//!
	std::map<ULong64_t,int> fTotalNumberOfTimePeriods;//!
	TPPG* fPPG; //!
	std::map<UInt_t, TH1D*> fHist; //!

	ClassDef(TScaler,2) //Contains scaler information
};
#endif
