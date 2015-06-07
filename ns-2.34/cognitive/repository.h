// Switchable Interface Implementation START
// @author:  Marco Di Felice	

#ifndef repository_H
#define repository_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random.h>
#include "object.h"
#include <common/mobilenode.h>


// Max number of nodes in the simulation
#define MAX_NODES	200

// Defines the time a node spends on each queue
#define QUEUE_UTILIZATION_INTERVAL 	1.0 
// Defines the channel switching delay
#define SWITCHING_DELAY			0.001
// Defines the TIMEOUT_ALIVE to check wheter a node is active on a given channel or not
#define TIMEOUT_ALIVE			2

#define REPOSITORY_DATA_VALID 		2// Added by Deepti on 25 Oct 2013
// Multi-radio multi-channel specification
// Channels/Radio Definition. DO NOT MODIFY HERE!

// Interface (Radio) Classification
#define CONTROL_RADIO 		0
#define TRANSMITTER_RADIO 	1
#define RECEIVER_RADIO  	2

// Channe/Radio Information 
#define MAX_RADIO	3
#define	MAX_CHANNELS 	11
#define CONTROL_CHANNEL 0

//#define SENSING_VERBOSE_MODE

#define IO_SPECTRUM_DEBUG 1

// Channel Entry for receiver nodes
struct repository_entry_recv {
	int recv_channel;	// receiving channel
};

// Channel Entry for sender nodes
struct repository_entry_send {
	bool active;		//Flag indicating wheter the channel is used for transmitting
	double time;		//Last time the channel was used
};

struct repository_channel {
	bool free; 		//Flag indicating wheter the channel is free or not -- 1:free, 0:busy, others:data not valid
	double time;		//Last time the channel was used
};

// Added by Deepti --Start
// Spectrum Entry Information
struct repository_spectrum_data  {
	double bandwidth;	// current bandwidth
	double frequency;
	double per;		// Packet Error Rate (PER) value	
};
// Added by Deepti --Stop

// Cross-Layer Repository class
class Repository : public NsObject {
        public:
		 // Initializer
                 Repository();
                 int command(int argc, const char*const* argv);
                 void recv(Packet*, Handler*);
		  
		 // Set/Get Function for the Receiver Channel Table
		 int get_recv_channel(int node);
		 void set_recv_channel(int node, int channel);
		 
		 // Added by Deepti on 25 Oct 2013 -- start 
		 void set_channel_busy(int node, int channel);
		 void set_channel_free(int node, int channel);
		 bool is_channel_free(int node, int channel);
		 // Added by Deepti on 25 Oct 2013 -- end
		 
		 // Added by Deepti --Start
		 double get_channel_bandwidth(int channel);
		 double get_channel_frequency(int channel);
		 double get_channel_per(int channel);
		 // Added by Deepti --Stop
		 
		 // Set/Get Function for the Sender Channel Table
		 void update_send_channel(int node, int channel, double time);
		 bool is_channel_used_for_sending(int node, int channel, double timeNow);
	private:
		// Receiver Channel table: repository_table[i] contains the channels used for receiving by node i
		repository_entry_recv repository_table[MAX_NODES];
		// Sender Channel table: repository_table_sender[i][j] contains the information (active/time) for sending node i and channel j
		repository_entry_send repository_table_sender[MAX_NODES][MAX_CHANNELS];
		
		// Added by Deepti on 25 Oct 2013
		repository_channel repository_table_channel[MAX_NODES][MAX_CHANNELS]; // 0:busy & 1:free & others:data not valid
		
		repository_spectrum_data repository_table_spectrum_data[MAX_CHANNELS]; // Added by Deepti 
		
		// Returns a random channel between 1 and MAX_CHANNELS
		int get_random_channel();
		
		// read the current spectrum file, and load the information in the repository_table_spectrum_data
		void read_spectrum_file(char *fileName);		
};
// Switchable Interface Implementation END

#endif


