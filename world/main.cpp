/**
 * EQEmulator: Everquest Server Emulator
 * Copyright (C) 2001-2019 EQEmulator Development Team (https://github.com/EQEmu/Server)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY except by those people which sell it, which
 * are required to give you total support for your newly bought product;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define PLATFORM_WORLD 1

#include "../common/global_define.h"

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_set>

#include <signal.h>

#include "../common/strings.h"
#include "../common/eqemu_logsys.h"
#include "../common/queue.h"
#include "../common/timer.h"
#include "../common/eq_packet.h"
#include "../common/seperator.h"
#include "../common/version.h"
#include "../common/eqtime.h"
#include "../common/timeoutmgr.h"
#include "../common/strings.h"
#include "../common/emu_constants.h"
#include "../common/event/event_loop.h"
#include "../common/eq_stream_factory.h"
#include "../common/net/eqstream.h"
#include "../common/opcodemgr.h"
#include "../common/guilds.h"
#include "../common/eq_stream_ident.h"
#include "../common/rulesys.h"
#include "../common/platform.h"
#include "../common/crash.h"
#include "../common/misc.h"
#include "client.h"
#include "worlddb.h"
#include "wguild_mgr.h"

#ifdef _WINDOWS
#include <process.h>
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp	_stricmp
#include <conio.h>
#else

#include <sys/sem.h>
#include <thread>

#endif

#include "../common/patches/patches.h"
#include "zoneserver.h"
#include "login_server.h"
#include "login_server_list.h"
#include "world_config.h"
#include "zonelist.h"
#include "clientlist.h"
#include "launcher_list.h"
#include "ucs.h"
#include "queryserv.h"
#include "web_interface.h"
#include "console.h"

#include "world_server_cli.h"
#include "../common/content/world_content_service.h"
#include "../common/zone_store.h"
#include "world_event_scheduler.h"
#include "../common/path_manager.h"
#include "../common/skill_caps.h"

SkillCaps skill_caps;
ZoneStore zone_store;
TimeoutManager timeout_manager;
EQStreamFactory eqsf(WorldStream,9000);
ClientList client_list;
ZSList zoneserver_list;
LoginServerList loginserverlist;
UCSConnection UCSLink;
QueryServConnection QSLink;
LauncherList launcher_list; 
WorldEventScheduler event_scheduler;
EQ::Random emu_random;
volatile bool RunLoops = true;
uint32 numclients = 0;
uint32 numzones = 0;
bool holdzones = false;
const WorldConfig *Config;
EQEmuLogSys LogSys;
ServerEarthquakeImminent_Struct next_quake;
std::unordered_set<uint32> ipWhitelist;
std::mutex		ipMutex;
bool bSkipFactoryAuth = false;
WebInterfaceList web_interface;
WorldContentService content_service;
PathManager         path;

void CatchSignal(int sig_num);

void LoadDatabaseConnections()
{
	LogInfo(
			"Connecting to MySQL {0}@{1}:{2}...", 
			Config->DatabaseUsername.c_str(), 
			Config->DatabaseHost.c_str(), 
			Config->DatabasePort
	);
	if (!database.Connect(
			Config->DatabaseHost.c_str(),
			Config->DatabaseUsername.c_str(),
			Config->DatabasePassword.c_str(),
			Config->DatabaseDB.c_str(),
			Config->DatabasePort
	)) {
		LogError("Cannot continue without a database connection.");

		std::exit(1);
	}
}

Timer NextQuakeTimer(900000);
Timer DisableQuakeTimer(900000);

void TriggerManualQuake(QuakeType in_quake_type)
{
	uint32 cur_time = Timer::GetTimeSeconds();
	database.SaveNextQuakeTime(next_quake, in_quake_type);

	NextQuakeTimer.Enable();
	NextQuakeTimer.Start((next_quake.next_start_timestamp - cur_time) * 1000);

	std::string motd_str = "Welcome to Project Quarm! ";
	motd_str += "An earthquake ruleset is currently in effect in raid zones.";

	database.SetVariable("MOTD", motd_str.c_str());

	auto pack2 = new ServerPacket(ServerOP_Motd, sizeof(ServerMotd_Struct));
	auto mss = (ServerMotd_Struct*)pack2->pBuffer;
	strn0cpy(mss->myname, "Druzzil", sizeof(mss->myname));
	strn0cpy(mss->motd, motd_str.c_str(), sizeof(mss->motd));

	zoneserver_list.SendPacket(pack2);

	//Roleplay flavor text, go!
	zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Red, "Druzzil Ro's voice echoes in your mind, 'Beware, mortal. Creatures of legendary strength return to the world for a limited time.'");
	zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Yellow, "Druzzil Ro's projection alters time and space. Raid creatures have appeared in open world for a short time. Rule 9.x and Rule 10.x have been suspended in open world raid zones temporarily.");

	//Inform of imminent quake. This happens after the MOTD so zone denizens are informed again with relevant information.
	auto pack = new ServerPacket(ServerOP_QuakeImminent, sizeof(ServerEarthquakeImminent_Struct));
	ServerEarthquakeImminent_Struct* seis = (ServerEarthquakeImminent_Struct*)pack->pBuffer;
	seis->quake_type = in_quake_type;
	seis->next_start_timestamp = next_quake.next_start_timestamp;
	seis->start_timestamp = next_quake.start_timestamp;
	zoneserver_list.SendPacket(pack);

	safe_delete(pack2);
	safe_delete(pack);

	//Timer needs to be set to enforce MOTD rules.
	DisableQuakeTimer.Enable();
	DisableQuakeTimer.Start(((next_quake.start_timestamp - cur_time) + RuleI(Quarm, QuakeEndTimeDuration)) * 1000);
}

void LoadServerConfig()
{
	// Load server configuration
	LogInfo("Loading server configuration..");
	if (!WorldConfig::LoadConfig()) {
		LogError("Loading server configuration failed.");
		std::exit(1);
	}
}

void RegisterLoginservers()
{
	// add login server config to list
	if (Config->LoginCount == 0) {
		if (Config->LoginHost.length()) {
			loginserverlist.Add(
				Config->LoginHost.c_str(), 
				Config->LoginPort, 
				Config->LoginAccount.c_str(), 
				Config->LoginPassword.c_str(), 
				Config->LoginType
			);
			LogInfo("Added loginserver [{0}]:[{1}]", Config->LoginHost.c_str(), Config->LoginPort);
		}
	}
	else {
		LinkedList<LoginConfig*> loginlist = Config->loginlist;
		LinkedListIterator<LoginConfig*> iterator(loginlist);
		iterator.Reset();
		while (iterator.MoreElements()) {
			if (iterator.GetData()->LoginHost.length()) {
				loginserverlist.Add(
					iterator.GetData()->LoginHost.c_str(),
					iterator.GetData()->LoginPort,
					iterator.GetData()->LoginAccount.c_str(),
					iterator.GetData()->LoginPassword.c_str(),
					iterator.GetData()->LoginType
				);
				LogInfo(
					"Added loginserver [{0}]:[{1}]",
					iterator.GetData()->LoginHost.c_str(),
					iterator.GetData()->LoginPort
				);
			}
			iterator.Advance();
		}
	}
}

/**
 * World process entrypoint
 *
 * @param argc
 * @param argv
 * @return
 */

int main(int argc, char** argv) {
	RegisterExecutablePlatform(ExePlatformWorld);
	LogSys.LoadLogSettingsDefaults();
	set_exception_handler();

	if (argc > 1) {
		LogSys.SilenceConsoleLogging();
		path.LoadPaths();
		WorldConfig::LoadConfig();
		RuleManager::Instance()->LoadRules(&database, "default", false);
		LogSys.EnableConsoleLogging();
		WorldserverCLI::CommandHandler(argc, argv);
	}

	path.LoadPaths();

	LoadServerConfig();

	Config = WorldConfig::get();

	LogInfo("CURRENT_VERSION: [{0}]", CURRENT_VERSION);


	if (signal(SIGINT, CatchSignal) == SIG_ERR)	{
		LogError("Could not set signal handler");
		return 1;
	}
	if (signal(SIGTERM, CatchSignal) == SIG_ERR) {
		LogError("Could not set signal handler");
		return 1;
	}
#ifndef WIN32
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		LogError("Could not set signal handler");
		return 1;
	}
#endif

	RegisterLoginservers();
	LoadDatabaseConnections();

	auto logging = LogSys.SetDatabase(&database)
		->SetLogPath(path.GetLogPath())
		->LoadLogDatabaseSettings();

	logging->StartFileLogs();

	LogInfo("Loading variables..");
	database.LoadVariables();

	std::string hotfix_name;
	if (database.GetVariable("hotfix_name", hotfix_name)) {
		if (!hotfix_name.empty()) {
			LogInfo("Current hotfix in use: [{0}]", hotfix_name.c_str());
		}
	}

	guild_mgr.SetDatabase(&database);

	LogInfo("Purging expiring data buckets...");
	database.PurgeAllDeletedDataBuckets();

	LogInfo("Loading zones..");
	zone_store.LoadZones(database);

	if (zone_store.GetZones().empty()) {
		LogError("Failed to load zones data, check your schema for possible errors");
		return 1;
	}

	LogInfo("Clearing groups..");
	database.ClearGroup();
	LogInfo("Clearing raids..");
	database.ClearRaid();
	database.ClearRaidDetails();
	LogInfo("Loading items..");
	
	if (!database.LoadItems(hotfix_name)) {
		LogError("Error: Could not load item data. But ignoring");
	}

	LogInfo("Loading guilds..");
	guild_mgr.LoadGuilds();

	//rules:
	{
		if (!RuleManager::Instance()->UpdateOrphanedRules(&database)) {
			LogInfo("Failed to process 'Orphaned Rules' update operation.");
		}

		if (!RuleManager::Instance()->UpdateInjectedRules(&database, "default")) {
			LogInfo("Failed to process 'Injected Rules' for ruleset 'default' update operation.");
		}

		std::string tmp;
		if (database.GetVariable("RuleSet", tmp)) {
			LogInfo("Loading rule set [{0}]", tmp.c_str());
			if (!RuleManager::Instance()->LoadRules(&database, tmp.c_str())) {
				LogInfo("Failed to load ruleset [{0}], falling back to defaults.", tmp.c_str());
			}
		}
		else {
			if (!RuleManager::Instance()->LoadRules(&database, "default")) {
				LogInfo("No rule set configured, using default rules");
			}
			else {
				LogInfo("Loaded default rule set 'default'", tmp.c_str());
			}
		}

		if (!RuleManager::Instance()->RestoreRuleNotes(&database)) {
			LogInfo("Failed to process 'Restore Rule Notes' update operation.");
		}
	}

	if(RuleB(World, ClearTempMerchantlist)){
		LogInfo("Clearing temporary merchant lists...");
		database.ClearMerchantTemp();
	}

	if (RuleB(World, AdjustRespawnTimes)) {
		LogInfo("Clearing and adjusting boot time spawn timers...");
		database.AdjustSpawnTimes();
	}

	LogInfo("Loading EQ time of day..");
	TimeOfDay_Struct eqTime;
	time_t realtime;
	eqTime = database.LoadTime(realtime);
	Timer EQTimeTimer(600000);
	EQTimeTimer.Start(600000);

	memset(&next_quake, 0, sizeof(ServerEarthquakeImminent_Struct));
	NextQuakeTimer.Disable();
	DisableQuakeTimer.Disable();


	if (RuleB(Quarm, EnableQuakes))
	{
		//This will return false if we have bad quake data, or the quake happened within 24 hours of a downtime.
		bool bQuakeReset = database.LoadNextQuakeTime(next_quake);
		if (bQuakeReset)
		{
			//We're outside of the 24 hour window. Players will wait normal "next_start_timestamp" amount.
			next_quake.quake_type = QuakeType::QuakeDisabled;
			NextQuakeTimer.Enable();
			NextQuakeTimer.Start((next_quake.next_start_timestamp - Timer::GetTimeSeconds()) * 1000);
			Log(Logs::Detail, Logs::WorldServer, "Using next_start_timestamp to calculate next trigger time.. %i", (next_quake.next_start_timestamp - Timer::GetTimeSeconds()));
		}
		else
		{
			//Start the timer in 15 minutes. (magic value is set in fail condition)
			//Process normal quake logic after.
			Log(Logs::Detail, Logs::WorldServer, "Using start_timestamp to calculate next trigger time.. %i", (next_quake.start_timestamp - Timer::GetTimeSeconds()));
			next_quake.quake_type = QuakeType::QuakeDisabled;
			NextQuakeTimer.Enable();
			NextQuakeTimer.Start((next_quake.start_timestamp - Timer::GetTimeSeconds()) * 1000);
		}
	}
	zoneserver_list.worldclock.SetCurrentEQTimeOfDay(eqTime,realtime);

	LogInfo("Clearing Saylinks..");
	database.ClearSayLink();

	LogInfo("Deleted [{0}] stale player corpses from database", database.DeleteStalePlayerCorpses());

	LogInfo("Clearing active accounts...");
	database.ClearAllActive();

	LogInfo("Clearing consented characters...");
	database.ClearAllConsented();

	LogInfo("Loading char create info...");
	database.LoadCharacterCreateAllocations();
	database.LoadCharacterCreateCombos();

	LogInfo("Initializing [EventScheduler]");
	event_scheduler.SetDatabase(&database)->LoadScheduledEvents();

	// global loads
	LogInfo("Loading launcher list..");
	launcher_list.LoadList();
	zoneserver_list.Init();

	std::unique_ptr<EQ::Net::ConsoleServer> console;
	if (Config->TelnetEnabled) {
		LogInfo("Console (TCP) listener started on [{}:{}]", Config->TelnetIP, Config->TelnetTCPPort);
		console = std::make_unique<EQ::Net::ConsoleServer>(Config->TelnetIP, Config->TelnetTCPPort);
		RegisterConsoleFunctions(console);
	}

	LogInfo("Initializing [WorldContentService]");
	content_service.SetDatabase(&database)
		->SetExpansionContext()
		->ReloadContentFlags();

	skill_caps.SetContentDatabase(&database)->LoadSkillCaps();

	std::unique_ptr<EQ::Net::ServertalkServer> server_connection;
	server_connection = std::make_unique<EQ::Net::ServertalkServer>();

	EQ::Net::ServertalkServerOptions server_opts;
	server_opts.port = Config->WorldTCPPort;
	server_opts.ipv6 = false;
	server_opts.credentials = Config->SharedKey;
	server_connection->Listen(server_opts);
	LogInfo("Server (TCP) listener started on port [{}]", Config->WorldTCPPort);
		
	server_connection->OnConnectionIdentified(
		"Zone", [&console](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			numzones++;
			zoneserver_list.Add(new ZoneServer(connection, console.get()));

			LogInfo(
				"New Zone Server connection from [{}] at [{}:{}] zone_count [{}]",
				connection->Handle()->RemoteIP(),
				connection->Handle()->RemotePort(),
				connection->GetUUID(),
				numzones
			);
		}
	);

	server_connection->OnConnectionRemoved(
		"Zone", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			numzones--;
			zoneserver_list.Remove(connection->GetUUID());

			LogInfo(
				"Removed Zone Server connection from [{}] total zone_count [{}]",
				connection->GetUUID(),
				numzones
			);
		}
	);

	server_connection->OnConnectionIdentified(
		"Launcher", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo(
				"New Launcher connection from [{}] at [{}:{}]",
				connection->Handle()->RemoteIP(),
				connection->Handle()->RemotePort(),
				connection->GetUUID()
			);

			launcher_list.Add(connection);
		}
	);

	server_connection->OnConnectionRemoved(
		"Launcher", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo(
					"Removed Launcher connection from [{0}]",
				connection->GetUUID()
			);
				
			launcher_list.Remove(connection);
		}
	);

	server_connection->OnConnectionIdentified(
		"QueryServ", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo(
				"New Query Server connection from [{}] at [{}:{}]",
				connection->Handle()->RemoteIP(),
				connection->Handle()->RemotePort(),
				connection->GetUUID());

			QSLink.AddConnection(connection);
		}
	);

	server_connection->OnConnectionRemoved(
		"QueryServ", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo(
				"Removed Query Server connection from [{}]",
				connection->GetUUID()
			);

			QSLink.RemoveConnection(connection);
		}
	);

	server_connection->OnConnectionIdentified(
		"UCS", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo(
				"New UCS Server connection from [{}] at [{}:{}]",
				connection->Handle()->RemoteIP(),
				connection->Handle()->RemotePort(),
				connection->GetUUID()
			);

			UCSLink.SetConnection(connection);
		}
	);

	server_connection->OnConnectionRemoved(
		"UCS", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
			LogInfo("Connection lost from UCS Server [{}]", 
				connection->GetUUID());

			auto ucs_connection = UCSLink.GetConnection();
			if (ucs_connection->GetUUID() == connection->GetUUID()) {
				UCSLink.SetConnection(nullptr);
			}
		}
	);

	server_connection->OnConnectionIdentified(
		"WebInterface", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
		LogInfo(
			"New WebInterface Server connection from [{}] at [{}:{}]",
			connection->Handle()->RemoteIP(),
			connection->Handle()->RemotePort(),
			connection->GetUUID()
		);

		web_interface.AddConnection(connection);
	}
	);

	server_connection->OnConnectionRemoved(
		"WebInterface", [](std::shared_ptr<EQ::Net::ServertalkServerConnection> connection) {
		LogInfo(
			"Removed WebInterface Server connection from [{}]",
			connection->GetUUID()
		);

		web_interface.RemoveConnection(connection);
	}
	);



	if (eqsf.Open()) {
		LogInfo("Client (UDP) listener started.");
	} else {
		LogInfo("Failed to start client (UDP) listener (port 9000)");
		return 1;
	}

	//register all the patches we have avaliable with the stream identifier.
	EQStreamIdentifier stream_identifier;
	RegisterAllPatches(stream_identifier);
	zoneserver_list.shutdowntimer = new Timer(60000);
	zoneserver_list.shutdowntimer->Disable();
	zoneserver_list.reminder = new Timer(20000);
	zoneserver_list.reminder->Disable();
	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
	InterserverTimer.Trigger();
	uint8 ReconnectCounter = 100;
	std::shared_ptr<EQStream> eqs;
	std::shared_ptr<EQOldStream> eqos;
	EQStreamInterface *eqsi;

	auto loop_fn = [&](EQ::Timer* t) {
		Timer::SetCurrentTime();

		if (!RunLoops) {
			EQ::EventLoop::Get().Shutdown();
			return;
		}

		//give the stream identifier a chance to do its work....
		stream_identifier.Process();
		int i = 0;
		auto mSeconds = std::chrono::milliseconds(RuleI(Quarm, MaxTimeSpentProcessingConns));
		auto endTime = std::chrono::high_resolution_clock::now();
		//check the factory for any new incoming streams.
		// 
		//while (beginTime + mSeconds <= std::chrono::high_resolution_clock::now() && (eqs = eqsf.Pop())) {
		//	//pull the stream out of the factory and give it to the stream identifier
		//	//which will figure out what patch they are running, and set up the dynamic
		//	//structures and opcodes for that patch.
		//	struct in_addr	in{};
		//	in.s_addr = eqs->GetRemoteIP();
		//	LogInfo("New connection from {0}:{1}", inet_ntoa(in),ntohs(eqs->GetRemotePort()));
		//	stream_identifier.AddStream(eqs);	//takes the stream
		//}

		i = 0;
		endTime = std::chrono::high_resolution_clock::now() + std::chrono::duration_cast<std::chrono::nanoseconds>(mSeconds);
		//check the factory for any new incoming streams.
		while (std::chrono::high_resolution_clock::now() <= endTime && (eqos = eqsf.PopOld())) {
			//pull the stream out of the factory and give it to the stream identifier
			//which will figure out what patch they are running, and set up the dynamic
			//structures and opcodes for that patch.
			struct in_addr	in{};
			in.s_addr = eqos->GetRemoteIP();
			LogInfo("New connection from {0}:{1}", inet_ntoa(in), ntohs(eqos->GetRemotePort()));
			stream_identifier.AddOldStream(eqos);	//takes the stream
			i++;
		}

		//check the stream identifier for any now-identified streams
		while((eqsi = stream_identifier.PopIdentified())) {
			//now that we know what patch they are running, start up their client object
			struct in_addr	in{};
			in.s_addr = eqsi->GetRemoteIP();
			if (RuleB(World, UseBannedIPsTable)){ //Lieka: Check to see if we have the responsibility for blocking IPs.
				LogInfo("Checking inbound connection [{0}] against BannedIPs table", inet_ntoa(in));
				if (!database.CheckBannedIPs(inet_ntoa(in))){ //Lieka: Check inbound IP against banned IP table.
					LogInfo("Connection [{0}] PASSED banned IPs check. Processing connection.", inet_ntoa(in));
					auto client = new Client(eqsi);
					// @merth: client->zoneattempt=0;
					client_list.Add(client);
				} else {
					LogInfo("Connection from [{0}] FAILED banned IPs check. Closing connection.", inet_ntoa(in));
					eqsi->Close(); //Lieka: If the inbound IP is on the banned table, close the EQStream.
				}
			}
			if (!RuleB(World, UseBannedIPsTable)){
					LogRulesDetail("New connection from [{0}]:[{1}], processing connection", inet_ntoa(in), ntohs(eqsi->GetRemotePort()));
					auto client = new Client(eqsi);
					// @merth: client->zoneattempt=0;
					client_list.Add(client);
			}
		}

		event_scheduler.Process(&zoneserver_list);

		client_list.Process();
		
		if(EQTimeTimer.Check()) {
			TimeOfDay_Struct tod;
			zoneserver_list.worldclock.GetCurrentEQTimeOfDay(time(0), &tod);
			if (!database.SaveTime(tod.minute, tod.hour, tod.day, tod.month, tod.year)) {
				LogEqTime("Failed to save eqtime");
			}
			else {
				LogEqTimeDetail("EQTime successfully saved - time is now year [{}] month [{}] day [{}] hour [{}] minute [{}]",
					tod.year,
					tod.month,
					tod.day,
					tod.hour,
					tod.minute
				);
			}
		}

		if (RuleB(Quarm, EnableQuakes))
		{
			if (NextQuakeTimer.Check())
			{
				Log(Logs::Detail, Logs::WorldServer, "Triggered quake! %i", (next_quake.start_timestamp - Timer::GetTimeSeconds()));
				uint32 cur_time = Timer::GetTimeSeconds();
				database.SaveNextQuakeTime(next_quake);

				NextQuakeTimer.Enable();
				NextQuakeTimer.Start((next_quake.next_start_timestamp - cur_time) * 1000);

				std::string motd_str = "Welcome to Project Quarm! ";
				motd_str += "The '";
				motd_str += QuakeTypeToString(next_quake.quake_type);
				motd_str += "' earthquake ruleset is currently in effect in raid zones.";

				database.SetVariable("MOTD", motd_str.c_str());

				auto pack2 = new ServerPacket(ServerOP_Motd, sizeof(ServerMotd_Struct));
				auto mss = (ServerMotd_Struct*)pack2->pBuffer;
				strn0cpy(mss->myname, "Druzzil", sizeof(mss->myname));
				strn0cpy(mss->motd, motd_str.c_str(), sizeof(mss->motd));

				zoneserver_list.SendPacket(pack2);

				//Roleplay flavor text, go!
				zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Red, "Druzzil Ro's voice echoes in your mind, 'Beware, mortal. Creatures of legendary strength return to the world for a limited time.'");
				zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Yellow, "Druzzil Ro's projection alters time and space. Raid creatures have appeared in open world for a short time. Rule 9.x and Rule 10.x have been suspended in open world raid zones temporarily.");

				//Inform of imminent quake. This happens after the MOTD so zone denizens are informed again with relevant information.
				auto pack = new ServerPacket(ServerOP_QuakeImminent, sizeof(ServerEarthquakeImminent_Struct));
				ServerEarthquakeImminent_Struct* seis = (ServerEarthquakeImminent_Struct*)pack->pBuffer;
				seis->quake_type = next_quake.quake_type;
				seis->next_start_timestamp = next_quake.next_start_timestamp;
				seis->start_timestamp = next_quake.start_timestamp;
				zoneserver_list.SendPacket(pack);

				safe_delete(pack2);
				safe_delete(pack);

				//Timer needs to be set to enforce MOTD rules.
				DisableQuakeTimer.Enable();
				DisableQuakeTimer.Start(((next_quake.start_timestamp - cur_time) + RuleI(Quarm, QuakeEndTimeDuration)) * 1000);
			}

			if (DisableQuakeTimer.Check())
			{
				std::string motd_str = "Welcome to Project Quarm! ";
				motd_str += "The standard ruleset is currently in effect.";
				database.SetVariable("MOTD", motd_str.c_str());

				auto pack3 = new ServerPacket(ServerOP_Motd, sizeof(ServerMotd_Struct));
				auto mss = (ServerMotd_Struct*)pack3->pBuffer;
				strn0cpy(mss->myname, "Druzzil", sizeof(mss->myname));
				strn0cpy(mss->motd, motd_str.c_str(), sizeof(mss->motd));
				zoneserver_list.SendPacket(pack3);
				safe_delete(pack3);


				//Inform of imminent quake. This happens after the MOTD so zone denizens are informed again with relevant information.
				auto pack4 = new ServerPacket(ServerOP_QuakeEnded, sizeof(ServerEarthquakeImminent_Struct));
				ServerEarthquakeImminent_Struct* seis = (ServerEarthquakeImminent_Struct*)pack4->pBuffer;
				next_quake.quake_type = QuakeType::QuakeDisabled;
				seis->quake_type = next_quake.quake_type;
				seis->next_start_timestamp = next_quake.next_start_timestamp;
				seis->start_timestamp = next_quake.start_timestamp;
				zoneserver_list.SendPacket(pack4);
				safe_delete(pack4);

				//MOTD has been set. Roleplay flavor text, go!
				zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Red, "Druzzil Ro's voice echoes in your mind, 'It seems as though the mortals have had enough of my games...'");
				zoneserver_list.SendEmoteMessage(0, 0, AccountStatus::Player, Chat::Yellow, "Druzzil Ro's grasp no longer archors this land... for now. The Earthquake has ended, and Rules 9.x and 10.x once again apply.");

				//We're no longer using the timer; we've done our job. The next quake will enable it again.
				DisableQuakeTimer.Disable();
			}
		}

		//check for timeouts in other threads
		timeout_manager.CheckTimeouts();
		zoneserver_list.Process();
		launcher_list.Process();

		//WILink.Process();

		if (InterserverTimer.Check()) {
			InterserverTimer.Start();
			database.ping();
			// AsyncLoadVariables(dbasync, &database);
			ReconnectCounter++;
			if (ReconnectCounter >= 12) { // only create thread to reconnect every 10 minutes. previously we were creating a new thread every 10 seconds
				ReconnectCounter = 0;
			}
		}
	};

	EQ::Timer process_timer(loop_fn);
	process_timer.Start(32, true);

	EQ::EventLoop::Get().Run();

	LogInfo("World main loop completed.");
	LogInfo("Shutting down zone connections (if any).");
	zoneserver_list.KillAll();
	LogInfo("Zone (TCP) listener stopped.");
	LogInfo("Client (UDP) listener stopped.");
	eqsf.Close();
	LogSys.CloseFileLogs();

	return 0;
}

void CatchSignal(int sig_num) {
	Log(Logs::General, Logs::WorldServer,"Caught signal %d",sig_num);
	RunLoops = false;
}

void UpdateWindowTitle(char* iNewTitle) {
#ifdef _WINDOWS
	char tmp[500];
	if (iNewTitle) {
		snprintf(tmp, sizeof(tmp), "World: %s", iNewTitle);
	}
	else {
		snprintf(tmp, sizeof(tmp), "World");
	}
	SetConsoleTitle(tmp);
#endif
}

