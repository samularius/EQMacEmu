#include "../client.h"

void command_delacct(Client *c, const Seperator *sep){
	if (sep->arg[1][0] == 0)
		c->Message(Chat::Default, "Format: #delacct accountname");
	else
		if (database.DeleteAccount(sep->arg[1]))
			c->Message(Chat::Default, "The account was deleted.");
		else
			c->Message(Chat::Default, "Unable to delete account.");
}

