#include "../client.h"

void command_emotesearch(Client *c, const Seperator *sep){
	if (sep->arg[1][0] == 0)
		c->Message(Chat::Default, "Usage: #emotesearch [search string or emoteid]");
	else
	{
		const char *search_criteria = sep->argplus[1];
		int count = 0;

		if (Seperator::IsNumber(search_criteria))
		{
			uint16 emoteid = atoi(search_criteria);
			for (auto &i : zone->npc_emote_list)
			{
				NPC_Emote_Struct* nes = i;
				if (emoteid == nes->emoteid)
				{
					c->Message(Chat::Default, "EmoteID: %i Event: %i Type: %i Text: %s", nes->emoteid, nes->event_, nes->type, nes->text);
					count++;
				}
			}
			if (count == 0)
				c->Message(Chat::Default, "No emotes found.");
			else
				c->Message(Chat::Default, "%i emote(s) found", count);
		}
		else
		{
			std::string sText;
			std::string sCriteria;
			sCriteria = search_criteria;
			for (auto & c : sCriteria) c = toupper(c);

			for (auto &i : zone->npc_emote_list)
			{
				NPC_Emote_Struct* nes = i;
				sText = nes->text;
				for (auto & c : sText) c = toupper(c);
				if (sText.find(sCriteria) != std::string::npos)
				{
					c->Message(Chat::Default, "EmoteID: %i Event: %i Type: %i Text: %s", nes->emoteid, nes->event_, nes->type, nes->text);
					count++;
				}
				if (count == 50)
					break;
			}
			if (count == 50)
				c->Message(Chat::Default, "50 emotes shown...too many results.");
			else
				c->Message(Chat::Default, "%i emote(s) found", count);
		}
	}
}

