// pu_activity.h

// A model for PU activity and CR channel sensing

#ifndef NS_PU_MODEL_H
#define NS_PU_MODEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random.h>
#include "object.h"

#include "repository.h"
#include <common/mobilenode.h>

// Constant value for the PU Mapping file
# define MAX_PU_USERS 		60	// Max number of PUs 
//# define MAX_CHANNEL 		11	// Max number of PU channel spectrum
# define MAX_PU_DATA_ENTRY  	700	// Max number of PU-data entry
# define IO_DEBUG		0	// Debug variable: enable verbose mode

# define PEI		3.1415926535897
//PU information
struct pu_activity {
	int main_channel;				// channel used for tx
	int number_data; 				// number of arrival/departure entries
	double x_loc;					// current location
	double y_loc;					// current location
	double arrival_time[MAX_PU_DATA_ENTRY];	// arrival time
	double departure_time[MAX_PU_DATA_ENTRY];	// departure time
	bool detected[MAX_PU_DATA_ENTRY];
	double x_loc_receiver;				// PU receiver location
	double y_loc_receiver;				// PU receiver location
	double alpha;					// PU <alpha-beta> activity description
	double beta;					// PU <alpha-beta> activity description
	double radius;					// PU transmitting range
	double interference;				// Avg. interference caused by CR on the PU receiver
};

class PUmodel : public NsObject {
	public:	
		// PUmodel creator
		PUmodel();
		// Method for receiving command from OTCL
		int command(int argc, const char*const* argv);
		// Receiving packet method (NOT used)
		void recv(Packet*, Handler*); 	
		// Return true if a PU is transmitting in the same spectrum of the CR
		bool is_PU_active(double timeNow, double ts, double x, double y, int channel);
		bool scan_PU_activity(double timeNow, double ts, int node_id, int channel,double prob_misdetect_) ;
		// Write the statistics about interference on PU receivers
		void write_stat(int param);
		// Check if the tranmission of a CR may cause interference to a PU receiver
		void update_stat_pu_receiver(int id, double timeNow, double txtime, double x, double y, int channel, double TX_POWER);
		
		void setRepository(Repository* rep);
	private:
		// Number of PUs in the current scenario
		int number_pu_;
		// Data structures with information of PUs
		pu_activity pu_data[MAX_PU_USERS];
		// Method to read data from PU file and save them in the pu_activity data structure 
		void read_data(char * dir);
		// Method to get the distance from the PU transmitter
		double distance(double x, double y, int channel);
		// Method to get the distance from the PU receiver
		double distance_receiver(double x, double y, int channel);
		// Method to check if a PU is transmitting on a given spectrum at a given time
		bool check_active(double timeNow, double ts, int channel);
		// PU-Receiver interference statistics
		int interference_events_;
		double interference_power_;
		
		Repository 	*repository_;		// Cross-layer repository 
		
};

#endif