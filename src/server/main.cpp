//
// Created by olivier on 07/08/2021.
//

#include "main.h"
#include <iostream>
#include <SDL/SDL.h>
#include "server.h"
#include "src/common/system.h"
#include "src/system/_extension/nlohmann.h"

#include "src/common/object/entity/mapZone.h"
#include "src/server/service/database/worldMap.h"
#include "src/system/_base/settings/linearDB.h"
#include "src/common/type/database/lineardb3.h"
#include "src/system/_base/object/entity/exception.h"
#include "src/system/_base/object/store/memory/random/biome.h"
#include "minorGems/util/SettingsManager.h"

openLife::Server* server;

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
openLife::system::object::store::memory::random::Biome* cachedBiome;
openLife::system::type::Value2D_U32 mapGenSeed;
int maxSpeechPipeIndex = 0;

#include "src/server/definition/services.h"

#define REL_PATH_CONFIG_MAP "../../conf/server/map.json"


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

		//!
		openLife::system::settings::LinearDB biomeDBSettings;
		newBiomeDB = new openLife::system::object::store::device::random::LinearDB(biomeDBSettings);
		newBiomeDB->init(&biomeDB);
		cachedBiome = new openLife::system::object::store::memory::random::Biome(BIOME_CACHE_SIZE);

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

		std::cout << "\nall biome should be set ...";
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
		server = new openLife::Server(serverSettings, worldMapSettings, &biomeDB, &anyBiomesInDB, cachedBiome);

		std::vector<int> testBiome = server->getWorldMap()->getBiomes();
		//std::cout << "\n==========> Biomes : [";for(unsigned int i=0; i<testBiome.size();i++){std::cout << " " << testBiome[i];}std::cout << " ]";

		//!test
		/*
		const char* someJson = "{\"title\":\"document title\"}";
		nlohmann::json config = nlohmann::json::parse(someJson);
		SimpleVector<int> *testBiomeOrderList = SettingsManager::getIntSettingMulti( "biomeOrder" );
		biomes = testBiomeOrderList->getElementArray();
		openLife::system::type::record::Biome testedBiome;
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


		nextID =
				SettingsManager::getIntSetting( "nextPlayerID", 2 );


		// make backup and delete old backup every day
		AppLog::setLog( new FileLog( "log.txt", 86400 ) );

		AppLog::setLoggingLevel( Log::DETAIL_LEVEL );
		AppLog::printAllMessages( true );

		printf( "\n" );
		AppLog::info( "Server starting up" );

		printf( "\n" );




		nextSequenceNumber =
				SettingsManager::getIntSetting( "sequenceNumber", 1 );

		requireClientPassword =
				SettingsManager::getIntSetting( "requireClientPassword", 1 );

		requireTicketServerCheck =
				SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );

		clientPassword =
				SettingsManager::getStringSetting( "clientPassword" );


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



		secondsPerYear =
				SettingsManager::getFloatSetting( "secondsPerYear", 60.0f );


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




		int port =
				SettingsManager::getIntSetting( "port", 5077 );



		SocketServer *socketServer = new SocketServer( port, 256 );

		sockPoll.addSocketServer( socketServer );

		AppLog::infoF( "Listening for connection on port %d", port );

		// if we received one the last time we looped, don't sleep when
		// polling for socket being ready, because there could be more data
		// waiting in the buffer for a given socket
		char someClientMessageReceived = false;


		int shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
		int forceShutdownMode =
				SettingsManager::getIntSetting( "forceShutdownMode", 0 );


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

		server->start();
		std::cout << "\n\nStart main routine";
		while( !quit )
		{
			double curStepTime = Time::getCurrentTime();

			// flush past players hourly
			if( curStepTime - lastPastPlayerFlushTime > 3600 )
			{
				//std::cout << "\n===>flush past players hourly";

				// default one week
				int pastPlayerFlushTime =
						SettingsManager::getIntSetting( "pastPlayerFlushTime", 604000 );

				for( int i=0; i<pastPlayers.size(); i++ ) {
					DeadObject *o = pastPlayers.getElement( i );

					if( curStepTime - o->lifeStartTimeSeconds >
					pastPlayerFlushTime ) {
						// stale
						delete [] o->name;
						delete o->lineage;
						pastPlayers.deleteElement( i );
						i--;
					}
				}

				lastPastPlayerFlushTime = curStepTime;
			}

//!
			char periodicStepThisStep = false;
			if( curStepTime - lastPeriodicStepTime > periodicStepTime )
			{
				periodicStepThisStep = true;
				lastPeriodicStepTime = curStepTime;
			}
			if( periodicStepThisStep )
			{
				shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
				forceShutdownMode =
						SettingsManager::getIntSetting( "forceShutdownMode", 0 );

				if(!openLife::system::isFileWritable()) {
					// read-only file system causes all kinds of weird
					// behavior
					// shut this server down NOW
					printf( "File system read only, forcing server shutdown.\n" );

					// force-run cron script one time here
					// this will send warning email to admin
					// (cron jobs stop running if filesystem read-only)
					system( "../scripts/checkServerRunningCron.sh" );

					shutdownMode = 1;
					forceShutdownMode = 1;
				}
			}

//!check shutdown + force shutdown?
			if( forceShutdownMode )
			{
				shutdownMode = 1;

				const char *shutdownMessage = "SD\n#";
				int messageLength = strlen( shutdownMessage );

				// send everyone who's still alive a shutdown message
				for( int i=0; i<players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );

					if( nextPlayer->error ) {
						continue;
					}

					if( nextPlayer->connected ) {
						nextPlayer->sock->send(
								(unsigned char*)shutdownMessage,
								messageLength,
								false, false );

						nextPlayer->gotPartOfThisFrame = true;
					}

					// don't worry about num sent
					// it's the last message to this client anyway
					setDeathReason( nextPlayer,
									"forced_shutdown" );
					nextPlayer->error = true;
					nextPlayer->errorCauseString =
							"Forced server shutdown";
				}
			}
			else if( shutdownMode )
			{
				// any disconnected players should be killed now
				for( int i=0; i<players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );
					if( ! nextPlayer->error && ! nextPlayer->connected ) {

						setDeathReason( nextPlayer,
										"disconnect_shutdown" );

						nextPlayer->error = true;
						nextPlayer->errorCauseString =
								"Disconnected during shutdown";
					}
				}
			}

//!periodic steps
			if( periodicStepThisStep )
			{

				apocalypseStep();
				monumentStep();

				updateSpecialBiomes( players.size() );

				//checkBackup();

				stepFoodLog();
				stepFailureLog();

				stepPlayerStats();
				stepLineageLog();
				stepCurseServerRequests();

				stepLifeTokens();
				stepFitnessScore();

				stepMapLongTermCulling( players.size() );

				stepArcReport();

				int arcMilestone = getArcYearsToReport( secondsPerYear, 100 );

				// don't send global arc messages if Eve injection on
				// arcs never end
				int eveInjectionOn =
						SettingsManager::getIntSetting( "eveInjectionOn", 0 );

				if( arcMilestone != -1 && ! eveInjectionOn ) {

					int familyLimitAfterEveWindow =
							SettingsManager::getIntSetting(
									"familyLimitAfterEveWindow", 15 );

					int minFamiliesAfterEveWindow =
							SettingsManager::getIntSetting(
									"minFamiliesAfterEveWindow", 5 );

					char eveWindow = isEveWindow();

					char *familyLine;

					if( familyLimitAfterEveWindow > 0 &&
					! eveWindow ) {
						familyLine = autoSprintf( "of %d",
												  familyLimitAfterEveWindow );
					}
					else {
						familyLine = stringDuplicate( "" );
					}

					const char *familyWord = "FAMILIES ARE";

					int numFams = countFamilies();

					if( numFams == 1 ) {
						familyWord = "FAMILY IS";
					}

					char *arcEndMessage;

					if( !eveWindow && minFamiliesAfterEveWindow > 0 ) {
						arcEndMessage = autoSprintf( " (ARC ENDS BELOW %d)",
													 minFamiliesAfterEveWindow );
					}
					else {
						arcEndMessage = stringDuplicate( "" );
					}


					char *message = autoSprintf( "ARC HAS LASTED %d YEARS.  "
												 "ARC NAME IS :%s:**"
												 "%d %s %s ALIVE%s",
												 arcMilestone,
												 getArcName(),
												 numFams,
												 familyLine,
												 familyWord,
												 arcEndMessage );
					delete [] familyLine;
					delete [] arcEndMessage;

					sendGlobalMessage( message );

					delete [] message;
				}

				checkOrderPropagation();

				checkCustomGlobalMessage();


				int lowestCravingID = INT_MAX;

				for( int i=0; i< players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );

					if( nextPlayer->cravingFood.uniqueID > -1 &&
					nextPlayer->cravingFood.uniqueID < lowestCravingID ) {

						lowestCravingID = nextPlayer->cravingFood.uniqueID;
					}

					// also send queued global messages
					if( nextPlayer->globalMessageQueue.size() > 0 &&
					curStepTime - nextPlayer->lastGlobalMessageTime >
					minGlobalMessageSpacingSeconds ) {

						// send next one
						char *message =
								nextPlayer->globalMessageQueue.getElementDirect( 0 );
						nextPlayer->globalMessageQueue.deleteElement( 0 );

						sendGlobalMessage( message, nextPlayer );

						delete [] message;
					}
				}
				purgeStaleCravings( lowestCravingID );
			}


			int numLive = players.size();

//!
			if( shouldRunObjectSurvey() )
			{
				SimpleVector<GridPos> livePlayerPos;

				for( int i=0; i<numLive; i++ )
				{
					LiveObject *nextPlayer = players.getElement( i );

					if( nextPlayer->error ) {
						continue;
					}

					livePlayerPos.push_back( getPlayerPos( nextPlayer ) );
				}

				startObjectSurvey( &livePlayerPos );
			}
			stepObjectSurvey();

			stepLanguage();

//!
			double secPerYear = 1.0 / getAgeRate();


			// check for timeout for shortest player move or food decrement
			// so that we wake up from listening to socket to handle it
			double minMoveTime = 999999;

			double curTime = Time::getCurrentTime();

			for( int i=0; i<numLive; i++ )
			{
				LiveObject *nextPlayer = players.getElement( i );

				// clear at the start of each step
				nextPlayer->responsiblePlayerID = -1;

				if( nextPlayer->error ) {
					continue;
				}

				if( nextPlayer->xd != nextPlayer->xs ||
				nextPlayer->yd != nextPlayer->ys ) {

					double moveTimeLeft =
							nextPlayer->moveTotalSeconds -
							( curTime - nextPlayer->moveStartTime );

					if( moveTimeLeft < 0 ) {
						moveTimeLeft = 0;
					}

					if( moveTimeLeft < minMoveTime ) {
						minMoveTime = moveTimeLeft;
					}
				}


				double timeLeft = minMoveTime;

				if( ! nextPlayer->vogMode ) {
					// look at food decrement time too

					timeLeft =
							nextPlayer->foodDecrementETASeconds - curTime;

					if( timeLeft < 0 ) {
						timeLeft = 0;
					}
					if( timeLeft < minMoveTime ) {
						minMoveTime = timeLeft;
					}
				}

				// look at held decay too
				if( nextPlayer->holdingEtaDecay != 0 ) {

					timeLeft = nextPlayer->holdingEtaDecay - curTime;

					if( timeLeft < 0 ) {
						timeLeft = 0;
					}
					if( timeLeft < minMoveTime ) {
						minMoveTime = timeLeft;
					}
				}

				for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
					if( nextPlayer->clothingEtaDecay[c] != 0 ) {
						timeLeft = nextPlayer->clothingEtaDecay[c] - curTime;

						if( timeLeft < 0 ) {
							timeLeft = 0;
						}
						if( timeLeft < minMoveTime ) {
							minMoveTime = timeLeft;
						}
					}
					for( int cc=0; cc<nextPlayer->clothingContained[c].size();
					cc++ ) {
						timeSec_t decay =
								nextPlayer->clothingContainedEtaDecays[c].
								getElementDirect( cc );

						if( decay != 0 ) {
							timeLeft = decay - curTime;

							if( timeLeft < 0 ) {
								timeLeft = 0;
							}
							if( timeLeft < minMoveTime ) {
								minMoveTime = timeLeft;
							}
						}
					}
				}

				// look at old age death to
				double ageLeft = forceDeathAge - computeAge( nextPlayer );

				double ageSecondsLeft = ageLeft * secPerYear;

				if( ageSecondsLeft < minMoveTime ) {
					minMoveTime = ageSecondsLeft;

					if( minMoveTime < 0 ) {
						minMoveTime = 0;
					}
				}


				// as low as it can get, no need to check other players
				if( minMoveTime == 0 ) {
					break;
				}
			}


			SocketOrServer *readySock =  NULL;

			// at bare minimum, run our periodic steps at a fixed
			// frequency
			double pollTimeout = periodicStepTime;

			if( minMoveTime < pollTimeout )
			{
				// shorter timeout if we have to wake up for a move

				// HOWEVER, always keep max timout at 2 sec
				// so we always wake up periodically to catch quit signals, etc

				pollTimeout = minMoveTime;
			}

			if( pollTimeout > 0 )
			{
				int shortestDecay = getNextDecayDelta();

				if( shortestDecay != -1 ) {

					if( shortestDecay < pollTimeout ) {
						pollTimeout = shortestDecay;
					}
				}
			}


			char anyTicketServerRequestsOut = false;

			for( int i=0; i<newConnections.size(); i++ )
			{

				FreshConnection *nextConnection = newConnections.getElement( i );

				if( nextConnection->ticketServerRequest != NULL ) {
					anyTicketServerRequestsOut = true;
					break;
				}
			}

			if( anyTicketServerRequestsOut )
			{
				// need to step outstanding ticket server web requests
				// sleep a tiny amount of time to avoid cpu spin
				pollTimeout = 0.01;
			}


			if( areTriggersEnabled() )
			{
				// need to handle trigger timing
				pollTimeout = 0.01;
			}

			if( someClientMessageReceived )
			{
				// don't wait at all
				// we need to check for next message right away
				pollTimeout = 0;
			}

			if( tutorialLoadingPlayers.size() > 0 ) {
				// don't wait at all if there are tutorial maps to load
				pollTimeout = 0;
			}


			if( pollTimeout > 0.1 && activeKillStates.size() > 0 ) {
				// we have active kill requests pending
				// want a short timeout so that we can catch kills
				// when player's paths cross
				pollTimeout = 0.1;
			}


			// we thus use zero CPU as long as no messages or new connections
			// come in, and only wake up when some timed action needs to be
			// handled

			readySock = sockPoll.wait( (int)( pollTimeout * 1000 ) );




			if( readySock != NULL && !readySock->isSocket ) {
				// server ready
				Socket *sock = socketServer->acceptConnection( 0 );

				if( sock != NULL ) {
					HostAddress *a = sock->getRemoteHostAddress();

					if( a == NULL ) {
						AppLog::info( "Got connection from unknown address" );
					}
					else {
						AppLog::infoF( "Got connection from %s:%d",
									   a->mAddressString, a->mPort );
						delete a;
					}


					FreshConnection newConnection;

					newConnection.connectionStartTimeSeconds =
							Time::getCurrentTime();

					newConnection.email = NULL;

					newConnection.sock = sock;

					newConnection.sequenceNumber = nextSequenceNumber;

					newConnection.reconnectOnly = false;


					char *secretString =
							SettingsManager::getStringSetting(
									"statsServerSharedSecret", "sdfmlk3490sadfm3ug9324" );

					char *numberString =
							autoSprintf( "%lu", newConnection.sequenceNumber );

					char *nonce = hmac_sha1( secretString, numberString );

					delete [] secretString;
					delete [] numberString;

					newConnection.sequenceNumberString =
							autoSprintf( "%s%lu", nonce,
										 newConnection.sequenceNumber );

					delete [] nonce;


					newConnection.tutorialNumber = 0;
					newConnection.curseStatus.curseLevel = 0;
					newConnection.curseStatus.excessPoints = 0;

					newConnection.twinCode = NULL;
					newConnection.twinCount = 0;

					newConnection.clientTag = NULL;

					nextSequenceNumber ++;

					SettingsManager::setSetting( "sequenceNumber",
												 (int)nextSequenceNumber );

					char *message;

					int maxPlayers =
							SettingsManager::getIntSetting( "maxPlayers", 200 );

					int currentPlayers = players.size() + newConnections.size();


					if( apocalypseTriggered || shutdownMode ) {

						AppLog::info( "We are in shutdown mode, "
									  "deflecting new connection" );

						AppLog::infoF( "%d player(s) still alive on server.",
									   players.size() );

						message = autoSprintf( "SHUTDOWN\n"
											   "%d/%d\n"
											   "#",
											   currentPlayers, maxPlayers );
						newConnection.shutdownMode = true;
					}
					else if( currentPlayers >= maxPlayers ) {
						AppLog::infoF( "%d of %d permitted players connected, "
									   "deflecting new connection",
									   currentPlayers, maxPlayers );

						message = autoSprintf( "SERVER_FULL\n"
											   "%d/%d\n"
											   "#",
											   currentPlayers, maxPlayers );

						newConnection.shutdownMode = true;
					}
					else {
						message = autoSprintf( "SN\n"
											   "%d/%d\n"
											   "%s\n"
											   "%lu\n#",
											   currentPlayers, maxPlayers,
											   newConnection.sequenceNumberString,
											   versionNumber );
						newConnection.shutdownMode = false;
					}


					// wait for email and hashes to come from client
					// (and maybe ticket server check isn't required by settings)
					newConnection.ticketServerRequest = NULL;
					newConnection.ticketServerAccepted = false;
					newConnection.lifeTokenSpent = false;

					// -1 is a possible score now
					// use -99999 as still-waiting marker
					newConnection.fitnessScore = -99999;

					newConnection.error = false;
					newConnection.errorCauseString = "";
					newConnection.rejectedSendTime = 0;

					int messageLength = strlen( message );

					int numSent =
							sock->send( (unsigned char*)message,
										messageLength,
										false, false );

					delete [] message;


					if( numSent != messageLength ) {
						// failed or blocked on our first send attempt

						// reject it right away

						delete sock;
						sock = NULL;
					}
					else {
						// first message sent okay
						newConnection.sockBuffer = new SimpleVector<char>();


						sockPoll.addSocket( sock );

						newConnections.push_back( newConnection );
					}

					AppLog::infoF( "Listening for another connection on port %d",
								   port );

				}
			}


			stepTriggers();


			// listen for messages from new connections
			double currentTime = Time::getCurrentTime();

			for( int i=0; i<newConnections.size(); i++ ) {

				FreshConnection *nextConnection = newConnections.getElement( i );

				if( nextConnection->error ) {
					continue;
				}

				if( nextConnection->email != NULL &&
				nextConnection->curseStatus.curseLevel == -1 ) {
					// keep checking if curse level has arrived from
					// curse server
					nextConnection->curseStatus =
							getCurseLevel( nextConnection->email );
					if( nextConnection->curseStatus.curseLevel != -1 ) {
						AppLog::infoF(
								"Got curse level for %s from curse server: "
								"%d (excess %d)",
								nextConnection->email,
								nextConnection->curseStatus.curseLevel,
								nextConnection->curseStatus.excessPoints );
					}
				}
				else if( nextConnection->email != NULL &&
				nextConnection->lifeStats.lifeCount == -1 ) {
					// keep checking if life stats have arrived from
					// stats server
					int statsResult = getPlayerLifeStats( nextConnection->email,
														  &( nextConnection->lifeStats.lifeCount ),
														  &( nextConnection->lifeStats.lifeTotalSeconds ) );

					if( statsResult == -1 ) {
						// error
						// it's done now!
						nextConnection->lifeStats.lifeCount = 0;
						nextConnection->lifeStats.lifeTotalSeconds = 0;
						nextConnection->lifeStats.error = true;
					}
					else if( statsResult == 1 ) {
						AppLog::infoF(
								"Got life stats for %s from stats server: "
								"%d lives, %d total seconds (%.2lf hours)",
								nextConnection->email,
								nextConnection->lifeStats.lifeCount,
								nextConnection->lifeStats.lifeTotalSeconds,
								nextConnection->lifeStats.lifeTotalSeconds / 3600.0 );
					}
				}
				else if( nextConnection->email != NULL &&
				nextConnection->fitnessScore == -99999 ) {
					// still waiting for fitness score
					int fitResult =
							getFitnessScore( nextConnection->email,
											 &nextConnection->fitnessScore );

					if( fitResult == -1 ) {
						// failed
						// stop asking now
						nextConnection->fitnessScore = 0;
					}
				}
				else if( nextConnection->ticketServerRequest != NULL &&
				! nextConnection->ticketServerAccepted ) {

					int result;

					if( currentTime - nextConnection->ticketServerRequestStartTime
					< 8 ) {
						// 8-second timeout on ticket server requests
						result = nextConnection->ticketServerRequest->step();
					}
					else {
						result = -1;
					}

					if( result == -1 ) {
						AppLog::info( "Request to ticket server failed, "
									  "client rejected." );
						nextConnection->error = true;
						nextConnection->errorCauseString =
								"Ticket server failed";
					}
					else if( result == 1 ) {
						// done, have result

						char *webResult =
								nextConnection->ticketServerRequest->getResult();

						if( strstr( webResult, "INVALID" ) != NULL ) {
							AppLog::info(
									"Client key hmac rejected by ticket server, "
									"client rejected." );
							nextConnection->error = true;
							nextConnection->errorCauseString =
									"Client key check failed";
						}
						else if( strstr( webResult, "VALID" ) != NULL ) {
							// correct!
							nextConnection->ticketServerAccepted = true;
						}
						else {
							AppLog::errorF(
									"Unexpected result from ticket server, "
									"client rejected:  %s", webResult );
							nextConnection->error = true;
							nextConnection->errorCauseString =
									"Client key check failed "
									"(bad ticketServer response)";
						}
						delete [] webResult;
					}
				}
				else if( nextConnection->ticketServerRequest != NULL &&
				nextConnection->ticketServerAccepted &&
				! nextConnection->lifeTokenSpent ) {

					// this "butDisconnected" state applies even if
					// we see them as connected, becasue they are clearly
					// reconnecting now
					char liveButDisconnected = false;

					for( int p=0; p<players.size(); p++ ) {
						LiveObject *o = players.getElement( p );
						if( ! o->error &&
						strcmp( o->email,
								nextConnection->email ) == 0 ) {
							liveButDisconnected = true;
							break;
						}
					}

					if( liveButDisconnected ) {
						// spent when they first connected, don't respend now
						nextConnection->lifeTokenSpent = true;
					}
					else {
						int spendResult =
								spendLifeToken( nextConnection->email );
						if( spendResult == -1 ) {
							AppLog::info(
									"Failed to spend life token for client, "
									"client rejected." );

							const char *message = "NO_LIFE_TOKENS\n#";
							nextConnection->sock->send( (unsigned char*)message,
														strlen( message ),
														false, false );

							nextConnection->error = true;
							nextConnection->errorCauseString =
									"Client life token spend failed";
						}
						else if( spendResult == 1 ) {
							nextConnection->lifeTokenSpent = true;
						}
					}
				}
				else if( nextConnection->ticketServerRequest != NULL &&
				nextConnection->ticketServerAccepted &&
				nextConnection->lifeTokenSpent ) {
					// token spent successfully (or token server not used)

					const char *message = "ACCEPTED\n#";
					int messageLength = strlen( message );

					int numSent =
							nextConnection->sock->send(
									(unsigned char*)message,
									messageLength,
									false, false );


					if( numSent != messageLength ) {
						AppLog::info( "Failed to write to client socket, "
									  "client rejected." );
						nextConnection->error = true;
						nextConnection->errorCauseString =
								"Socket write failed";

					}
					else {
						// ready to start normal message exchange
						// with client

						AppLog::info( "Got new player logged in" );
						logClientTag( nextConnection );

						delete nextConnection->ticketServerRequest;
						nextConnection->ticketServerRequest = NULL;

						delete [] nextConnection->sequenceNumberString;
						nextConnection->sequenceNumberString = NULL;

						delete [] nextConnection->clientTag;
						nextConnection->clientTag = NULL;

						if( nextConnection->twinCode != NULL
						&&
						nextConnection->twinCount > 0 ) {
							processWaitingTwinConnection( *nextConnection );
						}
						else {
							if( nextConnection->twinCode != NULL ) {
								delete [] nextConnection->twinCode;
								nextConnection->twinCode = NULL;
							}

							processLoggedInPlayer(
									nextConnection->reconnectOnly ? 2 : true,
									nextConnection->sock,
									nextConnection->sockBuffer,
									nextConnection->email,
									nextConnection->tutorialNumber,
									nextConnection->curseStatus,
									nextConnection->lifeStats,
									nextConnection->fitnessScore );
						}

						newConnections.deleteElement( i );
						i--;
					}
				}
				else if( nextConnection->ticketServerRequest == NULL ) {

					double timeDelta = Time::getCurrentTime() -
							nextConnection->connectionStartTimeSeconds;




					char result =
							readSocketFull( nextConnection->sock,
											nextConnection->sockBuffer );

					if( ! result ) {
						AppLog::info( "Failed to read from client socket, "
									  "client rejected." );
						nextConnection->error = true;

						// force connection close right away
						// don't send REJECTED message and wait
						nextConnection->rejectedSendTime = 1;

						nextConnection->errorCauseString =
								"Socket read failed";
					}

					char *message = NULL;
					int timeLimit = 10;

					if( ! nextConnection->shutdownMode ) {
						message =
								getNextClientMessage( nextConnection->sockBuffer );
					}
					else {
						timeLimit = 5;
					}

					if( message != NULL ) {


						if( strstr( message, "LOGIN" ) != NULL ) {

							if( strstr( message, "RLOGIN" ) != NULL ) {
								nextConnection->reconnectOnly = true;
							}

							SimpleVector<char *> *tokens =
									tokenizeString( message );

							if( strstr( message, "client_" ) != NULL ) {
								// new client_ parameter

								// it is the first parameter after LOGIN

								// save and remove it
								nextConnection->clientTag =
										tokens->getElementDirect( 1 );

								tokens->deleteElement( 1 );
							}

							if( tokens->size() == 4 || tokens->size() == 5 ||
							tokens->size() == 7 ) {

								nextConnection->email =
										stringToLowerCase(
												tokens->getElementDirect( 1 ) );
								char *pwHash = tokens->getElementDirect( 2 );
								char *keyHash = tokens->getElementDirect( 3 );

								if( tokens->size() >= 5 ) {
									sscanf( tokens->getElementDirect( 4 ),
											"%d",
											&( nextConnection->tutorialNumber ) );
								}

								if( tokens->size() == 7 ) {
									nextConnection->twinCode =
											stringDuplicate(
													tokens->getElementDirect( 5 ) );

									sscanf( tokens->getElementDirect( 6 ),
											"%d",
											&( nextConnection->twinCount ) );

									int maxCount =
											SettingsManager::getIntSetting(
													"maxTwinPartySize", 4 );

									if( nextConnection->twinCount > maxCount ) {
										nextConnection->twinCount = maxCount;
									}
								}


								// this may return -1 if curse server
								// request is pending
								// we'll catch that case later above
								nextConnection->curseStatus =
										getCurseLevel( nextConnection->email );


								nextConnection->lifeStats.lifeCount = -1;
								nextConnection->lifeStats.lifeTotalSeconds = -1;
								nextConnection->lifeStats.error = false;

								// this will leave them as -1 if request pending
								// we'll catch that case later above
								int statsResult = getPlayerLifeStats(
										nextConnection->email,
										&( nextConnection->
										lifeStats.lifeCount ),
										&( nextConnection->
										lifeStats.lifeTotalSeconds ) );

								if( statsResult == -1 ) {
									// error
									// it's done now!
									nextConnection->lifeStats.lifeCount = 0;
									nextConnection->lifeStats.lifeTotalSeconds = 0;
									nextConnection->lifeStats.error = true;
								}



								if( requireClientPassword &&
								! nextConnection->error  ) {

									char *trueHash =
											hmac_sha1(
													clientPassword,
													nextConnection->sequenceNumberString );


									if( strcmp( trueHash, pwHash ) != 0 ) {
										AppLog::info( "Client password hmac bad, "
													  "client rejected." );
										nextConnection->error = true;
										nextConnection->errorCauseString =
												"Password check failed";
									}

									delete [] trueHash;
								}

								if( requireTicketServerCheck &&
								! nextConnection->error ) {

									char *encodedEmail =
											URLUtils::urlEncode(
													nextConnection->email );

									char *url = autoSprintf(
											"%s?action=check_ticket_hash"
											"&email=%s"
											"&hash_value=%s"
											"&string_to_hash=%s",
											ticketServerURL,
											encodedEmail,
											keyHash,
											nextConnection->sequenceNumberString );

									delete [] encodedEmail;

									nextConnection->ticketServerRequest =
											new WebRequest( "GET", url, NULL );
									nextConnection->ticketServerAccepted = false;

									nextConnection->ticketServerRequestStartTime
									= currentTime;

									delete [] url;
								}
								else if( !requireTicketServerCheck &&
								!nextConnection->error ) {

									// let them in without checking

									const char *message = "ACCEPTED\n#";
									int messageLength = strlen( message );

									int numSent =
											nextConnection->sock->send(
													(unsigned char*)message,
													messageLength,
													false, false );


									if( numSent != messageLength ) {
										AppLog::info(
												"Failed to send on client socket, "
												"client rejected." );
										nextConnection->error = true;
										nextConnection->errorCauseString =
												"Socket write failed";
									}
									else {
										// ready to start normal message exchange
										// with client

										AppLog::info( "Got new player logged in" );
										logClientTag( nextConnection );

										delete nextConnection->ticketServerRequest;
										nextConnection->ticketServerRequest = NULL;

										delete []
										nextConnection->sequenceNumberString;
										nextConnection->sequenceNumberString = NULL;

										delete [] nextConnection->clientTag;
										nextConnection->clientTag = NULL;


										if( nextConnection->twinCode != NULL
										&&
										nextConnection->twinCount > 0 ) {
											processWaitingTwinConnection(
													*nextConnection );
										}
										else {
											if( nextConnection->twinCode != NULL ) {
												delete [] nextConnection->twinCode;
												nextConnection->twinCode = NULL;
											}
											processLoggedInPlayer(
													nextConnection->reconnectOnly ?
													2 : true,
													nextConnection->sock,
													nextConnection->sockBuffer,
													nextConnection->email,
													nextConnection->tutorialNumber,
													nextConnection->curseStatus,
													nextConnection->lifeStats,
													nextConnection->fitnessScore );
										}

										newConnections.deleteElement( i );
										i--;
									}
								}
							}
							else {
								AppLog::info( "LOGIN message has wrong format, "
											  "client rejected." );
								nextConnection->error = true;
								nextConnection->errorCauseString =
										"Bad login message";
							}


							tokens->deallocateStringElements();
							delete tokens;
						}
						else {
							AppLog::info( "Client's first message not LOGIN, "
										  "client rejected." );
							nextConnection->error = true;
							nextConnection->errorCauseString =
									"Unexpected first message";
						}

						delete [] message;
					}
					else if( timeDelta > timeLimit ) {
						if( nextConnection->shutdownMode ) {
							AppLog::info( "5 second grace period for new "
										  "connection in shutdown mode, closing." );
						}
						else {
							AppLog::info(
									"Client failed to LOGIN after 10 seconds, "
									"client rejected." );
						}
						nextConnection->error = true;
						nextConnection->errorCauseString =
								"Login timeout";
					}
				}
			}



			// make sure all twin-waiting sockets are still connected
			for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
				FreshConnection *nextConnection =
						waitingForTwinConnections.getElement( i );

				char result =
						readSocketFull( nextConnection->sock,
										nextConnection->sockBuffer );

				if( ! result ) {
					AppLog::info( "Failed to read from twin-waiting client socket, "
								  "client rejected." );

					refundLifeToken( nextConnection->email );

					nextConnection->error = true;

					// force connection close right away
					// don't send REJECTED message and wait
					nextConnection->rejectedSendTime = 1;

					nextConnection->errorCauseString =
							"Socket read failed";
				}
			}



			// now clean up any new connections that have errors

			// FreshConnections are in two different lists
			// clean up errors in both
			currentTime = Time::getCurrentTime();

			SimpleVector<FreshConnection> *connectionLists[2] =
					{ &newConnections, &waitingForTwinConnections };
			for( int c=0; c<2; c++ ) {
				SimpleVector<FreshConnection> *list = connectionLists[c];

				for( int i=0; i<list->size(); i++ ) {

					FreshConnection *nextConnection = list->getElement( i );

					if( nextConnection->error ) {

						if( nextConnection->rejectedSendTime == 0 ) {

							// try sending REJECTED message at end
							// give them 5 seconds to receive it before closing
							// the connection
							const char *message = "REJECTED\n#";
							nextConnection->sock->send( (unsigned char*)message,
														strlen( message ),
														false, false );
							nextConnection->rejectedSendTime = currentTime;
						}
						else if( currentTime - nextConnection->rejectedSendTime >
						5 ) {
							// 5 sec passed since REJECTED sent

							AppLog::infoF( "Closing new connection on error "
										   "(cause: %s)",
										   nextConnection->errorCauseString );

							if( nextConnection->sock != NULL ) {
								sockPoll.removeSocket( nextConnection->sock );
							}

							deleteMembers( nextConnection );

							list->deleteElement( i );
							i--;
						}
					}
				}
			}


			// step tutorial map load for player at front of line

			// 5 ms
			double timeLimit = 0.005;

			for( int i=0; i<tutorialLoadingPlayers.size(); i++ ) {
				LiveObject *nextPlayer = tutorialLoadingPlayers.getElement( i );

				char moreLeft = loadTutorialStep( &( nextPlayer->tutorialLoad ),
												  timeLimit );

				if( moreLeft ) {
					// only load one step from first in line
					break;
				}

				// first in line is done

				AppLog::infoF( "New player %s tutorial loaded after %u steps, "
							   "%f total sec (loadID = %u )",
							   nextPlayer->email,
							   nextPlayer->tutorialLoad.stepCount,
							   Time::getCurrentTime() -
							   nextPlayer->tutorialLoad.startTime,
							   nextPlayer->tutorialLoad.uniqueLoadID );

				// remove it and any twins
				unsigned int uniqueID = nextPlayer->tutorialLoad.uniqueLoadID;


				players.push_back( *nextPlayer );

				tutorialLoadingPlayers.deleteElement( i );

				LiveObject *twinPlayer = NULL;

				if( i < tutorialLoadingPlayers.size() ) {
					twinPlayer = tutorialLoadingPlayers.getElement( i );
				}

				while( twinPlayer != NULL &&
				twinPlayer->tutorialLoad.uniqueLoadID == uniqueID ) {

					AppLog::infoF( "Twin %s tutorial loaded too (loadID = %u )",
								   twinPlayer->email,
								   uniqueID );

					players.push_back( *twinPlayer );

					tutorialLoadingPlayers.deleteElement( i );

					twinPlayer = NULL;

					if( i < tutorialLoadingPlayers.size() ) {
						twinPlayer = tutorialLoadingPlayers.getElement( i );
					}
				}
				break;

			}





			someClientMessageReceived = false;

			numLive = players.size();


			// listen for any messages from clients

			// track index of each player that needs an update sent about it
			// we compose the full update message below
			SimpleVector<int> playerIndicesToSendUpdatesAbout;

			SimpleVector<int> playerIndicesToSendLineageAbout;

			SimpleVector<int> playerIndicesToSendCursesAbout;

			SimpleVector<int> playerIndicesToSendNamesAbout;

			SimpleVector<int> playerIndicesToSendDyingAbout;

			SimpleVector<int> playerIndicesToSendHealingAbout;



			newOwnerPos.push_back_other( &recentlyRemovedOwnerPos );
			recentlyRemovedOwnerPos.deleteAll();


			SimpleVector<UpdateRecord> newUpdates;
			SimpleVector<ChangePosition> newUpdatesPos;
			SimpleVector<int> newUpdatePlayerIDs;

			SimpleVector<int> newFlipPlayerIDs;
			SimpleVector<int> newFlipFacingLeft;
			SimpleVector<GridPos> newFlipPositions;


			// these are global, so they're not tagged with positions for
			// spatial filtering
			SimpleVector<UpdateRecord> newDeleteUpdates;


			SimpleVector<MapChangeRecord> mapChanges;
			SimpleVector<ChangePosition> mapChangesPos;


			SimpleVector<FlightDest> newFlightDest;




			timeSec_t curLookTime = Time::timeSec();

			for( int i=0; i<numLive; i++ ) {
				LiveObject *nextPlayer = players.getElement( i );

				nextPlayer->updateSent = false;

				if( nextPlayer->error ) {
					continue;
				}

				if( nextPlayer->numToolSlots == -1 ) {

					setupToolSlots( nextPlayer );
				}


				double curCrossTime = Time::getCurrentTime();

				char checkCrossing = true;

				if( curCrossTime < nextPlayer->playerCrossingCheckTime +
				playerCrossingCheckStepTime ) {
					// player not due for another check yet
					checkCrossing = false;
				}
				else {
					// time for next check for this player
					nextPlayer->playerCrossingCheckTime = curCrossTime;
					checkCrossing = true;
				}


				if( checkCrossing ) {
					GridPos curPos = { nextPlayer->xd, nextPlayer->yd };

					if( nextPlayer->xd != nextPlayer->xs ||
					nextPlayer->yd != nextPlayer->ys ) {

						curPos = computePartialMoveSpot( nextPlayer );
					}

					int curOverID = getMapObject( curPos.x, curPos.y );

					char riding = false;

					if( nextPlayer->holdingID > 0 &&
					getObject( nextPlayer->holdingID )->rideable ) {
						riding = true;
					}


					GridPos deadlyDestPos = curPos;

					if( ! riding ) {
						// check if player is standing on
						// a non-deadly object OR on nothing
						// if so, moving deadly objects might still be able
						// to get them
						ObjectRecord *curOverObj = NULL;

						if( curOverID > 0 ) {
							curOverObj = getObject( curOverID );
						}

						if( curOverObj == NULL ||
						! curOverObj->permanent ||
						curOverObj->deadlyDistance == 0 ) {

							int movingDestX, movingDestY;

							int curMovingID =
									getDeadlyMovingMapObject(
											curPos.x, curPos.y,
											&movingDestX, &movingDestY );

							if( curMovingID != 0 ) {

								// make sure that dest object hasn't changed
								// since moving record was created
								// (if a bear is shot mid-move, for example,
								//  the movement record will still show the unshot
								//  bear)
								curMovingID = getMapObject( movingDestX,
															movingDestY );

								ObjectRecord *movingObj = getObject( curMovingID );
								if( movingObj->permanent &&
								movingObj->deadlyDistance > 0 ) {
									curOverID = curMovingID;

									deadlyDestPos.x = movingDestX;
									deadlyDestPos.y = movingDestY;
								}
							}
						}
					}



					if( ! nextPlayer->heldByOther &&
					! nextPlayer->vogMode &&
					curOverID != 0 &&
					! isMapObjectInTransit( curPos.x, curPos.y ) &&
					! wasRecentlyDeadly( curPos ) ) {

						ObjectRecord *curOverObj = getObject( curOverID );


						if( !riding &&
						curOverObj->permanent &&
						curOverObj->deadlyDistance > 0 ) {

							char wasSick = false;

							if( nextPlayer->holdingID > 0 &&
							strstr(
									getObject( nextPlayer->holdingID )->
									description,
									"sick" ) != NULL ) {
								wasSick = true;
							}


							addDeadlyMapSpot( curPos );

							setDeathReason( nextPlayer,
											"killed",
											curOverID );

							nextPlayer->deathSourceID = curOverID;

							if( curOverObj->isUseDummy ) {
								nextPlayer->deathSourceID =
										curOverObj->useDummyParent;
							}

							nextPlayer->errorCauseString =
									"Player killed by permanent object";

							if( ! nextPlayer->dying || wasSick ) {
								// if was sick, they had a long stagger
								// time set, so cutting it in half makes no sense

								int staggerTime =
										SettingsManager::getIntSetting(
												"deathStaggerTime", 20 );

								double currentTime =
										Time::getCurrentTime();

								nextPlayer->dying = true;
								nextPlayer->dyingETA =
										currentTime + staggerTime;

								playerIndicesToSendDyingAbout.
								push_back(
										getLiveObjectIndex(
												nextPlayer->id ) );
							}
							else {
								// already dying, and getting attacked again

								// halve their remaining stagger time
								double currentTime =
										Time::getCurrentTime();

								double staggerTimeLeft =
										nextPlayer->dyingETA - currentTime;

								if( staggerTimeLeft > 0 ) {
									staggerTimeLeft /= 2;
									nextPlayer->dyingETA =
											currentTime + staggerTimeLeft;
								}
							}


							// generic on-person
							TransRecord *r =
									getPTrans( curOverID, 0 );

							if( r != NULL ) {
								setMapObject( deadlyDestPos.x, deadlyDestPos.y,
											  r->newActor );

								// new target specifies wound
								// but never replace an existing wound
								// death time is shortened above
								// however, wounds can replace sickness
								if( r->newTarget > 0 &&
								( ! nextPlayer->holdingWound || wasSick ) ) {
									// don't drop their wound
									if( nextPlayer->holdingID != 0 &&
									! nextPlayer->holdingWound &&
									! nextPlayer->holdingBiomeSickness ) {
										handleDrop(
												curPos.x, curPos.y,
												nextPlayer,
												&playerIndicesToSendUpdatesAbout );
									}
									nextPlayer->holdingID =
											r->newTarget;
									holdingSomethingNew( nextPlayer );

									setFreshEtaDecayForHeld( nextPlayer );

									checkSickStaggerTime( nextPlayer );


									nextPlayer->holdingWound = true;
									nextPlayer->holdingBiomeSickness = false;

									ForcedEffects e =
											checkForForcedEffects(
													nextPlayer->holdingID );

									if( e.emotIndex != -1 ) {
										nextPlayer->emotFrozen = true;
										nextPlayer->emotFrozenIndex = e.emotIndex;

										newEmotPlayerIDs.push_back(
												nextPlayer->id );
										newEmotIndices.push_back( e.emotIndex );
										newEmotTTLs.push_back( e.ttlSec );
										interruptAnyKillEmots( nextPlayer->id,
															   e.ttlSec );
									}
									if( e.foodModifierSet &&
									e.foodCapModifier != 1 ) {
										nextPlayer->yummyBonusStore = 0;
										nextPlayer->foodCapModifier =
												e.foodCapModifier;
										nextPlayer->foodUpdate = true;
									}
									if( e.feverSet ) {
										nextPlayer->fever = e.fever;
									}


									playerIndicesToSendUpdatesAbout.
									push_back(
											getLiveObjectIndex(
													nextPlayer->id ) );
								}
							}
						}
						else if( riding &&
						curOverObj->permanent &&
						curOverObj->deadlyDistance > 0 ) {
							// rode over something deadly
							// see if it affects what we're riding

							TransRecord *r =
									getPTrans( nextPlayer->holdingID, curOverID );

							if( r != NULL ) {
								handleHoldingChange( nextPlayer,
													 r->newActor );
								nextPlayer->heldTransitionSourceID = curOverID;
								playerIndicesToSendUpdatesAbout.push_back( i );

								setMapObject( curPos.x, curPos.y, r->newTarget );

								// it attacked their vehicle
								// put it on cooldown so it won't immediately
								// attack them
								addDeadlyMapSpot( curPos );
							}
						}
					}
				}


				if( curLookTime - nextPlayer->lastRegionLookTime > 5 ) {
					lookAtRegion( nextPlayer->xd - 8, nextPlayer->yd - 7,
								  nextPlayer->xd + 8, nextPlayer->yd + 7 );
					nextPlayer->lastRegionLookTime = curLookTime;
				}

				char *message = NULL;

				if( nextPlayer->connected ) {
					char result =
							readSocketFull( nextPlayer->sock, nextPlayer->sockBuffer );

					if( ! result ) {
						setPlayerDisconnected( nextPlayer, "Socket read failed" );
					}
					else {
						// don't even bother parsing message buffer for players
						// that are not currently connected
						message = getNextClientMessage( nextPlayer->sockBuffer );
					}
				}

				if( message != NULL ) {
					someClientMessageReceived = true;

					AppLog::infoF( "Got client message from %d: %s",
								   nextPlayer->id, message );

					ClientMessage m = parseMessage( nextPlayer, message );

					delete [] message;


					//Thread::staticSleep(
					//    testRandSource.getRandomBoundedInt( 0, 450 ) );


					// GOTO below jumps here if we need to reparse the message
					// as a different type
					RESTART_MESSAGE_ACTION:

					if( m.type == UNKNOWN ) {
						AppLog::info( "Client error, unknown message type." );

						//setPlayerDisconnected( nextPlayer,
						//                       "Unknown message type" );

						// do not disconnect client here
						// keep server flexible, so client can be updated
						// with a protocol change before the server gets updated
					}
					else if( m.type == BUG ) {
						int allow =
								SettingsManager::getIntSetting( "allowBugReports", 0 );

						if( allow ) {
							char *bugName =
									autoSprintf( "bug_%d_%d_%f",
												 m.bug,
												 nextPlayer->id,
												 Time::getCurrentTime() );
							char *bugInfoName = autoSprintf( "%s_info.txt",
															 bugName );
							char *bugOutName = autoSprintf( "%s_out.txt",
															bugName );
							FILE *bugInfo = fopen( bugInfoName, "w" );
							if( bugInfo != NULL ) {
								fprintf( bugInfo,
										 "Bug report from player %d\n"
										 "Bug text:  %s\n",
										 nextPlayer->id,
										 m.bugText );
								fclose( bugInfo );

								File outFile( NULL, "serverOut.txt" );
								if( outFile.exists() ) {
									fflush( stdout );
									File outCopyFile( NULL, bugOutName );

									outFile.copy( &outCopyFile );
								}
							}
							delete [] bugName;
							delete [] bugInfoName;
							delete [] bugOutName;
						}
					}
					else if( m.type == MAP ) {

						int allow =
								SettingsManager::getIntSetting( "allowMapRequests", 0 );


						if( allow ) {

							SimpleVector<char *> *list =
									SettingsManager::getSetting(
											"mapRequestAllowAccounts" );

							allow = false;

							for( int i=0; i<list->size(); i++ ) {
								if( strcmp( nextPlayer->email,
											list->getElementDirect( i ) ) == 0 ) {

									allow = true;
									break;
								}
							}

							list->deallocateStringElements();
							delete list;
						}


						if( allow && nextPlayer->connected ) {

							// keep them full of food so they don't
							// die of hunger during the pull
							nextPlayer->foodStore =
									computeFoodCapacity( nextPlayer );


							int length;

							// map chunks sent back to client absolute
							// relative to center instead of birth pos
							GridPos centerPos = { 0, 0 };

							unsigned char *mapChunkMessage =
									getChunkMessage( m.x - chunkDimensionX / 2,
													 m.y - chunkDimensionY / 2,
													 chunkDimensionX,
													 chunkDimensionY,
													 centerPos,
													 &length );

							int numSent =
									nextPlayer->sock->send( mapChunkMessage,
															length,
															false, false );

							nextPlayer->gotPartOfThisFrame = true;

							delete [] mapChunkMessage;

							if( numSent != length ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}
						}
						else {
							AppLog::infoF( "Map pull request rejected for %s",
										   nextPlayer->email );
						}
					}
					else if( m.type == TRIGGER ) {
						if( areTriggersEnabled() ) {
							trigger( m.trigger );
						}
					}
					else if( m.type == VOGS ) {
						int allow =
								SettingsManager::getIntSetting( "allowVOGMode", 0 );

						if( allow ) {

							SimpleVector<char *> *list =
									SettingsManager::getSetting(
											"vogAllowAccounts" );

							allow = false;

							for( int i=0; i<list->size(); i++ ) {
								if( strcmp( nextPlayer->email,
											list->getElementDirect( i ) ) == 0 ) {

									allow = true;
									break;
								}
							}

							list->deallocateStringElements();
							delete list;
						}


						if( allow && nextPlayer->connected ) {
							nextPlayer->vogMode = true;
							nextPlayer->preVogPos = getPlayerPos( nextPlayer );
							nextPlayer->preVogBirthPos = nextPlayer->birthPos;
							nextPlayer->vogJumpIndex = 0;
						}
					}
					else if( m.type == VOGN ) {
						if( nextPlayer->vogMode &&
						players.size() > 1 ) {

							nextPlayer->vogJumpIndex++;
							if( nextPlayer->vogJumpIndex == i ) {
								nextPlayer->vogJumpIndex++;
							}
							if( nextPlayer->vogJumpIndex >= players.size() ) {
								nextPlayer->vogJumpIndex = 0;
							}
							if( nextPlayer->vogJumpIndex == i ) {
								nextPlayer->vogJumpIndex++;
							}

							LiveObject *otherPlayer =
									players.getElement(
											nextPlayer->vogJumpIndex );

							GridPos o = getPlayerPos( otherPlayer );

							GridPos oldPos = getPlayerPos( nextPlayer );


							nextPlayer->xd = o.x;
							nextPlayer->yd = o.y;

							nextPlayer->xs = o.x;
							nextPlayer->ys = o.y;

							if( distance( oldPos, o ) > 10000 ) {
								nextPlayer->birthPos = o;
							}

							char *message = autoSprintf( "VU\n%d %d\n#",
														 nextPlayer->xs -
														 nextPlayer->birthPos.x,
														 nextPlayer->ys -
														 nextPlayer->birthPos.y );
							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );

							delete [] message;

							nextPlayer->firstMessageSent = false;
							nextPlayer->firstMapSent = false;
						}
					}
					else if( m.type == VOGP ) {
						if( nextPlayer->vogMode &&
						players.size() > 1 ) {

							nextPlayer->vogJumpIndex--;

							if( nextPlayer->vogJumpIndex == i ) {
								nextPlayer->vogJumpIndex--;
							}
							if( nextPlayer->vogJumpIndex < 0 ) {
								nextPlayer->vogJumpIndex = players.size() - 1;
							}
							if( nextPlayer->vogJumpIndex == i ) {
								nextPlayer->vogJumpIndex--;
							}

							LiveObject *otherPlayer =
									players.getElement(
											nextPlayer->vogJumpIndex );

							GridPos o = getPlayerPos( otherPlayer );

							GridPos oldPos = getPlayerPos( nextPlayer );


							nextPlayer->xd = o.x;
							nextPlayer->yd = o.y;

							nextPlayer->xs = o.x;
							nextPlayer->ys = o.y;

							if( distance( oldPos, o ) > 10000 ) {
								nextPlayer->birthPos = o;
							}

							char *message = autoSprintf( "VU\n%d %d\n#",
														 nextPlayer->xs -
														 nextPlayer->birthPos.x,
														 nextPlayer->ys -
														 nextPlayer->birthPos.y );
							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );

							delete [] message;

							nextPlayer->firstMessageSent = false;
							nextPlayer->firstMapSent = false;
						}
					}
					else if( m.type == VOGM ) {
						if( nextPlayer->vogMode ) {
							nextPlayer->xd = m.x;
							nextPlayer->yd = m.y;

							nextPlayer->xs = m.x;
							nextPlayer->ys = m.y;

							char *message = autoSprintf( "VU\n%d %d\n#",
														 nextPlayer->xs -
														 nextPlayer->birthPos.x,
														 nextPlayer->ys -
														 nextPlayer->birthPos.y );
							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );

							delete [] message;
						}
					}
					else if( m.type == VOGI ) {
						if( nextPlayer->vogMode ) {
							if( m.id > 0 &&
							getObject( m.id ) != NULL ) {

								setMapObject( m.x, m.y, m.id );
							}
						}
					}
					else if( m.type == VOGT && m.saidText != NULL ) {
						if( nextPlayer->vogMode ) {

							newLocationSpeech.push_back(
									stringDuplicate( m.saidText ) );
							GridPos p = getPlayerPos( nextPlayer );

							ChangePosition cp;
							cp.x = p.x;
							cp.y = p.y;
							cp.global = false;

							newLocationSpeechPos.push_back( cp );
						}
					}
					else if( m.type == VOGX ) {
						if( nextPlayer->vogMode ) {
							nextPlayer->vogMode = false;

							GridPos p = nextPlayer->preVogPos;

							nextPlayer->xd = p.x;
							nextPlayer->yd = p.y;

							nextPlayer->xs = p.x;
							nextPlayer->ys = p.y;

							nextPlayer->birthPos = nextPlayer->preVogBirthPos;

							// send them one last VU message to move them
							// back instantly
							char *message = autoSprintf( "VU\n%d %d\n#",
														 nextPlayer->xs -
														 nextPlayer->birthPos.x,
														 nextPlayer->ys -
														 nextPlayer->birthPos.y );
							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );

							delete [] message;

							nextPlayer->postVogMode = true;
							nextPlayer->firstMessageSent = false;
							nextPlayer->firstMapSent = false;
						}
					}
					else if( nextPlayer->vogMode ) {
						// ignore non-VOG messages from them
					}
					else if( m.type == FORCE ) {
						if( m.x == nextPlayer->xd &&
						m.y == nextPlayer->yd ) {

							// player has ack'ed their forced pos correctly

							// stop ignoring their messages now
							nextPlayer->waitingForForceResponse = false;
						}
						else {
							AppLog::infoF(
									"FORCE message has unexpected "
									"absolute pos (%d,%d), expecting (%d,%d)",
									m.x, m.y,
									nextPlayer->xd, nextPlayer->yd );
						}
					}
					else if( m.type == PING ) {
						// immediately send pong
						char *message = autoSprintf( "PONG\n%d#", m.id );

						sendMessageToPlayer( nextPlayer, message,
											 strlen( message ) );
						delete [] message;
					}
					else if( m.type == DIE ) {
						if( computeAge( nextPlayer ) < 2 ) {

							// killed self
							// SID triggers a lineage ban
							nextPlayer->suicide = true;

							setDeathReason( nextPlayer, "SID" );

							nextPlayer->error = true;
							nextPlayer->errorCauseString = "Baby suicide";
							int parentID = nextPlayer->parentID;

							LiveObject *parentO =
									getLiveObject( parentID );

							if( parentO != NULL && nextPlayer->everHeldByParent ) {
								// mother picked up this SID baby at least
								// one time
								// mother can have another baby right away
								parentO->birthCoolDown = 0;
							}

							if( parentO != NULL &&
							parentO->lastSidsBabyEmail != NULL ) {
								delete [] parentO->lastSidsBabyEmail;
								parentO->lastSidsBabyEmail = NULL;
							}

							// walk through all other players and clear THIS
							// player from their SIDS mememory
							// we only track the most recent parent who had this
							// baby SIDS
							for( int p=0; p<players.size(); p++ ) {
								LiveObject *parent = players.getElement( p );

								if( parent->lastSidsBabyEmail != NULL &&
								strcmp( parent->lastSidsBabyEmail,
										nextPlayer->email ) == 0 ) {
									delete [] parent->lastSidsBabyEmail;
									parent->lastSidsBabyEmail = NULL;
								}
							}

							if( parentO != NULL ) {
								parentO->lastSidsBabyEmail =
										stringDuplicate( nextPlayer->email );
							}

							int holdingAdultID = nextPlayer->heldByOtherID;

							LiveObject *adult = NULL;
							if( nextPlayer->heldByOther ) {
								adult = getLiveObject( holdingAdultID );
							}

							int babyBonesID =
									SettingsManager::getIntSetting(
											"babyBones", -1 );

							if( adult != NULL ) {

								if( babyBonesID != -1 ) {
									ObjectRecord *babyBonesO =
											getObject( babyBonesID );

									if( babyBonesO != NULL ) {

										// don't leave grave on ground just yet
										nextPlayer->customGraveID = 0;

										GridPos adultPos =
												getPlayerPos( adult );

										// put invisible grave there for now
										// find an empty spot for this grave
										// where there's no grave already
										GridPos gravePos = adultPos;

										// give up after 100 steps
										// huge graveyard around?
										int stepCount = 0;
										while( getGravePlayerID(
												gravePos.x,
												gravePos.y ) > 0 &&
												stepCount < 100 ) {
											gravePos.x ++;
											stepCount ++;
										}

										GraveInfo graveInfo =
												{ gravePos,
												  nextPlayer->id,
												  nextPlayer->lineageEveID };
										newGraves.push_back( graveInfo );

										setGravePlayerID(
												gravePos.x, gravePos.y,
												nextPlayer->id );


										playerIndicesToSendUpdatesAbout.push_back(
												getLiveObjectIndex( holdingAdultID ) );

										// what if baby wearing clothes?
										for( int c=0;
										c < NUM_CLOTHING_PIECES;
										c++ ) {

											ObjectRecord *cObj = clothingByIndex(
													nextPlayer->clothing, c );

											if( cObj != NULL ) {
												// put clothing in adult's hand
												// and then drop
												adult->holdingID = cObj->id;
												if( nextPlayer->
												clothingContained[c].
												size() > 0 ) {

													adult->numContained =
															nextPlayer->
															clothingContained[c].
															size();

													adult->containedIDs =
															nextPlayer->
															clothingContained[c].
															getElementArray();
													adult->containedEtaDecays =
															nextPlayer->
															clothingContainedEtaDecays
															[c].
															getElementArray();

													adult->subContainedIDs
													= new
															SimpleVector<int>[
																	adult->numContained ];
													adult->subContainedEtaDecays
													= new
															SimpleVector<timeSec_t>[
																	adult->numContained ];
												}

												handleDrop(
														adultPos.x, adultPos.y,
														adult,
														NULL );
											}
										}

										// finally leave baby bones
										// in their hands
										adult->holdingID = babyBonesID;

										setHeldGraveOrigin( adult,
															gravePos.x,
															gravePos.y,
															0 );


										// this works to force client to play
										// creation sound for baby bones.
										adult->heldTransitionSourceID =
												nextPlayer->displayID;

										nextPlayer->heldByOther = false;
									}
								}
							}
							else {

								int babyBonesGroundID =
										SettingsManager::getIntSetting(
												"babyBonesGround", -1 );

								if( babyBonesGroundID != -1 ) {
									nextPlayer->customGraveID = babyBonesGroundID;
								}
								else if( babyBonesID != -1 ) {
									// else figure out what the held baby bones
									// become when dropped on ground
									TransRecord *groundTrans =
											getPTrans( babyBonesID, -1 );

									if( groundTrans != NULL &&
									groundTrans->newTarget > 0 ) {

										nextPlayer->customGraveID =
												groundTrans->newTarget;
									}
								}
								// else just use standard grave
							}
						}
					}
					else if( m.type == GRAVE ) {
						// immediately send GO response

						int id = getGravePlayerID( m.x, m.y );

						DeadObject *o = NULL;
						for( int i=0; i<pastPlayers.size(); i++ ) {
							DeadObject *oThis = pastPlayers.getElement( i );

							if( oThis->id == id ) {
								o = oThis;
								break;
							}
						}

						SimpleVector<int> *defaultLineage =
								new SimpleVector<int>();

						defaultLineage->push_back( 0 );
						DeadObject defaultO =
								{ 0,
								  0,
								  stringDuplicate( "~" ),
								  defaultLineage,
								  0,
								  0 };

						if( o == NULL ) {
							// check for living player too
							for( int i=0; i<players.size(); i++ ) {
								LiveObject *oThis = players.getElement( i );

								if( oThis->id == id ) {
									defaultO.id = oThis->id;
									defaultO.displayID = oThis->displayID;

									if( oThis->name != NULL ) {
										delete [] defaultO.name;
										defaultO.name =
												stringDuplicate( oThis->name );
									}

									defaultO.lineage->push_back_other(
											oThis->lineage );

									defaultO.lineageEveID = oThis->lineageEveID;
									defaultO.lifeStartTimeSeconds =
											oThis->lifeStartTimeSeconds;
									defaultO.deathTimeSeconds =
											oThis->deathTimeSeconds;
								}
							}
						}


						if( o == NULL ) {
							o = &defaultO;
						}

						if( o != NULL ) {
							char *formattedName;

							if( o->name != NULL ) {
								char found;
								formattedName =
										replaceAll( o->name, " ", "_", &found );
							}
							else {
								formattedName = stringDuplicate( "~" );
							}

							SimpleVector<char> linWorking;

							for( int j=0; j<o->lineage->size(); j++ ) {
								char *mID =
										autoSprintf(
												" %d",
												o->lineage->getElementDirect( j ) );
								linWorking.appendElementString( mID );
								delete [] mID;
							}
							char *linString = linWorking.getElementString();

							double age;

							if( o->deathTimeSeconds > 0 ) {
								// "age" in years since they died
								age = computeAge( o->deathTimeSeconds );
							}
							else {
								// grave of unknown person
								// let client know that age is bogus
								age = -1;
							}

							char *message = autoSprintf(
									"GO\n%d %d %d %d %lf %s%s eve=%d\n#",
									m.x - nextPlayer->birthPos.x,
									m.y - nextPlayer->birthPos.y,
									o->id, o->displayID,
									age,
									formattedName, linString,
									o->lineageEveID );
							printf( "Processing %d,%d from birth pos %d,%d\n",
									m.x, m.y, nextPlayer->birthPos.x,
									nextPlayer->birthPos.y );

							delete [] formattedName;
							delete [] linString;

							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );
							delete [] message;
						}

						delete [] defaultO.name;
						delete defaultO.lineage;
					}
					else if( m.type == OWNER ) {
						// immediately send OW response
						SimpleVector<char> messageWorking;
						messageWorking.appendElementString( "OW\n" );

						char *leadString =
								autoSprintf( "%d %d",
											 m.x - nextPlayer->birthPos.x,
											 m.y - nextPlayer->birthPos.y );
						messageWorking.appendElementString( leadString );
						delete [] leadString;

						char *ownerString = getOwnershipString( m.x, m.y );
						messageWorking.appendElementString( ownerString );
						delete [] ownerString;

						messageWorking.appendElementString( "\n#" );
						char *message = messageWorking.getElementString();

						sendMessageToPlayer( nextPlayer, message,
											 strlen( message ) );
						delete [] message;

						GridPos p = { m.x, m.y };

						if( ! isKnownOwned( nextPlayer, p ) ) {
							// remember that we know about it
							nextPlayer->knownOwnedPositions.push_back( p );
						}
					}
					else if( m.type == PHOTO ) {
						// immediately send photo response

						char *photoServerSharedSecret =
								SettingsManager::
								getStringSetting( "photoServerSharedSecret",
												  "secret_phrase" );

						char *idString = autoSprintf( "%d", m.id );

						char *hash;

						// is a photo device present at x and y?
						char photo = false;

						int oID = getMapObject( m.x, m.y );

						if( oID > 0 ) {
							if( strstr( getObject( oID )->description,
										"+photo" ) != NULL ) {
								photo = true;
							}
						}

						if( ! photo ) {
							hash = hmac_sha1( "dummy", idString );
						}
						else {
							hash = hmac_sha1( photoServerSharedSecret, idString );
						}

						delete [] photoServerSharedSecret;
						delete [] idString;

						char *message = autoSprintf( "PH\n%d %d %s#",
													 m.x, m.y, hash );

						delete [] hash;

						sendMessageToPlayer( nextPlayer, message,
											 strlen( message ) );
						delete [] message;
					}
					else if( m.type == LEAD ) {
						LiveObject *topLeaderO =
								getLiveObject( getTopLeader( nextPlayer ) );

						if( topLeaderO != NULL && topLeaderO != nextPlayer ) {


							if( ! isExiled( topLeaderO, nextPlayer ) ) {

								// they have a leader and haven't been exiled
								// by that leader

								// give them an arrow toward that leader

								GridPos lPos = getPlayerPos( topLeaderO );

								char *topLeaderName =
										getLeadershipName( topLeaderO );

								char *psMessage =
										autoSprintf( "PS\n"
													 "%d/0 MY %s "
													 "*leader %d *map %d %d\n#",
													 nextPlayer->id,
													 topLeaderName,
													 topLeaderO->id,
													 lPos.x - nextPlayer->birthPos.x,
													 lPos.y - nextPlayer->birthPos.y );

								delete [] topLeaderName;

								sendMessageToPlayer( nextPlayer,
													 psMessage,
													 strlen( psMessage ) );
								delete [] psMessage;
							}
							else {
								char *psMessage =
										autoSprintf( "PS\n"
													 "%d/0 +EXILED+\n#",
													 nextPlayer->id );

								sendMessageToPlayer( nextPlayer,
													 psMessage,
													 strlen( psMessage ) );
								delete [] psMessage;
							}
						}
						else {
							char *psMessage =
									autoSprintf( "PS\n"
												 "%d/0 +NO LEADER+\n#",
												 nextPlayer->id );
							sendMessageToPlayer( nextPlayer,
												 psMessage, strlen( psMessage ) );
							delete [] psMessage;
						}
					}
					else if( m.type == UNFOL ) {
						// following no one
						nextPlayer->followingID = -1;
						nextPlayer->followingUpdate = true;
						char *psMessage =
								autoSprintf( "PS\n"
											 "%d/0 +NO LEADER+\n#",
											 nextPlayer->id );
						sendMessageToPlayer( nextPlayer,
											 psMessage, strlen( psMessage ) );
						delete [] psMessage;
					}
					else if( m.type == FLIP ) {

						if( currentTime - nextPlayer->lastFlipTime > 1.75 ) {
							// client should send at most one flip ever 2 seconds
							// allow some wiggle room
							GridPos p = getPlayerPos( nextPlayer );

							int oldFacingLeft = nextPlayer->facingLeft;

							if( m.x > p.x ) {
								nextPlayer-> facingLeft = 0;
							}
							else if( m.x < p.x ) {
								nextPlayer->facingLeft = 1;
							}

							if( oldFacingLeft != nextPlayer->facingLeft ) {
								nextPlayer->lastFlipTime = currentTime;
								newFlipPlayerIDs.push_back( nextPlayer->id );
								newFlipFacingLeft.push_back(
										nextPlayer->facingLeft );
								newFlipPositions.push_back( p );
							}
						}
					}
					else if( m.type != SAY && m.type != EMOT &&
					nextPlayer->waitingForForceResponse ) {
						// if we're waiting for a FORCE response, ignore
						// all messages from player except SAY and EMOT

						AppLog::infoF( "Ignoring client message because we're "
									   "waiting for FORCE ack message after a "
									   "forced-pos PU at (%d, %d), "
									   "relative=(%d, %d)",
									   nextPlayer->xd, nextPlayer->yd,
									   nextPlayer->xd - nextPlayer->birthPos.x,
									   nextPlayer->yd - nextPlayer->birthPos.y );
					}
					// if player is still moving (or held by an adult),
					// ignore all actions
					// except for move interrupts
					else if( ( nextPlayer->xs == nextPlayer->xd &&
					nextPlayer->ys == nextPlayer->yd &&
					! nextPlayer->heldByOther )
					||
					m.type == MOVE ||
					m.type == JUMP ||
					m.type == SAY ||
					m.type == EMOT ) {

						if( m.type == MOVE &&
						m.sequenceNumber != -1 ) {
							nextPlayer->lastMoveSequenceNumber = m.sequenceNumber;
						}

						if( ( m.type == MOVE || m.type == JUMP ) &&
						nextPlayer->heldByOther ) {

							// only JUMP actually makes them jump out
							if( m.type == JUMP ) {
								// baby wiggling out of parent's arms

								// block them from wiggling from their own
								// mother's arms if they are under 1

								if( computeAge( nextPlayer ) >= 1  ||
								nextPlayer->heldByOtherID !=
								nextPlayer->parentID ) {

									handleForcedBabyDrop(
											nextPlayer,
											&playerIndicesToSendUpdatesAbout );
								}
								else {
									// baby wiggles
									nextPlayer->wiggleUpdate = true;
								}
							}

							// ignore their move requests while
							// in-arms, until they JUMP out
						}
						else if( m.type == JUMP &&
						computeAge( nextPlayer ) < startWalkingAge ) {
							// tiny infant wiggling on ground
							nextPlayer->wiggleUpdate = true;
						}
						else if( m.type == MOVE &&
						computeAge( nextPlayer ) < startWalkingAge ) {
							// ignore moves for the tiniest infants
						}
						else if( m.type == MOVE && nextPlayer->holdingID > 0 &&
						getObject( nextPlayer->holdingID )->
						speedMult == 0 ) {
							// next player holding something that prevents
							// movement entirely
							printf( "  Processing move, "
									"but player holding a speed-0 object, "
									"ending now\n" );
							nextPlayer->xd = nextPlayer->xs;
							nextPlayer->yd = nextPlayer->ys;

							nextPlayer->posForced = true;

							// send update about them to end the move
							// right now
							playerIndicesToSendUpdatesAbout.push_back( i );
						}
						else if( m.type == MOVE ) {
							//Thread::staticSleep( 1000 );

							/*
							printf( "  Processing move, "
									"we think player at old start pos %d,%d\n",
									nextPlayer->xs,
									nextPlayer->ys );
							printf( "  Player's last path = " );
							for( int p=0; p<nextPlayer->pathLength; p++ ) {
								printf( "(%d, %d) ",
										nextPlayer->pathToDest[p].x,
										nextPlayer->pathToDest[p].y );
								}
							printf( "\n" );
							*/

							char interrupt = false;
							char pathPrefixAdded = false;


							// where exactly did we used to be standing?
							doublePair startPosPrecise =
									computePartialMoveSpotPrecise( nextPlayer );


							// first, construct a path from any existing
							// path PLUS path that player is suggesting
							SimpleVector<GridPos> unfilteredPath;

							if( nextPlayer->pathLength > 0 &&
							nextPlayer->pathToDest != NULL &&
							( nextPlayer->xs != m.x ||
							nextPlayer->ys != m.y ) ) {

								// start pos of their submitted path
								// donesn't match where we think they are

								// it could be an interrupt to past move
								// OR, if our server sees move as done but client
								// doesn't yet, they may be sending a move
								// from the middle of their last path.

								// treat this like an interrupt to last move
								// in both cases.

								// a new move interrupting a non-stationary object
								interrupt = true;

								// where we think they are along last move path
								GridPos cPos;
								int c;

								if( nextPlayer->xs != nextPlayer->xd
								||
								nextPlayer->ys != nextPlayer->yd ) {

									// a real interrupt to a move that is
									// still in-progress on server
									c = computePartialMovePathStep( nextPlayer );
									cPos = computePartialMoveSpot( nextPlayer, c );
								}
								else {
									// we think their last path is done
									cPos.x = nextPlayer->xs;
									cPos.y = nextPlayer->ys;
									// we think they are on final destination
									// spot on last path
									c = nextPlayer->pathLength - 1;
								}

								/*
								printf( "   we think player in motion or "
										"done moving at %d,%d\n",
										cPos.x,
										cPos.y );
								*/
								nextPlayer->xs = cPos.x;
								nextPlayer->ys = cPos.y;


								char cOnTheirNewPath = false;


								for( int p=0; p<m.numExtraPos; p++ ) {
									if( equal( cPos, m.extraPos[p] ) ) {
										cOnTheirNewPath = true;
										break;
									}
								}

								if( cPos.x == m.x && cPos.y == m.y ) {
									// also if equal to their start pos
									cOnTheirNewPath = true;
								}



								if( !cOnTheirNewPath &&
								nextPlayer->pathLength > 0 ) {

									// add prefix to their path from
									// c to the start of their path

									// index where they think they are

									// could be ahead or behind where we think
									// they are

									int theirPathIndex = -1;

									for( int p=0; p<nextPlayer->pathLength; p++ ) {
										GridPos pos = nextPlayer->pathToDest[p];

										if( m.x == pos.x && m.y == pos.y ) {
											// reached point along old path
											// where player thinks they
											// actually are
											theirPathIndex = p;
											break;
										}
									}

									char theirIndexNotFound = false;

									if( theirPathIndex == -1 ) {
										// if not found, assume they think they
										// are at start of their old path

										theirIndexNotFound = true;
										theirPathIndex = 0;
									}

									/*
									printf( "They are on our path at index %d\n",
											theirPathIndex );
									*/

									// okay, they think they are on last path
									// that we had for them

									// step through path from where WE
									// think they should be to where they
									// think they are and add this as a prefix
									// to the path they submitted
									// (we may walk backward along the old
									//  path to do this)


									// -1 means starting, pre-path
									// pos is closest
									// but okay to leave c at -1, because
									// we will add pathStep=1 to it

									int pathStep = 0;

									if( theirPathIndex < c ) {
										pathStep = -1;
									}
									else if( theirPathIndex > c ) {
										pathStep = 1;
									}

									if( pathStep != 0 ) {

										if( c == -1 ) {
											// fix weird case where our start
											// pos is on our path
											// not sure what causes this
											// but it causes the valid path
											// check to fail below
											int firstStep = c + pathStep;
											GridPos firstPos =
													nextPlayer->pathToDest[ firstStep ];

											if( firstPos.x == nextPlayer->xs &&
											firstPos.y == nextPlayer->ys ) {
												c = 0;
											}
										}

										for( int p = c + pathStep;
										p != theirPathIndex + pathStep;
										p += pathStep ) {
											GridPos pos =
													nextPlayer->pathToDest[p];

											unfilteredPath.push_back( pos );
										}
									}

									if( theirIndexNotFound ) {
										// add their path's starting pos
										// at the end of the prefix
										GridPos pos = { m.x, m.y };

										unfilteredPath.push_back( pos );
									}

									// otherwise, they are where we think
									// they are, and we don't need to prefix
									// their path

									/*
									printf( "Prefixing their path "
											"with %d steps\n",
											unfilteredPath.size() );
									*/
								}
							}

							if( unfilteredPath.size() > 0 ) {
								pathPrefixAdded = true;
							}

							// now add path player says they want to go down

							for( int p=0; p < m.numExtraPos; p++ ) {
								unfilteredPath.push_back( m.extraPos[p] );
							}

							/*
							printf( "Unfiltered path = " );
							for( int p=0; p<unfilteredPath.size(); p++ ) {
								printf( "(%d, %d) ",
										unfilteredPath.getElementDirect(p).x,
										unfilteredPath.getElementDirect(p).y );
								}
							printf( "\n" );
							*/

							// remove any duplicate spots due to doubling back

							for( int p=1; p<unfilteredPath.size(); p++ ) {

								if( equal( unfilteredPath.getElementDirect(p-1),
										   unfilteredPath.getElementDirect(p) ) ) {
									unfilteredPath.deleteElement( p );
									p--;
									//printf( "FOUND duplicate path element\n" );
								}
							}




							nextPlayer->xd = m.extraPos[ m.numExtraPos - 1].x;
							nextPlayer->yd = m.extraPos[ m.numExtraPos - 1].y;


							if( nextPlayer->xd == nextPlayer->xs &&
							nextPlayer->yd == nextPlayer->ys ) {
								// this move request truncates to where
								// we think player actually is

								// send update to terminate move right now
								playerIndicesToSendUpdatesAbout.push_back( i );
								/*
								printf( "A move that takes player "
										"where they already are, "
										"ending move now\n" );
								*/
							}
							else {
								// an actual move away from current xs,ys

								if( interrupt ) {
									//printf( "Got valid move interrupt\n" );
								}


								// check path for obstacles
								// and make sure it contains the location
								// where we think they are

								char truncated = 0;

								SimpleVector<GridPos> validPath;

								char startFound = false;


								int startIndex = 0;
								// search from end first to find last occurrence
								// of start pos
								for( int p=unfilteredPath.size() - 1; p>=0; p-- ) {

									if( unfilteredPath.getElementDirect(p).x
									== nextPlayer->xs
									&&
									unfilteredPath.getElementDirect(p).y
									== nextPlayer->ys ) {

										startFound = true;
										startIndex = p;
										break;
									}
								}
								/*
								printf( "Start index = %d (startFound = %d)\n",
										startIndex, startFound );
								*/

								if( ! startFound &&
								! isGridAdjacentDiag(
										unfilteredPath.
										getElementDirect(startIndex).x,
										unfilteredPath.
										getElementDirect(startIndex).y,
										nextPlayer->xs,
										nextPlayer->ys ) ) {
									// path start jumps away from current player
									// start
									// ignore it
								}
								else {

									GridPos lastValidPathStep =
											{ m.x, m.y };

									if( pathPrefixAdded ) {
										lastValidPathStep.x = nextPlayer->xs;
										lastValidPathStep.y = nextPlayer->ys;
									}

									// we know where we think start
									// of this path should be,
									// but player may be behind this point
									// on path (if we get their message late)
									// So, it's not safe to pre-truncate
									// the path

									// However, we will adjust timing, below,
									// to match where we think they should be

									// enforce client behavior of not walking
									// down through objects in our cell that are
									// blocking us
									char currentBlocked = false;

									if( isMapSpotBlockingForPlayer(
											nextPlayer,
											lastValidPathStep.x,
											lastValidPathStep.y ) ) {
										currentBlocked = true;
									}


									for( int p=0;
									p<unfilteredPath.size(); p++ ) {

										GridPos pos =
												unfilteredPath.getElementDirect(p);

										if( isMapSpotBlockingForPlayer(
												nextPlayer, pos.x, pos.y ) ) {
											// blockage in middle of path
											// terminate path here
											truncated = 1;
											break;
										}

										if( currentBlocked && p == 0 &&
										pos.y == lastValidPathStep.y - 1 ) {
											// attempt to walk down through
											// blocking object at starting location
											truncated = 1;
											break;
										}


										// make sure it's not more
										// than one step beyond
										// last step

										if( ! isGridAdjacentDiag(
												pos, lastValidPathStep ) ) {
											// a path with a break in it
											// terminate it here
											truncated = 1;
											break;
										}

										// no blockage, no gaps, add this step
										validPath.push_back( pos );
										lastValidPathStep = pos;
									}
								}

								if( validPath.size() == 0 ) {
									// path not permitted
									AppLog::info( "Path submitted by player "
												  "not valid, "
												  "ending move now" );
									//assert( false );
									nextPlayer->xd = nextPlayer->xs;
									nextPlayer->yd = nextPlayer->ys;

									nextPlayer->posForced = true;

									// send update about them to end the move
									// right now
									playerIndicesToSendUpdatesAbout.push_back( i );
								}
								else {
									// a good path

									if( nextPlayer->pathToDest != NULL ) {
										delete [] nextPlayer->pathToDest;
										nextPlayer->pathToDest = NULL;
									}

									nextPlayer->pathTruncated = truncated;

									nextPlayer->pathLength = validPath.size();

									nextPlayer->pathToDest =
											validPath.getElementArray();

									// path may be truncated from what was
									// requested, so set new d
									nextPlayer->xd =
											nextPlayer->pathToDest[
													nextPlayer->pathLength - 1 ].x;
									nextPlayer->yd =
											nextPlayer->pathToDest[
													nextPlayer->pathLength - 1 ].y;

									// distance is number of orthogonal steps

									double dist =
											measurePathLength( nextPlayer->xs,
															   nextPlayer->ys,
															   nextPlayer->pathToDest,
															   nextPlayer->pathLength );

									nextPlayer->pathDist = dist;


									// get precise about distance for move timing
									// we don't necessarily start right
									// at naiveStart, but often some distance
									// along (or further behind)
									GridPos naiveStart;

									if( startIndex > 0 ) {
										naiveStart =
												nextPlayer->pathToDest[ startIndex -1 ];
									}
									else {
										naiveStart.x = nextPlayer->xs;
										naiveStart.y = nextPlayer->ys;
									}

									double naiveStartDist =
											distance(
													naiveStart,
													nextPlayer->pathToDest[startIndex] );

									// subtract out this naive
									// first-step distance
									// before adding in the true distance
									dist -= naiveStartDist;

									doublePair newFirstSpot =
											{ (double)
											  nextPlayer->pathToDest[startIndex].x,
											  (double)
											  nextPlayer->pathToDest[startIndex].y };


									// now add in true distance to first spot
									dist +=
											distance( startPosPrecise, newFirstSpot );




									double distAlreadyDone =
											measurePathLength( nextPlayer->xs,
															   nextPlayer->ys,
															   nextPlayer->pathToDest,
															   startIndex );

									double moveSpeed = computeMoveSpeed(
											nextPlayer ) *
													getPathSpeedModifier(
															nextPlayer->pathToDest,
															nextPlayer->pathLength );

									nextPlayer->moveTotalSeconds = dist /
											moveSpeed;

									if( nextPlayer->moveTotalSeconds <= 0.1 ) {
										// never allow moveTotalSeconds to be
										// 0, too small, or negative
										// (we divide by it in certain
										// calculations)
										nextPlayer->moveTotalSeconds = 0.1;
									}

									double secondsAlreadyDone = distAlreadyDone /
											moveSpeed;
									/*
									printf( "Skipping %f seconds along new %f-"
											"second path\n",
											secondsAlreadyDone,
											nextPlayer->moveTotalSeconds );
									*/
									nextPlayer->moveStartTime =
											Time::getCurrentTime() -
											secondsAlreadyDone;

									nextPlayer->newMove = true;


									// check if path passes over
									// an object with autoDefaultTrans
									for( int p=0; p< nextPlayer->pathLength; p++ ) {
										int x = nextPlayer->pathToDest[p].x;
										int y = nextPlayer->pathToDest[p].y;

										int oID = getMapObject( x, y );

										if( oID > 0 &&
										getObject( oID )->autoDefaultTrans ) {
											TransRecord *t = getPTrans( -2, oID );

											if( t == NULL ) {
												// also consider applying bare-hand
												// action, if defined and if
												// it produces nothing in the hand
												t = getPTrans( 0, oID );

												if( t != NULL &&
												t->newActor > 0 ) {
													t = NULL;
												}
											}

											if( t != NULL && t->newTarget > 0 ) {
												int newTarg = t->newTarget;
												setMapObject( x, y, newTarg );

												TransRecord *timeT =
														getPTrans( -1, newTarg );

												if( timeT != NULL &&
												timeT->autoDecaySeconds < 20 ) {
													// target will decay to
													// something else in a short
													// time
													// Likely meant to reset
													// after person passes through

													// fix the time based on our
													// pass-through time
													double timeLeft =
															nextPlayer->moveTotalSeconds
															- secondsAlreadyDone;

													double plannedETADecay =
															Time::getCurrentTime()
															+ timeLeft
															// pad with extra second
															+ 1;

													timeSec_t actual =
															getEtaDecay( x, y );

													// don't ever shorten
													// we could be interrupting
													// another player who
													// is on a longer path
													// through the same object
													if( plannedETADecay >
													actual ) {
														setEtaDecay(
																x, y, plannedETADecay );
													}
												}
											}
										}
									}



									// check if this move goes into a bad biome
									// and makes them sick
									int sicknessObjectID = -1;

									// but only if an earlier part of their path
									// was not a sick-making biome
									// i.e., only if their path passes INTO
									// a sick-making biome, not if it starts there
									// (they might start in a bad biome for various
									//  reasons, like if a posse breaks up)

									char nonSickPathStart = false;

									int curLocSicknessObjectID =
											getBiomeSickness(
													nextPlayer->displayID,
													nextPlayer->xs,
													nextPlayer->ys );

									if( curLocSicknessObjectID == -1 ) {
										nonSickPathStart = true;
									}

									for( int p=0; p< nextPlayer->pathLength; p++ ) {

										sicknessObjectID =
												getBiomeSickness(
														nextPlayer->displayID,
														nextPlayer->pathToDest[p].x,
														nextPlayer->pathToDest[p].y );

										if( sicknessObjectID != -1 ) {
											break;
										}
										else {
											// some path step before sickness
											// was non-sickness
											nonSickPathStart = true;
										}

									}


									if( ! nonSickPathStart &&
									! nextPlayer->holdingBiomeSickness ) {
										// path starts in sick biome
										// and player NOT already sick
										// don't make them sick now, until
										// they cross back in from outside later
										sicknessObjectID = -1;
									}


									if( nextPlayer->vogMode ||
									nextPlayer->forceSpawn ||
									nextPlayer->isTutorial ) {
										// these special-case players never
										// have biome sickness
										sicknessObjectID = -1;
									}

									// riding something prevents sickness
									if( sicknessObjectID > 0 &&
									nextPlayer->holdingID > 0 &&
									getObject( nextPlayer->holdingID )->
									rideable ) {

										sicknessObjectID = -1;
									}


									// being part of a full-size posse prevents
									// sicness
									if( sicknessObjectID > 0 ) {
										KillState *ks = getKillState( nextPlayer );

										if( ks != NULL &&
										ks->posseSize >=
										ks->minPosseSizeForKill ) {
											sicknessObjectID = -1;
										}
									}


									if( sicknessObjectID > 0 &&
									! nextPlayer->holdingWound &&
									nextPlayer->holdingID !=
									sicknessObjectID ) {

										// drop what they are holding
										if( nextPlayer->holdingID != 0 ) {
											// never drop held wounds
											// or neverDrop murder weapons

											if( ! nextPlayer->holdingWound &&
											! nextPlayer->
											holdingBiomeSickness &&
											! heldNeverDrop( nextPlayer ) ) {
												handleDrop(
														m.x, m.y, nextPlayer,
														&playerIndicesToSendUpdatesAbout );
											}
										}

										if( nextPlayer->holdingID == 0 ||
										nextPlayer->holdingBiomeSickness ) {
											// we dropped what they were holding
											// or they were holding a different
											// biome sickness, which we can now
											// freely replace

											makePlayerBiomeSick( nextPlayer,
																 sicknessObjectID );
											playerIndicesToSendUpdatesAbout.
											push_back( i );
										}
									}
									else if( sicknessObjectID == -1 &&
									nextPlayer->holdingBiomeSickness ) {

										endBiomeSickness(
												nextPlayer, i,
												&playerIndicesToSendUpdatesAbout );
									}

									if( sicknessObjectID == -1 &&
									! nextPlayer->emotFrozen ) {
										// check if path starts/ends
										// in/out of home

										int homeStart =
												isBirthland(
														nextPlayer->xs,
														nextPlayer->ys,
														nextPlayer->lineageEveID,
														nextPlayer->displayID );

										int endStep = nextPlayer->pathLength - 1;

										int homeEnd =
												isBirthland(
														nextPlayer->pathToDest[endStep].x,
														nextPlayer->pathToDest[endStep].y,
														nextPlayer->lineageEveID,
														nextPlayer->displayID );

										char boundaryCross = false;

										// skip this now
										// used to want to emphasize being homesick
										// again when ENTERING another homeland
										// but we don't need to do this anymore
										if( false &&
										homeStart == homeEnd &&
										homeEnd == -1 ) {
											// player still outside homeland
											// but did they cross a boundary
											// into some other homeland?

											int lineageA = 0;
											int lineageB = 0;
											GridPos dummyCenter;

											getHomelandCenter(
													nextPlayer->xs,
													nextPlayer->ys,
													&dummyCenter,
													&lineageA );
											getHomelandCenter(
													nextPlayer->pathToDest[endStep].x,
													nextPlayer->pathToDest[endStep].y,
													&dummyCenter,
													&lineageB );

											// even if B is -1, we have a boundary
											// cross
											if( lineageA != lineageB ) {
												boundaryCross = true;
											}
										}

										if( homeStart != homeEnd ) {
											boundaryCross = true;

											int newEmotIndex = -1;
											const char *speechWord = NULL;

											if( homeEnd == -1 ) {
												newEmotIndex =
														SettingsManager::
														getIntSetting(
																"homesickEmotionIndex",
																-1 );
												speechWord = "HOMESICK";
												// don't enforce the every-homesick
												// then homesick outside
												// of homelands restriction for
												// no-homeland Eves
												if( ! nextPlayer->isEve ) {
													nextPlayer->everHomesick = true;
												}
											}
											else if(
													homeEnd == 1 ||
													( ! nextPlayer->everHomesick &&
													homeEnd == 0 ) ) {
												newEmotIndex =
														SettingsManager::
														getIntSetting(
																"homeEmotionIndex",
																-1 );

												if( homeEnd == 0 ) {
													// a nomad with no homeland
													speechWord = "FREE REIN";
												}
												else {
													// returning to homeland
													speechWord = "HOME";
												}
											}

											if( newEmotIndex != -1 ) {
												newEmotPlayerIDs.push_back(
														nextPlayer->id );

												newEmotIndices.push_back(
														newEmotIndex );
												// 5 sec
												newEmotTTLs.push_back( 5 );
											}

											if( speechWord != NULL ) {
												// put word above their head
												// (only for them to see)
												char *message = autoSprintf(
														"PS\n"
														"%d/0 +%s+\n#",
														nextPlayer->id, speechWord );
												sendMessageToPlayer(
														nextPlayer,
														message,
														strlen( message ) );
												delete [] message;
											}
										}

										if( boundaryCross ) {
											// when player crosses boundary
											// check if they've entered a homeland
											// tell them about the center
											GridPos homeCenter;
											int homeLineageEveID;
											char isSomeHomeland =
													getHomelandCenter(
															nextPlayer->pathToDest[endStep].x,
															nextPlayer->pathToDest[endStep].y,
															&homeCenter,
															&homeLineageEveID );

											if( isSomeHomeland ) {
												// send them HL message
												sendHomelandMessage(
														nextPlayer,
														homeLineageEveID,
														homeCenter );
											}
										}


									}
								}
							}
						}
						else if( m.type == SAY && m.saidText != NULL &&
						Time::getCurrentTime() -
						nextPlayer->lastSayTimeSeconds >
						minSayGapInSeconds ) {

							nextPlayer->lastSayTimeSeconds =
									Time::getCurrentTime();

							unsigned int sayLimit = getSayLimit( nextPlayer );

							if( strlen( m.saidText ) > sayLimit ) {
								// truncate
								m.saidText[ sayLimit ] = '\0';
							}

							int len = strlen( m.saidText );

							// replace not-allowed characters with spaces
							for( int c=0; c<len; c++ ) {
								if( ! allowedSayCharMap[
										(int)( m.saidText[c] ) ] ) {

									m.saidText[c] = ' ';
								}
							}

							// now clean up gratuitous runs of spaces left behind
							// by removed characters (or submitted by a wayward
							// client)
							SimpleVector<char *> *tokens =
									tokenizeString( m.saidText );

							char *cleanedString;
							if( tokens->size() > 0 ) {

								char **tokensArray =
										tokens->getElementArray();

								// join words with single spaces
								cleanedString = join( tokensArray,
													  tokens->size(),
													  " " );

								tokens->deallocateStringElements();
								delete [] tokensArray;
							}
							else {
								cleanedString = stringDuplicate( "" );
							}

							delete tokens;

							delete [] m.saidText;
							m.saidText = cleanedString;


							if( nextPlayer->ownedPositions.size() > 0 ) {
								// consider phrases that assign ownership
								SimpleVector<LiveObject*> newOwners;


								char *namedOwner = isNamedGivingSay( m.saidText );

								if( namedOwner != NULL ) {
									LiveObject *o =
											getPlayerByName( namedOwner, nextPlayer );

									if( o != NULL ) {
										newOwners.push_back( o );
									}
									delete [] namedOwner;
								}

								if( newOwners.size() == 0 ) {

									if( isYouGivingSay( m.saidText ) ) {
										// find closest other player
										LiveObject *newOwnerPlayer =
												getClosestOtherPlayer( nextPlayer );

										if( newOwnerPlayer != NULL ) {
											newOwners.push_back( newOwnerPlayer );
										}
									}
									else if( isFamilyGivingSay( m.saidText ) ) {
										// add all family members
										for( int n=0; n<players.size(); n++ ) {
											LiveObject *o = players.getElement( n );
											if( o->error ||
											o->id == nextPlayer->id ) {
												continue;
											}
											if( o->lineageEveID ==
											nextPlayer->lineageEveID ) {
												newOwners.push_back( o );
											}
										}
									}
									else if( isOffspringGivingSay( m.saidText ) ) {
										// add all offspring
										for( int n=0; n<players.size(); n++ ) {
											LiveObject *o = players.getElement( n );
											if( o->error ||
											o->id == nextPlayer->id ) {
												continue;
											}
											if( o->parentID == nextPlayer->id ) {
												newOwners.push_back( o );
											}
										}
									}
								}


								if( newOwners.size() > 0 ) {
									// find closest spot that this player owns
									GridPos thisPos = getPlayerPos( nextPlayer );

									double minDist = DBL_MAX;

									GridPos closePos;

									for( int j=0;
									j< nextPlayer->ownedPositions.size();
									j++ ) {
										GridPos nextPos =
												nextPlayer->
												ownedPositions.getElementDirect( j );
										double d = distance( nextPos, thisPos );

										if( d < minDist ) {
											minDist = d;
											closePos = nextPos;
										}
									}

									if( minDist < DBL_MAX ) {
										// found one
										for( int n=0; n<newOwners.size(); n++ ) {
											LiveObject *newOwnerPlayer =
													newOwners.getElementDirect( n );

											if( ! isOwned( newOwnerPlayer,
														   closePos ) ) {
												newOwnerPlayer->
												ownedPositions.push_back(
														closePos );
												newOwnerPos.push_back( closePos );
											}
										}
									}
								}
							}


							// they must be holding something to join a posse
							// but not a wound or a sickness
							// nor a bloody weapon
							// what these non-working held items have in common
							// is that they are stuck in hand AND don't have
							// a use-on-bare ground transition defined
							char joiningPosse = false;
							if( isPosseJoiningSay( m.saidText ) ) {
								joiningPosse = true;
								if( nextPlayer->isTwin ) {
									const char *message =
											"TWINS CANNOT JOIN POSSES.";
									sendGlobalMessage( (char*)message, nextPlayer );
									joiningPosse = false;
								}
								else if( nextPlayer->isLastLifeShort ) {
									const char *message =
											"YOUR LAST LIFE WAS VERY SHORT.**"
											"YOU CANNOT JOIN POSSES IN THIS LIFE.";
									sendGlobalMessage( (char*)message, nextPlayer );
									joiningPosse = false;
								}
								else if( nextPlayer->holdingID <= 0  ||
								( getObject( nextPlayer->holdingID )->
								permanent &&
								getTrans( nextPlayer->holdingID, -
								1 ) == NULL ) ) {
									const char *message =
											"YOU MUST BE HOLDING SOMETHING "
											"TO JOIN A POSSE.";
									sendGlobalMessage( (char*)message, nextPlayer );
									joiningPosse = false;
								}

							}

							if( joiningPosse ) {

								GridPos ourPos = getPlayerPos( nextPlayer );

								// find closest player who is part of a KILL
								// record
								KillState *closestState = NULL;
								double closestDist = DBL_MAX;
								for( int i=0; i<activeKillStates.size(); i++ ) {
									KillState *s = activeKillStates.getElement( i );

									if( s->targetID == nextPlayer->id ) {
										// can't join posse targetting self
										continue;
									}
									if( s->killerID == nextPlayer->id ) {
										// can't join posse that we're already in
										continue;
									}

									if( s->posseSize == 1 &&
									isNoWaitWeapon( s->killerWeaponID ) ) {
										// single-person posse, where leader
										// not holding a deadly weapon
										// (snowball or tattoo needle)
										// Can't join this posse.
										continue;
									}

									LiveObject *killer =
											getLiveObject( s->killerID );

									GridPos killerPos = getPlayerPos( killer );

									double d = distance( killerPos, ourPos );

									if( d < 8 &&
									d < closestDist ) {
										// in range and closer
										closestState = s;
										closestDist = d;
									}
								}

								if( closestState != NULL &&
								! isAlreadyInKillState( nextPlayer ) ) {
									// they are joining, and they aren't already
									// in one.
									// infinite range
									removeAnyKillState( nextPlayer );

									char enteredState = addKillState(
											nextPlayer,
											getLiveObject( closestState->targetID ),
											true );
									if( enteredState ) {
										nextPlayer->emotFrozen = true;
										nextPlayer->emotFrozenIndex =
												killEmotionIndex;

										newEmotPlayerIDs.push_back(
												nextPlayer->id );
										newEmotIndices.push_back(
												killEmotionIndex );
										newEmotTTLs.push_back( 120 );
									}
								}
							}



							if( nextPlayer->isEve && nextPlayer->name == NULL ) {
								char *name = isFamilyNamingSay( m.saidText );

								if( name != NULL && strcmp( name, "" ) != 0 ) {
									nameEve( nextPlayer, name );
									playerIndicesToSendNamesAbout.push_back( i );
									replaceNameInSaidPhrase(
											name,
											&( m.saidText ),
											nextPlayer, true );

									if( ! isEveWindow() &&
									! nextPlayer->isTutorial &&
									nextPlayer->curseStatus.curseLevel == 0 ) {
										// new family name created
										restockPostWindowFamilies();
									}

									if( strstr( m.saidText, "EVE EVE" ) != NULL ) {
										// their naming phrase was I AM EVE SMITH
										// already
										char found;
										char *fixed =
												replaceOnce( m.saidText,
															 "EVE EVE",
															 "EVE",
															 &found );
										delete [] m.saidText;
										m.saidText = fixed;
									}
								}
							}



							LiveObject *otherToForgive = NULL;

							if( isYouForgivingSay( m.saidText ) ) {
								otherToForgive =
										getClosestOtherPlayer( nextPlayer );
							}
							else {
								char *forgiveName = isNamedForgivingSay( m.saidText );
								if( forgiveName != NULL ) {
									otherToForgive =
											getPlayerByName( forgiveName, nextPlayer );

								}
							}

							if( otherToForgive != NULL ) {
								clearDBCurse( nextPlayer->email,
											  otherToForgive->email );

								char *message =
										autoSprintf(
												"CU\n%d 0 %s_%s\n#",
												otherToForgive->id,
												getCurseWord( nextPlayer->email,
															  otherToForgive->email, 0 ),
															  getCurseWord( nextPlayer->email,
																			otherToForgive->email, 1 ) );
								sendMessageToPlayer( nextPlayer,
													 message, strlen( message ) );
								delete [] message;
							}


							LiveObject *otherToFollow = NULL;
							LiveObject *otherToExile = NULL;
							LiveObject *otherToRedeem = NULL;

							if( isYouFollowSay( m.saidText ) ) {
								otherToFollow = getClosestOtherPlayer( nextPlayer );
							}
							else {
								char *namedPlayer = isNamedFollowSay( m.saidText );

								if( namedPlayer != NULL ) {
									printf( "Named player = '%s\n", namedPlayer );
									otherToFollow =
											getPlayerByName( namedPlayer, nextPlayer );

									if( otherToFollow == NULL &&
									( strcmp( namedPlayer, "MYSELF" ) == 0 ||
									strcmp( namedPlayer, "NO ONE" ) == 0 ||
									strcmp( namedPlayer, "NOBODY" ) == 0 ) ) {
										otherToFollow = nextPlayer;
									}
								}
							}

							if( otherToFollow != NULL ) {
								if( otherToFollow == nextPlayer ) {
									if( nextPlayer->followingID != -1 ) {
										nextPlayer->followingID = -1;
										nextPlayer->followingUpdate = true;
									}
								}
								else if( nextPlayer->followingID !=
								otherToFollow->id ) {
									nextPlayer->followingID = otherToFollow->id;
									nextPlayer->followingUpdate = true;

									if( otherToFollow->leadingColorIndex == -1 ) {
										otherToFollow->leadingColorIndex =
												getUnusedLeadershipColor();
									}

									// break any loops
									LiveObject *o = nextPlayer;

									while( o != NULL && o->followingID != -1 ) {
										if( o->followingID == nextPlayer->id ) {
											// loop
											// break it by having next player's
											// new leader follow no one
											otherToFollow->followingID = -1;
											otherToFollow->followingUpdate = true;
											break;
										}
										o = getLiveObject( o->followingID );
									}


									if( ! isExiled( otherToFollow,
													nextPlayer )
													&&
													nextPlayer->lineageEveID !=
													otherToFollow->lineageEveID ) {

										// tell the new leader about their
										// new follower who is from another family

										const char *name = "NAMELESS PERSON";
										if( nextPlayer->name != NULL ) {
											name = nextPlayer->name;
										}
										char *message = autoSprintf(
												"PS\n"
												"%d/0 OUTSIDER %s IS MY NEW FOLLOWER "
												"*visitor %d *map %d %d\n#",
												otherToFollow->id,
												name,
												nextPlayer->id,
												nextPlayer->xs -
												otherToFollow->birthPos.x,
												nextPlayer->ys -
												otherToFollow->birthPos.y );
										sendMessageToPlayer( otherToFollow,
															 message,
															 strlen( message ) );
										delete [] message;
									}
								}
							}
							else {
								if( isYouExileSay( m.saidText ) ) {
									otherToExile =
											getClosestOtherPlayer( nextPlayer );
								}
								else {
									char *namedPlayer =
											isNamedExileSay( m.saidText );

									if( namedPlayer != NULL ) {
										otherToExile =
												getPlayerByName( namedPlayer,
																 nextPlayer );
									}
								}

								if( otherToExile != NULL ) {
									if( otherToExile->
									exiledByIDs.getElementIndex(
											nextPlayer->id ) == -1 ) {
										otherToExile->exiledByIDs.push_back(
												nextPlayer->id );

										otherToExile->exileUpdate = true;

										if( isFollower( nextPlayer,
														otherToExile ) ) {
											// exiled by their leader
											// warn them about it

											char *leadershipName =
													getLeadershipName( nextPlayer );

											const char *allyWord = "ALLIES";
											int numAllies =
													countAllies(
															otherToExile,
															getPlayerPos( otherToExile ),
															possePopulationRadius );

											if( numAllies == 1 ) {
												allyWord = "ALLY";
											}
											char *warnMessage =
													autoSprintf(
															"YOU HAVE BEEN EXILED BY "
															"YOUR %s.**"
															"YOU NOW HAVE %d "
															"IN-RANGE %s LEFT.",
															leadershipName,
															numAllies,
															allyWord );

											delete [] leadershipName;

											sendGlobalMessage( warnMessage,
															   otherToExile );
											delete [] warnMessage;
										}
									}
								}
								else {
									if( isYouRedeemSay( m.saidText ) ) {
										otherToRedeem =
												getClosestOtherPlayer( nextPlayer );
									}
									else {
										char *namedPlayer =
												isNamedRedeemSay( m.saidText );

										if( namedPlayer != NULL ) {
											otherToRedeem =
													getPlayerByName( namedPlayer,
																	 nextPlayer );
										}
									}

									if( otherToRedeem != NULL ) {
										// pass redemption downward
										// clearing up exiles perpetrated by
										// our followers

										int exileChanged = false;

										for( int e=0;
										e<otherToRedeem->exiledByIDs.size();
										e++ ) {

											// for
											LiveObject *exiler =
													getLiveObject(
															otherToRedeem->
															exiledByIDs.
															getElementDirect( e ) );

											if( exiler == nextPlayer ) {

												otherToRedeem->
												exiledByIDs.deleteElement( e );
												e--;
												otherToRedeem->exileUpdate = true;
												exileChanged = true;
											}
											else if( exiler != NULL &&
											isFollower( nextPlayer,
														exiler ) ) {
												otherToRedeem->
												exiledByIDs.deleteElement( e );
												e--;
												otherToRedeem->exileUpdate = true;
												exileChanged = true;
											}
										}
										if( exileChanged &&
										isFollower( nextPlayer,
													otherToRedeem ) ) {
											// redeemed by their leader
											// tell them about it

											char *leadershipName =
													getLeadershipName( nextPlayer );

											const char *allyWord = "ALLIES";
											int numAllies =
													countAllies(
															otherToRedeem,
															getPlayerPos( otherToRedeem ),
															possePopulationRadius );

											if( numAllies == 1 ) {
												allyWord = "ALLY";
											}
											char *warnMessage =
													autoSprintf(
															"YOU HAVE BEEN REDEEMED BY "
															"YOUR %s.**"
															"YOU NOW HAVE %d "
															"IN-RANGE %s.",
															leadershipName,
															numAllies,
															allyWord );

											delete [] leadershipName;

											sendGlobalMessage( warnMessage,
															   otherToRedeem );
											delete [] warnMessage;
										}
									}
								}
							}

							if( strstr( m.saidText, orderPhrase ) == m.saidText ) {
								// starts with ORDER phrase

								char *order =
										trimWhitespace(
												&( m.saidText[ strlen( orderPhrase ) ] ) );

								char *leadershipName =
										getLeadershipName( nextPlayer );

								if( leadershipName != NULL ) {
									// they are a leader

									// let leader know that order was made live
									char *selfLeadershipName =
											getLeadershipName( nextPlayer, true );

									int followerCount =
											countFollowers( nextPlayer );

									const char *followerWord = "FOLLOWERS";

									if( followerCount == 1 ) {
										followerWord = "FOLLOWER";
									}

									char *confirmMessage =
											autoSprintf( "AS %s, "
														 "YOU ISSUED AN ORDER "
														 "TO YOUR %d %s:**%s",
														 selfLeadershipName,
														 followerCount,
														 followerWord,
														 order );

									delete [] selfLeadershipName;

									sendGlobalMessage( confirmMessage, nextPlayer );
									delete [] confirmMessage;

									char *formattedOrder =
											autoSprintf( "ORDER FROM YOUR %s:**%s",
														 leadershipName, order );

									delete [] leadershipName;

									// originated with this player
									replaceOrder( nextPlayer, formattedOrder,
												  nextOrderNumber,
												  nextPlayer->id );

									delete [] formattedOrder;
									nextOrderNumber++;

									// give them a pointer to their closest
									// follower
									LiveObject *closeF =
											getClosestFollower( nextPlayer );
									if( closeF != NULL ) {
										GridPos fPos = getPlayerPos( closeF );

										char *newSaidText =
												autoSprintf( "%s "
															 "*follower %d *map %d %d\n#",
															 m.saidText,
															 closeF->id,
															 fPos.x,
															 fPos.y );
										delete [] m.saidText;
										m.saidText = newSaidText;
									}
								}
								delete [] order;
							}


							if( nextPlayer->holdingID > 0 &&
							getObject( nextPlayer->holdingID )->deadlyDistance
							> 0 ) {
								// are they speaking intent to kill?

								LiveObject *otherToKill = NULL;

								if( isYouKillSay( m.saidText ) ) {
									otherToKill =
											getClosestOtherPlayer( nextPlayer );
								}
								else {
									char *namedPlayer =
											isNamedKillSay( m.saidText );

									if( namedPlayer != NULL ) {
										otherToKill =
												getPlayerByName( namedPlayer,
																 nextPlayer );
										delete [] namedPlayer;
									}

									if( otherToKill != NULL ) {
										if( distance(
												getPlayerPos( nextPlayer ),
												getPlayerPos( otherToKill ) )
												>= 20 ) {
											// same limit as for YOU
											otherToKill = NULL;
										}
									}
								}

								if( otherToKill != NULL ) {
									playerIndicesToSendUpdatesAbout.push_back( i );
									// spoken intent to kill has unlimited distance
									// based on limits above instead
									tryToStartKill(
											nextPlayer, otherToKill->id,
											&playerIndicesToSendUpdatesAbout, true );
								}
							}



							if( nextPlayer->holdingID < 0 ) {

								// we're holding a baby
								// (no longer matters if it's our own baby)
								// (we let adoptive parents name too)

								LiveObject *babyO =
										getLiveObject( - nextPlayer->holdingID );

								if( babyO != NULL && babyO->name == NULL ) {
									char *name = isBabyNamingSay( m.saidText );

									if( name != NULL && strcmp( name, "" ) != 0 ) {
										nameBaby( nextPlayer, babyO, name,
												  &playerIndicesToSendNamesAbout );
										replaceNameInSaidPhrase( name,
																 &( m.saidText ),
																 babyO );
									}
								}
							}
							else {
								// not holding anyone

								char *name = isBabyNamingSay( m.saidText );

								if( name != NULL && strcmp( name, "" ) != 0 ) {
									// still, check if we're naming a nearby,
									// nameless non-baby

									LiveObject *closestOther =
											getClosestOtherPlayer( nextPlayer,
																   babyAge, true );

									if( closestOther != NULL ) {

										if( closestOther->isEve ) {

											name = isEveNamingSay( m.saidText );

											if( name != NULL &&
											strcmp( name, "" ) != 0 ) {

												nameEve( closestOther, name );
												playerIndicesToSendNamesAbout.
												push_back(
														getLiveObjectIndex(
																closestOther->id ) );

												replaceNameInSaidPhrase(
														name,
														&( m.saidText ),
														closestOther, true );

												if( ! isEveWindow() &&
												! closestOther->isTutorial &&
												closestOther->
												curseStatus.curseLevel == 0 ) {
													// new family name created
													restockPostWindowFamilies();
												}
											}
										}
										else {
											// non-Eve
											nameBaby(
													nextPlayer, closestOther,
													name,
													&playerIndicesToSendNamesAbout );

											replaceNameInSaidPhrase(
													name,
													&( m.saidText ),
													closestOther, false );
										}
									}
								}

								// also check if we're holding something writable
								unsigned char metaData[ MAP_METADATA_LENGTH ];
								int len = strlen( m.saidText );

								if( nextPlayer->holdingID > 0 &&
								len < MAP_METADATA_LENGTH &&
								getObject(
										nextPlayer->holdingID )->writable &&
										// and no metadata already on it
										! getMetadata( nextPlayer->holdingID,
													   metaData ) ) {

									char *textToAdd = NULL;


									if( strstr(
											getObject( nextPlayer->holdingID )->
											description,
											"+map" ) != NULL ) {
										// holding a potential map
										// add coordinates to where we're standing
										GridPos p = getPlayerPos( nextPlayer );

										char *mapStuff = autoSprintf(
												" *map %d %d %.f",
												p.x, p.y, Time::timeSec() );

										int mapStuffLen = strlen( mapStuff );

										char *saidText =
												stringDuplicate( m.saidText );

										int saidLen = strlen( saidText );

										int extra = saidLen + mapStuffLen
												- ( MAP_METADATA_LENGTH - 1 );

										if( extra > 0 ) {
											// too long to fit in metadata,
											// trim speech, not map data

											saidLen = saidLen - extra;

											// truncate
											saidText[ saidLen ] = '\0';
										}

										textToAdd = autoSprintf(
												"%s%s", saidText, mapStuff );

										delete [] saidText;
										delete [] mapStuff;
									}
									else {
										textToAdd = stringDuplicate( m.saidText );
									}

									int lenToAdd = strlen( textToAdd );

									// leave room for null char at end
									if( lenToAdd > MAP_METADATA_LENGTH - 1 ) {
										lenToAdd = MAP_METADATA_LENGTH - 1;
									}

									memset( metaData, 0, MAP_METADATA_LENGTH );
									// this will leave 0 null character at end
									// left over from memset of full length
									memcpy( metaData, textToAdd, lenToAdd );

									delete [] textToAdd;

									nextPlayer->holdingID =
											addMetadata( nextPlayer->holdingID,
														 metaData );

									TransRecord *writingHappenTrans =
											getMetaTrans( 0, nextPlayer->holdingID );

									if( writingHappenTrans != NULL &&
									writingHappenTrans->newTarget > 0 &&
									getObject( writingHappenTrans->newTarget )
									->written ) {
										// bare hands transition going from
										// writable to written
										// use this to transform object in
										// hands as we write
										handleHoldingChange(
												nextPlayer,
												writingHappenTrans->newTarget );
										playerIndicesToSendUpdatesAbout.
										push_back( i );
									}
								}
							}

							// trim whitespace and make sure we're not
							// adding an empty string
							// empty or whitespace strings causes trouble
							// elsewhere in code
							char *cleanSay = trimWhitespace( m.saidText );

							if( strcmp( cleanSay, "" ) != 0 ) {
								makePlayerSay( nextPlayer, cleanSay );
							}
							delete [] cleanSay;
						}
						else if( m.type == KILL ) {
							playerIndicesToSendUpdatesAbout.push_back( i );
							tryToStartKill( nextPlayer, m.id,
											&playerIndicesToSendUpdatesAbout );
						}
						else if( m.type == USE ) {
							// send update even if action fails (to let them
							// know that action is over)
							playerIndicesToSendUpdatesAbout.push_back( i );

							// track whether this USE resulted in something
							// new on the ground in case of placing a grave
							int newGroundObject = -1;
							GridPos newGroundObjectOrigin =
									{ nextPlayer->heldGraveOriginX,
									  nextPlayer->heldGraveOriginY };

							// save current value here, because it may
							// change below
							int heldGravePlayerID = nextPlayer->heldGravePlayerID;


							char distanceUseAllowed = false;

							if( nextPlayer->holdingID > 0 ) {

								// holding something
								ObjectRecord *heldObj =
										getObject( nextPlayer->holdingID );

								if( heldObj->useDistance > 1 ) {
									// it's long-distance

									GridPos targetPos = { m.x, m.y };
									GridPos playerPos = { nextPlayer->xd,
														  nextPlayer->yd };

									double d = distance( targetPos,
														 playerPos );

									if( heldObj->useDistance >= d &&
									! directLineBlocked( nextPlayer,
														 playerPos,
														 targetPos ) ) {
										distanceUseAllowed = true;
									}
								}
							}



							int target = getMapObject( m.x, m.y );



							char isAdjacent = false;

							if( !distanceUseAllowed ) {
								isAdjacent = isGridAdjacent( m.x, m.y,
															 nextPlayer->xd,
															 nextPlayer->yd );


								if( ! isAdjacent &&
								m.x == nextPlayer->xd &&
								m.y == nextPlayer->yd ) {

									isAdjacent = true;
								}


								if( ! isAdjacent && target > 0 ) {
									ObjectRecord *destO = getObject( target );

									if( destO->wide ) {
										for( int r=0; r<destO->leftBlockingRadius;
										r++ ) {
											int testX = m.x - r - 1;
											if( isGridAdjacent( testX, m.y,
																nextPlayer->xd,
																nextPlayer->yd ) ) {
												// don't count wide object
												// as adjacent if it hangs
												// out over another blocking
												// object (prevent wide truck
												// from being stolen through
												// fence)
												int blockOID =
														getMapObject( testX, m.y );
												if( blockOID == 0 ||
												! getObject( blockOID )->
												blocksWalking ) {
													isAdjacent = true;
													break;
												}
											}
										}
										if( ! isAdjacent )
											for( int r=0; r<destO->rightBlockingRadius;
											r++ ) {
												int testX = m.x + r + 1;
												if( isGridAdjacent( testX, m.y,
																	nextPlayer->xd,
																	nextPlayer->yd ) ) {
													int blockOID =
															getMapObject( testX, m.y );
													if( blockOID == 0 ||
													! getObject( blockOID )->
													blocksWalking ) {
														isAdjacent = true;
														break;
													}
												}
											}
									}
								}
							}


							if( isBiomeAllowedForPlayer( nextPlayer, m.x, m.y ) )
								if( ! isPlayerBlockedFromHoldingByPosse( nextPlayer ) )
									if( distanceUseAllowed
									||
									isAdjacent ) {

										nextPlayer->actionAttempt = 1;
										nextPlayer->actionTarget.x = m.x;
										nextPlayer->actionTarget.y = m.y;

										if( m.x > nextPlayer->xd ) {
											nextPlayer->facingOverride = 1;
										}
										else if( m.x < nextPlayer->xd ) {
											nextPlayer->facingOverride = -1;
										}

										// can only use on targets next to us for now,
										// no diags



										ObjectRecord *targetObj = NULL;
										if( target != 0 ) {
											targetObj = getObject( target );
										}

										if( targetObj != NULL &&
										targetObj->normalOnly &&
										( nextPlayer->isTutorial ||
										nextPlayer->curseStatus.curseLevel > 0 ) ) {

											// non-normal player blocked
											targetObj = NULL;
											target = 0;
										}


										int oldHolding = nextPlayer->holdingID;

										char accessBlocked =
												isAccessBlocked( nextPlayer, m.x, m.y, target,
																 oldHolding );

										if( accessBlocked ) {
											// ignore action from wrong side
											// or that players don't own
										}
										else if( targetObj != NULL ) {

											// see if target object is permanent
											// and has writing on it.
											// if so, read by touching it

											if( targetObj->permanent &&
											targetObj->written ) {
												forcePlayerToRead( nextPlayer, target );
											}

											if( targetObj->permanent &&
											targetObj->expertFind ) {
												findExpertForPlayer( nextPlayer,
																	 targetObj );
											}


											// try using object on this target

											TransRecord *r = NULL;
											char defaultTrans = false;

											char heldCanBeUsed = false;
											char containmentTransfer = false;
											if( // if what we're holding contains
											// stuff, block it from being
											// used as a tool
											nextPlayer->numContained == 0 ) {
												heldCanBeUsed = true;
											}
											else if( nextPlayer->holdingID > 0 ) {
												// see if result of trans
												// would preserve containment

												r = getPTrans( nextPlayer->holdingID,
															   target );


												ObjectRecord *heldObj = getObject(
														nextPlayer->holdingID );

												if( r != NULL && r->newActor == 0 &&
												r->newTarget > 0 ) {
													ObjectRecord *newTargetObj =
															getObject( r->newTarget );

													if( targetObj->numSlots == 0
													&& newTargetObj->numSlots >=
													heldObj->numSlots
													&&
													newTargetObj->slotSize >=
													heldObj->slotSize ) {

														containmentTransfer = true;
														heldCanBeUsed = true;
													}
												}

												if( r == NULL ) {
													// no transition applies for this
													// held, whether full or empty

													// let it be used anyway, so
													// that generic transition (below)
													// might apply
													heldCanBeUsed = true;
												}

												r = NULL;
											}


											if( nextPlayer->holdingID >= 0 &&
											heldCanBeUsed ) {
												// negative holding is ID of baby
												// which can't be used
												// (and no bare hand action available)
												r = getPTrans( nextPlayer->holdingID,
															   target );


												if( r == NULL ) {
													// no transition applies
													// check if held or target has
													// 1-second decay trans defined
													// If so, treat it as instant
													// and let it go through now
													// (skip if result of decay is 0)
													TransRecord *heldDecay =
															getPTrans(
																	-1,
																	nextPlayer->holdingID );
													if( heldDecay != NULL &&
													heldDecay->autoDecaySeconds == 1 &&
													heldDecay->newTarget > 0 ) {
														// force decay NOW and try again
														handleHeldDecay(
																nextPlayer,
																i,
																&playerIndicesToSendUpdatesAbout,
																&playerIndicesToSendHealingAbout );
														r = getPTrans(
																nextPlayer->holdingID,
																target );
													}

												}
												if( r == NULL ) {

													int newTarget =
															checkTargetInstantDecay(
																	target, m.x, m.y );

													// if so, let transition go through
													// (skip if result of decay is 0)
													if( newTarget != 0 &&
													newTarget != target ) {

														target = newTarget;
														targetObj = getObject( target );

														r = getPTrans(
																nextPlayer->holdingID,
																target );
													}
												}
											}

											char blockedTool = false;

											if( nextPlayer->holdingID > 0 &&
											r != NULL &&
											r->newActor != 0 ) {
												// make sure player can use this tool
												// only counts as a real use if something
												// is left in the hand
												// otherwise, it could be a stacking action
												// (like putting a wool pad in a bowl)

												// also, watch out for action where
												// we're inserting an object into
												// a container that it can also be used
												// on
												char insertion = false;
												ObjectRecord *heldO =
														getObject( nextPlayer->holdingID );

												if( targetObj->numSlots > 0 &&
												heldO->containable &&
												targetObj->slotSize >=
												heldO->containSize &&
												containmentPermitted(
														target,
														nextPlayer->holdingID ) &&
														getNumContained( m.x, m.y ) > 0 ) {

													insertion = true;
												}

												// watch out for transformation
												// from one tool to another
												// (sterilizing knife shouldn't learn
												//  knife, for example)
												char transformation = false;
												if( r->newActor != nextPlayer->holdingID ) {

													int newActorToolSet =
															getToolSet( r->newActor );
													int heldToolSet =
															getToolSet( nextPlayer->holdingID );

													if( newActorToolSet != -1 &&
													newActorToolSet != heldToolSet ) {
														transformation = true;
													}
												}

												// watch out for case where target is
												// NOT transformed
												// In that case, the tool isn't actually
												// being used (for example, recycling a
												// sword in a roller)
												char nonTransformTarget = false;

												if( target == r->newTarget ) {
													nonTransformTarget = true;
												}

												// EXCEPT in case where new actor
												// stuck in hand
												// (for fishing case, which doesn't
												//  transform water hole)
												if( nonTransformTarget &&
												r->newActor > 0 &&
												getObject( r->newActor )->permanent ) {

													nonTransformTarget = false;
												}


												// also watch out for failed
												// tool use due to hungry work
												char hungBlocked = false;
												if( ! insertion &&
												r->newTarget > 0 ) {

													int hCost = 0;
													hungBlocked = isHungryWorkBlocked(
															nextPlayer,
															r->newTarget,
															&hCost );
												}


												if( ! insertion &&
												! hungBlocked &&
												! transformation &&
												! nonTransformTarget &&
												! canPlayerUseOrLearnTool(
														nextPlayer,
														nextPlayer->holdingID ) ) {
													r = NULL;
													blockedTool = true;
												}
											}

											if( ! blockedTool &&
											target > 0 &&
											r != NULL ) {

												char couldBeTool = false;

												if( getObject( target )->permanent ) {
													couldBeTool = true;
												}
												else {
													// non-perm

													// some tools sit loose on ground
													// and then we do something to them
													// to make them permanent
													// (like pounding stakes)
													// Check if this is the case

													if( r->newTarget > 0 &&
													getObject( r->newTarget )->
													permanent ) {

														// but ONLY if there's
														// no bare-hand action possible
														// on the result

														// if that exists, we took
														// apart the loose tool into
														// a stack of parts

														TransRecord *bareTrans =
																getPTrans( 0, r->newTarget );

														if( bareTrans == NULL ) {
															couldBeTool = true;
														}
													}
												}

												// make sure player can use this ground-tool
												if( couldBeTool &&
												! canPlayerUseOrLearnTool(
														nextPlayer,
														target ) ) {
													r = NULL;
													blockedTool = true;
													sendToolExpertMessage( nextPlayer,
																		   target );
												}
												else if( couldBeTool &&
												! canPlayerUseTool( nextPlayer,
																	target ) ) {
													// maybe this is their first trial
													// use of the ground tool
													// show them who else can use it
													sendToolExpertMessage( nextPlayer,
																		   target );
												}
											}
											else if( ! blockedTool &&
											target > 0 &&
											r == NULL ) {
												// no trans applies
												if( ! canPlayerUseTool( nextPlayer,
																		target ) ) {
													// tell them who can use it
													sendToolExpertMessage( nextPlayer,
																		   target );
												}
											}


											if( r != NULL &&
											r->newActor > 0 &&
											getObject( r->newActor )->floor ) {
												// special case:
												// ending up with floor in hand means
												// we stick floor UNDER target
												// object on ground

												// but only if there's no floor there
												// already
												if( getMapFloor( m.x, m.y ) == 0 ) {
													setMapFloor( m.x, m.y, r->newActor );
													nextPlayer->holdingID = 0;
													nextPlayer->holdingEtaDecay = 0;
												}

												// always cancel transition in either case
												r = NULL;
											}


											if( r != NULL &&
											targetObj->numSlots > 0 ) {
												// target has number of slots

												int numContained =
														getNumContained( m.x, m.y );

												int numSlotsInNew = 0;

												if( r->newTarget > 0 ) {
													numSlotsInNew =
															getObject( r->newTarget )->numSlots;
												}

												if( numContained > numSlotsInNew &&
												numSlotsInNew == 0 ) {
													// not enough room in new target

													// check if new actor will contain
													// them (reverse containment transfer)

													if( r->newActor > 0 &&
													nextPlayer->numContained == 0 ) {
														// old actor empty

														int numSlotsNewActor =
																getObject( r->newActor )->
																numSlots;

														numSlotsInNew = numSlotsNewActor;
													}
												}


												if( numContained > numSlotsInNew ) {
													// would result in shrinking
													// and flinging some contained
													// objects
													// block it.
													heldCanBeUsed = false;
													r = NULL;
												}
											}


											if( r == NULL &&
											( nextPlayer->holdingID != 0 ||
											targetObj->permanent ) &&
											( isGridAdjacent( m.x, m.y,
															  nextPlayer->xd,
															  nextPlayer->yd )
															  ||
															  ( m.x == nextPlayer->xd &&
															  m.y == nextPlayer->yd ) ) ) {

												// block default transitions from
												// happening at a distance

												// search for default
												r = getPTrans( -2, target );

												if( r != NULL ) {
													defaultTrans = true;
												}
												else if( nextPlayer->holdingID <= 0 ||
												targetObj->numSlots == 0 ) {
													// also consider bare-hand
													// action that produces
													// no new held item

													// but only on non-container
													// objects (example:  we don't
													// want to kick minecart into
													// motion every time we try
													// to add something to it)

													// treat this the same as
													// default
													r = getPTrans( 0, target );

													if( r != NULL &&
													r->newActor == 0 ) {

														defaultTrans = true;
													}
													else {
														r = NULL;
													}
												}
											}


											if( r != NULL &&
											r->newTarget > 0 &&
											r->newTarget != target ) {

												ObjectRecord *newTargetObj =
														getObject( r->newTarget );

												// target would change here
												if( getMapFloor( m.x, m.y ) != 0 ) {
													// floor present

													// make sure new target allowed
													// to exist on floor
													if( strstr( newTargetObj->
													description,
													"groundOnly" ) != NULL ) {
														r = NULL;
													}
												}
												//printf("\n(0)=====> main context ...");
												if( r != NULL &&
												newTargetObj->isBiomeLimited &&
												! canBuildInBiome(
														newTargetObj,
														getMapBiome( m.x,
																	 m.y ) ) ) {
													// can't make this object
													// in this biome
													r = NULL;
												}
											}


											if( r == NULL &&
											nextPlayer->holdingID > 0 &&
											! blockedTool ) {

												logTransitionFailure(
														nextPlayer->holdingID,
														target );
											}

											double playerAge = computeAge( nextPlayer );

											int hungryWorkCost = 0;

											if( r != NULL &&
											r->newTarget > 0 ) {

												if( isHungryWorkBlocked(
														nextPlayer,
														r->newTarget,
														&hungryWorkCost ) ) {
													r = NULL;

													sendHungryWorkSpeech( nextPlayer );
												}
											}


											if( r != NULL && containmentTransfer ) {
												// special case contained items
												// moving from actor into new target
												// (and hand left empty)
												setResponsiblePlayer( - nextPlayer->id );

												setMapObject( m.x, m.y, r->newTarget );
												newGroundObject = r->newTarget;

												setResponsiblePlayer( -1 );

												transferHeldContainedToMap( nextPlayer,
																			m.x, m.y );
												handleHoldingChange( nextPlayer,
																	 r->newActor );

												setHeldGraveOrigin( nextPlayer, m.x, m.y,
																	r->newTarget );
											}
											else if( r != NULL &&
											// are we old enough to handle
											// what we'd get out of this transition?
											( ( r->newActor == 0 &&
											playerAge >= defaultActionAge )
											||
											( r->newActor > 0 &&
											canPickup( r->newActor, playerAge ) ) )
											&&
											// does this create a blocking object?
											// only consider vertical-blocking
											// objects (like vertical walls and doors)
											// because these look especially weird
											// on top of a player
											// We can detect these because they
											// also hug the floor
											// Horizontal doors look fine when
											// closed on player because of their
											// vertical offset.
											//
											// if so, make sure there's not someone
											// standing still there
											( r->newTarget == 0 ||
											!
											( getObject( r->newTarget )->
											blocksWalking
											&&
											getObject( r->newTarget )->
											floorHugging )
											||
											isMapSpotEmptyOfPlayers( m.x,
																	 m.y ) ) ) {

												if( ! defaultTrans ) {
													handleHoldingChange( nextPlayer,
																		 r->newActor );

													setHeldGraveOrigin( nextPlayer,
																		m.x, m.y,
																		r->newTarget );

													if( r->target > 0 ) {
														nextPlayer->heldTransitionSourceID =
																r->target;
													}
													else {
														nextPlayer->heldTransitionSourceID =
																-1;
													}
												}



												// has target shrunken as a container?
												int oldSlots =
														getNumContainerSlots( target );
												int newSlots =
														getNumContainerSlots( r->newTarget );

												if( oldSlots > 0 &&
												newSlots == 0 &&
												r->actor == 0 &&
												r->newActor > 0
												&&
												getNumContainerSlots( r->newActor ) ==
												oldSlots &&
												getObject( r->newActor )->slotSize >=
												targetObj->slotSize ) {

													// bare-hand action that results
													// in something new in hand
													// with same number of slots
													// as target
													// keep what was contained

													FullMapContained f =
															getFullMapContained( m.x, m.y );

													setContained( nextPlayer, f );

													clearAllContained( m.x, m.y );

													restretchDecays(
															nextPlayer->numContained,
															nextPlayer->containedEtaDecays,
															target, r->newActor );
												}
												else {
													// target on ground changed
													// and we don't have the same
													// number of slots in a new held obj

													shrinkContainer( m.x, m.y, newSlots );

													if( newSlots > 0 ) {
														restretchMapContainedDecays(
																m.x, m.y,
																target,
																r->newTarget );
													}
												}


												timeSec_t oldEtaDecay =
														getEtaDecay( m.x, m.y );

												setResponsiblePlayer( - nextPlayer->id );

												if( r->newTarget > 0
												&& getObject( r->newTarget )->floor ) {

													// it turns into a floor
													setMapObject( m.x, m.y, 0 );

													setMapFloor( m.x, m.y, r->newTarget );

													if( r->newTarget == target ) {
														// unchanged
														// keep old decay in place
														setFloorEtaDecay( m.x, m.y,
																		  oldEtaDecay );
													}
												}
												else {
													setMapObject( m.x, m.y, r->newTarget );
													newGroundObject = r->newTarget;
												}

												if( hungryWorkCost > 0 ) {
													if( nextPlayer->yummyBonusStore > 0 ) {
														if( nextPlayer->yummyBonusStore
														>= hungryWorkCost ) {
															nextPlayer->yummyBonusStore -=
																	hungryWorkCost;
															hungryWorkCost = 0;
														}
														else {
															hungryWorkCost -=
																	nextPlayer->yummyBonusStore;
															nextPlayer->yummyBonusStore = 0;
														}
													}

													nextPlayer->foodStore -= hungryWorkCost;

													// we checked above, so player
													// never is taken down below 5 here
													nextPlayer->foodUpdate = true;
												}


												setResponsiblePlayer( -1 );

												if( target == r->newTarget ) {
													// target not changed
													// keep old decay in place
													setEtaDecay( m.x, m.y, oldEtaDecay );
												}

												if( target > 0 && r->newTarget > 0 &&
												target != r->newTarget &&
												! getObject( target )->isOwned &&
												getObject( r->newTarget )->isOwned ) {

													// player just created an owned
													// object here
													GridPos newPos = { m.x, m.y };

													nextPlayer->
													ownedPositions.push_back( newPos );
													newOwnerPos.push_back( newPos );
												}


												if( r->actor == 0 &&
												target > 0 && r->newTarget > 0 &&
												target != r->newTarget ) {

													TransRecord *oldDecayTrans =
															getTrans( -1, target );

													TransRecord *newDecayTrans =
															getTrans( -1, r->newTarget );

													if( oldDecayTrans != NULL &&
													newDecayTrans != NULL  &&
													oldDecayTrans->epochAutoDecay ==
													newDecayTrans->epochAutoDecay &&
													oldDecayTrans->autoDecaySeconds ==
													newDecayTrans->autoDecaySeconds &&
													oldDecayTrans->autoDecaySeconds
													> 0 ) {

														// old target and new
														// target decay into something
														// in same amount of time
														// and this was a bare-hand
														// action

														// doesn't matter if they
														// decay into SAME thing.

														// keep old decay time in place
														// (instead of resetting timer)
														setEtaDecay( m.x, m.y,
																	 oldEtaDecay );
													}
												}




												if( r->newTarget != 0 ) {

													handleMapChangeToPaths(
															m.x, m.y,
															getObject( r->newTarget ),
															&playerIndicesToSendUpdatesAbout );
												}
											}
											else if( nextPlayer->holdingID == 0 &&
											! targetObj->permanent &&
											canPickup( targetObj->id,
													   computeAge(
															nextPlayer ) ) ) {
												// no bare-hand transition applies to
												// this non-permanent target object

												// treat it like pick up

												pickupToHold( nextPlayer, m.x, m.y,
															  target );
											}
											else if( nextPlayer->holdingID >= 0 ) {

												char handled = false;

												if( m.i != -1 && targetObj->permanent &&
												targetObj->numSlots > m.i &&
												getNumContained( m.x, m.y ) > m.i &&
												strstr( targetObj->description,
														"+useOnContained" ) != NULL ) {
													// a valid slot specified to use
													// held object on.
													// AND container allows this

													int contTarget =
															getContained( m.x, m.y, m.i );

													char isSubCont = false;
													if( contTarget < 0 ) {
														contTarget = -contTarget;
														isSubCont = true;
													}

													ObjectRecord *contTargetObj =
															getObject( contTarget );

													TransRecord *contTrans =
															getPTrans( nextPlayer->holdingID,
																	   contTarget );

													ObjectRecord *newTarget = NULL;

													if( ! isSubCont &&
													contTrans != NULL &&
													( contTrans->newActor ==
													nextPlayer->holdingID ||
													contTrans->newActor == 0 ||
													canPickup(
															contTrans->newActor,
															computeAge(
																	nextPlayer ) ) ) ) {

														// a trans applies, and we
														// can hold the resulting actor
														if( contTrans->newTarget > 0 ) {
															newTarget = getObject(
																	contTrans->newTarget );
														}
													}
													if( newTarget != NULL &&
													isContainable(
															contTrans->newTarget ) &&
															newTarget->containSize <=
															targetObj->slotSize &&
															containmentPermitted(
																	targetObj->id,
																	newTarget->id ) &&
																	( nextPlayer->holdingID == 0
																	||
																	canPlayerUseOrLearnTool(
																			nextPlayer,
																			nextPlayer->holdingID ) ) ) {

														int oldHeld =
																nextPlayer->holdingID;

														handleHoldingChange(
																nextPlayer,
																contTrans->newActor );

														nextPlayer->heldOriginValid = 0;
														nextPlayer->heldOriginX = 0;
														nextPlayer->heldOriginY = 0;
														nextPlayer->
														heldTransitionSourceID = 0;

														if( contTrans->newActor > 0 &&
														contTrans->newActor !=
														oldHeld ) {

															nextPlayer->
															heldTransitionSourceID
															= contTargetObj->id;
														}


														setResponsiblePlayer(
																- nextPlayer->id );

														changeContained(
																m.x, m.y,
																m.i,
																contTrans->newTarget );

														setResponsiblePlayer( -1 );
														handled = true;
													}
												}


												// consider other cases
												if( ! handled ) {
													if( nextPlayer->holdingID == 0 &&
													targetObj->permanent ) {

														// try removing from permanent
														// container
														removeFromContainerToHold(
																nextPlayer,
																m.x, m.y,
																m.i );
													}
													else if( nextPlayer->holdingID > 0 ) {
														// try adding what we're holding to
														// target container

														addHeldToContainer(
																nextPlayer, target, m.x, m.y );
													}
												}
											}


											if( targetObj->permanent &&
											targetObj->foodValue > 0 ) {

												// just touching this object
												// causes us to eat from it

												nextPlayer->justAte = true;
												nextPlayer->justAteID =
														targetObj->id;

												nextPlayer->lastAteID =
														targetObj->id;
												nextPlayer->lastAteFillMax =
														nextPlayer->foodStore;

												nextPlayer->foodStore +=
														getScaledFoodValue(
																nextPlayer,
																targetObj->foodValue );

												updateYum( nextPlayer, targetObj->id );


												int bonus = getEatBonus( nextPlayer );

												if( targetObj->alcohol > 0 ) {
													bonus = 0;
												}

												logEating( targetObj->id,
														   targetObj->foodValue + bonus,
														   computeAge( nextPlayer ),
														   m.x, m.y );

												nextPlayer->foodStore += bonus;

												checkForFoodEatingEmot( nextPlayer,
																		targetObj->id );

												int cap =
														nextPlayer->lastReportedFoodCapacity;

												if( nextPlayer->foodStore > cap ) {

													int over = nextPlayer->foodStore - cap;

													nextPlayer->foodStore = cap;

													int overflowCap =
															computeOverflowFoodCapacity( cap );

													if( over > overflowCap ) {
														over = overflowCap;
													}
													nextPlayer->yummyBonusStore += over;
												}


												// we eat everything BUT what
												// we picked from it, if anything
												if( oldHolding == 0 &&
												nextPlayer->holdingID != 0 ) {

													ObjectRecord *newHeld =
															getObject( nextPlayer->holdingID );

													if( newHeld->foodValue > 0 ) {
														nextPlayer->foodStore -=
																newHeld->foodValue;

														if( nextPlayer->lastAteFillMax >
														nextPlayer->foodStore ) {

															nextPlayer->foodStore =
																	nextPlayer->lastAteFillMax;
														}
													}

												}


												if( targetObj->alcohol != 0 ) {
													drinkAlcohol( nextPlayer,
																  targetObj->alcohol );
												}


												nextPlayer->foodDecrementETASeconds =
														Time::getCurrentTime() +
														computeFoodDecrementTimeSeconds(
																nextPlayer );

												nextPlayer->foodUpdate = true;
											}
										}
										else if( nextPlayer->holdingID > 0 ) {
											// target location emtpy
											// target not where we're standing
											// we're holding something

											char usedOnFloor = false;
											int floorID = getMapFloor( m.x, m.y );

											if( floorID > 0 ) {

												TransRecord *r =
														getPTrans( nextPlayer->holdingID,
																   floorID );

												char blockedTool = false;

												if( nextPlayer->holdingID > 0 &&
												r != NULL ) {
													// make sure player can use this tool

													if( ! canPlayerUseOrLearnTool(
															nextPlayer,
															nextPlayer->holdingID ) ) {
														r = NULL;
														blockedTool = true;
													}
												}

												// floor might be a tool too
												if( r != NULL ) {
													if( ! canPlayerUseOrLearnTool(
															nextPlayer, floorID ) ) {
														r = NULL;
														blockedTool = true;
														sendToolExpertMessage( nextPlayer,
																			   floorID );
													}
													else if(
															! canPlayerUseTool( nextPlayer,
																				floorID ) ) {
														// maybe this is their first trial
														// use of the floor tool
														// show them who else can use it
														sendToolExpertMessage( nextPlayer,
																			   floorID );
													}
												}
												else {
													// no trans applies
													if( ! canPlayerUseTool( nextPlayer,
																			floorID ) ) {
														// tell them who can use it
														sendToolExpertMessage( nextPlayer,
																			   floorID );
													}
												}


												if( r == NULL && ! blockedTool ) {
													logTransitionFailure(
															nextPlayer->holdingID,
															floorID );
												}


												if( r != NULL &&
												// make sure we're not too young
												// to hold result of on-floor
												// transition
												( r->newActor == 0 ||
												canPickup(
														r->newActor,
														computeAge( nextPlayer ) ) ) ) {

													// applies to floor
													int resultID = r->newTarget;

													if( getObject( resultID )->floor ) {
														// changing floor to floor
														// go ahead
														usedOnFloor = true;

														if( resultID != floorID ) {
															setMapFloor( m.x, m.y,
																		 resultID );
														}
														handleHoldingChange( nextPlayer,
																			 r->newActor );

														setHeldGraveOrigin( nextPlayer,
																			m.x, m.y,
																			resultID );
													}
													else {
														// changing floor to non-floor
														char canPlace = true;
														if( getObject( resultID )->
														blocksWalking &&
														! isMapSpotEmpty( m.x, m.y ) ) {
															canPlace = false;
														}

														if( canPlace &&
														! isBiomeAllowedForPlayer(
																nextPlayer,
																m.x, m.y, true ) ) {
															// this floor is over a bad
															// biome for this player
															// don't let them remove it
															canPlace = false;
														}

														if( canPlace ) {
															setMapFloor( m.x, m.y, 0 );

															setMapObject( m.x, m.y,
																		  resultID );

															handleHoldingChange(
																	nextPlayer,
																	r->newActor );
															setHeldGraveOrigin( nextPlayer,
																				m.x, m.y,
																				resultID );

															usedOnFloor = true;
														}
													}
												}
											}



											// consider a use-on-bare-ground transtion

											ObjectRecord *obj =
													getObject( nextPlayer->holdingID );

											if( ! usedOnFloor && obj->foodValue == 0 &&
											// player didn't try to click something
											m.id == -1 ) {

												// get no-target transtion
												// (not a food transition, since food
												//   value is 0)
												TransRecord *r =
														getPTrans( nextPlayer->holdingID,
																   -1 );


												char canPlace = false;

												if( r != NULL &&
												r->newTarget != 0
												&&
												// make sure we're not too young
												// to hold result of bare ground
												// transition
												( r->newActor == 0 ||
												canPickup(
														r->newActor,
														computeAge( nextPlayer ) ) ) ) {

													canPlace = true;

													ObjectRecord *newTargetObj =
															getObject( r->newTarget );

													//printf("\n(0)=====> main2 context ...");
													if( newTargetObj->blocksWalking &&
													! isMapSpotEmpty( m.x, m.y ) ) {

														// can't do on-bare ground
														// transition where person
														// standing
														// if it creates a blocking
														// object
														canPlace = false;
													}
													else if(
															strstr( newTargetObj->description,
																	"groundOnly" ) != NULL
																	&&
																	getMapFloor( m.x, m.y ) != 0 ) {
														// floor present

														// new target not allowed
														// to exist on floor
														canPlace = false;
													}
													else if( newTargetObj->isBiomeLimited &&
													! canBuildInBiome(
															newTargetObj,
															getMapBiome( m.x,
																		 m.y ) ) ) {
														// can't make this object
														// in this biome
														canPlace = false;
													}
													else if( newTargetObj->blocksWalking &&
													( ( newTargetObj->sideAccess
													&&
													! isBiomeAllowedForPlayer(
															nextPlayer,
															m.x - 1,
															m.y ) &&
															! isBiomeAllowedForPlayer(
																	nextPlayer,
																	m.x + 1,
																	m.y ) )
																	||
																	( newTargetObj->noBackAccess
																	&&
																	! isBiomeAllowedForPlayer(
																			nextPlayer,
																			m.x,
																			m.y - 1 ) )
																			||
																			( ! isBiomeAllowedForPlayer(
																					nextPlayer,
																					m.x,
																					m.y + 1 ) &&
																					! isBiomeAllowedForPlayer(
																							nextPlayer,
																							m.x,
																							m.y - 1 ) ) ) ) {
														// we're setting down something that
														// will block us walking to that
														// tile again
														// So we'll need to access it from
														// an adjacent tile, and those
														// adjacent tiles are in bad biomes
														canPlace = false;
													}
												}

												if( canPlace ) {
													nextPlayer->heldTransitionSourceID =
															nextPlayer->holdingID;

													if( nextPlayer->numContained > 0 &&
													r->newActor == 0 &&
													r->newTarget > 0 &&
													getObject( r->newTarget )->numSlots
													>= nextPlayer->numContained &&
													getObject( r->newTarget )->slotSize
													>= obj->slotSize ) {

														// use on bare ground with full
														// container that leaves
														// hand empty

														// and there's room in newTarget

														setResponsiblePlayer(
																- nextPlayer->id );

														setMapObject( m.x, m.y,
																	  r->newTarget );
														newGroundObject = r->newTarget;

														setResponsiblePlayer( -1 );

														transferHeldContainedToMap(
																nextPlayer, m.x, m.y );

														handleHoldingChange( nextPlayer,
																			 r->newActor );

														setHeldGraveOrigin( nextPlayer,
																			m.x, m.y,
																			r->newTarget );
													}
													else {
														handleHoldingChange( nextPlayer,
																			 r->newActor );

														setHeldGraveOrigin( nextPlayer,
																			m.x, m.y,
																			r->newTarget );

														setResponsiblePlayer(
																- nextPlayer->id );

														if( r->newTarget > 0
														&& getObject( r->newTarget )->
														floor ) {

															setMapFloor( m.x, m.y,
																		 r->newTarget );
														}
														else {
															setMapObject( m.x, m.y,
																		  r->newTarget );
															newGroundObject = r->newTarget;
														}

														setResponsiblePlayer( -1 );

														handleMapChangeToPaths(
																m.x, m.y,
																getObject( r->newTarget ),
																&playerIndicesToSendUpdatesAbout );
													}
												}
											}
										}


										if( target == 0 && newGroundObject > 0 ) {
											// target location was empty, and now it's not
											// check if we moved a grave here

											ObjectRecord *o = getObject( newGroundObject );

											if( strstr( o->description, "origGrave" )
											!= NULL ) {

												setGravePlayerID(
														m.x, m.y, heldGravePlayerID );

												int swapDest =
														isGraveSwapDest( m.x, m.y,
																		 nextPlayer->id );

												GraveMoveInfo g = {
														{ newGroundObjectOrigin.x,
														  newGroundObjectOrigin.y },
														  { m.x,
															m.y },
															swapDest };
												newGraveMoves.push_back( g );
											}
										}
									}
						}
						else if( m.type == BABY ) {
							playerIndicesToSendUpdatesAbout.push_back( i );

							if( computeAge( nextPlayer ) >= minPickupBabyAge
							&&
							( isGridAdjacent( m.x, m.y,
											  nextPlayer->xd,
											  nextPlayer->yd )
											  ||
											  ( m.x == nextPlayer->xd &&
											  m.y == nextPlayer->yd ) ) ) {

								nextPlayer->actionAttempt = 1;
								nextPlayer->actionTarget.x = m.x;
								nextPlayer->actionTarget.y = m.y;

								if( m.x > nextPlayer->xd ) {
									nextPlayer->facingOverride = 1;
								}
								else if( m.x < nextPlayer->xd ) {
									nextPlayer->facingOverride = -1;
								}


								if( nextPlayer->holdingID == 0 ) {
									// target location empty and
									// and our hands are empty

									// check if there's a baby to pick up there

									// is anyone there?
									LiveObject *hitPlayer =
											getHitPlayer( m.x, m.y, m.id,
														  false, babyAge );

									double hitPlayerAge = 0;

									if( hitPlayer != NULL ) {
										hitPlayerAge = computeAge( hitPlayer );
									}

									if( hitPlayer != NULL &&
									!hitPlayer->heldByOther &&
									hitPlayerAge < babyAge  ) {

										// negative holding IDs to indicate
										// holding another player
										nextPlayer->holdingID = -hitPlayer->id;
										holdingSomethingNew( nextPlayer );

										nextPlayer->holdingEtaDecay = 0;

										hitPlayer->heldByOther = true;
										hitPlayer->heldByOtherID = nextPlayer->id;

										if( hitPlayer->heldByOtherID ==
										hitPlayer->parentID ) {
											hitPlayer->everHeldByParent = true;
										}


										// force baby to drop what they are
										// holding

										if( hitPlayer->holdingID != 0 ) {
											// never drop held wounds
											// they are the only thing a baby can
											// while held
											if( ! hitPlayer->holdingWound &&
											! hitPlayer->holdingBiomeSickness &&
											hitPlayer->holdingID > 0 ) {
												handleDrop(
														m.x, m.y, hitPlayer,
														&playerIndicesToSendUpdatesAbout );
											}
										}

										if( hitPlayer->xd != hitPlayer->xs
										||
										hitPlayer->yd != hitPlayer->ys ) {

											// force baby to stop moving
											hitPlayer->xd = m.x;
											hitPlayer->yd = m.y;
											hitPlayer->xs = m.x;
											hitPlayer->ys = m.y;

											// but don't send an update
											// about this
											// (everyone will get the pick-up
											//  update for the holding adult)
										}

										// if adult fertile female, baby auto-fed
										if( isFertileAge( nextPlayer ) ) {

											if( hitPlayer->foodStore <=
											babyFeedingLevel ||
											hitPlayerAge >= defaultActionAge ) {

												// babies that aren't starving
												// refuse to nurse
												// but still have pick-up cost
												// below

												hitPlayer->foodStore =
														computeFoodCapacity(
																hitPlayer );

												hitPlayer->foodUpdate = true;
												hitPlayer->responsiblePlayerID =
														nextPlayer->id;

												// reset their food decrement time
												hitPlayer->foodDecrementETASeconds =
														Time::getCurrentTime() +
														computeFoodDecrementTimeSeconds(
																hitPlayer );

												checkForFoodEatingEmot( hitPlayer,
																		0 );
											}
											else {
												setRefuseFoodEmote( hitPlayer );
											}

											int thisNurseCost = nurseCost;

											if( nextPlayer->yummyBonusStore > 0 ) {
												nextPlayer->yummyBonusStore -=
														thisNurseCost;
												thisNurseCost = 0;
												if( nextPlayer->yummyBonusStore <
												0 ) {

													// not enough to cover full
													// nurse cost

													// pass remaining nurse
													// cost onto main food store
													thisNurseCost = - nextPlayer->
															yummyBonusStore;
													nextPlayer->yummyBonusStore = 0;
												}
											}


											nextPlayer->foodStore -= thisNurseCost;

											if( nextPlayer->foodStore < 0 ) {
												// catch mother death later
												// at her next food decrement
												nextPlayer->foodStore = 0;
											}
											// leave their food decrement
											// time alone
											nextPlayer->foodUpdate = true;
										}

										nextPlayer->heldOriginValid = 1;
										nextPlayer->heldOriginX = m.x;
										nextPlayer->heldOriginY = m.y;
										nextPlayer->heldTransitionSourceID = -1;
									}

								}
							}
						}
						else if( m.type == SELF || m.type == UBABY ) {
							playerIndicesToSendUpdatesAbout.push_back( i );

							char holdingFood = false;
							char holdingDrugs = false;

							if( nextPlayer->holdingID > 0 ) {
								ObjectRecord *obj =
										getObject( nextPlayer->holdingID );

								if( obj->foodValue > 0 ) {
									holdingFood = true;

									if( strstr( obj->description, "remapStart" )
									!= NULL ) {
										// don't count drugs as food to
										// feed other people
										holdingFood = false;
										holdingDrugs = true;
									}
								}
							}

							LiveObject *targetPlayer = NULL;

							if( nextPlayer->holdingID < 0 ) {
								// holding a baby
								// don't allow this action through
								// keep targetPlayer NULL
							}
							else if( m.type == SELF ) {
								if( m.x == nextPlayer->xd &&
								m.y == nextPlayer->yd ) {

									// use on self
									targetPlayer = nextPlayer;
								}
							}
							else if( m.type == UBABY ) {

								if( isGridAdjacent( m.x, m.y,
													nextPlayer->xd,
													nextPlayer->yd ) ||
													( m.x == nextPlayer->xd &&
													m.y == nextPlayer->yd ) ) {


									if( m.x > nextPlayer->xd ) {
										nextPlayer->facingOverride = 1;
									}
									else if( m.x < nextPlayer->xd ) {
										nextPlayer->facingOverride = -1;
									}

									// try click on baby
									int hitIndex;
									LiveObject *hitPlayer =
											getHitPlayer( m.x, m.y, m.id,
														  false,
														  babyAge, -1, &hitIndex );

									if( hitPlayer != NULL && holdingDrugs ) {
										// can't even feed baby drugs
										// too confusing
										hitPlayer = NULL;
									}

									if( hitPlayer == NULL ||
									hitPlayer == nextPlayer ) {
										// try click on elderly
										hitPlayer =
												getHitPlayer( m.x, m.y, m.id,
															  false, -1,
															  55, &hitIndex );
									}

									if( ( hitPlayer == NULL ||
									hitPlayer == nextPlayer )
									&&
									holdingFood ) {

										// feeding action
										// try click on everyone
										hitPlayer =
												getHitPlayer( m.x, m.y, m.id,
															  false, -1, -1,
															  &hitIndex );
									}


									if( ( hitPlayer == NULL ||
									hitPlayer == nextPlayer )
									&&
									! holdingDrugs ) {

										// see if clicked-on player is dying
										hitPlayer =
												getHitPlayer( m.x, m.y, m.id,
															  false, -1, -1,
															  &hitIndex );
										if( hitPlayer != NULL &&
										! hitPlayer->dying ) {
											hitPlayer = NULL;
										}
									}


									if( hitPlayer != NULL &&
									hitPlayer != nextPlayer ) {

										targetPlayer = hitPlayer;

										playerIndicesToSendUpdatesAbout.push_back(
												hitIndex );

										targetPlayer->responsiblePlayerID =
												nextPlayer->id;
									}
								}
							}


							if( targetPlayer != NULL ) {

								// use on self/baby
								nextPlayer->actionAttempt = 1;
								nextPlayer->actionTarget.x = m.x;
								nextPlayer->actionTarget.y = m.y;

								double targetPlayerAge = computeAge( targetPlayer );

								if( targetPlayer != nextPlayer &&
								targetPlayer->dying &&
								! holdingFood ) {

									// try healing wound

									TransRecord *healTrans =
											getMetaTrans( nextPlayer->holdingID,
														  targetPlayer->holdingID );

									char oldEnough = true;

									if( healTrans != NULL ) {
										int healerWillHold = healTrans->newActor;

										if( healerWillHold > 0 ) {
											if( ! canPickup(
													healerWillHold,
													computeAge( nextPlayer ) ) ) {
												oldEnough = false;
											}
										}
									}

									char canDo = true;


									if( nextPlayer->holdingID > 0 &&
									! canPlayerUseOrLearnTool(
											nextPlayer,
											nextPlayer->holdingID ) ) {
										// player can't learn this tool
										canDo = false;
									}



									if( canDo && oldEnough && healTrans != NULL ) {
										targetPlayer->holdingID =
												healTrans->newTarget;
										holdingSomethingNew( targetPlayer );

										// their wound has been changed
										// no longer track embedded weapon
										targetPlayer->embeddedWeaponID = 0;
										targetPlayer->embeddedWeaponEtaDecay = 0;


										nextPlayer->holdingID =
												healTrans->newActor;
										holdingSomethingNew( nextPlayer );

										setFreshEtaDecayForHeld(
												nextPlayer );
										setFreshEtaDecayForHeld(
												targetPlayer );

										nextPlayer->heldOriginValid = 0;
										nextPlayer->heldOriginX = 0;
										nextPlayer->heldOriginY = 0;
										nextPlayer->heldTransitionSourceID =
												healTrans->target;

										targetPlayer->heldOriginValid = 0;
										targetPlayer->heldOriginX = 0;
										targetPlayer->heldOriginY = 0;
										targetPlayer->heldTransitionSourceID =
												-1;

										if( targetPlayer->holdingID == 0 ) {
											// not dying anymore

											if( targetPlayer->murderPerpID > 0 ) {
												logHealOfKill( targetPlayer,
															   nextPlayer );
											}

											setNoLongerDying(
													targetPlayer,
													&playerIndicesToSendHealingAbout );
										}
										else {
											// wound changed?

											ForcedEffects e =
													checkForForcedEffects(
															targetPlayer->holdingID );

											if( e.emotIndex != -1 ) {
												targetPlayer->emotFrozen = true;
												targetPlayer->emotFrozenIndex =
														e.emotIndex;
												newEmotPlayerIDs.push_back(
														targetPlayer->id );
												newEmotIndices.push_back(
														e.emotIndex );
												newEmotTTLs.push_back( e.ttlSec );
												interruptAnyKillEmots(
														targetPlayer->id, e.ttlSec );
											}
											if( e.foodCapModifier != 1 ) {
												targetPlayer->yummyBonusStore = 0;
												targetPlayer->foodCapModifier =
														e.foodCapModifier;
												targetPlayer->foodUpdate = true;
											}
											if( e.feverSet ) {
												targetPlayer->fever = e.fever;
											}
										}
									}
								}
								else if( nextPlayer->holdingID > 0 ) {
									ObjectRecord *obj =
											getObject( nextPlayer->holdingID );

									// don't use "live" computed capacity here
									// because that will allow player to spam
									// click to pack in food between food
									// decrements when they are growing
									// instead, stick to the food cap shown
									// in the client (what we last reported
									// to them)
									int cap = targetPlayer->lastReportedFoodCapacity;


									// first case:
									// player clicked on clothing
									// try adding held into clothing, but if
									// that fails go on to other cases

									// except do not force them to eat
									// something that could have gone
									// into a full clothing container!
									char couldHaveGoneIn = false;

									ObjectRecord *clickedClothing = NULL;
									TransRecord *clickedClothingTrans = NULL;
									if( m.i >= 0 &&
									m.i < NUM_CLOTHING_PIECES ) {
										clickedClothing =
												clothingByIndex( nextPlayer->clothing,
																 m.i );

										if( clickedClothing != NULL ) {

											clickedClothingTrans =
													getPTrans( nextPlayer->holdingID,
															   clickedClothing->id );

											if( clickedClothingTrans == NULL ) {
												// check if held has instant-decay
												TransRecord *heldDecay =
														getPTrans(
																-1,
																nextPlayer->holdingID );
												if( heldDecay != NULL &&
												heldDecay->autoDecaySeconds
												== 1 &&
												heldDecay->newTarget > 0 ) {

													// force decay NOW and try again
													handleHeldDecay(
															nextPlayer,
															i,
															&playerIndicesToSendUpdatesAbout,
															&playerIndicesToSendHealingAbout );
													clickedClothingTrans =
															getPTrans(
																	nextPlayer->holdingID,
																	clickedClothing->id );
												}
											}


											if( clickedClothingTrans != NULL ) {
												int na =
														clickedClothingTrans->newActor;

												if( na > 0 &&
												! canPickup(
														na,
														computeAge(
																nextPlayer ) ) ) {
													// too young for trans
													clickedClothingTrans = NULL;
												}

												int nt =
														clickedClothingTrans->newTarget;

												if( nt > 0 &&
												getObject( nt )->clothing
												!= clickedClothing->clothing ) {
													// don't allow transitions
													// that leave a non-wearable
													// item on your body
													// OR convert clothing into
													// a different type of clothing
													// (converting a shrit to a hat)
													clickedClothingTrans = NULL;
												}
											}
										}
									}


									if( targetPlayer == nextPlayer &&
									m.i >= 0 &&
									m.i < NUM_CLOTHING_PIECES &&
									addHeldToClothingContainer(
											nextPlayer,
											m.i,
											false,
											&couldHaveGoneIn) ) {
										// worked!
									}
									// next case:  can what they're holding
									// be used to transform clothing?
									else if( m.i >= 0 &&
									m.i < NUM_CLOTHING_PIECES &&
									clickedClothing != NULL &&
									clickedClothingTrans != NULL ) {

										// NOTE:
										// this is a niave way of handling
										// this case, and it doesn't deal
										// with all kinds of possible complexities
										// (like if the clothing decay time should
										//  change, or number of slots change)
										// Assuming that we won't add transitions
										// for clothing with those complexities
										// Works for removing sword
										// from backpack

										handleHoldingChange(
												nextPlayer,
												clickedClothingTrans->newActor );

										setClothingByIndex(
												&( nextPlayer->clothing ),
												m.i,
												getObject(
														clickedClothingTrans->newTarget ) );
									}
									// next case, holding food
									// that couldn't be put into clicked clothing
									else if( obj->foodValue > 0 &&
									( targetPlayerAge < defaultActionAge
									&& targetPlayer->foodStore >
									babyFeedingLevel )
									&&
									! couldHaveGoneIn ) {
										// special case for babies refusing
										// food when not starving.
										setRefuseFoodEmote( targetPlayer );
									}
									else if( obj->foodValue > 0 &&
									targetPlayer->foodStore < cap &&
									! couldHaveGoneIn ) {

										targetPlayer->justAte = true;
										targetPlayer->justAteID =
												nextPlayer->holdingID;

										targetPlayer->lastAteID =
												nextPlayer->holdingID;
										targetPlayer->lastAteFillMax =
												targetPlayer->foodStore;

										targetPlayer->foodStore +=
												getScaledFoodValue( targetPlayer,
																	obj->foodValue );

										updateYum( targetPlayer, obj->id,
												   targetPlayer == nextPlayer );

										int bonus = getEatBonus( targetPlayer );

										if( obj->alcohol > 0 ) {
											bonus = 0;
										}

										logEating( obj->id,
												   obj->foodValue + bonus,
												   targetPlayerAge,
												   m.x, m.y );

										targetPlayer->foodStore += bonus;

										checkForFoodEatingEmot( targetPlayer,
																obj->id );

										if( targetPlayer->foodStore > cap ) {
											int over =
													targetPlayer->foodStore - cap;

											targetPlayer->foodStore = cap;

											int overflowCap =
													computeOverflowFoodCapacity( cap );

											if( over > overflowCap ) {
												over = overflowCap;
											}
											targetPlayer->yummyBonusStore += over;
										}
										targetPlayer->foodDecrementETASeconds =
												Time::getCurrentTime() +
												computeFoodDecrementTimeSeconds(
														targetPlayer );

										// get eat transtion
										TransRecord *r =
												getPTrans( nextPlayer->holdingID,
														   -1 );



										if( r != NULL ) {
											int oldHolding = nextPlayer->holdingID;
											nextPlayer->holdingID = r->newActor;
											holdingSomethingNew( nextPlayer,
																 oldHolding );

											if( oldHolding !=
											nextPlayer->holdingID ) {

												setFreshEtaDecayForHeld(
														nextPlayer );
											}
										}
										else {
											// default, holding nothing after eating
											nextPlayer->holdingID = 0;
											nextPlayer->holdingEtaDecay = 0;
										}

										if( obj->alcohol != 0 ) {
											drinkAlcohol( targetPlayer,
														  obj->alcohol );
										}


										nextPlayer->heldOriginValid = 0;
										nextPlayer->heldOriginX = 0;
										nextPlayer->heldOriginY = 0;
										nextPlayer->heldTransitionSourceID = -1;

										targetPlayer->foodUpdate = true;
									}
									// final case, holding clothing that
									// we could put on
									else if( obj->clothing != 'n' &&
									( targetPlayer == nextPlayer
									||
									computeAge( targetPlayer ) <
									babyAge) ) {

										// wearable, dress self or baby

										nextPlayer->holdingID = 0;
										timeSec_t oldEtaDecay =
												nextPlayer->holdingEtaDecay;

										nextPlayer->holdingEtaDecay = 0;

										nextPlayer->heldOriginValid = 0;
										nextPlayer->heldOriginX = 0;
										nextPlayer->heldOriginY = 0;
										nextPlayer->heldTransitionSourceID = -1;

										ObjectRecord *oldC = NULL;
										timeSec_t oldCEtaDecay = 0;
										int oldNumContained = 0;
										int *oldContainedIDs = NULL;
										timeSec_t *oldContainedETADecays = NULL;


										ObjectRecord **clothingSlot = NULL;
										int clothingSlotIndex;

										switch( obj->clothing ) {
											case 'h':
												clothingSlot =
														&( targetPlayer->clothing.hat );
												clothingSlotIndex = 0;
												break;
												case 't':
													clothingSlot =
															&( targetPlayer->clothing.tunic );
													clothingSlotIndex = 1;
													break;
													case 'b':
														clothingSlot =
																&( targetPlayer->
																clothing.bottom );
														clothingSlotIndex = 4;
														break;
														case 'p':
															clothingSlot =
																	&( targetPlayer->
																	clothing.backpack );
															clothingSlotIndex = 5;
															break;
															case 's':
																if( targetPlayer->clothing.backShoe
																== NULL ) {

																	clothingSlot =
																			&( targetPlayer->
																			clothing.backShoe );
																	clothingSlotIndex = 3;

																}
																else if(
																		targetPlayer->clothing.frontShoe
																		== NULL ) {

																	clothingSlot =
																			&( targetPlayer->
																			clothing.frontShoe );
																	clothingSlotIndex = 2;
																}
																else {
																	// replace whatever shoe
																	// doesn't match what we're
																	// holding

																	if( targetPlayer->
																	clothing.backShoe ==
																	obj ) {

																		clothingSlot =
																				&( targetPlayer->
																				clothing.frontShoe );
																		clothingSlotIndex = 2;
																	}
																	else if( targetPlayer->
																	clothing.frontShoe ==
																	obj ) {
																		clothingSlot =
																				&( targetPlayer->
																				clothing.backShoe );
																		clothingSlotIndex = 3;
																	}
																	else {
																		// both shoes are
																		// different from what
																		// we're holding

																		// pick shoe to swap
																		// based on what we
																		// clicked on

																		if( m.i == 3 ) {
																			clothingSlot =
																					&( targetPlayer->
																					clothing.backShoe );
																			clothingSlotIndex = 3;
																		}
																		else {
																			// default to front
																			// shoe
																			clothingSlot =
																					&( targetPlayer->
																					clothing.frontShoe );
																			clothingSlotIndex = 2;
																		}
																	}
																}
																break;
										}

										if( clothingSlot != NULL ) {

											oldC = *clothingSlot;
											int ind = clothingSlotIndex;

											oldCEtaDecay =
													targetPlayer->clothingEtaDecay[ind];

											oldNumContained =
													targetPlayer->
													clothingContained[ind].size();

											if( oldNumContained > 0 ) {
												oldContainedIDs =
														targetPlayer->
														clothingContained[ind].
														getElementArray();
												oldContainedETADecays =
														targetPlayer->
														clothingContainedEtaDecays[ind].
														getElementArray();
											}

											*clothingSlot = obj;
											targetPlayer->clothingEtaDecay[ind] =
													oldEtaDecay;

											targetPlayer->
											clothingContained[ind].
											deleteAll();
											targetPlayer->
											clothingContainedEtaDecays[ind].
											deleteAll();

											if( nextPlayer->numContained > 0 ) {

												targetPlayer->clothingContained[ind]
												.appendArray(
														nextPlayer->containedIDs,
														nextPlayer->numContained );

												targetPlayer->
												clothingContainedEtaDecays[ind]
												.appendArray(
														nextPlayer->
														containedEtaDecays,
														nextPlayer->numContained );


												// ignore sub-contained
												// because clothing can
												// never contain containers
												clearPlayerHeldContained(
														nextPlayer );
											}


											if( oldC != NULL ) {
												nextPlayer->holdingID =
														oldC->id;
												holdingSomethingNew( nextPlayer );

												nextPlayer->holdingEtaDecay
												= oldCEtaDecay;

												nextPlayer->numContained =
														oldNumContained;

												freePlayerContainedArrays(
														nextPlayer );

												nextPlayer->containedIDs =
														oldContainedIDs;
												nextPlayer->containedEtaDecays =
														oldContainedETADecays;

												// empty sub-contained vectors
												// because clothing never
												// never contains containers
												nextPlayer->subContainedIDs
												= new SimpleVector<int>[
														nextPlayer->numContained ];
												nextPlayer->subContainedEtaDecays
												= new SimpleVector<timeSec_t>[
														nextPlayer->numContained ];
											}
										}
									}
								}
								else {
									// empty hand on self/baby, remove clothing

									int clothingSlotIndex = m.i;

									ObjectRecord **clothingSlot =
											getClothingSlot( targetPlayer, m.i );


									TransRecord *bareHandClothingTrans =
											getBareHandClothingTrans( nextPlayer,
																	  clothingSlot );


									if( targetPlayer == nextPlayer &&
									bareHandClothingTrans != NULL ) {

										// bare hand transforms clothing

										// this may not handle all possible cases
										// correctly.  A naive implementation for
										// now.  Works for removing sword
										// from backpack

										handleHoldingChange(
												nextPlayer,
												bareHandClothingTrans->newActor );

										nextPlayer->heldOriginValid = 0;
										nextPlayer->heldOriginX = 0;
										nextPlayer->heldOriginY = 0;


										if( bareHandClothingTrans->newTarget > 0 ) {
											*clothingSlot =
													getObject( bareHandClothingTrans->
													newTarget );
										}
										else {
											*clothingSlot = NULL;

											int ind = clothingSlotIndex;

											targetPlayer->clothingEtaDecay[ind] = 0;

											targetPlayer->clothingContained[ind].
											deleteAll();

											targetPlayer->
											clothingContainedEtaDecays[ind].
											deleteAll();
										}
									}
									else if( clothingSlot != NULL ) {
										// bare hand removes clothing

										removeClothingToHold( nextPlayer,
															  targetPlayer,
															  clothingSlot,
															  clothingSlotIndex );
									}
								}
							}
						}
						else if( m.type == DROP ) {
							//Thread::staticSleep( 2000 );

							// send update even if action fails (to let them
							// know that action is over)
							playerIndicesToSendUpdatesAbout.push_back( i );

							char canDrop = true;

							if( nextPlayer->holdingID > 0 &&
							getObject( nextPlayer->holdingID )->permanent ) {
								canDrop = false;
							}

							int target = getMapObject( m.x, m.y );


							char accessBlocked =
									isAccessBlocked( nextPlayer,
													 m.x, m.y, target );

							GridPos playerPos = getPlayerPos( nextPlayer );

							if( playerPos.x != m.x || playerPos.y != m.y ) {
								if( ! isBiomeAllowedForPlayer( nextPlayer,
															   playerPos.x,
															   playerPos.y ) ) {
									// don't allow player to drop/swap
									// from a bad biome into a good one
									// (they might be in a vehicle in the bad biome)
									accessBlocked = true;
								}
							}


							if( ! accessBlocked )
								if( isBiomeAllowedForPlayer( nextPlayer, m.x, m.y ) )
									if( ( isGridAdjacent( m.x, m.y,
														  nextPlayer->xd,
														  nextPlayer->yd )
														  ||
														  ( m.x == nextPlayer->xd &&
														  m.y == nextPlayer->yd )  ) ) {

										nextPlayer->actionAttempt = 1;
										nextPlayer->actionTarget.x = m.x;
										nextPlayer->actionTarget.y = m.y;

										if( m.x > nextPlayer->xd ) {
											nextPlayer->facingOverride = 1;
										}
										else if( m.x < nextPlayer->xd ) {
											nextPlayer->facingOverride = -1;
										}

										if( nextPlayer->holdingID != 0 ) {

											if( nextPlayer->holdingID < 0 ) {
												// baby drop

												if( target == 0 // nothing here
												||
												! getObject( target )->
												blocksWalking ) {
													handleDrop(
															m.x, m.y, nextPlayer,
															&playerIndicesToSendUpdatesAbout );
												}
											}
											else if( canDrop &&
											isMapSpotEmpty( m.x, m.y ) ) {

												// empty spot to drop non-baby into

												handleDrop(
														m.x, m.y, nextPlayer,
														&playerIndicesToSendUpdatesAbout );
											}
											else if( canDrop &&
											m.c >= 0 &&
											m.c < NUM_CLOTHING_PIECES &&
											m.x == nextPlayer->xd &&
											m.y == nextPlayer->yd  &&
											nextPlayer->holdingID > 0 ) {

												// drop into clothing indicates right-click
												// so swap

												int oldHeld = nextPlayer->holdingID;

												// first add to top of container
												// if possible
												addHeldToClothingContainer( nextPlayer,
																			m.c,
																			true );
												if( nextPlayer->holdingID == 0 ) {
													// add to top worked

													double playerAge =
															computeAge( nextPlayer );

													// now take off bottom to hold
													// but keep looking to find something
													// different than what we were
													// holding before
													// AND that we are old enough to pick
													// up
													for( int s=0;
													s < nextPlayer->
													clothingContained[m.c].size()
													- 1;
													s++ ) {

														int otherID =
																nextPlayer->
																clothingContained[m.c].
																getElementDirect( s );

														if( otherID !=
														oldHeld &&
														canPickup( otherID,
																   playerAge ) ) {

															removeFromClothingContainerToHold(
																	nextPlayer, m.c, s );
															break;
														}
													}

													// check to make sure remove worked
													// (otherwise swap failed)
													ObjectRecord *cObj =
															clothingByIndex(
																	nextPlayer->clothing, m.c );
													if( nextPlayer->clothingContained[m.c].
													size() > cObj->numSlots ) {

														// over-full, remove failed

														// pop top item back off into hand
														removeFromClothingContainerToHold(
																nextPlayer, m.c,
																nextPlayer->
																clothingContained[m.c].
																size() - 1 );
													}
												}

											}
											else if( nextPlayer->holdingID > 0 ) {
												// non-baby drop

												ObjectRecord *droppedObj
												= getObject(
														nextPlayer->holdingID );

												if( target != 0 ) {

													ObjectRecord *targetObj =
															getObject( target );


													if( !canDrop ) {
														// user may have a permanent object
														// stuck in their hand with no place
														// to drop it

														// need to check if
														// a use-on-bare-ground
														// transition applies.  If so, we
														// can treat it like a swap


														if( ! targetObj->permanent
														&&
														canPickup(
																targetObj->id,
																computeAge(
																		nextPlayer ) ) ) {

															// target can be picked up

															// "set-down" type bare ground
															// trans exists?
															TransRecord
															*r = getPTrans(
																	nextPlayer->holdingID,
																	-1 );

															if( r != NULL &&
															r->newActor == 0 &&
															r->newTarget > 0 ) {

																// only applies if the
																// bare-ground
																// trans leaves nothing in
																// our hand

																// now swap it with the
																// non-permanent object
																// on the ground.

																swapHeldWithGround(
																		nextPlayer,
																		target,
																		m.x,
																		m.y,
																		&playerIndicesToSendUpdatesAbout );
															}
														}
													}


													int targetSlots =
															targetObj->numSlots;

													float targetSlotSize = 0;

													if( targetSlots > 0 ) {
														targetSlotSize =
																targetObj->slotSize;
													}

													char canGoIn = false;

													if( canDrop &&
													droppedObj->containable &&
													targetSlotSize >=
													droppedObj->containSize &&
													containmentPermitted(
															target,
															droppedObj->id ) ) {
														canGoIn = true;
													}

													char forceUse = false;

													if( canDrop &&
													canGoIn &&
													targetSlots > 0 &&
													nextPlayer->numContained == 0 &&
													getNumContained( m.x, m.y ) == 0 ) {

														// container empty
														// is there a transition that might
														// apply instead?

														// only consider a consuming
														// transition (custom containment
														// like grapes in a basket which
														// aren't in container slots )

														TransRecord *t =
																getPTrans(
																		nextPlayer->holdingID,
																		target );

														if( t != NULL &&
														t->newActor == 0 ) {
															forceUse = true;
														}
													}


													// DROP indicates they
													// right-clicked on container
													// so use swap mode
													if( canDrop &&
													canGoIn &&
													! forceUse &&
													addHeldToContainer(
															nextPlayer,
															target,
															m.x, m.y, true ) ) {
														// handled
													}
													else if( forceUse ||
													( canDrop &&
													! canGoIn &&
													targetObj->permanent &&
													nextPlayer->numContained
													== 0 ) ) {
														// try treating it like
														// a USE action
														m.type = USE;
														m.id = -1;
														m.c = -1;
														playerIndicesToSendUpdatesAbout.
														deleteElementEqualTo( i );
														goto RESTART_MESSAGE_ACTION;
													}
													else if( canDrop &&
													! canGoIn &&
													! targetObj->permanent
													&&
													canPickup(
															targetObj->id,
															computeAge(
																	nextPlayer ) ) ) {
														// drop onto a spot where
														// something exists, and it's
														// not a container

														// swap what we're holding for
														// target

														int oldHeld =
																nextPlayer->holdingID;
														int oldNumContained =
																nextPlayer->numContained;

														// now swap
														swapHeldWithGround(
																nextPlayer, target, m.x, m.y,
																&playerIndicesToSendUpdatesAbout );

														if( oldHeld ==
														nextPlayer->holdingID &&
														oldNumContained ==
														nextPlayer->numContained ) {
															// no change
															// are they the same object?
															if( oldNumContained == 0 &&
															oldHeld == target ) {
																// try using empty held
																// on target
																TransRecord *sameTrans
																= getPTrans(
																		oldHeld, target );
																if( sameTrans != NULL &&
																sameTrans->newActor ==
																0 ) {
																	// keep it simple
																	// for now
																	// this is usually
																	// just about
																	// stacking
																	handleHoldingChange(
																			nextPlayer,
																			sameTrans->
																			newActor );

																	setMapObject(
																			m.x, m.y,
																			sameTrans->
																			newTarget );
																}
															}
														}
													}
												}
												else if( canDrop ) {
													// no object here

													// maybe there's a person
													// standing here

													// only allow drop if what we're
													// dropping is non-blocking


													if( ! droppedObj->blocksWalking ) {

														handleDrop(
																m.x, m.y, nextPlayer,
																&playerIndicesToSendUpdatesAbout
																);
													}
												}
											}
										}
									}
						}
						else if( m.type == REMV ) {
							// send update even if action fails (to let them
							// know that action is over)
							playerIndicesToSendUpdatesAbout.push_back( i );

							if( isBiomeAllowedForPlayer( nextPlayer, m.x, m.y ) )
								if( ! isPlayerBlockedFromHoldingByPosse( nextPlayer ) )
									if( isGridAdjacent( m.x, m.y,
														nextPlayer->xd,
														nextPlayer->yd )
														||
														( m.x == nextPlayer->xd &&
														m.y == nextPlayer->yd ) ) {

										int target = getMapObject( m.x, m.y );

										char accessBlocked =
												isAccessBlocked( nextPlayer, m.x, m.y, target );


										char handEmpty = ( nextPlayer->holdingID == 0 );

										if( ! accessBlocked )
											removeFromContainerToHold( nextPlayer,
																	   m.x, m.y, m.i );

										if( ! accessBlocked )
											if( handEmpty &&
											nextPlayer->holdingID == 0 ) {
												// hand still empty?

												int target = getMapObject( m.x, m.y );

												if( target > 0 ) {
													ObjectRecord *targetObj =
															getObject( target );

													if( targetObj->normalOnly &&
													( nextPlayer->isTutorial ||
													nextPlayer->
													curseStatus.curseLevel > 0 ) ) {

														// non-normal player blocked
														targetObj = NULL;
													}

													if( targetObj != NULL &&
													! targetObj->permanent &&
													canPickup( targetObj->id,
															   computeAge(
																	nextPlayer ) ) ) {

														// treat it like pick up
														pickupToHold( nextPlayer, m.x, m.y,
																	  target );
													}
													else if( targetObj != NULL &&
													targetObj->permanent ) {
														// consider bare-hand action
														TransRecord *handTrans = getPTrans(
																0, target );

														if( handTrans == NULL ) {
															// check for instant decay
															int newTarget =
																	checkTargetInstantDecay(
																			target, m.x, m.y );

															// if so, let transition go through
															// (skip if result of decay is 0)
															if( newTarget != 0 &&
															newTarget != target ) {

																target = newTarget;
																targetObj = getObject( target );

																handTrans =
																		getPTrans( 0, target );
															}
														}


														// handle only simplest case here
														// (to avoid side-effects)
														// REMV on container stack
														// (make sure they have the same
														//  use parent)
														if( handTrans != NULL &&
														handTrans->newTarget > 0 &&
														getObject( handTrans->newTarget )->
														numSlots == targetObj->numSlots &&
														handTrans->newActor > 0 &&
														canPickup(
																handTrans->newActor,
																computeAge( nextPlayer ) ) ) {

															handleHoldingChange(
																	nextPlayer,
																	handTrans->newActor );
															setMapObject(
																	m.x, m.y,
																	handTrans->newTarget );
														}
													}
												}
											}
									}
						}
						else if( m.type == SREMV ) {
							playerIndicesToSendUpdatesAbout.push_back( i );

							// remove contained object from clothing
							char worked = false;

							if( m.x == nextPlayer->xd &&
							m.y == nextPlayer->yd &&
							nextPlayer->holdingID == 0 ) {

								nextPlayer->actionAttempt = 1;
								nextPlayer->actionTarget.x = m.x;
								nextPlayer->actionTarget.y = m.y;

								if( m.c >= 0 && m.c < NUM_CLOTHING_PIECES ) {
									worked = removeFromClothingContainerToHold(
											nextPlayer, m.c, m.i );
								}
							}

							if( nextPlayer->holdingID == 0 &&
							m.c >= 0 && m.c < NUM_CLOTHING_PIECES  &&
							! worked ) {

								// hmm... nothing to remove from slots in clothing

								// player is right-clicking, and maybe they
								// can't left-click, because there's a
								// transition in the way

								// if so, right click should
								// remove the clothing itself

								ObjectRecord **clothingSlot =
										getClothingSlot( nextPlayer, m.c );


								TransRecord *bareHandClothingTrans =
										getBareHandClothingTrans( nextPlayer,
																  clothingSlot );

								if( bareHandClothingTrans != NULL ) {
									// there's a transition blocking
									// regular-click to remove empty
									// clothing.
									// allow right click to do it

									removeClothingToHold( nextPlayer,
														  nextPlayer,
														  clothingSlot,
														  m.c );
								}
							}
						}
						else if( m.type == EMOT &&
						! nextPlayer->emotFrozen ) {
							// ignore new EMOT requres from player if emot
							// frozen

							if( m.i <= SettingsManager::getIntSetting(
									"allowedEmotRange", 6 ) ) {

								SimpleVector<int> *forbidden =
										SettingsManager::getIntSettingMulti(
												"forbiddenEmots" );

								if( forbidden->getElementIndex( m.i ) == -1 ) {
									// not forbidden

									double curTime = Time::getCurrentTime();

									char cooldown = false;

									if( nextPlayer->emoteCooldown ) {
										if( curTime -
										nextPlayer->
										emoteCooldownStartTimeSeconds >
										emoteCooldownSeconds ) {
											// cooldown over
											nextPlayer->emoteCooldown = false;
											nextPlayer->firstEmoteTimeSeconds =
													curTime;
											nextPlayer->emoteCountInWindow = 0;
										}
										else {
											cooldown = true;
										}
									}

									if( ! cooldown ) {
										// fire off emote
										newEmotPlayerIDs.push_back(
												nextPlayer->id );
										newEmotIndices.push_back( m.i );
										// player-requested emots have
										// no specific TTL
										newEmotTTLs.push_back( 0 );

										// now see if cooldown has been triggered
										if( curTime -
										nextPlayer->firstEmoteTimeSeconds
										> emoteWindowSeconds ) {
											// window expired
											// start a new one
											nextPlayer->firstEmoteTimeSeconds =
													curTime;
											nextPlayer->emoteCountInWindow = 0;
										}
										else {
											// in window time
											nextPlayer->emoteCountInWindow ++;

											if( nextPlayer->emoteCountInWindow >
											maxEmotesInWindow ) {
												// put 'em on cooldown
												nextPlayer->emoteCooldown = true;
												nextPlayer->
												emoteCooldownStartTimeSeconds =
														curTime;
											}
										}
									}
								}
								delete forbidden;
							}
						}
					}

					if( m.numExtraPos > 0 ) {
						delete [] m.extraPos;
					}

					if( m.saidText != NULL ) {
						delete [] m.saidText;
					}
					if( m.bugText != NULL ) {
						delete [] m.bugText;
					}
				}
			}


			// process pending KILL actions
			for( int i=0; i<activeKillStates.size(); i++ ) {
				KillState *s = activeKillStates.getElement( i );

				LiveObject *killer = getLiveObject( s->killerID );
				LiveObject *target = getLiveObject( s->targetID );

				if( killer == NULL || target == NULL ||
				killer->error || target->error ||
				killer->holdingID != s->killerWeaponID ||
				target->heldByOther ) {
					// either player dead, or held-weapon change
					// or target baby now picked up (safe)

					// kill request done

					removeKillState( killer, target );

					i--;
					continue;
				}

				// kill request still active!

				// see if it is realized (close enough)?
				GridPos playerPos = getPlayerPos( killer );
				GridPos targetPos = getPlayerPos( target );

				double dist = distance( playerPos, targetPos );

				double curTime = Time::getCurrentTime();

				// vary delay based on posse size
				double delay = killDelayTime;
				if( posseDelayReductionFactor > 0 && s->posseSize > 1 ) {
					delay /=
							pow( posseDelayReductionFactor, s->posseSize - 1 );
				}

				if( curTime - s->killStartTime  > delay &&
				s->posseSize >= s->minPosseSizeForKill &&
				getObject( killer->holdingID )->deadlyDistance >= dist &&
				! directLineBlocked( killer, playerPos, targetPos ) ) {
					// enough warning time has passed
					// and
					// posse meets min size requirements
					// and
					// close enough to kill

					executeKillAction( getLiveObjectIndex( s->killerID ),
									   getLiveObjectIndex( s->targetID ),
									   &playerIndicesToSendUpdatesAbout,
									   &playerIndicesToSendDyingAbout,
									   &newEmotPlayerIDs,
									   &newEmotIndices,
									   &newEmotTTLs );
				}
				else {
					// still not close enough
					// see if we need to renew emote

					if( ! isNoWaitWeapon( s->killerWeaponID ) )
						if( curTime - s->emotStartTime > s->emotRefreshSeconds ||
						( s->posseSize >= s->minPosseSizeForKill &&
						target->emotFrozenIndex !=
						victimTerrifiedEmotionIndex ) ||
						( s->posseSize < s->minPosseSizeForKill &&
						target->emotFrozenIndex !=
						victimEmotionIndex ) ) {

							// emote time expired OR posse size changed
							// and demands different victim emote

							s->emotStartTime = curTime;

							// refresh again in 30 seconds, even if we had a shorter
							// refresh time because of an intervening emot
							s->emotRefreshSeconds = 30;

							newEmotPlayerIDs.push_back( killer->id );

							newEmotIndices.push_back( killEmotionIndex );
							newEmotTTLs.push_back( 120 );

							newEmotPlayerIDs.push_back( target->id );

							int emotIndex = victimEmotionIndex;

							if( s->posseSize >= s->minPosseSizeForKill ) {
								emotIndex = victimTerrifiedEmotionIndex;


								if( s->posseSize > 1 ) {
									tryToForceDropHeld(
											target,
											&playerIndicesToSendUpdatesAbout );
								}
							}

							newEmotIndices.push_back( emotIndex );
							target->emotFrozenIndex = emotIndex;


							newEmotTTLs.push_back( 120 );
						}
				}
			}



			// now that messages have been processed for all
			// loop over and handle all post-message checks

			// for example, if a player later in the list sends a message
			// killing an earlier player, we need to check to see that
			// player deleted afterward here
			for( int i=0; i<numLive; i++ ) {
				LiveObject *nextPlayer = players.getElement( i );

				double curTime = Time::getCurrentTime();


				if( nextPlayer->emotFrozen &&
				nextPlayer->emotUnfreezeETA != 0 &&
				curTime >= nextPlayer->emotUnfreezeETA ) {

					nextPlayer->emotFrozen = false;
					nextPlayer->emotUnfreezeETA = 0;
				}

				if( ! nextPlayer->error &&
				! nextPlayer->cravingKnown &&
				computeAge( nextPlayer ) >= minAgeForCravings ) {

					sendCraving( nextPlayer );
				}



				if( nextPlayer->dying && ! nextPlayer->error &&
				curTime >= nextPlayer->dyingETA ) {
					// finally died
					nextPlayer->error = true;


					if( ! nextPlayer->isTutorial ) {
						GridPos deathPos =
								getPlayerPos( nextPlayer );
						logDeath( nextPlayer->id,
								  nextPlayer->email,
								  nextPlayer->isEve,
								  computeAge( nextPlayer ),
								  getSecondsPlayed(
										nextPlayer ),
										! getFemale( nextPlayer ),
										deathPos.x, deathPos.y,
										players.size() - 1,
										false,
										nextPlayer->murderPerpID,
										nextPlayer->murderPerpEmail );

						if( shutdownMode ) {
							handleShutdownDeath(
									nextPlayer, nextPlayer->xd, nextPlayer->yd );
						}
					}

					nextPlayer->deathLogged = true;
				}



				if( nextPlayer->isNew ) {
					// their first position is an update


					playerIndicesToSendUpdatesAbout.push_back( i );
					playerIndicesToSendLineageAbout.push_back( i );


					if( nextPlayer->curseStatus.curseLevel > 0 ) {
						playerIndicesToSendCursesAbout.push_back( i );
					}

					if( usePersonalCurses ) {
						// send a unique CU message to each player
						// who has this player cursed

						// but wait until next step, because other players
						// haven't heard initial PU about this player yet
						nextPlayer->isNewCursed = true;
					}

					nextPlayer->isNew = false;

					// force this PU to be sent to everyone
					nextPlayer->updateGlobal = true;
				}
				else if( nextPlayer->isNewCursed ) {
					// update sent about this new player
					// time to send personal curse status (b/c other players
					// know about this player now)
					for( int p=0; p<players.size(); p++ ) {
						LiveObject *otherPlayer = players.getElement( p );

						if( otherPlayer == nextPlayer ) {
							continue;
						}
						if( otherPlayer->error ||
						! otherPlayer->connected ) {
							continue;
						}

						if( isCursed( otherPlayer->email,
									  nextPlayer->email ) ) {
							char *message = autoSprintf(
									"CU\n%d 1 %s_%s\n#",
									nextPlayer->id,
									getCurseWord( otherPlayer->email,
												  nextPlayer->email, 0 ),
												  getCurseWord( otherPlayer->email,
																nextPlayer->email, 1 ) );

							sendMessageToPlayer( otherPlayer,
												 message, strlen( message ) );
							delete [] message;
						}
					}
					nextPlayer->isNewCursed = false;
				}
				else if( nextPlayer->error && ! nextPlayer->deleteSent ) {

					// generate log line whenever player dies
					logFamilyCounts();


					// check if we should send global message about a family's
					// demise
					if( ! nextPlayer->isTutorial &&
					nextPlayer->curseStatus.curseLevel == 0 &&
					! isEveWindow() ) {
						int minFamiliesAfterEveWindow =
								SettingsManager::getIntSetting(
										"minFamiliesAfterEveWindow", 5 );
						if( minFamiliesAfterEveWindow > 0 ) {
							// is this the last player of this family?

							if( nextPlayer->familyName != NULL ) {
								int otherCount = 0;

								for( int n=0; n<players.size(); n++ ) {
									LiveObject *otherPlayer =
											players.getElement( n );

									if( otherPlayer->error ) {
										// don't worry about counting
										// nextPlayer here, b/c they have an
										// error set already
										continue;
									}
									if( otherPlayer->lineageEveID ==
									nextPlayer->lineageEveID ) {

										otherCount++;
										// actually, only need to count 1
										break;
									}
								}
								if( otherCount == 0 ) {
									// family died out!
									int cFam = countFamilies();

									const char *famWord = "FAMILIES";
									if( cFam == 1 ) {
										famWord = "FAMILY";
									}

									char *message =
											autoSprintf( "%s FAMILY JUST DIED OUT**"
														 "%d %s LEFT "
														 "(ARC ENDS BELOW %d)",
														 nextPlayer->familyName,
														 cFam,
														 famWord,
														 minFamiliesAfterEveWindow );

									sendGlobalMessage( message );
									delete [] message;
								}
							}
						}
					}

					leaderDied( nextPlayer );

					removeAllOwnership( nextPlayer );

					int numLangSpeakersLeft =
							decrementLanguageCount( nextPlayer->lineageEveID );

					if( numLangSpeakersLeft == 0 ) {
						// last member of this family died out
						homelandsDead( nextPlayer->lineageEveID );
					}

					removePlayerLanguageMaps( nextPlayer->id );

					if( nextPlayer->heldByOther ) {

						handleForcedBabyDrop( nextPlayer,
											  &playerIndicesToSendUpdatesAbout );
					}
					else if( nextPlayer->holdingID < 0 ) {
						LiveObject *babyO =
								getLiveObject( - nextPlayer->holdingID );

						handleForcedBabyDrop( babyO,
											  &playerIndicesToSendUpdatesAbout );
					}


					newDeleteUpdates.push_back(
							getUpdateRecord( nextPlayer, true ) );

					nextPlayer->deathTimeSeconds = Time::getCurrentTime();

					nextPlayer->isNew = false;

					nextPlayer->deleteSent = true;
					// wait 10 seconds before closing their connection
					// so they can get the message
					nextPlayer->deleteSentDoneETA = Time::getCurrentTime() + 10;

					if( areTriggersEnabled() ) {
						// add extra time so that rest of triggers can be received
						// and rest of trigger results can be sent
						// back to this client

						// another hour...
						nextPlayer->deleteSentDoneETA += 3600;
						// and don't set their error flag after all
						// keep receiving triggers from them

						nextPlayer->error = false;
					}
					else {
						if( nextPlayer->sock != NULL ) {
							// stop listening for activity on this socket
							sockPoll.removeSocket( nextPlayer->sock );
						}
					}


					GridPos dropPos;

					if( nextPlayer->xd ==
					nextPlayer->xs &&
					nextPlayer->yd ==
					nextPlayer->ys ) {
						// deleted player standing still

						dropPos.x = nextPlayer->xd;
						dropPos.y = nextPlayer->yd;
					}
					else {
						// player moving

						dropPos =
								computePartialMoveSpot( nextPlayer );
					}

					// report to lineage server once here
					double age = computeAge( nextPlayer );

					int killerID = -1;
					if( nextPlayer->murderPerpID > 0 ) {
						killerID = nextPlayer->murderPerpID;
					}
					else if( nextPlayer->deathSourceID > 0 ) {
						// include as negative of ID
						killerID = - nextPlayer->deathSourceID;
					}
					else if( nextPlayer->suicide ) {
						// self id is killer
						killerID = nextPlayer->id;
					}



					char male = ! getFemale( nextPlayer );

					if( ! nextPlayer->isTutorial )
						recordPlayerLineage( nextPlayer->email,
											 age,
											 nextPlayer->id,
											 nextPlayer->parentID,
											 nextPlayer->displayID,
											 killerID,
											 nextPlayer->name,
											 nextPlayer->lastSay,
											 male );


					// both tutorial and non-tutorial players
					logFitnessDeath( nextPlayer );


					if( age < shortLifeAge ) {
						addShortLife( nextPlayer->email );
					}


					if( SettingsManager::getIntSetting(
							"babyApocalypsePossible", 1 )
							&&
							players.size() >
							SettingsManager::getIntSetting(
									"minActivePlayersForBabyApocalypse", 15 ) ) {

						double curTime = Time::getCurrentTime();

						if( ! nextPlayer->isEve ) {

							// player was born as a baby

							int barrierRadius =
									SettingsManager::getIntSetting(
											"barrierRadius", 250 );
							int barrierOn = SettingsManager::getIntSetting(
									"barrierOn", 1 );

							char insideBarrier = true;

							if( barrierOn &&
							( abs( dropPos.x ) > barrierRadius ||
							abs( dropPos.y ) > barrierRadius ) ) {

								insideBarrier = false;
							}


							float threshold = SettingsManager::getFloatSetting(
									"babySurvivalYearsBeforeApocalypse", 15.0f );

							if( insideBarrier && age > threshold ) {
								// baby passed threshold, update last-passed time
								lastBabyPassedThresholdTime = curTime;
							}
							else {
								// baby died young
								// OR older, outside barrier
								// check if we're due for an apocalypse

								if( lastBabyPassedThresholdTime > 0 &&
								curTime - lastBabyPassedThresholdTime >
								SettingsManager::getIntSetting(
										"babySurvivalWindowSecondsBeforeApocalypse",
										3600 ) ) {
									// we're outside the window
									// people have been dying young for a long time

									triggerApocalypseNow(
											"Everyone dying young for too long" );
								}
								else if( lastBabyPassedThresholdTime == 0 ) {
									// first baby to die, and we have enough
									// active players.

									// start window now
									lastBabyPassedThresholdTime = curTime;
								}
							}
						}
					}
					else {
						// not enough players
						// reset window
						lastBabyPassedThresholdTime = curTime;
					}


					// don't use age here, because it unfairly gives Eve
					// +14 years that she didn't actually live
					// use true played years instead
					double yearsLived =
							getSecondsPlayed( nextPlayer ) * getAgeRate();

					if( ! nextPlayer->isTutorial ) {

						recordLineage(
								nextPlayer->email,
								nextPlayer->originalBirthPos,
								yearsLived,
								// count true murder victims here, not suicide
								( killerID > 0 && killerID != nextPlayer->id ),
								// killed other or committed SID suicide
								nextPlayer->everKilledAnyone ||
								nextPlayer->suicide );

						if( nextPlayer->suicide ) {
							// add to player's skip list
							skipFamily( nextPlayer->email,
										nextPlayer->lineageEveID );
						}
					}



					if( ! nextPlayer->deathLogged ) {
						char disconnect = true;

						if( age >= forceDeathAge ) {
							disconnect = false;
						}

						if( ! nextPlayer->isTutorial ) {
							logDeath( nextPlayer->id,
									  nextPlayer->email,
									  nextPlayer->isEve,
									  age,
									  getSecondsPlayed( nextPlayer ),
									  male,
									  dropPos.x, dropPos.y,
									  players.size() - 1,
									  disconnect,
									  nextPlayer->murderPerpID,
									  nextPlayer->murderPerpEmail );

							if( shutdownMode ) {
								handleShutdownDeath(
										nextPlayer, dropPos.x, dropPos.y );
							}
						}

						nextPlayer->deathLogged = true;
					}

					// now that death has been logged, and delete sent,
					// we can clear their email address so that the
					// can log in again during the deleteSentDoneETA window

					if( nextPlayer->email != NULL ) {
						if( nextPlayer->origEmail != NULL ) {
							delete [] nextPlayer->origEmail;
						}
						nextPlayer->origEmail =
								stringDuplicate( nextPlayer->email );
						delete [] nextPlayer->email;
					}
					nextPlayer->email = stringDuplicate( "email_cleared" );

					int deathID = getRandomDeathMarker();

					if( nextPlayer->customGraveID > -1 ) {
						deathID = nextPlayer->customGraveID;
					}

					char deathMarkerHasSlots = false;

					if( deathID > 0 ) {
						deathMarkerHasSlots =
								( getObject( deathID )->numSlots > 0 );
					}

					int oldObject = getMapObject( dropPos.x, dropPos.y );

					SimpleVector<int> oldContained;
					SimpleVector<timeSec_t> oldContainedETADecay;

					if( deathID != 0 ) {


						int nX[4] = { -1, 1,  0, 0 };
						int nY[4] = {  0, 0, -1, 1 };

						int n = 0;
						GridPos centerDropPos = dropPos;

						while( oldObject != 0 && n < 4 ) {

							// don't combine graves
							if( ! isGrave( oldObject ) ) {
								ObjectRecord *r = getObject( oldObject );

								if( deathMarkerHasSlots &&
								r->numSlots == 0 && ! r->permanent
								&& ! r->rideable ) {

									// found a containble object
									// we can empty this spot to make room
									// for a grave that can go here, and
									// put the old object into the new grave.

									oldContained.push_back( oldObject );
									oldContainedETADecay.push_back(
											getEtaDecay( dropPos.x, dropPos.y ) );

									setMapObject( dropPos.x, dropPos.y, 0 );
									oldObject = 0;
								}
							}

							oldObject = getMapObject( dropPos.x, dropPos.y );

							if( oldObject != 0 ) {

								// try next neighbor
								dropPos.x = centerDropPos.x + nX[n];
								dropPos.y = centerDropPos.y + nY[n];

								n++;
								oldObject = getMapObject( dropPos.x, dropPos.y );
							}
						}
					}


					if( ! isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {

						// failed to find an empty spot, or a containable object
						// at center or four neighbors

						// search outward in spiral of up to 100 points
						// look for some empty spot

						char foundEmpty = false;

						GridPos newDropPos = findClosestEmptyMapSpot(
								dropPos.x, dropPos.y, 100, &foundEmpty );

						if( foundEmpty ) {
							dropPos = newDropPos;
						}
					}


					// assume death markes non-blocking, so it's safe
					// to drop one even if other players standing here
					if( isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {

						if( deathID > 0 ) {

							setResponsiblePlayer( - nextPlayer->id );
							setMapObject( dropPos.x, dropPos.y,
										  deathID );
							setResponsiblePlayer( -1 );

							GraveInfo graveInfo = { dropPos, nextPlayer->id,
													nextPlayer->lineageEveID };
							newGraves.push_back( graveInfo );

							setGravePlayerID( dropPos.x, dropPos.y,
											  nextPlayer->id );

							ObjectRecord *deathObject = getObject( deathID );

							int roomLeft = deathObject->numSlots;

							if( roomLeft >= 1 ) {
								// room for weapon remnant
								if( nextPlayer->embeddedWeaponID != 0 ) {
									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->embeddedWeaponID,
											nextPlayer->embeddedWeaponEtaDecay );
									roomLeft--;
								}
							}


							if( roomLeft >= 5 ) {
								// room for clothing

								if( nextPlayer->clothing.tunic != NULL ) {

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->clothing.tunic->id,
											nextPlayer->clothingEtaDecay[1] );
									roomLeft--;
								}
								if( nextPlayer->clothing.bottom != NULL ) {

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->clothing.bottom->id,
											nextPlayer->clothingEtaDecay[4] );
									roomLeft--;
								}
								if( nextPlayer->clothing.backpack != NULL ) {

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->clothing.backpack->id,
											nextPlayer->clothingEtaDecay[5] );
									roomLeft--;
								}
								if( nextPlayer->clothing.backShoe != NULL ) {

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->clothing.backShoe->id,
											nextPlayer->clothingEtaDecay[3] );
									roomLeft--;
								}
								if( nextPlayer->clothing.frontShoe != NULL ) {

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->clothing.frontShoe->id,
											nextPlayer->clothingEtaDecay[2] );
									roomLeft--;
								}
								if( nextPlayer->clothing.hat != NULL ) {

									addContained( dropPos.x, dropPos.y,
												  nextPlayer->clothing.hat->id,
												  nextPlayer->clothingEtaDecay[0] );
									roomLeft--;
								}
							}

							// room for what clothing contained
							timeSec_t curTime = Time::timeSec();

							for( int c=0; c < NUM_CLOTHING_PIECES && roomLeft > 0;
							c++ ) {

								float oldStretch = 1.0;

								ObjectRecord *cObj = clothingByIndex(
										nextPlayer->clothing, c );

								if( cObj != NULL ) {
									oldStretch = cObj->slotTimeStretch;
								}

								float newStretch = deathObject->slotTimeStretch;

								for( int cc=0;
								cc < nextPlayer->clothingContained[c].size()
								&&
								roomLeft > 0;
								cc++ ) {

									if( nextPlayer->
									clothingContainedEtaDecays[c].
									getElementDirect( cc ) != 0 &&
									oldStretch != newStretch ) {

										timeSec_t offset =
												nextPlayer->
												clothingContainedEtaDecays[c].
												getElementDirect( cc ) -
												curTime;

										offset = offset * oldStretch;
										offset = offset / newStretch;

										*( nextPlayer->
										clothingContainedEtaDecays[c].
										getElement( cc ) ) =
												curTime + offset;
									}

									addContained(
											dropPos.x, dropPos.y,
											nextPlayer->
											clothingContained[c].
											getElementDirect( cc ),
											nextPlayer->
											clothingContainedEtaDecays[c].
											getElementDirect( cc ) );
									roomLeft --;
								}
							}

							int oc = 0;

							while( oc < oldContained.size() && roomLeft > 0 ) {
								addContained(
										dropPos.x, dropPos.y,
										oldContained.getElementDirect( oc ),
										oldContainedETADecay.getElementDirect( oc ) );
								oc++;
								roomLeft--;
							}
						}
					}
					if( nextPlayer->holdingID != 0 ) {

						char doNotDrop = false;

						if( nextPlayer->murderSourceID > 0 ) {

							TransRecord *woundHit =
									getPTrans( nextPlayer->murderSourceID,
											   0, true, false );

							if( woundHit != NULL &&
							woundHit->newTarget > 0 ) {

								if( nextPlayer->holdingID == woundHit->newTarget ) {
									// they are simply holding their wound object
									// don't drop this on the ground
									doNotDrop = true;
								}
							}
						}
						if( nextPlayer->holdingWound ||
						nextPlayer->holdingBiomeSickness ) {
							// holding a wound from some other, non-murder cause
							// of death
							doNotDrop = true;
						}


						if( ! doNotDrop ) {
							// drop what they were holding

							// this will almost always involve a throw
							// (death marker, at least, will be in the way)
							handleDrop(
									dropPos.x, dropPos.y,
									nextPlayer,
									&playerIndicesToSendUpdatesAbout );
						}
						else {
							// just clear what they were holding
							nextPlayer->holdingID = 0;
						}
					}
				}
				else if( ! nextPlayer->error ) {
					// other update checks for living players

					if( nextPlayer->holdingEtaDecay != 0 &&
					nextPlayer->holdingEtaDecay < curTime ) {

						// what they're holding has decayed
						handleHeldDecay( nextPlayer, i,
										 &playerIndicesToSendUpdatesAbout,
										 &playerIndicesToSendHealingAbout );
					}

					// check if anything in the container they are holding
					// has decayed
					if( nextPlayer->holdingID > 0 &&
					nextPlayer->numContained > 0 ) {

						char change = false;

						SimpleVector<int> newContained;
						SimpleVector<timeSec_t> newContainedETA;

						SimpleVector< SimpleVector<int> > newSubContained;
						SimpleVector< SimpleVector<timeSec_t> > newSubContainedETA;

						for( int c=0; c< nextPlayer->numContained; c++ ) {
							int oldID = abs( nextPlayer->containedIDs[c] );
							int newID = oldID;

							timeSec_t newDecay =
									nextPlayer->containedEtaDecays[c];

							SimpleVector<int> subCont =
									nextPlayer->subContainedIDs[c];
							SimpleVector<timeSec_t> subContDecay =
									nextPlayer->subContainedEtaDecays[c];

							if( newDecay != 0 && newDecay < curTime ) {

								change = true;

								TransRecord *t = getPTrans( -1, oldID );

								newDecay = 0;

								if( t != NULL ) {

									newID = t->newTarget;

									if( newID != 0 ) {
										float stretch =
												getObject( nextPlayer->holdingID )->
												slotTimeStretch;

										TransRecord *newDecayT =
												getMetaTrans( -1, newID );

										if( newDecayT != NULL ) {
											newDecay =
													Time::timeSec() +
													newDecayT->autoDecaySeconds /
													stretch;
										}
										else {
											// no further decay
											newDecay = 0;
										}
									}
								}
							}

							SimpleVector<int> cVec;
							SimpleVector<timeSec_t> dVec;

							if( newID != 0 ) {
								int oldSlots = subCont.size();

								int newSlots = getObject( newID )->numSlots;

								if( newID != oldID
								&&
								newSlots < oldSlots ) {

									// shrink sub-contained
									// this involves items getting lost
									// but that's okay for now.
									subCont.shrink( newSlots );
									subContDecay.shrink( newSlots );
								}
							}
							else {
								subCont.deleteAll();
								subContDecay.deleteAll();
							}

							// handle decay for each sub-contained object
							for( int s=0; s<subCont.size(); s++ ) {
								int oldSubID = subCont.getElementDirect( s );
								int newSubID = oldSubID;
								timeSec_t newSubDecay =
										subContDecay.getElementDirect( s );

								if( newSubDecay != 0 && newSubDecay < curTime ) {

									change = true;

									TransRecord *t = getPTrans( -1, oldSubID );

									newSubDecay = 0;

									if( t != NULL ) {

										newSubID = t->newTarget;

										if( newSubID != 0 ) {
											float subStretch =
													getObject( newID )->
													slotTimeStretch;

											TransRecord *newSubDecayT =
													getMetaTrans( -1, newSubID );

											if( newSubDecayT != NULL ) {
												newSubDecay =
														Time::timeSec() +
														newSubDecayT->autoDecaySeconds /
														subStretch;
											}
											else {
												// no further decay
												newSubDecay = 0;
											}
										}
									}
								}

								if( newSubID != 0 ) {
									cVec.push_back( newSubID );
									dVec.push_back( newSubDecay );
								}
							}

							if( newID != 0 ) {
								newSubContained.push_back( cVec );
								newSubContainedETA.push_back( dVec );

								if( cVec.size() > 0 ) {
									newID *= -1;
								}

								newContained.push_back( newID );
								newContainedETA.push_back( newDecay );
							}
						}



						if( change ) {
							playerIndicesToSendUpdatesAbout.push_back( i );

							freePlayerContainedArrays( nextPlayer );

							nextPlayer->numContained = newContained.size();

							if( nextPlayer->numContained == 0 ) {
								nextPlayer->containedIDs = NULL;
								nextPlayer->containedEtaDecays = NULL;
								nextPlayer->subContainedIDs = NULL;
								nextPlayer->subContainedEtaDecays = NULL;
							}
							else {
								nextPlayer->containedIDs =
										newContained.getElementArray();
								nextPlayer->containedEtaDecays =
										newContainedETA.getElementArray();

								nextPlayer->subContainedIDs =
										newSubContained.getElementArray();
								nextPlayer->subContainedEtaDecays =
										newSubContainedETA.getElementArray();
							}
						}
					}


					// check if their clothing has decayed
					// or what's in their clothing
					for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
						ObjectRecord *cObj =
								clothingByIndex( nextPlayer->clothing, c );

						if( cObj != NULL &&
						nextPlayer->clothingEtaDecay[c] != 0 &&
						nextPlayer->clothingEtaDecay[c] <
						curTime ) {

							// what they're wearing has decayed

							int oldID = cObj->id;

							TransRecord *t = getPTrans( -1, oldID );

							if( t != NULL ) {

								int newID = t->newTarget;

								ObjectRecord *newCObj = NULL;
								if( newID != 0 ) {
									newCObj = getObject( newID );

									TransRecord *newDecayT =
											getMetaTrans( -1, newID );

									if( newDecayT != NULL ) {
										nextPlayer->clothingEtaDecay[c] =
												Time::timeSec() +
												newDecayT->autoDecaySeconds;
									}
									else {
										// no further decay
										nextPlayer->clothingEtaDecay[c] = 0;
									}
								}
								else {
									nextPlayer->clothingEtaDecay[c] = 0;
								}

								setClothingByIndex( &( nextPlayer->clothing ),
													c, newCObj );

								int oldSlots =
										nextPlayer->clothingContained[c].size();

								int newSlots = getNumContainerSlots( newID );

								if( newSlots < oldSlots ) {
									// new container can hold less
									// truncate

									// drop extras onto map
									timeSec_t curTime = Time::timeSec();
									float stretch = cObj->slotTimeStretch;

									GridPos dropPos =
											getPlayerPos( nextPlayer );

									// offset to counter-act offsets built into
									// drop code
									dropPos.x += 1;
									dropPos.y += 1;

									for( int s=newSlots; s<oldSlots; s++ ) {

										char found = false;
										GridPos spot;

										if( getMapObject( dropPos.x,
														  dropPos.y ) == 0 ) {
											spot = dropPos;
											found = true;
										}
										else {
											found = findDropSpot(
													nextPlayer,
													dropPos.x, dropPos.y,
													dropPos.x, dropPos.y,
													&spot );
										}


										if( found ) {
											setMapObject(
													spot.x, spot.y,
													nextPlayer->
													clothingContained[c].
													getElementDirect( s ) );

											timeSec_t eta =
													nextPlayer->
													clothingContainedEtaDecays[c].
													getElementDirect( s );

											if( stretch != 1.0 ) {
												timeSec_t offset =
														eta - curTime;

												offset = offset / stretch;
												eta = curTime + offset;
											}

											setEtaDecay( spot.x, spot.y, eta );
										}
									}

									nextPlayer->
									clothingContained[c].
									shrink( newSlots );

									nextPlayer->
									clothingContainedEtaDecays[c].
									shrink( newSlots );
								}

								float oldStretch =
										cObj->slotTimeStretch;
								float newStretch;

								if( newCObj != NULL ) {
									newStretch = newCObj->slotTimeStretch;
								}
								else {
									newStretch = oldStretch;
								}

								if( oldStretch != newStretch ) {
									timeSec_t curTime = Time::timeSec();

									for( int cc=0;
									cc < nextPlayer->
									clothingContainedEtaDecays[c].size();
									cc++ ) {

										if( nextPlayer->
										clothingContainedEtaDecays[c].
										getElementDirect( cc ) != 0 ) {

											timeSec_t offset =
													nextPlayer->
													clothingContainedEtaDecays[c].
													getElementDirect( cc ) -
													curTime;

											offset = offset * oldStretch;
											offset = offset / newStretch;

											*( nextPlayer->
											clothingContainedEtaDecays[c].
											getElement( cc ) ) =
													curTime + offset;
										}
									}
								}

								playerIndicesToSendUpdatesAbout.push_back( i );
							}
							else {
								// no valid decay transition, end it
								nextPlayer->clothingEtaDecay[c] = 0;
							}

						}

						// check for decay of what's contained in clothing
						if( cObj != NULL &&
						nextPlayer->clothingContainedEtaDecays[c].size() > 0 ) {

							char change = false;

							SimpleVector<int> newContained;
							SimpleVector<timeSec_t> newContainedETA;

							for( int cc=0;
							cc <
							nextPlayer->
							clothingContainedEtaDecays[c].size();
							cc++ ) {

								int oldID = nextPlayer->
										clothingContained[c].getElementDirect( cc );
								int newID = oldID;

								timeSec_t decay =
										nextPlayer->clothingContainedEtaDecays[c]
										.getElementDirect( cc );

								timeSec_t newDecay = decay;

								if( decay != 0 && decay < curTime ) {

									change = true;

									TransRecord *t = getPTrans( -1, oldID );

									newDecay = 0;

									if( t != NULL ) {
										newID = t->newTarget;

										if( newID != 0 ) {
											TransRecord *newDecayT =
													getMetaTrans( -1, newID );

											if( newDecayT != NULL ) {
												newDecay =
														Time::timeSec() +
														newDecayT->
														autoDecaySeconds /
														cObj->slotTimeStretch;
											}
											else {
												// no further decay
												newDecay = 0;
											}
										}
									}
								}

								if( newID != 0 ) {
									newContained.push_back( newID );
									newContainedETA.push_back( newDecay );
								}
							}

							if( change ) {
								playerIndicesToSendUpdatesAbout.push_back( i );

								// assignment operator for vectors
								// copies one vector into another
								// replacing old contents
								nextPlayer->clothingContained[c] =
										newContained;
								nextPlayer->clothingContainedEtaDecays[c] =
										newContainedETA;
							}

						}


					}


					// check if they are done moving
					// if so, send an update


					if( nextPlayer->xd != nextPlayer->xs ||
					nextPlayer->yd != nextPlayer->ys ) {


						// don't end new moves here (moves that
						// other players haven't been told about)
						// even if they have come to an end time-wise
						// wait until after we've told everyone about them
						if( ! nextPlayer->newMove &&
						Time::getCurrentTime() - nextPlayer->moveStartTime
						>
						nextPlayer->moveTotalSeconds ) {

							double moveSpeed = computeMoveSpeed( nextPlayer ) *
									getPathSpeedModifier( nextPlayer->pathToDest,
														  nextPlayer->pathLength );


							// done
							nextPlayer->xs = nextPlayer->xd;
							nextPlayer->ys = nextPlayer->yd;

							if( nextPlayer->pathTruncated ) {
								// truncated, but never told them about it
								// force update now
								nextPlayer->posForced = true;
							}
							playerIndicesToSendUpdatesAbout.push_back( i );

							if( nextPlayer->holdingBiomeSickness ) {
								int sicknessObjectID =
										getBiomeSickness(
												nextPlayer->displayID,
												nextPlayer->xs,
												nextPlayer->ys );
								if( sicknessObjectID == -1 ) {
									endBiomeSickness(
											nextPlayer, i,
											&playerIndicesToSendUpdatesAbout );
								}
							}


							// if they went far enough and fast enough
							if( nextPlayer->holdingFlightObject &&
							moveSpeed >= minFlightSpeed &&
							! nextPlayer->pathTruncated &&
							nextPlayer->pathLength >= 2 ) {

								// player takes off ?

								double xDir =
										nextPlayer->pathToDest[
												nextPlayer->pathLength - 1 ].x
												-
												nextPlayer->pathToDest[
														nextPlayer->pathLength - 2 ].x;
								double yDir =
										nextPlayer->pathToDest[
												nextPlayer->pathLength - 1 ].y
												-
												nextPlayer->pathToDest[
														nextPlayer->pathLength - 2 ].y;

								int beyondEndX = nextPlayer->xs + xDir;
								int beyondEndY = nextPlayer->ys + yDir;

								int endFloorID = getMapFloor( nextPlayer->xs,
															  nextPlayer->ys );

								int beyondEndFloorID = getMapFloor( beyondEndX,
																	beyondEndY );

								if( beyondEndFloorID != endFloorID ) {
									// went all the way to the end of the
									// current floor in this direction,
									// take off there

									doublePair takeOffDir = { xDir, yDir };

									int radiusLimit = -1;

									int barrierOn = SettingsManager::getIntSetting(
											"barrierOn", 1 );
									int barrierBlocksPlanes =
											SettingsManager::getIntSetting(
													"barrierBlocksPlanes", 1 );

									if( barrierOn && barrierBlocksPlanes ) {
										int barrierRadius =
												SettingsManager::getIntSetting(
														"barrierRadius", 250 );
										radiusLimit = barrierRadius;
									}

									GridPos destPos = { -1, -1 };

									char foundMap = false;
									if( Time::getCurrentTime() -
									nextPlayer->forceFlightDestSetTime
									< 30 ) {
										// map fresh in memory


										destPos = getClosestLandingPos(
												nextPlayer->forceFlightDest,
												&foundMap );

										// find strip closest to last
										// read map position
										AppLog::infoF(
												"Player %d flight taking off from (%d,%d), "
												"map dest (%d,%d), found=%d, found (%d,%d)",
												nextPlayer->id,
												nextPlayer->xs, nextPlayer->ys,
												nextPlayer->forceFlightDest.x,
												nextPlayer->forceFlightDest.y,
												foundMap,
												destPos.x, destPos.y );
									}
									if( ! foundMap ) {
										// find strip in flight direction

										destPos = getNextFlightLandingPos(
												nextPlayer->xs,
												nextPlayer->ys,
												takeOffDir,
												radiusLimit );

										AppLog::infoF(
												"Player %d non-map flight taking off "
												"from (%d,%d), "
												"flightDir (%f,%f), dest (%d,%d)",
												nextPlayer->id,
												nextPlayer->xs, nextPlayer->ys,
												xDir, yDir,
												destPos.x, destPos.y );
									}




									// send them a brand new map chunk
									// around their new location
									// and re-tell them about all players
									// (relative to their new "birth" location...
									//  see below)
									nextPlayer->firstMessageSent = false;
									nextPlayer->firstMapSent = false;
									nextPlayer->inFlight = true;

									int destID = getMapObject( destPos.x,
															   destPos.y );

									char heldTransHappened = false;

									if( destID > 0 &&
									getObject( destID )->isFlightLanding ) {
										// found a landing place
										TransRecord *tr =
												getPTrans( nextPlayer->holdingID,
														   destID );

										if( tr != NULL ) {
											heldTransHappened = true;

											setMapObject( destPos.x, destPos.y,
														  tr->newTarget );

											transferHeldContainedToMap(
													nextPlayer,
													destPos.x, destPos.y );

											handleHoldingChange(
													nextPlayer,
													tr->newActor );

											// stick player next to landing
											// pad
											destPos.x --;
										}
									}
									if( ! heldTransHappened ) {
										// crash landing
										// force decay of held
										// no matter how much time is left
										// (flight uses fuel)
										TransRecord *decayTrans =
												getPTrans( -1,
														   nextPlayer->holdingID );

										if( decayTrans != NULL ) {
											handleHoldingChange(
													nextPlayer,
													decayTrans->newTarget );
										}
									}

									FlightDest fd = {
											nextPlayer->id,
											destPos };

									newFlightDest.push_back( fd );

									nextPlayer->xd = destPos.x;
									nextPlayer->xs = destPos.x;
									nextPlayer->yd = destPos.y;
									nextPlayer->ys = destPos.y;

									// reset their birth location
									// their landing position becomes their
									// new 0,0 for now

									// birth-relative coordinates enable the client
									// (which is on a GPU with 32-bit floats)
									// to operate at true coordinates well above
									// the 23-bit preciions of 32-bit floats.

									// We keep the coordinates small by assuming
									// that a player can never get too far from
									// their birth location in one lifetime.

									// Flight teleportation violates this
									// assumption.
									nextPlayer->birthPos.x = nextPlayer->xs;
									nextPlayer->birthPos.y = nextPlayer->ys;
									nextPlayer->heldOriginX = nextPlayer->xs;
									nextPlayer->heldOriginY = nextPlayer->ys;

									nextPlayer->actionTarget.x = nextPlayer->xs;
									nextPlayer->actionTarget.y = nextPlayer->ys;
								}
							}
						}
					}

					// check if we need to decrement their food
					double curTime = Time::getCurrentTime();

					if( ! nextPlayer->vogMode &&
					curTime >
					nextPlayer->foodDecrementETASeconds ) {

						// only if femail of fertile age
						char heldByFemale = false;

						if( nextPlayer->heldByOther ) {
							LiveObject *adultO = getAdultHolding( nextPlayer );

							if( adultO != NULL &&
							isFertileAge( adultO ) ) {

								heldByFemale = true;
							}
						}


						LiveObject *decrementedPlayer = NULL;

						if( !heldByFemale ) {

							if( nextPlayer->yummyBonusStore > 0 ) {
								nextPlayer->yummyBonusStore--;
							}
							else {
								nextPlayer->foodStore--;
							}
							decrementedPlayer = nextPlayer;
						}
						// if held by fertile female, food for baby is free for
						// duration of holding

						// only update the time of the fed player
						nextPlayer->foodDecrementETASeconds = curTime +
								computeFoodDecrementTimeSeconds( nextPlayer );

						if( nextPlayer->drunkenness > 0 ) {
							// for every unit of food consumed, consume one
							// half unit of drunkenness
							nextPlayer->drunkenness -= 0.5;
							if( nextPlayer->drunkenness < 0 ) {
								nextPlayer->drunkenness = 0;
							}
						}


						if( decrementedPlayer != NULL &&
						decrementedPlayer->foodStore < 0 ) {
							// player has died

							// break the connection with them

							if( heldByFemale ) {
								setDeathReason( decrementedPlayer,
												"nursing_hunger" );
							}
							else {
								setDeathReason( decrementedPlayer,
												"hunger" );
							}

							decrementedPlayer->error = true;
							decrementedPlayer->errorCauseString = "Player starved";


							GridPos deathPos;

							if( decrementedPlayer->xd ==
							decrementedPlayer->xs &&
							decrementedPlayer->yd ==
							decrementedPlayer->ys ) {
								// deleted player standing still

								deathPos.x = decrementedPlayer->xd;
								deathPos.y = decrementedPlayer->yd;
							}
							else {
								// player moving

								deathPos =
										computePartialMoveSpot( decrementedPlayer );
							}

							if( ! decrementedPlayer->deathLogged &&
							! decrementedPlayer->isTutorial ) {
								logDeath( decrementedPlayer->id,
										  decrementedPlayer->email,
										  decrementedPlayer->isEve,
										  computeAge( decrementedPlayer ),
										  getSecondsPlayed( decrementedPlayer ),
										  ! getFemale( decrementedPlayer ),
										  deathPos.x, deathPos.y,
										  players.size() - 1,
										  false,
										  nextPlayer->murderPerpID,
										  nextPlayer->murderPerpEmail );
							}

							if( shutdownMode &&
							! decrementedPlayer->isTutorial ) {
								handleShutdownDeath( decrementedPlayer,
													 deathPos.x, deathPos.y );
							}

							decrementedPlayer->deathLogged = true;


							// no negative
							decrementedPlayer->foodStore = 0;
						}

						if( decrementedPlayer != NULL ) {
							decrementedPlayer->foodUpdate = true;

							if( computeAge( decrementedPlayer ) >
							defaultActionAge ) {

								double decTime =
										computeFoodDecrementTimeSeconds(
												decrementedPlayer );

								int totalFood =
										decrementedPlayer->yummyBonusStore
										+ decrementedPlayer->foodStore;

								double totalTime = decTime * totalFood;

								if( totalTime < 20 ) {
									// 20 seconds left before death
									// show starving emote
									newEmotPlayerIDs.push_back(
											decrementedPlayer->id );

									newEmotIndices.push_back(
											starvingEmotionIndex );

									newEmotTTLs.push_back( 30 );
									decrementedPlayer->starving = true;
								}
							}
						}
					}

				}


			}



			// check for any that have been individually flagged, but
			// aren't on our list yet (updates caused by external triggers)
			for( int i=0; i<players.size() ; i++ ) {
				LiveObject *nextPlayer = players.getElement( i );

				if( nextPlayer->needsUpdate ) {
					playerIndicesToSendUpdatesAbout.push_back( i );

					nextPlayer->needsUpdate = false;
				}
			}


			// send updates about players who have had a posse-size change
			for( int i=0; i<killStatePosseChangedPlayerIDs.size(); i++ ) {
				int id = killStatePosseChangedPlayerIDs.getElementDirect( i );

				int index = getLiveObjectIndex( id );

				if( index != -1 ) {
					playerIndicesToSendUpdatesAbout.push_back( index );

					// end current move to allow move speed to change instantly
					endAnyMove( getLiveObject( id ) );
				}
			}

			killStatePosseChangedPlayerIDs.deleteAll();

/**********************************************************************************************************************/
			//std::cout << "\nstep 1\n";
			if( playerIndicesToSendUpdatesAbout.size() > 0 )
			{

				SimpleVector<char> updateList;

				//std::cout << "\nupdateList.getElementString";
				for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ )
				{
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

					char *playerString = autoSprintf( "%d, ", nextPlayer->id );
					updateList.appendElementString( playerString );

					delete [] playerString;
				}

				//std::cout << "\nupdateList.getElementString";
				char *updateListString = updateList.getElementString();

				AppLog::infoF( "\nNeed to send updates about these %d players: %s",
							   playerIndicesToSendUpdatesAbout.size(),
							   updateListString );
				delete [] updateListString;
			}

/**********************************************************************************************************************/
			//std::cout << "\nstep 2";
			double currentTimeHeat = Time::getCurrentTime();
			if( currentTimeHeat - lastHeatUpdateTime >= heatUpdateTimeStep ) {
				// a heat step has passed


				// recompute heat map here for next players in line
				int r = 0;
				for( r=lastPlayerIndexHeatRecomputed+1;
				r < lastPlayerIndexHeatRecomputed + 1 +
				numPlayersRecomputeHeatPerStep
				&&
				r < players.size(); r++ ) {

					recomputeHeatMap( players.getElement( r ) );
				}

				lastPlayerIndexHeatRecomputed = r - 1;

				if( r >= players.size() ) {
					// done updating for last player
					// start over
					lastPlayerIndexHeatRecomputed = -1;
				}
				lastHeatUpdateTime = currentTimeHeat;
			}

			//std::cout << "\nstep 3";
			// update personal heat value of any player that is due
			// once every 2 seconds
			currentTime = Time::getCurrentTime();
			for( int i=0; i< players.size(); i++ ) {
				LiveObject *nextPlayer = players.getElement( i );

				if( nextPlayer->error ||
				currentTime - nextPlayer->lastHeatUpdate < heatUpdateSeconds ) {
					continue;
				}

				// in case we cross a biome boundary since last time
				// there will be thermal shock that will take them to
				// other side of target temp.
				//
				// but never make them more comfortable (closer to
				// target) then they were before
				float oldDiffFromTarget =
						targetHeat - nextPlayer->bodyHeat;


				if( nextPlayer->lastBiomeHeat != nextPlayer->biomeHeat ) {


					float lastBiomeDiffFromTarget =
							targetHeat - nextPlayer->lastBiomeHeat;

					float biomeDiffFromTarget = targetHeat - nextPlayer->biomeHeat;

					// for any biome
					// there's a "shock" when you enter it, if it's heat value
					// is on the other side of "perfect" from the temp you were at
					if( lastBiomeDiffFromTarget != 0 &&
					biomeDiffFromTarget != 0 &&
					sign( oldDiffFromTarget ) !=
					sign( biomeDiffFromTarget ) ) {


						// shock them to their mirror temperature on the meter
						// (reflected across target temp)
						nextPlayer->bodyHeat = targetHeat + oldDiffFromTarget;
					}

					// we've handled this shock
					nextPlayer->lastBiomeHeat = nextPlayer->biomeHeat;
				}



				float clothingHeat = computeClothingHeat( nextPlayer );

				float heldHeat = computeHeldHeat( nextPlayer );


				float clothingR = computeClothingR( nextPlayer );

				// clothingR modulates heat lost (or gained) from environment
				float clothingLeak = 1 - clothingR;



				// what our body temp will move toward gradually
				// clothing heat and held heat are conductive
				// if they are present, they move envHeat up or down, before
				// we compute diff with body heat
				// (if they are 0, they have no effect)
				float envHeatTarget = clothingHeat + heldHeat + nextPlayer->envHeat;

				if( envHeatTarget < targetHeat ) {
					// we're in a cold environment

					if( nextPlayer->isIndoors ) {
						float targetDiff = targetHeat - envHeatTarget;
						float indoorAdjustedDiff = targetDiff / 2;
						envHeatTarget = targetHeat - indoorAdjustedDiff;
					}

					// clothing actually reduces how cold it is
					// based on its R-value

					// in other words, it "closes the gap" between our
					// perfect temp and our environmental temp

					// perfect clothing R would cut the environmental cold
					// factor in half

					float targetDiff = targetHeat - envHeatTarget;

					float clothingAdjustedDiff = targetDiff / ( 1 + clothingR );

					// how much did clothing improve our situation?
					float improvement = targetDiff - clothingAdjustedDiff;

					if( nextPlayer->isIndoors ) {
						// if indoors, double the improvement of clothing
						// thus, if it took us half-way to perfect, being
						// indoors will take us all the way to perfect
						// think about this as a reduction in the wind chill
						// factor

						improvement *= 2;
					}
					clothingAdjustedDiff = targetDiff - improvement;


					envHeatTarget = targetHeat - clothingAdjustedDiff;
				}


				// clothing only slows down temp movement AWAY from perfect
				if( abs( targetHeat - envHeatTarget ) <
				abs( targetHeat - nextPlayer->bodyHeat ) ) {
					// env heat is closer to perfect than our current body temp
					// clothing R should not apply in this case
					clothingLeak = 1.0;
				}


				float heatDelta =
						clothingLeak * ( envHeatTarget
						-
						nextPlayer->bodyHeat );

				// slow this down a bit
				heatDelta *= 0.5;

				// feed through curve that is asymtotic at 1
				// (so we never change heat faster than 1 unit per timestep)

				float heatDeltaAbs = fabs( heatDelta );
				float heatDeltaSign = sign( heatDelta );

				float maxDelta = 2;
				// larger values make a sharper "knee"
				float deltaSlope = 0.5;

				// max - max/(slope*x+1)

				float heatDeltaScaled =
						maxDelta - maxDelta / ( deltaSlope * heatDeltaAbs + 1 );

				heatDeltaScaled *= heatDeltaSign;


				nextPlayer->bodyHeat += heatDeltaScaled;

				// cap body heat, so that it doesn't climb way out of range
				// even in extreme situations
				if( nextPlayer->bodyHeat > 2 * targetHeat ) {
					nextPlayer->bodyHeat = 2 * targetHeat;
				}
				else if( nextPlayer->bodyHeat < 0 ) {
					nextPlayer->bodyHeat = 0;
				}


				float totalBodyHeat = nextPlayer->bodyHeat + nextPlayer->fever;

				// 0.25 body heat no longer added in each step above
				// add in a flat constant here to reproduce its effects
				// but only in a cold env (just like the old body heat)
				if( envHeatTarget < targetHeat ) {
					totalBodyHeat += 0.003;
				}



				// convert into 0..1 range, where 0.5 represents targetHeat
				nextPlayer->heat = ( totalBodyHeat / targetHeat ) / 2;
				if( nextPlayer->heat > 1 ) {
					nextPlayer->heat = 1;
				}
				if( nextPlayer->heat < 0 ) {
					nextPlayer->heat = 0;
				}

				nextPlayer->heatUpdate = true;
				nextPlayer->lastHeatUpdate = currentTime;
			}

			//std::cout << "\nstep 4";
			for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ )
			{
				LiveObject *nextPlayer = players.getElement(
						playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

				if( nextPlayer->updateSent ) {
					continue;
				}


				if( nextPlayer->vogMode ) {
					// VOG players
					// handle this here, to take them out of circulation
					nextPlayer->updateSent = true;
					continue;
				}


				// also force-recompute heat maps for players that are getting
				// updated
				// don't bother with this for now
				// all players update on the same cycle
				// recomputeHeatMap( nextPlayer );



				newUpdates.push_back( getUpdateRecord( nextPlayer, false ) );

				newUpdatePlayerIDs.push_back( nextPlayer->id );


				if( nextPlayer->posForced &&
				nextPlayer->connected &&
				SettingsManager::getIntSetting( "requireClientForceAck", 1 ) ) {
					// block additional moves/actions from this player until
					// we get a FORCE response, syncing them up with
					// their forced position.

					// don't do this for disconnected players
					nextPlayer->waitingForForceResponse = true;
				}
				nextPlayer->posForced = false;


				ChangePosition p = { nextPlayer->xs, nextPlayer->ys,
									 nextPlayer->updateGlobal };
				newUpdatesPos.push_back( p );


				nextPlayer->updateSent = true;
				nextPlayer->updateGlobal = false;
			}

			if( newUpdates.size() > 0 ) {

				SimpleVector<char> trueUpdateList;


				for( int i=0; i<newUpdates.size(); i++ ) {
					char *s = autoSprintf(
							"%d, ", newUpdatePlayerIDs.getElementDirect( i ) );
					trueUpdateList.appendElementString( s );
					delete [] s;
				}

				char *updateListString = trueUpdateList.getElementString();

				AppLog::infoF( "Sending updates about these %d players: %s",
							   newUpdatePlayerIDs.size(),
							   updateListString );
				delete [] updateListString;
			}

			SimpleVector<ChangePosition> movesPos;

			SimpleVector<MoveRecord> moveList = getMoveRecords( true, &movesPos );


			// add changes from auto-decays on map,
			// mixed with player-caused changes
			stepMap( &mapChanges, &mapChangesPos );




			if( periodicStepThisStep ) {

				// figure out who has recieved a new curse token
				// they are sent a message about it below (CX)
				SimpleVector<char*> newCurseTokenEmails;
				getNewCurseTokenHolders( &newCurseTokenEmails );

				for( int i=0; i<newCurseTokenEmails.size(); i++ ) {
					char *email = newCurseTokenEmails.getElementDirect( i );

					for( int j=0; j<numLive; j++ ) {
						LiveObject *nextPlayer = players.getElement(j);

						// don't give mid-life tokens to twins or cursed players
						if( ! nextPlayer->isTwin &&
						nextPlayer->curseStatus.curseLevel == 0 &&
						strcmp( nextPlayer->email, email ) == 0 ) {

							nextPlayer->curseTokenCount = 1;
							nextPlayer->curseTokenUpdate = true;
							break;
						}
					}

					delete [] email;
				}
			}





			unsigned char *lineageMessage = NULL;
			int lineageMessageLength = 0;

			if( playerIndicesToSendLineageAbout.size() > 0 ) {
				SimpleVector<char> linWorking;
				linWorking.appendElementString( "LN\n" );

				int numAdded = 0;
				for( int i=0; i<playerIndicesToSendLineageAbout.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendLineageAbout.getElementDirect( i ) );

					if( nextPlayer->error ) {
						continue;
					}
					getLineageLineForPlayer( nextPlayer, &linWorking );
					numAdded++;
				}

				linWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *lineageMessageText = linWorking.getElementString();

					lineageMessageLength = strlen( lineageMessageText );

					if( lineageMessageLength < maxUncompressedSize ) {
						lineageMessage = (unsigned char*)lineageMessageText;
					}
					else {
						// compress for all players once here
						lineageMessage = makeCompressedMessage(
								lineageMessageText,
								lineageMessageLength, &lineageMessageLength );

						delete [] lineageMessageText;
					}
				}
			}




			unsigned char *cursesMessage = NULL;
			int cursesMessageLength = 0;

			if( playerIndicesToSendCursesAbout.size() > 0 ) {
				SimpleVector<char> curseWorking;
				curseWorking.appendElementString( "CU\n" );

				int numAdded = 0;
				for( int i=0; i<playerIndicesToSendCursesAbout.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendCursesAbout.getElementDirect( i ) );

					if( nextPlayer->error ) {
						continue;
					}

					// leave name out of it for now
					// this bit of code left over from before personal curses
					// we don't have the sender's email here, b/c this
					// message goes to everyone.
					char *line = autoSprintf( "%d %d\n", nextPlayer->id,
											  nextPlayer->curseStatus.curseLevel );

					curseWorking.appendElementString( line );
					delete [] line;
					numAdded++;
				}

				curseWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *cursesMessageText = curseWorking.getElementString();

					cursesMessageLength = strlen( cursesMessageText );

					if( cursesMessageLength < maxUncompressedSize ) {
						cursesMessage = (unsigned char*)cursesMessageText;
					}
					else {
						// compress for all players once here
						cursesMessage = makeCompressedMessage(
								cursesMessageText,
								cursesMessageLength, &cursesMessageLength );

						delete [] cursesMessageText;
					}
				}
			}



			int followingMessageLength = 0;
			unsigned char *followingMessage =
					getFollowingMessage( false, &followingMessageLength );


			int exileMessageLength = 0;
			unsigned char *exileMessage =
					getExileMessage( false, &exileMessageLength );



			unsigned char *namesMessage = NULL;
			int namesMessageLength = 0;

			if( playerIndicesToSendNamesAbout.size() > 0 ) {
				SimpleVector<char> namesWorking;
				namesWorking.appendElementString( "NM\n" );

				int numAdded = 0;
				for( int i=0; i<playerIndicesToSendNamesAbout.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendNamesAbout.getElementDirect( i ) );

					if( nextPlayer->error ) {
						continue;
					}

					char *line = autoSprintf( "%d %s\n", nextPlayer->id,
											  nextPlayer->name );
					numAdded++;
					namesWorking.appendElementString( line );
					delete [] line;
				}

				namesWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *namesMessageText = namesWorking.getElementString();

					namesMessageLength = strlen( namesMessageText );

					if( namesMessageLength < maxUncompressedSize ) {
						namesMessage = (unsigned char*)namesMessageText;
					}
					else {
						// compress for all players once here
						namesMessage = makeCompressedMessage(
								namesMessageText,
								namesMessageLength, &namesMessageLength );

						delete [] namesMessageText;
					}
				}
			}



			unsigned char *dyingMessage = NULL;
			int dyingMessageLength = 0;

			if( playerIndicesToSendDyingAbout.size() > 0 ) {
				SimpleVector<char> dyingWorking;
				dyingWorking.appendElementString( "DY\n" );

				int numAdded = 0;
				for( int i=0; i<playerIndicesToSendDyingAbout.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendDyingAbout.getElementDirect( i ) );

					if( nextPlayer->error ) {
						continue;
					}

					char *line;

					if( nextPlayer->holdingID > 0 &&
					strstr(
							getObject( nextPlayer->holdingID )->description,
							"sick" ) != NULL ) {
						// flag as sick
						line = autoSprintf( "%d 1\n", nextPlayer->id );
					}
					else {
						line = autoSprintf( "%d\n", nextPlayer->id );
					}

					numAdded++;
					dyingWorking.appendElementString( line );
					delete [] line;
				}

				dyingWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *dyingMessageText = dyingWorking.getElementString();

					dyingMessageLength = strlen( dyingMessageText );

					if( dyingMessageLength < maxUncompressedSize ) {
						dyingMessage = (unsigned char*)dyingMessageText;
					}
					else {
						// compress for all players once here
						dyingMessage = makeCompressedMessage(
								dyingMessageText,
								dyingMessageLength, &dyingMessageLength );

						delete [] dyingMessageText;
					}
				}
			}




			unsigned char *healingMessage = NULL;
			int healingMessageLength = 0;

			if( playerIndicesToSendHealingAbout.size() > 0 ) {
				SimpleVector<char> healingWorking;
				healingWorking.appendElementString( "HE\n" );

				int numAdded = 0;
				for( int i=0; i<playerIndicesToSendHealingAbout.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement(
							playerIndicesToSendHealingAbout.getElementDirect( i ) );

					if( nextPlayer->error ) {
						continue;
					}

					char *line = autoSprintf( "%d\n", nextPlayer->id );

					numAdded++;
					healingWorking.appendElementString( line );
					delete [] line;
				}

				healingWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *healingMessageText = healingWorking.getElementString();

					healingMessageLength = strlen( healingMessageText );

					if( healingMessageLength < maxUncompressedSize ) {
						healingMessage = (unsigned char*)healingMessageText;
					}
					else {
						// compress for all players once here
						healingMessage = makeCompressedMessage(
								healingMessageText,
								healingMessageLength, &healingMessageLength );

						delete [] healingMessageText;
					}
				}
			}




			unsigned char *emotMessage = NULL;
			int emotMessageLength = 0;

			if( newEmotPlayerIDs.size() > 0 ) {
				SimpleVector<char> emotWorking;
				emotWorking.appendElementString( "PE\n" );

				int numAdded = 0;
				for( int i=0; i<newEmotPlayerIDs.size(); i++ ) {

					int ttl = newEmotTTLs.getElementDirect( i );
					int pID = newEmotPlayerIDs.getElementDirect( i );
					int eInd = newEmotIndices.getElementDirect( i );

					char *line;

					if( ttl == 0  ) {
						line = autoSprintf(
								"%d %d\n", pID, eInd );
					}
					else {
						line = autoSprintf(
								"%d %d %d\n", pID, eInd, ttl );

						if( ttl == -1 ) {
							// a new permanent emot
							LiveObject *pO = getLiveObject( pID );
							if( pO != NULL ) {
								pO->permanentEmots.push_back( eInd );
							}
						}

					}

					numAdded++;
					emotWorking.appendElementString( line );
					delete [] line;
				}

				emotWorking.push_back( '#' );

				if( numAdded > 0 ) {

					char *emotMessageText = emotWorking.getElementString();

					emotMessageLength = strlen( emotMessageText );

					if( emotMessageLength < maxUncompressedSize ) {
						emotMessage = (unsigned char*)emotMessageText;
					}
					else {
						// compress for all players once here
						emotMessage = makeCompressedMessage(
								emotMessageText,
								emotMessageLength, &emotMessageLength );

						delete [] emotMessageText;
					}
				}
			}


			SimpleVector<char*> newOwnerStrings;
			for( int u=0; u<newOwnerPos.size(); u++ ) {
				newOwnerStrings.push_back(
						getOwnershipString( newOwnerPos.getElementDirect( u ) ) );
			}


			SimpleVector<char> babyWiggleLines;
			for( int i=0; i<players.size(); i++ ) {

				LiveObject *nextPlayer = players.getElement(i);

				if( nextPlayer->error ) {
					continue;
				}
				if( nextPlayer->wiggleUpdate ) {

					char *idString = autoSprintf( "%d\n", nextPlayer->id );
					babyWiggleLines.appendElementString( idString );
					delete [] idString;

					nextPlayer->wiggleUpdate = false;
				}
			}


			char *wiggleMessage = NULL;
			int wiggleMessageLength = 0;

			if( babyWiggleLines.size() > 0 ) {
				char *lines = babyWiggleLines.getElementString();

				wiggleMessage = autoSprintf( "BW\n%s#", lines );
				wiggleMessageLength = strlen( wiggleMessage );

				delete [] lines;
			}



			SimpleVector<HomelandInfo> homelandList = getHomelandChanges();

			if( homelandList.size() > 0 ) {
				for( int i=0; i<homelandList.size(); i++ ) {
					HomelandInfo hi = homelandList.getElementDirect( i );

					for( int p=0; p<players.size(); p++ ) {
						LiveObject *nextPlayer = players.getElement( p );

						GridPos pos = getPlayerPos( nextPlayer );

						if( distance( pos, hi.center ) < hi.radius ) {
							sendHomelandMessage( nextPlayer,
												 hi.lineageEveID, hi.center );
						}
					}
				}
			}



			// send moves and updates to clients


			SimpleVector<int> playersReceivingPlayerUpdate;


			for( int i=0; i<numLive; i++ ) {

				LiveObject *nextPlayer = players.getElement(i);


				// everyone gets all flight messages
				// even if they haven't gotten first message yet
				// (because the flier will get their first message again
				// when they land, and we need to tell them about flight first)
				if( nextPlayer->firstMapSent ||
				nextPlayer->inFlight ) {

					if( newFlightDest.size() > 0 ) {

						// compose FD messages for this player

						for( int u=0; u<newFlightDest.size(); u++ ) {
							FlightDest *f = newFlightDest.getElement( u );

							char *flightMessage =
									autoSprintf( "FD\n%d %d %d\n#",
												 f->playerID,
												 f->destPos.x -
												 nextPlayer->birthPos.x,
												 f->destPos.y -
												 nextPlayer->birthPos.y );

							sendMessageToPlayer( nextPlayer, flightMessage,
												 strlen( flightMessage ) );
							delete [] flightMessage;
						}
					}
				}



				double maxDist = getMaxChunkDimension();
				double maxDist2 = maxDist * 2;


				if( ! nextPlayer->firstMessageSent ) {

					// send them their learned tool set
					// in case they are reconnecting and already know some tools
					sendLearnedToolMessage( nextPlayer,
											&( nextPlayer->learnedTools ) );


					// first, send the map chunk around them

					int numSent = sendMapChunkMessage( nextPlayer );

					if( numSent == -2 ) {
						// still not sent, try again later
						continue;
					}


					// next send info about valley lines

					int valleySpacing =
							SettingsManager::getIntSetting( "valleySpacing", 40 );

					char *valleyMessage =
							autoSprintf( "VS\n"
										 "%d %d\n#",
										 valleySpacing,
										 nextPlayer->birthPos.y % valleySpacing );

					sendMessageToPlayer( nextPlayer,
										 valleyMessage, strlen( valleyMessage ) );

					delete [] valleyMessage;



					SimpleVector<int> outOfRangePlayerIDs;


					// now send starting message
					SimpleVector<char> messageBuffer;

					messageBuffer.appendElementString( "PU\n" );

					int numPlayers = players.size();

					// must be last in message
					char *playersLine = NULL;

					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( ( o != nextPlayer && o->error )
						||
						o->vogMode ) {
							continue;
						}

						char oWasForced = o->posForced;

						if( nextPlayer->inFlight ||
						nextPlayer->vogMode || nextPlayer->postVogMode ) {
							// not a true first message

							// force all positions for all players
							o->posForced = true;
						}


						// true mid-move positions for first message
						// all relative to new player's birth pos
						char *messageLine = getUpdateLine( o,
														   nextPlayer->birthPos,
														   getPlayerPos(
																nextPlayer ),
																false, true );

						if( nextPlayer->inFlight ||
						nextPlayer->vogMode || nextPlayer->postVogMode ) {
							// restore
							o->posForced = oWasForced;
						}


						// skip sending info about errored players in
						// first message
						if( o->id != nextPlayer->id ) {
							messageBuffer.appendElementString( messageLine );
							delete [] messageLine;

							double d = intDist( o->xd, o->yd,
												nextPlayer->xd,
												nextPlayer->yd );

							if( d > maxDist ) {
								outOfRangePlayerIDs.push_back( o->id );
							}
						}
						else {
							// save until end
							playersLine = messageLine;
						}
					}

					if( playersLine != NULL ) {
						messageBuffer.appendElementString( playersLine );
						delete [] playersLine;
					}

					messageBuffer.push_back( '#' );

					char *message = messageBuffer.getElementString();


					sendMessageToPlayer( nextPlayer, message, strlen( message ) );

					delete [] message;


					// send out-of-range message for all players in PU above
					// that were out of range
					if( outOfRangePlayerIDs.size() > 0 ) {
						SimpleVector<char> messageChars;

						messageChars.appendElementString( "PO\n" );

						for( int i=0; i<outOfRangePlayerIDs.size(); i++ ) {
							char buffer[20];
							sprintf( buffer, "%d\n",
									 outOfRangePlayerIDs.getElementDirect( i ) );

							messageChars.appendElementString( buffer );
						}
						messageChars.push_back( '#' );

						char *outOfRangeMessageText =
								messageChars.getElementString();

						sendMessageToPlayer( nextPlayer, outOfRangeMessageText,
											 strlen( outOfRangeMessageText ) );

						delete [] outOfRangeMessageText;
					}



					char *movesMessage =
							getMovesMessage( false,
											 nextPlayer->birthPos,
											 getPlayerPos( nextPlayer ) );

					if( movesMessage != NULL ) {


						sendMessageToPlayer( nextPlayer, movesMessage,
											 strlen( movesMessage ) );

						delete [] movesMessage;
					}



					// send homeland status for where player is standing
					GridPos playerPos = getPlayerPos( nextPlayer );
					GridPos homeCenter;
					int homeLineageEveID;

					char isSomeHomeland =
							getHomelandCenter( playerPos.x, playerPos.y,
											   &homeCenter, &homeLineageEveID );
					if( isSomeHomeland ) {
						// send them HL message
						sendHomelandMessage(
								nextPlayer,
								homeLineageEveID,
								homeCenter );
					}



					// send lineage for everyone alive


					SimpleVector<char> linWorking;
					linWorking.appendElementString( "LN\n" );

					int numAdded = 0;

					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( o->error ) {
							continue;
						}

						getLineageLineForPlayer( o, &linWorking );
						numAdded++;
					}

					linWorking.push_back( '#' );

					if( numAdded > 0 ) {
						char *linMessage = linWorking.getElementString();


						sendMessageToPlayer( nextPlayer, linMessage,
											 strlen( linMessage ) );

						delete [] linMessage;
					}



					// send names for everyone alive

					SimpleVector<char> namesWorking;
					namesWorking.appendElementString( "NM\n" );

					numAdded = 0;

					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( o->error || o->name == NULL) {
							continue;
						}

						char *line = autoSprintf( "%d %s\n", o->id, o->name );
						namesWorking.appendElementString( line );
						delete [] line;

						numAdded++;
					}

					namesWorking.push_back( '#' );

					if( numAdded > 0 ) {
						char *namesMessage = namesWorking.getElementString();


						sendMessageToPlayer( nextPlayer, namesMessage,
											 strlen( namesMessage ) );

						delete [] namesMessage;
					}



					// send cursed status for all living cursed

					SimpleVector<char> cursesWorking;
					cursesWorking.appendElementString( "CU\n" );

					numAdded = 0;

					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( o->error ) {
							continue;
						}

						int level = o->curseStatus.curseLevel;

						if( level == 0 ) {

							if( usePersonalCurses ) {
								if( isCursed( nextPlayer->email,
											  o->email ) ) {
									level = 1;
								}
							}
						}

						if( level == 0 ) {
							continue;
						}


						char *line = autoSprintf( "%d %d %s_%s\n", o->id, level,
												  getCurseWord( nextPlayer->email,
																o->email, 0 ),
																getCurseWord( nextPlayer->email,
																			  o->email, 1 ) );
						cursesWorking.appendElementString( line );
						delete [] line;

						numAdded++;
					}

					cursesWorking.push_back( '#' );

					if( numAdded > 0 ) {
						char *cursesMessage = cursesWorking.getElementString();


						sendMessageToPlayer( nextPlayer, cursesMessage,
											 strlen( cursesMessage ) );

						delete [] cursesMessage;
					}


					if( nextPlayer->curseStatus.curseLevel > 0 ) {
						// send player their personal report about how
						// many excess curse points they have

						char *message = autoSprintf(
								"CS\n%d#",
								nextPlayer->curseStatus.excessPoints );

						sendMessageToPlayer( nextPlayer, message,
											 strlen( message ) );

						delete [] message;
					}


					// send following status for everyone alive
					int followL = 0;
					unsigned char *followM = getFollowingMessage( true, &followL );

					if( followM != NULL && nextPlayer->connected ) {
						nextPlayer->sock->send(
								followM,
								followL,
								false, false );
						delete [] followM;
					}



					// send exile status for everyone alive
					int exileL = 0;
					unsigned char *exileM = getExileMessage( true, &exileL );

					if( exileM != NULL && nextPlayer->connected ) {
						nextPlayer->sock->send(
								exileM,
								exileL,
								false, false );
						delete [] exileM;
					}





					// send dying for everyone who is dying

					SimpleVector<char> dyingWorking;
					dyingWorking.appendElementString( "DY\n" );

					numAdded = 0;

					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( o->error || ! o->dying ) {
							continue;
						}

						char *line = autoSprintf( "%d\n", o->id );
						dyingWorking.appendElementString( line );
						delete [] line;

						numAdded++;
					}

					dyingWorking.push_back( '#' );

					if( numAdded > 0 ) {
						char *dyingMessage = dyingWorking.getElementString();


						sendMessageToPlayer( nextPlayer, dyingMessage,
											 strlen( dyingMessage ) );

						delete [] dyingMessage;
					}


					// catch them up on war/peace states
					sendWarReportToOne( nextPlayer );

					if( ! nextPlayer->isTutorial &&
					! nextPlayer->forceSpawn ) {
						// not skipping vog mode here, b/c it's never
						// enabled until after first message sent

						// tell them about their own bad biomes
						char *bbMessage =
								getBadBiomeMessage( nextPlayer->displayID );
						sendMessageToPlayer( nextPlayer, bbMessage,
											 strlen( bbMessage ) );

						delete [] bbMessage;
					}


					// tell them about all permanent emots
					SimpleVector<char> emotMessageWorking;
					emotMessageWorking.appendElementString( "PE\n" );
					for( int i=0; i<numPlayers; i++ ) {

						LiveObject *o = players.getElement( i );

						if( o->error ) {
							continue;
						}
						for( int e=0; e< o->permanentEmots.size(); e ++ ) {
							// ttl -2 for permanent but not new
							char *line = autoSprintf(
									"%d %d -2\n",
									o->id,
									o->permanentEmots.getElementDirect( e ) );
							emotMessageWorking.appendElementString( line );
							delete [] line;
						}
					}
					emotMessageWorking.push_back( '#' );

					char *emotMessage = emotMessageWorking.getElementString();

					sendMessageToPlayer( nextPlayer, emotMessage,
										 strlen( emotMessage ) );

					delete [] emotMessage;



					nextPlayer->firstMessageSent = true;
					nextPlayer->inFlight = false;
					nextPlayer->postVogMode = false;
				}
				else {
					// this player has first message, ready for updates/moves


					if( nextPlayer->monumentPosSet &&
					! nextPlayer->monumentPosSent &&
					computeAge( nextPlayer ) > 0.5 ) {

						// they learned about a monument from their mother

						// wait until they are half a year old to tell them
						// so they have a chance to load the sound first

						char *monMessage =
								autoSprintf( "MN\n%d %d %d\n#",
											 nextPlayer->lastMonumentPos.x -
											 nextPlayer->birthPos.x,
											 nextPlayer->lastMonumentPos.y -
											 nextPlayer->birthPos.y,
											 nextPlayer->lastMonumentID );

						sendMessageToPlayer( nextPlayer, monMessage,
											 strlen( monMessage ) );

						nextPlayer->monumentPosSent = true;

						delete [] monMessage;
					}




					// everyone gets all grave messages
					if( newGraves.size() > 0 ) {

						// compose GV messages for this player

						for( int u=0; u<newGraves.size(); u++ ) {
							GraveInfo *g = newGraves.getElement( u );

							// only graves that are either in-range
							// OR that are part of our family line.
							// This prevents leaking relative positions
							// through grave locations, but still allows
							// us to return home after a long journey
							// and find the grave of a family member
							// who died while we were away.
							if( distance( g->pos, getPlayerPos( nextPlayer ) )
							< maxDist2
							||
							g->lineageEveID == nextPlayer->lineageEveID ) {

								char *graveMessage =
										autoSprintf( "GV\n%d %d %d\n#",
													 g->pos.x -
													 nextPlayer->birthPos.x,
													 g->pos.y -
													 nextPlayer->birthPos.y,
													 g->playerID );

								sendMessageToPlayer( nextPlayer, graveMessage,
													 strlen( graveMessage ) );
								delete [] graveMessage;
							}
						}
					}


					// everyone gets all grave move messages
					if( newGraveMoves.size() > 0 ) {

						// compose GM messages for this player

						for( int u=0; u<newGraveMoves.size(); u++ ) {
							GraveMoveInfo *g = newGraveMoves.getElement( u );

							// lineage info lost once grave moves
							// and we still don't want long-distance relative
							// position leaking happening here.
							// So, far-away grave moves simply won't be
							// transmitted.  This may result in some confusion
							// between different clients that have different
							// info about graves, but that's okay.

							// Anyway, if you're far from home, and your relative
							// dies, you'll hear about the original grave.
							// But then if someone moves the bones before you
							// get home, you won't be able to find the grave
							// by name after that.

							GridPos playerPos = getPlayerPos( nextPlayer );

							if( distance( g->posStart, playerPos )
							< maxDist2
							||
							distance( g->posEnd, playerPos )
							< maxDist2 ) {

								char *graveMessage =
										autoSprintf( "GM\n%d %d %d %d %d\n#",
													 g->posStart.x -
													 nextPlayer->birthPos.x,
													 g->posStart.y -
													 nextPlayer->birthPos.y,
													 g->posEnd.x -
													 nextPlayer->birthPos.x,
													 g->posEnd.y -
													 nextPlayer->birthPos.y,
													 g->swapDest );

								sendMessageToPlayer( nextPlayer, graveMessage,
													 strlen( graveMessage ) );
								delete [] graveMessage;
							}
						}
					}


					// everyone gets all owner change messages
					if( newOwnerPos.size() > 0 ) {

						GridPos nextPlayerPos = getPlayerPos( nextPlayer );

						// compose OW messages for this player
						for( int u=0; u<newOwnerPos.size(); u++ ) {
							GridPos p = newOwnerPos.getElementDirect( u );

							// only pos that are either in-range
							// OR are already known to this player.
							// This prevents leaking relative positions
							// through owned locations, but still allows
							// us to instantly learn about important ownership
							// changes
							char known = isKnownOwned( nextPlayer, p );

							if( known ||
							distance( p, nextPlayerPos )
							< maxDist2
							||
							isOwned( nextPlayer, p ) ) {

								if( ! known ) {
									// remember that we know about it now
									nextPlayer->knownOwnedPositions.push_back( p );
								}

								char *ownerMessage =
										autoSprintf(
												"OW\n%d %d%s\n#",
												p.x -
												nextPlayer->birthPos.x,
												p.y -
												nextPlayer->birthPos.y,
												newOwnerStrings.getElementDirect( u ) );

								sendMessageToPlayer( nextPlayer, ownerMessage,
													 strlen( ownerMessage ) );
								delete [] ownerMessage;
							}
						}
					}



					if( newFlipPlayerIDs.size() > 0 ) {

						GridPos nextPlayerPos = getPlayerPos( nextPlayer );

						// compose FL messages for this player
						// only for in-range players that flipped
						SimpleVector<char> messageWorking;

						char firstLine = true;

						for( int u=0; u<newFlipPlayerIDs.size(); u++ ) {
							GridPos p = newFlipPositions.getElementDirect( u );

							if( distance( p, nextPlayerPos ) < maxDist2 ) {

								if( firstLine ) {
									messageWorking.appendElementString( "FL\n" );
									firstLine = false;
								}

								char *line =
										autoSprintf(
												"%d %d\n",
												newFlipPlayerIDs.getElementDirect( u ),
												newFlipFacingLeft.getElementDirect( u ) );

								messageWorking.appendElementString( line );

								delete [] line;
							}
						}
						if( messageWorking.size() > 0 ) {
							messageWorking.push_back( '#' );

							char *message = messageWorking.getElementString();

							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );
							delete [] message;
						}
					}



					int playerXD = nextPlayer->xd;
					int playerYD = nextPlayer->yd;

					if( nextPlayer->heldByOther ) {
						LiveObject *holdingPlayer =
								getLiveObject( nextPlayer->heldByOtherID );

						if( holdingPlayer != NULL ) {
							playerXD = holdingPlayer->xd;
							playerYD = holdingPlayer->yd;
						}
					}


					if( abs( playerXD - nextPlayer->lastSentMapX ) > 7
					||
					abs( playerYD - nextPlayer->lastSentMapY ) > 8
					||
					! nextPlayer->firstMapSent ) {

						// moving out of bounds of chunk, send update
						// or player flagged as needing first map again

						sendMapChunkMessage( nextPlayer,
											 // override if held
											 nextPlayer->heldByOther,
											 playerXD,
											 playerYD );


						// send updates about any non-moving players
						// that are in this chunk
						SimpleVector<char> chunkPlayerUpdates;

						SimpleVector<char> chunkPlayerMoves;


						// add chunk updates for held babies first
						for( int j=0; j<numLive; j++ ) {
							LiveObject *otherPlayer = players.getElement( j );

							if( otherPlayer->error ) {
								continue;
							}


							if( otherPlayer->heldByOther ) {
								LiveObject *adultO =
										getAdultHolding( otherPlayer );

								if( adultO != NULL ) {


									if( adultO->id != nextPlayer->id &&
									otherPlayer->id != nextPlayer->id ) {
										// parent not us
										// baby not us

										double d = intDist( playerXD,
															playerYD,
															adultO->xd,
															adultO->yd );


										if( d <= getMaxChunkDimension() / 2 ) {
											// adult holding this baby
											// is close enough
											// send update about baby
											char *updateLine =
													getUpdateLine( otherPlayer,
																   nextPlayer->birthPos,
																   getPlayerPos(
																		nextPlayer ),
																		false );

											chunkPlayerUpdates.
											appendElementString( updateLine );
											delete [] updateLine;
										}
									}
								}
							}
						}


						int ourHolderID = -1;

						if( nextPlayer->heldByOther ) {
							LiveObject *adult = getAdultHolding( nextPlayer );

							if( adult != NULL ) {
								ourHolderID = adult->id;
							}
						}

						// now send updates about all non-held babies,
						// including any adults holding on-chunk babies
						// here, AFTER we update about the babies

						// (so their held status overrides the baby's stale
						//  position status).
						for( int j=0; j<numLive; j++ ) {
							LiveObject *otherPlayer =
									players.getElement( j );

							if( otherPlayer->error ||
							otherPlayer->vogMode ) {
								continue;
							}


							if( !otherPlayer->heldByOther &&
							otherPlayer->id != nextPlayer->id &&
							otherPlayer->id != ourHolderID ) {
								// not us
								// not a held baby (covered above)
								// no the adult holding us

								double d = intDist( playerXD,
													playerYD,
													otherPlayer->xd,
													otherPlayer->yd );


								if( d <= getMaxChunkDimension() / 2 ) {

									// send next player a player update
									// about this player, telling nextPlayer
									// where this player was last stationary
									// and what they're holding

									char *updateLine =
											getUpdateLine( otherPlayer,
														   nextPlayer->birthPos,
														   getPlayerPos( nextPlayer ),
														   false );

									chunkPlayerUpdates.appendElementString(
											updateLine );
									delete [] updateLine;


									// We don't need to tell player about
									// moves in progress on this chunk.
									// We're receiving move messages from
									// a radius of 32
									// but this chunk has a radius of 16
									// so we're hearing about player moves
									// before they're on our chunk.
									// Player moves have limited length,
									// so there's no chance of a long move
									// that started outside of our 32-radius
									// finishinging inside this new chunk.
								}
							}
						}


						if( chunkPlayerUpdates.size() > 0 ) {
							chunkPlayerUpdates.push_back( '#' );
							char *temp = chunkPlayerUpdates.getElementString();

							char *message = concatonate( "PU\n", temp );
							delete [] temp;

							sendMessageToPlayer( nextPlayer, message,
												 strlen( message ) );

							delete [] message;
						}


						if( chunkPlayerMoves.size() > 0 ) {
							char *temp = chunkPlayerMoves.getElementString();

							sendMessageToPlayer( nextPlayer, temp, strlen( temp ) );

							delete [] temp;
						}

						// done handling sending new map chunk and player updates
						// for players in the new chunk
					}
					else {
						// check if moving path goes near edge of player's
						// known map
						LiveObject *playerToCheck = nextPlayer;
						if( nextPlayer->heldByOther ) {
							LiveObject *holdingPlayer =
									getLiveObject( nextPlayer->heldByOtherID );

							if( holdingPlayer != NULL ) {
								playerToCheck = holdingPlayer;
							}
						}

						if( ( playerToCheck->xd != playerToCheck->xs ||
						playerToCheck->yd != playerToCheck->ys )
						&&
						playerToCheck->pathToDest != NULL
						&&
						( nextPlayer->mapChunkPathCheckedDest.x
						!= playerToCheck->xd ||
						nextPlayer->mapChunkPathCheckedDest.y
						!= playerToCheck->yd ) ) {
							// moving and haven't checked this path before
							// to see if it gets too close to the edge of the
							// map

							// remember it to not check it again
							nextPlayer->mapChunkPathCheckedDest.x =
									playerToCheck->xd;
							nextPlayer->mapChunkPathCheckedDest.y =
									playerToCheck->yd;

							// find most distant points on current path

							GridPos xFarPos, yFarPos;
							int xFarPosDist = 0;
							int yFarPosDist = 0;

							for( int i=0; i < playerToCheck->pathLength; i++ ) {
								GridPos p = playerToCheck->pathToDest[i];

								int xdist =
										abs( p.x - nextPlayer->lastSentMapX );
								int ydist =
										abs( p.y - nextPlayer->lastSentMapY );

								if( xdist > xFarPosDist ) {
									xFarPos = p;
									xFarPosDist = xdist;
								}
								if( ydist > yFarPosDist ) {
									yFarPos = p;
									yFarPosDist = ydist;
								}
							}

							if( xFarPosDist > 0 &&
							abs( xFarPos.x -
							nextPlayer->lastSentMapX ) > 7 ) {

								sendMapChunkMessage( nextPlayer,
													 // override chunk pos
													 true,
													 xFarPos.x,
													 xFarPos.y );
							}
							if( yFarPosDist > 0 &&
							abs( yFarPos.y -
							nextPlayer->lastSentMapY ) > 7 ) {

								sendMapChunkMessage( nextPlayer,
													 // override chunk pos
													 true,
													 yFarPos.x,
													 yFarPos.y );
							}
						}
					}


					// EVERYONE gets info about dying players

					// do this first, so that PU messages about what they
					// are holding post-wound come later
					if( dyingMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										dyingMessage,
										dyingMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != dyingMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// EVERYONE gets info about now-healed players
					if( healingMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										healingMessage,
										healingMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != healingMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// EVERYONE gets info about emots
					if( emotMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										emotMessage,
										emotMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != emotMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// everyone gets wiggle message
					if( wiggleMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										(unsigned char*)wiggleMessage,
										wiggleMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != wiggleMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}



					// greater than maxDis but within maxDist2
					// for either PU or PM messages
					// (send PO for both, because we can have case
					// were a player coninously walks through the middleDistance
					// w/o ever stopping to create a PU message)
					SimpleVector<int> middleDistancePlayerIDs;




					if( newUpdates.size() > 0 && nextPlayer->connected ) {

						double minUpdateDist = maxDist2 * 2;

						for( int u=0; u<newUpdatesPos.size(); u++ ) {
							ChangePosition *p = newUpdatesPos.getElement( u );

							// update messages can be global when a new
							// player joins or an old player is deleted
							if( p->global ) {
								minUpdateDist = 0;
							}
							else {
								double d = intDist( p->x, p->y,
													playerXD,
													playerYD );

								if( d < minUpdateDist ) {
									minUpdateDist = d;
								}
								if( d > maxDist && d <= maxDist2 ) {
									middleDistancePlayerIDs.push_back(
											newUpdatePlayerIDs.getElementDirect( u ) );
								}
							}
						}

						if( minUpdateDist <= maxDist ) {
							// some updates close enough

							// compose PU message for this player

							unsigned char *updateMessage = NULL;
							int updateMessageLength = 0;
							SimpleVector<char> updateChars;

							for( int u=0; u<newUpdates.size(); u++ ) {
								ChangePosition *p = newUpdatesPos.getElement( u );

								double d = intDist( p->x, p->y,
													playerXD, playerYD );

								if( ! p->global && d > maxDist ) {
									// skip this one, too far away
									continue;
								}

								if( p->global &&  d > maxDist ) {
									// out of range global updates should
									// also be followed by PO message
									middleDistancePlayerIDs.push_back(
											newUpdatePlayerIDs.getElementDirect( u ) );
								}


								char *line =
										getUpdateLineFromRecord(
												newUpdates.getElement( u ),
												nextPlayer->birthPos,
												getPlayerPos( nextPlayer ) );

								updateChars.appendElementString( line );
								delete [] line;
							}


							if( updateChars.size() > 0 ) {
								updateChars.push_back( '#' );
								char *temp = updateChars.getElementString();

								char *updateMessageText =
										concatonate( "PU\n", temp );
								delete [] temp;

								updateMessageLength = strlen( updateMessageText );

								if( updateMessageLength < maxUncompressedSize ) {
									updateMessage =
											(unsigned char*)updateMessageText;
								}
								else {
									updateMessage = makeCompressedMessage(
											updateMessageText,
											updateMessageLength, &updateMessageLength );

									delete [] updateMessageText;
								}
							}

							if( updateMessage != NULL ) {
								playersReceivingPlayerUpdate.push_back(
										nextPlayer->id );

								int numSent =
										nextPlayer->sock->send(
												updateMessage,
												updateMessageLength,
												false, false );

								nextPlayer->gotPartOfThisFrame = true;

								delete [] updateMessage;

								if( numSent != updateMessageLength ) {
									setPlayerDisconnected( nextPlayer,
														   "Socket write failed" );
								}
							}
						}
					}




					if( moveList.size() > 0 && nextPlayer->connected ) {

						double minUpdateDist = getMaxChunkDimension() * 2;

						for( int u=0; u<movesPos.size(); u++ ) {
							ChangePosition *p = movesPos.getElement( u );

							// move messages are never global

							double d = intDist( p->x, p->y,
												playerXD, playerYD );

							if( d < minUpdateDist ) {
								minUpdateDist = d;
							}
							if( d > maxDist && d <= maxDist2 ) {
								middleDistancePlayerIDs.push_back(
										moveList.getElement( u )->playerID );
							}
						}

						if( minUpdateDist <= maxDist ) {

							SimpleVector<MoveRecord> closeMoves;

							for( int u=0; u<movesPos.size(); u++ ) {
								ChangePosition *p = movesPos.getElement( u );

								// move messages are never global

								double d = intDist( p->x, p->y,
													playerXD, playerYD );

								if( d > maxDist ) {
									continue;
								}
								closeMoves.push_back(
										moveList.getElementDirect( u ) );
							}

							if( closeMoves.size() > 0 ) {

								char *moveMessageText = getMovesMessageFromList(
										&closeMoves, nextPlayer->birthPos );

								unsigned char *moveMessage = NULL;
								int moveMessageLength = 0;

								if( moveMessageText != NULL ) {
									moveMessage = (unsigned char*)moveMessageText;
									moveMessageLength = strlen( moveMessageText );

									if( moveMessageLength > maxUncompressedSize ) {
										moveMessage = makeCompressedMessage(
												moveMessageText,
												moveMessageLength,
												&moveMessageLength );
										delete [] moveMessageText;
									}
								}

								int numSent =
										nextPlayer->sock->send(
												moveMessage,
												moveMessageLength,
												false, false );

								nextPlayer->gotPartOfThisFrame = true;

								delete [] moveMessage;

								if( numSent != moveMessageLength ) {
									setPlayerDisconnected( nextPlayer,
														   "Socket write failed" );
								}
							}
						}
					}



					// now send PO for players that are out of range
					// who are moving or updating above
					if( middleDistancePlayerIDs.size() > 0
					&& nextPlayer->connected ) {

						unsigned char *outOfRangeMessage = NULL;
						int outOfRangeMessageLength = 0;

						if( middleDistancePlayerIDs.size() > 0 ) {
							SimpleVector<char> messageChars;

							messageChars.appendElementString( "PO\n" );

							for( int i=0;
							i<middleDistancePlayerIDs.size(); i++ ) {
								char buffer[20];
								sprintf(
										buffer, "%d\n",
										middleDistancePlayerIDs.
										getElementDirect( i ) );

								messageChars.appendElementString( buffer );
							}
							messageChars.push_back( '#' );

							char *outOfRangeMessageText =
									messageChars.getElementString();

							outOfRangeMessageLength =
									strlen( outOfRangeMessageText );

							if( outOfRangeMessageLength <
							maxUncompressedSize ) {
								outOfRangeMessage =
										(unsigned char*)outOfRangeMessageText;
							}
							else {
								// compress
								outOfRangeMessage = makeCompressedMessage(
										outOfRangeMessageText,
										outOfRangeMessageLength,
										&outOfRangeMessageLength );

								delete [] outOfRangeMessageText;
							}
						}

						int numSent =
								nextPlayer->sock->send(
										outOfRangeMessage,
										outOfRangeMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						delete [] outOfRangeMessage;

						if( numSent != outOfRangeMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}




					if( mapChanges.size() > 0 && nextPlayer->connected ) {
						double minUpdateDist = getMaxChunkDimension() * 2;

						for( int u=0; u<mapChangesPos.size(); u++ ) {
							ChangePosition *p = mapChangesPos.getElement( u );

							// map changes are never global

							double d = intDist( p->x, p->y,
												playerXD, playerYD );

							if( d < minUpdateDist ) {
								minUpdateDist = d;
							}
						}

						if( minUpdateDist <= maxDist ) {
							// at least one thing in map change list is close
							// enough to this player

							// format custom map change message for this player


							unsigned char *mapChangeMessage = NULL;
							int mapChangeMessageLength = 0;
							SimpleVector<char> mapChangeChars;

							for( int u=0; u<mapChanges.size(); u++ ) {
								ChangePosition *p = mapChangesPos.getElement( u );

								double d = intDist( p->x, p->y,
													playerXD, playerYD );

								if( d > maxDist ) {
									// skip this one, too far away
									continue;
								}
								MapChangeRecord *r =
										mapChanges.getElement( u );

								char *lineString =
										getMapChangeLineString(
												r,
												nextPlayer->birthPos.x,
												nextPlayer->birthPos.y );

								mapChangeChars.appendElementString( lineString );
								delete [] lineString;
							}


							if( mapChangeChars.size() > 0 ) {
								mapChangeChars.push_back( '#' );
								char *temp = mapChangeChars.getElementString();

								char *mapChangeMessageText =
										concatonate( "MX\n", temp );
								delete [] temp;

								mapChangeMessageLength =
										strlen( mapChangeMessageText );

								if( mapChangeMessageLength <
								maxUncompressedSize ) {
									mapChangeMessage =
											(unsigned char*)mapChangeMessageText;
								}
								else {
									mapChangeMessage = makeCompressedMessage(
											mapChangeMessageText,
											mapChangeMessageLength,
											&mapChangeMessageLength );

									delete [] mapChangeMessageText;
								}
							}


							if( mapChangeMessage != NULL ) {

								int numSent =
										nextPlayer->sock->send(
												mapChangeMessage,
												mapChangeMessageLength,
												false, false );

								nextPlayer->gotPartOfThisFrame = true;

								delete [] mapChangeMessage;

								if( numSent != mapChangeMessageLength ) {
									setPlayerDisconnected( nextPlayer,
														   "Socket write failed" );
								}
							}
						}
					}
					if( newSpeechPos.size() > 0 && nextPlayer->connected ) {
						double minUpdateDist = maxSpeechRadius * 2;

						for( int u=0; u<newSpeechPos.size(); u++ ) {
							ChangePosition *p = newSpeechPos.getElement( u );

							// speech never global

							double d = intDist( p->x, p->y,
												playerXD, playerYD );

							if( d < minUpdateDist ) {
								minUpdateDist = d;
							}
						}

						if( minUpdateDist <= maxSpeechRadius ) {

							SimpleVector<char> messageWorking;
							messageWorking.appendElementString( "PS\n" );


							for( int u=0; u<newSpeechPos.size(); u++ ) {

								ChangePosition *p = newSpeechPos.getElement( u );

								// speech never global

								double d = intDist( p->x, p->y,
													playerXD, playerYD );

								if( d <= maxSpeechRadius ) {

									int speakerID =
											newSpeechPlayerIDs.getElementDirect( u );
									LiveObject *speakerObj =
											getLiveObject( speakerID );

									int listenerEveID = nextPlayer->lineageEveID;
									int listenerID = nextPlayer->id;
									double listenerAge = computeAge( nextPlayer );

									int speakerEveID;
									double speakerAge;

									if( speakerObj != NULL ) {
										speakerEveID = speakerObj->lineageEveID;
										speakerID = speakerObj->id;
										speakerAge = computeAge( speakerObj );
									}
									else {
										// speaker dead, doesn't matter what we
										// do
										speakerEveID = listenerEveID;
										speakerID = listenerID;
										speakerAge = listenerAge;
									}


									char *trimmedPhrase =
											stringDuplicate( newSpeechPhrases.
											getElementDirect( u ) );

									char *starLoc =
											strstr( trimmedPhrase, " *map" );

									if( starLoc != NULL ) {
										if( speakerID != listenerID ) {
											// only send map metadata through
											// if we picked up the map ourselves
											// trim it otherwise

											starLoc[0] = '\0';
										}
										else {
											// make coords birth-relative
											// to person reading map
											int mapX, mapY;

											// turn time into relative age in sec
											timeSec_t mapT = 0;

											int numRead =
													sscanf( starLoc,
															" *map %d %d %lf",
															&mapX, &mapY, &mapT );
											if( numRead == 2 || numRead == 3 ) {
												starLoc[0] = '\0';

												timeSec_t age = 0;

												if( numRead == 3 ) {
													age = Time::timeSec() - mapT;
												}

												char *newTrimmed = autoSprintf(
														"%s *map %d %d %.f",
														trimmedPhrase,
														mapX - nextPlayer->birthPos.x,
														mapY - nextPlayer->birthPos.y,
														age );

												delete [] trimmedPhrase;
												trimmedPhrase = newTrimmed;

												if( speakerObj != NULL ) {
													speakerObj->forceFlightDest.x
													= mapX;
													speakerObj->forceFlightDest.y
													= mapY;
													speakerObj->
													forceFlightDestSetTime
													= Time::getCurrentTime();
												}
											}
										}
									}


									// any other * metadata before *map?
									char *otherStarLoc = strstr( trimmedPhrase,
																 " *" );
									if( otherStarLoc != NULL ) {
										if( speakerID != listenerID ) {
											// only send * metadata through
											// to speaker
											// trim it otherwise

											otherStarLoc[0] = '\0';
										}
									}



									char *translatedPhrase =
											translatePhraseFromSpeaker(
													trimmedPhrase, speakerObj, nextPlayer );

									if( speakerEveID !=
									listenerEveID
									&& speakerAge > 55
									&& listenerAge > 55 ) {

										if( strcmp( translatedPhrase, "PEACE" )
										== 0 ) {
											// an elder speaker
											// said PEACE
											// in elder listener's language
											addPeaceTreaty( speakerEveID,
															listenerEveID );
										}
										else if( strcmp( translatedPhrase,
														 "WAR" )
														 == 0 ) {
											// an elder speaker
											// said WAR
											// in elder listener's language
											removePeaceTreaty( speakerEveID,
															   listenerEveID );
										}
									}

									if( speakerObj != NULL &&
									speakerObj->drunkenness > 0 ) {
										// slur their speech

										char *slurredPhrase =
												slurSpeech( speakerObj->id,
															translatedPhrase,
															speakerObj->drunkenness );

										delete [] translatedPhrase;
										translatedPhrase = slurredPhrase;
									}


									int curseFlag =
											newSpeechCurseFlags.getElementDirect( u );

									char *line = autoSprintf( "%d/%d %s\n",
															  speakerID,
															  curseFlag,
															  translatedPhrase );
									delete [] translatedPhrase;
									delete [] trimmedPhrase;

									messageWorking.appendElementString( line );

									delete [] line;
								}
							}

							messageWorking.appendElementString( "#" );

							char *messageText =
									messageWorking.getElementString();

							int messageLen = strlen( messageText );

							unsigned char *message =
									(unsigned char*) messageText;


							if( messageLen >= maxUncompressedSize ) {
								char *old = messageText;
								int oldLen = messageLen;

								message = makeCompressedMessage(
										old,
										oldLen, &messageLen );

								delete [] old;
							}


							int numSent =
									nextPlayer->sock->send(
											message,
											messageLen,
											false, false );

							delete [] message;

							nextPlayer->gotPartOfThisFrame = true;

							if( numSent != messageLen ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}
						}
					}


					if( newLocationSpeech.size() > 0 && nextPlayer->connected ) {
						double minUpdateDist = maxSpeechRadius * 2;

						for( int u=0; u<newLocationSpeechPos.size(); u++ ) {
							ChangePosition *p =
									newLocationSpeechPos.getElement( u );

							// locationSpeech never global

							double d = intDist( p->x, p->y,
												playerXD, playerYD );

							if( d < minUpdateDist ) {
								minUpdateDist = d;
							}
						}

						if( minUpdateDist <= maxSpeechRadius ) {
							// some of location speech in range

							SimpleVector<char> working;

							working.appendElementString( "LS\n" );

							for( int u=0; u<newLocationSpeechPos.size(); u++ ) {
								ChangePosition *p =
										newLocationSpeechPos.getElement( u );

								double d = intDist( p->x, p->y,
													playerXD, playerYD );

								if( d <= maxSpeechRadius ) {

									char *line = autoSprintf(
											"%d %d %s\n",
											p->x - nextPlayer->birthPos.x,
											p->y - nextPlayer->birthPos.y,
											newLocationSpeech.getElementDirect( u ) );
									working.appendElementString( line );

									delete [] line;
								}
							}
							working.push_back( '#' );

							char *message =
									working.getElementString();
							int len = working.size();


							if( len > maxUncompressedSize ) {
								int compLen = 0;

								unsigned char *compMessage = makeCompressedMessage(
										message,
										len,
										&compLen );

								delete [] message;
								len = compLen;
								message = (char*)compMessage;
							}

							int numSent =
									nextPlayer->sock->send(
											(unsigned char*)message,
											len,
											false, false );

							delete [] message;

							nextPlayer->gotPartOfThisFrame = true;

							if( numSent != len ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}
						}
					}



					// EVERYONE gets updates about deleted players
					if( nextPlayer->connected ) {

						unsigned char *deleteUpdateMessage = NULL;
						int deleteUpdateMessageLength = 0;

						SimpleVector<char> deleteUpdateChars;

						for( int u=0; u<newDeleteUpdates.size(); u++ ) {

							char *line = getUpdateLineFromRecord(
									newDeleteUpdates.getElement( u ),
									nextPlayer->birthPos,
									getPlayerPos( nextPlayer ) );

							deleteUpdateChars.appendElementString( line );

							delete [] line;
						}


						if( deleteUpdateChars.size() > 0 ) {
							deleteUpdateChars.push_back( '#' );
							char *temp = deleteUpdateChars.getElementString();

							char *deleteUpdateMessageText =
									concatonate( "PU\n", temp );
							delete [] temp;

							deleteUpdateMessageLength =
									strlen( deleteUpdateMessageText );

							if( deleteUpdateMessageLength < maxUncompressedSize ) {
								deleteUpdateMessage =
										(unsigned char*)deleteUpdateMessageText;
							}
							else {
								// compress for all players once here
								deleteUpdateMessage = makeCompressedMessage(
										deleteUpdateMessageText,
										deleteUpdateMessageLength,
										&deleteUpdateMessageLength );

								delete [] deleteUpdateMessageText;
							}
						}

						if( deleteUpdateMessage != NULL ) {
							int numSent =
									nextPlayer->sock->send(
											deleteUpdateMessage,
											deleteUpdateMessageLength,
											false, false );

							nextPlayer->gotPartOfThisFrame = true;

							delete [] deleteUpdateMessage;

							if( numSent != deleteUpdateMessageLength ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}
						}
					}



					// EVERYONE gets lineage info for new babies
					if( lineageMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										lineageMessage,
										lineageMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != lineageMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// EVERYONE gets curse info
					if( cursesMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										cursesMessage,
										cursesMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != cursesMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}

					// EVERYONE gets newly-given names
					if( namesMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										namesMessage,
										namesMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != namesMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// EVERYONE gets following message
					if( followingMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										followingMessage,
										followingMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != followingMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}


					// EVERYONE gets exile message
					if( exileMessage != NULL && nextPlayer->connected ) {
						int numSent =
								nextPlayer->sock->send(
										exileMessage,
										exileMessageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != exileMessageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}
					}




					if( nextPlayer->foodUpdate ) {
						// send this player a food status change

						int cap = computeFoodCapacity( nextPlayer );

						if( cap < nextPlayer->foodStore ) {
							nextPlayer->foodStore = cap;
						}

						if( cap > nextPlayer->lastReportedFoodCapacity ) {

							// stomach grew

							// fill empty space from bonus store automatically
							int extraCap =
									cap - nextPlayer->lastReportedFoodCapacity;

							while( nextPlayer->yummyBonusStore > 0 &&
							extraCap > 0 &&
							nextPlayer->foodStore < cap ) {
								nextPlayer->foodStore ++;
								extraCap --;
								nextPlayer->yummyBonusStore--;
							}
						}


						nextPlayer->lastReportedFoodCapacity = cap;


						int yumMult = nextPlayer->yummyFoodChain.size() - 1;

						if( yumMult < 0 ) {
							yumMult = 0;
						}

						if( yumBonusCap != -1 &&
						yumMult > yumBonusCap ) {
							yumMult = yumBonusCap;
						}

						if( nextPlayer->connected ) {

							char *foodMessage = autoSprintf(
									"FX\n"
									"%d %d %d %d %.2f %d "
									"%d %d\n"
									"#",
									nextPlayer->foodStore,
									cap,
									hideIDForClient( nextPlayer->lastAteID ),
									nextPlayer->lastAteFillMax,
									computeMoveSpeed( nextPlayer ),
									nextPlayer->responsiblePlayerID,
									nextPlayer->yummyBonusStore,
									yumMult );

							int messageLength = strlen( foodMessage );

							int numSent =
									nextPlayer->sock->send(
											(unsigned char*)foodMessage,
											messageLength,
											false, false );

							nextPlayer->gotPartOfThisFrame = true;

							if( numSent != messageLength ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}

							delete [] foodMessage;
						}

						nextPlayer->foodUpdate = false;
						nextPlayer->lastAteID = 0;
						nextPlayer->lastAteFillMax = 0;
					}



					if( nextPlayer->heatUpdate && nextPlayer->connected ) {
						// send this player a heat status change

						// recompute now to update their decrement time
						// and indoor bonus for this message
						computeFoodDecrementTimeSeconds( nextPlayer );

						char *heatMessage = autoSprintf(
								"HX\n"
								"%.2f %.2f %.2f#",
								nextPlayer->heat,
								nextPlayer->foodDrainTime,
								nextPlayer->indoorBonusTime );

						int messageLength = strlen( heatMessage );

						int numSent =
								nextPlayer->sock->send(
										(unsigned char*)heatMessage,
										messageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != messageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}

						delete [] heatMessage;
					}
					nextPlayer->heatUpdate = false;


					if( nextPlayer->curseTokenUpdate &&
					nextPlayer->connected ) {
						// send this player a curse token status change

						char *tokenMessage = autoSprintf(
								"CX\n"
								"%d#",
								nextPlayer->curseTokenCount );

						int messageLength = strlen( tokenMessage );

						int numSent =
								nextPlayer->sock->send(
										(unsigned char*)tokenMessage,
										messageLength,
										false, false );

						nextPlayer->gotPartOfThisFrame = true;

						if( numSent != messageLength ) {
							setPlayerDisconnected( nextPlayer,
												   "Socket write failed" );
						}

						delete [] tokenMessage;
					}
					nextPlayer->curseTokenUpdate = false;

				}
			}


			for( int u=0; u<moveList.size(); u++ ) {
				MoveRecord *r = moveList.getElement( u );
				delete [] r->formatString;
			}



			for( int u=0; u<mapChanges.size(); u++ ) {
				MapChangeRecord *r = mapChanges.getElement( u );
				delete [] r->formatString;
			}

			if( newUpdates.size() > 0 ) {

				SimpleVector<char> playerList;

				for( int i=0; i<playersReceivingPlayerUpdate.size(); i++ ) {
					char *playerString =
							autoSprintf(
									"%d, ",
									playersReceivingPlayerUpdate.getElementDirect( i ) );
					playerList.appendElementString( playerString );
					delete [] playerString;
				}

				char *playerListString = playerList.getElementString();

				AppLog::infoF( "%d/%d players were sent part of a %d-line PU: %s",
							   playersReceivingPlayerUpdate.size(),
							   numLive, newUpdates.size(),
							   playerListString );

				delete [] playerListString;
			}


			for( int u=0; u<newUpdates.size(); u++ ) {
				UpdateRecord *r = newUpdates.getElement( u );
				delete [] r->formatString;
			}

			for( int u=0; u<newDeleteUpdates.size(); u++ ) {
				UpdateRecord *r = newDeleteUpdates.getElement( u );
				delete [] r->formatString;
			}


			if( lineageMessage != NULL ) {
				delete [] lineageMessage;
			}
			if( cursesMessage != NULL ) {
				delete [] cursesMessage;
			}
			if( namesMessage != NULL ) {
				delete [] namesMessage;
			}
			if( followingMessage != NULL ) {
				delete [] followingMessage;
			}
			if( exileMessage != NULL ) {
				delete [] exileMessage;
			}
			if( dyingMessage != NULL ) {
				delete [] dyingMessage;
			}
			if( healingMessage != NULL ) {
				delete [] healingMessage;
			}
			if( emotMessage != NULL ) {
				delete [] emotMessage;
			}
			if( wiggleMessage != NULL ) {
				delete [] wiggleMessage;
			}


			newOwnerStrings.deallocateStringElements();


			// these are global, so we must clear it every loop
			newSpeechPos.deleteAll();
			newSpeechPlayerIDs.deleteAll();
			newSpeechCurseFlags.deleteAll();
			newSpeechPhrases.deallocateStringElements();

			newLocationSpeech.deallocateStringElements();
			newLocationSpeechPos.deleteAll();

			newGraves.deleteAll();
			newGraveMoves.deleteAll();


			newEmotPlayerIDs.deleteAll();
			newEmotIndices.deleteAll();
			newEmotTTLs.deleteAll();

			newOwnerPos.deleteAll();


			// handle end-of-frame for all players that need it
			const char *frameMessage = "FM\n#";
			int frameMessageLength = strlen( frameMessage );

			for( int i=0; i<players.size(); i++ ) {
				LiveObject *nextPlayer = players.getElement(i);

				if( nextPlayer->gotPartOfThisFrame && nextPlayer->connected ) {
					int numSent =
							nextPlayer->sock->send(
									(unsigned char*)frameMessage,
									frameMessageLength,
									false, false );

					if( numSent != frameMessageLength ) {
						setPlayerDisconnected( nextPlayer, "Socket write failed" );
					}
				}
				nextPlayer->gotPartOfThisFrame = false;
			}



			// handle closing any that have an error
			for( int i=0; i<players.size(); i++ ) {
				LiveObject *nextPlayer = players.getElement(i);

				if( nextPlayer->error && nextPlayer->deleteSent &&
				nextPlayer->deleteSentDoneETA < Time::getCurrentTime() ) {
					AppLog::infoF( "Closing connection to player %d on error "
								   "(cause: %s)",
								   nextPlayer->id, nextPlayer->errorCauseString );

					AppLog::infoF( "%d remaining player(s) alive on server ",
								   players.size() - 1 );

					addPastPlayer( nextPlayer );

					if( nextPlayer->sock != NULL ) {
						sockPoll.removeSocket( nextPlayer->sock );

						delete nextPlayer->sock;
						nextPlayer->sock = NULL;
					}

					if( nextPlayer->sockBuffer != NULL ) {
						delete nextPlayer->sockBuffer;
						nextPlayer->sockBuffer = NULL;
					}

					delete nextPlayer->lineage;

					delete nextPlayer->ancestorIDs;

					nextPlayer->ancestorEmails->deallocateStringElements();
					delete nextPlayer->ancestorEmails;

					nextPlayer->ancestorRelNames->deallocateStringElements();
					delete nextPlayer->ancestorRelNames;

					delete nextPlayer->ancestorLifeStartTimeSeconds;


					if( nextPlayer->name != NULL ) {
						delete [] nextPlayer->name;
					}

					if( nextPlayer->currentOrder != NULL ) {
						delete [] nextPlayer->currentOrder;
					}

					if( nextPlayer->familyName != NULL ) {
						delete [] nextPlayer->familyName;
					}

					if( nextPlayer->lastSay != NULL ) {
						delete [] nextPlayer->lastSay;
					}

					freePlayerContainedArrays( nextPlayer );

					if( nextPlayer->pathToDest != NULL ) {
						delete [] nextPlayer->pathToDest;
					}

					if( nextPlayer->email != NULL ) {
						delete [] nextPlayer->email;
					}
					if( nextPlayer->origEmail != NULL  ) {
						delete [] nextPlayer->origEmail;
					}
					if( nextPlayer->lastBabyEmail != NULL ) {
						delete [] nextPlayer->lastBabyEmail;
					}
					if( nextPlayer->lastSidsBabyEmail != NULL ) {
						delete [] nextPlayer->lastSidsBabyEmail;
					}

					if( nextPlayer->murderPerpEmail != NULL ) {
						delete [] nextPlayer->murderPerpEmail;
					}

					if( nextPlayer->deathReason != NULL ) {
						delete [] nextPlayer->deathReason;
					}

					nextPlayer->globalMessageQueue.deallocateStringElements();

					delete nextPlayer->babyBirthTimes;
					delete nextPlayer->babyIDs;

					players.deleteElement( i );
					i--;
				}
			}


			if( players.size() == 0 && newConnections.size() == 0 ) {
				if( shutdownMode ) {
					AppLog::info( "No live players or connections in shutdown "
								  " mode, auto-quitting." );
					quit = true;
				}
			}
		}

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


