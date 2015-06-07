#ifndef SWITCH_EVENT_H
#define SWITCH_EVENT_H

// This file is Added by Deepti

// EventSwitch: Sent by the MAC layer to ask for a new packet on a given channel
class EventSwitch: public Event {

	public:
		EventSwitch() {};
		int channel;
};



#endif
