#include "../client.h"
#include "../../common/skill_caps.h"

extern SkillCaps skill_caps;

void command_maxallskills(Client *c, const Seperator *sep){
	if (c)
	{
		for (int i = 0; i <= EQ::skills::HIGHEST_SKILL; ++i)
		{
			if (i >= EQ::skills::SkillSpecializeAbjure && i <= EQ::skills::SkillSpecializeEvocation)
			{
				c->SetSkill((EQ::skills::SkillType)i, 50);
			}
			else
			{
				SkillCapsRepository::SkillCaps max_skill_level = skill_caps.GetSkillCap(c->GetClass(), (EQ::skills::SkillType)i, c->GetLevel());
				c->SetSkill((EQ::skills::SkillType)i, max_skill_level.level);
			}
		}
	}
}

