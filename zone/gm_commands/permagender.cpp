#include "../client.h"

void command_permagender(Client *c, const Seperator *sep){
	Client *t = c;

	if (c->GetTarget() && c->GetTarget()->IsClient())
		t = c->GetTarget()->CastToClient();

	if (sep->arg[1][0] == 0) {
		c->Message(Chat::Default, "Usage: #permagender <gendernum>");
		c->Message(Chat::Default, "Gender Numbers: 0=Male, 1=Female, 2=Neuter");
	}
	else if (!t->IsClient())
		c->Message(Chat::Default, "Target is not a client.");
	else {
		c->Message(Chat::Default, "Setting %s's gender - zone to take effect",t->GetName());
		Log(Logs::General, Logs::Normal, "Permanant gender change request from %s for %s, requested gender:%i", c->GetName(), t->GetName(), atoi(sep->arg[1]) );
		t->SetBaseGender(atoi(sep->arg[1]));
		t->Save();
		t->SendIllusionPacket(atoi(sep->arg[1]));
	}
}

