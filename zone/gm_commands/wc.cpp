#include "../client.h"

uint32 command_wc_parse_int(char* str)
{
	if (!str)
		return 0;

	if (str[0] == '0' && str[1] == 'x')
		return strtoul(&str[2], nullptr, 16);

	if (str[0] == '0' && str[1] == 'b')
		return strtoul(&str[2], nullptr, 2);
	
	return strtoul(str, nullptr, 10);
}

void command_wc(Client *c, const Seperator *sep){
	if (sep->argnum < 2)
	{
		c->Message(Chat::White, "Usage: #wc slot material [color], or #wc slot material [r g b] [mask]");
	}
	else if (c->GetTarget() == nullptr) {
		c->Message(Chat::Red, "You must have a target to do a wear change.");
	}
	else
	{
		uint8 wearslot = atoi(sep->arg[1]);
		uint16 texture = atoi(sep->arg[2]);
		uint32 color = 0;
		
		if (sep->argnum == 3) // [color]
		{
			color = command_wc_parse_int(sep->arg[3]);
		}
		else if (sep->argnum == 5 || sep->argnum == 6) // [r] [g] [b] [mask]
		{
			uint32 red = command_wc_parse_int(sep->arg[3]) & 0xFF;
			uint32 green = command_wc_parse_int(sep->arg[4]) & 0xFF;
			uint32 blue = command_wc_parse_int(sep->arg[5]) & 0xFF;
			uint32 mask = sep->argnum == 6 ? command_wc_parse_int(sep->arg[6]) & 0xFF : 0;
			color = (mask << 24) | (red << 16) | (green << 8) | blue;
		}
		else if (sep->argnum > 2)
		{
			c->Message(Chat::White, "Usage: #wc slot material [color], or #wc slot material red green blue [mask]");
		}

		c->GetTarget()->WearChange(wearslot, texture, color);
	}
}

