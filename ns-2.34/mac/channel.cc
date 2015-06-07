/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mac/channel.cc,v 1.47 2009/01/02 21:50:24 tom_henderson Exp $ (UCB)";
#endif

// Time interval for updating a position of a node in the X-List
// (can be adjusted by the user, depending on the nodes mobility). /* VAL NAUMOV */
#define XLIST_POSITION_UPDATE_INTERVAL 1.0 //seconds

// Change made by Deepti
// Change all nextX_ to nextX_[this->index()] and prevX_ to prevX_[this->index()]
    
    
//#include "template.h"
#include <float.h>

#include "trace.h"
#include "delay.h"
#include "object.h"
#include "packet.h"
#include "mac.h"
#include "channel.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "wireless-phy.h"
#include "mobilenode.h"
#include "ip.h"
#include "dsr/hdr_sr.h"
#include "gridkeeper.h"
#include "tworayground.h"
#include "wireless-phyExt.h"

static class ChannelClass : public TclClass {
public:
	ChannelClass() : TclClass("Channel") {}
	TclObject* create(int, const char*const*) {
		return (new Channel);
	}
} class_channel;

/*static class DuplexChannelClass : public TclClass {
public:
	DuplexChannelClass() : TclClass("Channel/Duplex") {}
	TclObject* create(int, const char*const*) {
		return (new DuplexChannel);
	}
	} class_channel_duplex; */

static class WirelessChannelClass : public TclClass {
public:
        WirelessChannelClass() : TclClass("Channel/WirelessChannel") {}
        TclObject* create(int, const char*const*) {
                return (new WirelessChannel);
        }
} class_Wireless_channel;



/* ==================================================================
   NS Initialization Functions
   =================================================================*/
static int ChannelIndex = 0;
Channel::Channel() : TclObject()
{
	index_ = ChannelIndex++;
	LIST_INIT(&ifhead_);
	bind_time("delay_", &delay_);
}

int Channel::command(int argc, const char*const* argv)
{
	
	if (argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		if (strcmp(argv[1], "trace-target") == 0) {
			trace_ = (Trace*) obj;
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "addif") == 0) {
			((Phy*) obj)->insertchnl(&ifhead_);
			((Phy*) obj)->setchnl(this);
			return TCL_OK;
		}

		// add interface for grid_keeper_
		/*else if(strncasecmp(argv[1], "grid_keeper", 5) == 0) {
			grid_keeper_ = (GridKeeper*)obj;
			return TCL_OK;
			}*/
	} else if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		if (strcmp(argv[1], "trace-target") == 0) {
			tcl.resultf("%s", trace_->name());
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", index_);
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}


void Channel::recv(Packet* p, Handler* h)
{
  
	hdr_cmn::access(p)->localif_ = index_; //Added by Deepti to support Signle Inferface Multi Channel
	sendUp(p, (Phy*)h);
}



void
Channel::sendUp(Packet* p, Phy *tifp)
{
	Scheduler &s = Scheduler::instance();
	Phy *rifp = ifhead_.lh_first;
	Node *tnode = tifp->node();
	Node *rnode = 0;
	Packet *newp;
	double propdelay = 0.0;
	struct hdr_cmn *hdr = HDR_CMN(p);

	hdr->direction() = hdr_cmn::UP;
	for( ; rifp; rifp = rifp->nextchnl()) {
		rnode = rifp->node();
		if(rnode == tnode)
			continue;
		/*
		 * Each node needs to get their own copy of this packet.
		 * Since collisions occur at the receiver, we can have
		 * two nodes canceling and freeing the *same* simulation
		 * event.
		 *
		 */
		newp = p->copy();
		propdelay = get_pdelay(tnode, rnode);
		
		/*
		 * Each node on the channel receives a copy of the
		 * packet.  The propagation delay determines exactly
		 * when the receiver's interface detects the first
		 * bit of this packet.
		 */
		s.schedule(rifp, newp, propdelay);
	}

	Packet::free(p);
}




double 
Channel::get_pdelay(Node* /*tnode*/, Node* /*rnode*/)
{
	// Dummy function
	return delay_;
}


void
Channel::dump(void)
{
	Phy *n;
	
	fprintf(stdout, "Network Interface List\n");
 	for(n = ifhead_.lh_first; n; n = n->nextchnl() )
		n->dump();
	fprintf(stdout, "--------------------------------------------------\n");
}

/* NoDupChannel------------------------------------------------------------
 * NoDupChannel is currently acting the same as Channel but with one
 * important difference: it uses reference-copying of the packet,
 * thus, a lot of time is saved (e.g. for 49 senders and 1 receiver
 * overflowing mac802.3 and running for 10 seconds sim time, factors
 * of 60 and more of savings in actual running time have been
 * observed).
 *
 * DRAWBACKS: 
 *
 *	- No propagation model supported (uses constant prop delay for 
 *        all nodes), although it should be easy to change that.
 *	- Macs should be EXTREMELY careful handling these reference 
 *        copies: essentially they all are expected not to modify them 
 *	  in any way (including scheduling!) while reference counter is
 *        positive.  802.3 seems to work with it, other macs may need
 *	  some changes.
 */

struct ChannelDelayEvent : public Event {
public:
	ChannelDelayEvent(Packet *p, Phy *txphy) : p_(p), txphy_(txphy) {};
	Packet *p_;
	Phy *txphy_;
};

class NoDupChannel : public Channel, public Handler {
public:
	void recv(Packet* p, Handler*);	
	void handle(Event*);
protected:
	int phy_counter_;
private:
	void sendUp(Packet *p, Phy *txif);

};

void NoDupChannel::recv(Packet* p, Handler* h) {
	assert(hdr_cmn::access(p)->direction() == hdr_cmn::DOWN);
	// Delay this packet
	Scheduler &s = Scheduler::instance();
	ChannelDelayEvent *de = new ChannelDelayEvent(p, (Phy *)h);
	s.schedule(this, de, delay_);
}
void NoDupChannel::handle(Event *e) {
	ChannelDelayEvent *cde = (ChannelDelayEvent *)e;
	sendUp(cde->p_, cde->txphy_);
	delete cde;
}

void NoDupChannel::sendUp(Packet *p, Phy *txif) {
	struct hdr_cmn *hdr = HDR_CMN(p);
	hdr->direction() = hdr_cmn::UP;

	for(Phy *rifp = ifhead_.lh_first; rifp; rifp = rifp->nextchnl()) {
		if(rifp == txif)
			continue;
		rifp->recv(p->refcopy(), 0);
	}
	Packet::free(p);
}

static class NoDupChannelClass : public TclClass {
public:
	NoDupChannelClass() : TclClass("Channel/NoDup") {}
	TclObject* create(int, const char*const*) {
		return (new NoDupChannel);
	}
} class_nodupchannel;



// Wireless extensions
class MobileNode;

double WirelessChannel::highestAntennaZ_ = -1; // i.e., uninitialized
double WirelessChannel::distCST_ = -1;

WirelessChannel::WirelessChannel(void) : Channel(), numNodes_(0), 
					 xListHead_(NULL), sorted_(0) {
	// Added by Deepti -- start					
	bind("bandwidth_", &bandwidth_);	
	bind("frequency_", &frequency_);	
	bind("per_", &per_);	
	// Added by Deepti -- end 
}

int WirelessChannel::command(int argc, const char*const* argv)
{
	
	if (argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		if (strcmp(argv[1], "add-node") == 0) {
			addNodeToList((MobileNode*) obj);
			return TCL_OK;
		}
		else if (strcmp(argv[1], "remove-node") == 0) {
			removeNodeFromList((MobileNode*) obj);
			return TCL_OK;
		}
	}
	return Channel::command(argc, argv);
}


void
WirelessChannel::sendUp(Packet* p, Phy *tifp)
{
	Scheduler &s = Scheduler::instance();
	Phy *rifp = ifhead_.lh_first;
	Node *tnode = tifp->node();
	Node *rnode = 0;
	Packet *newp;
	double propdelay = 0.0;
	struct hdr_cmn *hdr = HDR_CMN(p);

         /* list-based improvement */
         if(highestAntennaZ_ == -1) {
                 fprintf(stdout, "channel.cc:sendUp - Calc highestAntennaZ_ and distCST_\n");
                 calcHighestAntennaZ(tifp);
                 fprintf(stdout, "highestAntennaZ_ = %0.1f,  distCST_ = %0.1f\n", highestAntennaZ_, distCST_);
         }
	
	 hdr->direction() = hdr_cmn::UP;

	 // still keep grid-keeper around ??
	 if (GridKeeper::instance()) {
	    int i;
	    GridKeeper* gk = GridKeeper::instance();
	    int size = gk->size_; 
	    
	    MobileNode **outlist = new MobileNode *[size];
	 
       	    int out_index = gk->get_neighbors((MobileNode*)tnode,
						         outlist);
	    for (i=0; i < out_index; i ++) {
		
		  newp = p->copy();
		  rnode = outlist[i];
		  propdelay = get_pdelay(tnode, rnode);

		  rifp = (rnode->ifhead()).lh_first; 
		  for(; rifp; rifp = rifp->nextnode()){
			  if (rifp->channel() == this){
				 s.schedule(rifp, newp, propdelay); 
				 break;
			  }
		  }
 	    }
	    delete [] outlist; 
	 
	 } else { // use list-based improvement
	 
		 MobileNode *mtnode = (MobileNode *) tnode;
		 MobileNode **affectedNodes;// **aN;
		 int numAffectedNodes = -1, i;
		 
		 if(!sorted_){
			 sortLists();
		 }
		 
		 affectedNodes = getAffectedNodes(mtnode, distCST_ + /* safety */ 5, &numAffectedNodes);
		 for (i=0; i < numAffectedNodes; i++) {
			 rnode = affectedNodes[i];
			 
			 if(rnode == tnode)
				 continue;
			 
			 newp = p->copy();
			 
			 propdelay = get_pdelay(tnode, rnode);
			 
			 rifp = (rnode->ifhead()).lh_first;
			 for(; rifp; rifp = rifp->nextnode()){
				//Added by Deepti -- start
				if (rifp->getchannelnum() > 0) {
					if(rifp->getmultichannel(this->index()) == this){
						s.schedule(rifp, newp, propdelay);
					}
				}
				else {
					s.schedule(rifp, newp, propdelay);
				}
				//s.schedule(rifp, newp, propdelay);
				//Added by Deepti  -- end 
			 }
		 }
		 delete [] affectedNodes;
	 }
	 Packet::free(p);
}


void
WirelessChannel::addNodeToList(MobileNode *mn)
{
	MobileNode *tmp;

	// create list of mobilenodes for this channel
	if (xListHead_ == NULL) {
		fprintf(stderr, "INITIALIZE THE LIST xListHead\n");
		xListHead_ = mn;
		xListHead_->nextX_[this->index()] = NULL;
		xListHead_->prevX_[this->index()] = NULL;
	} else {
		for (tmp = xListHead_; tmp->nextX_[this->index()] != NULL; tmp=tmp->nextX_[this->index()]);
		tmp->nextX_[this->index()] = mn;
		mn->prevX_[this->index()] = tmp;
		mn->nextX_[this->index()] = NULL;
	}
	numNodes_++;
}

void
WirelessChannel::removeNodeFromList(MobileNode *mn) {
	
	MobileNode *tmp;
	// Find node in list
	for (tmp = xListHead_; tmp->nextX_[this->index()] != NULL; tmp=tmp->nextX_[this->index()]) {
		if (tmp == mn) {
			if (tmp == xListHead_) {
				xListHead_ = tmp->nextX_[this->index()];
				if (tmp->nextX_[this->index()] != NULL)
					tmp->nextX_[this->index()]->prevX_[this->index()] = NULL;
			} else if (tmp->nextX_[this->index()] == NULL) 
				tmp->prevX_[this->index()]->nextX_[this->index()] = NULL;
			else {
				tmp->prevX_[this->index()]->nextX_[this->index()] = tmp->nextX_[this->index()];
				tmp->nextX_[this->index()]->prevX_[this->index()] = tmp->prevX_[this->index()];
			}
			numNodes_--;
			return;
		}
	}
	fprintf(stderr, "Channel: node not found in list\n");
}

void
WirelessChannel::sortLists(void) {
	bool flag = true;
	MobileNode *m, *q;

	sorted_ = true;
	
	fprintf(stderr, "SORTING LISTS ...");
	/* Buble sort algorithm */
	// SORT x-list
	while(flag) {
		flag = false;
		m = xListHead_;
		while (m != NULL){
			if(m->nextX_[this->index()] != NULL)
				if ( m->X() > m->nextX_[this->index()]->X() ){
					flag = true;
					//delete_after m;
					q = m->nextX_[this->index()];
					m->nextX_[this->index()] = q->nextX_[this->index()];
					if (q->nextX_[this->index()] != NULL)
						q->nextX_[this->index()]->prevX_[this->index()] = m;
			    
					//insert_before m;
					q->nextX_[this->index()] = m;
					q->prevX_[this->index()] = m->prevX_[this->index()];
					m->prevX_[this->index()] = q;
					if (q->prevX_[this->index()] != NULL)
						q->prevX_[this->index()]->nextX_[this->index()] = q;

					// adjust Head of List
					if(m == xListHead_) 
						xListHead_ = m->prevX_[this->index()];
				}
			m = m -> nextX_[this->index()];
		}
	}
	
	fprintf(stderr, "DONE!\n");
}

void
WirelessChannel::updateNodesList(class MobileNode *mn, double oldX) {
	
	MobileNode* tmp;
	double X = mn->X();
	bool skipX=false;
	
	if(!sorted_) {
		sortLists();
		return;
	}
	
	/* xListHead cannot be NULL here (they are created during creation of mobilenode) */
	
	/***  DELETE ***/
	// deleting mn from x-list
	if(mn->nextX_[this->index()] != NULL) {
		if(mn->prevX_[this->index()] != NULL){
			if((mn->nextX_[this->index()]->X() >= X) && (mn->prevX_[this->index()]->X() <= X)) skipX = true; // the node doesn't change its position in the list
			else{
				mn->nextX_[this->index()]->prevX_[this->index()] = mn->prevX_[this->index()];
				mn->prevX_[this->index()]->nextX_[this->index()] = mn->nextX_[this->index()];
			}
		}
		
		else{
			if(mn->nextX_[this->index()]->X() >= X) skipX = true; // skip updating the first element
			else{
				mn->nextX_[this->index()]->prevX_[this->index()] = NULL;
				xListHead_ = mn->nextX_[this->index()];
			}
		}
	}
	
	else if(mn->prevX_[this->index()] !=NULL){
		if(mn->prevX_[this->index()]->X() <= X) skipX = true; // skip updating the last element
		else mn->prevX_[this->index()]->nextX_[this->index()] = NULL;
	}
	
	if ((mn->prevX_[this->index()] == NULL) && (mn->nextX_[this->index()] == NULL)) skipX = true; //skip updating if only one element in list
	
	/*** INSERT ***/
	//inserting mn in x-list
	if(!skipX){
		if(X > oldX){			
			for(tmp = mn; tmp->nextX_[this->index()] != NULL && tmp->nextX_[this->index()]->X() < X; tmp = tmp->nextX_[this->index()]);
			//fprintf(stdout,"Scanning the element addr %d X=%0.f, next addr %d X=%0.f\n", tmp, tmp->X(), tmp->nextX_[this->index()], tmp->nextX_[this->index()]->X());
			if(tmp->nextX_[this->index()] == NULL) { 
				//fprintf(stdout, "tmp->nextX_[this->index()] is NULL\n");
				tmp->nextX_[this->index()] = mn;
				mn->prevX_[this->index()] = tmp;
				mn->nextX_[this->index()] = NULL;
			} 
			else{ 
				//fprintf(stdout, "tmp->nextX_[this->index()] is not NULL, tmp->nextX_[this->index()]->X()=%0.f\n", tmp->nextX_[this->index()]->X());
				mn->prevX_[this->index()] = tmp->nextX_[this->index()]->prevX_[this->index()];
				mn->nextX_[this->index()] = tmp->nextX_[this->index()];
				tmp->nextX_[this->index()]->prevX_[this->index()] = mn;  	
				tmp->nextX_[this->index()] = mn;
			} 
		}
		else{
			for(tmp = mn; tmp->prevX_[this->index()] != NULL && tmp->prevX_[this->index()]->X() > X; tmp = tmp->prevX_[this->index()]);
				//fprintf(stdout,"Scanning the element addr %d X=%0.f, prev addr %d X=%0.f\n", tmp, tmp->X(), tmp->prevX_[this->index()], tmp->prevX_[this->index()]->X());
			if(tmp->prevX_[this->index()] == NULL) {
				//fprintf(stdout, "tmp->prevX_[this->index()] is NULL\n");
				tmp->prevX_[this->index()] = mn;
				mn->nextX_[this->index()] = tmp;
				mn->prevX_[this->index()] = NULL;
				xListHead_ = mn;
			} 
			else{
				//fprintf(stdout, "tmp->prevX_[this->index()] is not NULL, tmp->prevX_[this->index()]->X()=%0.f\n", tmp->prevX_[this->index()]->X());
				mn->nextX_[this->index()] = tmp->prevX_[this->index()]->nextX_[this->index()];
				mn->prevX_[this->index()] = tmp->prevX_[this->index()];
				tmp->prevX_[this->index()]->nextX_[this->index()] = mn;  	
				tmp->prevX_[this->index()] = mn;		
			}
		}
	}
}


MobileNode **
WirelessChannel::getAffectedNodes(MobileNode *mn, double radius,
				  int *numAffectedNodes)
{
	double xmin, xmax, ymin, ymax;
	int n = 0;
	MobileNode *tmp, **list, **tmpList;

	if (xListHead_ == NULL) {
		*numAffectedNodes=-1;
		fprintf(stderr, "xListHead_ is NULL when trying to send!!!\n");
		return NULL;
	}
	
	xmin = mn->X() - radius;
	xmax = mn->X() + radius;
	ymin = mn->Y() - radius;
	ymax = mn->Y() + radius;
	
	// First allocate as much as possibly needed
	tmpList = new MobileNode*[numNodes_];
	
	for(tmp = xListHead_; tmp != NULL; tmp = tmp->nextX_[this->index()]) tmpList[n++] = tmp;
	for(int i = 0; i < n; ++i)
		if(tmpList[i]->speed()!=0.0 && (Scheduler::instance().clock() -
						tmpList[i]->getUpdateTime()) > XLIST_POSITION_UPDATE_INTERVAL )
			tmpList[i]->update_position();
	n=0;
	
	for(tmp = mn; tmp != NULL && tmp->X() >= xmin; tmp=tmp->prevX_[this->index()])
		if(tmp->Y() >= ymin && tmp->Y() <= ymax){
			tmpList[n++] = tmp;
		}
	for(tmp = mn->nextX_[this->index()]; tmp != NULL && tmp->X() <= xmax; tmp=tmp->nextX_[this->index()]){
		if(tmp->Y() >= ymin && tmp->Y() <= ymax){
			tmpList[n++] = tmp;
		}
	}
	
	list = new MobileNode*[n];
	memcpy(list, tmpList, n * sizeof(MobileNode *));
	delete [] tmpList;
         
	*numAffectedNodes = n;
	return list;
}
 


/* Only to be used with mobile nodes (WirelessPhy).
 * NS-2 at its current state support only a flat (non 3D) movement of nodes,
 * so we assume antenna heights do not change for the dureation of
 * a simulation.
 * Another assumption - all nodes have the same wireless interface, so that
 * the maximum distance, corresponding to CST (at max transmission power 
 * level) stays the same for all nodes.
 */
void
WirelessChannel::calcHighestAntennaZ(Phy *tifp)
{
       double highestZ = 0;
       Phy *n;
 
       // HACK: the dynamic_cast is a workaround only!
       for(n = ifhead_.lh_first; n; n = n->nextchnl()) {
    	   if(dynamic_cast<WirelessPhyExt*>(n)) {
    		   if(((WirelessPhyExt *)n)->getAntennaZ() > highestZ)
    			   highestZ = ((WirelessPhyExt *)n)->getAntennaZ();
    	   } else if (dynamic_cast<WirelessPhy*>(n)) {
    		   if(((WirelessPhy *)n)->getAntennaZ() > highestZ)
    			   highestZ = ((WirelessPhy *)n)->getAntennaZ();
    	   } else highestZ = 0;
       }

       highestAntennaZ_ = highestZ;

       if (dynamic_cast<WirelessPhyExt*>(tifp)) {
    	   WirelessPhyExt *wifp = (WirelessPhyExt *)tifp;
    	   distCST_ = wifp->getDist(wifp->getPowerMonitorThresh(), wifp->getPt(), wifp->getAntennaRxGain(), wifp->getAntennaTxGain(),
			   highestZ , highestZ, wifp->getL(),
			   wifp->getLambda());
       } else if (dynamic_cast<WirelessPhy*>(tifp)) {
    	   WirelessPhy *wifp = (WirelessPhy *)tifp;
    	   distCST_ = wifp->getDist(wifp->getCSThresh(), wifp->getPt(), 1.0, 1.0,
    			   highestZ , highestZ, wifp->getL(),
    			   wifp->getLambda());
       } else distCST_ = DBL_MAX;
}


double
WirelessChannel::get_pdelay(Node* tnode, Node* rnode)
{
	// Scheduler	&s = Scheduler::instance();
	MobileNode* tmnode = (MobileNode*)tnode;
	MobileNode* rmnode = (MobileNode*)rnode;
	double propdelay = 0;
	
	propdelay = tmnode->propdelay(rmnode);

	assert(propdelay >= 0.0);
	if (propdelay == 0.0) {
		/* if the propdelay is 0 b/c two nodes are on top of 
		   each other, move them slightly apart -dam 7/28/98 */
		propdelay = 2 * DBL_EPSILON;
		//printf ("propdelay 0: %d->%d at %f\n",
		//	tmnode->address(), rmnode->address(), s.clock());
	}
	return propdelay;
}

	

// send():
//  The packet occupies the channel for the transmission time, txtime
//  If collision occur (>1 pkts overlap), corrupt all pkts involved
//	by setting the error bit or discard them

// int Channel::send(Packet* p, Phy *tifp)
// {
// 	// without collision, return 0
// 	Scheduler& s = Scheduler::instance();
// 	double now = s.clock();

// 	// busy = time when the channel are still busy with earlier tx
// 	double busy = max(txstop_, cwstop_);

// 	// txstop = when the channel is no longer busy from this tx
// 	txstop_ = max(busy, now + txtime);

// 	// now < busy => collision
// 	//	mark the pkt error bit, EF_COLLISION
// 	//	drop if there is a drop target, drop_
// 	if (now < busy) {
// 		// if still transmit earlier packet, pkt_, then corrupt it
// 		if (pkt_ && pkt_->time_ > now) {
// 			hdr_cmn::access(pkt_)->error() |= EF_COLLISION;
// 			if (drop_) {
// 				s.cancel(pkt_);
// 				drop(pkt_);
// 				pkt_ = 0;
// 			}
// 		}

// 		// corrupts the current packet p, and drop if drop_ exists
// 		hdr_cmn::access(p)->error() |= EF_COLLISION;
// 		if (drop_) {
// 			drop(p);
// 			return 1;
// 		}
// 	}

// 	// if p was not dropped, call recv() or hand it to trace_ if present
// 	pkt_ = p;
// 	trace_ ? trace_->recv(p, 0) : recv(p, 0);
// 	return 0;
// }


// contention():
//  The MAC calls this Channel::contention() to enter contention period
//  It determines when the contention window is over, cwstop_,
//	and schedule a callback to the MAC for the actual send()

// void Channel::contention(Packet* p, Handler* h)
// {
// 	Scheduler& s = Scheduler::instance();
// 	double now = s.clock();
// 	if (now > cwstop_) {
// 		cwstop_ = now + delay_;
// 		numtx_ = 0;
// 	}
// 	numtx_++;
// 	s.schedule(h, p, cwstop_ - now);
// }


// jam():
//  Jam the channel for a period txtime
//  Some MAC protocols use this to let other MAC detect collisions

// int Channel::jam(double txtime)
// {
// 	// without collision, return 0
// 	double now = Scheduler::instance().clock();
// 	if (txstop_ > now) {
// 		txstop_ = max(txstop_, now + txtime);
// 		return 1;
// 	}
// 	txstop_ = now + txtime;
// 	return (now < cwstop_);
// }


// int DuplexChannel::send(Packet* p, double txtime)
// {
// 	double now = Scheduler::instance().clock();
// 	txstop_ = now + txtime;
// 	trace_ ? trace_->recv(p, 0) : recv(p, 0);
// 	return 0;
// }


// void DuplexChannel::contention(Packet* p, Handler* h)
// {
// 	Scheduler::instance().schedule(h, p, delay_);
// 	numtx_ = 1;
// }






