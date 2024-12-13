#include "../client.h"

int find_race(std::string& input)
{
	if (input.find("human") != std::string::npos)
		return 1;
	else if (input.find("barbarian") != std::string::npos)
		return 2;
	else if (input.find("erudite") != std::string::npos)
		return 3;
	else if (input.find("wood elf") != std::string::npos || input.find("wood-elf") != std::string::npos)
		return 4;
	else if (input.find("high elf") != std::string::npos || input.find("high-elf") != std::string::npos)
		return 5;
	else if (input.find("dark elf") != std::string::npos || input.find("dark-elf") != std::string::npos)
		return 6;
	else if (input.find("half elf") != std::string::npos || input.find("half-elf") != std::string::npos)
		return 7;
	else if (input.find("dwarf") != std::string::npos)
		return 8;
	else if (input.find("troll") != std::string::npos)
		return 9;
	else if (input.find("ogre") != std::string::npos)
		return 10;
	else if (input.find("halfling") != std::string::npos)
		return 11;
	else if (input.find("gnome") != std::string::npos)
		return 12;
	else if (input.find("iksar") != std::string::npos)
		return 128;
	else if (input.find("vah shir") != std::string::npos || input.find("vahshir") != std::string::npos)
		return 130;
	return -1;
}

int find_racial_deity(std::string& input)
{
	if (input.find("agnostic") != std::string::npos)
		return 396;
	else if (input.find("bertox") != std::string::npos)
		return 201;
	else if (input.find("brell") != std::string::npos)
		return 202;
	else if (input.find("cazic") != std::string::npos)
		return 203;
	else if (input.find("erollisi") != std::string::npos)
		return 204;
	else if (input.find("fizzle") != std::string::npos)
		return 205;
	else if (input.find("innoruuk") != std::string::npos)
		return 206;
	else if (input.find("karana") != std::string::npos)
		return 207;
	else if (input.find("mithaniel") != std::string::npos)
		return 208;
	else if (input.find("prexus") != std::string::npos)
		return 209;
	else if (input.find("quellious") != std::string::npos)
		return 210;
	else if (input.find("rallos") != std::string::npos)
		return 211;
	else if (input.find("rodect") != std::string::npos)
		return 212;
	else if (input.find("solusek") != std::string::npos)
		return 213;
	else if (input.find("tribunal") != std::string::npos)
		return 214;
	else if (input.find("tunare") != std::string::npos)
		return 215;
	else if (input.find("veeshan") != std::string::npos)
		return 216;
	return -1;
}

int find_racial_city(std::string& input)
{
	if (input.find("paineel") != std::string::npos || input.find("erudin") != std::string::npos)
		return 0;
	else if (input.find("qeynos") != std::string::npos)
		return 1;
	else if (input.find("halas") != std::string::npos)
		return 2;
	else if (input.find("rivervale") != std::string::npos)
		return 3;
	else if (input.find("freeport") != std::string::npos)
		return 4;
	else if (input.find("neriak") != std::string::npos || input.find("grobb") != std::string::npos)
		return 5;
	else if (input.find("oggok") != std::string::npos)
		return 7;
	else if (input.find("kaladim") != std::string::npos)
		return 8;
	else if (input.find("kelethin") != std::string::npos)
		return 9;
	else if (input.find("felwithe") != std::string::npos)
		return 10;
	else if (input.find("akanon") != std::string::npos)
		return 11;
	else if (input.find("cabilis") != std::string::npos)
		return 12;
	else if (input.find("sharvahl") != std::string::npos || input.find("shar vahl") != std::string::npos)
		return 13;
	return -1;
}

void command_permarace(Client *c, const Seperator *sep){
	Client *t = c;

	std::string input = sep->msg;

	int race = find_race(input);
	if (race == -1 && sep->arg[1][0] != 0) {
		race = atoi(sep->arg[1]);
	}
	int deity = find_racial_deity(input);
	int city = find_racial_city(input);
	bool force = input.find("force") != std::string::npos;

	if (c->GetTarget() && c->GetTarget()->IsClient())
		t = c->GetTarget()->CastToClient();

	if (race < 0 || (deity < 0 && city < 0 && !force)) {
		c->Message(Chat::White, "Usage: '#permarace <race> <deity> <city> [force]'. Example: '#permarace human agnostic freeport. <race> can be racenum or string.");
		c->Message(Chat::White, "NOTE: Only valid race/deity/city combinations are possible by default. Add 'force' keyword to override race anyway (to use old permarace logic, will result in incorrect base stats).");
		c->Message(Chat::White, "NOTE: Not all models are global. If a model is not global, it will appear as a human on character select and in zones without the model.");
	}
	else if (!t->IsClient())
		c->Message(Chat::White, "Target is not a client.");
	else {
		c->Message(Chat::White, "Setting %s's race - zone to take effect", t->GetName());
		Log(Logs::General, Logs::Normal, "Permanant race change request from %s for %s, requested race:%i", c->GetName(), t->GetName(), race);
		uint32 tmp = Mob::GetDefaultGender(race, t->GetBaseGender());
		if (deity >= 0 && city >= 0 && t->PermaRace(c, race, deity, city)) {
			t->SetBaseGender(tmp);
			t->Save();
			t->SendIllusionPacket(race);
			return;
		}
		if (force) {
			uint32 tmp = Mob::GetDefaultGender(race, t->GetBaseGender());
			t->SetBaseRace(race);
			if (deity >= 0) {
				t->SetDeity(deity);
			}
			t->SetBaseGender(tmp);
			t->Save();
			t->SendIllusionPacket(race);
			c->Message(Chat::Yellow, "[Warning] Forcefully updated race and deity as requested. Base stats may need to be fixed manually.");
		}
	}
}