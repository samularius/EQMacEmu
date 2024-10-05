#include "../client.h"

void command_version(Client *c, const Seperator *sep){
	c->Message(Chat::Default, "Current version information.");
	c->Message(Chat::Default, "	%s", CURRENT_VERSION);
	c->Message(Chat::Default, "	Compiled on: %s at %s", COMPILE_DATE, COMPILE_TIME);
	c->Message(Chat::Default, "	Last modified on: %s", LAST_MODIFIED);
}

