#include "../client.h"
#include "../water_map.h"

void command_bestz(Client *c, const Seperator *sep)
{
	float xcoord = c->GetX();
	float ycoord = c->GetY();
	if (sep->IsNumber(1) && sep->IsNumber(2))
	{
		xcoord = atof(sep->arg[1]);
		ycoord = atof(sep->arg[2]);
	}

	if (zone->zonemap == nullptr) {
		c->Message(Chat::Default, "Map not loaded for this zone");
	}
	else {
		glm::vec3 me;
		me.x = xcoord;
		me.y = ycoord;
		me.z = c->GetZ();
		glm::vec3 hit;
		glm::vec3 bme(me);
		bme.z -= 500;

		float best_z = zone->zonemap->FindBestZ(me, &hit);

		if (best_z != BEST_Z_INVALID)
		{
			c->Message(Chat::Default, "Z is %.3f at (%.3f, %.3f).", best_z, me.x, me.y);
		}
		else
		{
			c->Message(Chat::Default, "Found no Z.");
		}
	}

	if (zone->watermap == nullptr) {
		c->Message(Chat::Default, "Water Region Map not loaded for this zone");
	}
	else {
		WaterRegionType RegionType;
		float z;

		if (c->GetTarget()) {
			z = c->GetTarget()->GetZ();
			auto position = glm::vec3(c->GetTarget()->GetX(), c->GetTarget()->GetY(), z);
			RegionType = zone->watermap->ReturnRegionType(position);
			c->Message(Chat::Default,"InWater returns %d", zone->watermap->InWater(position));
			c->Message(Chat::Default,"InLava returns %d", zone->watermap->InLava(position));

		}
		else {
			z = c->GetZ();
			auto position = glm::vec3(c->GetX(), c->GetY(), z);
			RegionType = zone->watermap->ReturnRegionType(position);
			c->Message(Chat::Default,"InWater returns %d", zone->watermap->InWater(position));
			c->Message(Chat::Default,"InLava returns %d", zone->watermap->InLava(position));

		}

		switch (RegionType) {
		case RegionTypeNormal:	{ c->Message(Chat::Default, "There is nothing special about the region you are in!"); break; }
		case RegionTypeWater:	{ c->Message(Chat::Default, "You/your target are in Water."); break; }
		case RegionTypeLava:	{ c->Message(Chat::Default, "You/your target are in Lava."); break; }
		case RegionTypeVWater:	{ c->Message(Chat::Default, "You/your target are in VWater (Icy Water?)."); break; }
		case RegionTypePVP:	{ c->Message(Chat::Default, "You/your target are in a pvp enabled area."); break; }
		case RegionTypeSlime:	{ c->Message(Chat::Default, "You/your target are in slime."); break; }
		case RegionTypeIce:	{ c->Message(Chat::Default, "You/your target are in ice."); break; }
		default: c->Message(Chat::Default, "You/your target are in an unknown region type %d.", (int)RegionType);
		}
	}


}

