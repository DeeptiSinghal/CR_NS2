
#include "PUmodel.h"
/* ==========================================================================================*/
 /* PU model class : Implementation of the model of PU activity for CRAHNs */
/*===========================================================================================*/

/* ==========================================================================================*/
/* TCL Hooks */
/* ==========================================================================================*/
static class PUModelclass : public TclClass {
public:
        PUModelclass() : TclClass("PUMap") {}
        TclObject* create(int argc, const char*const* argv) {
          return (new PUmodel());
        }
} class_PU_model;

/* ==========================================================================================*/
// PUmodel Initializer
/* ==========================================================================================*/
PUmodel::PUmodel() {
	number_pu_ = 0;
	// Initialize Interference Statistics
	interference_events_ = 0;
	interference_power_ = 0.0;
}


//setRepository: set the current cross-layer repository
void PUmodel::setRepository(Repository* rep) {
	repository_=rep;
}

/* ==========================================================================================*/
// Read the content of the current file and saves the information on the PU data structure
/* ==========================================================================================*/
void PUmodel::read_data(char* dir) {
	FILE* fd;	
	fd=fopen(dir,"rt");	
	if (IO_DEBUG) 
		printf("Reading PU Data from File: %s \n", dir);
	if (fd==NULL) {
		printf(" ERROR. Can't open file %s \n",dir);
		exit(0);
	}
	// The first line contains the following entry:
	// <# number_of_PU>
	fscanf(fd,"%d",&number_pu_);
	if (ferror(fd)) {
		printf(" ERROR. Can't read PU number Information from file %s \n", dir);		
		exit(0);
	}
	if (IO_DEBUG)
		printf("[READING MAP FILE] #PU users: %d \n",number_pu_); 
	if (number_pu_>	 MAX_PU_USERS) {
		printf(" ERROR. Too many PU in the file. Max allowed is %d \n", MAX_PU_USERS);
                exit(0);
	}
	
	// The second section contains the following entry:
	// <PU_id, x_loc, y_loc, x_loc_receiver, y_loc_receiver, alpha, beta, tx_range>
	for (int i=0; i< number_pu_; i++) {
		int channel;
		float x,y,x2,y2;
		float alpha, beta;
		float range;		
		fscanf(fd,"%d %f %f %f %f %e %e %f",&channel,&x,&y,&x2,&y2,&alpha,&beta,&range);
		if (ferror(fd)) {
			printf(" ERROR. Can't read PU number Information from file %s \n", dir);		
			exit(0);
		}
		if (IO_DEBUG)
			printf("[READING MAP FILE] PU Channel: %d #PU Location: %f %f #PU Receiver: %f %f ALPHA: %e BETA: %e TX RANGE: %f\n",channel,x,y,x2,y2,alpha,beta,range); 
		pu_data[i].main_channel=channel;
		pu_data[i].x_loc=x;
		pu_data[i].y_loc=y;
		pu_data[i].x_loc_receiver=x2;
		pu_data[i].y_loc_receiver=y2;
		pu_data[i].alpha=alpha;
		pu_data[i].beta=beta;
		pu_data[i].radius=range;
		pu_data[i].interference=0.0;
	}

	// The third section contains the following entry:
	// <arrival_PU_0, ..,arrival_PU_n>
	// <departure_PU_0, ..,departure_PU_n>
	
	for (int j=0; j< number_pu_; j++) {
		int arrivals=0;
		int departures=0;
		int number=0;
		if (IO_DEBUG)
			 printf("\n [READING MAP FILE] For PU User %d \n",(j+1));
		fscanf(fd,"%d",&number);
		if (ferror(fd)) {
			printf(" ERROR. Can't read PU number DATA Information from file %s \n", dir);		
			exit(0);
		}
		if (IO_DEBUG)
			printf("[READING MAP FILE] #PU data are %d \n",number); 
		if (number>MAX_PU_DATA_ENTRY) {
			printf(" ERROR. Too many PU DATA in the file. Max allowed is %d %d\n", MAX_PU_DATA_ENTRY,number);
                	exit(0);
		}
		
		pu_data[j].number_data=number;

		for (int i=0; i< (2*number); i++) {
			float time;
			fscanf(fd," %f ",&time);
			if (i%2==0) {		
				// Reading the arrival time			
				pu_data[j].arrival_time[arrivals]=time;
				arrivals++;
				if (IO_DEBUG)
					printf("[READING MAP FILE] #PU arrival: %f \n",time); 
			} else {
				// Reading the departure time
				pu_data[j].departure_time[departures]=time;
				departures++;
				if (IO_DEBUG)
					printf("[READING MAP FILE] #PU departure: %f \n",time); 
			}			
			pu_data[j].detected[arrivals]=false;
		}
	}
}

/* ==========================================================================================*/
// is_PU_active: Check if a PU is active in the time interval [timeNow, timeNow + ts]
/* ==========================================================================================*/
/*bool PUmodel::is_PU_active(double timeNow, double ts, double x, double y, int channel) {
	bool active = false;
	for (int i=0; i< number_pu_; i++) {
		if ((pu_data[i].main_channel == channel) && (distance(x,y,i) <= pu_data[i].radius))
			active = check_active(timeNow,ts,i);
		if (active) {
			//printf("[PU active] PU Node %d active on channel %d at time %f \n",i,channel,Scheduler::instance().clock());  //Added by Deepti Singhal
			return true;
		}
	}
	return false;
}
*/

/* //scan_PU_activity: Check if a PU is active in the time interval [timeNow, timeNow + ts] on channel given 
 * and also set update the channels free or busy for the node in repository */
bool PUmodel::scan_PU_activity(double timeNow, double ts, int node_id, int channel, double prob_misdetect_) {
	MobileNode *pnode = (MobileNode*)Node::get_node_by_address(node_id);
	double x = pnode->X();
	double y = pnode->Y();	
  	bool active = false;
	for (int j = 0; j < MAX_CHANNELS; j++) {
		repository_->set_channel_free(node_id, j);
	}
	for (int i=0; i< number_pu_; i++) {
		if ((distance(x,y,i) <= pu_data[i].radius)) {
			active = check_active(timeNow,ts,i);
			// Apply the probability of false negative detection
			double randomValue = Random::uniform();
			if ((randomValue < prob_misdetect_) && active)
				active = false;
		}
		if (active) {
			repository_->set_channel_busy(node_id, pu_data[i].main_channel);
		}
	}
	return !(repository_->is_channel_free(node_id, channel));
}


/* ==========================================================================================*/
// check:active: Check if a PU is transmitting in the intervale [timeNow, timeNow + ts]
/* ==========================================================================================*/
bool PUmodel::check_active(double timeNow, double ts, int pu_no) {
	double endTime=timeNow+ts;
	int number=pu_data[pu_no].number_data;
	double active=false;

	for (int i=0; i<number; i++) {
		// Check if there is an overlapping with the current PU activity	
		if( (pu_data[pu_no].arrival_time[i] >= timeNow && 
		  pu_data[pu_no].departure_time[i] >= endTime && 
		  pu_data[pu_no].arrival_time[i] <= endTime)   
		  || (pu_data[pu_no].arrival_time[i]<=timeNow && 
		  pu_data[pu_no].departure_time[i] >= endTime) 
		  || (pu_data[pu_no].arrival_time[i]<=timeNow && 
		  pu_data[pu_no].departure_time[i]>=timeNow && 
		  pu_data[pu_no].departure_time[i]<=endTime)) {
			active=true;
			pu_data[pu_no].detected[i]=true;
		}
		// If there is on overlapping, then jump out from the cycle
		if (active || pu_data[pu_no].arrival_time[i]>endTime)
        	              i=number; 
	}
	return(active);
}

/**********************************************************/
/* DISTANCE METHODS
 **********************************************************/

/* ==========================================================================================*/
//distance: Return the current distance from the PU transmitter on a given channel
/* ==========================================================================================*/
double PUmodel::distance(double x, double y, int pu_no) {
	double dx=(x-pu_data[pu_no].x_loc)*(x-pu_data[pu_no].x_loc);
	double dy=(y-pu_data[pu_no].y_loc)*(y-pu_data[pu_no].y_loc);
	double dist=sqrt(dx+dy);
	return dist;
}

/* ==========================================================================================*/
//distance_receiver: Return the current distance from the PU receiver on a given channel 
/* ==========================================================================================*/
double PUmodel::distance_receiver(double x, double y, int puNode) {
	double dx=(x-pu_data[puNode].x_loc_receiver)*(x-pu_data[puNode].x_loc_receiver);
	double dy=(y-pu_data[puNode].y_loc_receiver)*(y-pu_data[puNode].y_loc_receiver);
	double dist=sqrt(dx+dy);
	return dist;
}

/* ==========================================================================================*/
// recv method: Receive a pkt (EMPTY METHOD)
/* ==========================================================================================*/
void PUmodel::recv(Packet*, Handler*) {
}

/* ==========================================================================================*/
// update_stat_pu_receiver: Check if the tranmission of a CR may cause interference to a PU receiver
/* ==========================================================================================*/
void PUmodel::update_stat_pu_receiver(int id, double timeNow, double txtime, double x, double y, int channel, double TX_POWER) {
	double active=false;
	double d, lambda, M, power;
	FILE *fd;
	// Power injected by CR nodes 
	//double TX_POWER=0.2818;
	// Check all the PU receivers
	for (int i=0; i < number_pu_; i++) {
		d = distance_receiver(x,y,i);
		// Check if the CR is in the channel and range of a PU receiver		
		if ((pu_data[i].main_channel==channel) && (d < pu_data[i].radius) ) {
			// Check if PU is transmitting at that time
			active=check_active(timeNow, txtime, i);	
			// If the PU is transmitting, compute the amount of interference injected by the CR
			if (active) {
				// Compute the Interference Power * Interference Time received by the destination
		 		//interference_power_+=((TX_POWER * pow(1.5,4))/(pow(d,4))) * txtime;
				//interference_events_++;
				
				//Added by Deepti -- Start
				/*
				* Friis free space equation:
				*
				*       Pt * Gt * Gr * (lambda^2)
				*   P = --------------------------
				*       (4 * pi * d)^2 * L
				* 
				* Gt = Gr = L = 1
				* power = (TX_POWER * Gt * Gr * (M * M)) / L ;
				* power = (TX_POWER * 1 * 1 * (M * M)) / 1 ;
				*/
				lambda = 3e+8 / repository_->get_channel_frequency(channel);
				M = lambda / (4 * PEI * d);
				power = (TX_POWER * 1 * 1 * (M * M)) / 1 ;
				if (power > 3.652e-10) { //3.652e-10 is the RxThreashold
					interference_events_++;
					fd=fopen("interference_pu","a");	
					//fprintf(fd,"Time \t CRUser \t Event# \t RecvPower\n");
					fprintf(fd,"%6.2f \t %d \t %d \t %e\n",timeNow, id, interference_events_, power);
					fclose(fd);
				}
				//Added by Deepti -- End
			}
		}
	}
}

/* ==========================================================================================*/
// write_stat: Write the statistics about avg interference on PU receivers on a file
/* ==========================================================================================*/
void PUmodel::write_stat(int param) {
	double power=0;	
	FILE *fd;
	FILE *fd2;
	// Statistics 1: Compute the avg. Interference perceived by each PU receiver
	if (interference_events_>0)
		power=interference_power_ * 1000 / (100*27);
		//power=interference_power_/interference_events_;
	fd=fopen("interference_pu","w");	
	fprintf(fd,"%d %e\n",param,power);
	fclose(fd);

	// Statistics 2: Compute the avg probability of PU detection
	int  number_PU_events=0;
	int  number_PU_sense_detected=0;
	// Compute the number of time a PU was transmitting and a CR detected its transmission
	for (int i=0; (i< number_pu_); i++) {
		int number_activities=pu_data[i].number_data;
		number_PU_events+=number_activities;

		for (int j=0; j<number_activities; j++) 
			if (pu_data[i].detected[j])  
				number_PU_sense_detected++;					
	}
	fd2=fopen("sensing","w");	
	fprintf(fd2,"%d %f\n",param,((double)number_PU_sense_detected)/number_PU_events);
	fclose(fd2);
}

/* ==========================================================================================*/
// Command method
/* ==========================================================================================*/
int PUmodel::command(int argc, const char*const* argv) {
	if(argc == 3) {
		// Read the current PU activity file
		if(strcmp(argv[1], "set_input_map") == 0) {
  		    read_data((char*)argv[2]);
   		    return TCL_OK;
		}
		// Write the statistics about current PU activity on the interference file
		else if(strcmp(argv[1], "write_stat") == 0) {
		    write_stat(atoi(argv[2]));
   		    return TCL_OK;
		}
	} 
	return TCL_OK;
}