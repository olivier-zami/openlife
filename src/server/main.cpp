//
// Created by olivier on 07/08/2021.
//

#include "main.h"
#include <iostream>
#include <SDL/SDL.h>
#include "server.h"
#include "src/system/_base/log.h"

#include "src/common/system.h"
#include "src/system/_extension/nlohmann.h"


#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/network/SocketPoll.h"
#include "src/common/object/entity/mapZone.h"
#include "src/server/bank/worldMap.h"
#include "src/common/type/database/lineardb3.h"
#include "src/system/_base/object/entity/exception.h"
#include "src/system/_base/object/store/memory/random/biome.h"

/**********************************************************************************************************************/
#include "minorGems/util/SettingsManager.h"
#include "OneLife/server/specialBiomes.h"
#include "OneLife/server/triggers.h"
#include "OneLife/server/familySkipList.h"
#include "OneLife/server/language.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include "src/server/main.h"
#include "src/server/process/message/getMapChunk.h"
#include "minorGems/io/file/Directory.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "minorGems/system/Thread.h"
#include "minorGems/system/Time.h"
#include "minorGems/game/doublePair.h"
#include "src/third_party/jason_rohrer/minorGems/util/log/AppLog.h"
#include "src/third_party/jason_rohrer/minorGems/util/log/FileLog.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/io/file/File.h"
#include "OneLife/server/map.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/objectMetadata.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/commonSource/sayLimit.h"
#include "OneLife/server/lifeLog.h"
#include "OneLife/server/foodLog.h"
#include "OneLife/server/backup.h"
#include "OneLife/server/triggers.h"
#include "OneLife/server/playerStats.h"
#include "OneLife/server/lineageLog.h"
#include "OneLife/server/serverCalls.h"
#include "OneLife/server/failureLog.h"
#include "OneLife/server/names.h"
#include "OneLife/server/curses.h"
#include "OneLife/server/lineageLimit.h"
#include "OneLife/server/objectSurvey.h"
#include "OneLife/server/language.h"
#include "OneLife/server/familySkipList.h"
#include "OneLife/server/lifeTokens.h"
#include "OneLife/server/fitnessScore.h"
#include "OneLife/server/arcReport.h"
#include "OneLife/server/curseDB.h"
#include "OneLife/server/cravings.h"
#include "OneLife/server/offspringTracker.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "src/server/type/entities.h"
#include "src/server/server.h"
#include "OneLife/gameSource/GridPos.h"
#include "minorGems/util/crc32.h"
// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "OneLife/gameSource/spriteBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/server/spiral.h"
/**********************************************************************************************************************/

openLife::Server* server;

volatile char quit = false;
float targetHeat = 10;
double secondsPerYear = 60.0;
int minPickupBabyAge = 10;
int babyAge = 5;
int defaultActionAge = 3;// age when bare-hand actions become available to a baby (opening doors, etc.)
double startWalkingAge = 0.20;// can't walk for first 12 seconds
double forceDeathAge = 60;
double minSayGapInSeconds = 1.0;
double emoteWindowSeconds = 60.0;// for emote throttling
int maxEmotesInWindow = 10;
double emoteCooldownSeconds = 120.0;
int maxLineageTracked = 1024;// each generation is at minimum 14 minutes apart // so 1024 generations is approximately 10 days
int apocalypsePossible = 0;
char apocalypseTriggered = false;
char apocalypseRemote = false;
GridPos apocalypseLocation = { 0, 0 };
int lastApocalypseNumber = 0;
double apocalypseStartTime = 0;
char apocalypseStarted = false;
char postApocalypseStarted = false;
double remoteApocalypseCheckInterval = 30;
double lastRemoteApocalypseCheckTime = 0;
WebRequest *apocalypseRequest = NULL;
char monumentCallPending = false;
int monumentCallX = 0;
int monumentCallY = 0;
int monumentCallID = 0;
char *curseYouPhrase = NULL;
char *curseBabyPhrase = NULL;
int nextID = 2;
int numConnections = 0;
int longestShutdownLine = -1;

double lastPastPlayerFlushTime;
double deadlyMapSpotTimeoutSec = 10;
double minFlightSpeed; // min speed for takeoff
double playerCrossingCheckStepTime;// how often do we check what a player is standing on top of for attack effects?
double periodicStepTime;// for steps in main loop that shouldn't happen every loop// (loop runs faster or slower depending on how many messages are incoming)
double lastPeriodicStepTime;
int numPlayersRecomputeHeatPerStep;
int lastPlayerIndexHeatRecomputed;
double lastHeatUpdateTime;// recompute heat for fixed number of players per timestep
double heatUpdateTimeStep;
double heatUpdateSeconds;// how often the player's personal heat advances toward their environmental// heat value
float rAir = 0.04;// air itself offers some insulation// a vacuum panel has R-value that is 25x greater than air
char eveWindowOver = false;


SimpleVector<float> recentScoresForPickingEve;
SimpleVector<char *>recentScoreEmailsForPickingEve;
SimpleVector<FreshConnection> newConnections;
SimpleVector<FreshConnection> waitingForTwinConnections;
SimpleVector<LiveObject> players;
SimpleVector<LiveObject> tutorialLoadingPlayers;
SimpleVector<DeadObject> pastPlayers;
SimpleVector<GridPos> newOwnerPos;
SimpleVector<GridPos> recentlyRemovedOwnerPos;
SimpleVector<WarPeaceMessageRecord> warPeaceRecords;
SimpleVector<int> killStatePosseChangedPlayerIDs;
SimpleVector<ChangePosition> newSpeechPos;
SimpleVector<char*> newSpeechPhrases;
SimpleVector<int> newSpeechPlayerIDs;
SimpleVector<char> newSpeechCurseFlags;
SimpleVector<char*> newLocationSpeech;
SimpleVector<ChangePosition> newLocationSpeechPos;
JenkinsRandomSource curseSource;
char allowSecondPlaceBiomes = false;
char anyBiomesInDB = false;//legacy: static char anyBiomesInDB = false;
int maxBiomeXLoc = -2000000000;//legacy: static int maxBiomeXLoc = -2000000000;
int maxBiomeYLoc = -2000000000;//legacy: static int maxBiomeYLoc = -2000000000;
int minBiomeXLoc = 2000000000;//legacy: static int minBiomeXLoc = 2000000000;
int minBiomeYLoc = 2000000000;//legacy: static int minBiomeYLoc = 2000000000;
char lookTimeDBEmpty = false;
char skipLookTimeCleanup = 0;
// if lookTimeDBEmpty, then we init all map cell look times to NOW
int cellsLookedAtToInit = 0;
int specialBiomeBandMode;
int numSpecialBiomes;
LINEARDB3 biomeDB;
char biomeDBOpen = false;
int regularBiomeLimit;
float *biomeCumuWeights;
float biomeTotalWeight;
int numBiomes;
unsigned int biomeRandSeedA = 727;
unsigned int biomeRandSeedB = 941;
openLife::system::type::Value2D_U32 mapGenSeed;
int maxSpeechPipeIndex = 0;

double minFoodDecrementSeconds = 5.0;
double maxFoodDecrementSeconds = 20;
double foodScaleFactor = 1.0;
double foodScaleFactorFloor = 0.5;
double foodScaleFactorHalfLife = 50;
double foodScaleFactorGamma = 1.5;
double newPlayerFoodDecrementSecondsBonus = 8;
int newPlayerFoodEatingBonus = 5;
double newPlayerFoodBonusHalfLifeSeconds = 36000;// first 10 hours of living
double indoorFoodDecrementSecondsBonus = 20.0;
int babyBirthFoodDecrement = 10;
int babyFeedingLevel = 2;
int nurseCost = 1;// fixed cost to pick up baby// this still encourages baby-parent// communication so as not// to get the most mileage out of// food
int eatBonus = 0;// bonus applied to all foods// makes whole server a bit easier (or harder, if negative)
double eatBonusFloor = 0;
double eatBonusHalfLife = 50;
double eatCostMax = 5;
double eatCostGrowthRate = 0.1;
int canYumChainBreak = 0;// -1 for no cap
int yumBonusCap = -1;
double minAgeForCravings = 10;
double posseSizeSpeedMultipliers[4] = { 0.75, 1.25, 1.5, 2.0 };
double killerVulnerableSeconds = 60;
int minActivePlayersForLanguages = 15;
unsigned int nextSequenceNumber = 1;// keep a running sequence number to challenge each connecting client// to produce new login hashes, avoiding replay attacks.
int requireClientPassword = 1;
int requireTicketServerCheck = 1;
char *clientPassword = NULL;
char *ticketServerURL = NULL;
char *reflectorURL = NULL;
int versionNumber = 1;// larger of dataVersionNumber.txt or serverCodeVersionNumber.txt
double childSameRaceLikelihood = 0.9;
int familySpan = 2;
SimpleVector<char*> nameGivingPhrases;// phrases that trigger baby and family naming
SimpleVector<char*> familyNameGivingPhrases;
SimpleVector<char*> eveNameGivingPhrases;
SimpleVector<char*> cursingPhrases;
SimpleVector<char*> forgivingPhrases;
SimpleVector<char*> youForgivingPhrases;
SimpleVector<char*> youGivingPhrases;
SimpleVector<char*> namedGivingPhrases;
SimpleVector<char*> familyGivingPhrases;
SimpleVector<char*> offspringGivingPhrases;
SimpleVector<char*> posseJoiningPhrases;
SimpleVector<char*> youFollowPhrases;
SimpleVector<char*> namedFollowPhrases;
SimpleVector<char*> youExilePhrases;
SimpleVector<char*> namedExilePhrases;
SimpleVector<char*> youRedeemPhrases;
SimpleVector<char*> namedRedeemPhrases;
SimpleVector<char*> youKillPhrases;
SimpleVector<char*> namedKillPhrases;
SimpleVector<char*> namedAfterKillPhrases;
SimpleVector<WarState> warStates;
char *orderPhrase = NULL;
char *eveName = NULL;
char allowedSayCharMap[256];// maps extended ascii codes to true/false for characters allowed in SAY// messages
const char *allowedSayChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! ";
int killEmotionIndex = 2;
int victimEmotionIndex = 2;
int victimTerrifiedEmotionIndex = 2;
int starvingEmotionIndex = 2;
int satisfiedEmotionIndex = 2;
double lastBabyPassedThresholdTime = 0;
int recentScoreWindowForPickingEve = 10;
double eveWindowStart = 0;
FILE *familyDataLogFile = NULL;
JenkinsRandomSource randSource;
SimpleVector<PeaceTreaty> peaceTreaties;
double minGlobalMessageSpacingSeconds = 7;// messages with no follow-up hang out on client for 10 seconds// 7 seconds should be long enough to read if there's a follow-up waiting
SimpleVector<DeadlyMapSpot> deadlyMapSpots;
double diagLength = 1.41421356237;// diags are square root of 2 in length
SimpleVector<char *> curseWords;
char *curseSecret = NULL;
SimpleVector<char *> familyNamesAfterEveWindow;
SimpleVector<int> familyLineageEveIDsAfterEveWindow;
SimpleVector<int> familyCountsAfterEveWindow;
int nextBabyFamilyIndex = 0;
FILE *postWindowFamilyLogFile = NULL;
double shortLifeAge = 10;// keep track of players whose last life was short
SimpleVector<char*> shortLifeEmails;
double minPosseFraction = 0.5;
int minPosseCap = 3;
double possePopulationRadius = 30;
int cursesUseSenderEmail = 0;
int useCurseWords = 1;
int pathDeltaMax = 16;
double baseWalkSpeed = 3.75;// with 128-wide tiles, character moves at 480 pixels per second// at 60 fps, this is 8 pixels per frame// important that it's a whole number of pixels for smooth camera movement
int chunkDimensionX = 32;
int chunkDimensionY = 30;
int maxSpeechRadius = 16;
SocketPoll sockPoll;
int orderDistance = 10;
double killDelayTime = 12.0;
double posseDelayReductionFactor = 2.0;
int maxPlacementX = 5000000;// for placement of tutorials out of the way
int tutorialOffsetX = 400000;// tutorial is alwasy placed 400,000 to East of furthest birth/Eve// location
int tutorialCount = 0;// each subsequent tutorial gets put in a diferent place
SimpleVector<char*> tempTwinEmails;// fill this with emails that should also affect lineage ban// if any twin in group is banned, all should be
char nextLogInTwin = false;
int firstTwinID = -1;
SimpleVector<GraveInfo> newGraves;
SimpleVector<GraveMoveInfo> newGraveMoves;
int usePersonalCurses = 0;

#include "src/server/definition/services.h"

#define REL_PATH_CONFIG_MAP "../../conf/server/map.json"

SocketServer *socketServer;
int port;
openLife::system::object::store::device::random::LinearDB *newBiomeDB;

int main()
{
	if(!openLife::system::isFileWritable())
	{
		openLife::system::notice("File system read-only.  Server exiting.");
		return 1;
	}

	openLife::system::notice("Attempt to start the server ...");

	nlohmann::json dataSettingsServer = openLife::system::nlohmann::getJsonFromFile("../../conf/server.json");
	nlohmann::json dataClimate = openLife::system::nlohmann::getJsonFromFile("../../conf/entity/climate.json");

	nlohmann::json dataWorldMap = openLife::system::nlohmann::getJsonFromFile(REL_PATH_CONFIG_MAP);

	try
	{
		/**
		 * InitMap() : server/map.cpp => l2879
		 **/

		//!set worldMap
		openLife::server::settings::WorldMap worldMapSettings;
		worldMapSettings.specialBiomeBandMode = 1;
		worldMapSettings.filename = "biome.db";
		worldMapSettings.mapSize.width = 10;
		worldMapSettings.mapSize.height = 10;
		worldMapSettings.map.seed.x = dataWorldMap["mapGenerator"]["seed"]["x"];
		worldMapSettings.map.seed.y = dataWorldMap["mapGenerator"]["seed"]["y"];
		std::cout << "\nworldMapSettings.climate.capacity()" << worldMapSettings.climate.capacity();
		worldMapSettings.climate.reserve(dataClimate.size());
		std::cout << "\nworldMapSettings.climate.capacity()" << worldMapSettings.climate.capacity();
		std::cout << "\n";
		std::cout << "\nworldMapSettings.climate.size() : " << worldMapSettings.climate.size();
		for(unsigned int i=0; i<dataClimate.size(); i++)
		{
			openLife::server::type::entity::Climate climate;
			std::cout << "\nsetting " << dataClimate[i]["label"] << " climate";
			climate.label = dataClimate[i]["label"].get<std::string>();
			worldMapSettings.climate.push_back(climate);
		}

		worldMapSettings.map.specialBiomeBandThickness = 300;

		for(unsigned int i=0; i<dataWorldMap["biomeOrder"].size(); i++)
		{
			worldMapSettings.biome.order.push_back(dataWorldMap["biomeOrder"][i].get<int>());
		}
		std::cout << "\nSetting biome order size("<<worldMapSettings.biome.order.size()<<") [";
		for(unsigned int i=0; i<worldMapSettings.biome.order.size(); i++) std::cout << "  " << worldMapSettings.biome.order[i];
		std::cout << "]";


		for(unsigned int i=0; i<dataWorldMap["specialBiomeBandYCenter"].size(); i++)
		{
			worldMapSettings.map.specialBiomeBandYCenter.push_back(dataWorldMap["specialBiomeBandYCenter"][i].get<int>());
		}
		std::cout << "\nSetting specialBiomeBandYCenter size("<<worldMapSettings.map.specialBiomeBandYCenter.size()<<") [";
		for(unsigned int i=0; i<worldMapSettings.map.specialBiomeBandYCenter.size(); i++) std::cout << "  " << worldMapSettings.map.specialBiomeBandYCenter[i];
		std::cout << "]";


		for(unsigned int i=0; i<dataWorldMap["specialBiomeBandOrder"].size(); i++)
		{
			worldMapSettings.map.specialBiomeBandOrder.push_back(dataWorldMap["specialBiomeBandOrder"][i].get<int>());
		}
		std::cout << "\nSetting specialBiomeBandOrder size("<<worldMapSettings.map.specialBiomeBandOrder.size()<<") [";
		for(unsigned int i=0; i<worldMapSettings.map.specialBiomeBandOrder.size(); i++) std::cout << "  " << worldMapSettings.map.specialBiomeBandOrder[i];
		std::cout << "]";

		for(unsigned int i=0; i<dataWorldMap["specialBiomes"].size(); i++)
		{
			worldMapSettings.map.specialBiomes.push_back(dataWorldMap["specialBiomes"][i].get<int>());
		}
		std::cout << "\nSetting specialBiomes size("<<worldMapSettings.map.specialBiomes.size()<<") [";
		for(unsigned int i=0; i<worldMapSettings.map.specialBiomes.size(); i++) std::cout << "  " << worldMapSettings.map.specialBiomes[i];
		std::cout << "]";

		for(unsigned int i=0; i<dataWorldMap["biomeWeight"].size(); i++)
		{
			worldMapSettings.map.biomeWeight.push_back(dataWorldMap["biomeWeight"][i].get<float>());
		}

		std::cout << "\nSetting biomeWeight size("<<worldMapSettings.map.biomeWeight.size()<<") [";
		for(unsigned int i=0; i<worldMapSettings.map.biomeWeight.size(); i++) std::cout << "  " <<worldMapSettings.map.biomeWeight[i];
		std::cout << "]";

		openLife::system::Log::trace("all biome should be set ...");
		for(unsigned int i=0; i<dataSettingsServer["mapping"]["biome"].size(); i++)
		{
			openLife::server::type::entity::Biome biome;
			biome.label = dataSettingsServer["mapping"]["biome"][i]["label"];
			biome.code = dataSettingsServer["mapping"]["biome"][i]["code"];
			biome.value = dataSettingsServer["mapping"]["biome"][i]["value"];
			worldMapSettings.biome1.push_back(biome);//TODO: rename biome1 to biome after worldMapSetting modification
		}

		//!new
		openLife::server::Settings serverSettings;

		for(unsigned int i=0; i<dataSettingsServer["map"]["relief"].size();i++)
		{
			serverSettings.map.relief.push_back(dataSettingsServer["map"]["relief"][i].get<unsigned int>());
		}

		serverSettings.mapGenerator.type = dataSettingsServer["mapGenerator"]["type"];
		serverSettings.mapGenerator.sketch.filename = dataSettingsServer["mapGenerator"]["sketch"]["filename"];

		char someClientMessageReceived = false;
		serverSettings.global.someClientMessageReceived = &someClientMessageReceived;

		int shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
		serverSettings.global.shutdownMode = &shutdownMode;

		int forceShutdownMode = SettingsManager::getIntSetting( "forceShutdownMode", 0 );
		serverSettings.global.forceShutdownMode = &forceShutdownMode;

		server = new openLife::Server(serverSettings, worldMapSettings, &biomeDB, &anyBiomesInDB);


		/*//TODO: segfault check return value of server->getWorldMap()->getBiomes()
		std::vector<int> testBiome = server->getWorldMap()->getBiomes();
		std::cout << "\n==========> Biomes : [";for(unsigned int i=0; i<testBiome.size();i++){std::cout << " " << testBiome[i];}std::cout << " ]";
		*/

		//!test
		/*
		const char* someJson = "{\"title\":\"document title\"}";
		nlohmann::json config = nlohmann::json::parse(someJson);
		SimpleVector<int> *testBiomeOrderList = SettingsManager::getIntSettingMulti( "biomeOrder" );
		biomes = testBiomeOrderList->getElementArray();
		openLife::system::type::entity::Biome testedBiome;
		testedBiome = worldMap->select(0, 0)->getBiomeRecord();
		std::cout << "\n################### Test ####### => Biome ("<<testedBiome.x<<", "<<testedBiome.y<<")" << " value = " << testedBiome.value;
		std::cout << "\n################### Test ####### => BiomeLine [";
		for(int i=-12; i<=0; i++)
		{
			testedBiome = worldMap->select(i, 0)->getBiomeRecord();
			std::cout << "("<<i<<", 0):"<<  testedBiome.value << " ";
		}
		std::cout << "]";
		std::cout << "\n################### Test ####### => Config title " << config["title"];
		*/

		//!net map biome around spawning zone
		/*
		common::object::entity::MapZone* mapZone;
		mapZone = common::process::convert::image::getMapZoneFromBitmap("/home/olivier/Projets/OpenLife/data/images/maps/mini_map.bmp");
		for(long unsigned int i=0; i<mapZone->getSize(); i++)
		{
			switch(mapZone->p(i))
			{

				case 16777215:	mapZone->p(i) = 6; break;//polar
				//case 255:		mapZone->p(i) = -1; break;//water
				case 8355711:	mapZone->p(i) = 3; break;//montain/taiga/toundra
				case 65280:		mapZone->p(i) = 1; break;//grassland
				case 32639:		mapZone->p(i) = 0; break;//swamp
				case 16744192:	mapZone->p(i) = 2; break;//savannah mediterranean
				case 16776960:	mapZone->p(i) = 5; break;//desert
				case 32512:		mapZone->p(i) = 4; break;//jungle
				default:
					mapZone->p(i) = 1;
					break;
			}
		}
		*******/

		//!old
		//feature family stuff
		familyDataLogFile = fopen( "familyDataLog.txt", "a" );
		if( familyDataLogFile != NULL ) {
			fprintf( familyDataLogFile, "%.2f server starting up\n",
					 Time::getCurrentTime() );
		}


		//feature allowedSayChars
		memset( allowedSayCharMap, false, 256 );

		int numAllowed = strlen( allowedSayChars );
		for( int i=0; i<numAllowed; i++ ) {
			allowedSayCharMap[ (int)( allowedSayChars[i] ) ] = true;
		}

		nextID = SettingsManager::getIntSetting( "nextPlayerID", 2 );

		// make backup and delete old backup every day
		AppLog::setLog( new FileLog( "log.txt", 86400 ) );

		AppLog::setLoggingLevel( Log::DETAIL_LEVEL );
		AppLog::printAllMessages( true );

		printf( "\n" );
		AppLog::info( "Server starting up" );

		printf( "\n" );

		nextSequenceNumber = SettingsManager::getIntSetting( "sequenceNumber", 1 );
		requireClientPassword = SettingsManager::getIntSetting( "requireClientPassword", 1 );
		requireTicketServerCheck = SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );
		clientPassword = SettingsManager::getStringSetting( "clientPassword" );

		int dataVer = readIntFromFile( "dataVersionNumber.txt", 1 );
		int codVer = readIntFromFile( "serverCodeVersionNumber.txt", 1 );

		versionNumber = dataVer;
		if( codVer > versionNumber ) {
			versionNumber = codVer;
		}

		printf( "\n" );
		AppLog::infoF( "Server using version number %d", versionNumber );

		printf( "\n" );




		minFoodDecrementSeconds =
				SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );

		maxFoodDecrementSeconds =
				SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

		babyBirthFoodDecrement =
				SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );

		indoorFoodDecrementSecondsBonus = SettingsManager::getFloatSetting(
				"indoorFoodDecrementSecondsBonus", 20 );



		secondsPerYear = SettingsManager::getFloatSetting( "secondsPerYear", 60.0f );


		if( clientPassword == NULL ) {
			requireClientPassword = 0;
		}


		ticketServerURL =
				SettingsManager::getStringSetting( "ticketServerURL" );


		if( ticketServerURL == NULL ) {
			requireTicketServerCheck = 0;
		}


		reflectorURL = SettingsManager::getStringSetting( "reflectorURL" );

		apocalypsePossible =
				SettingsManager::getIntSetting( "apocalypsePossible", 0 );

		lastApocalypseNumber =
				SettingsManager::getIntSetting( "lastApocalypseNumber", 0 );


		childSameRaceLikelihood =
				(double)SettingsManager::getFloatSetting( "childSameRaceLikelihood",
														  0.90 );

		familySpan =
				SettingsManager::getIntSetting( "familySpan", 2 );

		eveName =
				SettingsManager::getStringSetting( "eveName", "EVE" );


		readPhrases( "babyNamingPhrases", &nameGivingPhrases );
		readPhrases( "familyNamingPhrases", &familyNameGivingPhrases );

		readPhrases( "babyNamingPhrases", &eveNameGivingPhrases );

		// add YOU ARE EVE SMITH versions of these
		// put them in front
		SimpleVector<char*> oldPhrases( eveNameGivingPhrases.size() * 2 );
		oldPhrases.push_back_other( &eveNameGivingPhrases );
		eveNameGivingPhrases.deleteAll();
		int numEvePhrases = oldPhrases.size();
		for( int i=0; i<numEvePhrases; i++ ) {
			char *phrase = oldPhrases.getElementDirect( i );

			char *newPhrase = autoSprintf( "%s %s", phrase, eveName );
			eveNameGivingPhrases.push_back( newPhrase );
		}
		eveNameGivingPhrases.push_back_other( &oldPhrases );


		readPhrases( "cursingPhrases", &cursingPhrases );

		readPhrases( "forgivingPhrases", &forgivingPhrases );
		readPhrases( "forgiveYouPhrases", &youForgivingPhrases );


		readPhrases( "youGivingPhrases", &youGivingPhrases );
		readPhrases( "namedGivingPhrases", &namedGivingPhrases );

		readPhrases( "familyGivingPhrases", &familyGivingPhrases );
		readPhrases( "offspringGivingPhrases", &offspringGivingPhrases );


		readPhrases( "posseJoiningPhrases", &posseJoiningPhrases );


		readPhrases( "youFollowPhrases", &youFollowPhrases );
		readPhrases( "namedFollowPhrases", &namedFollowPhrases );

		readPhrases( "youExilePhrases", &youExilePhrases );
		readPhrases( "namedExilePhrases", &namedExilePhrases );

		readPhrases( "youRedeemPhrases", &youRedeemPhrases );
		readPhrases( "namedRedeemPhrases", &namedRedeemPhrases );


		readPhrases( "youKillPhrases", &youKillPhrases );
		readPhrases( "namedKillPhrases", &namedKillPhrases );
		readPhrases( "namedAfterKillPhrases", &namedAfterKillPhrases );


		orderPhrase =
				SettingsManager::getSettingContents( "orderPhrase",
													 "ORDER," );

		orderDistance =
				SettingsManager::getIntSetting( "orderDistance", 10 );


		curseYouPhrase =
				SettingsManager::getSettingContents( "curseYouPhrase",
													 "CURSE YOU" );

		curseBabyPhrase =
				SettingsManager::getSettingContents( "curseBabyPhrase",
													 "CURSE MY BABY" );




		killEmotionIndex =
				SettingsManager::getIntSetting( "killEmotionIndex", 2 );

		victimEmotionIndex =
				SettingsManager::getIntSetting( "victimEmotionIndex", 2 );

		victimTerrifiedEmotionIndex =
				SettingsManager::getIntSetting( "victimTerrifiedEmotionIndex", 2 );


		starvingEmotionIndex =
				SettingsManager::getIntSetting( "starvingEmotionIndex", 2 );

		satisfiedEmotionIndex =
				SettingsManager::getIntSetting( "satisfiedEmotionIndex", 2 );


		FILE *f = fopen( "curseWordList.txt", "r" );

		if( f != NULL ) {

			int numRead = 1;

			char buff[100];

			while( numRead == 1 ) {
				numRead = fscanf( f, "%99s", buff );

				if( numRead == 1 ) {
					if( strlen( buff ) < 6 ) {
						// short words only, 3, 4, 5 letters
						curseWords.push_back( stringToUpperCase( buff ) );
					}
				}
			}
			fclose( f );
		}
		printf( "Curse word list has %d words\n", curseWords.size() );


	#ifdef WIN_32
		printf( "\n\nPress CTRL-C to shut down server gracefully\n\n" );

		SetConsoleCtrlHandler( ctrlHandler, TRUE );
	#else
		printf( "\n\nPress CTRL-Z to shut down server gracefully\n\n" );

		signal( SIGTSTP, intHandler );
	#endif

		initNames();

		initCurses();

		initLifeTokens();

		initFitnessScore();


		initLifeLog();
		//initBackup();

		initPlayerStats();
		initLineageLog();

		initLineageLimit();

		initCurseDB();

		initOffspringTracker();



		char rebuilding;

		initAnimationBankStart( &rebuilding );
		while( initAnimationBankStep() < 1.0 );
		initAnimationBankFinish();


		initObjectBankStart( &rebuilding, true, true );
		while( initObjectBankStep() < 1.0 );
		initObjectBankFinish();


		initCategoryBankStart( &rebuilding );
		while( initCategoryBankStep() < 1.0 );
		initCategoryBankFinish();


		// auto-generate category-based transitions
		initTransBankStart( &rebuilding, true, true, true, true );
		while( initTransBankStep() < 1.0 );
		initTransBankFinish();


		// defaults to one hour
		int epochSeconds =
				SettingsManager::getIntSetting( "epochSeconds", 3600 );

		setTransitionEpoch( epochSeconds );


		initFoodLog();
		initFailureLog();

		initObjectSurvey();

		initLanguage();
		initFamilySkipList();


		initTriggers();

		initSpecialBiomes();

		// if we received one the last time we looped, don't sleep when
		// polling for socket being ready, because there could be more data
		// waiting in the buffer for a given socket


		if( initMap() != true )
		{
			// cannot continue after map init fails
			return 1;
		}



		if( false )
		{

			printf( "Running map sampling\n" );

			int idA = 290;
			int idB = 942;

			int totalCountA = 0;
			int totalCountB = 0;
			int numRuns = 2;

			for( int i=0; i<numRuns; i++ ) {


				int countA = 0;
				int countB = 0;

				int x = randSource.getRandomBoundedInt( 10000, 300000 );
				int y = randSource.getRandomBoundedInt( 10000, 300000 );

				printf( "Sampling at %d,%d\n", x, y );


				for( int yd=y; yd<y + 2400; yd++ ) {
					for( int xd=x; xd<x + 2400; xd++ ) {
						int oID = getMapObject( xd, yd );

						if( oID == idA ) {
							countA ++;
						}
						else if( oID == idB ) {
							countB ++;
						}
					}
				}
				printf( "   Count at %d,%d is %d = %d, %d = %d\n",
						x, y, idA, countA, idB, countB );

				totalCountA += countA;
				totalCountB += countB;
			}
			printf( "Average count %d (%s) = %f,  %d (%s) = %f  over %d runs\n",
					idA, getObject( idA )->description,
					totalCountA / (double)numRuns,
					idB, getObject( idB )->description,
					totalCountB / (double)numRuns,
					numRuns );
			printf( "Press ENTER to continue:\n" );

			int readInt;
			scanf( "%d", &readInt );
		}




		port = SettingsManager::getIntSetting( "port", 5077 );



		socketServer = new SocketServer( port, 256 );

		sockPoll.addSocketServer( socketServer );

		AppLog::infoF( "Listening for connection on port %d", port );

		server->start();

		//!tests
		openLife::system::Log::trace("Trying to clean @ position (1,1)");
		server->getWorldMap()->select(1,1)->reset();




		// test code for printing sample eve locations
		// direct output from server to out.txt
		// then run:
		// grep "Eve location" out.txt | sed -e "s/Eve location //" |
		//      sed -e "s/,/ /" > eveTest.txt
		// Then in gnuplot, do:
		//  plot "eveTest.txt" using 1:2 with linespoints;

		/*
		for( int i=0; i<1000; i++ ) {
			int x, y;

			SimpleVector<GridPos> temp;

			getEvePosition( "test@blah", 1, &x, &y, &temp, false );

			printf( "Eve location %d,%d\n", x, y );
			}
		*/


		// stop listening on server socket immediately, before running
		// cleanup steps.  Cleanup may take a while, and we don't want to leave
		// server socket listening, because it will answer reflector and player
		// connection requests but then just hang there.

		// Closing the server socket makes these connection requests fail
		// instantly (instead of relying on client timeouts).
		delete socketServer;

		quitCleanup();


		AppLog::info( "Done." );
	}
	catch(openLife::system::object::entity::Exception* e)
	{
		std::cout << "\nException occurred : " << e->getMessage();
	}

	return 0;
}


