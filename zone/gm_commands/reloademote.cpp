#include "../client.h"

void command_reloademote(Client *c, const Seperator *sep){
	zone->LoadNPCEmotes(&zone->npc_emote_list);
	c->Message(Chat::Default, "NPC emotes reloaded.");
}

