#include "../client.h"

void command_showquake(Client *c, const Seperator *sep)
{
	if (!c)
		return;
	
	if (c->GuildID() == GUILD_NONE)
	{
		c->Message(Chat::White, "You must be part of a guild to use this command.");
		return;
	}

	if (c->GuildRank() == 0)
	{
		c->Message(Chat::White, "You must be an officer rank or higher to use this command.");
		return;
	}

	int64 curTime = Timer::GetTimeSeconds();
	int64 nextQuakeTime = nextQuakeTime - curTime;
	if (zone && zone->cached_quake_struct.quake_type == QuakeDisabled || nextQuakeTime < 0)
	{
		//Load the next quake time
		database.LoadQuakeData(zone->cached_quake_struct);
	}

	if (nextQuakeTime - curTime > 0)
	{
		std::string time_str = "The next earthquake will begin in ";
		time_str += Strings::SecondsToTime(nextQuakeTime - curTime);
		time_str += "";
		c->Message(Chat::Yellow, time_str.c_str());
	}
}

