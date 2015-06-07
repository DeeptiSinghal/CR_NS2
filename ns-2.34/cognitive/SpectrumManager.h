// CRAHNs Model END
// @author:  Marco Di Felice

 /*
 * Implementation of the spectrum management activities performed by CRs
 * Includes:
 * 1. Spectrum sensing:      PU detection through Carrier Sensing Detection
 * 2. Spectrum decision:     Decision of the next-channel to be used in case of PU detection
 * 3. Spectrum mobility:     Handoff Managment    
 * 4. Spectrum data loader:  Loader of spectrum information
 * 5. Spectrum timers:       Timers for spectrum management (handoff/sensing/transmitting/...) 
 */

#ifndef SPECTRUM_MANAGER_H
#define SPECTRUM_MANAGER_H

//#include "SpectrumData.h"
#include "PUmodel.h"
#include "repository.h"

#include <mac/mac-802_11.h>
#include <common/packet.h>
#include <common/mobilenode.h>
#include <link/delay.h>
#include <common/connector.h>
#include <tools/random.h>

// Spectrum Manager modules
// Enable/Disable channel allocation performed at MAC Layer, by the SpectrumDecision Module
// Comment this line if channel allocation is performed by the routing agent 
//#define CHANNEL_DECISION_MAC_LAYER

// Enable/Disable Notifications MAC->NET layers, in case of PU detection on the current channel
//#define ENABLE_SPECTRUM_HANDOFF_NOTIFICATION

// Spectrum Manager constant values
#define DEFAULT_SENSING_INTERVAL 	0.1    //Default Sensing duration
#define DEFAULT_TRANSMITTING_INTERVAL 	0.5    //Default Transmitting duration
#define DEFAULT_PROBABILITY_MISDETECT   0.0    //Accuracy of the sensing scheme 


// Decision policy, when a PU is detected on the current channel
#define DECISION_POLICY_ALWAYS_SWITCH 		0
#define DECISION_POLICY_PROBABILISTIC_SWITCH 	1 //Switch with probability THRESHOLD_SWITCH

#define THRESHOLD_SWITCH 	0.8

// Spectrum Selection Policy
#define ROUND_ROBIN_SWITCH  	0
#define RANDOM_SWITCH		1


// Other classes
class Mac802_11;
class SpectrumManager;

/* ======================================================================================*/
/* Timers */

class SenseStartTimer: public Handler {
	public:
		SenseStartTimer(SpectrumManager *s); 
        	void handle(Event *e);
		void start(double time);
	private:
		Event           intr;		
		SpectrumManager *agent;
};


class SenseStopTimer: public Handler {
	public:
		SenseStopTimer(SpectrumManager *s);
		void handle(Event *e);
		 void start(double time);
	private:
		Event           intr;
		SpectrumManager *agent;
};

class HandoffTimer: public Handler {
	public:
		HandoffTimer(SpectrumManager *s);
		void  handle(Event *e);
		void  start (double time);
	private:
		Event           intr;
		SpectrumManager *agent;
};
/* ======================================================================================*/

// Spectrum Manager Implementation
class SpectrumManager  {
	friend class SenseStartTimer;
	friend class SenseStopTimer;
	friend class HandoffTimer;
	public:
		// Initialize a new Spectrum Manager
		SpectrumManager(Mac802_11 *mac, int id);
		// Initialize a new Spectrum Manager
		SpectrumManager(Mac802_11 *mac, int id, double sense_time, double transmit_time, bool ChDecisionMAC);
		
		// Start method: CR agent starts sensing activity on the current channel
		void start();

		// Setup Functions
		void setPUmodel(double prob, PUmodel *p);
		void setRepository(Repository* rep);
		//void setSpectrumData(SpectrumData *sd);

		// Return true if CR is NOT doing sensing and is NOT doing spectrum handoff
		bool is_channel_available();
		// Return true if CR is NOT doing spectrum handoff
		bool is_channel_switching();
		// Return true if a PU is active while receiving the packet, on the same channel
		bool is_PU_interfering(Packet *p);		
		
		inline double nodeId() { return nodeId_; } 
		void dataMod_(int next_channel);
		
		bool update_pu_interference(int nodeid, double txpwr, double time_tx); //Added by Deepti
		
	private:
		// Spectrum Cycle Timers and Variables
		SenseStartTimer 	sstarttimer_;	// Sensing Timer
		SenseStopTimer		sstoptimer_;	// Transmitting Timer
		HandoffTimer 		htimer_;	// Handoff Timer
		
		double 		sense_time_;		// Sensing interval
		double 		transmit_time_;		// Transmitting interval

		// State Variables
		bool 		pu_on_;			// pu_on_ is true wheter PU activity is detected in the current sensing cycle
		bool 		sensing_;		// sensing_ is true wheter a CR is performing sensing
		bool 		switching_;		// Switching state

		bool 		ChDecisionMAC_; //Added by Deepti
		
		int 		decision_policy_;	// Decision policy: stay or leave the current channel
		int		spectrum_policy_;	// Switching policy: decide the next channel to be used
		double 		prob_misdetect_;	// Probability to have false negative detection of PUs.
		
		Mac802_11 	*mac_;			// MAC References
		int 		nodeId_;		// Node address

		// Modules
		PUmodel 	*pumodel_;		// Primary User Map and Model
		Repository 	*repository_;		// Cross-layer repository 
		//SpectrumData	*dataMod_;		// Spectrum Data Loader Module
		
		// Timer Handlers
		// Handler for sensing timer
		void senseHandler();
		// Handler for transmission timer: start sensing on the current channel
		void stopTransmitting();
		// Start handoff timer
		void performHandoff();	
		// Handler for handoff management: start sensing on the new channel
		void endHandoff();
		
		// Decide wheter to stay on the current channel or switch to a new channel
		bool decideSwitch();		
		// Get the next channel to be used, based on the allocation policy	
		int decideSpectrum(int current_channel);	
		
		// Perform sensing and return true if PU activity is detected on the current channel
		bool sense(int id, double sense_time, double transmit_time, int channel);		
};

#endif

// CRAHNs Model END
// @author:  Marco Di Felice


