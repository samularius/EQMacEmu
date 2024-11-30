#include "../client.h"

void command_hatelist(Client *c, const Seperator *sep){
	Mob *target = c->GetTarget();
	if (target == nullptr) {
		c->Message(Chat::Default, "Error: you must have a target.");
		return;
	}

	c->Message(Chat::Default, "Display hate list for %s..", target->GetName());
	target->PrintHateListToClient(c);
}

