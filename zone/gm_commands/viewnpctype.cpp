#include "../client.h"

void command_viewnpctype(Client *c, const Seperator *sep){
	if (!sep->IsNumber(1))
		c->Message(Chat::Default, "Usage: #viewnpctype [npctype id]");
	else
	{
		uint32 npctypeid = atoi(sep->arg[1]);
		const NPCType* npct = database.LoadNPCTypesData(npctypeid);
		if (npct) {
			c->Message(Chat::Default, " NPCType Info, ");
			c->Message(Chat::Default, "  NPCTypeID: %u", npct->npc_id);
			c->Message(Chat::Default, "  Name: %s", npct->name);
			c->Message(Chat::Default, "  Level: %i", npct->level);
			c->Message(Chat::Default, "  Race: %i", npct->race);
			c->Message(Chat::Default, "  Class: %i", npct->class_);
			c->Message(Chat::Default, "  MinDmg: %i", npct->min_dmg);
			c->Message(Chat::Default, "  MaxDmg: %i", npct->max_dmg);
			c->Message(Chat::Default, "  Special Abilities: %s", npct->special_abilities);
			c->Message(Chat::Default, "  Spells: %i", npct->npc_spells_id);
			c->Message(Chat::Default, "  Loot Table: %i", npct->loottable_id);
			c->Message(Chat::Default, "  NPCFactionID: %i", npct->npc_faction_id);
		}
		else
			c->Message(Chat::Default, "NPC #%d not found", npctypeid);
	}
}

