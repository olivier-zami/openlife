//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_SERVER_H
#define OPENLIFE_SERVER_H

#include <string>
#include <array>
#include <vector>

#include "src/server/channel/ipc.h"
#include "src/server/service/database/worldMap.h"
#include "src/server/type/entities.h"
#include "src/system/_base/object/abstract/service.h"
#include "OneLife/gameSource/GridPos.h"
#include "minorGems/network/web/WebRequest.h"
#include "OneLife/gameSource/spriteBank.h"

namespace openLife::server
{
	typedef struct{
		std::vector<openLife::server::type::entity::Biome> biome;
		struct{
			int* shutdownMode;
			int* forceShutdownMode;
			char* someClientMessageReceived;
		}global;
		struct{
			std::vector<unsigned int> relief;
		}map;
		struct{
			int type;
			struct{
				std::string filename;
			}sketch;
		}mapGenerator;
	}Settings;
}

typedef struct KillState
{
	int killerID;
	int killerWeaponID;
	int targetID;
	double killStartTime;
	double emotStartTime;
	int emotRefreshSeconds;
	int posseSize;
	// set when the first person joins the posse
	// based on population size in radius
	// later joiners copy this value
	int minPosseSizeForKill;

	// true if solo posse permitted in high-pop area
	// due to enemies overwhelming allies
	char fullForceSoloPosse;
} KillState;

// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
	Socket *sock;
	SimpleVector<char> *sockBuffer;

	unsigned int sequenceNumber;
	char *sequenceNumberString;

	WebRequest *ticketServerRequest;
	char ticketServerAccepted;
	char lifeTokenSpent;

	float fitnessScore;

	double ticketServerRequestStartTime;

	char error;
	const char *errorCauseString;

	double rejectedSendTime;

	char shutdownMode;

	// for tracking connections that have failed to LOGIN
	// in a timely manner
	double connectionStartTimeSeconds;

	char *email;

	int tutorialNumber;
	CurseStatus curseStatus;
	PastLifeStats lifeStats;

	char *twinCode;
	int twinCount;

	char *clientTag;

	char reconnectOnly;

} FreshConnection;

typedef struct UpdateRecord{
	char *formatString;
	char posUsed;
	int absolutePosX, absolutePosY;
	GridPos absoluteActionTarget;
	int absoluteHeldOriginX, absoluteHeldOriginY;
} UpdateRecord;

typedef struct PeaceTreaty
{
	int lineageAEveID;
	int lineageBEveID;

	// they have to say it in both directions
	// before it comes into effect
	char dirAToB;
	char dirBToA;

	// track directions of breaking it later
	char dirAToBBroken;
	char dirBToABroken;
} PeaceTreaty;

typedef struct WarState
{
	int lineageAEveID;
	int lineageBEveID;
} WarState;

typedef struct DeadObject
{
	int id;

	int displayID;

	char *name;

	SimpleVector<int> *lineage;

	// id of Eve that started this line
	int lineageEveID;

	// time that this life started (for computing age)
	// not actual creation time (can be adjusted to tweak starting age,
	// for example, in case of Eve who starts older).
	double lifeStartTimeSeconds;

	// time this person died
	double deathTimeSeconds;
} DeadObject;

typedef struct GraveInfo
{
	GridPos pos;
	int playerID;
	// eve that started the line of this dead person
	// used for tracking whether grave is part of player's family or not
	int lineageEveID;
}GraveInfo;

typedef struct GraveMoveInfo
{
	GridPos posStart;
	GridPos posEnd;
	int swapDest;
}GraveMoveInfo;

// tracking spots on map that inflicted a mortal wound
// put them on timeout afterward so that they don't attack
// again immediately
typedef struct DeadlyMapSpot
{
	GridPos pos;
	double timeOfAttack;
}DeadlyMapSpot;

typedef struct FullMapContained
{
	int numContained;
	int *containedIDs;
	timeSec_t *containedEtaDecays;
	SimpleVector<int> *subContainedIDs;
	SimpleVector<timeSec_t> *subContainedEtaDecays;
} FullMapContained;

typedef enum messageType {
	MOVE,
	USE,
	SELF,
	BABY,
	UBABY,
	REMV,
	SREMV,
	DROP,
	KILL,
	SAY,
	EMOT,
	JUMP,
	DIE,
	GRAVE,
	OWNER,
	FORCE,
	MAP,
	TRIGGER,
	BUG,
	PING,
	VOGS,
	VOGN,
	VOGP,
	VOGM,
	VOGI,
	VOGT,
	VOGX,
	PHOTO,
	LEAD,
	UNFOL,
	FLIP,
	UNKNOWN
} messageType;

typedef struct ClientMessage
{
	messageType type;
	int x, y, c, i, id;

	int trigger;
	int bug;

	// some messages have extra positions attached
	int numExtraPos;

	// NULL if there are no extra
	GridPos *extraPos;

	// null if type not SAY
	char *saidText;

	// null if type not BUG
	char *bugText;

	// for MOVE messages
	int sequenceNumber;
} ClientMessage;

typedef struct MoveRecord
{
	int playerID;
	char *formatString;
	int absoluteX, absoluteY;
}MoveRecord;

typedef struct WarPeaceMessageRecord {
	char war;
	int lineageAEveID;
	int lineageBEveID;
	double t;
}WarPeaceMessageRecord;

typedef struct ForcedEffects {
	// -1 if no emot specified
	int emotIndex;
	int ttlSec;

	char foodModifierSet;
	double foodCapModifier;

	char feverSet;
	float fever;
} ForcedEffects;

typedef struct FlightDest {
	int playerID;
	GridPos destPos;
} FlightDest;

typedef struct ForceSpawnRecord {
	GridPos pos;
	double age;
	char *firstName;
	char *lastName;
	int displayID;
	int hatID;
	int tunicID;
	int bottomID;
	int frontShoeID;
	int backShoeID;
} ForceSpawnRecord;

namespace openLife
{
	class Server
	{
		public:
			Server(
					openLife::server::Settings serverSettings,
					openLife::server::settings::WorldMap worldMapSettings,
					LINEARDB3* biomeDB,
					char* anyBiomesInDB);
			~Server();

			void init();
			void start();
			openLife::server::service::database::WorldMap* getWorldMap();

			//private:
			int initMap();
			void initSpeechService();
			openLife::server::channel::Ipc* ipcManager;

			int* shutdownMode;
			int* forceShutdownMode;
			char* someClientMessageReceived;
			int nextOrderNumber;
			double lastPastPlayerFlushTime;
			double deadlyMapSpotTimeoutSec;
			double minFlightSpeed; // min speed for takeoff
			double playerCrossingCheckStepTime;// how often do we check what a player is standing on top of for attack effects?
			double periodicStepTime;// for steps in main loop that shouldn't happen every loop// (loop runs faster or slower depending on how many messages are incoming)
			double lastPeriodicStepTime;
			int numPlayersRecomputeHeatPerStep;
			int lastPlayerIndexHeatRecomputed;
			double lastHeatUpdateTime;// recompute heat for fixed number of players per timestep
			double heatUpdateTimeStep;
			double heatUpdateSeconds;// how often the player's personal heat advances toward their environmental// heat value
			float rAir;// air itself offers some insulation// a vacuum panel has R-value that is 25x greater than air
			char eveWindowOver;

			openLife::server::service::database::WorldMap* worldMap;

			static const unsigned int WORLD_MAP;
	};
}

void freePlayerContainedArrays(LiveObject *inPlayer);
void addPastPlayer(LiveObject *inPlayer);
double measurePathLength(int inXS, int inYS, GridPos *inPathPos, int inPathLength);
double getPathSpeedModifier(GridPos *inPathPos, int inPathLength);
char equal(GridPos inA, GridPos inB);
char addHeldToContainer( LiveObject *inPlayer, int inTargetID, int inContX, int inContY, char inSwap = false);
char *getToolSetDescription( ObjectRecord *inToolO );
void sendLearnedToolMessage( LiveObject *inPlayer, SimpleVector<int> *inNewToolSets );
char heldNeverDrop( LiveObject *inPlayer );
KillState *getKillState( LiveObject *inKiller );
void removeKillState( LiveObject *inKiller, LiveObject *inTarget );
char *translatePhraseFromSpeaker( char *inPhrase, LiveObject *speakerObj, LiveObject *listenerObj );
void replaceOrder( LiveObject *nextPlayer, char *inFormattedOrder, int inOrderNumber, int inOriginatorID );
void tryToForceDropHeld(LiveObject *inTargetPlayer, SimpleVector<int> *playerIndicesToSendUpdatesAbout );
char canPlayerUseOrLearnTool( LiveObject *inPlayer, int inToolID );
char *getLineageLastName( int inLineageEveID );
char isAlreadyInKillState( LiveObject *inKiller );
void removeAnyKillState( LiveObject *inKiller );
void setHeldGraveOrigin( LiveObject *inPlayer, int inX, int inY, int inNewTarget );
void sendCraving( LiveObject *inPlayer );
void setRefuseFoodEmote( LiveObject *hitPlayer );
int checkTargetInstantDecay( int inTarget, int inX, int inY );
void handleHeldDecay(LiveObject *nextPlayer, int i, SimpleVector<int> *playerIndicesToSendUpdatesAbout,SimpleVector<int> *playerIndicesToSendHealingAbout );
void checkOrderPropagation();
void tryToStartKill( LiveObject *nextPlayer, int inTargetID, SimpleVector<int> *playerIndicesToSendUpdatesAbout, char inInfiniteRange = false );
LiveObject *getClosestFollower( LiveObject *inLeader );
void leaderDied( LiveObject *inLeader );
unsigned char *getExileMessage( char inAll, int *outLength );
unsigned char *getFollowingMessage( char inAll, int *outLength );
void findExpertForPlayer( LiveObject *inPlayer, ObjectRecord *inTouchedObject );
LiveObject *getPlayerByName( char *inName, LiveObject *inPlayerSayingName );
char isAccessBlocked( LiveObject *inPlayer, int inTargetX, int inTargetY, int inTargetID, int inHoldingID = 0 );
char isPlayerBlockedFromHoldingByPosse( LiveObject *inPlayer );
void logClientTag( FreshConnection *inConnection );
void endBiomeSickness(LiveObject *nextPlayer, int i, SimpleVector<int> *playerIndicesToSendUpdatesAbout );
void sendHomelandMessage( LiveObject *nextPlayer, int homeLineageEveID, GridPos homeCenter );
void nameEve( LiveObject *nextPlayer, char *name );
void checkForFoodEatingEmot( LiveObject *inPlayer, int inEatenID );
void sendWarReportToOne( LiveObject *inO );
TransRecord *getBareHandClothingTrans( LiveObject *nextPlayer, ObjectRecord **clothingSlot );
void removeClothingToHold( LiveObject *nextPlayer, LiveObject *targetPlayer, ObjectRecord **clothingSlot, int clothingSlotIndex );
ObjectRecord **getClothingSlot( LiveObject *targetPlayer, int inIndex );
char removeFromClothingContainerToHold( LiveObject *inPlayer, int inC, int inI);
void pickupToHold( LiveObject *inPlayer, int inX, int inY, int inTargetID );
void changeContained(int inX, int inY, int inSlotNumber, int inNewObjectID );
char containmentPermitted( int inContainerID, int inContainedID );
int getContainerSwapIndex( LiveObject *inPlayer, int idToAdd, int inStillHeld, int inSearchLimit, int inContX, int inContY );
char isYummy( LiveObject *inPlayer, int inObjectID );
int objectRecordToID( ObjectRecord *inRecord );
void restockPostWindowFamilies();
char addHeldToClothingContainer( LiveObject *inPlayer, int inC, char inWillSwap = false, char *outCouldHaveGoneIn = NULL );
void processWaitingTwinConnection( FreshConnection inConnection );
void setupToolSlots( LiveObject *inPlayer );
void triggerApocalypseNow( const char *inMessage );
char isEveWindow();
int countFollowers( LiveObject *inLeader );
UpdateRecord getUpdateRecord(LiveObject *inPlayer, char inDelete, char inPartial = false );
double getLinearFoodScaleFactor( LiveObject *inPlayer );
int getEatCost( LiveObject *inPlayer );
char *getUpdateLineFromRecord(UpdateRecord *inRecord, GridPos inRelativeToPos, GridPos inObserverPos );
float computeHeldHeat( LiveObject *inPlayer );
double rCombine( double inRA, double inRB );
LiveObject *getHitPlayer( int inX, int inY, int inTargetID = -1, char inCountMidPath = false, int inMaxAge = -1, int inMinAge = -1, int *outHitIndex = NULL);
char *getUpdateLine( LiveObject *inPlayer, GridPos inRelativeToPos, GridPos inObserverPos, char inDelete, char inPartial = false );
void updateYum( LiveObject *inPlayer, int inFoodEatenID, char inFedSelf = true);
int getScaledFoodValue( LiveObject *inPlayer, int inFoodValue );
int getEatBonus( LiveObject *inPlayer );
void swapHeldWithGround(LiveObject *inPlayer, int inTargetID, int inMapX, int inMapY, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout);
char isGridAdjacentDiag( GridPos inA, GridPos inB );
char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB );
void recomputeHeatMap( LiveObject *inPlayer );
float computeClothingHeat( LiveObject *inPlayer );
float computeClothingR( LiveObject *inPlayer );
float sign( float inF );
void drinkAlcohol( LiveObject *inPlayer, int inAlcoholAmount );
void addShortLife( char *inEmail );
double getPathSpeedModifier( GridPos *inPathPos, int inPathLength );
double measurePathLength(int inXS, int inYS, GridPos *inPathPos, int inPathLength );
void addDeadlyMapSpot( GridPos inPos );
char wasRecentlyDeadly( GridPos inPos );
void addPastPlayer( LiveObject *inPlayer );
void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID );
void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID );
void sendMessageToPlayer( LiveObject *inPlayer, char *inMessage, int inLength );
int getLiveObjectIndex( int inID );
char realSpriteBank();
void stopOutputAllFrames();
void startOutputAllFrames();
void freeSprite( void* );
void *loadSpriteBase( const char*, char );
void unCountLiveUse( SoundUsage inUsage );
void countLiveUse( SoundUsage inUsage );
void toggleMultiplicativeBlend( char inMultiplicative );
char getNoFlip( int inID );
char getUsesMultiplicativeBlending( int inID );
char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset );
void checkIfSoundStillNeeded( int inID );
SpriteRecord *getSpriteRecord( int inSpriteID );
void stopStencil();
void startDrawingThroughStencil( char );
void startAddingToStencil( char, char, float );
void drawSquare( doublePair, double );
void toggleAdditiveBlend( char );
void toggleAdditiveTextureColoring( char inAdditive );
float getTotalGlobalFade();
void setDrawFade( float );
void setDrawColor( FloatColor inColor );
void setDrawColor( float inR, float inG, float inB, float inA );
void drawSprite( void*, doublePair, double, double, char );
void stepSpriteBank();
char markSpriteLive( int );
char isSpriteBankLoaded();
char *getSpriteTag( int );
void *getSprite( int );
void makePlayerBiomeSick( LiveObject *nextPlayer, int sicknessObjectID );
char *getLeadershipName( LiveObject *nextPlayer, char inNoName  = false );
int getUnusedLeadershipColor();
char isFollower( LiveObject *inLeader, LiveObject *inTestFollower );
char isHungryWorkBlocked( LiveObject *inPlayer, int inNewTarget, int *outCost );
void sendHungryWorkSpeech( LiveObject *inPlayer );
char isBiomeAllowedForPlayer( LiveObject *inPlayer, int inX, int inY, char inIgnoreFloor = false );
void logFitnessDeath( LiveObject *nextPlayer );
void getLineageLineForPlayer( LiveObject *inPlayer, SimpleVector<char> *inVector );
void replaceNameInSaidPhrase( char *inSaidName, char **inSaidPhrase, LiveObject *inNamedPerson, char inForceBoth = false);
void nameBaby( LiveObject *inNamer, LiveObject *inBaby, char *inName, SimpleVector<int> *playerIndicesToSendNamesAbout );
void executeKillAction(
		int inKillerIndex,
		int inTargetIndex,
		SimpleVector<int> *playerIndicesToSendUpdatesAbout,
		SimpleVector<int> *playerIndicesToSendDyingAbout,
		SimpleVector<int> *newEmotPlayerIDs,
		SimpleVector<int> *newEmotIndices,
		SimpleVector<int> *newEmotTTLs );
void logHealOfKill( LiveObject *inVictim, LiveObject *inHealer );
void logKillHit( LiveObject *inVictim, LiveObject *inKiller );
FILE *getKillLogFile( char outTimeStamp[16] );
void setPerpetratorHoldingAfterKill( LiveObject *nextPlayer, TransRecord *woundHit, TransRecord *rHit, TransRecord *r );
void interruptAnyKillEmots( int inPlayerID, int inInterruptingTTL );
char addKillState( LiveObject *inKiller, LiveObject *inTarget, char inInfiniteRange);
char isNoWaitWeapon( int inObjectID );
void updatePosseSize( LiveObject *inTarget, LiveObject *inRemovedKiller);
int countPosseSize( LiveObject *inTarget, int *outMinPosseSizeForKill = NULL, char *outFullForceSoloPosse = NULL);
void checkSickStaggerTime( LiveObject *inPlayer );
void setNoLongerDying( LiveObject *inPlayer, SimpleVector<int> *inPlayerIndicesToSendHealingAbout );
ForcedEffects checkForForcedEffects( int inHeldObjectID );
char *getUniqueCursableName( char *inPlayerName, char *outSuffixAdded, char inIsEve, char inFemale );
void monumentStep();
void apocalypseStep();
int readIntFromFile( const char *inFileName, int inDefaultValue );
LiveObject *getClosestOtherPlayer( LiveObject *inThisPlayer, double inMinAge = 0, char inNameMustBeNULL = false);
char *isNamedForgivingSay( char *inSaidString );
char isYouForgivingSay( char *inSaidString );
char *isNamedKillSay( char *inSaidString );
char isYouKillSay( char *inSaidString );
char *isNamedRedeemSay( char *inSaidString );
char isYouRedeemSay( char *inSaidString );
char *isNamedExileSay( char *inSaidString );
char isYouExileSay( char *inSaidString );
char *isNamedFollowSay( char *inSaidString );
char isYouFollowSay( char *inSaidString );
char isPosseJoiningSay( char *inSaidString );
char isOffspringGivingSay( char *inSaidString );
char isFamilyGivingSay( char *inSaidString );
char isYouGivingSay( char *inSaidString );
char isWildcardGivingSay( char *inSaidString, SimpleVector<char*> *inPhrases );
char *isNamedGivingSay( char *inSaidString );
char *isCurseNamingSay( char *inSaidString );
char *isEveNamingSay( char *inSaidString );
char *isFamilyNamingSay( char *inSaidString );
char *isBabyNamingSay( char *inSaidString );
char *isReverseNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList );
char *isNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList );
void readPhrases( const char *inSettingsName, SimpleVector<char*> *inList );
void sendWarReportToAll();
char *getWarReportMessage();
unsigned char *makeCompressedMessage( char *inMessage, int inLength, int *outLength );
void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID );
char removeFromContainerToHold( LiveObject *inPlayer, int inContX, int inContY, int inSlotNumber );
char directLineBlocked( LiveObject *inShooter, GridPos inSource, GridPos inDest );
char isMapSpotBlockingForPlayer( LiveObject *inPlayer, int inX, int inY );
int processLoggedInPlayer(
		int inAllowOrForceReconnect,
	   	Socket *inSock,
		SimpleVector<char> *inSockBuffer,
		char *inEmail,
		int inTutorialNumber,
		CurseStatus inCurseStatus,
		PastLifeStats inLifeStats,
		float inFitnessScore,
		int inForceParentID = -1,
		int inForceDisplayID = -1,
		GridPos *inForcePlayerPos = NULL );
int countLivingChildren( int inMotherID );
void makeOffspringSayMarker( int inPlayerID, int inIDToSkip );
char getForceSpawn( char *inEmail, ForceSpawnRecord *outRecordToFill );
int countEnemies( LiveObject *inPlayer, GridPos inPos, double inRadius );
int countAllies( LiveObject *inPlayer, GridPos inPos, double inRadius );
char isExiled( LiveObject *inViewer, LiveObject *inTarget );
char isLeader( LiveObject *inPlayer, int inPossibleLeaderID );
int getTopLeader( LiveObject *inPlayer );
int countFamilies();
int countNonHelpless( GridPos inPos, double inRadius, double inMinAge);
int countLivingPlayers();
int countHelplessBabies();
int countGirls( int inLineageEveID, int inRace);
int countFertileMothers( int inLineageEveID, int inRace);
int isPlayerCountable( LiveObject *p, int inLineageEveID, int inRace);
char canPlayerUseTool( LiveObject *inPlayer, int inToolID );
int getToolSet( int inToolID );
void handleForcedBabyDrop(LiveObject *inBabyObject, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
LiveObject *getAdultHolding( LiveObject *inBabyObject );
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
int isGraveSwapDest( int inTargetX, int inTargetY, int inDroppingPlayerID );
void holdingSomethingNew( LiveObject *inPlayer, int inOldHoldingID = 0 );
void sendToolExpertMessage( LiveObject *inPlayer, int inGroundToolID);
void forcePlayerToRead( LiveObject *inPlayer, int inObjectID );
void makePlayerSay( LiveObject *inPlayer, char *inToSay );
LiveObject *getPlayerByEmail( char *inEmail );
GridPos findClosestEmptyMapSpot( int inX, int inY, int inMaxPointsToCheck, char *outFound );
char findDropSpot( LiveObject *inDroppingPlayer, int inX, int inY, int inSourceX, int inSourceY, GridPos *outSpot );
void handleMapChangeToPaths(int inX, int inY, ObjectRecord *inNewObject, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
void endAnyMove( LiveObject *nextPlayer );
void truncateMove( LiveObject *otherPlayer, int blockedStep );
void setFreshEtaDecayForHeld( LiveObject *inPlayer );
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true );
char isMapSpotEmptyOfPlayers( int inX, int inY );
char *getHoldingString( LiveObject *inObject );
int sendMapChunkMessage( LiveObject *inO, char inDestOverride = false, int inDestOverrideX = 0, int inDestOverrideY = 0 );
void checkCustomGlobalMessage();
void sendPeaceWarMessage( const char *inPeaceOrWar, char inWar, int inLineageAEveID, int inLineageBEveID );
void sendGlobalMessage( char *inMessage, LiveObject *inOnePlayerOnly = NULL );
void setPlayerDisconnected( LiveObject *inPlayer, const char *inReason );
int getMaxChunkDimension();
GridPos getClosestPlayerPos( int inX, int inY );
char isGridAdjacent( int inXA, int inYA, int inXB, int inYB );
char *getMovesMessage( char inNewMovesOnly, GridPos inRelativeToPos, GridPos inLocalPos, SimpleVector<ChangePosition> *inChangeVector = NULL );
double intDist( int inXA, int inYA, int inXB, int inYB );
char *getMovesMessageFromList( SimpleVector<MoveRecord> *inMoves, GridPos inRelativeToPos );
SimpleVector<MoveRecord> getMoveRecords(char inNewMovesOnly, SimpleVector<ChangePosition> *inChangeVector );
MoveRecord getMoveRecord( LiveObject *inPlayer, char inNewMovesOnly, SimpleVector<ChangePosition> *inChangeVector );
double computeMoveSpeed( LiveObject *inPlayer );
char *slurSpeech( int inSpeakerID, char *inTranslatedPhrase, double inDrunkenness );
int computeOverflowFoodCapacity( int inBaseCapacity );
int computeFoodCapacity( LiveObject *inPlayer );
char isFertileAge( LiveObject *inPlayer );
int getFirstFertileAge();
int getSecondsPlayed( LiveObject *inPlayer );
int getSayLimit( LiveObject *inPlayer );
double computeAge( LiveObject *inPlayer );
double computeAge( double inLifeStartTimeSeconds );
void handleShutdownDeath( LiveObject *inPlayer, int inX, int inY );
void setDeathReason( LiveObject *inPlayer, const char *inTag, int inOptionalID = 0 );
double getAgeRate();
double computeFoodDecrementTimeSeconds( LiveObject *inPlayer );
void forcePlayerAge( const char *inEmail, double inAge );
GridPos killPlayer( const char *inEmail );
void logFamilyCounts();
GridPos getPlayerPos( LiveObject *inPlayer );
GridPos computePartialMoveSpot( LiveObject *inPlayer, int inOverrideC = -2  );
doublePair computePartialMoveSpotPrecise( LiveObject *inPlayer );
int computePartialMovePathStep( LiveObject *inPlayer );
double computePartialMovePathStepPrecise( LiveObject *inPlayer );
ClientMessage parseMessage( LiveObject *inPlayer, char *inMessage );
int stringToInt( char *inString );
char *getNextClientMessage( SimpleVector<char> *inBuffer );
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer );
void intHandler( int inUnused );
const char *getCurseWord( char *inSenderEmail, char *inEmail, int inWordIndex );
void quitCleanup();
char isShortLife( char *inEmail );
void deleteMembers( FreshConnection *inConnection );
void transferHeldContainedToMap( LiveObject *inPlayer, int inX, int inY );
void clearPlayerHeldContained( LiveObject *inPlayer );
void setContained( LiveObject *inPlayer, FullMapContained inContained );
void freePlayerContainedArrays( LiveObject *inPlayer );
FullMapContained getFullMapContained( int inX, int inY );
double pickBirthCooldownSeconds();
char isPlayerIgnoredForEvePlacement( int inID );
int getPlayerLineage( int inID );
char *getPlayerName( int inID );
LiveObject *getLiveObject( int inID );
void backToBasics( LiveObject *inPlayer );
char *getOwnershipString( GridPos inPos );
char *getOwnershipString( int inX, int inY );
void removeAllOwnership( LiveObject *inPlayer, char inProcessInherit = true  );
void endOwnership( int inX, int inY, int inObjectID );
char isKnownOwned( LiveObject *inPlayer, GridPos inPos );
char isKnownOwned( LiveObject *inPlayer, int inX, int inY );
char isOwnedOrAllyOwned( LiveObject *inPlayer, ObjectRecord *inObject, int inX, int inY );
char isMapSpotLeaderOwned( LiveObject *inPlayer, int inX, int inY );
char isOwned( LiveObject *inPlayer, GridPos inPos );
char isOwned( LiveObject *inPlayer, int inX, int inY );
LiveObject *findHeir( LiveObject *inPlayer );
LiveObject *findFittestOffspring( int inPlayerID, int inSkipID );
char getFemale( LiveObject *inPlayer );
char doesEveLineExist( int inEveID );
int getNumPlayers();
char isWarState( int inLineageAEveID, int inLineageBEveID );
char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID, PeaceTreaty **outPartialTreaty = NULL );
PeaceTreaty *getMatchingTreaty( int inLineageAEveID, int inLineageBEveID );
float getHighestRecentScore();
void addRecentScore( char *inEmail, float inScore );
void cleanRecentScores();
char learnTool( LiveObject *inPlayer, int inToolID );
#endif //OPENLIFE_SERVER_H
