#include "../client.h"

void command_timers(Client *c, const Seperator *sep){
	if (!c->GetTarget() || !c->GetTarget()->IsClient()) {
		c->Message(Chat::Default, "Need a player target for timers.");
		return;
	}
	Client *them = c->GetTarget()->CastToClient();

	std::vector< std::pair<pTimerType, PersistentTimer *> > res;
	them->GetPTimers().ToVector(res);

	c->Message(Chat::Default, "Timers for target:");

	int r;
	int l = res.size();
	for (r = 0; r < l; r++) {
		c->Message(Chat::Default, "Timer %d: %d seconds remain.", res[r].first, res[r].second->GetRemainingTime());
	}
}

