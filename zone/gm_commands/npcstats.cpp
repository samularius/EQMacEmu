#include "../client.h"

void command_npcstats(Client *c, const Seperator *sep){
	if (c->GetTarget() == 0)
		c->Message(Chat::Default, "ERROR: No target!");
	else if (!c->GetTarget()->IsNPC())
		c->Message(Chat::Default, "ERROR: Target is not a NPC!");
	else {
		c->GetTarget()->CastToNPC()->ShowQuickStats(c);
	}
}

