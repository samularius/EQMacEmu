#include "../client.h"

void command_showspellslist(Client *c, const Seperator *sep){
	Mob *target = c->GetTarget();

	if (!target) {
		c->Message(Chat::Default, "Must target an NPC.");
		return;
	}

	if (!target->IsNPC()) {
		c->Message(Chat::Default, "%s is not an NPC.", target->GetName());
		return;
	}

	target->CastToNPC()->AISpellsList(c);

	return;
}

