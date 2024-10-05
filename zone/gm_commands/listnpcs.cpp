#include "../client.h"

void command_listnpcs(Client *c, const Seperator *sep){
	if (strcasecmp(sep->arg[1], "attacked") == 0)
	{
		entity_list.ListNPCs(c, "null", "null", 3);
	}
	else if (strcasecmp(sep->arg[1], "all") == 0)
		entity_list.ListNPCs(c, sep->arg[1], sep->arg[2], 0);
	else if (sep->IsNumber(1) && sep->IsNumber(2))
		entity_list.ListNPCs(c, sep->arg[1], sep->arg[2], 2);
	else if (sep->arg[1][0] != 0)
		entity_list.ListNPCs(c, sep->arg[1], sep->arg[2], 1);
	else {
		c->Message(Chat::Default, "Usage of #listnpcs: (all/attacked/npc_name/#) (#)");
		c->Message(Chat::Default, "#listnpcs [#] [#] (Each number would search by ID, ex. #listnpcs 1 30, searches 1-30)");
		c->Message(Chat::Default, "#listnpcs [name] (Would search for a npc with [name])");
	}
}

