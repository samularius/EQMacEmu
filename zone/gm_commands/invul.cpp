#include "../client.h"

void command_invul(Client *c, const Seperator *sep)
{
	int arguments = sep->argnum;
	if (!arguments) {
		c->Message(Chat::Default, "Usage: #invul [On|Off]");
		return;
	}

	bool invul_flag = atobool(sep->arg[1]);
	Client* target = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		target = c->GetTarget()->CastToClient();
	}

	target->SetInvul(invul_flag);
	uint32 account = target->AccountID();
	database.SetGMInvul(account, invul_flag);
	c->Message(
		Chat::Default,
		fmt::format(
<<<<<<< HEAD
			"{} {} now {}.",
			c == target ? "You" : target->GetCleanName(),
			c == target ? "are" : "is",
			invul_flag ? "invulnerable" : "vulnerable"
=======
			"{} Command{} listed{}.",
			found_count,
			found_count != 1 ? "s" : "",
			(
				!search_criteria.empty() ?
				fmt::format(
					" matching '{}'",
					search_criteria
				) :
				""
				)
>>>>>>> upstream/main
		).c_str()
	);
}

<<<<<<< HEAD
