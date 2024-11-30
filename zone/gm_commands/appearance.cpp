#include "../client.h"

void command_appearance(Client* c, const Seperator* sep) {

	Mob *t = c;
	if (!c)
		return;
	if (c->GetTarget()) {
			t = c->GetTarget();
	}

	const uint32 type = Strings::ToUnsignedInt(sep->arg[1]);
	const uint32 value = Strings::ToUnsignedInt(sep->arg[2]);

	t->SendAppearancePacket(type, value);

	c->Message(
		Chat::White,
		fmt::format(
			"Appearance Sent to {} | Type: {} ({}) Value: {}",
			c->GetTargetDescription(t, TargetDescriptionType::UCSelf),
			EQ::constants::GetAppearanceTypeName(type),
			type,
			value
		).c_str()
	);
}