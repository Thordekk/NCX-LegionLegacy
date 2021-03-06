#include <map>

#include "Config.h"
#include "ScriptMgr.h"
#include "Unit.h"
#include "Player.h"
#include "Pet.h"
#include "Map.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Chat.h"
#include "Log.h"

/*
 * TODO:
 * 1. Dispel target regeneration
 * 2. Provide unlimited http://www.wowhead.com/item=17333/aqual-quintessence
 */

namespace {

class solocraft_player_instance_handler : public PlayerScript 
{
    public:
	    solocraft_player_instance_handler() : PlayerScript("solocraft_player_instance_handler") {
		    TC_LOG_INFO(LOG_FILTER_CHARACTER, "[Solocraft] solocraft_player_instance_handler Loaded");
	    }
	
        void OnLogin(Player *player, bool firstLogin) override
        {
            if (sConfigMgr->GetBoolDefault("Solocraft.Enable", true))
            {
                ChatHandler(player->GetSession()).SendSysMessage("Solocraft loaded");
            }
        }

        void OnMapChanged(Player *player) override
        {
            if (sConfigMgr->GetBoolDefault("Solocraft.Enable", true))
            {
                Map *map = player->GetMap();
                int difficulty = CalculateDifficulty(map, player);
                int numInGroup = GetNumInGroup(player);
                ApplyBuffs(player, map, difficulty, numInGroup);
            }
        }

    private:
        std::map<ObjectGuid, int> _unitDifficulty;

        // Get difficulty values from config
        const uint32 D5 = sConfigMgr->GetIntDefault("Solocraft.Dungeon", 5);
        const uint32 D10 = sConfigMgr->GetIntDefault("Solocraft.Heroic", 10);
        const uint32 D25 = sConfigMgr->GetIntDefault("Solocraft.Raid25", 25);
        const uint32 D40 = sConfigMgr->GetIntDefault("Solocraft.Raid40", 40);

        int CalculateDifficulty(Map *map, Player *player)
        {
            int difficulty = 1;
            if (map)
            {
                if (map->Is25ManRaid())
                    difficulty = D25;
                else if (map->IsHeroic())
                    difficulty = D10;
                else if (map->IsRaid())
                    difficulty = D40;
                else if (map->IsDungeon())
                    difficulty = D5;
            }
            return difficulty;
        }

        int GetNumInGroup(Player *player)
        {
            int numInGroup = 1;
            Group *group = player->GetGroup();
            if (group)
            {
                Group::MemberSlotList const& groupMembers = group->GetMemberSlots();
                numInGroup = groupMembers.size();
            }
            return numInGroup;
        }

        void ApplyBuffs(Player *player, Map *map, int difficulty, int numInGroup)
        {
            ClearBuffs(player, map);
            if (difficulty > 1)
            {
                //InstanceMap *instanceMap = map->ToInstanceMap();
                //InstanceScript *instanceScript = instanceMap->GetInstanceScript();
                ChatHandler(player->GetSession()).PSendSysMessage("Entered %s (difficulty = %d, numInGroup = %d)", map->GetMapName(), difficulty, numInGroup);
                _unitDifficulty[player->GetGUID()] = difficulty;
                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
                    player->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(difficulty * 100), true);
                player->SetFullHealth();
                if (player->getPowerType() == POWER_MANA) 
                    player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
            }
        }

	    void ClearBuffs(Player *player, Map *map)
        {
		    std::map<ObjectGuid, int>::iterator unitDifficultyIterator = _unitDifficulty.find(player->GetGUID());
		    if (unitDifficultyIterator != _unitDifficulty.end())
            {
			    int difficulty = unitDifficultyIterator->second;
			    _unitDifficulty.erase(unitDifficultyIterator);

			    ChatHandler(player->GetSession()).PSendSysMessage("Left to %s (removing difficulty = %d)", map->GetMapName(), difficulty);

                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
                    player->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(difficulty * 100), false);
		    }
	    }
    };
}


void AddSC_solocraft() 
{
	new solocraft_player_instance_handler();
}