#include "../client.h"

void command_flymode(Client *c, const Seperator *sep)
{
	int arguments = sep->argnum;
	if (!arguments || !sep->IsNumber(1)) {
		return;
	}

	Mob* target = c;

	if (c->GetTarget()) {
		target = c->GetTarget();
	}

	auto flymode_id = std::stoul(sep->arg[1]);
	uint32 account = c->AccountID();
	if (
		flymode_id < EQ::constants::GravityBehavior::Ground &&
		flymode_id > EQ::constants::GravityBehavior::Water
		) {
		c->Message(Chat::Default, "Usage:: #flymode [Flymode ID]");
		c->Message(Chat::Default, "0 = Ground, 1 = Flying, 2 = Levitating, 3 = Water");
		return;
	}

	target->SendAppearancePacket(AppearanceType::FlyMode, flymode_id);
	database.SetGMFlymode(account, static_cast<EQ::constants::GravityBehavior>(flymode_id));
	c->Message(
		Chat::Default,
		fmt::format(
			"Fly Mode for {} is now {} ({}).",
			(
				c == target ?
				"yourself" :
				fmt::format(
					"{} ({})",
					target->GetCleanName(),
					target->GetID()
				)
				),
			EQ::constants::GetFlyModeName(flymode_id),
			flymode_id
		).c_str()
	);
}

