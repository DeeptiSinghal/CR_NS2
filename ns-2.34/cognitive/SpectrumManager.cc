// CRAHNs Model END
// @author:  Marco Di Felice

#include "SpectrumManager.h"

/*===========================================================================================*/
//SpectrumManager Initializer
/*===========================================================================================*/
SpectrumManager::SpectrumManager(Mac802_11 *mac, int id): sstarttimer_(this), sstoptimer_(this), htimer_(this) {
	mac_ = mac;
	nodeId_ = id;
	
	// State Initialization
	pu_on_ = false;	
	sensing_ = false;
	switching_ = false;
	
	decision_policy_ = DECISION_POLICY_ALWAYS_SWITCH;
	spectrum_policy_ = ROUND_ROBIN_SWITCH; //Other value RANDOM_SWITCH; Deepti
	
	// State Initialization
	sense_time_ = DEFAULT_SENSING_INTERVAL;
	transmit_time_ = DEFAULT_TRANSMITTING_INTERVAL;
	
	ChDecisionMAC_ = true;
}


SpectrumManager::SpectrumManager(Mac802_11 *mac, int id, double sense_time, double transmit_time, bool ChDecisionMAC): 
sstarttimer_(this), sstoptimer_(this), htimer_(this) {
	mac_ = mac;
	nodeId_ = id;
	
	pu_on_ = false;
	sensing_ = false;	
	switching_ = false;
	
	decision_policy_ = DECISION_POLICY_ALWAYS_SWITCH;
	spectrum_policy_ = ROUND_ROBIN_SWITCH; //Other value RANDOM_SWITCH; DeeptiMAC
	
	// State Initialization
	sense_time_ = sense_time;
	transmit_time_ = transmit_time;
	
	ChDecisionMAC_ = ChDecisionMAC;
	
	/*if(ChDecisionMAC_) 
		 printf("ChDecisionMAC_ set to true\n");
	else
		 printf("ChDecisionMAC_ set to false\n");*/
	
}

/* ======================================================================================*/

/* ======================================================================================*/
/* Timers */
/* ======================================================================================*/
SenseStartTimer::SenseStartTimer(SpectrumManager *s) {
	agent=s;
}

void SenseStartTimer::handle(Event *e) {
	agent->senseHandler();	
}

void SenseStartTimer::start(double time) {
  	Scheduler &s = Scheduler::instance();
	s.schedule(this, &intr, time);
}

SenseStopTimer::SenseStopTimer(SpectrumManager *s) {
	agent=s;
}

void SenseStopTimer::handle(Event *e) {
	agent->stopTransmitting();
}

void SenseStopTimer::start(double time) {
	Scheduler &s = Scheduler::instance();
	s.schedule(this, &intr, time);
}

HandoffTimer::HandoffTimer(SpectrumManager *s) {
	agent=s;
}

void HandoffTimer::handle(Event *e) {
	agent->endHandoff();
}

void HandoffTimer::start(double time) {
	Scheduler &s = Scheduler::instance();
	s.schedule(this, &intr, time);
}
/* ======================================================================================*/

/*===========================================================================================*/
 // SETUP METHODS
/*===========================================================================================*/
//setPUmodel: set the current PU model
void SpectrumManager::setPUmodel(double prob, PUmodel *p) {
	pumodel_ = p;
	prob_misdetect_ = prob;
}

//setRepository: set the current cross-layer repository
void SpectrumManager::setRepository(Repository* rep) {
	repository_=rep;
}

/*//setSpectrumData: set the current Spectrum Loader module
void  SpectrumManager::setSpectrumData(SpectrumData *sd) {
	dataMod_=sd;
}*/
/*===========================================================================================*/

//start: CR starts sensing on the current channel
void SpectrumManager::start() {
	
	pumodel_->setRepository(repository_);
	
	// Start sensing on the current channel for a sense_time_ interval
	sstarttimer_.start(sense_time_);
}

//is_channel_available: return true if CR is NOT doing sensing and is NOT doing spectrum handoff
bool SpectrumManager::is_channel_available() {
	/*printf("[DeeptiMAC] T:%f ThisNode:%d sensing: %s switching: %s\n", 
	       Scheduler::instance().clock(), nodeId_,(sensing_ ? "True" : "false"),(switching_ ? "True" : "false"));*/
	bool available= !(sensing_ || switching_);
	return available;
}

//is_channel_switching: return true if CR is NOT doing spectrum handoff
bool SpectrumManager::is_channel_switching() {
	return switching_;
}

// is_PU_interfering: return true if there is a PU which is transmitting on the same channel and within the tx range of the CR receiving a packet
bool SpectrumManager::is_PU_interfering(Packet *p) {
	// Get the tx time of a packet
	double time_tx = HDR_CMN(p)->txtime();
	// Check if a PU is active in the interval [now: now+time_tx]
	int  current_channel = repository_->get_recv_channel(nodeId_);
	bool interference = sense(nodeId_,time_tx,transmit_time_, current_channel);
	return interference;
}


//senseHandler: handler for sensing timer. 
/* Check if PU was detected during the last sensing interval, if yes switch to a new channel. 
 * In case of channel switching, use Spectrum Mobility to perform handoff, 
 * and notify the event to the upper layers.*/
void SpectrumManager::senseHandler() {
	bool need_to_switch = false;
	int  current_channel = repository_->get_recv_channel(nodeId_);
	bool ch_free = false;
	

	
	// Check if PU was detected 
	if (pu_on_) {
		// Ask the Spectrum Decision if channel switching is needed
		need_to_switch = decideSwitch();
		if (need_to_switch) { 		// CR needs to vacate the channel
			performHandoff(); // Starts handoff timer
			if(ChDecisionMAC_) { // Channel allocation is decided at MAC Layer
				// Choose next channel and store the information in the shared repository
				int next_channel = current_channel;
				while(ch_free == false) {
					next_channel = decideSpectrum(next_channel);		
					ch_free = repository_->is_channel_free(nodeId_, next_channel);
				}				
				repository_->set_recv_channel(nodeId_,next_channel);
				//mac_->load_spectrum(repository_->get_channel_bandwidth(next_channel), repository_->get_channel_per(next_channel)); // Load the spectrum data for the new channel
			}
			//else { 
				mac_->notifyUpperLayer(current_channel); // Notify the spectrum handoff to the upper layers
			//}
			#ifdef SENSING_VERBOSE_MODE
				//printf("[SENSING-DBG] Node %d starts handoff on channel %d to channel %d at time %f \n",nodeId_,current_channel,next_channel,Scheduler::instance().clock()); 
				printf("[SENSING-DBG-DS] Handoff Starts %f %d %d %d\n",Scheduler::instance().clock(),nodeId_,current_channel,next_channel); // Added by Deepti Singhal
			#endif
			
			// Sensing Time is off, since the node is performing a spectrum handoff
			sensing_ = false;
				
		} 
		else  {
			// CR does not vacate the spectrum and keeps sensing and waits for the channel to be free 	
			pu_on_= sense(nodeId_,sense_time_,transmit_time_, current_channel);
			sensing_ = true; // Set the sensing ON
			sstarttimer_.start(sense_time_); // Start the sensing interval
		 }
	}
	else {
		// Start/Stop the backoff timer
		mac_->checkBackoffTimer();
		// The CR can transmit if PU is not detected 
		if ( !pu_on_ )  {
			sensing_ = false;
			switching_ = false;
			// No channel switching, the CR can start transmitting on the current channel
			sstoptimer_.start(transmit_time_);
			#ifdef SENSING_VERBOSE_MODE
				//printf("[SENSING-DBG] Node %d starts transmitting on channel %d at time %f \n",nodeId_,current_channel,Scheduler::instance().clock()); 
				printf("[SENSING-DBG-DS] Transmitting On %f %d %d --\n",Scheduler::instance().clock(),nodeId_,current_channel); // Added by Deepti Singhal
			#endif
		}
	}
}

//stopTransmitting: the CR stops transmitting, and starts sensing for PU detection
void  SpectrumManager::stopTransmitting() {
	int current_channel = repository_->get_recv_channel(nodeId_);
	pu_on_= sense(nodeId_,sense_time_,transmit_time_, current_channel);
	sensing_ = true; // Set the sensing ON
	sstarttimer_.start(sense_time_); // Start the sensing interval
	#ifdef SENSING_VERBOSE_MODE
		printf("[SENSING-DBG-DS] Sensing Starts %f %d %d --\n",Scheduler::instance().clock(),nodeId_,current_channel); // Added by Deepti Singhal
	#endif
	mac_->checkBackoffTimer(); // Stop any current backoff attempt
}

// performHandoff: start handoff timer, during which a CR can not transmit data               
void SpectrumManager::performHandoff() {
	switching_ = true;
	htimer_.start(SWITCHING_DELAY);
}

// endHandoff: Change current state and notify the Spectrum Manager, so that the CR can perform sensing on the new channel
//endHandoff: the CR has performed spectrum handoff to a new channel. Then, it starts sensing on it to detect PU activity.
void  SpectrumManager::endHandoff() {
	switching_ = false;
	int current_channel = repository_->get_recv_channel(nodeId_);
	pu_on_= sense(nodeId_,sense_time_,transmit_time_, current_channel);
	sensing_ = true; // Set the sensing ON
	sstarttimer_.start(sense_time_); // Start the sensing interval
	#ifdef SENSING_VERBOSE_MODE
		printf("[SENSING-DBG-DS] Handoff End %f %d %d --\n",Scheduler::instance().clock(),nodeId_,current_channel); // Added by Deepti Singhal
		printf("[SENSING-DBG-DS] Sensing Starts %f %d %d --\n",Scheduler::instance().clock(),nodeId_,current_channel); // Added by Deepti Singhal
	#endif
}

// decideSwitch: decide wether to stay or leave the current channel, when a PU is detected       
bool SpectrumManager::decideSwitch() {
	double randomValue;
	bool switch_decision;
	switch(decision_policy_) {
		// Switch with probability equal to THRESHOLD_SWITCH, stay otherwise
		case DECISION_POLICY_PROBABILISTIC_SWITCH:
			randomValue = Random::uniform();
			if (randomValue < THRESHOLD_SWITCH)
				switch_decision = true;
			else	
				switch_decision = false;
			break;
		// Switch to a new channel in anycase
		case DECISION_POLICY_ALWAYS_SWITCH:
			switch_decision = true;
			break;
		default:
			switch_decision = true;	
			break;	
	}
	return switch_decision;
}

// decideSpectrum: get the next spectrum to be used, based on the allocation policy
int SpectrumManager::decideSpectrum(int current_channel) {
	int next_channel;
	switch(spectrum_policy_){ 
		// Policy RANDOM_SWITCH: next_channel -> random(1..MAX_CHANNELS)
		case RANDOM_SWITCH:
			next_channel = ((int)(Random::uniform()*MAX_CHANNELS))+1;		
			if (next_channel >= MAX_CHANNELS)
				next_channel = MAX_CHANNELS - 1;
			break;
		// Policy ROUND_ROBIN_SWITCH: next channel -> ( next_channel + 1 ) % MAX_CHANNELS
		case ROUND_ROBIN_SWITCH:
			next_channel = (current_channel+1) % MAX_CHANNELS;
			if (next_channel == 0)
				next_channel++;
			break;
	 }
	return next_channel;
}

/*// sense: return true if PU activity is detected in the time interval [current_time:current_time + sense_time]
bool SpectrumManager::sense(int id, double sense_time, double transmit_time, int channel) {
	//printf("SpectrumSensing::sense N:%d ST:%f TT:%f CH:%d \n",id,sense_time,transmit_time, channel);
	MobileNode *pnode = (MobileNode*)Node::get_node_by_address(id);
        double x = pnode->X();
        double y = pnode->Y();
	bool cr_on = false;
	if (pumodel_) {
		double randomValue = Random::uniform();
		// Ask the PUmodel if a PU is active  in the time interval [current_time:current_time + sense_time]
		//cr_on = pumodel_->is_PU_active(Scheduler::instance().clock(),sense_time, x, y, channel); // Here transmit_time added by Deepti
		
		cr_on = pumodel_->scan_PU_activity(Scheduler::instance().clock(),sense_time, id,channel) ;
		// Apply the probability of false negative detection
		if ((randomValue < prob_misdetect_) && cr_on)
			cr_on = false;
	}
	// Added by Deepti on 25 Oct 2013 -- start 
	if (cr_on)
		repository_->set_channel_busy(nodeId_,channel);
	else 
		repository_->set_channel_free(nodeId_,channel);
	// Added by Deepti on 25 Oct 2013 -- end	
		
	return cr_on;	
}*/

// sense: return true if PU activity is detected in the time interval [current_time:current_time + sense_time]
bool SpectrumManager::sense(int id, double sense_time, double transmit_time, int channel) {
	#ifdef SENSING_VERBOSE_MODE
		printf("[DS] %d SenseTimeStart %f \n", nodeId_, Scheduler::instance().clock());
	#endif
	bool cr_on = false;
	if (pumodel_) {
		cr_on = pumodel_->scan_PU_activity(Scheduler::instance().clock(),sense_time, id,channel, prob_misdetect_) ;
	}
	#ifdef SENSING_VERBOSE_MODE
		printf("[DS] %d SenseTimeEnd %f \n", nodeId_, Scheduler::instance().clock());
	#endif
	return cr_on;	
}

// /* Added by Deepti -- Start
bool SpectrumManager::update_pu_interference(int nodeid, double txpwr, double time_tx) {
	printf("Test Node Id: %d %d\n",nodeid);
	double timeNow = Scheduler::instance().clock();
	MobileNode *thisnode = (MobileNode*)Node::get_node_by_address(nodeid);
	int current_channel = repository_->get_recv_channel(nodeid);
	pumodel_->update_stat_pu_receiver(nodeid, timeNow, time_tx, thisnode->X(), thisnode->Y(), current_channel, txpwr);
}
// Added by Deepti -- End */

// CRAHNs Model END
// @author:  Marco Di Felice