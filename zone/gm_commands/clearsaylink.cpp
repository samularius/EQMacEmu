#include "../client.h"

void command_clearsaylink(Client* c, const Seperator* sep) {
	database.ClearSayLink();
	c->Message(0, "Clearing Saylinks");
}

<<<<<<< HEAD
#include "gm_commands/reloadcontentflags.cpp"

//Please keep this at the bottom of command.cpp! Feel free to use this for temporary commands used in testing :)
