#include "../client.h"

void command_crashtest(Client *c, const Seperator *sep)
{
	c->Message(Chat::Default, "Alright, now we get an GPF ;) ");
	char* gpf = 0;
	memcpy(gpf, "Ready to crash", 30);
}

