/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemulator.org)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*

	To add a new command 3 things must be done:

	1.	At the bottom of command.h you must add a prototype for it.
	2.	Add the function in this file.
	3.	In the command_init function you must add a call to command_add
		for your function.

	Notes: If you want an alias for your command, add an entry to the
	`command_settings` table in your database. The access level you
	set with command_add is the default setting if the command isn't
	listed in the `command_settings` db table.

*/

#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <thread>

#ifdef _WINDOWS
#define strcasecmp _stricmp
#endif

#include "../common/global_define.h"
#include "../common/eq_packet.h"
#include "../common/features.h"
#include "../common/guilds.h"
#include "../common/patches/patches.h"
#include "../common/ptimer.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "../common/eqemu_logsys.h"
#include "../common/languages.h"

#include "data_bucket.h"
#include "command.h"
#include "guild_mgr.h"
#include "map.h"
#include "mob_movement_manager.h"
#include "qglobals.h"
#include "quest_parser_collection.h"
#include <stdlib.h>
#include "string_ids.h"
#include "titles.h"
#include "water_map.h"
#include "worldserver.h"
#include "queryserv.h"
#include "zonedb.h"

extern WorldServer worldserver;
extern QueryServ* QServ;

void CatchSignal(int sig_num);


int commandcount;					// how many commands we have

// this is the pointer to the dispatch function, updated once
// init has been performed to point at the real function
int(*command_dispatch)(Client *, char const *) = command_notavail;


void command_bestz(Client *c, const Seperator *message);
void command_pf(Client *c, const Seperator *message);

std::map<std::string, CommandRecord *> commandlist;
std::map<std::string, std::string> commandaliases;

//All allocated CommandRecords get put in here so they get deleted on shutdown
LinkedList<CommandRecord *> cleanup_commandlist;

/*
* command_notavail
* This is the default dispatch function when commands aren't loaded.
*
* Parameters:
*	not used
*
*/
int command_notavail(Client *c, const char *message)
{
	c->Message(Chat::Red, "Commands not available.");
	return -1;
}

/*****************************************************************************/
/* the rest below here could be in a dynamically loaded module eventually */
/*****************************************************************************/

/*

Access Levels:

0		Normal
10	* Steward *
20	* Apprentice Guide *
50	* Novice Guide *
80	* Guide *
81	* Senior Guide *
85	* GM-Tester *
90	* EQ Support *
95	* GM-Staff *
100	* GM-Admin *
150	* GM-Lead Admin *
160	* QuestMaster *
170	* GM-Areas *
180	* GM-Coder *
200	* GM-Mgmt *
250	* GM-Impossible *

*/

/*
 * command_init
 * initializes the command list, call at startup
 *
 * Parameters:
 *	none
 *
 * When adding a new command, only hard-code 'real' commands -
 * all command aliases are added later through a database call
 *
 */

int command_init(void)
{
	commandaliases.clear();

	if
		(
		command_add("advnpcspawn", "[maketype|makegroup|addgroupentry|addgroupspawn][removegroupspawn|movespawn|editgroupbox|cleargroupbox].", AccountStatus::GMImpossible, command_advnpcspawn) ||
		command_add("aggro", "(range) [-v] - Display aggro information for all mobs 'range' distance from your target. -v is verbose faction info.", AccountStatus::GMStaff, command_aggro) ||
		command_add("aggrozone", "[aggro] [0/1: Enforce ignore distance. If 0 or not set, all will come] - Aggro every mob in the zone with X aggro. Default is 0. Not recommend if you're not invulnerable.", AccountStatus::GMImpossible, command_aggrozone) ||
		command_add("ai", "[factionid/spellslist/con/guard/roambox/stop/start] - Modify AI on NPC target.", AccountStatus::GMImpossible, command_ai) || 
		command_add("allowexport", "[off, worn, inventory, bank] - Authorize export of this character to be included in nightly, open sourced database dumps.", AccountStatus::Player, command_allowexport) ||
		command_add("altactivate", "[argument] - activates alternate advancement abilities, use altactivate help for more information.", AccountStatus::GMAreas, command_altactivate) ||
		command_add("appearance", "[type] [value] - Send an appearance packet for you or your target.", AccountStatus::GMImpossible, command_appearance) ||
		command_add("apply_shared_memory", "[shared_memory_name] - Tells every zone and world to apply a specific shared memory segment by name.", AccountStatus::GMImpossible, command_apply_shared_memory) ||
		command_add("attack", "[targetname] - Make your NPC target attack targetname.", AccountStatus::QuestMaster, command_attack) ||
		command_add("attackentity", "[entityid] - Make your NPC target attack target entity.", AccountStatus::QuestMaster, command_attackentity) ||

		command_add("ban", "[name][reason] - Ban by character name.", AccountStatus::GMAdmin, command_ban) ||
		command_add("beard", "- Change the beard of your target.", AccountStatus::GMImpossible, command_beard) ||
		command_add("beardcolor", "- Change the beard color of your target.", AccountStatus::GMImpossible, command_beardcolor) ||
		command_add("bestz", "- Ask map for a good Z coord for your x,y coords.", AccountStatus::ApprenticeGuide, command_bestz) ||
		command_add("betabuff", "[level] - Buffs user's player to provided level, giving level * 100 platinum, along with providing a (potentially non era-specific) set of reagents, spells and skills.", AccountStatus::GMImpossible, command_betabuff) ||
		command_add("bind", "- Sets your targets bind spot to their current location.", AccountStatus::SeniorGuide, command_bind) ||
		command_add("boatinfo", "- Gets infomation about the boats currently spawned in the zone.", AccountStatus::SeniorGuide, command_boatinfo) ||
		command_add("bug", "- Bug report system. Encase your bug in quotes. Type: #bug <quote>I have a bug</quote>.", AccountStatus::EQSupport, command_bug) ||

		command_add("castspell", "[spellid] [gm_override] [entityid] - Cast a spell. GM override bypasses resist and stacking checks. If entityid is specified, that NPC will cast a spell on the target mob.", AccountStatus::QuestMaster, command_castspell) ||
		command_add("chat", "[channel num] [message] - Send a channel message to all zones.", AccountStatus::EQSupport, command_chat) ||
		command_add("chattest", "[color] [loops] - Sends a test message with the specified color to yourself.", AccountStatus::GMCoder, command_chattest) ||
		command_add("checklos", "- Check for line of sight to your target.", AccountStatus::GMStaff, command_checklos) ||
		command_add("cleartimers", "- [timer] Clears one or all persistant timers on target.", AccountStatus::GMMgmt, command_cleartimers) ||
		command_add("connectworldserver", "- Make zone attempt to connect to worldserver.", AccountStatus::GMTester, command_connectworldserver) ||
#ifdef WIN32
#else
		command_add("coredump", "Dumps a core log of any existing cores to view on web page.", AccountStatus::GMCoder, command_coredump) ||
#endif
		command_add("corpse", "- Manipulate corpses, use with no arguments for help.", AccountStatus::EQSupport, command_corpse) ||
		command_add("crashtest", "- Crash the zoneserver.", AccountStatus::GMImpossible, command_crashtest) ||
		command_add("cvs", "- Summary of client versions currently online.", AccountStatus::GMCoder, command_cvs) ||

		command_add("damage", "[amount] - Damage your target.", AccountStatus::QuestMaster, command_damage) ||
		command_add("damagetotals", "Displays a list of what has damaged your NPC target.", AccountStatus::GMAdmin, command_damagetotals) ||
		command_add("date", "[yyyy] [mm] [dd] [HH] [MM] - Set EQ time.", AccountStatus::GMImpossible, command_date) ||
		command_add("dbspawn2", "[spawngroup] [respawn] [variance] - Spawn an NPC from a predefined row in the spawn2 table.", AccountStatus::QuestMaster, command_dbspawn2) ||
		command_add("delacct", "[accountname] - Delete an account.", AccountStatus::GMImpossible, command_delacct) ||
		command_add("deletegraveyard", "[zone name] - Deletes the graveyard for the specified zone.", AccountStatus::GMImpossible, command_deletegraveyard) ||
		command_add("depop", "- Depop your NPC target.", AccountStatus::GMLeadAdmin, command_depop) ||
		command_add("depopzone", "- Depop the zone.", AccountStatus::GMAreas, command_depopzone) ||
		command_add("devtools", "Manages devtools", AccountStatus::QuestTroupe, command_devtools) ||
		command_add("disablerecipe", "[recipe_id] - Disables a recipe using the recipe id.", AccountStatus::GMImpossible, command_disablerecipe) ||
		command_add("distance", "- Reports the distance between you and your target.", AccountStatus::Steward, command_distance) ||
		command_add("d1", "[type] [spell] [damage] - Send an OP_Action packet with the specified values.", AccountStatus::GMImpossible, command_d1) ||
		command_add("doanim", "[animnum] [type] - Send an EmoteAnim for you or your target.", AccountStatus::GMStaff, command_doanim) ||

		command_add("emote", "['name'/'world'/'zone'] [type] [message] - Send an emote message.", AccountStatus::GMStaff, command_emote) ||
		command_add("emotesearch", "Searches NPC Emotes.", AccountStatus::GMStaff, command_emotesearch) ||
		command_add("emoteview", "Lists all NPC Emotes.", AccountStatus::GMStaff, command_emoteview) ||
		command_add("enablerecipe", "[recipe_id] - Enables a recipe using the recipe id.", AccountStatus::GMImpossible, command_enablerecipe) ||
		command_add("equipitem", "[slotid(0-21)] - Equip the item on your cursor into the specified slot.", AccountStatus::GMLeadAdmin, command_equipitem) ||
		command_add("expansion", "[accountname][expansion] - Sets the expansion value for the specified account.", AccountStatus::GMLeadAdmin, command_expansion) ||

		command_add("face", "- Change the face of your target.", AccountStatus::GMLeadAdmin, command_face) || 
		command_add("falltest", "[+Z] sends you to your current loc plus the Z specified.", AccountStatus::GMImpossible, command_falltest) ||
		command_add("fillbuffs", "Casts 15 buffs on the target for testing.", AccountStatus::QuestTroupe, command_fillbuffs) ||
		command_add("findaliases", "[search term]- Searches for available command aliases, by alias or command", AccountStatus::ApprenticeGuide, command_findaliases) ||
		command_add("findnpctype", "[search criteria] - Search database NPC types.", AccountStatus::GMStaff, command_findnpctype) ||
		command_add("findspell", "[searchstring] - Search for a spell.", AccountStatus::Player, command_findspell) ||
		command_add("findzone", "[search criteria] - Search database zones.", AccountStatus::QuestTroupe, command_findzone) ||
		command_add("fixmob", "[race|gender|texture|helm|face|hair|haircolor|beard|beardcolor|heritage|tattoo|detail] [next|prev] - Manipulate appearance of your target.", AccountStatus::GMImpossible, command_fixmob) ||
		command_add("flag", "[status] [acctname] - Refresh your admin status, or set an account's admin status if arguments provided.", AccountStatus::GMImpossible, command_flag) ||
		command_add("flagedit", "- Edit zone flags on your target.", AccountStatus::GMMgmt, command_flagedit) ||
		command_add("fleeinfo", "- Gives info about whether a NPC will flee or not, using the command issuer as top hate.", AccountStatus::QuestTroupe, command_fleeinfo) ||
		command_add("flags", "- displays the flags of you or your target.", AccountStatus::EQSupport, command_flags) ||
		command_add("flymode", "[0/1/2] - Set your or your player target's flymode to off/on/levitate.", AccountStatus::QuestTroupe, command_flymode) ||
		command_add("fov", "- Check wether you're behind or in your target's field of view.", AccountStatus::QuestTroupe, command_fov) ||
		command_add("freeze", "- Freeze your target.", AccountStatus::QuestMaster, command_freeze) ||

		command_add("gassign", "[id] - Assign targetted NPC to predefined wandering grid id.", AccountStatus::GMImpossible, command_gassign) ||
		command_add("gender", "[0/1/2] - Change your or your target's gender to male/female/neuter.", AccountStatus::QuestMaster, command_gender) ||
		command_add("getvariable", "[varname] - Get the value of a variable from the database.", AccountStatus::GMCoder, command_getvariable) ||
		command_add("ginfo", "- get group info on target.", AccountStatus::QuestTroupe, command_ginfo) ||
		command_add("giveitem", "[itemid] [charges] [reason] - Summon an item onto your target's cursor. Charges are optional.", AccountStatus::GMLeadAdmin, command_giveitem) ||
		command_add("givemoney", "[pp] [gp] [sp] [cp] [reason]  - Gives specified amount of money to the target player.", AccountStatus::GMLeadAdmin, command_givemoney) ||
		command_add("giveplayerfaction", "[factionid] [factionvalue] - Gives the target player faction with the given faction. (Acts as a hit).", AccountStatus::GMMgmt, command_giveplayerfaction) ||
		command_add("globalview", "Lists all qglobals in cache if you were to do a quest with this target.", AccountStatus::GMStaff, command_globalview) ||
		command_add("gm", "- Turn player target's or your GM flag on or off.", AccountStatus::GMStaff, command_gm) ||
		command_add("gmdamage", "[amount] [skipaggro] - Damage your target. Skips most combat checks, including invul.", AccountStatus::QuestMaster, command_gmdamage) ||
		command_add("gmspeed", "[on/off] - Turn GM speed hack on/off for you or your player target.", AccountStatus::GMLeadAdmin, command_gmspeed) ||
		command_add("godmode", "[on/off] - Turns on/off hideme, gmspeed, invul, and flymode.", AccountStatus::GMMgmt, command_godmode) ||
		command_add("goto", "[x] [y] [z] - Teleport to the provided coordinates or to your target.", AccountStatus::ApprenticeGuide, command_goto) ||
		command_add("grid", "[add/delete] [grid_num] [wandertype] [pausetype] - Create/delete a wandering grid.", AccountStatus::GMImpossible, command_grid) ||
		command_add("gridrecord", "[start|stop|addwp] - Record a grid using start, stop and use AddWp to add a waypoint to the grid in question.", 170, command_gridrecord) ||
		command_add("guild", "- Guild manipulation commands. Use argument help for more info.", AccountStatus::EQSupport, command_guild) ||
		command_add("guildapprove", "[guildapproveid] - Approve a guild with specified ID (guild creator receives the id).", AccountStatus::EQSupport, command_guildapprove) ||
		command_add("guildcreate", "[guildname] - Creates an approval setup for guild name specified.", AccountStatus::EQSupport, command_guildcreate) ||
		command_add("guildlist", "[guildapproveid] - Lists character names who have approved the guild specified by the approve id.", AccountStatus::EQSupport, command_guildlist) ||

		command_add("hair", "- Change the hair style of your target.", AccountStatus::GMImpossible, command_hair) ||
		command_add("haircolor", "- Change the hair color of your target.", AccountStatus::GMImpossible, command_haircolor) ||
		command_add("haste", "[percentage] - Set your haste percentage.", AccountStatus::GMLeadAdmin, command_haste) ||
		command_add("hatelist", " - Display hate list for target.", AccountStatus::QuestTroupe, command_hatelist) ||
		command_add("heal", "- Completely heal your target.", AccountStatus::GMMgmt, command_heal) ||
		command_add("helm", "- Change the helm of your target.", AccountStatus::GMImpossible, command_helm) ||
		command_add("help", "[search term] - List available commands and their description, specify partial command as argument to search.", AccountStatus::Player, command_help) ||
		command_add("hideme", "[on/off] - Hide yourself from spawn lists.", AccountStatus::SeniorGuide, command_hideme) ||
		command_add("hotfix", "[hotfix_name] - Reloads shared memory into a hotfix, equiv to load_shared_memory followed by apply_shared_memory", AccountStatus::GMImpossible, command_hotfix) ||
		command_add("hp", "- Refresh your HP bar from the server.", AccountStatus::Max, command_hp) ||

		command_add("interrogateinv", "- use [help] argument for available options.", AccountStatus::GMLeadAdmin, command_interrogateinv) ||
		command_add("interrogatelegacy", "- Interrogates legacy items of your current target", AccountStatus::GMAdmin, command_interrogatelegacy) ||

		command_add("interrupt", "[message id] [color] - Interrupt your casting. Arguments are optional.", AccountStatus::EQSupport, command_interrupt) ||
		command_add("invul", "[on/off] - Turn player target's or your invulnerable flag on or off", AccountStatus::QuestTroupe, command_invul) ||
		command_add("ipban", "[IP address] - Ban IP by character name.", AccountStatus::GMMgmt, command_ipban) ||
#ifdef IPC
		command_add("ipc", "- Toggle an NPC's interactive flag.", AccountStatus::GMImpossible, command_ipc) ||
#endif
		command_add("ipexemption", "[accountname] [exemption] - Set IP exemption amount for accountname by amount. Accounts default to 1.", AccountStatus::GMAdmin, command_ipexemption) ||
		command_add("iplookup", "[charname] - Look up IP address of charname.", AccountStatus::GMStaff, command_iplookup) ||
		command_add("iteminfo", "- Get information about the item on your cursor.", AccountStatus::Guide, command_iteminfo) ||
		command_add("itemsearch", "[search criteria] - Search for an item.", AccountStatus::GMAdmin, command_itemsearch) ||

		command_add("keyring", "Displays target's keyring items.", AccountStatus::EQSupport, command_keyring) ||
		command_add("kick", "[charname] - Disconnect charname.", AccountStatus::EQSupport, command_kick) ||
		command_add("kill", "- Kill your target.", AccountStatus::GMLeadAdmin, command_kill) ||

		command_add("lastname", "[new lastname] - Set your or your player target's lastname.", AccountStatus::EQSupport, command_lastname) ||
		command_add("leaderboard", "[SFHC|SSFHC|SFHCOnly|HC] - List hardcore leaderboard.", AccountStatus::Player, command_leaderboard) ||
		command_add("level", "[level] - Set your or your target's level.", AccountStatus::QuestTroupe, command_level) ||
		command_add("listnpcs", "[name/range] - Search NPCs.", AccountStatus::EQSupport, command_listnpcs) ||
		command_add("load_shared_memory", "[shared_memory_name] - Reloads shared memory and uses the input as output", AccountStatus::GMImpossible, command_load_shared_memory) ||
		command_add("loc", "- Print out your or your target's current location and heading.", AccountStatus::Player, command_loc) ||
		command_add("logs", "Manage anything to do with logs.", AccountStatus::GMCoder, command_logs) ||
		command_add("logtest", "Performs log performance testing.", AccountStatus::GMImpossible, command_logtest) ||

		command_add("makemule", "Flags the account of the player who runs the command as a mule.", AccountStatus::Player, command_makemule) ||
		command_add("makepet", "[level] [class] [race] [texture] - Make a pet.", AccountStatus::QuestMaster, command_makepet) ||
		command_add("mana", "- Fill your or your target's mana.", AccountStatus::GMMgmt, command_mana) ||
		command_add("manaburn", "- Use AA Wizard class skill manaburn on target.", AccountStatus::GMAreas, command_manaburn) ||
		command_add("manastat", "- Report your or your target's cur/max mana.", AccountStatus::Guide, command_manastat) ||
		command_add("maxskills", "Maxes skills for you.", AccountStatus::Guide, command_maxallskills) ||
		command_add("memspell", "[slotid] [spellid] - Memorize spellid in the specified slot.", AccountStatus::GMAreas, command_memspell) ||
		command_add("merchant_close_shop", "Closes a merchant shop.", AccountStatus::GMImpossible, command_merchantcloseshop) ||
		command_add("merchant_open_shop", "Opens a merchants shop.", AccountStatus::GMImpossible, command_merchantopenshop) ||
		command_add("modifynpcstat", "- Modifys a NPC's stats.", AccountStatus::GMImpossible, command_modifynpcstat) ||
		command_add("motd", "[Message of the Day] - Set Message of the Day (leave empty to have no Message of the Day)", AccountStatus::GMAreas, command_motd) ||
		command_add("movechar", "[charname] [zonename] - Move charname to zonename.", AccountStatus::GMStaff, command_movechar) ||
		command_add("mule", "[account name] [0/1] - Toggles the mule status of the specified account ", AccountStatus::GMImpossible, command_mule) ||
		command_add("mysql", "Mysql CLI, see 'help' for options.", AccountStatus::GMImpossible, command_mysql) ||
		command_add("mysqltest", "Akkadius MySQL Bench Test.", AccountStatus::GMImpossible, command_mysqltest) ||
		command_add("mystats", "- Show details about you or your pet.", AccountStatus::Guide, command_mystats) ||

		command_add("name", "[newname] - Rename your player target.", AccountStatus::GMStaff, command_name) ||
		command_add("netstats", "- Gets the network stats for a stream.", AccountStatus::GMCoder, command_netstats) ||
		command_add("npccast", "[targetname/entityid] [spellid] - Causes NPC target to cast spellid on targetname/entityid.", AccountStatus::QuestMaster, command_npccast) ||
		command_add("npcedit", "[column] [value] - Mega NPC editing command.", AccountStatus::GMImpossible, command_npcedit) ||
		command_add("npcemote", "[message] - Make your NPC target emote a message.", AccountStatus::QuestMaster, command_npcemote) ||
		command_add("npcloot", "[show/money/add/remove] [itemid/all/money: pp gp sp cp] - Manipulate the loot an NPC is carrying.", AccountStatus::QuestMaster, command_npcloot) ||
		command_add("npcsay", "[message] - Make your NPC target say a message.", AccountStatus::QuestMaster, command_npcsay) ||
		command_add("npcshout", "[message] - Make your NPC target shout a message.", AccountStatus::QuestMaster, command_npcshout) ||
		command_add("npcspawn", "[create/add/update/remove/delete] - Manipulate spawn DB.", AccountStatus::GMImpossible, command_npcspawn) ||
		command_add("npcstats", "- Show stats about target NPC.", AccountStatus::SeniorGuide, command_npcstats) ||
		command_add("npctypecache", "[id] or all - Clears the npc type cache for either the id or all npcs.", AccountStatus::GMImpossible, command_npctypecache) ||
		command_add("npctypespawn", "[npctypeid] [factionid] - Spawn an NPC from the db.", AccountStatus::QuestMaster, command_npctypespawn) ||
		command_add("nukebuffs", "- Strip all buffs on you or your target.", AccountStatus::GMLeadAdmin, command_nukebuffs) ||
		command_add("nukeitem", "[itemid] - Remove itemid from your player target's inventory.", AccountStatus::GMLeadAdmin, command_nukeitem) ||
		command_add("numauths", "- TODO: describe this command.", AccountStatus::Max, command_numauths) ||

		command_add("oocmute", "[1/0] - Mutes OOC chat.", AccountStatus::GMStaff, command_oocmute) ||
		command_add("opcode", "- opcode management.", AccountStatus::GMCoder, command_opcode) ||
		command_add("optest", "- solar's private test command.", AccountStatus::GMCoder, command_optest) ||

#ifdef PACKET_PROFILER
		command_add("packetprofile", "- Dump packet profile for target or self.", AccountStatus::GMCoder, command_packetprofile) ||
#endif
		command_add("path", "- view and edit pathing.", AccountStatus::GMImpossible, command_path) ||
		command_add("peekinv", "[worn/inv/cursor/bank/trade/world/all] - Print out contents of your player target's inventory.", AccountStatus::EQSupport, command_peekinv) ||
		command_add("permaclass", "[classnum] - Change your or your player target's class (target is disconnected).", AccountStatus::GMMgmt, command_permaclass) ||
		command_add("permagender", "[gendernum] - Change your or your player target's gender (zone to take effect).", AccountStatus::GMMgmt, command_permagender) ||
		command_add("permarace", "[racenum] - Change your or your player target's race (zone to take effect).", AccountStatus::GMMgmt, command_permarace) ||
		command_add("petition", "Handles everything petition related. Use with no args or with 'help' for how to use.", AccountStatus::ApprenticeGuide, command_petition) ||
		command_add("peqzone", "[zonename] - Go to specified zone, if you have > 75% health.", AccountStatus::Max, command_peqzone) ||
		command_add("pf", "- Display additional mob coordinate and wandering data.", AccountStatus::GMStaff, command_pf) ||
#ifdef EQPROFILE
		command_add("profiledump", "- Dump profiling info to logs.", AccountStatus::GMImpossible, command_profiledump) ||
		command_add("profilereset", "- Reset profiling info.", AccountStatus::GMImpossible, command_profilereset) ||
#endif
		command_add("playsound", "[number] - Plays a sound in the client.  Valid range 0-3999", AccountStatus::ApprenticeGuide, command_playsound) ||
		command_add("push", "[pushback] [pushup] - Pushes the target the specified amount.", AccountStatus::GMImpossible, command_push) ||
		command_add("pvp", "[on/off] - Set your or your player target's PVP status.", AccountStatus::GMAdmin, command_pvp) ||

		command_add("qglobal", "[on/off/view] - Toggles qglobal functionality on an NPC.", AccountStatus::GMImpossible, command_qglobal) ||
		command_add("qtest", "- QueryServ testing command.", AccountStatus::GMTester, command_qtest) ||
		command_add("quaketrigger", "- [type_num (1 = Normal, 2 = PVP)] Triggers an earthquake manually", AccountStatus::GMImpossible, command_quaketrigger) ||
		command_add("questerrors", "Shows quest errors.",AccountStatus::Player, command_questerrors) ||

		command_add("race", "[racenum] - Change your or your target's race. Use racenum 0 to return to normal.", AccountStatus::QuestMaster, command_race) ||
		command_add("raidloot", "LEADER|GROUPLEADER|SELECTED|ALL - Sets your raid loot settings if you have permission to do so.", 1, command_raidloot) ||
		command_add("randtest", "Perform a sampling of random number generation", AccountStatus::GMImpossible, command_randtest) ||
		command_add("randomfeatures", "- Temporarily randomizes the Facial Features of your target.", AccountStatus::GMCoder, command_randomfeatures) ||
		command_add("refreshgroup", "- Refreshes Group.", AccountStatus::EQSupport, command_refreshgroup) ||
		command_add("reloadallrules", "Executes a reload of all rules.", AccountStatus::GMCoder, command_reloadallrules) ||
		command_add("reloadcontentflags", "Executes a reload of all expansion and content flags", AccountStatus::QuestTroupe, command_reloadcontentflags) ||
		command_add("reloademote", "Reloads NPC Emotes.", AccountStatus::GMCoder, command_reloademote) ||
		command_add("reloadlevelmods", nullptr, AccountStatus::Max, command_reloadlevelmods) ||
		command_add("reloadmerchants", "Reloads NPC merchant list.", AccountStatus::Max, command_reloadmerchants) ||
		command_add("reloadqst", " - Clear quest cache (any argument causes it to also stop all timers).", AccountStatus::QuestMaster, command_reloadqst) ||
		command_add("reloadrulesworld", "Executes a reload of all rules in world specifically.", AccountStatus::GMCoder, command_reloadworldrules) ||
		command_add("reloadstatic", "- Reload Static Zone Data.", AccountStatus::GMCoder, command_reloadstatic) ||
		command_add("reloadtitles", "- Reload player titles from the database.", AccountStatus::GMCoder, command_reloadtitles) ||
		command_add("reloadtraps", "- Repops all traps in the current zone.", AccountStatus::QuestTroupe, command_reloadtraps) ||
		command_add("reloadworld", "[0|1] - Clear quest cache and reload all rules (0 - no repop, 1 - repop).", AccountStatus::GMImpossible, command_reloadworld) ||
		command_add("reloadzps", "- Reload zone points from database", AccountStatus::GMLeadAdmin, command_reloadzps) ||
		command_add("reloadspmod", "- Reload spell modifiers from database", AccountStatus::GMLeadAdmin, command_reloadspmod) ||
		command_add("removelegacyitem", "- Remove a legacy item from your target [itemid], or specify a [charid] [itemid] to remove the flag of.", AccountStatus::GMAdmin, command_removelegacyitem) ||
		command_add("repop", "[delay] - Repop the zone with optional delay.", AccountStatus::GMLeadAdmin, command_repop) ||
		command_add("repopclose", "[distance in units] Repops only NPC's nearby for fast development purposes", AccountStatus::GMAdmin, command_repopclose) ||
		command_add("resetaa", "- Resets a Player's AA in their profile and refunds spent AA's to unspent, disconnects player.", AccountStatus::GMImpossible, command_resetaa) ||
		command_add("resetboat", "- Sets player's boat to 0 in their profile.", AccountStatus::GMStaff, command_resetboat) ||
		command_add("revoke", "[charname] [1/0] - Makes charname unable to talk on OOC.", AccountStatus::GMStaff, command_revoke) ||
		command_add("rewind", nullptr,AccountStatus::Player, command_rewind) ||
		command_add("rules", "(subcommand) - Manage server rules.", AccountStatus::GMImpossible, command_rules) ||

		command_add("save", "- Force your player or player corpse target to be saved to the database.", AccountStatus::GMLeadAdmin, command_save) ||
		command_add("scribespell", "[spellid] - Scribe specified spell in your target's spell book.", AccountStatus::GMAreas, command_scribespell) ||
		command_add("scribespells", "[max level] [min level] - Scribe all spells for you or your player target that are usable by them, up to level specified. (may freeze client for a few seconds).", AccountStatus::GMAreas, command_scribespells) ||
		command_add("sendop", "[opcode] - LE's Private test command, leave it alone.", AccountStatus::GMCoder, command_sendop) ||
		command_add("sendzonespawns", "- Refresh spawn list for all clients in zone.", AccountStatus::GMAdmin, command_sendzonespawns) ||
		command_add("serverlock", "[0|1] - Lock or Unlock the World Server (0 = Unlocked, 1 = Locked)", AccountStatus::GMLeadAdmin, command_serverlock) ||
		command_add("serversidename", "- Prints target's server side name.", AccountStatus::GMAdmin, command_serversidename) ||
		command_add("setaapts", "[value] - Set your or your player target's available AA points.", AccountStatus::GMImpossible, command_setaapts) ||
		command_add("setaaxp", "[value] - Set your or your player target's AA experience.", AccountStatus::GMImpossible, command_setaaxp) ||
		command_add("setanim", "[animnum] - Set target's appearance to animnum.", AccountStatus::GMImpossible, command_setanim) ||
		command_add("setfaction", "[faction number] - Sets targeted NPC's faction in the database.", AccountStatus::GMImpossible, command_setfaction) ||
		command_add("setgraveyard", "[zone name] - Creates a graveyard for the specified zone based on your target's LOC.", AccountStatus::GMImpossible, command_setgraveyard) ||
		command_add("setgreed", "[greed] - Sets a merchant greed value.", AccountStatus::GMAdmin, command_setgreed) ||
		command_add("setlanguage", "[language ID] [value] - Set your target's language skillnum to value.", AccountStatus::GMAreas, command_setlanguage) ||
		command_add("setlsinfo", "[email] [password] - Set login server email address and password (if supported by login server).", AccountStatus::Max, command_setlsinfo) ||
		command_add("setnpcexpansion", "[min_expansion] [max_expansion] - Restrict an NPC's spawn2 (spawn location) by min, max expansion. These are float values.", AccountStatus::GMAdmin, command_setnpcexpansion) ||
		command_add("setpass", "[accountname] [password] - Set local password for accountname.", AccountStatus::Max, command_setpass) ||
		command_add("setskill", "[skillnum] [value] - Set your target's skill skillnum to value.", AccountStatus::GMAreas, command_setskill) ||
		command_add("setskillall", "[value] - Set all of your target's skills to value.", AccountStatus::GMAreas, command_setskillall) ||
		command_add("setxp", "[value] - Set your or your player target's experience.", AccountStatus::GMAreas, command_setxp) ||
		command_add("showbonusstats", "[item|spell|all] Shows bonus stats for target from items or spells. Shows both by default.", AccountStatus::Guide, command_showbonusstats) ||
		command_add("showbuffs", "- List buffs active on your target or you if no target.", AccountStatus::Guide, command_showbuffs) ||
		command_add("showfilters", "- list client serverfilter settings.", AccountStatus::GMCoder, command_showfilters) ||
		command_add("showhelm", "on/off [all] Toggles displaying of player helms (including your own.) Specifying 'all' toggles every character currently on your account", AccountStatus::Player, command_showhelm) ||
		command_add("showlootlockouts", "- Shows your currently active loot lockouts. These do not apply to earthquake creatures.", AccountStatus::Player, command_showlootlockouts) ||
		command_add("showpetspell", "[spellid/searchstring] - search pet summoning spells.", AccountStatus::Guide, command_showpetspell) ||
		command_add("showquake", "- Shows current earthquake timer. Requires you to be a guild officer or leader.", AccountStatus::Player, command_showquake) ||
		command_add("showregen", "- Shows information about your target's regen.", AccountStatus::GMAdmin, command_showregen) ||
		command_add("showskills", "- Show the values of your skills if no target, or your target's skills.", AccountStatus::Guide, command_showskills) ||
		command_add("showspellslist", "Shows spell list of targeted NPC.", AccountStatus::GMStaff, command_showspellslist) ||
		command_add("showstats", "[quick stats]- Show details about you or your target. Quick stats shows only key stats.", AccountStatus::Guide, command_showstats) ||
		command_add("showtraderitems", "Displays the list of items a trader has up for sale.", AccountStatus::QuestTroupe, command_showtraderitems) ||
		command_add("shutdown", "- Shut this zone process down.", AccountStatus::GMImpossible, command_shutdown) ||
		command_add("size", "[size] - Change size of you or your target.", AccountStatus::GMAdmin, command_size) ||
		command_add("skills", "List skill difficulty.", AccountStatus::GMAdmin, command_skilldifficulty) ||
		command_add("spawn", "[name] [race] [level] [material] [hp] [gender] [class] [priweapon] [secweapon] [merchantid] - Spawn an NPC.", AccountStatus::GMImpossible, command_spawn) ||
		command_add("spawnfix", "- Find targeted NPC in database based on its X/Y/heading and update the database to make it spawn at your current location/heading.", AccountStatus::GMImpossible, command_spawnfix) ||
		command_add("spawnstatus", "[a|u|s|d|e|spawnid|help] - Show respawn timer status.", AccountStatus::GMStaff, command_spawnstatus) ||
		command_add("spellinfo", "[spellid] - Get detailed info about a spell.", AccountStatus::Guide, command_spellinfo) ||
		command_add("starve", "Sets hunger and thirst to 0.", AccountStatus::GMCoder, command_starve) ||
		command_add("stun", "[duration] - Stuns you or your target for duration.", AccountStatus::QuestMaster, command_stun) ||
		command_add("summon", "[charname] - Summons your player/npc/corpse target, or charname if specified.", AccountStatus::EQSupport, command_summon) ||
		command_add("summonitem", "[itemid] [charges] - Summon an item onto your cursor. Charges are optional.", AccountStatus::QuestMaster, command_summonitem) ||
		command_add("suspend", "[name][days][reason] - Suspend by character name and for specificed number of days.", AccountStatus::EQSupport, command_suspend) ||
		command_add("synctod", "- Send a time of day update to every client in zone.", AccountStatus::GMAdmin, command_synctod) ||

		command_add("testcopy", "Sends a copy of the targets loginserver/game account/characters to a backup file.", AccountStatus::GMImpossible, command_testcopy) ||
		command_add("testcommand", "Template for temporary commands as needed. Don't delete.", AccountStatus::GMImpossible, command_testcommand) ||
		command_add("testspawn", "[memloc] [value] - spawns a NPC for you only, with the specified values set in the spawn struct.", AccountStatus::GMCoder, command_testspawn) ||
		command_add("testspawnkill", "- Sends an OP_Death packet for spawn made with #testspawn.", AccountStatus::GMCoder, command_testspawnkill) ||
		command_add("texture", "[texture] [helmtexture] - Change your or your target's appearance, use 255 to show equipment.", AccountStatus::GMImpossible, command_texture) ||
		command_add("time", "[HH] [MM] - Set EQ time", AccountStatus::GMImpossible, command_time) ||
		command_add("timers", "- Display persistent timers for target.", AccountStatus::GMAdmin, command_timers) ||
		command_add("timezone", "[HH] [MM] - Set timezone. Minutes are optional.", AccountStatus::GMImpossible, command_timezone) ||
		command_add("title", "[text] [1 = create title table row] - Set your or your player target's title.", AccountStatus::GMStaff, command_title) ||
		command_add("titlesuffix", "[text] [1 = create title table row] - Set your or your player target's title suffix.", AccountStatus::Max, command_titlesuffix) ||
		command_add("trapinfo", "- Gets infomation about the traps currently spawned in the zone.", AccountStatus::SeniorGuide, command_trapinfo) ||

		command_add("undeletechar", "- Undelete a character that was previously deleted.", AccountStatus::Max, command_undeletechar) ||
		command_add("underworld", "[z] - Reports NPCs that are below the given Z or if not given, below the lowest spawn2/grid coord. If red, the NPC is below the underworld coord.", AccountStatus::QuestTroupe, command_underworld) ||
		command_add("unfreeze", "- Unfreeze your target.", AccountStatus::QuestMaster, command_unfreeze) ||
		command_add("unmemspell", "[spellid] - Unmem specified spell from your target's spell bar.", AccountStatus::GMAreas, command_unmemspell) ||
		command_add("unmemspells", "- Clear out your or your player target's spell gems.", AccountStatus::GMAreas, command_unmemspells) ||
		command_add("unscribespell", "[spellid] - Unscribe specified spell from your target's spell book.", AccountStatus::GMAreas, command_unscribespell) ||
		command_add("unscribespells", "- Clear out your or your player target's spell book.", AccountStatus::GMAreas, command_unscribespells) ||
		command_add("update", "Handles all server updates/reboots. Use with no args or 'help' for how to use.", AccountStatus::GMMgmt, command_update) ||
		command_add("uptime", "[zone server id] - Get uptime of worldserver, or zone server if argument provided.", AccountStatus::GMStaff, command_uptime) ||

		command_add("version", "- Display current version of EQEmu server.", AccountStatus::GMCoder, command_version) ||
		command_add("viewnpctype", "[npctype id] - Show info about an npctype.", AccountStatus::GMStaff, command_viewnpctype) ||
		command_add("viewplayerfaction", "[factionid] - Shows current personal and modified faction with the given ID.", AccountStatus::GMAdmin, command_viewplayerfaction) ||
		command_add("viewzoneloot", "[item id] - Allows you to search a zone's loot for a specific item ID. (0 shows all loot in the zone)", AccountStatus::QuestTroupe, command_viewzoneloot) ||
	
		command_add("wc", "[wear slot] [material] - Sends an OP_WearChange for your target.", AccountStatus::GMImpossible, command_wc) ||
		command_add("weather", "[0/1/2] (Off/Rain/Snow) [0/1] Serverwide [minutes] Duration - Change the weather.", AccountStatus::QuestMaster, command_weather) ||
		command_add("worldshutdown", "- Shut down world and all zones.", AccountStatus::GMImpossible, command_worldshutdown) ||
		command_add("wp", "[add/delete] [grid_num] [pause] [wp_num] [-h] - Add/delete a waypoint to/from a wandering grid.", AccountStatus::GMImpossible, command_wp) ||
		command_add("wpadd", "[pause] [-h] - Add your current location as a waypoint to your NPC target's AI path.", AccountStatus::GMImpossible, command_wpadd) ||
		command_add("wpinfo", "- Show waypoint info about your NPC target.", AccountStatus::GMImpossible, command_wpinfo) ||

		command_add("xpinfo", "- Show XP info about your current target.", AccountStatus::GMStaff, command_xpinfo) ||

		command_add("zclip", "[min] [max] - modifies and resends zhdr packet.", AccountStatus::GMImpossible, command_zclip) ||
		command_add("zcolor", "[red] [green] [blue] - Change sky color.", AccountStatus::GMImpossible, command_zcolor) ||
		command_add("zheader", "[zonename] - Load zheader for zonename from the database.", AccountStatus::GMImpossible, command_zheader) ||
		command_add("zone", "[Zone ID|Zone Short Name] [X] [Y] [Z] - Teleport to specified Zone by ID or Short Name (coordinates are optional).", AccountStatus::QuestTroupe, command_zone) ||
		command_add("zonebootup", "(shortname) (ZoneServerID) - Make a zone server boot a specific zone. If no arguments are given, it will find and boot any crashed zones.", AccountStatus::GMImpossible, command_zonebootup) ||
			command_add("zoneguild", "[Zone ID|Zone Short Name] [Guild ID] [X] [Y] [Z] - Teleport to specified Zone by ID or Short Name (coordinates are optional).", AccountStatus::QuestTroupe, command_zoneguild) ||
		command_add("zonelock", "[list/lock/unlock] - Set/query lock flag for zoneservers.", AccountStatus::GMAreas, command_zonelock) ||
		command_add("zoneshutdown", "[shortname] - Shut down a zone server.", AccountStatus::GMImpossible, command_zoneshutdown) ||
		command_add("zonespawn", "- Not implemented.", AccountStatus::Max, command_zonespawn) ||
		command_add("zonestatus", "- Show connected zoneservers, synonymous with /servers.", AccountStatus::GMStaff, command_zonestatus) ||
		command_add("zopp", "Troubleshooting command - Sends a fake item packet to you. No server reference is created.", AccountStatus::GMCoder, command_zopp) ||
		command_add("zsafecoords", "[x] [y] [z] - Set safe coords.", AccountStatus::GMImpossible, command_zsafecoords) ||
		command_add("zsave", " - Saves zheader to the database.", AccountStatus::GMImpossible, command_zsave) ||
		command_add("zsky", "[skytype] - Change zone sky type.", AccountStatus::GMImpossible, command_zsky) ||
		command_add("zstats", "- Show info about zone header.", AccountStatus::QuestTroupe, command_zstats) ||
		command_add("zunderworld", "[zcoord] - Sets the underworld using zcoord.", AccountStatus::GMImpossible, command_zunderworld) ||
		command_add("zuwcoords", "[z coord] - Set underworld coord.", AccountStatus::GMImpossible, command_zuwcoords)
		)
	{
		command_deinit();
		return -1;
	}

	std::map<std::string, std::pair<uint8, std::vector<std::string>>> command_settings;
	database.GetCommandSettings(command_settings);

	std::vector<std::pair<std::string, uint8>> injected_command_settings;
	std::vector<std::string> orphaned_command_settings;

	for (auto cs_iter : command_settings) {

		auto cl_iter = commandlist.find(cs_iter.first);
		if (cl_iter == commandlist.end()) {

			orphaned_command_settings.push_back(cs_iter.first);
			LogInfo(
				"Command [{}] no longer exists... Deleting orphaned entry from `command_settings` table...",
				cs_iter.first.c_str()
			);
		}
	}

	if (orphaned_command_settings.size()) {
		if (!database.UpdateOrphanedCommandSettings(orphaned_command_settings)) {
			LogInfo("Failed to process 'Orphaned Commands' update operation.");
		}
	}

	auto working_cl = commandlist;
	for (auto working_cl_iter : working_cl) {

		auto cs_iter = command_settings.find(working_cl_iter.first);
		if (cs_iter == command_settings.end()) {

			injected_command_settings.push_back(std::pair<std::string, uint8>(working_cl_iter.first, working_cl_iter.second->access));
			LogInfo(
				"New Command [{}] found... Adding to `command_settings` table with access [{}]...",
				working_cl_iter.first.c_str(),
				working_cl_iter.second->access
			);

			if (working_cl_iter.second->access == 0) {
				LogCommands(
					"command_init(): Warning: Command [{}] defaulting to access level 0!",
					working_cl_iter.first.c_str()
				);
			}

			continue;
		}

		working_cl_iter.second->access = cs_iter->second.first;
		LogCommands(
			"command_init(): - Command [{}] set to access level [{}]",
			working_cl_iter.first.c_str(),
			cs_iter->second.first
		);

		if (cs_iter->second.second.empty()) {
			continue;
		}

		for (auto alias_iter : cs_iter->second.second) {
			if (alias_iter.empty()) {
				continue;
			}

			if (commandlist.find(alias_iter) != commandlist.end()) {
				LogCommands(
					"command_init(): Warning: Alias [{}] already exists as a command - skipping!",
					alias_iter.c_str()
				);

				continue;
			}

			commandlist[alias_iter] = working_cl_iter.second;
			commandaliases[alias_iter] = working_cl_iter.first;

			LogCommands(
				"command_init(): - Alias [{}] added to command [{}]",
				alias_iter.c_str(),
				commandaliases[alias_iter].c_str()
			);
		}
	}

	if (injected_command_settings.size()) {
		if (!database.UpdateInjectedCommandSettings(injected_command_settings)) {
			LogInfo("Failed to process 'Injected Commands' update operation.");
		}
	}

	command_dispatch = command_realdispatch;

	return commandcount;
}

void command_deinit(void){
	/*
	* command_deinit
	* clears the command list, freeing resources
	*
	* Parameters:
	*	none
	*
	*/
	commandlist.clear();
	commandaliases.clear();

	command_dispatch = command_notavail;
	commandcount = 0;
}

int command_add(std::string command_name, const char *desc, int access, CmdFuncPtr function){
	/*
	* command_add
	* adds a command to the command list; used by command_init
	*
	* Parameters:
	*	command_name	- the command ex: "spawn"
	*	desc		- text description of command for #help
	*	access		- default access level required to use command
	*	function		- pointer to function that handles command
	*
	*/
	if (command_name.empty()) {
		Log(Logs::General, Logs::Error, "command_add() - Command added with empty name string - check command.cpp.");
		return -1;
	}
	if (function == nullptr) {
		Log(Logs::General, Logs::Error, "command_add() - Command '%s' added without a valid function pointer - check command.cpp.", command_name.c_str());
		return -1;
	}
	if (commandlist.count(command_name) != 0) {
		Log(Logs::General, Logs::Error, "command_add() - Command '%s' is a duplicate command name - check command.cpp.", command_name.c_str());
		return -1;
	}
	for (auto iter = commandlist.begin(); iter != commandlist.end(); ++iter) {
		if (iter->second->function != function)
			continue;
		Log(Logs::General, Logs::Error, "command_add() - Command '%s' equates to an alias of '%s' - check command.cpp.", command_name.c_str(), iter->first.c_str());
		return -1;
	}

	auto c = new CommandRecord;
	c->access = access;
	c->desc = desc;
	c->function = function;

	commandlist[command_name] = c;
	commandaliases[command_name] = command_name;
	cleanup_commandlist.Append(c);
	commandcount++;

	return 0;
}

int command_realdispatch(Client *c, const char *message){
	/*
	*
	* command_realdispatch
	* Calls the correct function to process the client's command string.
	* Called from Client::ChannelMessageReceived if message starts with
	* command character (#).
	*
	* Parameters:
	*	c			- pointer to the calling client object
	*	message		- what the client typed
	*
	*/
	Seperator sep(message, ' ', 10, 100, true); // "three word argument" should be considered 1 arg

	command_logcommand(c, message);

	std::string cstr(sep.arg[0] + 1);

	if (commandlist.count(cstr) != 1)
	{
		return(-2);
	}

	CommandRecord *cur = commandlist[cstr];
	if (c->Admin() < cur->access)
	{
		c->Message(Chat::Red, "Your access level is not high enough to use this command.");
		return(-1);
	}

	if(cur->access >= COMMANDS_LOGGING_MIN_STATUS)
	{
		QServ->QSLogCommands(c, sep.arg[0], (char *)sep.argplus[1]);
	}

	if (cur->function == nullptr)
	{
		Log(Logs::General, Logs::Error, "Command '%s' has a null function\n", cstr.c_str());
		return(-1);
	}
	else
	{
		//dispatch C++ command
		cur->function(c, &sep);	// dispatch command
	}
	return 0;
}

void command_help(Client *c, const Seperator *sep){
	int commands_shown = 0;

	c->Message(Chat::Default, "Available EQEMu commands:");

	std::map<std::string, CommandRecord *>::iterator cur, end;
	cur = commandlist.begin();
	end = commandlist.end();

	for (; cur != end; ++cur) {
		if (sep->arg[1][0]) {
			if (cur->first.find(sep->arg[1]) == std::string::npos) {
				continue;
			}
		}

		if (c->Admin() < cur->second->access)
			continue;
		commands_shown++;
		c->Message(Chat::Default, "	%c%s %s", COMMAND_CHAR, cur->first.c_str(), cur->second->desc == nullptr ? "" : cur->second->desc);
	}
	c->Message(Chat::Default, "%d command%s listed.", commands_shown, commands_shown != 1 ? "s" : "");

}

void command_testcopy(Client *c, const Seperator *sep)
{
	if (c->GetTarget() == nullptr)
	{
		c->Message(Chat::Red, "You must target a player.");
	}
	else if (!c->GetTarget()->IsClient())
	{
		c->Message(Chat::Red, "You can only save backups of players.");
	}
	else if (c->GetTarget() && c->GetTarget()->IsClient())
	{
		Client* client = c->GetTarget()->CastToClient();
		int targetLSID = client->LSAccountID();
		int targetID = client->CharacterID();
		auto targetAcctID = database.GetAccountIDByChar(targetID);
		const char* targetName = client->GetName();
		c->Message(Chat::Default, "Backing up targetName: %s, targetLSID: %i, targetID: %i, targetAcctID: %i", targetName, targetLSID, targetAcctID, targetID);

		char String[255];

#ifdef _WINDOWS
		sprintf(String, "tak_testcopy.bat %s %i %i %i", targetName, targetLSID, targetAcctID, targetID);
#else
		sprintf(String, "./tak_testcopy %s %i %i %i", targetName, targetLSID, targetAcctID, targetID);
#endif
		int system_var = system(String);
	}
}

void command_testspawnkill(Client *c, const Seperator *sep){
	/*	auto outapp = new EQApplicationPacket(OP_Death, sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)outapp->pBuffer;
	d->corpseid = 1000;
	//	d->unknown011 = 0x05;
	d->spawn_id = 1000;
	d->killer_id = c->GetID();
	d->damage = 1;
	d->spell_id = 0;
	d->type = BASH;
	d->bindzoneid = 0;
	c->FastQueuePacket(&outapp);*/
}

void command_showpetspell(Client *c, const Seperator *sep){
	if (sep->arg[1][0] == 0)
		c->Message(Chat::Default, "Usage: #ShowPetSpells [spellid | searchstring]");
	else if (SPDAT_RECORDS <= 0)
		c->Message(Chat::Default, "Spells not loaded");
	else if (Seperator::IsNumber(sep->argplus[1]))
	{
		int spellid = atoi(sep->argplus[1]);
		if (spellid <= 0 || spellid >= SPDAT_RECORDS)
			c->Message(Chat::Default, "Error: Number out of range");
		else
			c->Message(Chat::Default, "	%i: %s, %s", spellid, spells[spellid].teleport_zone, spells[spellid].name);
	}
	else
	{
		int count = 0;
		std::string sName;
		std::string sCriteria;
		sCriteria = sep->argplus[1];
		for (auto & c : sCriteria) c = toupper(c);
		for (int i = 0; i < SPDAT_RECORDS; i++)
		{
			if (spells[i].name[0] != 0 && (spells[i].effectid[0] == SE_SummonPet || spells[i].effectid[0] == SE_NecPet))
			{
				sName = spells[i].teleport_zone;
				for (auto & c : sName) c = toupper(c);
				if (sName.find(sCriteria) != std::string::npos && (count <= 20))
				{
					c->Message(Chat::Default, "	%i: %s, %s", i, spells[i].teleport_zone, spells[i].name);
					count++;
				}
				else if (count > 20)
					break;
			}
		}
		if (count > 20)
			c->Message(Chat::Default, "20 spells found... max reached.");
		else
			c->Message(Chat::Default, "%i spells found.", count);
	}
}

#ifdef IPC
void command_ipc(Client *c, const Seperator *sep){
	if (c->GetTarget() && c->GetTarget()->IsNPC())
	{
		if (c->GetTarget()->CastToNPC()->IsInteractive())
		{
			c->GetTarget()->CastToNPC()->interactive = false;
			c->Message(Chat::Default, "Disabling IPC");
		}
		else
		{
			c->GetTarget()->CastToNPC()->interactive = true;
			c->Message(Chat::Default, "Enabling IPC");
		}
	}
	else
		c->Message(Chat::Default, "Error: You must target an NPC");
}
#endif /* IPC */

void command_findaliases(Client* c, const Seperator* sep)
{
	if (!sep->arg[1][0]) {
		c->Message(0, "Usage: #findaliases [alias | command]");
		return;
	}

	std::map<std::string, std::string>::iterator find_iter = commandaliases.find(sep->arg[1]);
	if (find_iter == commandaliases.end()) {
		c->Message(15, "No commands or aliases match '%s'", sep->arg[1]);
		return;
	}

	std::map<std::string, CommandRecord*>::iterator command_iter = commandlist.find(find_iter->second);
	if (find_iter->second.empty() || command_iter == commandlist.end()) {
		c->Message(0, "An unknown condition occurred...");
		return;
	}

	c->Message(0, "Available command aliases for '%s':", command_iter->first.c_str());

	int commandaliasesshown = 0;
	for (std::map<std::string, std::string>::iterator alias_iter = commandaliases.begin(); alias_iter != commandaliases.end(); ++alias_iter) {
		if (strcasecmp(find_iter->second.c_str(), alias_iter->second.c_str()) || c->Admin() < command_iter->second->access)
			continue;

		c->Message(0, "%c%s", COMMAND_CHAR, alias_iter->first.c_str());
		++commandaliasesshown;
	}
	c->Message(0, "%d command alias%s listed.", commandaliasesshown, commandaliasesshown != 1 ? "es" : "");
}

void command_npcedit(Client *c, const Seperator *sep){
	if (!c->GetTarget() || !c->GetTarget()->IsNPC())
	{
		c->Message(Chat::Default, "Error: Must have NPC targeted");
		return;
	}

	if (strcasecmp(sep->arg[1], "help") == 0) {

		c->Message(Chat::Default, "Help File for #npcedit. Syntax for commands are:");
		c->Message(Chat::Default, "#npcedit Name - Sets an NPC's name");
		c->Message(Chat::Default, "#npcedit Lastname - Sets an NPC's lastname");
		c->Message(Chat::Default, "#npcedit Level - Sets an NPC's level");
		c->Message(Chat::Default, "#npcedit Maxlevel - Sets an NPC's maximum level");
		c->Message(Chat::Default, "#npcedit Race - Sets an NPC's race");
		c->Message(Chat::Default, "#npcedit Class - Sets an NPC's class");
		c->Message(Chat::Default, "#npcedit Bodytype - Sets an NPC's bodytype");
		c->Message(Chat::Default, "#npcedit HP - Sets an NPC's hitpoints");
		c->Message(Chat::Default, "#npcedit Gender - Sets an NPC's gender");
		c->Message(Chat::Default, "#npcedit Texture - Sets an NPC's texture");
		c->Message(Chat::Default, "#npcedit Helmtexture - Sets an NPC's helmtexture");
		c->Message(Chat::Default, "#npcedit Armtexture - Sets an NPC's arm texture");
		c->Message(Chat::Default, "#npcedit Bracertexture - Sets an NPC's bracer texture");
		c->Message(Chat::Default, "#npcedit Handtexture - Sets an NPC's hand texture");
		c->Message(Chat::Default, "#npcedit Legtexture - Sets an NPC's leg texture");
		c->Message(Chat::Default, "#npcedit Feettexture - Sets an NPC's feettexture");
		c->Message(Chat::Default, "#npcedit Size - Sets an NPC's size");
		c->Message(Chat::Default, "#npcedit Hpregen - Sets an NPC's hitpoint regen rate per tick");
		c->Message(Chat::Default, "#npcedit Manaregen - Sets an NPC's mana regen rate per tick");
		c->Message(Chat::Default, "#npcedit Loottable - Sets the loottable ID for an NPC ");
		c->Message(Chat::Default, "#npcedit Merchantid - Sets the merchant ID for an NPC");
		c->Message(Chat::Default, "#npcedit npc_spells_effects_id - Sets the NPC Spell Effects ID");
		c->Message(Chat::Default, "#npcedit special_abilities - Sets the NPC's Special Abilities");
		c->Message(Chat::Default, "#npcedit Spell - Sets the npc spells list ID for an NPC");
		c->Message(Chat::Default, "#npcedit Faction - Sets the NPC's faction id");
		c->Message(Chat::Default, "#npcedit Mindmg - Sets an NPC's minimum damage");
		c->Message(Chat::Default, "#npcedit Maxdmg - Sets an NPC's maximum damage");
		c->Message(Chat::Default, "#npcedit Aggroradius - Sets an NPC's aggro radius");
		c->Message(Chat::Default, "#npcedit Assistradius - Sets an NPC's assist radius");
		c->Message(Chat::Default, "#npcedit Social - Set to 1 if an NPC should assist others on its faction");
		c->Message(Chat::Default, "#npcedit Runspeed - Sets an NPC's run speed");
		c->Message(Chat::Default, "#npcedit AGI - Sets an NPC's Agility");
		c->Message(Chat::Default, "#npcedit CHA - Sets an NPC's Charisma");
		c->Message(Chat::Default, "#npcedit DEX - Sets an NPC's Dexterity");
		c->Message(Chat::Default, "#npcedit INT - Sets an NPC's Intelligence");
		c->Message(Chat::Default, "#npcedit STA - Sets an NPC's Stamina");
		c->Message(Chat::Default, "#npcedit STR - Sets an NPC's Strength");
		c->Message(Chat::Default, "#npcedit WIS - Sets an NPC's Wisdom");
		c->Message(Chat::Default, "#npcedit MR - Sets an NPC's Magic Resistance");
		c->Message(Chat::Default, "#npcedit PR - Sets an NPC's Poison Resistance");
		c->Message(Chat::Default, "#npcedit DR - Sets an NPC's Disease Resistance");
		c->Message(Chat::Default, "#npcedit FR - Sets an NPC's Fire Resistance");
		c->Message(Chat::Default, "#npcedit CR - Sets an NPC's cold resistance");
		c->Message(Chat::Default, "#npcedit Seeinvis - Sets an NPC's ability to see invis");
		c->Message(Chat::Default, "#npcedit Seeinvisundead - Sets an NPC's ability to see through invis vs. undead");
		c->Message(Chat::Default, "#npcedit Seehide - Sets an NPC's ability to see through hide");
		c->Message(Chat::Default, "#npcedit Seeimprovedhide - Sets an NPC's ability to see through improved hide");
		c->Message(Chat::Default, "#npcedit AC - Sets an NPC's Armor Class");
		c->Message(Chat::Default, "#npcedit ATK - Sets an NPC's Attack");
		c->Message(Chat::Default, "#npcedit Accuracy - Sets an NPC's Accuracy");
		c->Message(Chat::Default, "#npcedit npcaggro - Sets an NPC's npc_aggro flag");
		c->Message(Chat::Default, "#npcedit qglobal - Sets an NPC's quest global flag");
		c->Message(Chat::Default, "#npcedit limit - Sets an NPC's spawn limit counter");
		c->Message(Chat::Default, "#npcedit Attackspeed - Sets an NPC's attack speed modifier");
		c->Message(Chat::Default, "#npcedit Attackdelay - Sets an NPC's attack delay");
		c->Message(Chat::Default, "#npcedit wep1 - Sets an NPC's primary weapon model");
		c->Message(Chat::Default, "#npcedit wep2 - Sets an NPC's secondary weapon model");
		c->Message(Chat::Default, "#npcedit featuresave - Saves all current facial features to the database");
		c->Message(Chat::Default, "#npcedit color - Sets an NPC's red, green, and blue armor tint");
		c->Message(Chat::Default, "#npcedit armortint_id - Set an NPC's Armor tint ID");
		c->Message(Chat::Default, "#npcedit setanimation - Set an NPC's animation on spawn (Stored in spawn2 table)");
		c->Message(Chat::Default, "#npcedit scalerate - Set an NPC's scaling rate");
		c->Message(Chat::Default, "#npcedit healscale - Set an NPC's heal scaling rate");
		c->Message(Chat::Default, "#npcedit spellscale - Set an NPC's spell scaling rate");
		c->Message(Chat::Default, "#npcedit version - Set an NPC's version");

	}

	uint32 npcTypeID = c->GetTarget()->CastToNPC()->GetNPCTypeID();

	if (strcasecmp(sep->arg[1], "name") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has the name %s.", npcTypeID, sep->argplus[2]);

		std::string query = StringFormat("UPDATE npc_types SET name = '%s' WHERE id = %i",
			sep->argplus[2], npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "lastname") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has the lastname %s.", npcTypeID, sep->argplus[2]);

		std::string query = StringFormat("UPDATE npc_types SET lastname = '%s' WHERE id = %i",
			sep->argplus[2], npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "race") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has the race %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET race = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "class") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now class %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET class = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "bodytype") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has type %i bodytype.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET bodytype = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "hp") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Hitpoints.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET hp = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "gender") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now gender %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET gender = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "texture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses texture %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET texture = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "helmtexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses helmtexture %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET helmtexture = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "armtexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses armtexture %i.", npcTypeID, atoi(sep->argplus[2]));
		std::string query = StringFormat("UPDATE npc_types SET armtexture = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "bracertexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses bracertexture %i.", npcTypeID, atoi(sep->argplus[2]));
		std::string query = StringFormat("UPDATE npc_types SET bracertexture = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "handtexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses handtexture %i.", npcTypeID, atoi(sep->argplus[2]));
		std::string query = StringFormat("UPDATE npc_types SET handtexture = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "legtexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses legtexture %i.", npcTypeID, atoi(sep->argplus[2]));
		std::string query = StringFormat("UPDATE npc_types SET legtexture = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "feettexture") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses feettexture %i.", npcTypeID, atoi(sep->argplus[2]));
		std::string query = StringFormat("UPDATE npc_types SET feettexture = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "size") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now size %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET size = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "hpregen") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now regens %i hitpoints per tick.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET hp_regen_rate = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "manaregen") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now regens %i mana per tick.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET mana_regen_rate = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "loottable") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now on loottable_id %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET loottable_id = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "merchantid") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now merchant_id %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET merchant_id = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "npc_spells_effects_id") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has field 'npc_spells_effects_id' set to %s.", npcTypeID, sep->argplus[2]);

		std::string query = StringFormat("UPDATE npc_types SET npc_spells_effects_id = '%s' WHERE id = %i",
			sep->argplus[2], npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "special_abilities") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has field 'special_abilities' set to %s.", npcTypeID, sep->argplus[2]);

		std::string query = StringFormat("UPDATE npc_types SET special_abilities = '%s' WHERE id = %i",
			sep->argplus[2], npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "spell") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now uses spell list %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET npc_spells_id = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "faction") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now faction %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET npc_faction_id = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "mindmg") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now hits for a min of %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET mindmg = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "maxdmg") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now hits for a max of %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET maxdmg = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "aggroradius") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has an aggro radius of %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET aggroradius = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "assistradius") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has an assist radius of %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET assistradius = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "social") == 0) {
		c->Message(Chat::Yellow, "NPCID %u social status is now %i", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET social = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "runspeed") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now runs at %f", npcTypeID, atof(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET runspeed = %f WHERE id = %i",
			atof(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "AGI") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Agility.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET AGI = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "CHA") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Charisma.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET CHA = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "DEX") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Dexterity.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET DEX = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "INT") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Intelligence.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET _INT = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "STA") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Stamina.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET STA = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "STR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Strength.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET STR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "WIS") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Magic Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET WIS = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "MR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Magic Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET MR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "DR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Disease Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET DR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "CR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Cold Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET CR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "FR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Fire Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET FR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "PR") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a Poison Resistance of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET PR = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "seeinvis") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has seeinvis set to %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET see_invis = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "seeinvisundead") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has seeinvisundead set to %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET see_invis_undead = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "seesneak") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has seesneak set to %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET see_sneak = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "seeimprovedhide") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has seeimprovedhide set to %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET see_improved_hide = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "AC") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Armor Class.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET ac = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "ATK") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Attack.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET atk = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "Accuracy") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i Accuracy.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET accuracy = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "level") == 0) {
		c->Message(Chat::Yellow, "NPCID %u is now level %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET level = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "maxlevel") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a maximum level of %i.", npcTypeID, atoi(sep->argplus[2]));

		std::string query = StringFormat("UPDATE npc_types SET maxlevel = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "qglobal") == 0) {
		c->Message(Chat::Yellow, "Quest globals have been %s for NPCID %u",
			atoi(sep->arg[2]) == 0 ? "disabled" : "enabled", npcTypeID);

		std::string query = StringFormat("UPDATE npc_types SET qglobal = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "npcaggro") == 0) {
		c->Message(Chat::Yellow, "NPCID %u will now %s other NPCs with negative faction npc_value",
			npcTypeID, atoi(sep->arg[2]) == 0 ? "not aggro" : "aggro");

		std::string query = StringFormat("UPDATE npc_types SET npc_aggro = %i WHERE id = %i",
			atoi(sep->argplus[2]) == 0 ? 0 : 1, npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "limit") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a spawn limit of %i",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET limit = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "Attackdelay") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has attack_delay set to %i", npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET attack_delay = %i WHERE id = %i", atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}


	if (strcasecmp(sep->arg[1], "wep1") == 0) {
		c->Message(Chat::Yellow, "NPCID %u will have item graphic %i set to his primary on repop.",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET d_melee_texture1 = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "wep2") == 0) {
		c->Message(Chat::Yellow, "NPCID %u will have item graphic %i set to his secondary on repop.",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET d_melee_texture2 = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "featuresave") == 0) {
		c->Message(Chat::Yellow, "NPCID %u saved with all current facial feature settings",
			npcTypeID);

		Mob* target = c->GetTarget();

		std::string query = StringFormat("UPDATE npc_types "
			"SET luclin_haircolor = %i, luclin_beardcolor = %i, "
			"luclin_hairstyle = %i, luclin_beard = %i, "
			"face = %i "
			"WHERE id = %i",
			target->GetHairColor(), target->GetBeardColor(),
			target->GetHairStyle(), target->GetBeard(),
			target->GetLuclinFace(), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "color") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has %i red, %i green, and %i blue tinting on their armor.",
			npcTypeID, atoi(sep->arg[2]), atoi(sep->arg[3]), atoi(sep->arg[4]));

		std::string query = StringFormat("UPDATE npc_types "
			"SET armortint_red = %i, armortint_green = %i, armortint_blue = %i "
			"WHERE id = %i",
			atoi(sep->arg[2]), atoi(sep->arg[3]), atoi(sep->arg[4]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "armortint_id") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has field 'armortint_id' set to %s",
			npcTypeID, sep->arg[2]);

		std::string query = StringFormat("UPDATE npc_types SET armortint_id = '%s' WHERE id = %i",
			sep->argplus[2], npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "setanimation") == 0) {
		int animation = 0;
		if (sep->arg[2] && atoi(sep->arg[2]) <= 4) {
			if ((strcasecmp(sep->arg[2], "stand") == 0) || atoi(sep->arg[2]) == 0)
				animation = 0; //Stand
			if ((strcasecmp(sep->arg[2], "sit") == 0) || atoi(sep->arg[2]) == 1)
				animation = 1; //Sit
			if ((strcasecmp(sep->arg[2], "crouch") == 0) || atoi(sep->arg[2]) == 2)
				animation = 2; //Crouch
			if ((strcasecmp(sep->arg[2], "dead") == 0) || atoi(sep->arg[2]) == 3)
				animation = 3; //Dead
			if ((strcasecmp(sep->arg[2], "loot") == 0) || atoi(sep->arg[2]) == 4)
				animation = 4; //Looting Animation
		}
		else {
			c->Message(Chat::Default, "You must specifiy an animation stand, sit, crouch, dead, loot (0-4)");
			c->Message(Chat::Default, "Example: #npcedit setanimation sit");
			c->Message(Chat::Default, "Example: #npcedit setanimation 0");
			return;
		}

		c->Message(Chat::Yellow,"NPCID %u now has the animation set to %i on spawn with spawngroup %i", npcTypeID, animation, c->GetTarget()->CastToNPC()->GetSp2() );

		std::string query = StringFormat("UPDATE spawn2 SET animation = %i "
			"WHERE spawngroupID = %i",
			animation, c->GetTarget()->CastToNPC()->GetSp2());
		database.QueryDatabase(query);

		c->GetTarget()->SetAppearance(EmuAppearance(animation));
		return;
		}

	if (strcasecmp(sep->arg[1], "scalerate") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a scaling rate of %i.",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET scalerate = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "healscale") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a heal scaling rate of %i.",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET healscale = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if (strcasecmp(sep->arg[1], "spellscale") == 0) {
		c->Message(Chat::Yellow, "NPCID %u now has a spell scaling rate of %i.",
			npcTypeID, atoi(sep->arg[2]));

		std::string query = StringFormat("UPDATE npc_types SET spellscale = %i WHERE id = %i",
			atoi(sep->argplus[2]), npcTypeID);
		database.QueryDatabase(query);
		return;
	}

	if ((sep->arg[1][0] == 0 || strcasecmp(sep->arg[1], "*") == 0) || ((c->GetTarget() == 0) || (c->GetTarget()->IsClient())))
		c->Message(Chat::Default, "Type #npcedit help for more info");

}

#ifdef PACKET_PROFILER
void command_packetprofile(Client *c, const Seperator *sep){
	Client *t = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		t = c->GetTarget()->CastToClient();
	}
	c->DumpPacketProfile();
}
#endif

#ifdef EQPROFILE
void command_profiledump(Client *c, const Seperator *sep){
	DumpZoneProfile();
}

void command_update(Client *c, const Seperator *sep)
{
	int admin = c->Admin();
	int system_var = 0;
	std::string help0 = "Update commands usage:";
	std::string help1 = "  #update quests - Updates all zone quests on the server from svn - Does not reload quests.";
	std::string help2 = "  #update source - Fires off a 10 min shutdown warning, takes down server,";
	std::string help3 = "					downloads current git and compiles then restarts the server.";
	std::string help4 = "  #update reboot - Fires off a 10 min shutdown warning and restarts the server without updates.";
	std::string help5 = "  #update rebootNOW - Restarts the server without updates immediately.";

	std::string help[] = { help0, help1, help2, help3, help4, help5 };

	if (strcasecmp(sep->arg[1], "help") == 0)
	{
		int size = sizeof(help) / sizeof(std::string);
		for (int i = 0; i < size; i++)
		{
			c->Message(Chat::Default, help[i].c_str());
		}
	}
	else if (strcasecmp(sep->arg[1], "quests") == 0)
	{
		FILE *fp;
#ifdef _WINDOWS
		char buf[1024];
		fp = _popen("svn update quests", "r");

		while (fgets(buf, 1024, fp))
		{
			const char * output = (const char *)buf;
			c->Message(Chat::Default, "%s", output);
		}
		fclose(fp);
#else
		char* buf = NULL;
		size_t len = 0;
		fflush(NULL);
		fp = popen("svn update quests", "r");

		while (getline(&buf, &len, fp) != -1)
		{
			const char * output = (const char *)buf;
			c->Message(Chat::Default, "%s", output);
		}
		free(buf);
		fflush(fp);
#endif
		c->Message(Chat::Default, "Quests are updated.");
	}
	else if (strcasecmp(sep->arg[1], "source") == 0)
	{
		if (admin >= 205)
		{
#ifdef _WINDOWS
			// TODO: Add same functionality for windows from the following command.
			c->Message(Chat::Default, "Not yet implemented for windows.");
#else
			c->Message(Chat::Default, "Server will be going down and building, 10 min warning issued.");
			system_var = system("./EQServer.sh update");
#endif
		}
		else
			c->Message(Chat::Default, "Your access level is not high enough to use this command.");
	}
	else if (strcasecmp(sep->arg[1], "reboot") == 0)
	{
		if (admin >= 205)
		{
#ifdef _WINDOWS
			// TODO: Add same functionality for windows from the following command.
			c->Message(Chat::Default, "Not yet implemented for windows.");
#else
			c->Message(Chat::Default, "Server will be going down for reboot, 10 min warning issued.");
			system_var = system("./EQServer.sh reboot");
#endif
		}
		else
			c->Message(Chat::Default, "Your access level is not high enough to use this command.");
	}
	else if (strcasecmp(sep->arg[1], "rebootNOW") == 0)
	{
		if (admin >= 250)
		{
#ifdef _WINDOWS
			// TODO: Add same functionality for windows from the following command.
			c->Message(Chat::Default, "Not yet implemented for windows.");
#else
			c->Message(Chat::Default, "Server will be going down for reboot.");
			system_var = system("./EQServer.sh rebootNOW");
#endif
		}
		else
			c->Message(Chat::Default, "Your access level is not high enough to use this command.");
	}
	else
	{
		int size = sizeof(help) / sizeof(std::string);
		for (int i = 0; i < size; i++)
		{
			c->Message(Chat::Default, help[i].c_str());
		}
	}
}

void command_coredump(Client *c, const Seperator *sep)
{
#ifdef _WINDOWS
	// TODO: Add same functionality for windows from the following command.
	// Maybe have a batch file spit logs to a web folder?
	c->Message(Chat::Default, "Not yet implemented for windows.");
#else
	int system_var = system("./dump");
	c->Message(Chat::Default, "core dumped.");
#endif
}

void command_enablerecipe(Client *c, const Seperator *sep){
	uint32 recipe_id = 0;
	bool success = false;
	if (c) {
		if (sep->argnum == 1) {
			recipe_id = atoi(sep->arg[1]);
		}
		else {
			c->Message(Chat::Default, "Invalid number of arguments.\nUsage: #enablerecipe recipe_id");
			return;
		}
		if (recipe_id > 0) {
			success = database.EnableRecipe(recipe_id);
			if (success) {
				c->Message(Chat::Default, "Recipe enabled.");
			}
			else {
				c->Message(Chat::Default, "Recipe not enabled.");
			}
		}
		else {
			c->Message(Chat::Default, "Invalid recipe id.\nUsage: #enablerecipe recipe_id");
		}
	}
}

void command_hotfix(Client *c, const Seperator *sep)
{
	auto items_count = database.GetItemsCount();
	auto shared_items_count = database.GetSharedItemsCount();
	if (items_count != shared_items_count) {
		c->Message(Chat::Yellow, "Your database does not have the same item count as your shared memory.");

		c->Message(
			Chat::Yellow,
			fmt::format(
				"Database Count: {} Shared Memory Count: {}",
				items_count,
				shared_items_count
			).c_str()
		);

		c->Message(Chat::Yellow, "If you want to be able to add new items to your server while it is online, you need to create placeholder entries in the database ahead of time and do not add or remove rows/entries. Only modify the existing placeholder rows/entries to safely use #hotfix.");

		return;
	}

	auto spells_count = database.GetSpellsCount();
	auto shared_spells_count = database.GetSharedSpellsCount();
	if (spells_count != shared_spells_count) {
		c->Message(Chat::Yellow, "Your database does not have the same spell count as your shared memory.");

		c->Message(
			Chat::Yellow,
			fmt::format(
				"Database Count: {} Shared Memory Count: {}",
				spells_count,
				shared_spells_count
			).c_str()
		);

		c->Message(Chat::Yellow, "If you want to be able to add new spells to your server while it is online, you need to create placeholder entries in the database ahead of time and do not add or remove rows/entries. Only modify the existing placeholder rows/entries to safely use #hotfix.");

		c->Message(Chat::Yellow, "Note: You may still have to distribute a spell file, even with dynamic changes.");

		return;
	}

	std::string hotfix;
	database.GetVariable("hotfix_name", hotfix);

	std::string hotfix_name;
	if(!strcasecmp(hotfix.c_str(), "hotfix_")) {
		hotfix_name = "";
	}
	else
	{
		hotfix_name = "hotfix_";
	}

	c->Message(Chat::Default, "Creating and applying hotfix");
	std::thread t1([c, hotfix_name]() {
		int sysRet = -1;
#ifdef WIN32
		if (hotfix_name.length() > 0)
		{
			sysRet = system(StringFormat("shared_memory -hotfix=%s", hotfix_name.c_str()).c_str());
		}
		else
		{
			sysRet = system(StringFormat("shared_memory").c_str());
		}
#else
		if (hotfix_name.length() > 0)
		{
			sysRet = system(StringFormat("./shared_memory -hotfix=%s", hotfix_name.c_str()).c_str());
		}
		else
		{
			sysRet = system(StringFormat("./shared_memory").c_str());
		}
#endif
		if (sysRet == -1)
		{
			c->Message(Chat::Default, "Hotfix failed.");
			return;
		}
		database.SetVariable("hotfix_name", hotfix_name);

		ServerPacket pack(ServerOP_ChangeSharedMem, hotfix_name.length() + 1);
		if (hotfix_name.length() > 0)
		{
			strcpy((char*)pack.pBuffer, hotfix_name.c_str());
		}
		worldserver.SendPacket(&pack);

		c->Message(Chat::Default, "Hotfix applied");
	});

	t1.detach();
}

void command_load_shared_memory(Client *c, const Seperator *sep)
{
	std::string hotfix;
	database.GetVariable("hotfix_name", hotfix);

	std::string hotfix_name;
	if(strcasecmp(hotfix.c_str(), sep->arg[1]) == 0) {
		c->Message(Chat::Default, "Cannot attempt to load this shared memory segment as it is already loaded.");
		return;
	}

	hotfix_name = sep->arg[1];
	c->Message(Chat::Default, "Loading shared memory segment %s", hotfix_name.c_str());
	std::thread t1([c, hotfix_name]()
	{
		int sysRet = -1;
#ifdef WIN32
		if(hotfix_name.length() > 0)
		{
			sysRet = system(StringFormat("shared_memory -hotfix=%s", hotfix_name.c_str()).c_str());
		}
		else
		{
			sysRet = system(StringFormat("shared_memory").c_str());
		}
#else
		if(hotfix_name.length() > 0)
		{
			sysRet = system(StringFormat("./shared_memory -hotfix=%s", hotfix_name.c_str()).c_str());
		}
		else
		{
			sysRet = system(StringFormat("./shared_memory").c_str());
		}
#endif
		if (sysRet == -1)
		{
			c->Message(Chat::Default, "Shared memory segment failed loading.");
			return;
		}
		c->Message(Chat::Default, "Shared memory segment finished loading.");
	});

	t1.detach();
}

void command_apply_shared_memory(Client *c, const Seperator *sep) {
	std::string hotfix;
	database.GetVariable("hotfix_name", hotfix);
	std::string hotfix_name = sep->arg[1];

	c->Message(Chat::Default, "Applying shared memory segment %s", hotfix_name.c_str());
	database.SetVariable("hotfix_name", hotfix_name);

	ServerPacket pack(ServerOP_ChangeSharedMem, hotfix_name.length() + 1);
	if(hotfix_name.length() > 0) {
		strcpy((char*)pack.pBuffer, hotfix_name.c_str());
	}
	worldserver.SendPacket(&pack);
}

