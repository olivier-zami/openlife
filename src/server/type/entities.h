//
// Created by olivier on 30/08/2021.
//

#ifndef OPENLIFE_SYSTEM_TYPE_ENTITY_H
#define OPENLIFE_SYSTEM_TYPE_ENTITY_H

#include <string>

#include "OneLife/gameSource/objectBank.h"
#include "minorGems/network/Socket.h"
#include "OneLife/server/cravings.h"
#include "OneLife/server/curses.h"

#define HEAT_MAP_D 13

namespace openLife::server::type::entity
{
	typedef struct{
		std::string label;
		int code;
		int value;
	}Biome;

	typedef struct{
		std::string label;
	}Climate;
}

typedef struct {
	unsigned int uniqueLoadID;
	char *mapFileName;
	char fileOpened;
	FILE *file;
	int x, y;
	double startTime;
	int stepCount;
} TutorialLoadProgress;

typedef struct PastLifeStats {
	int lifeCount;
	int lifeTotalSeconds;
	char error;
} PastLifeStats;

typedef struct LiveObject {
	char *email;
	// for tracking old email after player has been deleted
	// but is still on list
	char *origEmail;

	int id;

	float fitnessScore;

	int numToolSlots;
	// these aren't object IDs but tool set index numbers
	// some tools are grouped together
	SimpleVector<int> learnedTools;

	// tool set index numbers that have been tried at least once
	SimpleVector<int> partiallyLearnedTools;


	// object ID used to visually represent this player
	int displayID;

	char *name;
	char nameHasSuffix;

	char *familyName;


	char *lastSay;

	CurseStatus curseStatus;
	PastLifeStats lifeStats;

	int curseTokenCount;
	char curseTokenUpdate;


	char isEve;

	char isTutorial;

	char isTwin;

	// if last life is too short, assume they are die-cycling
	// to pick their birth location, and don't count them when computing
	// posse size
	char isLastLifeShort;


	// used to track incremental tutorial map loading
	TutorialLoadProgress tutorialLoad;


	GridPos birthPos;
	GridPos originalBirthPos;


	int parentID;

	// 1 for Eve
	int parentChainLength;

	SimpleVector<int> *lineage;

	SimpleVector<int> *ancestorIDs;
	SimpleVector<char*> *ancestorEmails;
	SimpleVector<char*> *ancestorRelNames;
	SimpleVector<double> *ancestorLifeStartTimeSeconds;

	// id of Eve that started this line
	int lineageEveID;


	// who this player is following
	// might be a dead player
	// -1 means following self (no one)
	int followingID;

	// -1 if not set
	int leadingColorIndex;

	// people who have exiled this player
	// some could be dead
	SimpleVector<int> exiledByIDs;

	char followingUpdate;
	char exileUpdate;

	int currentOrderNumber;
	// who issued this order?
	int currentOrderOriginatorID;
	char *currentOrder;


	// time that this life started (for computing age)
	// not actual creation time (can be adjusted to tweak starting age,
	// for example, in case of Eve who starts older).
	double lifeStartTimeSeconds;

	// time when this player actually died
	double deathTimeSeconds;


	// the wall clock time when this life started
	// used for computing playtime, not age
	double trueStartTimeSeconds;


	double lastSayTimeSeconds;

	double firstEmoteTimeSeconds;
	int emoteCountInWindow;
	char emoteCooldown;
	double emoteCooldownStartTimeSeconds;


	// held by other player?
	char heldByOther;
	int heldByOtherID;
	char everHeldByParent;

	// player that's responsible for updates that happen to this
	// player during current step
	int responsiblePlayerID;

	// affects movement speed if part of posse
	int killPosseSize;


	// start and dest for a move
	// same if reached destination
	int xs;
	int ys;

	int xd;
	int yd;

	// next player update should be flagged
	// as a forced position change
	char posForced;

	char waitingForForceResponse;

	int lastMoveSequenceNumber;


	int facingLeft;
	double lastFlipTime;


	int pathLength;
	GridPos *pathToDest;

	char pathTruncated;

	char firstMapSent;
	int lastSentMapX;
	int lastSentMapY;

	// path dest for the last full path that we checked completely
	// for getting too close to player's known map chunk
	GridPos mapChunkPathCheckedDest;


	double moveTotalSeconds;
	double moveStartTime;

	double pathDist;


	int facingOverride;
	int actionAttempt;
	GridPos actionTarget;

	int holdingID;

	// absolute time in seconds that what we're holding should decay
	// or 0 if it never decays
	timeSec_t holdingEtaDecay;


	// where on map held object was picked up from
	char heldOriginValid;
	int heldOriginX;
	int heldOriginY;


	// track origin of held separate to use when placing a grave
	int heldGraveOriginX;
	int heldGraveOriginY;
	int heldGravePlayerID;


	// if held object was created by a transition on a target, what is the
	// object ID of the target from the transition?
	int heldTransitionSourceID;


	int numContained;
	int *containedIDs;
	timeSec_t *containedEtaDecays;

	// vector of sub-contained for each contained item
	SimpleVector<int> *subContainedIDs;
	SimpleVector<timeSec_t> *subContainedEtaDecays;


	// if they've been killed and part of a weapon (bullet?) has hit them
	// this will be included in their grave
	int embeddedWeaponID;
	timeSec_t embeddedWeaponEtaDecay;

	// and what original weapon killed them?
	int murderSourceID;
	char holdingWound;

	char holdingBiomeSickness;

	// who killed them?
	int murderPerpID;
	char *murderPerpEmail;

	// or if they were killed by a non-person, what was it?
	int deathSourceID;

	// true if this character landed a mortal wound on another player
	char everKilledAnyone;

	// when they last landed a kill
	double lastKillTime;

	// true in case of sudden infant death
	char suicide;


	Socket *sock;
	SimpleVector<char> *sockBuffer;

	// indicates that some messages were sent to this player this
	// frame, and they need a FRAME terminator message
	char gotPartOfThisFrame;


	char isNew;
	char isNewCursed;
	char firstMessageSent;

	char inFlight;


	char dying;
	// wall clock time when they will be dead
	double dyingETA;

	// in cases where their held wound produces a forced emot
	char emotFrozen;
	double emotUnfreezeETA;
	int emotFrozenIndex;

	char starving;


	char connected;

	char error;
	const char *errorCauseString;



	int customGraveID;

	char *deathReason;

	char deleteSent;
	// wall clock time when we consider the delete good and sent
	// and can close their connection
	double deleteSentDoneETA;

	char deathLogged;

	char newMove;

	// heat map that player carries around with them
	// every time they stop moving, it is updated to compute
	// their local temp
	float heatMap[ HEAT_MAP_D * HEAT_MAP_D ];

	// net heat of environment around player
	// map is tracked in heat units (each object produces an
	// integer amount of heat)
	// this is in base heat units, range 0 to infinity
	float envHeat;

	// amount of heat currently in player's body, also in
	// base heat units
	float bodyHeat;


	// used track current biome heat for biome-change shock effects
	float biomeHeat;
	float lastBiomeHeat;


	// body heat normalized to [0,1], with targetHeat at 0.5
	float heat;

	// flags this player as needing to recieve a heat update
	char heatUpdate;

	// wall clock time of last time this player was sent
	// a heat update
	double lastHeatUpdate;

	// true if heat map features player surrounded by walls
	char isIndoors;

	double foodDrainTime;
	double indoorBonusTime;
	double indoorBonusFraction;


	int foodStore;

	double foodCapModifier;

	double drunkenness;


	double fever;


	// wall clock time when we should decrement the food store
	double foodDecrementETASeconds;

	// should we send player a food status message
	char foodUpdate;

	// info about the last thing we ate, for FX food messages sent
	// just to player
	int lastAteID;
	int lastAteFillMax;

	// this is for PU messages sent to everyone
	char justAte;
	int justAteID;

	// chain of non-repeating foods eaten
	SimpleVector<int> yummyFoodChain;

	// how many bonus from yummy food is stored
	// these are used first before food is decremented
	int yummyBonusStore;

	// last time we told player their capacity in a food update
	// what did we tell them?
	int lastReportedFoodCapacity;


	ClothingSet clothing;

	timeSec_t clothingEtaDecay[NUM_CLOTHING_PIECES];

	SimpleVector<int> clothingContained[NUM_CLOTHING_PIECES];

	SimpleVector<timeSec_t>
			clothingContainedEtaDecays[NUM_CLOTHING_PIECES];

	char needsUpdate;
	char updateSent;
	char updateGlobal;

	char wiggleUpdate;


	// babies born to this player
	SimpleVector<timeSec_t> *babyBirthTimes;
	SimpleVector<int> *babyIDs;

	// for CURSE MY BABY after baby is dead/deleted
	char *lastBabyEmail;


	// wall clock time after which they can have another baby
	// starts at 0 (start of time epoch) for non-mothers, as
	// they can have their first baby right away.
	timeSec_t birthCoolDown;


	timeSec_t lastRegionLookTime;

	double playerCrossingCheckTime;


	char monumentPosSet;
	GridPos lastMonumentPos;
	int lastMonumentID;
	char monumentPosSent;

	char monumentPosInherited;


	char holdingFlightObject;

	char vogMode;
	GridPos preVogPos;
	GridPos preVogBirthPos;
	int vogJumpIndex;
	char postVogMode;

	char forceSpawn;


	// list of positions owned by this player
	SimpleVector<GridPos> ownedPositions;

	// list of owned positions that this player has heard about
	SimpleVector<GridPos> knownOwnedPositions;

	GridPos forceFlightDest;
	double forceFlightDestSetTime;

	SimpleVector<int> permanentEmots;

	// email of last baby that we had that did /DIE
	char *lastSidsBabyEmail;

	char everHomesick;

	double lastGateVisitorNoticeTime;
	double lastNewBabyNoticeTime;

	Craving cravingFood;
	int cravingFoodYumIncrement;
	char cravingKnown;

	// to give new players a boost
	// set these at birth based on how long they have played so far
	int personalEatBonus;
	double personalFoodDecrementSecondsBonus;


	// don't send global messages too quickly
	// give player chance to read each one
	double lastGlobalMessageTime;

	SimpleVector<char*> globalMessageQueue;


} LiveObject;

#endif //OPENLIFE_SYSTEM_TYPE_ENTITY_H
