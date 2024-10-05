#include "../client.h"

void command_spawn(Client *c, const Seperator *sep){
	if (sep->arg[1][0] != 0){
		Client* client = entity_list.GetClientByName(sep->arg[1]);
		if (client){
			c->Message(Chat::Default, "You cannot spawn a mob with the same name as a character!");
			return;
		}
	}

	NPC* npc = NPC::SpawnNPC(sep->argplus[1], c->GetPosition(), c);
	if (!npc) {
		c->Message(Chat::Default, "Format: #spawn name race level material hp gender class priweapon secweapon merchantid bodytype - spawns a npc those parameters.");
		c->Message(Chat::Default, "Name Format: NPCFirstname_NPCLastname - All numbers in a name are stripped and \"_\" characters become a space.");
		c->Message(Chat::Default, "Note: Using \"-\" for gender will autoselect the gender for the race. Using \"-\" for HP will use the calculated maximum HP.");
	}
}

