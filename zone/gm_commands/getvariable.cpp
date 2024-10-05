#include "../client.h"

void command_getvariable(Client *c, const Seperator *sep){
	std::string tmp;
	if (database.GetVariable(sep->argplus[1], tmp))
		c->Message(Chat::Default, "%s = %s",  sep->argplus[1], tmp.c_str());
	else
		c->Message(Chat::Default, "GetVariable(%s) returned false", sep->argplus[1]);
}

