#include "../client.h"

void command_numauths(Client *c, const Seperator *sep){
	c->Message(Chat::Default, "NumAuths: %i", zone->CountAuth());
	c->Message(Chat::Default, "Your WID: %i", c->GetWID());
}

