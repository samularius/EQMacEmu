#include "../client.h"

void command_manastat(Client *c, const Seperator *sep){
	Mob *target = c->GetTarget() ? c->GetTarget() : c;

	c->Message(Chat::Default, "Mana for %s:", target->GetName());
	c->Message(Chat::Default, "  Current Mana: %d", target->GetMana());
	c->Message(Chat::Default, "  Max Mana: %d", target->GetMaxMana());
}

