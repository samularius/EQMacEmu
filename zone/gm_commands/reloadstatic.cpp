#include "../client.h"

void command_reloadstatic(Client *c, const Seperator *sep){
	c->Message(Chat::Default, "Reloading zone static data...");
	zone->ReloadStaticData();
}

