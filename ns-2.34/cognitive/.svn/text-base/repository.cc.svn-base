// Switchable Interface Implementation START
// @author:  Marco Di Felice	

// Class Repository 
// Cross-Layer Repository to enable channel information sharing between MAC and routing protocols

#include "repository.h"

/* ==========================================================================================*/
/* TCL Hooks */
/* ==========================================================================================*/
static class Repositoryclass : public TclClass {
public:
        Repositoryclass() : TclClass("CrossLayerRepository") {}
        TclObject* create(int argc, const char*const* argv) {
          return (new Repository());
        }

	// Initialize each send
} class_repository;

/* ==========================================================================================*/
// Initializer
/* ==========================================================================================*/
Repository::Repository() {
	// Set randomly the receiver channel for each node	
	for (int i=0; i < MAX_NODES; i++) {
		int channel = get_random_channel();
		set_recv_channel(i, channel);
		printf("[Repo] Node: %d Channel: %d\n", i, channel);
		//repository_table[i].recv_channel = channel;
	}
	// Initialize each sending channel as NOT active for each node
	for (int node = 0; node < MAX_NODES; node++)  {
		for (int channel = 0; channel < MAX_CHANNELS; channel++) {
			repository_table_sender[node][channel].active=false;
			set_channel_free(node, channel);
		}
	}
	for (int channel = 0; channel < MAX_CHANNELS; channel++) {
		repository_table_spectrum_data[channel].bandwidth = 0.0;
		repository_table_spectrum_data[channel].frequency = 0.0;
		repository_table_spectrum_data[channel].per = 0.0;
	}
}

/* ==========================================================================================*/
//get_recv_channel: Return the receiving channel for a node
/* ==========================================================================================*/
int Repository::get_recv_channel(int node) {
	if (node < MAX_NODES) {
		return repository_table[node].recv_channel;
	}
	else
		return -1;
}
		 
/* ==========================================================================================*/
//set_recv_channel: Set the receiving channel for a node
/* ==========================================================================================*/
void Repository::set_recv_channel(int node, int channel) {
	if (node < MAX_NODES)
		repository_table[node].recv_channel=channel;
}

/* ==========================================================================================*/
// update_send_channel: Set the sending channel as active, at the current time
/* ==========================================================================================*/
void Repository::update_send_channel(int node, int channel, double time) {
	if (node < MAX_NODES)  {
		repository_table_sender[node][channel].active=true;
		repository_table_sender[node][channel].time=time;
	 }
}
		 
/* ==========================================================================================*/
//is_channel_used_for_sending: Check wheter a given sending channel is active for a given node
/* ==========================================================================================*/
bool Repository::is_channel_used_for_sending(int node, int channel, double timeNow) {
	if (repository_table_sender[node][channel].active) {
		if (timeNow - repository_table_sender[node][channel].time > TIMEOUT_ALIVE)
			repository_table_sender[node][channel].active=false;
	}
	return repository_table_sender[node][channel].active;
}

/* ==========================================================================================*/
//get_random_channel: Return a random channel between 1 and MAX_CHANNELS
/* ==========================================================================================*/
int Repository::get_random_channel() {
	int channel=((int)(Random::uniform()*MAX_CHANNELS))+1;		
	if (channel >= MAX_CHANNELS)
		channel = MAX_CHANNELS-1;
	return channel;
}


/* ==========================================================================================*/
//set_channel_free: Set the channel for a node as free
/* ==========================================================================================*/
void Repository::set_channel_free(int node, int channel) {
	repository_table_channel[node][channel].free = true; // 1: free
	repository_table_channel[node][channel].time = Scheduler::instance().clock();
	//printf("[Repo] Node:%d CH:%d T:%f Free:%s %s\n", node, channel, Scheduler::instance().clock(),"set_true",repository_table_channel[node][channel].free?"true":"false");
					
}
/* ==========================================================================================*/

/* ==========================================================================================*/
//set_channel_busy: Set the channel for a node as busy
/* ==========================================================================================*/
void Repository::set_channel_busy(int node, int channel) {
	repository_table_channel[node][channel].free = false; // 0: busy
	repository_table_channel[node][channel].time = Scheduler::instance().clock();
	//printf("[Repo] Node:%d CH:%d T:%f Free:%s %s\n", node, channel, Scheduler::instance().clock(),"set_false",repository_table_channel[node][channel].free?"true":"false");
	
}
/* ==========================================================================================*/


bool Repository::is_channel_free(int node, int channel) {
	//printf("Repository::is_channel_free CH: %d \n", channel);
	/*if ( (repository_table_channel[node][channel].free) && 
	  (Scheduler::instance().clock() - repository_table_channel[node][channel].time > REPOSITORY_DATA_VALID)) {
		repository_table_channel[node][channel].free = false;
	}*/
	//printf("[Repo] Node:%d CH:%d T:%f Free:%s\n", node, channel, Scheduler::instance().clock(),repository_table_channel[node][channel].free?"true":"false");
	return repository_table_channel[node][channel].free;
 }

double Repository::get_channel_bandwidth(int channel){
	if ((channel >= 0 ) && (channel < MAX_CHANNELS))
		  return (repository_table_spectrum_data[channel].bandwidth);
	else 
		 return -1;
}

double Repository::get_channel_frequency(int channel) {
	if ((channel >= 0 ) && (channel < MAX_CHANNELS))
		return (repository_table_spectrum_data[channel].frequency);
	else 
		 return -1;
}

double Repository::get_channel_per(int channel) {
	if ((channel >= 0 ) && (channel < MAX_CHANNELS))
		return repository_table_spectrum_data[channel].per;
	else 
		 return -1;	
}
 
// recv: Empty method
void Repository::recv(Packet*, Handler* = 0) {

}

// command: Empty method
int Repository::command(int argc, const char*const* argv) {
	if(argc == 3) {
		// Read the current spectrum file		
		if(strcmp(argv[1], "set_input_map") == 0) {
		    read_spectrum_file((char*)argv[2]);
   		    return TCL_OK;
		}
	} 
	return TCL_OK;
}


/* ==========================================================================================*/
// read_spectrum_file: load the information form the spectrum file into the spectrum_table_
/* ==========================================================================================*/
void Repository::read_spectrum_file(char *fileName) {
	FILE* fd;	
	fd=fopen(fileName,"rt");	
	if (IO_SPECTRUM_DEBUG) 
		printf("Reading channel Data from File: %s \n", fileName);
	if (fd==NULL) {
		printf(" ERROR. Can't open file %s \n",fileName);
		exit(0);
	}
	// For each channel in the range [0: MAX_CHANNELS] the spectrum file contains these entries:
	// bandwidth (b/s) and packet error rate 
	for (int i=0; i<MAX_CHANNELS; i++)  {
		int channel;
		float bandwidth;
		float frequency;
		float per;
		// read the next entry
		fscanf(fd,"%d %f %f %f", &channel, &bandwidth, &frequency, &per);
		if (ferror(fd)) {
			printf(" ERROR. Can't read Spectrum Information from file %s \n", fileName);		
			exit(0);
		}
		if (IO_SPECTRUM_DEBUG)
			printf("[READING SPECTRUM FILE] #CHANNEL: %d #BANDWIDTH: %f FRQ: %f PER: %f\n",channel, bandwidth, frequency, per); 
		// save the information in the repository_table_spectrum_data
		if(channel >= 0 && channel < MAX_CHANNELS ) {
			repository_table_spectrum_data[channel].bandwidth = bandwidth;
			repository_table_spectrum_data[channel].frequency = frequency;
			repository_table_spectrum_data[channel].per = per;
		}
	}
	fclose(fd);
}
// Switchable Interface Implementation END

