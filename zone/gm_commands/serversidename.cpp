#include "../client.h"

void command_serversidename(Client *c, const Seperator *sep){
	if (c->GetTarget())
		c->Message(Chat::Default, c->GetTarget()->GetName());
	else
		c->Message(Chat::Default, "Error: no target");
}

