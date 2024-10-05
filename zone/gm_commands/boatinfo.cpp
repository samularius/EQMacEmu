#include "../client.h"

void command_boatinfo(Client *c, const Seperator *sep)
{
	if(!zone->IsBoatZone())
	{
		c->Message(Chat::Default, "Zone is not a boat zone!");
	}
	else
	{
		entity_list.GetBoatInfo(c);
	}
}

