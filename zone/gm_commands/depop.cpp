#include "../client.h"
#include "../corpse.h"

void command_depop(Client *c, const Seperator *sep){
	if (c->GetTarget() == 0 || !(c->GetTarget()->IsNPC() || c->GetTarget()->IsNPCCorpse()))
		c->Message(Chat::Default, "You must have a NPC target for this command. (maybe you meant #depopzone?)");
	else {
		c->Message(Chat::Default, "Depoping '%s'.", c->GetTarget()->GetName());
		c->GetTarget()->Depop();
	}
}

