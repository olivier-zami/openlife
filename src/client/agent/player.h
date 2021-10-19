//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_PLAYER_H
#define OPENLIFE_PLAYER_H

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/GridPos.h"
#include "OneLife/gameSource/emotion.h"
#include "minorGems/game/gameGraphics.h"



typedef struct LiveObject
{
	int id;

	int displayID;

	char allSpritesLoaded;

	char onScreen;

	double age;
	double ageRate;

	char finalAgeSet;

	double lastAgeSetTime;

	SimpleVector<int> lineage;

	int lineageEveID;


	char outOfRange;
	char dying;
	char sick;

	char *name;

	char *relationName;

	// -1 war
	// 0 neutral
	// 1 peace
	int warPeaceStatus;


	int curseLevel;

	char *curseName;

	int excessCursePoints;

	int curseTokenCount;


	// roll back age temporarily to make baby revert to crying
	// when baby speaks
	char tempAgeOverrideSet;
	double tempAgeOverride;
	double tempAgeOverrideSetTime;


	int foodStore;
	int foodCapacity;

	int maxFoodStore;
	int maxFoodCapacity;

	// -1 unless we're currently being held
	// by an adult
	int heldByAdultID;

	// -1 if not set
	// otherwise, ID of adult that is holding us according to pending
	// messages, but not according to already-played messages
	int heldByAdultPendingID;


	// usually 0, unless we're being held by an adult
	// and just got put down
	// then we slide back into position
	doublePair heldByDropOffset;

	// the actual world pos we were last held at
	char lastHeldByRawPosSet;
	doublePair lastHeldByRawPos;

	// true if locally-controlled baby is attempting to jump out of arms
	char babyWiggle;
	double babyWiggleProgress;


	// usually 0, but used to slide into and out of riding position
	doublePair ridingOffset;


	// 0 or positive holdingID means holding nothing or an object
	// a negative number here means we're holding another player (baby)
	// and the number, made positive, is the ID of the other player
	int holdingID;
	int lastHoldingID;

	char holdingFlip;

	double lastFlipSendTime;
	char lastFlipSent;


	// if not learned, held flipped 180 degrees
	char heldLearned;


	char heldPosOverride;
	char heldPosOverrideAlmostOver;
	doublePair heldObjectPos;
	double heldObjectRot;
	int heldPosSlideStepCount;

	AnimType curAnim;
	AnimType lastAnim;
	double lastAnimFade;

	// anim tracking for held object
	AnimType curHeldAnim;
	AnimType lastHeldAnim;
	double lastHeldAnimFade;



	// furture states that curAnim should fade to, one at a time
	SimpleVector<AnimType> *futureAnimStack;
	SimpleVector<AnimType> *futureHeldAnimStack;

	// store frame counts in fractional form
	// this allows animations that can vary in speed without
	// ever experiencing discontinuities
	// (an integer frame count, with a speed modifier applied later
	// could jump backwards in time when the modifier changes)
	double animationFrameCount;
	double heldAnimationFrameCount;

	double lastAnimationFrameCount;
	double lastHeldAnimationFrameCount;


	double frozenRotFrameCount;
	double heldFrozenRotFrameCount;

	char frozenRotFrameCountUsed;
	char heldFrozenRotFrameCountUsed;


	float heat;
	float foodDrainTime;
	float indoorBonusTime;


	int numContained;
	int *containedIDs;
	SimpleVector<int> *subContainedIDs;

	ClothingSet clothing;
	// stacks of items contained in each piece of clothing
	SimpleVector<int> clothingContained[ NUM_CLOTHING_PIECES ];

	float clothingHighlightFades[ NUM_CLOTHING_PIECES ];

	int currentMouseOverClothingIndex;


	// current fractional grid position and speed
	doublePair currentPos;
	// current speed is move delta per frame
	double currentSpeed;

	// current move speed in grid cells per sec
	double currentGridSpeed;


	// for instant reaction to move command when server hasn't
	// responded yet
	// in grid spaces per sec
	double lastSpeed;

	// recompute speed periodically during move so that we don't
	// fall behind when frame rate fluctuates
	double timeOfLastSpeedUpdate;

	// destination grid position
	int xd;
	int yd;

	// true if xd,yd set based on a truncated PM from the server
	char destTruncated;


	// use a waypoint along the way during pathfinding.
	// path must pass through this point on its way to xd,yd
	char useWaypoint;
	int waypointX, waypointY;
	// max path length to find that uses waypoint
	// if waypoint-including path is longer than this
	// a path stopping at the waypoint will be used instead
	// and xd, yd will be repaced with waypoint
	int maxWaypointPathLength;


	// last confirmed stationary position of this
	// object on the server (from the last player_update)
	int xServer;
	int yServer;


	int pathLength;
	GridPos *pathToDest;

	// closest spot on pathToDest to currentPos
	GridPos closestPathPos;


	int closestDestIfPathFailedX;
	int closestDestIfPathFailedY;


	int currentPathStep;
	doublePair currentMoveDirection;

	int numFramesOnCurrentStep;

	char onFinalPathStep;


	// how long whole move should take
	double moveTotalTime;

	// wall clock time in seconds object should arrive
	double moveEtaTime;

	// skip drawing this object
	char hide;

	char inMotion;

	int lastMoveSequenceNumber;

	char displayChar;

	int actionTargetX;
	int actionTargetY;

	// tweak for when we are performing an action on a moving object
	// that hasn't reach its destination yet.  actionTargetX,Y is the
	// destination, but this is the closest cell where it was at
	// when we clicked on it.
	int actionTargetTweakX;
	int actionTargetTweakY;

	char pendingAction;
	float pendingActionAnimationProgress;
	double pendingActionAnimationStartTime;

	double lastActionSendStartTime;
	// how long it took server to get back to us with a PU last
	// time we sent an action.  Most recent round-trip time
	double lastResponseTimeDelta;


	// NULL if no active speech
	char *currentSpeech;
	double speechFade;
	// wall clock time when speech should start fading
	double speechFadeETATime;

	char speechIsSuccessfulCurse;

	char speechIsCurseTag;
	double lastCurseTagDisplayTime;

	char speechIsOverheadLabel;

	char shouldDrawPathMarks;
	double pathMarkFade;


	// messages that arrive while we're still showing our current
	// movement animation
	SimpleVector<char*> pendingReceivedMessages;
	char somePendingMessageIsMoreMovement;


	// NULL if none
	Emotion *currentEmot;
	// wall clock time when emot clears
	double emotClearETATime;

	SimpleVector<Emotion*> permanentEmots;


	char killMode;
	int killWithID;

	char chasingUs;



	// id of who this player is following, or -1 if following self
	int followingID;

	int highestLeaderID;

	// list of other players who have exiled this player
	SimpleVector<int> exiledByIDs;

	// how many tiers of people are below this person
	// 0 if has no followers
	// 1 if has some followers
	// 2 if has some leaders as followers
	// 3 if has some leader-leaders as followers
	int leadershipLevel;

	char hasBadge;
	// color to draw badge
	FloatColor badgeColor;

	FloatColor personalLeadershipColor;


	// does the local player see this person as exiled?
	char isExiled;

	// does the local player see this person as dubious?
	// if they are following someone we see as exiled
	char isDubious;


	// does local player see this person as a follower?
	char followingUs;

	// for mouse over, what this local player sees
	// in front of this player's name
	char *leadershipNameTag;

	char isGeneticFamily;

} LiveObject;

namespace openLife::client::agent
{
	class Player
	{
		public:
			Player();
			~Player();
	};
}

void printPath( GridPos *inPath, int inLength );
void removeDoubleBacksFromPath( GridPos **inPath, int *inLength );
double computeCurrentAgeNoOverride( LiveObject *inObj );
double computeCurrentAge( LiveObject *inObj );
char *getDisplayObjectDescription( int inID );

#endif //OPENLIFE_PLAYER_H
