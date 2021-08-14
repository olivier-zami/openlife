
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <float.h>

#include "src/server/main.h"

#include "minorGems/io/file/Directory.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/SocketPoll.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/system/Thread.h"
#include "minorGems/system/Time.h"

#include "minorGems/game/doublePair.h"

#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/log/FileLog.h"

#include "minorGems/formats/encodingUtils.h"

#include "minorGems/io/file/File.h"


#include "map.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/objectMetadata.h"
#include "../gameSource/animationBank.h"
#include "../gameSource/categoryBank.h"
#include "../commonSource/sayLimit.h"

#include "lifeLog.h"
#include "foodLog.h"
#include "backup.h"
#include "triggers.h"
#include "playerStats.h"
#include "lineageLog.h"
#include "serverCalls.h"
#include "failureLog.h"
#include "names.h"
#include "curses.h"
#include "lineageLimit.h"
#include "objectSurvey.h"
#include "language.h"
#include "familySkipList.h"
#include "lifeTokens.h"
#include "fitnessScore.h"
#include "arcReport.h"
#include "curseDB.h"
#include "specialBiomes.h"
#include "cravings.h"
#include "offspringTracker.h"


#include "minorGems/util/random/JenkinsRandomSource.h"


//#define IGNORE_PRINTF

#ifdef IGNORE_PRINTF
#define printf(fmt, ...) (0)
#endif


static FILE *familyDataLogFile = NULL;


static JenkinsRandomSource randSource;


#include "../gameSource/GridPos.h"


#define HEAT_MAP_D 13

float targetHeat = 10;


double secondsPerYear = 60.0;


#define NUM_BADGE_COLORS 17


#define PERSON_OBJ_ID 12


int minPickupBabyAge = 10;

int babyAge = 5;

// age when bare-hand actions become available to a baby (opening doors, etc.)
int defaultActionAge = 3;

// can't walk for first 12 seconds
double startWalkingAge = 0.20;



double forceDeathAge = 60;


double minSayGapInSeconds = 1.0;

// for emote throttling
double emoteWindowSeconds = 60.0;
int maxEmotesInWindow = 10;

double emoteCooldownSeconds = 120.0;


// each generation is at minimum 14 minutes apart
// so 1024 generations is approximately 10 days
int maxLineageTracked = 1024;

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




static double minFoodDecrementSeconds = 5.0;
static double maxFoodDecrementSeconds = 20;
static double foodScaleFactor = 1.0;
static double foodScaleFactorFloor = 0.5;
static double foodScaleFactorHalfLife = 50;
static double foodScaleFactorGamma = 1.5;

static double newPlayerFoodDecrementSecondsBonus = 8;
static int newPlayerFoodEatingBonus = 5;
// first 10 hours of living
static double newPlayerFoodBonusHalfLifeSeconds = 36000;



static double indoorFoodDecrementSecondsBonus = 20.0;

static int babyBirthFoodDecrement = 10;

static int babyFeedingLevel = 2;


// fixed cost to pick up baby
// this still encourages baby-parent
// communication so as not
// to get the most mileage out of 
// food
static int nurseCost = 1;


// bonus applied to all foods
// makes whole server a bit easier (or harder, if negative)
static int eatBonus = 0;

static double eatBonusFloor = 0;
static double eatBonusHalfLife = 50;

static double eatCostMax = 5;
static double eatCostGrowthRate = 0.1;


static int canYumChainBreak = 0;
// -1 for no cap
static int yumBonusCap = -1;

static double minAgeForCravings = 10;


static double posseSizeSpeedMultipliers[4] = { 0.75, 1.25, 1.5, 2.0 };

static double killerVulnerableSeconds = 60;


static int minActivePlayersForLanguages = 15;


// keep a running sequence number to challenge each connecting client
// to produce new login hashes, avoiding replay attacks.
static unsigned int nextSequenceNumber = 1;


static int requireClientPassword = 1;
static int requireTicketServerCheck = 1;
static char *clientPassword = NULL;
static char *ticketServerURL = NULL;
static char *reflectorURL = NULL;

// larger of dataVersionNumber.txt or serverCodeVersionNumber.txt
static int versionNumber = 1;


static double childSameRaceLikelihood = 0.9;
static int familySpan = 2;


// phrases that trigger baby and family naming
static SimpleVector<char*> nameGivingPhrases;
static SimpleVector<char*> familyNameGivingPhrases;
static SimpleVector<char*> eveNameGivingPhrases;
static SimpleVector<char*> cursingPhrases;

char *curseYouPhrase = NULL;
char *curseBabyPhrase = NULL;

static SimpleVector<char*> forgivingPhrases;
static SimpleVector<char*> youForgivingPhrases;


static SimpleVector<char*> youGivingPhrases;
static SimpleVector<char*> namedGivingPhrases;

static SimpleVector<char*> familyGivingPhrases;
static SimpleVector<char*> offspringGivingPhrases;

static SimpleVector<char*> posseJoiningPhrases;


static SimpleVector<char*> youFollowPhrases;
static SimpleVector<char*> namedFollowPhrases;

static SimpleVector<char*> youExilePhrases;
static SimpleVector<char*> namedExilePhrases;


static SimpleVector<char*> youRedeemPhrases;
static SimpleVector<char*> namedRedeemPhrases;


static SimpleVector<char*> youKillPhrases;
static SimpleVector<char*> namedKillPhrases;
static SimpleVector<char*> namedAfterKillPhrases;



static int nextOrderNumber = 1;

static char *orderPhrase = NULL;


static char *eveName = NULL;


// maps extended ascii codes to true/false for characters allowed in SAY
// messages
static char allowedSayCharMap[256];

static const char *allowedSayChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! ";


static int killEmotionIndex = 2;
static int victimEmotionIndex = 2;
static int victimTerrifiedEmotionIndex = 2;

static int starvingEmotionIndex = 2;
static int satisfiedEmotionIndex = 2;


static double lastBabyPassedThresholdTime = 0;


static int recentScoreWindowForPickingEve = 10;

SimpleVector<float> recentScoresForPickingEve;
SimpleVector<char *>recentScoreEmailsForPickingEve;



static void cleanRecentScores() {
    while( recentScoresForPickingEve.size() > recentScoreWindowForPickingEve ) {
        recentScoresForPickingEve.deleteElement( 0 );
        delete [] recentScoreEmailsForPickingEve.getElementDirect( 0 );
        recentScoreEmailsForPickingEve.deleteElement( 0 );
        }
    }



static void addRecentScore( char *inEmail, float inScore ) {

    // only one score per account
    
    for( int i=0; i<recentScoreEmailsForPickingEve.size(); i++ ) {
        char *oldEmail = recentScoreEmailsForPickingEve.getElementDirect( i );
        
        if( strcmp( oldEmail, inEmail ) == 0 ) {
            delete [] oldEmail;
            recentScoreEmailsForPickingEve.deleteElement( i );
            recentScoresForPickingEve.deleteElement( i );
            break;
            }
        }
    
    
    recentScoresForPickingEve.push_back( inScore );
    recentScoreEmailsForPickingEve.push_back( stringDuplicate( inEmail ) );

    cleanRecentScores();
    }



static float getHighestRecentScore() {
    if( recentScoreEmailsForPickingEve.size() < 
        recentScoreWindowForPickingEve ) {
        // window not full
        // assume highest score seen is max
        // (this will block low-score players from sneaking through the test
        //  immediately after server restart).
        return FLT_MAX;
        }
    
    float highest = - FLT_MAX;
    for( int i=0; i<recentScoresForPickingEve.size(); i++ ) {
        float s = recentScoresForPickingEve.getElementDirect( i );
        if( s > highest ) {
            highest = s;
            }
        }
    return highest;
    }




static double eveWindowStart = 0;
static char eveWindowOver = false;


typedef struct PeaceTreaty {
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

    

static SimpleVector<PeaceTreaty> peaceTreaties;


typedef struct WarState {
        int lineageAEveID;
        int lineageBEveID;
    } WarState;

static SimpleVector<WarState> warStates;



// may be partial
static PeaceTreaty *getMatchingTreaty( int inLineageAEveID, 
                                       int inLineageBEveID ) {
    
    for( int i=0; i<peaceTreaties.size(); i++ ) {
        PeaceTreaty *p = peaceTreaties.getElement( i );
        

        if( ( p->lineageAEveID == inLineageAEveID &&
              p->lineageBEveID == inLineageBEveID )
            ||
            ( p->lineageAEveID == inLineageBEveID &&
              p->lineageBEveID == inLineageAEveID ) ) {
            // they match a treaty.
            return p;
            }
        }
    return NULL;
    }



// parial treaty returned if it's requested
static char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID,
                           PeaceTreaty **outPartialTreaty = NULL ) {
    
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
        
    if( p != NULL ) {
        
        if( !( p->dirAToB && p->dirBToA ) ) {
            // partial treaty
            if( outPartialTreaty != NULL ) {
                *outPartialTreaty = p;
                }
            return false;
            }
        return true;
        }
    return false;
    }



static char isWarState( int inLineageAEveID, int inLineageBEveID ) {
    for( int i=0; i<warStates.size(); i++ ) {
        WarState *w = warStates.getElement( i );
        
        
        if( ( w->lineageAEveID == inLineageAEveID &&
              w->lineageBEveID == inLineageBEveID )
            ||
            ( w->lineageAEveID == inLineageBEveID &&
              w->lineageBEveID == inLineageAEveID ) ) {
            
            return true;
            }
        }
    return false;
    }






void sendWarReportToAll();


void sendPeaceWarMessage( const char *inPeaceOrWar,
                          char inWar,
                          int inLineageAEveID, int inLineageBEveID );


static void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID ) {
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
    
    if( p != NULL ) {
        char peaceBefore = p->dirAToB && p->dirBToA;
        
        // maybe it has been sealed in a new direction?
        if( p->lineageAEveID == inLineageAEveID ) {
            p->dirAToB = true;
            p->dirBToABroken = false;
            }
        if( p->lineageBEveID == inLineageAEveID ) {
            p->dirBToA = true;
            p->dirBToABroken = false;
            }
        if( p->dirAToB && p->dirBToA &&
            ! peaceBefore ) {
            // new peace!

            // clear any war state
            for( int i=0; i<warStates.size(); i++ ) {
                WarState *w = warStates.getElement( i );
                
                
                if( ( w->lineageAEveID == inLineageAEveID &&
                      w->lineageBEveID == inLineageBEveID )
                    ||
                    ( w->lineageAEveID == inLineageBEveID &&
                      w->lineageBEveID == inLineageAEveID ) ) {
                    
                    warStates.deleteElement( i );
                    break;
                    }
                }

            sendPeaceWarMessage( "PEACE", 
                                 false,
                                 p->lineageAEveID, p->lineageBEveID );
            sendWarReportToAll();
            }
        }
    else {
        // else doesn't exist, create new unidirectional
        PeaceTreaty p = { inLineageAEveID, inLineageBEveID,
                          true, false,
                          false, false };
        
        peaceTreaties.push_back( p );
        }
    }



static void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID ) {
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
    
    char remove = false;
    
    char messageSent = false;
    
    if( p != NULL ) {
        if( p->dirAToB && p->dirBToA ) {
            // established
            
            // maybe it has been broken in a new direction?
            if( p->lineageAEveID == inLineageAEveID ) {
                p->dirAToBBroken = true;
                }
            if( p->lineageBEveID == inLineageAEveID ) {
                p->dirBToABroken = true;
                }
            
            if( p->dirAToBBroken && p->dirBToABroken ) {
                // fully broken
                // remove it
                remove = true;

                // new war!
                sendPeaceWarMessage( "WAR",
                                     true,
                                     p->lineageAEveID, p->lineageBEveID );
                messageSent = true;
                }
            }
        else {
            // not fully established
            // remove it 
            
            // this means if one person says PEACE and the other
            // responds with WAR, the first person's PEACE half-way treaty
            // is canceled.  Both need to say PEACE again once WAR has been
            // mentioned
            remove = true;
            }
        }

    
    if( remove || p == NULL ) {
        // no treaty exists, or it will be removed
        
        // some elder said "WAR"
        // war state created if it doesn't exist
        
        char found = false;
        for( int i=0; i<warStates.size(); i++ ) {
            WarState *w = warStates.getElement( i );
        

            if( ( w->lineageAEveID == inLineageAEveID &&
                  w->lineageBEveID == inLineageBEveID )
                ||
                ( w->lineageAEveID == inLineageBEveID &&
                  w->lineageBEveID == inLineageAEveID ) ) {
                found = true;
                break;
                }
            }
        
        if( !found ) {
            // add new one
            WarState w = { inLineageAEveID, inLineageBEveID };
            warStates.push_back( w );

            if( ! messageSent ) {
                sendPeaceWarMessage( "WAR", 
                                     true,
                                     inLineageAEveID, inLineageBEveID );
                messageSent = true;
                }
            }
        }
    
    if( messageSent ) {
        sendWarReportToAll();
        }

    if( remove ) {
        for( int i=0; i<peaceTreaties.size(); i++ ) {
            PeaceTreaty *otherP = peaceTreaties.getElement( i );
            
            if( otherP->lineageAEveID == p->lineageAEveID &&
                otherP->lineageBEveID == p->lineageBEveID ) {
                
                peaceTreaties.deleteElement( i );
                return;
                }
            }
        }
    }


typedef struct PastLifeStats {
        int lifeCount;
        int lifeTotalSeconds;
        char error;
    } PastLifeStats;

    



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


SimpleVector<FreshConnection> newConnections;

SimpleVector<FreshConnection> waitingForTwinConnections;



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



SimpleVector<LiveObject> players;
SimpleVector<LiveObject> tutorialLoadingPlayers;


int getNumPlayers() {
    return players.size();
    }



char doesEveLineExist( int inEveID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ( ! o->error ) && o->lineageEveID == inEveID ) {
            return true;
            }
        }
    return false;
    }



double computeAge( LiveObject *inPlayer );



// false for male, true for female
char getFemale( LiveObject *inPlayer ) {
    ObjectRecord *r = getObject( inPlayer->displayID );
    
    return ! r->male;
    }



// find most fit offspring
static LiveObject *findFittestOffspring( int inPlayerID, int inSkipID ) {
    LiveObject *fittestOffspring = NULL;
    double fittestOffspringFitness = 0;

    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        
        if( ! otherPlayer->error &&
            otherPlayer->id != inPlayerID &&
            otherPlayer->id != inSkipID ) {
            
            if( otherPlayer->fitnessScore > fittestOffspringFitness ) {
                
                if( otherPlayer->lineage->getElementIndex( inPlayerID ) 
                    != -1 ) {
                    
                    // player is direct offspring of inPlayer
                    // (child, grandchild, etc).
                    fittestOffspring = otherPlayer;
                    fittestOffspringFitness = otherPlayer->fitnessScore;
                    }
                }
            }
        }
    
    return fittestOffspring;
    }
    


static LiveObject *findHeir( LiveObject *inPlayer ) {
    LiveObject *offspring = NULL;
    
    if( getFemale( inPlayer ) ) {
        offspring = findFittestOffspring( inPlayer->id, inPlayer->id );
        }
    
    if( offspring == NULL ) {
        // no direct offspring found
        
        // walk up through lineage and find oldest close relative
        // oldest person who shares our mother
        // oldest person who shares our gma
        // oldest person who shares our ggma
        
        // start with ma
        int lineageStep = 0;
        
        while( offspring == NULL &&
               lineageStep < inPlayer->lineage->size() ) {
            
            offspring = findFittestOffspring( 
                inPlayer->lineage->getElementDirect( lineageStep ),
                inPlayer->id );
            
            lineageStep++;
            }
        }

    return offspring;
    }





typedef struct DeadObject {
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



static double lastPastPlayerFlushTime = 0;

SimpleVector<DeadObject> pastPlayers;



static void addPastPlayer( LiveObject *inPlayer ) {
    
    DeadObject o;
    
    o.id = inPlayer->id;
    o.displayID = inPlayer->displayID;
    o.name = NULL;
    if( inPlayer->name != NULL ) {
        o.name = stringDuplicate( inPlayer->name );
        }
    o.lineageEveID = inPlayer->lineageEveID;
    o.lifeStartTimeSeconds = inPlayer->lifeStartTimeSeconds;
    o.deathTimeSeconds = inPlayer->deathTimeSeconds;
    
    o.lineage = new SimpleVector<int>();
    for( int i=0; i< inPlayer->lineage->size(); i++ ) {
        o.lineage->push_back( inPlayer->lineage->getElementDirect( i ) );
        }
    
    pastPlayers.push_back( o );
    }



char isOwned( LiveObject *inPlayer, int inX, int inY ) {
    for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
        GridPos *p = inPlayer->ownedPositions.getElement( i );
        
        if( p->x == inX && p->y == inY ) {
            return true;
            }
        }
    return false;
    }



char isOwned( LiveObject *inPlayer, GridPos inPos ) {
    return isOwned( inPlayer, inPos.x, inPos.y );
    }


char isExiled( LiveObject *inViewer, LiveObject *inTarget );
LiveObject *getLiveObject( int inID );



static char isMapSpotLeaderOwned( LiveObject *inPlayer, int inX, int inY ) {    
    // walk up leadership chain for inPlayer and see if anyone owns it
        
    LiveObject *nextToCheck = inPlayer;
    
    while( nextToCheck != NULL ) {
        
        if( isOwned( nextToCheck, inX, inY ) &&
            ! isExiled( nextToCheck, inPlayer ) ) {
            // found a leader above them who owns it
            return true;
            }
            
        int nextID = nextToCheck->followingID;
        
        if( nextID == -1 ) {
            nextToCheck = NULL;
            }
        else {
            nextToCheck = getLiveObject( nextID );
            }
        }
    // found no leader above us who owns it and doesn't see
    // us as exiled
    // we're no ally of owner
    return false;
    }



char isOwnedOrAllyOwned( LiveObject *inPlayer, ObjectRecord *inObject,
                         int inX, int inY ) {
    if( isOwned( inPlayer, inX, inY ) ) {
        return true;
        }
    // do followers have access to this object?  If so, look for a leader above
    // who owns it
    else if( inObject->isFollowerOwned && 
             isMapSpotLeaderOwned( inPlayer, inX, inY ) ) {
        return true;
        }
    return false;
    }



char isKnownOwned( LiveObject *inPlayer, int inX, int inY ) {
    for( int i=0; i<inPlayer->knownOwnedPositions.size(); i++ ) {
        GridPos *p = inPlayer->knownOwnedPositions.getElement( i );
        
        if( p->x == inX && p->y == inY ) {
            return true;
            }
        }
    return false;
    }



char isKnownOwned( LiveObject *inPlayer, GridPos inPos ) {
    return isKnownOwned( inPlayer, inPos.x, inPos.y );
    }


// messages with no follow-up hang out on client for 10 seconds
// 7 seconds should be long enough to read if there's a follow-up waiting
static double minGlobalMessageSpacingSeconds = 7;


void sendGlobalMessage( char *inMessage,
                        LiveObject *inOnePlayerOnly = NULL );


void sendMessageToPlayer( LiveObject *inPlayer, 
                          char *inMessage, int inLength );



static void endOwnership( int inX, int inY, int inObjectID ) {
    SimpleVector<int> *deathMarkers = getAllPossibleDeathIDs();
    for( int j=0; j<deathMarkers->size(); j++ ) {
        int deathID = deathMarkers->getElementDirect( j );
        TransRecord *t = getTrans( deathID, inObjectID );
        
        if( t != NULL ) {
                    
            setMapObject( inX, inY, t->newTarget );
            break;
            }
        }
    }




SimpleVector<GridPos> newOwnerPos;

SimpleVector<GridPos> recentlyRemovedOwnerPos;


void removeAllOwnership( LiveObject *inPlayer, char inProcessInherit = true ) {
    double startTime = Time::getCurrentTime();
    int num = inPlayer->ownedPositions.size();
    
    SimpleVector<LiveObject*> heirList;
    

    for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
        GridPos *p = inPlayer->ownedPositions.getElement( i );

        recentlyRemovedOwnerPos.push_back( *p );
        
        int oID = getMapObject( p->x, p->y );

        if( oID <= 0 ) {
            continue;
            }

        char noOtherOwners = true;
        
        for( int j=0; j<players.size(); j++ ) {
            LiveObject *otherPlayer = players.getElement( j );
            
            if( otherPlayer != inPlayer ) {
                if( isOwned( otherPlayer, *p ) ) {
                    noOtherOwners = false;
                    break;
                    }
                }
            }

        
        if( noOtherOwners && inProcessInherit ) {
            // find closest relative
            
            LiveObject *heir = findHeir( inPlayer );
            
            if( heir != NULL ) {
                heir->ownedPositions.push_back( *p );
                newOwnerPos.push_back( *p );
                    
                noOtherOwners = false;
                
                // only send them message about first piece of property
                // they inherit from this death (don't overwhelm them with
                // lots of messages)
                if( heirList.getElementIndex( heir ) == -1 ) {
                    heirList.push_back( heir );                    

                    const char *name = "SOMEONE";
                
                    if( inPlayer->name != NULL ) {
                        name = inPlayer->name;
                        }
                    
                    char *message = 
                        autoSprintf( "%s JUST DIED.**"
                                     "YOU INHERITED THEIR PROPERTY.",
                                     name);
                    
                    sendGlobalMessage( message, heir );
                    delete [] message;
                    
                    // send them a map pointer too
                    message = autoSprintf( "PS\n"
                                           "%d/0 MY INHERITED PROPERTY "
                                           "*prop %d *map %d %d\n#",
                                           heir->id,
                                           0,
                                           p->x - heir->birthPos.x,
                                           p->y - heir->birthPos.y );
                    sendMessageToPlayer( heir, message, strlen( message ) );
                    delete [] message;
                    }
                }
            }
        

        
        if( noOtherOwners ) {
            // last owner of p just died
            // force end transition

            endOwnership( p->x, p->y, oID );
            }
        }
    
    inPlayer->ownedPositions.deleteAll();

    AppLog::infoF( "Removing all ownership (%d owned) for "
                   "player %d (%s) took %lf sec",
                   num, inPlayer->id, inPlayer->email, 
                   Time::getCurrentTime() - startTime );
    }



char *getOwnershipString( int inX, int inY ) {    
    char foundAny = false;
    
    SimpleVector<char> messageWorking;
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            isOwned( otherPlayer, inX, inY ) ) {
            char *playerIDString = 
                autoSprintf( " %d", otherPlayer->id );
            messageWorking.appendElementString( 
                playerIDString );
            delete [] playerIDString;
            foundAny = true;
            }
        }
    
    if( ! foundAny ) {
        // an orphaned owned object?
        // this should never happen, but in case it ever does, clear
        // it when we discover it.
        int oID = getMapObject( inX, inY );
        if( oID > 0 ) {
            endOwnership( inX, inY, oID );
            }
        }

    char *message = messageWorking.getElementString();
    return message;
    }


char *getOwnershipString( GridPos inPos ) {
    return getOwnershipString( inPos.x, inPos.y );
    }





// returns a person to their natural state
static void backToBasics( LiveObject *inPlayer ) {
    LiveObject *p = inPlayer;

    // do not heal dying people
    if( ! p->holdingWound && p->holdingID > 0 ) {
        
        p->holdingID = 0;
        
        p->holdingEtaDecay = 0;
        
        p->heldOriginValid = false;
        p->heldTransitionSourceID = -1;
        
        
        p->numContained = 0;
        if( p->containedIDs != NULL ) {
            delete [] p->containedIDs;
            delete [] p->containedEtaDecays;
            p->containedIDs = NULL;
        p->containedEtaDecays = NULL;
            }
        
        if( p->subContainedIDs != NULL ) {
            delete [] p->subContainedIDs;
            delete [] p->subContainedEtaDecays;
            p->subContainedIDs = NULL;
            p->subContainedEtaDecays = NULL;
            }
        }
        
        
    p->clothing = getEmptyClothingSet();
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        p->clothingEtaDecay[c] = 0;
        p->clothingContained[c].deleteAll();
        p->clothingContainedEtaDecays[c].deleteAll();
        }

    p->emotFrozen = false;
    p->emotUnfreezeETA = 0;

    p->learnedTools.deleteAll();
    p->partiallyLearnedTools.deleteAll();
    }




typedef struct GraveInfo {
        GridPos pos;
        int playerID;
        // eve that started the line of this dead person
        // used for tracking whether grave is part of player's family or not
        int lineageEveID;
    } GraveInfo;


typedef struct GraveMoveInfo {
        GridPos posStart;
        GridPos posEnd;
        int swapDest;
    } GraveMoveInfo;




// tracking spots on map that inflicted a mortal wound
// put them on timeout afterward so that they don't attack
// again immediately
typedef struct DeadlyMapSpot {
        GridPos pos;
        double timeOfAttack;
    } DeadlyMapSpot;


static double deadlyMapSpotTimeoutSec = 10;

static SimpleVector<DeadlyMapSpot> deadlyMapSpots;


static char wasRecentlyDeadly( GridPos inPos ) {
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<deadlyMapSpots.size(); i++ ) {
        
        DeadlyMapSpot *s = deadlyMapSpots.getElement( i );
        
        if( curTime - s->timeOfAttack >= deadlyMapSpotTimeoutSec ) {
            deadlyMapSpots.deleteElement( i );
            i--;
            }
        else if( s->pos.x == inPos.x && s->pos.y == inPos.y ) {
            // note that this is a lazy method that only walks through
            // the whole list and checks for timeouts when
            // inPos isn't found
            return true;
            }
        }
    return false;
    }



static void addDeadlyMapSpot( GridPos inPos ) {
    // don't check for duplicates
    // we're only called to add a new deadly spot when the spot isn't
    // currently on deadly cooldown anyway
    DeadlyMapSpot s = { inPos, Time::getCurrentTime() };
    deadlyMapSpots.push_back( s );
    }




LiveObject *getLiveObject( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    
    return NULL;
    }


char *getPlayerName( int inID ) {
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return o->name;
        }
    return NULL;
    }



int getPlayerLineage( int inID ) {
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return o->lineageEveID;
        }
    return -1;
    }



char isPlayerIgnoredForEvePlacement( int inID ) {
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return ( o->curseStatus.curseLevel > 0 ) || o->isTutorial;
        }

    // player id doesn't even exist
    return true;
    }




static double pickBirthCooldownSeconds() {
    // Kumaraswamy distribution
    // PDF:
    // k(x,a,b) = a * b * x**( a - 1 ) * (1-x**a)**(b-1)
    // CDF:
    // kCDF(x,a,b) = 1 - (1-x**a)**b
    // Invers CDF:
    // kCDFInv(y,a,b) = ( 1 - (1-y)**(1.0/b) )**(1.0/a)

    // For b=1, PDF curve starts at 0 and curves upward, for all a > 2
    // good values seem to be a=1.5, b=1

    // actually, make it more bell-curve like, with a=2, b=3

    double a = 2;
    double b = 3;
    
    // mean is around 2 minutes
    

    // uniform
    double u = randSource.getRandomDouble();
    
    // feed into inverted CDF to sample a value from the distribution
    double v = pow( 1 - pow( 1-u, (1/b) ), 1/a );
    
    // v is in [0..1], the value range of Kumaraswamy

    // put max at 5 minutes
    return v * 5 * 60;
    }




typedef struct FullMapContained{ 
        int numContained;
        int *containedIDs;
        timeSec_t *containedEtaDecays;
        SimpleVector<int> *subContainedIDs;
        SimpleVector<timeSec_t> *subContainedEtaDecays;
    } FullMapContained;



// including contained and sub contained in one call
FullMapContained getFullMapContained( int inX, int inY ) {
    FullMapContained r;
    
    r.containedIDs = getContained( inX, inY, &( r.numContained ) );
    r.containedEtaDecays = 
        getContainedEtaDecay( inX, inY, &( r.numContained ) );
    
    if( r.numContained == 0 ) {
        r.subContainedIDs = NULL;
        r.subContainedEtaDecays = NULL;
        }
    else {
        r.subContainedIDs = new SimpleVector<int>[ r.numContained ];
        r.subContainedEtaDecays = new SimpleVector<timeSec_t>[ r.numContained ];
        }
    
    for( int c=0; c< r.numContained; c++ ) {
        if( r.containedIDs[c] < 0 ) {
            
            int numSub;
            int *subContainedIDs = getContained( inX, inY, &numSub,
                                                 c + 1 );
            
            if( subContainedIDs != NULL ) {
                
                r.subContainedIDs[c].appendArray( subContainedIDs, numSub );
                delete [] subContainedIDs;
                }
            
            timeSec_t *subContainedEtaDecays = 
                getContainedEtaDecay( inX, inY, &numSub,
                                      c + 1 );

            if( subContainedEtaDecays != NULL ) {
                
                r.subContainedEtaDecays[c].appendArray( subContainedEtaDecays, 
                                                        numSub );
                delete [] subContainedEtaDecays;
                }
            }
        }
    
    return r;
    }



void freePlayerContainedArrays( LiveObject *inPlayer ) {
    if( inPlayer->containedIDs != NULL ) {
        delete [] inPlayer->containedIDs;
        }
    if( inPlayer->containedEtaDecays != NULL ) {
        delete [] inPlayer->containedEtaDecays;
        }
    if( inPlayer->subContainedIDs != NULL ) {
        delete [] inPlayer->subContainedIDs;
        }
    if( inPlayer->subContainedEtaDecays != NULL ) {
        delete [] inPlayer->subContainedEtaDecays;
        }

    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }



void setContained( LiveObject *inPlayer, FullMapContained inContained ) {
    
    inPlayer->numContained = inContained.numContained;
     
    freePlayerContainedArrays( inPlayer );
    
    inPlayer->containedIDs = inContained.containedIDs;
    
    inPlayer->containedEtaDecays =
        inContained.containedEtaDecays;
    
    inPlayer->subContainedIDs =
        inContained.subContainedIDs;
    inPlayer->subContainedEtaDecays =
        inContained.subContainedEtaDecays;
    }
    
    
    
    
void clearPlayerHeldContained( LiveObject *inPlayer ) {
    inPlayer->numContained = 0;
    
    delete [] inPlayer->containedIDs;
    delete [] inPlayer->containedEtaDecays;
    delete [] inPlayer->subContainedIDs;
    delete [] inPlayer->subContainedEtaDecays;
    
    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }
    



void transferHeldContainedToMap( LiveObject *inPlayer, int inX, int inY ) {
    if( inPlayer->numContained != 0 ) {
        timeSec_t curTime = Time::timeSec();
        float stretch = 
            getObject( inPlayer->holdingID )->slotTimeStretch;
        
        for( int c=0;
             c < inPlayer->numContained;
             c++ ) {
            
            // undo decay stretch before adding
            // (stretch applied by adding)
            if( stretch != 1.0 &&
                inPlayer->containedEtaDecays[c] != 0 ) {
                
                timeSec_t offset = 
                    inPlayer->containedEtaDecays[c] - curTime;
                
                offset = offset * stretch;
                
                inPlayer->containedEtaDecays[c] =
                    curTime + offset;
                }

            addContained( 
                inX, inY,
                inPlayer->containedIDs[c],
                inPlayer->containedEtaDecays[c] );

            int numSub = inPlayer->subContainedIDs[c].size();
            if( numSub > 0 ) {

                int container = inPlayer->containedIDs[c];
                
                if( container < 0 ) {
                    container *= -1;
                    }
                
                float subStretch = getObject( container )->slotTimeStretch;
                    
                
                int *subIDs = 
                    inPlayer->subContainedIDs[c].getElementArray();
                timeSec_t *subDecays = 
                    inPlayer->subContainedEtaDecays[c].
                    getElementArray();
                
                for( int s=0; s < numSub; s++ ) {
                    
                    // undo decay stretch before adding
                    // (stretch applied by adding)
                    if( subStretch != 1.0 &&
                        subDecays[s] != 0 ) {
                
                        timeSec_t offset = subDecays[s] - curTime;
                        
                        offset = offset * subStretch;
                        
                        subDecays[s] = curTime + offset;
                        }

                    addContained( inX, inY,
                                  subIDs[s], subDecays[s],
                                  c + 1 );
                    }
                delete [] subIDs;
                delete [] subDecays;
                }
            }

        clearPlayerHeldContained( inPlayer );
        }
    }





// diags are square root of 2 in length
static double diagLength = 1.41421356237;
    


// diagonal steps are longer
static double measurePathLength( int inXS, int inYS, 
                                 GridPos *inPathPos, int inPathLength ) {

    double totalLength = 0;
    
    GridPos lastPos = { inXS, inYS };
    
    for( int i=0; i<inPathLength; i++ ) {
        
        GridPos thisPos = inPathPos[i];
        
        if( thisPos.x != lastPos.x &&
            thisPos.y != lastPos.y ) {
            totalLength += diagLength;
            }
        else {
            // not diag
            totalLength += 1;
            }
        lastPos = thisPos;
        }
    
    return totalLength;
    }




static double getPathSpeedModifier( GridPos *inPathPos, int inPathLength ) {
    
    if( inPathLength < 1 ) {
        return 1;
        }
    

    int floor = getMapFloor( inPathPos[0].x, inPathPos[0].y );

    if( floor == 0 ) {
        return 1;
        }

    double speedMult = getObject( floor )->speedMult;
    
    if( speedMult == 1 ) {
        return 1;
        }
    

    // else we have a speed mult for at least first step in path
    // see if we have same floor for rest of path

    for( int i=1; i<inPathLength; i++ ) {
        
        int thisFloor = getMapFloor( inPathPos[i].x, inPathPos[i].y );
        
        if( ! sameRoadClass( thisFloor, floor ) ) {
            // not same floor whole way
            return 1;
            }
        }
    // same floor whole way
    printf( "Speed modifier = %f\n", speedMult );
    return speedMult;
    }



static int getLiveObjectIndex( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return i;
            }
        }

    return -1;
    }





int nextID = 2;


static void deleteMembers( FreshConnection *inConnection ) {
    delete inConnection->sock;
    delete inConnection->sockBuffer;
    
    if( inConnection->sequenceNumberString != NULL ) {    
        delete [] inConnection->sequenceNumberString;
        }
    
    if( inConnection->ticketServerRequest != NULL ) {
        delete inConnection->ticketServerRequest;
        }
    
    if( inConnection->email != NULL ) {
        delete [] inConnection->email;
        }
    
    if( inConnection->twinCode != NULL ) {
        delete [] inConnection->twinCode;
        }

    if( inConnection->clientTag != NULL ) {
        delete [] inConnection->clientTag;
        }
    }



static SimpleVector<char *> curseWords;

static char *curseSecret = NULL;


static SimpleVector<char *> familyNamesAfterEveWindow;
static SimpleVector<int> familyLineageEveIDsAfterEveWindow;
static SimpleVector<int> familyCountsAfterEveWindow;

static int nextBabyFamilyIndex = 0;


static FILE *postWindowFamilyLogFile = NULL;


// keep track of players whose last life was short
static double shortLifeAge = 10;

static SimpleVector<char*> shortLifeEmails;


// checks for presence of inEmail in short life list, and deletes it from list
// if present
static char isShortLife( char *inEmail ) {
    
    char hit = false;
    for( int i=0; i<shortLifeEmails.size(); i++ ) {
        if( strcmp( shortLifeEmails.getElementDirect( i ), inEmail ) == 0 ) {
            hit = true;
            delete [] shortLifeEmails.getElementDirect( i );
            shortLifeEmails.deleteElement( i );
            break;
            }
        }
    

    if( shortLifeEmails.size() > 1000 ) {
        // don't let it keep growing
        // remember only last 1000 unique short-life players.
        // forget rest
        int extra = shortLifeEmails.size() - 1000;
        for( int i=0; i<extra; i++ ) {
            delete [] shortLifeEmails.getElementDirect( i );
            }
        shortLifeEmails.deleteStartElements( extra );
        }
    
    return hit;
    }



// destroyed by caller
static void addShortLife( char *inEmail ) {
    // remove if present
    isShortLife( inEmail );

    shortLifeEmails.push_back( stringDuplicate( inEmail ) );
    }




void quitCleanup() {
    AppLog::info( "Cleaning up on quit..." );

    // FreshConnections are in two different lists
    // free structures from both
    SimpleVector<FreshConnection> *connectionLists[2] =
        { &newConnections, &waitingForTwinConnections };

    for( int c=0; c<2; c++ ) {
        SimpleVector<FreshConnection> *list = connectionLists[c];
        
        for( int i=0; i<list->size(); i++ ) {
            FreshConnection *nextConnection = list->getElement( i );
            deleteMembers( nextConnection );
            }
        list->deleteAll();
        }
    
    // add these to players to clean them up togeter
    for( int i=0; i<tutorialLoadingPlayers.size(); i++ ) {
        LiveObject nextPlayer = tutorialLoadingPlayers.getElementDirect( i );
        players.push_back( nextPlayer );
        }
    tutorialLoadingPlayers.deleteAll();
    


    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        
        removeAllOwnership( nextPlayer, false );

        if( nextPlayer->sock != NULL ) {
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
        
        if( nextPlayer->email != NULL  ) {
            delete [] nextPlayer->email;
            }
        if( nextPlayer->origEmail != NULL  ) {
            delete [] nextPlayer->origEmail;
            }
        if( nextPlayer->lastBabyEmail != NULL  ) {
            delete [] nextPlayer->lastBabyEmail;
            }
        if( nextPlayer->lastSidsBabyEmail != NULL ) {
            delete [] nextPlayer->lastSidsBabyEmail;
            }

        if( nextPlayer->murderPerpEmail != NULL  ) {
            delete [] nextPlayer->murderPerpEmail;
            }

        nextPlayer->globalMessageQueue.deallocateStringElements();
        
        
        freePlayerContainedArrays( nextPlayer );
        
        
        if( nextPlayer->pathToDest != NULL ) {
            delete [] nextPlayer->pathToDest;
            }
        
        if( nextPlayer->deathReason != NULL ) {
            delete [] nextPlayer->deathReason;
            }


        delete nextPlayer->babyBirthTimes;
        delete nextPlayer->babyIDs;        
        }
    players.deleteAll();


    for( int i=0; i<pastPlayers.size(); i++ ) {
        DeadObject *o = pastPlayers.getElement( i );
        
        delete [] o->name;
        delete o->lineage;
        }
    pastPlayers.deleteAll();
    

    freeLineageLimit();
    
    freePlayerStats();
    freeLineageLog();
    
    freeNames();
    
    freeCurses();
    
    freeCurseDB();

    freeLifeTokens();

    freeFitnessScore();

    freeLifeLog();
    
    freeFoodLog();
    freeFailureLog();
    
    freeObjectSurvey();
    
    freeLanguage();
    freeFamilySkipList();

    freeTriggers();

    freeSpecialBiomes();
    
    freeOffspringTracker();
    

    freeMap();

    freeTransBank();
    freeCategoryBank();
    freeObjectBank();
    freeAnimationBank();
    
    freeArcReport();
    

    if( clientPassword != NULL ) {
        delete [] clientPassword;
        clientPassword = NULL;
        }
    

    if( ticketServerURL != NULL ) {
        delete [] ticketServerURL;
        ticketServerURL = NULL;
        }

    if( reflectorURL != NULL ) {
        delete [] reflectorURL;
        reflectorURL = NULL;
        }

    nameGivingPhrases.deallocateStringElements();
    familyNameGivingPhrases.deallocateStringElements();
    eveNameGivingPhrases.deallocateStringElements();
    cursingPhrases.deallocateStringElements();
    
    forgivingPhrases.deallocateStringElements();
    youForgivingPhrases.deallocateStringElements();
    
    youGivingPhrases.deallocateStringElements();
    namedGivingPhrases.deallocateStringElements();
    
    familyGivingPhrases.deallocateStringElements();
    offspringGivingPhrases.deallocateStringElements();
    
    posseJoiningPhrases.deallocateStringElements();
    
    youFollowPhrases.deallocateStringElements();
    namedFollowPhrases.deallocateStringElements();
    
    youExilePhrases.deallocateStringElements();
    namedExilePhrases.deallocateStringElements();

    youRedeemPhrases.deallocateStringElements();
    namedRedeemPhrases.deallocateStringElements();


    youKillPhrases.deallocateStringElements();
    namedKillPhrases.deallocateStringElements();
    namedAfterKillPhrases.deallocateStringElements();
    
    if( orderPhrase != NULL ) {
        delete [] orderPhrase;
        orderPhrase = NULL;
        }


    if( curseYouPhrase != NULL ) {
        delete [] curseYouPhrase;
        curseYouPhrase = NULL;
        }
    if( curseBabyPhrase != NULL ) {
        delete [] curseBabyPhrase;
        curseBabyPhrase = NULL;
        }
    

    if( eveName != NULL ) {
        delete [] eveName;
        eveName = NULL;
        }

    if( apocalypseRequest != NULL ) {
        delete apocalypseRequest;
        apocalypseRequest = NULL;
        }

    if( familyDataLogFile != NULL ) {
        fclose( familyDataLogFile );
        familyDataLogFile = NULL;
        }

    familyNamesAfterEveWindow.deallocateStringElements();
    familyLineageEveIDsAfterEveWindow.deleteAll();
    familyCountsAfterEveWindow.deleteAll();
    nextBabyFamilyIndex = 0;
    
    if( postWindowFamilyLogFile != NULL ) {
        fclose( postWindowFamilyLogFile );
        postWindowFamilyLogFile = NULL;
        }

    curseWords.deallocateStringElements();
    
    if( curseSecret != NULL ) {
        delete [] curseSecret;
        curseSecret = NULL;
        }


    recentScoreEmailsForPickingEve.deallocateStringElements();
    recentScoresForPickingEve.deleteAll();

    shortLifeEmails.deallocateStringElements();
    }



static double minPosseFraction = 0.5;
static int minPosseCap = 3;
static double possePopulationRadius = 30;



#include "minorGems/util/crc32.h"

JenkinsRandomSource curseSource;


static int cursesUseSenderEmail = 0;

static int useCurseWords = 1;


// result NOT destroyed by caller
static const char *getCurseWord( char *inSenderEmail,
                                 char *inEmail, int inWordIndex ) {
    if( ! useCurseWords || curseWords.size() == 0 ) {
        return "X";
        }

    if( curseSecret == NULL ) {
        curseSecret = 
            SettingsManager::getStringSetting( 
                "statsServerSharedSecret", "sdfmlk3490sadfm3ug9324" );
        }
    
    char *emailPlusSecret;

    if( cursesUseSenderEmail ) {
        emailPlusSecret =
            autoSprintf( "%s_%s_%s", inSenderEmail, inEmail, curseSecret );
        }
    else {
        emailPlusSecret = 
            autoSprintf( "%s_%s", inEmail, curseSecret );
        }
    
    unsigned int c = crc32( (unsigned char*)emailPlusSecret, 
                            strlen( emailPlusSecret ) );
    
    delete [] emailPlusSecret;

    curseSource.reseed( c );
    
    // mix based on index
    for( int i=0; i<inWordIndex; i++ ) {
        curseSource.getRandomDouble();
        }

    int index = curseSource.getRandomBoundedInt( 0, curseWords.size() - 1 );
    
    return curseWords.getElementDirect( index );
    }




volatile char quit = false;

void intHandler( int inUnused ) {
    AppLog::info( "Quit received for unix" );
    
    // since we handled this singal, we will return to normal execution
    quit = true;
    }


#ifdef WIN_32
#include <windows.h>
BOOL WINAPI ctrlHandler( DWORD dwCtrlType ) {
    if( CTRL_C_EVENT == dwCtrlType ) {
        AppLog::info( "Quit received for windows" );
        
        // will auto-quit as soon as we return from this handler
        // so cleanup now
        //quitCleanup();
        
        // seems to handle CTRL-C properly if launched by double-click
        // or batch file
        // (just not if launched directly from command line)
        quit = true;
        }
    return true;
    }
#endif


int numConnections = 0;







// reads all waiting data from socket and stores it in buffer
// returns true if socket still good, false on error
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer ) {

    char buffer[512];
    
    int numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
    
    if( numRead == -1 ) {

        if( ! inSock->isSocketInFDRange() ) {
            // the internal FD of this socket is out of range
            // probably some kind of heap corruption.

            // save a bug report
            int allow = 
                SettingsManager::getIntSetting( "allowBugReports", 0 );

            if( allow ) {
                char *bugName = 
                    autoSprintf( "bug_socket_%f", Time::getCurrentTime() );
                
                char *bugOutName = autoSprintf( "%s_out.txt", bugName );
                
                File outFile( NULL, "serverOut.txt" );
                if( outFile.exists() ) {
                    fflush( stdout );
                    File outCopyFile( NULL, bugOutName );
                    
                    outFile.copy( &outCopyFile );
                    }
                delete [] bugName;
                delete [] bugOutName;
                }
            }
        
            
        return false;
        }
    
    while( numRead > 0 ) {
        inBuffer->appendArray( buffer, numRead );

        numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
        }

    return true;
    }



// NULL if there's no full message available
char *getNextClientMessage( SimpleVector<char> *inBuffer ) {
    // find first terminal character #

    int index = inBuffer->getElementIndex( '#' );
        
    if( index == -1 ) {

        if( inBuffer->size() > 200 ) {
            // 200 characters with no message terminator?
            // client is sending us nonsense
            // cut it off here to avoid buffer overflow
            
            AppLog::info( "More than 200 characters in client receive buffer "
                          "with no messsage terminator present, "
                          "generating NONSENSE message." );
            
            return stringDuplicate( "NONSENSE 0 0" );
            }

        return NULL;
        }
    
    if( index > 1 && 
        inBuffer->getElementDirect( 0 ) == 'K' &&
        inBuffer->getElementDirect( 1 ) == 'A' ) {
        
        // a KA (keep alive) message
        // short-cicuit the processing here
        
        inBuffer->deleteStartElements( index + 1 );
        return NULL;
        }
    
        

    char *message = new char[ index + 1 ];
    
    // all but terminal character
    for( int i=0; i<index; i++ ) {
        message[i] = inBuffer->getElementDirect( i );
        }
    
    // delete from buffer, including terminal character
    inBuffer->deleteStartElements( index + 1 );
    
    message[ index ] = '\0';
    
    return message;
    }





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




typedef struct ClientMessage {
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


static int pathDeltaMax = 16;



static int stringToInt( char *inString ) {
    return strtol( inString, NULL, 10 );
    }



// if extraPos present in result, destroyed by caller
// inMessage may be modified by this call
ClientMessage parseMessage( LiveObject *inPlayer, char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.i = -1;
    m.c = -1;
    m.id = -1;
    m.trigger = -1;
    m.numExtraPos = 0;
    m.extraPos = NULL;
    m.saidText = NULL;
    m.bugText = NULL;
    m.sequenceNumber = -1;
    
    // don't require # terminator here
    
    
    //int numRead = sscanf( inMessage, 
    //                      "%99s %d %d", nameBuffer, &( m.x ), &( m.y ) );
    

    // profiler finds sscanf as a hotspot
    // try a custom bit of code instead
    
    int numRead = 0;
    
    int parseLen = strlen( inMessage );
    if( parseLen > 99 ) {
        parseLen = 99;
        }
    
    for( int i=0; i<parseLen; i++ ) {
        if( inMessage[i] == ' ' ) {
            switch( numRead ) {
                case 0:
                    if( i != 0 ) {
                        memcpy( nameBuffer, inMessage, i );
                        nameBuffer[i] = '\0';
                        numRead++;
                        // rewind back to read the space again
                        // before the first number
                        i--;
                        }
                    break;
                case 1:
                    m.x = stringToInt( &( inMessage[i] ) );
                    numRead++;
                    break;
                case 2:
                    m.y = stringToInt( &( inMessage[i] ) );
                    numRead++;
                    break;
                }
            if( numRead == 3 ) {
                break;
                }
            }
        }
    

    
    if( numRead >= 2 &&
        strcmp( nameBuffer, "BUG" ) == 0 ) {
        m.type = BUG;
        m.bug = m.x;
        m.bugText = stringDuplicate( inMessage );
        return m;
        }


    if( numRead != 3 ) {
        
        if( numRead == 2 &&
            strcmp( nameBuffer, "TRIGGER" ) == 0 ) {
            m.type = TRIGGER;
            m.trigger = m.x;
            }
        else {
            m.type = UNKNOWN;
            }
        
        return m;
        }
    

    if( strcmp( nameBuffer, "MOVE" ) == 0) {
        m.type = MOVE;
        
        char *atPos = strstr( inMessage, "@" );
        
        int offset = 3;
        
        if( atPos != NULL ) {
            offset = 4;            
            }
        

        // in place, so we don't need to deallocate them
        SimpleVector<char *> *tokens =
            tokenizeStringInPlace( inMessage );
        
        // require an even number of extra coords beyond offset
        if( tokens->size() < offset + 2 || 
            ( tokens->size() - offset ) %2 != 0 ) {
            
            delete tokens;
            
            m.type = UNKNOWN;
            return m;
            }
        
        if( atPos != NULL ) {
            // skip @ symbol in token and parse int
            m.sequenceNumber = 
                stringToInt( &( tokens->getElementDirect( 3 )[1] ) );
            }

        int numTokens = tokens->size();
        
        m.numExtraPos = (numTokens - offset) / 2;
        
        m.extraPos = new GridPos[ m.numExtraPos ];

        for( int e=0; e<m.numExtraPos; e++ ) {
            
            char *xToken = tokens->getElementDirect( offset + e * 2 );
            char *yToken = tokens->getElementDirect( offset + e * 2 + 1 );
            
            // profiler found sscanf is a bottleneck here
            // try atoi (stringToInt) instead
            //sscanf( xToken, "%d", &( m.extraPos[e].x ) );
            //sscanf( yToken, "%d", &( m.extraPos[e].y ) );

            m.extraPos[e].x = stringToInt( xToken );
            m.extraPos[e].y = stringToInt( yToken );
            
            
            if( abs( m.extraPos[e].x ) > pathDeltaMax ||
                abs( m.extraPos[e].y ) > pathDeltaMax ) {
                // path goes too far afield
                
                // terminate it here
                m.numExtraPos = e;
                
                if( e == 0 ) {
                    delete [] m.extraPos;
                    m.extraPos = NULL;
                    m.numExtraPos = 0;
                    m.type = UNKNOWN;
                    delete tokens;
                    return m;
                    }
                break;
                }
                

            // make them absolute
            m.extraPos[e].x += m.x;
            m.extraPos[e].y += m.y;
            }
        
        delete tokens;
        }
    else if( strcmp( nameBuffer, "JUMP" ) == 0 ) {
        m.type = JUMP;
        }
    else if( strcmp( nameBuffer, "DIE" ) == 0 ) {
        m.type = DIE;
        }
    else if( strcmp( nameBuffer, "GRAVE" ) == 0 ) {
        m.type = GRAVE;
        }
    else if( strcmp( nameBuffer, "OWNER" ) == 0 ) {
        m.type = OWNER;
        }
    else if( strcmp( nameBuffer, "FORCE" ) == 0 ) {
        m.type = FORCE;
        }
    else if( strcmp( nameBuffer, "USE" ) == 0 ) {
        m.type = USE;
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ), &( m.i ) );
        
        if( numRead < 5 ) {
            m.i = -1;
            }
        if( numRead < 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "SELF" ) == 0 ) {
        m.type = SELF;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "UBABY" ) == 0 ) {
        m.type = UBABY;

        // id param optional
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ), &( m.id ) );
        
        if( numRead != 4 && numRead != 5 ) {
            m.type = UNKNOWN;
            }
        if( numRead != 5 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "BABY" ) == 0 ) {
        m.type = BABY;
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "PING" ) == 0 ) {
        m.type = PING;
        // read unique id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = 0;
            }
        }
    else if( strcmp( nameBuffer, "SREMV" ) == 0 ) {
        m.type = SREMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ),
                          &( m.i ) );
        
        if( numRead != 5 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "REMV" ) == 0 ) {
        m.type = REMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "DROP" ) == 0 ) {
        m.type = DROP;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "KILL" ) == 0 ) {
        m.type = KILL;
        
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "MAP" ) == 0 ) {
        m.type = MAP;
        }
    else if( strcmp( nameBuffer, "SAY" ) == 0 ) {
        m.type = SAY;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else if( strcmp( nameBuffer, "EMOT" ) == 0 ) {
        m.type = EMOT;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "VOGS" ) == 0 ) {
        m.type = VOGS;
        }
    else if( strcmp( nameBuffer, "VOGN" ) == 0 ) {
        m.type = VOGN;
        }
    else if( strcmp( nameBuffer, "VOGP" ) == 0 ) {
        m.type = VOGP;
        }
    else if( strcmp( nameBuffer, "VOGM" ) == 0 ) {
        m.type = VOGM;
        }
    else if( strcmp( nameBuffer, "VOGI" ) == 0 ) {
        m.type = VOGI;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "VOGT" ) == 0 ) {
        m.type = VOGT;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else if( strcmp( nameBuffer, "VOGX" ) == 0 ) {
        m.type = VOGX;
        }
    else if( strcmp( nameBuffer, "PHOTO" ) == 0 ) {
        m.type = PHOTO;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = 0;
            }
        }
    else if( strcmp( nameBuffer, "LEAD" ) == 0 ) {
        m.type = LEAD;
        }
    else if( strcmp( nameBuffer, "UNFOL" ) == 0 ) {
        m.type = UNFOL;
        }
    else if( strcmp( nameBuffer, "FLIP" ) == 0 ) {
        m.type = FLIP;
        }
    else {
        m.type = UNKNOWN;
        }
    
    // incoming client messages are relative to birth pos
    // except NOT map pull messages, which are absolute
    if( m.type != MAP ) {    
        m.x += inPlayer->birthPos.x;
        m.y += inPlayer->birthPos.y;

        for( int i=0; i<m.numExtraPos; i++ ) {
            m.extraPos[i].x += inPlayer->birthPos.x;
            m.extraPos[i].y += inPlayer->birthPos.y;
            }
        }

    return m;
    }



// computes a fractional index along path
// 1.25 means 1/4 way between index 1 and 2 on path
// thus, this can be as low as -1 (for starting position)
double computePartialMovePathStepPrecise( LiveObject *inPlayer ) {
    
    if( inPlayer->pathLength == 0 || inPlayer->pathToDest == NULL ) {
        return -1;
        }

    double fractionDone = 
        ( Time::getCurrentTime() - 
          inPlayer->moveStartTime )
        / inPlayer->moveTotalSeconds;
    
    if( fractionDone > 1 ) {
        fractionDone = 1;
        }
    
    if( fractionDone < 0 ) {
        fractionDone = 0;
        }

    if( fractionDone == 1 ) {
        // at last spot in path, no partial measurment necessary
        return inPlayer->pathLength - 1;
        }
    
    if( fractionDone == 0 ) {
        // at start location, before first spot in path
        return -1;
        }

    double distDone = fractionDone * inPlayer->pathDist;

    
    // walk through path steps until we see dist done
    double totalLength = 0;
    
    GridPos lastPos = { inPlayer->xs, inPlayer->ys };
    
    double lastPosDist = 0;

    for( int i=0; i<inPlayer->pathLength; i++ ) {

        GridPos thisPos = inPlayer->pathToDest[i];
        
        double stepLen;
        

        if( thisPos.x != lastPos.x &&
            thisPos.y != lastPos.y ) {
            stepLen = diagLength;
            }
        else {
            // not diag
            stepLen = 1;
            }

        totalLength += stepLen;

        if( totalLength > distDone ) {
            // add in extra
            return ( i - 1 ) + (distDone - lastPosDist) / stepLen;
            }

        lastPos = thisPos;
        lastPosDist += stepLen;
        }
    
    return inPlayer->pathLength - 1;
    }




int computePartialMovePathStep( LiveObject *inPlayer ) {
    return lrint( computePartialMovePathStepPrecise( inPlayer ) );
    }



doublePair computePartialMoveSpotPrecise( LiveObject *inPlayer ) {

    double c = computePartialMovePathStepPrecise( inPlayer );
    
    if( c == -1 ) {
        doublePair result = { (double)inPlayer->xs, 
                              (double)inPlayer->ys };
        return result;
        }

    
    int aInd = floor( c );
    int bInd = ceil( c );
    
    
    GridPos aPos;
    
    if( aInd >= 0 ) {
        aPos = inPlayer->pathToDest[ aInd ];
        }
    else {
        aPos.x = inPlayer->xs;
        aPos.y = inPlayer->ys;
        }
    
    double bMix = c - aInd;
    
    doublePair result = { (double)aPos.x, (double)aPos.y };
    
    if( bMix > 0 ) {
        GridPos bPos = inPlayer->pathToDest[ bInd ];
        
        double aMix = 1.0 - bMix;
        
        result.x *= aMix;
        result.y *= aMix;
        
        result.x += bMix * bPos.x;
        result.y += bMix * bPos.y;
        }
    
    return result;
    }




// if inOverrideC > -2, then it is used instead of current partial move step
GridPos computePartialMoveSpot( LiveObject *inPlayer, int inOverrideC = -2 ) {

    int c = inOverrideC;
    if( c < -1 ) {
        c = computePartialMovePathStep( inPlayer );
        }
    
    if( c >= 0 ) {
        
        GridPos cPos = inPlayer->pathToDest[c];
        
        return cPos;
        }
    else {
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    }



GridPos getPlayerPos( LiveObject *inPlayer ) {
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd ) {
        
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    else {
        return computePartialMoveSpot( inPlayer );
        }
    }





static void restockPostWindowFamilies() {
    // take stock of families
    familyNamesAfterEveWindow.deallocateStringElements();
    familyLineageEveIDsAfterEveWindow.deleteAll();
    familyCountsAfterEveWindow.deleteAll();
    nextBabyFamilyIndex = 0;
    
    int barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );


    if( postWindowFamilyLogFile != NULL ) {
        fclose( postWindowFamilyLogFile );
        }
    char *fileName = autoSprintf( "%.f_familyPopLog.txt", Time::timeSec() );
    
    File folder( NULL, "familyPopLogs" );
    
    if( ! folder.exists() ) {
        folder.makeDirectory();
        }

    File *logFile = folder.getChildFile( fileName );
    delete [] fileName;
    
    char *fullPath = logFile->getFullFileName();
    delete logFile;
    
    postWindowFamilyLogFile = fopen( fullPath, "w" );
    
    delete [] fullPath;
    

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error &&
            ! o->isTutorial &&
            o->curseStatus.curseLevel == 0 &&
            familyLineageEveIDsAfterEveWindow.getElementIndex( 
                o->lineageEveID ) == -1 ) {
            // haven't seen this family before

            if( barrierOn ) {
                // only fams inside the barrier
                GridPos pos = getPlayerPos( o );
                
                if( abs( pos.x ) >= barrierRadius ||
                    abs( pos.y ) >= barrierRadius ) {
                    // player outside barrier
                    continue;
                    }
                }

            familyLineageEveIDsAfterEveWindow.push_back( 
                o->lineageEveID );

            char *nameCopy = NULL;

            if( o->familyName != NULL ) {
                nameCopy = stringDuplicate( o->familyName );
                }
            else {
                // don't skip tracking families that have no names
                nameCopy = autoSprintf( "UNNAMED_%d", o->lineageEveID );
                }
            
            familyNamesAfterEveWindow.push_back( nameCopy );
        

            // start with estimate of one person per family
            familyCountsAfterEveWindow.push_back( 1 );
            
            
            if( postWindowFamilyLogFile != NULL ) {
                fprintf( postWindowFamilyLogFile, "\"%s\" ", nameCopy );
                }
            }
        }
    
    if( postWindowFamilyLogFile != NULL ) {
        fprintf( postWindowFamilyLogFile, "\n" );
        }
    }



static void logFamilyCounts() {
    if( postWindowFamilyLogFile != NULL ) {
        int barrierRadius = 
            SettingsManager::getIntSetting( "barrierRadius", 250 );
        int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );
        

        fprintf( postWindowFamilyLogFile, "%.2f ", Time::getCurrentTime() );
        
        for( int i=0; i<familyLineageEveIDsAfterEveWindow.size(); i++ ) {
            int lineageEveID = 
                familyLineageEveIDsAfterEveWindow.getElementDirect( i );
            
            int count = 0;

            for( int p=0; p<players.size(); p++ ) {
                LiveObject *o = players.getElement( p );
                
                if( ! o->error &&
                    ! o->isTutorial &&
                    o->curseStatus.curseLevel == 0 &&
                    o->lineageEveID == lineageEveID ) {
                    
                    if( barrierOn ) {
                        GridPos pos = getPlayerPos( o );
                        
                        if( abs( pos.x ) >= barrierRadius ||
                            abs( pos.y ) >= barrierRadius ) {
                            // player outside barrier
                            continue;
                            }
                        }
                    count++;
                    }
                }
            fprintf( postWindowFamilyLogFile, "%d ", count );
            // remember it
            *( familyCountsAfterEveWindow.getElement( i ) ) = count;
            }
        
        fprintf( postWindowFamilyLogFile, "\n" );
        }
    }






GridPos killPlayer( const char *inEmail ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            o->error = true;
            
            return computePartialMoveSpot( o );
            }
        }
    
    GridPos noPos = { 0, 0 };
    return noPos;
    }



void forcePlayerAge( const char *inEmail, double inAge ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            double ageSec = inAge / getAgeRate();
            
            o->lifeStartTimeSeconds = Time::getCurrentTime() - ageSec;
            o->needsUpdate = true;
            }
        }
    }





double computeFoodDecrementTimeSeconds( LiveObject *inPlayer ) {
    double value = maxFoodDecrementSeconds * 2 * inPlayer->heat;
    
    if( value > maxFoodDecrementSeconds ) {
        // also reduce if too hot (above 0.5 heat)
        
        double extra = value - maxFoodDecrementSeconds;

        value = maxFoodDecrementSeconds - extra;
        }
    
    // all player temp effects push us up above min
    value += minFoodDecrementSeconds;

    value += inPlayer->personalFoodDecrementSecondsBonus;

    inPlayer->indoorBonusTime = 0;
    
    if( inPlayer->isIndoors &&
        inPlayer->indoorBonusFraction > 0 &&
        computeAge( inPlayer ) > defaultActionAge ) {
        
        // non-babies get a bonus for being indoors
        inPlayer->indoorBonusTime = 
            indoorFoodDecrementSecondsBonus *
            inPlayer->indoorBonusFraction;
        
        value += inPlayer->indoorBonusTime;
        }
    
    inPlayer->foodDrainTime = value;

    return value;
    }


double getAgeRate() {
    return 1.0 / secondsPerYear;
    }


static void setDeathReason( LiveObject *inPlayer, const char *inTag,
                            int inOptionalID = 0 ) {
    
    if( inPlayer->deathReason != NULL ) {
        delete [] inPlayer->deathReason;
        }
    
    // leave space in front so it works at end of PU line
    if( strcmp( inTag, "killed" ) == 0 ||
        strcmp( inTag, "succumbed" ) == 0 ) {
        
        inPlayer->deathReason = autoSprintf( " reason_%s_%d", 
                                             inTag, inOptionalID );
        }
    else {
        // ignore ID
        inPlayer->deathReason = autoSprintf( " reason_%s", inTag );
        }
    }



int longestShutdownLine = -1;

void handleShutdownDeath( LiveObject *inPlayer,
                          int inX, int inY ) {
    if( inPlayer->curseStatus.curseLevel == 0 &&
        inPlayer->parentChainLength > longestShutdownLine ) {
        
        // never count a cursed player as a long line
        
        longestShutdownLine = inPlayer->parentChainLength;
        
        FILE *f = fopen( "shutdownLongLineagePos.txt", "w" );
        if( f != NULL ) {
            fprintf( f, "%d,%d", inX, inY );
            fclose( f );
            }
        }
    }



double computeAge( double inLifeStartTimeSeconds ) {
    
    double deltaSeconds = 
        Time::getCurrentTime() - inLifeStartTimeSeconds;
    
    double age = deltaSeconds * getAgeRate();
    
    return age;
    }



double computeAge( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer->lifeStartTimeSeconds );
    if( age >= forceDeathAge ) {
        setDeathReason( inPlayer, "age" );
        
        inPlayer->error = true;
        
        age = forceDeathAge;

        // they lived to old age
        // that means they can get born to their descendants in future
        int lineageID = inPlayer->id;
        if( ! getFemale( inPlayer ) ) {
            // use mother's ID instead
            // since men have no direct descendants
            // their sisters (and their decendants) count as their descendants
            // Note that it's impossible to get reborn to your older sister
            // (because she'll already be dead when you die of old age), 
            // but it might be possible to get reborn to your much-younger 
            // sister.
            lineageID = inPlayer->parentID;
            }
        
        trackOffspring( inPlayer->email, lineageID );
        }
    return age;
    }



int getSayLimit( LiveObject *inPlayer ) {
    return getSayLimit( computeAge( inPlayer ) );
    }




int getSecondsPlayed( LiveObject *inPlayer ) {
    double deltaSeconds = 
        Time::getCurrentTime() - inPlayer->trueStartTimeSeconds;

    return lrint( deltaSeconds );
    }




static int getFirstFertileAge() {
    return 14;
    }


char isFertileAge( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
                    
    char f = getFemale( inPlayer );
                    
    if( age >= getFirstFertileAge() && age <= 40 && f ) {
        return true;
        }
    else {
        return false;
        }
    }




int computeFoodCapacity( LiveObject *inPlayer ) {
    int ageInYears = lrint( computeAge( inPlayer ) );
    
    int returnVal = 0;
    
    if( ageInYears < 44 ) {
        
        if( ageInYears > 16 ) {
            ageInYears = 16;
            }
        
        returnVal = ageInYears + 4;
        }
    else {
        // food capacity decreases as we near 60
        int cap = 60 - ageInYears + 4;
        
        if( cap < 4 ) {
            cap = 4;
            }
        
        int lostBars = 20 - cap;

        if( lostBars > 0 && inPlayer->fitnessScore > 0 ) {
        
            // consider effect of fitness on reducing lost bars

            // for now, let's make it quadratic
            double maxLostBars = 
                16 - 16 * pow( inPlayer->fitnessScore / 60.0, 2 );
            
            if( lostBars > maxLostBars ) {
                lostBars = maxLostBars;
                }

            if( lostBars < 0 ) {
                lostBars = 0;
                }
            }
        
        returnVal = 20 - lostBars;
        }

    return ceil( returnVal * inPlayer->foodCapModifier );
    }



int computeOverflowFoodCapacity( int inBaseCapacity ) {
    // even littlest baby has +1 overflow, to get everyone used to the
    // concept.
    // by adulthood (when base cap is 20), overflow cap is 90.6
    return 1 + pow( inBaseCapacity, 8 ) * 0.0000000035;
    }



static void drinkAlcohol( LiveObject *inPlayer, int inAlcoholAmount ) {
    double doneGrowingAge = 16;
    
    double multiplier = 1.0;
    

    double age = computeAge( inPlayer );
    
    // alcohol affects a baby 2x
    // affects an 8-y-o 1.5x
    if( age < doneGrowingAge ) {
        multiplier += 1.0 - age / doneGrowingAge;
        }

    double amount = inAlcoholAmount * multiplier;
    
    inPlayer->drunkenness += amount;
    }



char *slurSpeech( int inSpeakerID,
                  char *inTranslatedPhrase, double inDrunkenness ) {
    char *working = stringDuplicate( inTranslatedPhrase );
    
    char *starPos = strstr( working, " *" );

    char *extraData = NULL;
    
    if( starPos != NULL ) {
        extraData = stringDuplicate( starPos );
        starPos[0] = '\0';
        }
    
    SimpleVector<char> slurredChars;
    
    // 1 in 10 letters slurred with 1 drunkenness
    // all characters slurred with 10 drunkenness
    double baseSlurChance = 0.1;
    
    double slurChance = baseSlurChance * inDrunkenness;

    // 2 in 10 words mixed up in order with 6 drunkenness
    // all words mixed up at 10 drunkenness
    double baseWordSwapChance = 0.1;

    // but don't start mixing up words at all until 6 drunkenness
    // thus, the 0 to 100% mix up range is from 6 to 10 drunkenness
    double wordSwapChance = 2 * baseWordSwapChance * ( inDrunkenness - 5 );



    // first, swap word order
    SimpleVector<char *> *words = tokenizeString( working );

    // always slurr exactly the same for a given speaker
    // repeating the same phrase won't keep remapping
    // but map different length phrases differently
    JenkinsRandomSource slurRand( inSpeakerID + 
                                  words->size() + 
                                  inDrunkenness );
    

    for( int i=0; i<words->size(); i++ ) {
        if( slurRand.getRandomBoundedDouble( 0, 1 ) < wordSwapChance ) {
            char *temp = words->getElementDirect( i );
            
            // possible swap distance based on drunkenness
            
            // again, don't start reording words until 6 drunkenness
            int maxDist = inDrunkenness - 5;

            if( maxDist >= words->size() - i ) {
                maxDist = words->size() - i - 1;
                }
            
            if( maxDist > 0 ) {
                int jump = slurRand.getRandomBoundedInt( 0, maxDist );
            
                
                *( words->getElement( i ) ) = 
                    words->getElementDirect( i + jump );
            
                *( words->getElement( i + jump ) ) = temp;
                }
            }
        }
    

    char **allWords = words->getElementArray();
    char *wordsTogether = join( allWords, words->size(), " " );
    
    words->deallocateStringElements();
    delete words;
    
    delete [] allWords;

    delete [] working;
    
    working = wordsTogether;


    int len = strlen( working );
    for( int i=0; i<len; i++ ) {
        char c = working[i];
        
        slurredChars.push_back( c );

        if( c < 'A' || c > 'Z' ) {
            // only A-Z, no slurred punctuation
            continue;
            }

        if( slurRand.getRandomBoundedDouble( 0, 1 ) < slurChance ) {
            slurredChars.push_back( c );
            }
        }

    delete [] working;
    
    if( extraData != NULL ) {
        slurredChars.appendElementString( extraData );
        delete [] extraData;
        }
    

    return slurredChars.getElementString();
    }





// with 128-wide tiles, character moves at 480 pixels per second
// at 60 fps, this is 8 pixels per frame
// important that it's a whole number of pixels for smooth camera movement
static double baseWalkSpeed = 3.75;

// min speed for takeoff
static double minFlightSpeed = 15;



double computeMoveSpeed( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
    

    double speed = baseWalkSpeed;
    
    // baby moves at 360 pixels per second, or 6 pixels per frame
    double babySpeedFactor = 0.75;

    double fullSpeedAge = 10.0;
    

    if( age < fullSpeedAge ) {
        
        double speedFactor = babySpeedFactor + 
            ( 1.0 - babySpeedFactor ) * age / fullSpeedAge;
        
        speed *= speedFactor;
        }


    // for now, try no age-based speed decrease
    /*
    if( age < 20 ) {
        speed *= age / 20;
        }
    if( age > 40 ) {
        // half speed by 60, then keep slowing down after that
        speed -= (age - 40 ) * 2.0 / 20.0;
        
        }
    */
    // no longer slow down with hunger
    /*
    int foodCap = computeFoodCapacity( inPlayer );
    
    
    if( inPlayer->foodStore <= foodCap / 2 ) {
        // jumps instantly to 1/2 speed at half food, then decays after that
        speed *= inPlayer->foodStore / (double) foodCap;
        }
    */



    // apply character's speed mult
    speed *= getObject( inPlayer->displayID )->speedMult;
    

    char riding = false;
    
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *r = getObject( inPlayer->holdingID );

        if( r->clothing == 'n' ) {
            // clothing only changes your speed when it's worn
            speed *= r->speedMult;
            }
        
        if( r->rideable ) {
            riding = true;
            }
        }
    

    if( !riding ) {
        // clothing can affect speed

        for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
            ObjectRecord *c = clothingByIndex( inPlayer->clothing, i );
            
            if( c != NULL ) {
                
                speed *= c->speedMult;
                }
            }
        }

    if( inPlayer->killPosseSize > 0 ) {
        // player part of a posse
        double posseSpeedMult = 1.0;
        
        if( inPlayer->killPosseSize <= 4 ) {
            posseSpeedMult = 
                posseSizeSpeedMultipliers[ inPlayer->killPosseSize - 1 ];
            }
        else {
            // 4+ same value as 4
            posseSpeedMult = posseSizeSpeedMultipliers[3];
            }
        
        if( inPlayer->isTwin || inPlayer->isLastLifeShort ) {
            // twins always run at slowest speed when trying to kill
            // same with people who are die-cycling to pick their birth location
            posseSpeedMult = posseSizeSpeedMultipliers[0];
            }

        speed *= posseSpeedMult;
        }
    

    // never move at 0 speed, divide by 0 errors for eta times
    if( speed < 0.01 ) {
        speed = 0.01;
        }

    
    // after all multipliers, make sure it's a whole number of pixels per frame

    double pixelsPerFrame = speed * 128.0 / 60.0;
    
    
    if( pixelsPerFrame > 0.5 ) {
        // can round to at least one pixel per frame
        pixelsPerFrame = lrint( pixelsPerFrame );
        }
    else {
        // fractional pixels per frame
        
        // ensure a whole number of frames per pixel
        double framesPerPixel = 1.0 / pixelsPerFrame;
        
        framesPerPixel = lrint( framesPerPixel );
        
        pixelsPerFrame = 1.0 / framesPerPixel;
        }
    
    speed = pixelsPerFrame * 60 / 128.0;
        
    return speed;
    }







static float sign( float inF ) {
    if (inF > 0) return 1;
    if (inF < 0) return -1;
    return 0;
    }


// how often do we check what a player is standing on top of for attack effects?
static double playerCrossingCheckStepTime = 0.25;


// for steps in main loop that shouldn't happen every loop
// (loop runs faster or slower depending on how many messages are incoming)
static double periodicStepTime = 0.25;
static double lastPeriodicStepTime = 0;




// recompute heat for fixed number of players per timestep
static int numPlayersRecomputeHeatPerStep = 8;
static int lastPlayerIndexHeatRecomputed = -1;
static double lastHeatUpdateTime = 0;
static double heatUpdateTimeStep = 0.1;


// how often the player's personal heat advances toward their environmental
// heat value
static double heatUpdateSeconds = 2;


// air itself offers some insulation
// a vacuum panel has R-value that is 25x greater than air
static float rAir = 0.04;



// blend R-values multiplicatively, for layers
// 1 - R( A + B ) = (1 - R(A)) * (1 - R(B))
//
// or
//
//R( A + B ) =  R(A) + R(B) - R(A) * R(B)
static double rCombine( double inRA, double inRB ) {
    return inRA + inRB - inRA * inRB;
    }




static float computeClothingR( LiveObject *inPlayer ) {
    
    float headWeight = 0.25;
    float chestWeight = 0.35;
    float buttWeight = 0.2;
    float eachFootWeigth = 0.1;
            
    float backWeight = 0.1;


    float clothingR = 0;
            
    if( inPlayer->clothing.hat != NULL ) {
        clothingR += headWeight *  inPlayer->clothing.hat->rValue;
        }
    if( inPlayer->clothing.tunic != NULL ) {
        clothingR += chestWeight * inPlayer->clothing.tunic->rValue;
        }
    if( inPlayer->clothing.frontShoe != NULL ) {
        clothingR += 
            eachFootWeigth * inPlayer->clothing.frontShoe->rValue;
        }
    if( inPlayer->clothing.backShoe != NULL ) {
        clothingR += eachFootWeigth * 
            inPlayer->clothing.backShoe->rValue;
        }
    if( inPlayer->clothing.bottom != NULL ) {
        clothingR += buttWeight * inPlayer->clothing.bottom->rValue;
        }
    if( inPlayer->clothing.backpack != NULL ) {
        clothingR += backWeight * inPlayer->clothing.backpack->rValue;
        }
    
    // even if the player is naked, they are insulated from their
    // environment by rAir
    return rCombine( rAir, clothingR );
    }



static float computeClothingHeat( LiveObject *inPlayer ) {
    // clothing can contribute heat
    // apply this separate from heat grid above
    float clothingHeat = 0;
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                
        ObjectRecord *cO = clothingByIndex( inPlayer->clothing, c );
            
        if( cO != NULL ) {
            clothingHeat += cO->heatValue;

            // contained items in clothing can contribute
            // heat, shielded by clothing r-values
            double cRFactor = 1 - cO->rValue;

            for( int s=0; 
                 s < inPlayer->clothingContained[c].size(); s++ ) {
                        
                ObjectRecord *sO = 
                    getObject( inPlayer->clothingContained[c].
                               getElementDirect( s ) );
                        
                clothingHeat += 
                    sO->heatValue * cRFactor;
                }
            }
        }
    return clothingHeat;
    }



static float computeHeldHeat( LiveObject *inPlayer ) {
    float heat = 0;
    
    // what player is holding can contribute heat
    // add this to the grid, since it's "outside" the player's body
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *heldO = getObject( inPlayer->holdingID );
                
        heat += heldO->heatValue;
                
        double heldRFactor = 1 - heldO->rValue;
                
        // contained can contribute too, but shielded by r-value
        // of container
        for( int c=0; c<inPlayer->numContained; c++ ) {
                    
            int cID = inPlayer->containedIDs[c];
            char hasSub = false;
                    
            if( cID < 0 ) {
                hasSub = true;
                cID = -cID;
                }

            ObjectRecord *contO = getObject( cID );
                    
            heat += 
                contO->heatValue * heldRFactor;
                    

            if( hasSub ) {
                // sub contained too, but shielded by both r-values
                double contRFactor = 1 - contO->rValue;

                for( int s=0; 
                     s<inPlayer->subContainedIDs[c].size(); s++ ) {
                        
                    ObjectRecord *subO =
                        getObject( inPlayer->subContainedIDs[c].
                                   getElementDirect( s ) );
                            
                    heat += 
                        subO->heatValue * 
                        contRFactor * heldRFactor;
                    }
                }
            }
        }
    return heat;
    }




static void recomputeHeatMap( LiveObject *inPlayer ) {
    
    int gridSize = HEAT_MAP_D * HEAT_MAP_D;

    // assume indoors until we find an air boundary of space
    inPlayer->isIndoors = true;
    

    // what if we recompute it from scratch every time?
    for( int i=0; i<gridSize; i++ ) {
        inPlayer->heatMap[i] = 0;
        }

    float heatOutputGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rFloorGrid[ HEAT_MAP_D * HEAT_MAP_D ];


    GridPos pos = getPlayerPos( inPlayer );


    // held baby's pos matches parent pos
    if( inPlayer->heldByOther ) {
        LiveObject *parentObject = getLiveObject( inPlayer->heldByOtherID );
        
        if( parentObject != NULL ) {
            pos = getPlayerPos( parentObject );
            }
        } 

    
    

    for( int y=0; y<HEAT_MAP_D; y++ ) {
        int mapY = pos.y + y - HEAT_MAP_D / 2;
                
        for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
            int mapX = pos.x + x - HEAT_MAP_D / 2;
                    
            int j = y * HEAT_MAP_D + x;
            heatOutputGrid[j] = 0;
            rGrid[j] = rAir;
            rFloorGrid[j] = rAir;


            // call Raw version for better performance
            // we don't care if object decayed since we last looked at it
            ObjectRecord *o = getObject( getMapObjectRaw( mapX, mapY ) );
                    
                    
                    

            if( o != NULL ) {
                heatOutputGrid[j] += o->heatValue;
                if( o->permanent ) {
                    // loose objects sitting on ground don't
                    // contribute to r-value (like dropped clothing)
                    rGrid[j] = rCombine( rGrid[j], o->rValue );
                    }


                // skip checking for heat-producing contained items
                // for now.  Consumes too many server-side resources
                // can still check for heat produced by stuff in
                // held container (below).
                        
                if( false && o->numSlots > 0 ) {
                    // contained can produce heat shielded by container
                    // r value
                    double oRFactor = 1 - o->rValue;
                            
                    int numCont;
                    int *cont = getContained( mapX, mapY, &numCont );
                            
                    if( cont != NULL ) {
                                
                        for( int c=0; c<numCont; c++ ) {
                                    
                            int cID = cont[c];
                            char hasSub = false;
                            if( cID < 0 ) {
                                hasSub = true;
                                cID = -cID;
                                }

                            ObjectRecord *cO = getObject( cID );
                            heatOutputGrid[j] += 
                                cO->heatValue * oRFactor;
                                    
                            if( hasSub ) {
                                double cRFactor = 1 - cO->rValue;
                                        
                                int numSub;
                                int *sub = getContained( mapX, mapY, 
                                                         &numSub, 
                                                         c + 1 );
                                if( sub != NULL ) {
                                    for( int s=0; s<numSub; s++ ) {
                                        ObjectRecord *sO = 
                                            getObject( sub[s] );
                                                
                                        heatOutputGrid[j] += 
                                            sO->heatValue * 
                                            cRFactor * 
                                            oRFactor;
                                        }
                                    delete [] sub;
                                    }
                                }
                            }
                        delete [] cont;
                        }
                    }
                }
                    

            // floor can insulate or produce heat too
            ObjectRecord *fO = getObject( getMapFloor( mapX, mapY ) );
                    
            if( fO != NULL ) {
                heatOutputGrid[j] += fO->heatValue;
                rFloorGrid[j] = rCombine( rFloorGrid[j], fO->rValue );
                }
            }
        }


    
    int numNeighbors = 8;
    int ndx[8] = { 0, 1,  0, -1,  1,  1, -1, -1 };
    int ndy[8] = { 1, 0, -1,  0,  1, -1,  1, -1 };
    
            
    int playerMapIndex = 
        ( HEAT_MAP_D / 2 ) * HEAT_MAP_D +
        ( HEAT_MAP_D / 2 );
        

    
            
    heatOutputGrid[ playerMapIndex ] += computeHeldHeat( inPlayer );
    

    // grid of flags for points that are in same airspace (surrounded by walls)
    // as player
    // This is the area where heat spreads evenly by convection
    char airSpaceGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    
    memset( airSpaceGrid, false, HEAT_MAP_D * HEAT_MAP_D );
    
    airSpaceGrid[ playerMapIndex ] = true;

    SimpleVector<int> frontierA;
    SimpleVector<int> frontierB;
    frontierA.push_back( playerMapIndex );
    
    SimpleVector<int> *thisFrontier = &frontierA;
    SimpleVector<int> *nextFrontier = &frontierB;

    while( thisFrontier->size() > 0 ) {

        for( int f=0; f<thisFrontier->size(); f++ ) {
            
            int i = thisFrontier->getElementDirect( f );
            
            char negativeYCutoff = false;
            
            if( rGrid[i] > rAir ) {
                // grid cell is insulating, and somehow it's in our
                // frontier.  Player must be standing behind a closed
                // door.  Block neighbors to south
                negativeYCutoff = true;
                }
            

            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( negativeYCutoff && ndy[n] < 1 ) {
                    continue;
                    }

                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {

                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ]
                        && rGrid[ nj ] <= rAir ) {

                        airSpaceGrid[ nj ] = true;
                        
                        nextFrontier->push_back( nj );
                        }
                    }
                }
            }
        
        thisFrontier->deleteAll();
        
        SimpleVector<int> *temp = thisFrontier;
        thisFrontier = nextFrontier;
        
        nextFrontier = temp;
        }
    
    if( rGrid[playerMapIndex] > rAir ) {
        // player standing in insulated spot
        // don't count this as part of their air space
        airSpaceGrid[ playerMapIndex ] = false;
        }

    int numInAirspace = 0;
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[ i ] ) {
            numInAirspace++;
            }
        }
    
    
    float rBoundarySum = 0;
    int rBoundarySize = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {
                    
                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ] ) {
                        
                        // boundary!
                        rBoundarySum += rGrid[ nj ];
                        rBoundarySize ++;
                        }
                    }
                else {
                    // boundary is off edge
                    // assume air R-value
                    rBoundarySum += rAir;
                    rBoundarySize ++;
                    inPlayer->isIndoors = false;
                    }
                }
            }
        }

    
    // floor counts as boundary too
    // 4x its effect (seems more important than one of 8 walls
    
    // count non-air floor tiles while we're at it
    int numFloorTilesInAirspace = 0;

    if( numInAirspace > 0 ) {
        for( int i=0; i<gridSize; i++ ) {
            if( airSpaceGrid[i] ) {
                rBoundarySum += 4 * rFloorGrid[i];
                rBoundarySize += 4;
                
                if( rFloorGrid[i] > rAir ) {
                    numFloorTilesInAirspace++;
                    }
                else {
                    // gap in floor
                    inPlayer->isIndoors = false;
                    }
                }
            }
        }
    


    float rBoundaryAverage = rAir;
    
    if( rBoundarySize > 0 ) {
        rBoundaryAverage = rBoundarySum / rBoundarySize;
        }

    if( inPlayer->isIndoors ) {
        // the more insulating the boundary, the bigger the bonus
        inPlayer->indoorBonusFraction = rBoundaryAverage;
        }
    
    



    float airSpaceHeatSum = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            airSpaceHeatSum += heatOutputGrid[i];
            }
        }


    float airSpaceHeatVal = 0;
    
    if( numInAirspace > 0 ) {
        // spread heat evenly over airspace
        airSpaceHeatVal = airSpaceHeatSum / numInAirspace;
        }

    float containedAirSpaceHeatVal = airSpaceHeatVal * rBoundaryAverage;
    


    float radiantAirSpaceHeatVal = 0;

    GridPos playerHeatMapPos = { playerMapIndex % HEAT_MAP_D, 
                                 playerMapIndex / HEAT_MAP_D };
    

    int numRadiantHeatSources = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] && heatOutputGrid[i] > 0 ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            GridPos heatPos = { x, y };
            

            double d = distance( playerHeatMapPos, heatPos );
            
            // avoid infinite heat when player standing on source

            radiantAirSpaceHeatVal += heatOutputGrid[ i ] / ( 1.5 * d + 1 );
            numRadiantHeatSources ++;
            }
        }
    

    float biomeHeatWeight = 1;
    float radiantHeatWeight = 1;
    
    float containedHeatWeight = 4;


    // boundary r-value also limits affect of biome heat on player's
    // environment... keeps biome "out"
    float boundaryLeak = 1 - rBoundaryAverage;

    if( numFloorTilesInAirspace != numInAirspace ) {
        // biome heat invades airspace if entire thing isn't covered by
        // a floor (not really indoors)
        boundaryLeak = 1;
        }


    // a hot biome only pulls us up above perfect
    // (hot biome leaking into a building can never make the building
    //  just right).
    // Enclosed walls can make a hot biome not as hot, but never cool
    float biomeHeat = getBiomeHeatValue( getMapBiome( pos.x, pos.y ) );
    
    if( biomeHeat > targetHeat ) {
        biomeHeat = boundaryLeak * (biomeHeat - targetHeat) + targetHeat;
        }
    else if( biomeHeat < 0 ) {
        // a cold biome's coldness is modulated directly by walls, however
        biomeHeat = boundaryLeak * biomeHeat;
        }
    
    // small offset to ensure that naked-on-green biome the same
    // in new heat model as old
    float constHeatValue = 1.1;

    inPlayer->envHeat = 
        radiantHeatWeight * radiantAirSpaceHeatVal + 
        containedHeatWeight * containedAirSpaceHeatVal +
        biomeHeatWeight * biomeHeat +
        constHeatValue;

    inPlayer->biomeHeat = biomeHeat + constHeatValue;
    }




typedef struct MoveRecord {
        int playerID;
        char *formatString;
        int absoluteX, absoluteY;
    } MoveRecord;



// formatString in returned record destroyed by caller
MoveRecord getMoveRecord( LiveObject *inPlayer,
                          char inNewMovesOnly,
                          SimpleVector<ChangePosition> *inChangeVector = 
                          NULL ) {

    MoveRecord r;
    r.playerID = inPlayer->id;
    
    // p_id xs ys xd yd fraction_done eta_sec
    
    double deltaSec = Time::getCurrentTime() - inPlayer->moveStartTime;
    
    double etaSec = inPlayer->moveTotalSeconds - deltaSec;
    
    if( etaSec < 0 ) {
        etaSec = 0;
        }

    
    r.absoluteX = inPlayer->xs;
    r.absoluteY = inPlayer->ys;
            
            
    SimpleVector<char> messageLineBuffer;
    
    // start is absolute
    char *startString = autoSprintf( "%d %%d %%d %.3f %.3f %d", 
                                     inPlayer->id, 
                                     inPlayer->moveTotalSeconds, etaSec,
                                     inPlayer->pathTruncated );
    // mark that this has been sent
    inPlayer->pathTruncated = false;

    if( inNewMovesOnly ) {
        inPlayer->newMove = false;
        }

            
    messageLineBuffer.appendElementString( startString );
    delete [] startString;
            
    for( int p=0; p<inPlayer->pathLength; p++ ) {
                // rest are relative to start
        char *stepString = autoSprintf( " %d %d", 
                                        inPlayer->pathToDest[p].x
                                        - inPlayer->xs,
                                        inPlayer->pathToDest[p].y
                                        - inPlayer->ys );
        
        messageLineBuffer.appendElementString( stepString );
        delete [] stepString;
        }
    
    messageLineBuffer.appendElementString( "\n" );
    
    r.formatString = messageLineBuffer.getElementString();    
    
    if( inChangeVector != NULL ) {
        ChangePosition p = { inPlayer->xd, inPlayer->yd, false };
        inChangeVector->push_back( p );
        }

    return r;
    }



SimpleVector<MoveRecord> getMoveRecords( 
    char inNewMovesOnly,
    SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    SimpleVector<MoveRecord> v;
    
    int numPlayers = players.size();
                
    for( int i=0; i<numPlayers; i++ ) {
                
        LiveObject *o = players.getElement( i );                
        
        if( o->error ) {
            continue;
            }

        if( ( o->xd != o->xs || o->yd != o->ys )
            &&
            ( o->newMove || !inNewMovesOnly ) ) {
            
 
            MoveRecord r = getMoveRecord( o, inNewMovesOnly, inChangeVector );
            
            v.push_back( r );
            }
        }

    return v;
    }



char *getMovesMessageFromList( SimpleVector<MoveRecord> *inMoves,
                               GridPos inRelativeToPos ) {

    int numLines = 0;
    
    SimpleVector<char> messageBuffer;

    messageBuffer.appendElementString( "PM\n" );

    for( int i=0; i<inMoves->size(); i++ ) {
        MoveRecord r = inMoves->getElementDirect(i);
        
        char *line = autoSprintf( r.formatString, 
                                  r.absoluteX - inRelativeToPos.x,
                                  r.absoluteY - inRelativeToPos.y );
        
        messageBuffer.appendElementString( line );
        delete [] line;
        
        numLines ++;
        }
    
    if( numLines > 0 ) {
        
        messageBuffer.push_back( '#' );
                
        char *message = messageBuffer.getElementString();
        
        return message;
        }
    
    return NULL;
    }



double intDist( int inXA, int inYA, int inXB, int inYB ) {
    double dx = (double)inXA - (double)inXB;
    double dy = (double)inYA - (double)inYB;

    return sqrt(  dx * dx + dy * dy );
    }
    
    
    
// returns NULL if there are no matching moves
// positions in moves relative to inRelativeToPos
// filters out moves that are taking place further than 32 away from inLocalPos
char *getMovesMessage( char inNewMovesOnly,
                       GridPos inRelativeToPos,
                       GridPos inLocalPos,
                       SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    
    SimpleVector<MoveRecord> v = getMoveRecords( inNewMovesOnly, 
                                                 inChangeVector );
    
    SimpleVector<MoveRecord> closeRecords;

    for( int i=0; i<v.size(); i++ ) {
        MoveRecord r = v.getElementDirect( i );
        
        double d = intDist( r.absoluteX, r.absoluteY,
                            inLocalPos.x, inLocalPos.y );
        
        if( d <= 32 ) {
            closeRecords.push_back( r );
            }
        }
    
    

    char *message = getMovesMessageFromList( &closeRecords, inRelativeToPos );
    
    for( int i=0; i<v.size(); i++ ) {
        delete [] v.getElement(i)->formatString;
        }
    
    return message;
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }


//static char isGridAdjacent( GridPos inA, GridPos inB ) {
//    return isGridAdjacent( inA.x, inA.y, inB.x, inB.y );
//    }


static char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB ) {
    if( isGridAdjacent( inXA, inYA, inXB, inYB ) ) {
        return true;
        }
    
    if( abs( inXA - inXB ) == 1 && abs( inYA - inYB ) == 1 ) {
        return true;
        }
    
    return false;
    }


static char isGridAdjacentDiag( GridPos inA, GridPos inB ) {
    return isGridAdjacentDiag( inA.x, inA.y, inB.x, inB.y );
    }



static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }





// returns (0,0) if no player found
GridPos getClosestPlayerPos( int inX, int inY ) {
    GridPos c = { inX, inY };
    
    double closeDist = DBL_MAX;
    GridPos closeP = { 0, 0 };
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        if( o->error ) {
            continue;
            }
        if( o->heldByOther ) {
            continue;
            }
        
        GridPos p;

        if( o->xs == o->xd && o->ys == o->yd ) {
            p.x = o->xd;
            p.y = o->yd;
            }
        else {
            p = computePartialMoveSpot( o );
            }
        
        double d = distance( p, c );
        
        if( d < closeDist ) {
            closeDist = d;
            closeP = p;
            }
        }
    return closeP;
    }




static int chunkDimensionX = 32;
static int chunkDimensionY = 30;

static int maxSpeechRadius = 16;


static int getMaxChunkDimension() {
    return chunkDimensionX;
    }


static SocketPoll sockPoll;



static void setPlayerDisconnected( LiveObject *inPlayer, 
                                   const char *inReason ) {    
    /*
    setDeathReason( inPlayer, "disconnected" );
    
    inPlayer->error = true;
    inPlayer->errorCauseString = inReason;
    */
    // don't kill them
    
    // just mark them as not connected

    AppLog::infoF( "Player %d (%s) marked as disconnected (%s).",
                   inPlayer->id, inPlayer->email, inReason );
    inPlayer->connected = false;

    // when player reconnects, they won't get a force PU message
    // so we shouldn't be waiting for them to ack
    inPlayer->waitingForForceResponse = false;


    if( inPlayer->vogMode ) {    
        inPlayer->vogMode = false;
                        
        GridPos p = inPlayer->preVogPos;
        
        inPlayer->xd = p.x;
        inPlayer->yd = p.y;
        
        inPlayer->xs = p.x;
        inPlayer->ys = p.y;

        inPlayer->birthPos = inPlayer->preVogBirthPos;
        }
    
    
    if( inPlayer->sock != NULL ) {
        // also, stop polling their socket, which will trigger constant
        // socket events from here on out, and cause us to busy-loop
        sockPoll.removeSocket( inPlayer->sock );

        delete inPlayer->sock;
        inPlayer->sock = NULL;
        }
    if( inPlayer->sockBuffer != NULL ) {
        delete inPlayer->sockBuffer;
        inPlayer->sockBuffer = NULL;
        }
    }



// if inOnePlayerOnly set, we only send to that player
void sendGlobalMessage( char *inMessage,
                        LiveObject *inOnePlayerOnly ) {
    
    double curTime = Time::getCurrentTime();
    
    char found;
    char *noSpaceMessage = replaceAll( inMessage, " ", "_", &found );

    char *fullMessage = autoSprintf( "MS\n%s\n#", noSpaceMessage );
    
    delete [] noSpaceMessage;

    int len = strlen( fullMessage );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( inOnePlayerOnly != NULL && o != inOnePlayerOnly ) {
            continue;
            }

        if( ! o->error && ! o->isTutorial && o->connected ) {


            if( curTime - o->lastGlobalMessageTime > 
                minGlobalMessageSpacingSeconds ) {
                
                int numSent = 
                    o->sock->send( (unsigned char*)fullMessage, 
                                   len, 
                                   false, false );
                
                o->lastGlobalMessageTime = curTime;
                
                if( numSent != len ) {
                    setPlayerDisconnected( o, "Socket write failed" );
                    }
                }
            else {
                // messages are coming too quickly for this player to read
                // them, wait before sending this one
                o->globalMessageQueue.push_back( stringDuplicate( inMessage ) );
                }
            }
        }
    delete [] fullMessage;
    }



typedef struct WarPeaceMessageRecord {
        char war;
        int lineageAEveID;
        int lineageBEveID;
        double t;
    } WarPeaceMessageRecord;

SimpleVector<WarPeaceMessageRecord> warPeaceRecords;



void sendPeaceWarMessage( const char *inPeaceOrWar,
                          char inWar,
                          int inLineageAEveID, int inLineageBEveID ) {
    
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<warPeaceRecords.size(); i++ ) {
        WarPeaceMessageRecord *r = warPeaceRecords.getElement( i );
        
        if( inWar != r->war ) {
            continue;
            }
        
        if( ( r->lineageAEveID == inLineageAEveID &&
              r->lineageBEveID == inLineageBEveID )
            ||
            ( r->lineageAEveID == inLineageBEveID &&
              r->lineageBEveID == inLineageAEveID ) ) {

            if( r->t > curTime - 3 * 60 ) {
                // stil fresh, last similar message happened
                // less than three minutes ago
                return;
                }
            else {
                // stale
                // remove it
                warPeaceRecords.deleteElement( i );
                break;
                }
            }
        }
    WarPeaceMessageRecord r = { inWar, inLineageAEveID, inLineageBEveID,
                                curTime };
    warPeaceRecords.push_back( r );


    const char *nameA = "NAMELESS";
    const char *nameB = "NAMELESS";
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *o = players.getElement( j );
                        
        if( ! o->error && 
            o->lineageEveID == inLineageAEveID &&
            o->familyName != NULL ) {
            nameA = o->familyName;
            break;
            }
        }
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *o = players.getElement( j );
                        
        if( ! o->error && 
            o->lineageEveID == inLineageBEveID &&
            o->familyName != NULL ) {
            nameB = o->familyName;
            break;
            }
        }

    char *message = autoSprintf( "%s BETWEEN %s**AND %s FAMILIES",
                                 inPeaceOrWar,
                                 nameA, nameB );

    sendGlobalMessage( message );
    
    delete [] message;
    }




void checkCustomGlobalMessage() {
    
    if( ! SettingsManager::getIntSetting( "customGlobalMessageOn", 0 ) ) {
        return;
        }


    double spacing = 
        SettingsManager::getDoubleSetting( 
            "customGlobalMessageSecondsSpacing", 10.0 );
    
    double lastTime = 
        SettingsManager::getDoubleSetting( 
            "customGlobalMessageLastSendTime", 0.0 );

    double curTime = Time::getCurrentTime();
    
    if( curTime - lastTime < spacing ) {
        return;
        }
        

    
    // check if there's a new custom message waiting
    char *message = 
        SettingsManager::getSettingContents( "customGlobalMessage", 
                                             "" );
    
    if( strcmp( message, "" ) != 0 ) {
        

        int numLines;
        
        char **lines = split( message, "\n", &numLines );
        
        int nextLine = 
            SettingsManager::getIntSetting( 
                "customGlobalMessageNextLine", 0 );
        
        if( nextLine < numLines ) {
            sendGlobalMessage( lines[nextLine] );
            
            nextLine++;
            SettingsManager::setSetting( 
                "customGlobalMessageNextLine", nextLine );

            SettingsManager::setDoubleSetting( 
                "customGlobalMessageLastSendTime", curTime );
            }
        else {
            // out of lines
            SettingsManager::setSetting( "customGlobalMessageOn", 0 );
            SettingsManager::setSetting( "customGlobalMessageNextLine", 0 );
            }

        for( int i=0; i<numLines; i++ ) {
            delete [] lines[i];
            }
        delete [] lines;
        }
    else {
        // no message, disable
        SettingsManager::setSetting( "customGlobalMessageOn", 0 );
        }
    
    delete [] message;
    }




/**
 * @note: sets lastSentMap in inO if chunk goes through
 * @note: returns result of send, auto-marks error in inO
 */
int sendMapChunkMessage( LiveObject *inO, 
                         char inDestOverride = false,
                         int inDestOverrideX = 0, 
                         int inDestOverrideY = 0 ) {
    
    if( ! inO->connected ) {
        // act like it was a successful send so we can move on until
        // they reconnect later
        return 1;
        }
    
    int messageLength = 0;

    int xd = inO->xd;
    int yd = inO->yd;
    
    if( inDestOverride ) {
        xd = inDestOverrideX;
        yd = inDestOverrideY;
        }
    
    
    int halfW = chunkDimensionX / 2;
    int halfH = chunkDimensionY / 2;
    
    int fullStartX = xd - halfW;
    int fullStartY = yd - halfH;
    
    int numSent = 0;

    

    if( ! inO->firstMapSent ) {
        // send full rect centered on x,y
        
        inO->firstMapSent = true;
        
        unsigned char *mapChunkMessage = getChunkMessage( fullStartX,
                                                          fullStartY,
                                                          chunkDimensionX,
                                                          chunkDimensionY,
                                                          inO->birthPos,
                                                          &messageLength );
                
        numSent += 
            inO->sock->send( mapChunkMessage, 
                             messageLength, 
                             false, false );
                
        delete [] mapChunkMessage;
        }
    else {
        
        // our closest previous chunk center
        int lastX = inO->lastSentMapX;
        int lastY = inO->lastSentMapY;


        // split next chunk into two bars by subtracting last chunk
        
        int horBarStartX = fullStartX;
        int horBarStartY = fullStartY;
        int horBarW = chunkDimensionX;
        int horBarH = chunkDimensionY;
        
        if( yd > lastY ) {
            // remove bottom of bar
            horBarStartY = lastY + halfH;
            horBarH = yd - lastY;
            }
        else {
            // remove top of bar
            horBarH = lastY - yd;
            }

        if( horBarH > chunkDimensionY ) {
            // don't allow bar to grow too big if we have a huge jump
            // like from VOG mode
            horBarH = chunkDimensionY;
            }
        

        int vertBarStartX = fullStartX;
        int vertBarStartY = fullStartY;
        int vertBarW = chunkDimensionX;
        int vertBarH = chunkDimensionY;
        
        if( xd > lastX ) {
            // remove left part of bar
            vertBarStartX = lastX + halfW;
            vertBarW = xd - lastX;
            }
        else {
            // remove right part of bar
            vertBarW = lastX - xd;
            }
        
        
        if( vertBarW > chunkDimensionX ) {
            // don't allow bar to grow too big if we have a huge jump
            // like from VOG mode
            vertBarW = chunkDimensionX;
            }
        
        
        // now trim vert bar where it intersects with hor bar
        if( yd > lastY ) {
            // remove top of vert bar
            vertBarH -= horBarH;
            }
        else {
            // remove bottom of vert bar
            vertBarStartY = horBarStartY + horBarH;
            vertBarH -= horBarH;
            }
        
        
        // only send if non-zero width and height
        if( horBarW > 0 && horBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( horBarStartX,
                                                              horBarStartY,
                                                              horBarW,
                                                              horBarH,
                                                              inO->birthPos,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        if( vertBarW > 0 && vertBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( vertBarStartX,
                                                              vertBarStartY,
                                                              vertBarW,
                                                              vertBarH,
                                                              inO->birthPos,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        }
    
    
    inO->gotPartOfThisFrame = true;
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = xd;
        inO->lastSentMapY = yd;
        }
    else {
        setPlayerDisconnected( inO, "Socket write failed" );
        }
    return numSent;
    }







char *getHoldingString( LiveObject *inObject ) {
    
    int holdingID = hideIDForClient( inObject->holdingID );    


    if( inObject->numContained == 0 ) {
        return autoSprintf( "%d", holdingID );
        }

    
    SimpleVector<char> buffer;
    

    char *idString = autoSprintf( "%d", holdingID );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    if( inObject->numContained > 0 ) {
        for( int i=0; i<inObject->numContained; i++ ) {
            
            char *idString = autoSprintf( 
                ",%d", 
                hideIDForClient( abs( inObject->containedIDs[i] ) ) );
    
            buffer.appendElementString( idString );
    
            delete [] idString;

            if( inObject->subContainedIDs[i].size() > 0 ) {
                for( int s=0; s<inObject->subContainedIDs[i].size(); s++ ) {
                    
                    idString = autoSprintf( 
                        ":%d", 
                        hideIDForClient( 
                            inObject->subContainedIDs[i].
                            getElementDirect( s ) ) );
    
                    buffer.appendElementString( idString );
                
                    delete [] idString;
                    }
                }
            }
        }
    
    return buffer.getElementString();
    }



// only consider living, non-moving players
char isMapSpotEmptyOfPlayers( int inX, int inY ) {

    int numLive = players.size();
    
    for( int i=0; i<numLive; i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        
        if( // not about to be deleted
            ! nextPlayer->error &&
            // held players aren't on map (their coordinates are stale)
            ! nextPlayer->heldByOther &&
            // stationary
            nextPlayer->xs == nextPlayer->xd &&
            nextPlayer->ys == nextPlayer->yd &&
            // in this spot
            inX == nextPlayer->xd &&
            inY == nextPlayer->yd ) {
            return false;            
            } 
        }
    
    return true;
    }




// checks both grid of objects and live, non-moving player positions
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true ) {
    int target = getMapObject( inX, inY );
    
    if( target != 0 ) {
        return false;
        }
    
    if( !inConsiderPlayers ) {
        return true;
        }
    
    return isMapSpotEmptyOfPlayers( inX, inY );
    }



static void setFreshEtaDecayForHeld( LiveObject *inPlayer ) {
    
    if( inPlayer->holdingID == 0 ) {
        inPlayer->holdingEtaDecay = 0;
        }
    
    // does newly-held object have a decay defined?
    TransRecord *newDecayT = getMetaTrans( -1, inPlayer->holdingID );
                    
    if( newDecayT != NULL ) {
        inPlayer->holdingEtaDecay = 
            Time::timeSec() + newDecayT->autoDecaySeconds;
        }
    else {
        // no further decay
        inPlayer->holdingEtaDecay = 0;
        }
    }




static void truncateMove( LiveObject *otherPlayer, int blockedStep ) {
    
    int c = computePartialMovePathStep( otherPlayer );
    
    otherPlayer->pathLength
        = blockedStep;
    otherPlayer->pathTruncated
        = true;
    
    // update timing
    double dist = 
        measurePathLength( otherPlayer->xs,
                           otherPlayer->ys,
                           otherPlayer->pathToDest,
                           otherPlayer->pathLength );    
    
    double distAlreadyDone =
        measurePathLength( otherPlayer->xs,
                           otherPlayer->ys,
                           otherPlayer->pathToDest,
                           c );
    
    double moveSpeed = computeMoveSpeed( otherPlayer ) *
        getPathSpeedModifier( otherPlayer->pathToDest,
                              otherPlayer->pathLength );
    
    otherPlayer->moveTotalSeconds 
        = 
        dist / 
        moveSpeed;
    
    double secondsAlreadyDone = 
        distAlreadyDone / 
        moveSpeed;
    
    otherPlayer->moveStartTime = 
        Time::getCurrentTime() - 
        secondsAlreadyDone;
    
    otherPlayer->newMove = true;
    
    otherPlayer->xd 
        = otherPlayer->pathToDest[
            blockedStep - 1].x;
    otherPlayer->yd 
        = otherPlayer->pathToDest[
            blockedStep - 1].y;
    }




static void endAnyMove( LiveObject *nextPlayer ) {
    
    if( nextPlayer->xd != nextPlayer->xs ||
        nextPlayer->yd != nextPlayer->ys ) {
        
        int truncationSpot = 
            computePartialMovePathStep( nextPlayer );
        
        if( truncationSpot < nextPlayer->pathLength - 2 ) {
            
            // truncate a step ahead, to reduce chance 
            // of client-side players needing to turn-around
            // to reach this truncation point
            
            truncateMove( nextPlayer, truncationSpot + 2 );
            }                    
        }
    }

                        


void handleMapChangeToPaths( 
    int inX, int inY, ObjectRecord *inNewObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    if( inNewObject->blocksWalking ) {
    
        GridPos dropSpot = { inX, inY };
          
        int numLive = players.size();
                      
        for( int j=0; j<numLive; j++ ) {
            LiveObject *otherPlayer = 
                players.getElement( j );
            
            if( otherPlayer->error ) {
                continue;
                }

            if( otherPlayer->xd != otherPlayer->xs ||
                otherPlayer->yd != otherPlayer->ys ) {
                
                GridPos cPos = 
                    computePartialMoveSpot( otherPlayer );
                                        
                if( distance( cPos, dropSpot ) 
                    <= 2 * pathDeltaMax ) {
                                            
                    // this is close enough
                    // to this path that it might
                    // block it
                
                    int c = computePartialMovePathStep( otherPlayer );

                    // -1 means starting, pre-path pos is closest
                    // push it up to first path step
                    if( c < 0 ) {
                        c = 0;
                        }

                    char blocked = false;
                    int blockedStep = -1;
                                            
                    for( int p=c; 
                         p<otherPlayer->pathLength;
                         p++ ) {
                                                
                        if( equal( 
                                otherPlayer->
                                pathToDest[p],
                                dropSpot ) ) {
                                                    
                            blocked = true;
                            blockedStep = p;
                            break;
                            }
                        }
                                            
                    if( blocked ) {
                        printf( 
                            "  Blocked by drop\n" );
                        }
                                            

                    if( blocked &&
                        blockedStep > 0 ) {
                        
                        truncateMove( otherPlayer, blockedStep );
                        }
                    else if( blocked ) {
                        // cutting off path
                        // right at the beginning
                        // nothing left

                        // end move now
                        otherPlayer->xd = 
                            otherPlayer->xs;
                                                
                        otherPlayer->yd = 
                            otherPlayer->ys;
                             
                        otherPlayer->posForced = true;
                    
                        inPlayerIndicesToSendUpdatesAbout->push_back( j );
                        }
                    } 
                                        
                }                                    
            }
        }
    
    }




char isBiomeAllowedForPlayer( LiveObject *inPlayer, int inX, int inY,
                              char inIgnoreFloor = false );




// returns true if found
char findDropSpot( LiveObject *inDroppingPlayer,
                   int inX, int inY, int inSourceX, int inSourceY, 
                   GridPos *outSpot ) {

    int barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );

    int targetBiome = getMapBiome( inX, inY );
    int targetFloor = getMapFloor( inX, inY );
    

    if( ! isBiomeAllowedForPlayer( inDroppingPlayer, inX, inY, true ) ) {
        // would be dropping in a bad biome (floor or not)
        // avoid this if the target spot is on the edge of a bad biome

        int nX[4] = { -1, 1, 0, 0 };
        int nY[4] = { 0, 0, -1, 1 };
        
        for( int i=0; i<4; i++ ) {
            int testX = inX + nX[i];
            int testY = inY + nY[i];
            
            if( isBiomeAllowedForPlayer( inDroppingPlayer, testX, testY, 
                                         true ) ) {
                targetBiome = getMapBiome( testX, testY );
                break;
                }
            }
        }
    else {
        // target biome not bad for player
        // allow dropping in ANY good biome for player
        targetFloor = -1;
        targetBiome = -1;
        }
    
    
    char found = false;
    int foundX = inX;
    int foundY = inY;
    
    // change direction of throw
    // to match direction of 
    // drop action
    int xDir = inX - inSourceX;
    int yDir = inY - inSourceY;
                                    
        
    if( xDir == 0 && yDir == 0 ) {
        xDir = 1;
        }
    
    // cap to magnitude
    // death drops can be non-adjacent
    if( xDir > 1 ) {
        xDir = 1;
        }
    if( xDir < -1 ) {
        xDir = -1;
        }
    
    if( yDir > 1 ) {
        yDir = 1;
        }
    if( yDir < -1 ) {
        yDir = -1;
        }
        

    // check in y dir first at each
    // expanded radius?
    char yFirst = false;
        
    if( yDir != 0 ) {
        yFirst = true;
        }


    int maxR = 10;


    if( barrierOn ) {
        // don't bother with barrier checks in loop unless we are near
        // barrier edge
        if( barrierOn ) {   
            if( abs( abs( inX ) - barrierRadius ) > maxR + 2 &&
                abs( abs( inY ) - barrierRadius ) > maxR + 2 ) {
                barrierOn = false;
                }
            }
        }
    

    // pass 0, respect target floor and biome
    // pass 1 respect target biome only
    // pass 2 don't respect either
    for( int pass=0; pass<3 && !found; pass++ )
    for( int d=1; d<maxR && !found; d++ ) {
            
        char doneY0 = false;
            
        for( int yD = -d; yD<=d && !found; 
             yD++ ) {
                
            if( ! doneY0 ) {
                yD = 0;
                }
                
            if( yDir != 0 ) {
                yD *= yDir;
                }
                
            char doneX0 = false;
                
            for( int xD = -d; 
                 xD<=d && !found; 
                 xD++ ) {
                    
                if( ! doneX0 ) {
                    xD = 0;
                    }
                    
                if( xDir != 0 ) {
                    xD *= xDir;
                    }
                    
                    
                if( yD == 0 && xD == 0 ) {
                    if( ! doneX0 ) {
                        doneX0 = true;
                            
                        // back up in loop
                        xD = -d - 1;
                        }
                    continue;
                    }
                                                
                int x = 
                    inSourceX + xD;
                int y = 
                    inSourceY + yD;
                                                
                if( yFirst ) {
                    // swap them
                    // to reverse order
                    // of expansion
                    x = 
                        inSourceX + yD;
                    y =
                        inSourceY + xD;
                    }
                                                


                if( isMapSpotEmpty( x, y ) 
                    && 
                    ( pass > 1 || 
                      ( targetBiome == -1 && 
                        isBiomeAllowedForPlayer( inDroppingPlayer, x, y ) )
                      || getMapBiome( x, y ) == targetBiome ) 
                    &&
                    ( pass > 0 || 
                      targetFloor == -1 || 
                      getMapFloor( x, y ) == targetFloor ) ) {
                    
                    found = true;
                    if( barrierOn ) {    
                        if( abs( x ) >= barrierRadius ||
                            abs( y ) >= barrierRadius ) {
                            // outside barrier
                            found = false;
                            }
                        }
                    
                    if( found ) {
                        foundX = x;
                        foundY = y;
                        }
                    }
                                                    
                if( ! doneX0 ) {
                    doneX0 = true;
                                                        
                    // back up in loop
                    xD = -d - 1;
                    }
                }
                                                
            if( ! doneY0 ) {
                doneY0 = true;
                                                
                // back up in loop
                yD = -d - 1;
                }
            }
        }

    outSpot->x = foundX;
    outSpot->y = foundY;
    return found;
    }



#include "spiral.h"

GridPos findClosestEmptyMapSpot( int inX, int inY, int inMaxPointsToCheck,
                                 char *outFound ) {

    int barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );


    GridPos center = { inX, inY };

    for( int i=0; i<inMaxPointsToCheck; i++ ) {
        GridPos p = getSpriralPoint( center, i );

        char found = false;
        
        if( isMapSpotEmpty( p.x, p.y, false ) ) {    
            found = true;
            
            if( barrierOn ) {    
                if( abs( p.x ) >= barrierRadius ||
                    abs( p.y ) >= barrierRadius ) {
                    // outside barrier
                    found = false;
                    }
                }
            }
        

        if( found ) {
            *outFound = true;
            return p;
            }
        }
    
    *outFound = false;
    GridPos p = { inX, inY };
    return p;
    }



// returns NULL if not found
static LiveObject *getPlayerByEmail( char *inEmail ) {
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            otherPlayer->email != NULL &&
            strcmp( otherPlayer->email, inEmail ) == 0 ) {
            
            return otherPlayer;
            }
        }
    return NULL;
    }



static int usePersonalCurses = 0;





SimpleVector<ChangePosition> newSpeechPos;

SimpleVector<char*> newSpeechPhrases;
SimpleVector<int> newSpeechPlayerIDs;
SimpleVector<char> newSpeechCurseFlags;



SimpleVector<char*> newLocationSpeech;
SimpleVector<ChangePosition> newLocationSpeechPos;




char *isCurseNamingSay( char *inSaidString );


static void makePlayerSay( LiveObject *inPlayer, char *inToSay ) {    
                        
    if( inPlayer->lastSay != NULL ) {
        delete [] inPlayer->lastSay;
        inPlayer->lastSay = NULL;
        }
    inPlayer->lastSay = stringDuplicate( inToSay );
                        

    char isCurse = false;

    char *cursedName = isCurseNamingSay( inToSay );

    char isYouShortcut = false;
    char isBabyShortcut = false;
    if( strcmp( inToSay, curseYouPhrase ) == 0 ) {
        isYouShortcut = true;
        }
    if( strcmp( inToSay, curseBabyPhrase ) == 0 ) {
        isBabyShortcut = true;
        }

    
    if( inPlayer->isTwin ) {
        // block twins from cursing
        cursedName = NULL;
        
        isYouShortcut = false;
        isBabyShortcut = false;
        }
    
    
    
    if( cursedName != NULL || isYouShortcut ) {

        if( ! SettingsManager::getIntSetting( 
                "allowCrossLineageCursing", 0 ) ) {
            
            // cross-lineage cursing in English forbidden

            int namedPersonLineageEveID = 
                getCurseReceiverLineageEveID( cursedName );
            
            if( namedPersonLineageEveID != inPlayer->lineageEveID ) {
                // We said the curse in plain English, but
                // the named person is not in our lineage
                cursedName = NULL;
                isYouShortcut = false;
                
                // BUT, check if this cursed phrase is correct in 
                // another language below
                }
            }
        }
    

    if( cursedName != NULL ) {
        // it's a pointer into inToSay
        
        // make a copy so we can delete it later
        cursedName = stringDuplicate( cursedName );
        }


            
    int curseDistance = SettingsManager::getIntSetting( "curseDistance", 200 );
    
        
    if( ! inPlayer->isTwin &&
        cursedName == NULL &&
        players.size() >= minActivePlayersForLanguages ) {
        
        // consider cursing in other languages

        int speakerAge = computeAge( inPlayer );
        
        GridPos speakerPos = getPlayerPos( inPlayer );
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ||
                otherPlayer->lineageEveID == inPlayer->lineageEveID ) {
                continue;
                }

            if( distance( speakerPos, getPlayerPos( otherPlayer ) ) >
                curseDistance ) {
                // only consider nearby players
                continue;
                }
                
            char *translatedPhrase =
                mapLanguagePhrase( 
                    inToSay,
                    inPlayer->lineageEveID,
                    otherPlayer->lineageEveID,
                    inPlayer->id,
                    otherPlayer->id,
                    speakerAge,
                    computeAge( otherPlayer ),
                    inPlayer->parentID,
                    otherPlayer->parentID,
                    inPlayer->drunkenness / 10.0 );
            
            cursedName = isCurseNamingSay( translatedPhrase );
            
            if( strcmp( translatedPhrase, curseYouPhrase ) == 0 ) {
                // said CURSE YOU in other language
                isYouShortcut = true;
                }

            // make copy so we can delete later an delete the underlying
            // translatedPhrase now
            
            if( cursedName != NULL ) {
                cursedName = stringDuplicate( cursedName );
                }

            delete [] translatedPhrase;

            if( cursedName != NULL ) {
                int namedPersonLineageEveID = 
                    getCurseReceiverLineageEveID( cursedName );
                
                if( namedPersonLineageEveID == otherPlayer->lineageEveID ) {
                    // the named person belonged to the lineage of the 
                    // person who spoke this language!
                    break;
                    }
                // else cursed in this language, for someone outside
                // this language's line
                delete [] cursedName;
                cursedName = NULL;
                }
            }
        }



    LiveObject *youCursePlayer = NULL;
    LiveObject *babyCursePlayer = NULL;

    if( isYouShortcut ) {
        // find closest player
        GridPos speakerPos = getPlayerPos( inPlayer );
        
        LiveObject *closestOther = NULL;
        double closestDist = 9999999;
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ) {
                continue;
                }
            double dist = distance( speakerPos, getPlayerPos( otherPlayer ) );

            if( dist > getMaxChunkDimension() ) {
                // only consider nearby players
                // don't use curseDistance setting here,
                // because we don't want CURSE YOU to apply from too
                // far away (would likely be a random target player)
                continue;
                }
            if( dist < closestDist ) {
                closestDist = dist;
                closestOther = otherPlayer;
                }
            }


        if( closestOther != NULL ) {
            youCursePlayer = closestOther;
            
            if( cursedName != NULL ) {
                delete [] cursedName;
                cursedName = NULL;
                }

            if( youCursePlayer->name != NULL ) {
                // allow name-based curse to go through, if possible
                cursedName = stringDuplicate( youCursePlayer->name );
                }
            }
        }
    else if( isBabyShortcut ) {
        // this case is more robust (below) by simply using the lastBabyEmail
        // in all cases

        // That way there's no confusing about who MY BABY is (always the
        // most-recent baby).
        }


    // make sure, no matter what, we can't curse living 
    // people at a great distance
    // note that, sice we're not tracking dead people here
    // that case will be caught below, since the curses.h tracks death
    // locations
    GridPos speakerPos = getPlayerPos( inPlayer );
    
    if( cursedName != NULL &&
        strcmp( cursedName, "" ) != 0 ) {

        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ) {
                continue;
                }
            if( otherPlayer->name != NULL &&
                strcmp( otherPlayer->name, cursedName ) == 0 ) {
                // matching player
                
                double dist = 
                    distance( speakerPos, getPlayerPos( otherPlayer ) );

                if( dist > curseDistance ) {
                    // too far
                    delete [] cursedName;
                    cursedName = NULL;
                    }
                break;
                }
            }
        }
    
    
    char *dbCurseTargetEmail = NULL;

    
    char canCurse = false;
    
    if( inPlayer->curseTokenCount > 0 ) {
        canCurse = true;
        }
    

    if( canCurse && 
        cursedName != NULL && 
        strcmp( cursedName, "" ) != 0 ) {
        
        isCurse = cursePlayer( inPlayer->id,
                               inPlayer->lineageEveID,
                               inPlayer->email,
                               speakerPos,
                               curseDistance,
                               cursedName );
        
        if( isCurse ) {
            char *targetEmail = getCurseReceiverEmail( cursedName );
            if( targetEmail != NULL ) {
                setDBCurse( inPlayer->email, targetEmail );
                dbCurseTargetEmail = targetEmail;
                }
            }
        }
    
    
    if( cursedName != NULL ) {
        delete [] cursedName;
        }
    

    if( canCurse && !isCurse ) {
        // named curse didn't happen above
        // maybe we used a shortcut, and target didn't have name
        
        if( isYouShortcut && youCursePlayer != NULL &&
            spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            setDBCurse( inPlayer->email, youCursePlayer->email );
            dbCurseTargetEmail = youCursePlayer->email;
            }
        else if( isBabyShortcut && babyCursePlayer != NULL &&
            spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            char *targetEmail = babyCursePlayer->email;
            
            if( strcmp( targetEmail, "email_cleared" ) == 0 ) {
                // deleted players allowed here
                targetEmail = babyCursePlayer->origEmail;
                }
            if( targetEmail != NULL ) {
                setDBCurse( inPlayer->email, targetEmail );
                dbCurseTargetEmail = targetEmail;
                }
            }
        else if( isBabyShortcut && babyCursePlayer == NULL &&
                 inPlayer->lastBabyEmail != NULL &&
                 spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            
            setDBCurse( inPlayer->email, inPlayer->lastBabyEmail );
            dbCurseTargetEmail = inPlayer->lastBabyEmail;
            }
        }

    if( dbCurseTargetEmail != NULL && usePersonalCurses ) {
        LiveObject *targetP = getPlayerByEmail( dbCurseTargetEmail );
        
        if( targetP != NULL ) {
            char *message = autoSprintf( "CU\n%d 1 %s_%s\n#", targetP->id,
                                         getCurseWord( inPlayer->email,
                                                       targetP->email, 0 ),
                                         getCurseWord( inPlayer->email,
                                                       targetP->email, 1 ) );
            sendMessageToPlayer( inPlayer,
                                 message, strlen( message ) );
            delete [] message;
            }
        }
    


    if( isCurse ) {
        if( ! inPlayer->isTwin && 
            inPlayer->curseStatus.curseLevel == 0 &&
            hasCurseToken( inPlayer->email ) ) {
            inPlayer->curseTokenCount = 1;
            }
        else {
            inPlayer->curseTokenCount = 0;
            }
        inPlayer->curseTokenUpdate = true;
        }

    

    int curseFlag = 0;
    if( isCurse ) {
        curseFlag = 1;
        }
    

    newSpeechPhrases.push_back( stringDuplicate( inToSay ) );
    newSpeechCurseFlags.push_back( curseFlag );
    newSpeechPlayerIDs.push_back( inPlayer->id );

                        
    ChangePosition p = { inPlayer->xd, inPlayer->yd, false };
                        
    // if held, speech happens where held
    if( inPlayer->heldByOther ) {
        LiveObject *holdingPlayer = 
            getLiveObject( inPlayer->heldByOtherID );
                
        if( holdingPlayer != NULL ) {
            p.x = holdingPlayer->xd;
            p.y = holdingPlayer->yd;
            }
        }

    newSpeechPos.push_back( p );



    SimpleVector<int> pipesIn;
    GridPos playerPos = getPlayerPos( inPlayer );
    
    
    if( inPlayer->heldByOther ) {    
        LiveObject *holdingPlayer = 
            getLiveObject( inPlayer->heldByOtherID );
                
        if( holdingPlayer != NULL ) {
            playerPos = getPlayerPos( holdingPlayer );
            }
        }
    
    getSpeechPipesIn( playerPos.x, playerPos.y, &pipesIn );
    
    if( pipesIn.size() > 0 ) {
        for( int p=0; p<pipesIn.size(); p++ ) {
            int pipeIndex = pipesIn.getElementDirect( p );

            SimpleVector<GridPos> *pipesOut = getSpeechPipesOut( pipeIndex );

            for( int i=0; i<pipesOut->size(); i++ ) {
                GridPos outPos = pipesOut->getElementDirect( i );
                
                newLocationSpeech.push_back( stringDuplicate( inToSay ) );
                
                ChangePosition outChangePos = { outPos.x, outPos.y, false };
                newLocationSpeechPos.push_back( outChangePos );
                }
            }
        }
    }


static void forcePlayerToRead( LiveObject *inPlayer,
                               int inObjectID ) {
            
    char metaData[ MAP_METADATA_LENGTH ];
    char found = getMetadata( inObjectID, 
                              (unsigned char*)metaData );

    if( found ) {
        // read what they picked up, subject to limit
                
        unsigned int sayLimit = getSayLimit( inPlayer );
        
        if( computeAge( inPlayer ) < 10 &&
            strlen( metaData ) > sayLimit ) {
            // truncate with ...
            metaData[ sayLimit ] = '.';
            metaData[ sayLimit + 1 ] = '.';
            metaData[ sayLimit + 2 ] = '.';
            metaData[ sayLimit + 3 ] = '\0';
            
            // watch for truncated map metadata
            // trim it off (too young to read maps)
            char *starLoc = strstr( metaData, " *" );
            
            if( starLoc != NULL ) {
                starLoc[0] = '\0';
                }
            }
        char *quotedPhrase = autoSprintf( ":%s", metaData );
        makePlayerSay( inPlayer, quotedPhrase );
        delete [] quotedPhrase;
        }
    }



char canPlayerUseTool( LiveObject *inPlayer, int inToolID );





// sends message to player about nearby players who already know
// the unlearned tool that they are holding
// (or the ground tool they just tried to use, if inGroundToolID != -1 )
static void sendToolExpertMessage( LiveObject *inPlayer, 
                                   int inGroundToolID = -1 ) {    
    int toolID = inGroundToolID;
    
    if( toolID == -1 ) {
        toolID = inPlayer->holdingID;
        }
    
    if( toolID <= 0 ) {
        return;
        }
    
    GridPos playerPos = getPlayerPos( inPlayer );
    
    SimpleVector<int> matchIDs;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *otherPlayer = players.getElement( i );

        if( otherPlayer->error ) {
            continue;
            }
        
        if( otherPlayer == inPlayer ) {
            continue;
            }
        
        if( otherPlayer->heldByOther ) {
            // ghost position of a held baby
            continue;
            }

        double d = distance( playerPos, getPlayerPos( otherPlayer ) );
        
        if( d <= maxSpeechRadius ) {
            
            if( canPlayerUseTool( otherPlayer, toolID ) ) {
                matchIDs.push_back( otherPlayer->id );
                }
            }
        }
    
    if( matchIDs.size() == 0 ) {
        return;
        }

    SimpleVector<char> messageWorking;
    
    messageWorking.appendElementString( "TE\n" );
    
    for( int i=0; i<matchIDs.size(); i++ ) {
        char *idString = autoSprintf( "%d", matchIDs.getElementDirect( i ) );
        
        if( i > 0 ) {
            messageWorking.appendElementString( " " );
            }
        messageWorking.appendElementString( idString );
        delete [] idString;
        }
    messageWorking.appendElementString( "\n#" );

    char *message = messageWorking.getElementString();
    
    sendMessageToPlayer( inPlayer, message, messageWorking.size() );
    
    delete [] message;
    }




void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer,
                 SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );



void makePlayerBiomeSick( LiveObject *nextPlayer, 
                          int sicknessObjectID );



static void holdingSomethingNew( LiveObject *inPlayer, 
                                 int inOldHoldingID = 0 ) {
    if( inPlayer->holdingID > 0 ) {
        
        if( ! canPlayerUseTool( inPlayer, inPlayer->holdingID ) ) {
            sendToolExpertMessage( inPlayer );
            }

        ObjectRecord *o = getObject( inPlayer->holdingID );
        
        ObjectRecord *oldO = NULL;
        if( inOldHoldingID > 0 ) {
            oldO = getObject( inOldHoldingID );
            }
        
        if( o->written &&
            ( oldO == NULL ||
              ! ( oldO->written || oldO->writable ) ) ) {

            forcePlayerToRead( inPlayer, inPlayer->holdingID );
            }

        if( o->isFlying ) {
            inPlayer->holdingFlightObject = true;
            }
        else {
            inPlayer->holdingFlightObject = false;
            }
        }
    else {
        inPlayer->holdingFlightObject = false;
        }

    if( inOldHoldingID > 0 && getObject( inOldHoldingID )->rideable &&
        ( inPlayer->holdingID <= 0 || 
          ! getObject( inPlayer->holdingID )->rideable ) ) {
        
        // check if they are now getting biome sick after dismount
        int sicknessObjectID = 
            getBiomeSickness( 
                inPlayer->displayID, 
                inPlayer->xd,
                inPlayer->yd );
        
        if( sicknessObjectID != -1 ) {

            if( inPlayer->holdingID != 0 ) {
                GridPos p = getPlayerPos( inPlayer );
                handleDrop( 
                    p.x, p.y, inPlayer, NULL );
                }
            
            makePlayerBiomeSick( inPlayer, 
                                 sicknessObjectID );
            }
        }
    }




static SimpleVector<GraveInfo> newGraves;
static SimpleVector<GraveMoveInfo> newGraveMoves;



static int isGraveSwapDest( int inTargetX, int inTargetY,
                            int inDroppingPlayerID ) {
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->error || o->id == inDroppingPlayerID ) {
            continue;
            }
        
        if( o->holdingID > 0 && strstr( getObject( o->holdingID )->description,
                                        "origGrave" ) != NULL ) {
            
            if( inTargetX == o->heldGraveOriginX &&
                inTargetY == o->heldGraveOriginY ) {
                return true;
                }
            }
        }
    
    return false;
    }



// drops an object held by a player at target x,y location
// doesn't check for adjacency (so works for thrown drops too)
// if target spot blocked, will search for empty spot to throw object into
// if inPlayerIndicesToSendUpdatesAbout is NULL, it is ignored
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer,
                 SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    
    if( ! isBiomeAllowedForPlayer( inDroppingPlayer, inX, inY, false ) ) {
        // would be dropping in a bad biome (not on floor)
        // avoid this if the target spot is on the edge of a bad biome

        int nX[4] = { -1, 1, 0, 0 };
        int nY[4] = { 0, 0, -1, 1 };
        
        for( int i=0; i<4; i++ ) {
            int testX = inX + nX[i];
            int testY = inY + nY[i];
            
            if( isBiomeAllowedForPlayer( inDroppingPlayer, testX, testY,
                                         false ) ) {
                inX = testX;
                inY = testY;
                
                break;
                }
            }
        }


    int oldHoldingID = inDroppingPlayer->holdingID;
    

    if( oldHoldingID > 0 &&
        getObject( oldHoldingID )->permanent ) {
        // what they are holding is stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  dismounting
        // a horse)
                            
        // note that if use on bare ground
        // also has a new actor, that will be lost
        // in this process.
        // (example:  holding a roped lamb when dying,
        //            lamb is dropped, rope is lost)

        TransRecord *bareTrans =
            getPTrans( oldHoldingID, -1 );
                            

        if( bareTrans == NULL ||
            bareTrans->newTarget == 0 ) {
            // no immediate bare ground trans
            // check if there's a timer transition for this held object
            // (like cast fishing pole)
            // and force-run that transition now
            TransRecord *timeTrans = getPTrans( -1, oldHoldingID );
            
            if( timeTrans != NULL && timeTrans->newTarget != 0 ) {
                oldHoldingID = timeTrans->newTarget;
            
                inDroppingPlayer->holdingID = 
                    timeTrans->newTarget;
                holdingSomethingNew( inDroppingPlayer, oldHoldingID );

                setFreshEtaDecayForHeld( inDroppingPlayer );
                }

            if( getObject( oldHoldingID )->permanent ) {
                // still permanent after timed trans
                
                // check again for a bare ground trans
                bareTrans =
                    getPTrans( oldHoldingID, -1 );
                }
            }
        

        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 ) {
            
            if( bareTrans->newActor > 0 ) {
                // something would be left in hand
                
                // throw it down first
                inDroppingPlayer->holdingID = bareTrans->newActor;
                setFreshEtaDecayForHeld( inDroppingPlayer );
                handleDrop( inX, inY, inDroppingPlayer, 
                            inPlayerIndicesToSendUpdatesAbout );
                }

            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }
    else if( oldHoldingID > 0 &&
             ! getObject( oldHoldingID )->permanent ) {
        // what they are holding is NOT stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  getting wounded while holding a goose)
                            
        // do not consider doing this if use-on-bare-ground leaves something
        // in the hand

        TransRecord *bareTrans =
            getPTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 &&
            bareTrans->newActor == 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }

    int targetX = inX;
    int targetY = inY;

    int mapID = getMapObject( inX, inY );
    char mapSpotBlocking = false;
    if( mapID > 0 ) {
        mapSpotBlocking = getObject( mapID )->blocksWalking;
        }
    

    if( ( inDroppingPlayer->holdingID < 0 && mapSpotBlocking )
        ||
        ( inDroppingPlayer->holdingID > 0 && mapID != 0 ) ) {
        
        // drop spot blocked
        // search for another
        // throw held into nearest empty spot
                                    
        
        GridPos spot;

        GridPos playerPos = getPlayerPos( inDroppingPlayer );
        
        char found = findDropSpot( inDroppingPlayer, inX, inY, 
                                   playerPos.x, playerPos.y,
                                   &spot );
        
        int foundX = spot.x;
        int foundY = spot.y;



        if( found && inDroppingPlayer->holdingID > 0 ) {
            targetX = foundX;
            targetY = foundY;
            }
        else {
            // no place to drop it, it disappears

            // OR we're holding a baby,
            // then just put the baby where we are
            // (don't ever throw babies, that's weird and exploitable)
            if( inDroppingPlayer->holdingID < 0 ) {
                int babyID = - inDroppingPlayer->holdingID;
                
                LiveObject *babyO = getLiveObject( babyID );
                
                if( babyO != NULL ) {
                    babyO->xd = inDroppingPlayer->xd;
                    babyO->xs = inDroppingPlayer->xd;
                    
                    babyO->yd = inDroppingPlayer->yd;
                    babyO->ys = inDroppingPlayer->yd;

                    babyO->heldByOther = false;

                    if( isFertileAge( inDroppingPlayer ) ) {    
                        // reset food decrement time
                        babyO->foodDecrementETASeconds =
                            Time::getCurrentTime() +
                            computeFoodDecrementTimeSeconds( babyO );
                        }
                    
                    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
                        inPlayerIndicesToSendUpdatesAbout->push_back( 
                            getLiveObjectIndex( babyID ) );
                        }
                    
                    }
                
                }
            
            inDroppingPlayer->holdingID = 0;
            inDroppingPlayer->holdingEtaDecay = 0;
            inDroppingPlayer->heldOriginValid = 0;
            inDroppingPlayer->heldOriginX = 0;
            inDroppingPlayer->heldOriginY = 0;
            inDroppingPlayer->heldTransitionSourceID = -1;
            
            if( inDroppingPlayer->numContained != 0 ) {
                clearPlayerHeldContained( inDroppingPlayer );
                }
            return;
            }            
        }
    
    
    if( inDroppingPlayer->holdingID < 0 ) {
        // dropping a baby
        
        int babyID = - inDroppingPlayer->holdingID;
                
        LiveObject *babyO = getLiveObject( babyID );
        
        if( babyO != NULL ) {
            babyO->xd = targetX;
            babyO->xs = targetX;
                    
            babyO->yd = targetY;
            babyO->ys = targetY;
            
            babyO->heldByOther = false;
            
            // force baby pos
            // baby can wriggle out of arms in same server step that it was
            // picked up.  In that case, the clients will never get the
            // message that the baby was picked up.  The baby client could
            // be in the middle of a client-side move, and we need to force
            // them back to their true position.
            babyO->posForced = true;
            
            if( isFertileAge( inDroppingPlayer ) ) {    
                // reset food decrement time
                babyO->foodDecrementETASeconds =
                    Time::getCurrentTime() +
                    computeFoodDecrementTimeSeconds( babyO );
                }

            if( inPlayerIndicesToSendUpdatesAbout != NULL ) {
                inPlayerIndicesToSendUpdatesAbout->push_back( 
                    getLiveObjectIndex( babyID ) );
                }
            }
        
        inDroppingPlayer->holdingID = 0;
        inDroppingPlayer->holdingEtaDecay = 0;
        inDroppingPlayer->heldOriginValid = 0;
        inDroppingPlayer->heldOriginX = 0;
        inDroppingPlayer->heldOriginY = 0;
        inDroppingPlayer->heldTransitionSourceID = -1;
        
        return;
        }
    
    setResponsiblePlayer( inDroppingPlayer->id );
    
    ObjectRecord *o = getObject( inDroppingPlayer->holdingID );
                                
    if( strstr( o->description, "origGrave" ) 
        != NULL ) {
                                    
        setGravePlayerID( 
            targetX, targetY, inDroppingPlayer->heldGravePlayerID );
        
        int swapDest = isGraveSwapDest( targetX, targetY, 
                                        inDroppingPlayer->id );
        
        // see if another player has target location in air


        GraveMoveInfo g = { 
            { inDroppingPlayer->heldGraveOriginX,
              inDroppingPlayer->heldGraveOriginY },
            { targetX,
              targetY },
            swapDest };
        newGraveMoves.push_back( g );
        }


    setMapObject( targetX, targetY, inDroppingPlayer->holdingID );
    setEtaDecay( targetX, targetY, inDroppingPlayer->holdingEtaDecay );

    transferHeldContainedToMap( inDroppingPlayer, targetX, targetY );
    
                                

    setResponsiblePlayer( -1 );
                                
    inDroppingPlayer->holdingID = 0;
    inDroppingPlayer->holdingEtaDecay = 0;
    inDroppingPlayer->heldOriginValid = 0;
    inDroppingPlayer->heldOriginX = 0;
    inDroppingPlayer->heldOriginY = 0;
    inDroppingPlayer->heldTransitionSourceID = -1;
                                
    // watch out for truncations of in-progress
    // moves of other players
            
    ObjectRecord *droppedObject = getObject( oldHoldingID );
   
    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
        handleMapChangeToPaths( targetX, targetY, droppedObject,
                                inPlayerIndicesToSendUpdatesAbout );
        }
    
    
    }



LiveObject *getAdultHolding( LiveObject *inBabyObject ) {
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {
            return adultO;
            }
        }
    return NULL;
    }



void handleForcedBabyDrop( 
    LiveObject *inBabyObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {

            // don't need to send update about this adult's
            // holding status.
            // the update sent about the baby will inform clients
            // that the baby is no longer held by this adult
            //inPlayerIndicesToSendUpdatesAbout->push_back( j );
            
            GridPos dropPos;
            
            if( adultO->xd == 
                adultO->xs &&
                adultO->yd ==
                adultO->ys ) {
                
                dropPos.x = adultO->xd;
                dropPos.y = adultO->yd;
                }
            else {
                dropPos = 
                    computePartialMoveSpot( adultO );
                }
            
            
            handleDrop( 
                dropPos.x, dropPos.y, 
                adultO,
                inPlayerIndicesToSendUpdatesAbout );

            
            break;
            }
        }
    }



static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID );



static void swapHeldWithGround( 
    LiveObject *inPlayer, int inTargetID, 
    int inMapX, int inMapY,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout) {
    
    
    if( inTargetID == inPlayer->holdingID &&
        inPlayer->numContained == 0 &&
        getNumContained( inMapX, inMapY ) == 0 ) {
        // swap of same non-container object with self
        // ignore this, to prevent weird case of swapping
        // grave basket with self
        return;
        }
    

    timeSec_t newHoldingEtaDecay = getEtaDecay( inMapX, inMapY );
    
    FullMapContained f = getFullMapContained( inMapX, inMapY );


    int gravePlayerID = getGravePlayerID( inMapX, inMapY );
        
    if( gravePlayerID > 0 ) {
            
        // player action actually picked up this grave
        
        // clear it from ground
        setGravePlayerID( inMapX, inMapY, 0 );
        }

    
    clearAllContained( inMapX, inMapY );
    setMapObject( inMapX, inMapY, 0 );
    
    handleDrop( inMapX, inMapY, inPlayer, inPlayerIndicesToSendUpdatesAbout );
    
    
    inPlayer->holdingID = inTargetID;
    inPlayer->holdingEtaDecay = newHoldingEtaDecay;
    
    setContained( inPlayer, f );


    // does bare-hand action apply to this newly-held object
    // one that results in something new in the hand and
    // nothing on the ground?
    
    // if so, it is a pick-up action, and it should apply here
    
    TransRecord *pickupTrans = getPTrans( 0, inTargetID );

    char newHandled = false;
                
    if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
        pickupTrans->newTarget == 0 ) {
                    
        int newTargetID = pickupTrans->newActor;
        
        if( newTargetID != inTargetID ) {
            handleHoldingChange( inPlayer, newTargetID );
            newHandled = true;
            }
        }
    
    if( ! newHandled ) {
        holdingSomethingNew( inPlayer );
        }
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inMapX;
    inPlayer->heldOriginY = inMapY;
    inPlayer->heldTransitionSourceID = -1;


    inPlayer->heldGravePlayerID = 0;

    if( inPlayer->holdingID > 0 &&
        strstr( getObject( inPlayer->holdingID )->description, 
                "origGrave" ) != NULL &&
        gravePlayerID > 0 ) {
    
        inPlayer->heldGraveOriginX = inMapX;
        inPlayer->heldGraveOriginY = inMapY;
        inPlayer->heldGravePlayerID = gravePlayerID;
        }
    }









// returns 0 for NULL
static int objectRecordToID( ObjectRecord *inRecord ) {
    if( inRecord == NULL ) {
        return 0;
        }
    else {
        return inRecord->id;
        }
    }



typedef struct UpdateRecord{
        char *formatString;
        char posUsed;
        int absolutePosX, absolutePosY;
        GridPos absoluteActionTarget;
        int absoluteHeldOriginX, absoluteHeldOriginY;
    } UpdateRecord;



static char *getUpdateLineFromRecord( 
    UpdateRecord *inRecord, GridPos inRelativeToPos, GridPos inObserverPos ) {
    
    if( inRecord->posUsed ) {
        
        GridPos updatePos = { inRecord->absolutePosX, inRecord->absolutePosY };
        
        if( distance( updatePos, inObserverPos ) > 
            getMaxChunkDimension() * 2 ) {
            
            // this update is for a far-away player
            
            // put dummy positions in to hide their coordinates
            // so that people sniffing the protocol can't get relative
            // location information
            
            return autoSprintf( inRecord->formatString,
                                1977, 1977,
                                1977, 1977,
                                1977, 1977 );
            }


        return autoSprintf( inRecord->formatString,
                            inRecord->absoluteActionTarget.x 
                            - inRelativeToPos.x,
                            inRecord->absoluteActionTarget.y 
                            - inRelativeToPos.y,
                            inRecord->absoluteHeldOriginX - inRelativeToPos.x, 
                            inRecord->absoluteHeldOriginY - inRelativeToPos.y,
                            inRecord->absolutePosX - inRelativeToPos.x, 
                            inRecord->absolutePosY - inRelativeToPos.y );
        }
    else {
        // posUsed false only if thise is a DELETE PU message
        // set all positions to 0 in that case
        return autoSprintf( inRecord->formatString,
                            0, 0,
                            0, 0 );
        }
    }



static SimpleVector<int> newEmotPlayerIDs;
static SimpleVector<int> newEmotIndices;
// 0 if no ttl specified
static SimpleVector<int> newEmotTTLs;



static int getEatBonus( LiveObject *inPlayer ) {
    int generation = inPlayer->parentChainLength - 1;
    
    int b = lrint( 
        ( eatBonus - eatBonusFloor ) * 
        pow( 0.5, 
             generation / eatBonusHalfLife )
        + eatBonusFloor );
    
    b += inPlayer->personalEatBonus;

    return b;
    }



static int getEatCost( LiveObject *inPlayer ) {

    if( eatCostMax == 0 ||
        eatCostGrowthRate == 0 ) {
        return 0;
        }

    // using P(t) form of logistic function from here:
    // https://en.wikipedia.org/wiki/Logistic_function#
    //         In_ecology:_modeling_population_growth


    int generation = inPlayer->parentChainLength - 1;

    // P(0) is 1, so we subtract 1 from the result value.
    // but add 1 to max param
    double K = eatCostMax + 1;

    double costFloat = 
        K / 
        ( 1 + ( K - 1 ) * 
          pow( M_E, -eatCostGrowthRate * generation ) );
    
    int cost = lrint( costFloat - 1 );

    return cost;
    }



static double getLinearFoodScaleFactor( LiveObject *inPlayer ) {
    
    if( foodScaleFactor == foodScaleFactorFloor ) {
        return foodScaleFactor;
        }

    int generation = inPlayer->parentChainLength - 1;
    
    double f = ( foodScaleFactor - foodScaleFactorFloor ) * 
        pow( 0.5, 
             generation / foodScaleFactorHalfLife )
        + foodScaleFactorFloor;
    
    return f;
    }


static int getScaledFoodValue( LiveObject *inPlayer, int inFoodValue ) {
    int v = inFoodValue;
    
    if( foodScaleFactorGamma == 1 ) {
        v = ceil( getLinearFoodScaleFactor( inPlayer ) * inFoodValue );
        }
    else {
        // apply half-life to gamma
        // gamma starts at 1.0 and approaches foodScaleFactorGamma over time
        // getting half-way there after foodScaleFactorHalfLife generations
        int generation = inPlayer->parentChainLength - 1;

        double h = pow( 0.5, 
                        generation / foodScaleFactorHalfLife );
        double g = h * 1.0 + (1-h) * foodScaleFactorGamma;
        
        int maxFoodValue = getMaxFoodValue();
        
        double scaledValue = inFoodValue / (double)maxFoodValue;
        
        double powerValue = pow( scaledValue, g );
        
        double rescaledValue = maxFoodValue * powerValue;
        
        // apply linear factor at end
        v = 
            ceil( getLinearFoodScaleFactor( inPlayer ) * rescaledValue );
        }

    if( v > 1 ) {
        v -= getEatCost( inPlayer );

        if( v < 1 ) {
            v = 1;
            }
        }
    
    return v;
    }




static char isYummy( LiveObject *inPlayer, int inObjectID ) {
    ObjectRecord *o = getObject( inObjectID );
    
    if( o->isUseDummy ) {
        inObjectID = o->useDummyParent;
        o = getObject( inObjectID );
        }

    if( o->foodValue == 0 ) {
        return false;
        }


    int origID = inObjectID;

    if( o->yumParentID != -1 ) {
        // set this whether valid or not
        inObjectID = o->yumParentID;
        
        // NOTE:
        // we're NOT replacing o with the yumParent object
        // because o isn't used beyond this point
        }   
    
    
    // don't consider yumParent when testing for craving satisfaction
    if( origID == inPlayer->cravingFood.foodID &&
        computeAge( inPlayer ) >= minAgeForCravings ) {
        return true;
        }

    for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
        if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
            return false;
            }
        }
    return true;
    }



static void updateYum( LiveObject *inPlayer, int inFoodEatenID,
                       char inFedSelf = true ) {

    char wasYummy = true;
    
    if( ! isYummy( inPlayer, inFoodEatenID ) ) {
        wasYummy = false;
        
        // chain broken
        
        // only feeding self can break chain
        if( inFedSelf && canYumChainBreak ) {
            inPlayer->yummyFoodChain.deleteAll();
            }
        }
    
    
    ObjectRecord *o = getObject( inFoodEatenID );
    
    if( o->isUseDummy ) {
        inFoodEatenID = o->useDummyParent;
        }
    
    
    // add to chain
    // might be starting a new chain
    // (do this if fed yummy food by other player too)
    if( wasYummy ||
        inPlayer->yummyFoodChain.size() == 0 ) {
        
        int eatenID = inFoodEatenID;

        ObjectRecord *eatenO = getObject( inFoodEatenID );
        
        if( eatenO->yumParentID != -1 ) {
            // this may or may not be a valid object id
            // doesn't matter, because it's never used as an object ID
            // just as a unique food ID in the yummyFoodChain list
            eatenID = eatenO->yumParentID;
            }

        inPlayer->yummyFoodChain.push_back( eatenID );
        
        // don't consider yumParent when testing for craving satisfaction
        if( inFoodEatenID == inPlayer->cravingFood.foodID &&
            computeAge( inPlayer ) >= minAgeForCravings ) {
            
            for( int i=0; i< inPlayer->cravingFoodYumIncrement; i++ ) {
                // add extra copies to YUM chain as a bonus
                inPlayer->yummyFoodChain.push_back( eatenID );
                }
            
            // craving satisfied, go on to next thing in list
            inPlayer->cravingFood = 
                getCravedFood( inPlayer->lineageEveID,
                               inPlayer->parentChainLength,
                               inPlayer->cravingFood );
            // reset generational bonus counter
            inPlayer->cravingFoodYumIncrement = 1;
            
            // flag them for getting a new craving message
            inPlayer->cravingKnown = false;
            
            // satisfied emot
            
            if( satisfiedEmotionIndex != -1 ) {
                inPlayer->emotFrozen = false;
                inPlayer->emotUnfreezeETA = 0;
        
                newEmotPlayerIDs.push_back( inPlayer->id );
                
                newEmotIndices.push_back( satisfiedEmotionIndex );
                // 3 sec
                newEmotTTLs.push_back( 1 );
                
                // don't leave starving status, or else non-starving
                // change might override our satisfied emote
                inPlayer->starving = false;
                }
            }
        }
    

    int currentBonus = inPlayer->yummyFoodChain.size() - 1;

    if( currentBonus < 0 ) {
        currentBonus = 0;
        }    
    
    if( yumBonusCap != -1 &&
        currentBonus > yumBonusCap ) {
        currentBonus = yumBonusCap;
        }

    if( wasYummy ) {
        // only get bonus if actually was yummy (whether fed self or not)
        // chain not broken if fed non-yummy by other, but don't get bonus
        
        // for now, do NOT scale current bonus
        // it's confusing to see a yum multiplier on the screen and then
        // not receive that number of bonus food points.
        inPlayer->yummyBonusStore += currentBonus;
        }
    
    }




// returns -1 if not in a tool set
int getToolSet( int inToolID ) {
    ObjectRecord *toolO = getObject( inToolID );
    
    // is it a marked tool?
    int toolSet = toolO->toolSetIndex;
    
    return toolSet;
    }



char canPlayerUseTool( LiveObject *inPlayer, int inToolID ) {
    ObjectRecord *toolO = getObject( inToolID );
                                    
    // is it a marked tool?
    int toolSet = toolO->toolSetIndex;
    
    if( toolSet != -1 &&
        inPlayer->learnedTools.getElementIndex( toolSet ) == -1 ) {
        // not in player's learned tool set
        return false;
        }
    
    return true;
    }



static UpdateRecord getUpdateRecord( 
    LiveObject *inPlayer,
    char inDelete,
    char inPartial = false ) {

    char *holdingString = getHoldingString( inPlayer );
    
    // this is 0 if still in motion (mid-move update)
    int doneMoving = 0;
    
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd &&
        ! inPlayer->heldByOther ) {
        // not moving
        doneMoving = inPlayer->lastMoveSequenceNumber;
        }
    
    char midMove = false;
    
    if( inPartial || 
        inPlayer->xs != inPlayer->xd ||
        inPlayer->ys != inPlayer->yd ) {
        
        midMove = true;
        }
    

    UpdateRecord r;
        

    char *posString;
    if( inDelete ) {
        posString = stringDuplicate( "0 0 X X" );
        r.posUsed = false;
        }
    else {
        int x, y;

        r.posUsed = true;

        if( doneMoving > 0 || ! midMove ) {
            x = inPlayer->xs;
            y = inPlayer->ys;
            }
        else {
            // mid-move, and partial position requested
            GridPos p = computePartialMoveSpot( inPlayer );
            
            x = p.x;
            y = p.y;
            }
        
        posString = autoSprintf( "%d %d %%d %%d",          
                                 doneMoving,
                                 inPlayer->posForced );
        r.absolutePosX = x;
        r.absolutePosY = y;
        }
    
    SimpleVector<char> clothingListBuffer;
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        ObjectRecord *cObj = clothingByIndex( inPlayer->clothing, c );
        int id = 0;
        
        if( cObj != NULL ) {
            id = objectRecordToID( cObj );
            }
        
        char *idString = autoSprintf( "%d", hideIDForClient( id ) );
        
        clothingListBuffer.appendElementString( idString );
        delete [] idString;
        
        if( cObj != NULL && cObj->numSlots > 0 ) {
            
            for( int cc=0; cc<inPlayer->clothingContained[c].size(); cc++ ) {
                char *contString = 
                    autoSprintf( 
                        ",%d", 
                        hideIDForClient( 
                            inPlayer->
                            clothingContained[c].getElementDirect( cc ) ) );
                
                clothingListBuffer.appendElementString( contString );
                delete [] contString;
                }
            }

        if( c < NUM_CLOTHING_PIECES - 1 ) {
            clothingListBuffer.push_back( ';' );
            }
        }
    
    char *clothingList = clothingListBuffer.getElementString();


    char *deathReason;
    
    if( inDelete && inPlayer->deathReason != NULL ) {
        deathReason = stringDuplicate( inPlayer->deathReason );
        }
    else {
        deathReason = stringDuplicate( "" );
        }
    
    
    int heldYum = 0;
    
    if( inPlayer->holdingID > 0 &&
        isYummy( inPlayer, inPlayer->holdingID ) ) {
        heldYum = 1;
        }

    int heldLearned = 0;
    
    if( inPlayer->holdingID > 0 &&
        canPlayerUseTool( inPlayer, inPlayer->holdingID ) ) {
        heldLearned = 1;
        }
        

    r.formatString = autoSprintf( 
        "%d %d %d %d %%d %%d %s %d %%d %%d %d "
        "%.2f %s %.2f %.2f %.2f %s %d %d %d %d %d%s\n",
        inPlayer->id,
        inPlayer->displayID,
        inPlayer->facingOverride,
        inPlayer->actionAttempt,
        //inPlayer->actionTarget.x - inRelativeToPos.x,
        //inPlayer->actionTarget.y - inRelativeToPos.y,
        holdingString,
        inPlayer->heldOriginValid,
        //inPlayer->heldOriginX - inRelativeToPos.x,
        //inPlayer->heldOriginY - inRelativeToPos.y,
        hideIDForClient( inPlayer->heldTransitionSourceID ),
        inPlayer->heat,
        posString,
        computeAge( inPlayer ),
        1.0 / getAgeRate(),
        computeMoveSpeed( inPlayer ),
        clothingList,
        inPlayer->justAte,
        hideIDForClient( inPlayer->justAteID ),
        inPlayer->responsiblePlayerID,
        heldYum,
        heldLearned,
        deathReason );
    
    delete [] deathReason;
    

    r.absoluteActionTarget = inPlayer->actionTarget;
    
    if( inPlayer->heldOriginValid ) {
        r.absoluteHeldOriginX = inPlayer->heldOriginX;
        r.absoluteHeldOriginY = inPlayer->heldOriginY;
        }
    else {
        // we set 0,0 to clear held origins in many places in the code
        // if we leave that as an absolute pos, our birth pos leaks through
        // when we make it birth-pos relative
        
        // instead, substitute our birth pos for all invalid held pos coords
        // to prevent this
        r.absoluteHeldOriginX = inPlayer->birthPos.x;
        r.absoluteHeldOriginY = inPlayer->birthPos.y;
        }
    
        

    inPlayer->justAte = false;
    inPlayer->justAteID = 0;
    
    // held origin only valid once
    inPlayer->heldOriginValid = 0;
    
    inPlayer->facingOverride = 0;
    inPlayer->actionAttempt = 0;

    delete [] holdingString;
    delete [] posString;
    delete [] clothingList;
    
    return r;
    }



// inDelete true to send X X for position
// inPartial gets update line for player's current possition mid-path
// positions in update line will be relative to inRelativeToPos
static char *getUpdateLine( LiveObject *inPlayer, GridPos inRelativeToPos,
                            GridPos inObserverPos,
                            char inDelete,
                            char inPartial = false ) {
    
    UpdateRecord r = getUpdateRecord( inPlayer, inDelete, inPartial );
    
    char *line = getUpdateLineFromRecord( &r, inRelativeToPos, inObserverPos );

    delete [] r.formatString;
    
    return line;
    }




// if inTargetID set, we only detect whether inTargetID is close enough to
// be hit
// otherwise, we find the lowest-id player that is hit and return that
static LiveObject *getHitPlayer( int inX, int inY,
                                 int inTargetID = -1,
                                 char inCountMidPath = false,
                                 int inMaxAge = -1,
                                 int inMinAge = -1,
                                 int *outHitIndex = NULL ) {
    GridPos targetPos = { inX, inY };

    int numLive = players.size();
                                    
    LiveObject *hitPlayer = NULL;
                                    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *otherPlayer = 
            players.getElement( j );
        
        if( otherPlayer->error ) {
            continue;
            }
        
        if( otherPlayer->heldByOther ) {
            // ghost position of a held baby
            continue;
            }
        
        if( inMaxAge != -1 &&
            computeAge( otherPlayer ) > inMaxAge ) {
            continue;
            }

        if( inMinAge != -1 &&
            computeAge( otherPlayer ) < inMinAge ) {
            continue;
            }
        
        if( inTargetID != -1 &&
            otherPlayer->id != inTargetID ) {
            continue;
            }

        if( otherPlayer->xd == 
            otherPlayer->xs &&
            otherPlayer->yd ==
            otherPlayer->ys ) {
            // other player standing still
                                            
            if( otherPlayer->xd ==
                inX &&
                otherPlayer->yd ==
                inY ) {
                                                
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            }
        else {
            // other player moving
                
            GridPos cPos = 
                computePartialMoveSpot( 
                    otherPlayer );
                                        
            if( equal( cPos, targetPos ) ) {
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            else if( inCountMidPath ) {
                
                int c = computePartialMovePathStep( otherPlayer );

                // consider path step before and after current location
                for( int i=-1; i<=1; i++ ) {
                    int testC = c + i;
                    
                    if( testC >= 0 && testC < otherPlayer->pathLength ) {
                        cPos = otherPlayer->pathToDest[testC];
                 
                        if( equal( cPos, targetPos ) ) {
                            // hit
                            hitPlayer = otherPlayer;
                            if( outHitIndex != NULL ) {
                                *outHitIndex = j;
                                }
                            break;
                            }
                        }
                    }
                if( hitPlayer != NULL ) {
                    break;
                    }
                }
            }
        }

    return hitPlayer;
    }



static int isPlayerCountable( LiveObject *p, int inLineageEveID = -1,
                              int inRace = -1 ) {
    if( p->error ) {
        return false;
        }
    if( p->isTutorial ) {
        return false;
        }
    if( p->curseStatus.curseLevel > 0 ) {
        return false;
        }
    if( p->vogMode ) {
        return false;
        }
    
    if( inLineageEveID != -1 &&
        p->lineageEveID != inLineageEveID ) {
        return false;
        }
    if( inRace != -1 &&
        getObject( p->displayID )->race != inRace ) {
        return false;
        }
    return true;
    }



// if inLineageEveID != -1, it specifies that we count fertile mothers
// ONLY in that family
static int countFertileMothers( int inLineageEveID = -1, int inRace = -1 ) {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( ! isPlayerCountable( p, inLineageEveID, inRace ) ) {
            continue;
            }

        if( isFertileAge( p ) ) {
            if( barrierOn ) {
                // only fertile mothers inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    c++;
                    }
                }
            else {
                c++;
                }
            }
        }
    
    return c;
    }



// girls are females who are not fertile yet, but will be
// if inLineageEveID != -1, it specifies that we count girls
// ONLY in that family
static int countGirls( int inLineageEveID = -1, int inRace = -1 ) {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );

        if( ! isPlayerCountable( p, inLineageEveID, inRace ) ) {
            continue;
            }
        
        if( getFemale( p ) && computeAge( p ) < getFirstFertileAge() ) {
            if( barrierOn ) {
                // only girls inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    c++;
                    }
                }
            else {
                c++;
                }
            }
        }
    
    return c;
    }



static int countHelplessBabies() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );

        if( ! isPlayerCountable( p ) ) {
            continue;
            }

        if( computeAge( p ) < defaultActionAge ) {
            if( barrierOn ) {
                // only babies inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    c++;
                    }
                }
            else {
                c++;
                }
            }
        }
    
    return c;
    }



// counts only those inside barrier, if barrier on
// always ignores tutorial and donkytown players
static int countLivingPlayers() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );

        if( ! isPlayerCountable( p ) ) {
            continue;
            }

        if( barrierOn ) {
            // only people inside the barrier
            GridPos pos = getPlayerPos( p );
            
            if( abs( pos.x ) < barrierRadius &&
                abs( pos.y ) < barrierRadius ) {
                c++;
                }
            }
        else {
            c++;
            }
        }
    
    return c;
    }




static int countNonHelpless( GridPos inPos, double inRadius, 
                             double inMinAge = 0 ) {
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );

        if( ! isPlayerCountable( p ) ) {
            continue;
            }

        double age = computeAge( p );
        
        if( age >= defaultActionAge && age >= inMinAge ) {
            double d = distance( getPlayerPos( p ), inPos );
            
            if( d <= inRadius ){
                c++;
                }
            }
        }
    
    return c;
    }




static int countFamilies() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    SimpleVector<int> uniqueLines;

    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );

        if( ! isPlayerCountable( p ) ) {
            continue;
            }

        int lineageEveID = p->lineageEveID;
            
        if( uniqueLines.getElementIndex( lineageEveID ) == -1 ) {
            
            if( barrierOn ) {
                // only those inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    uniqueLines.push_back( lineageEveID );
                    }
                }
            else {
                uniqueLines.push_back( lineageEveID );
                }
            }
        }
    
    return uniqueLines.size();
    }



static int getTopLeader( LiveObject *inPlayer ) {
    LiveObject *nextPlayer = inPlayer;
    
    int topID = nextPlayer->id;

    while( nextPlayer != NULL ) {
        int nextID = nextPlayer->followingID;
        
        nextPlayer = NULL;
        
        if( nextID != -1 ) {
            topID = nextID;
            nextPlayer = getLiveObject( nextID );
            }
        }
    
    return topID;
    }



static char isLeader( LiveObject *inPlayer, int inPossibleLeaderID ) {
    if( inPlayer->followingID == inPossibleLeaderID ) {
        return true;
        }
    else if( inPlayer->followingID == -1 ) {
        return false;
        }
    else {
        LiveObject *leader = getLiveObject( inPlayer->followingID );
        if( leader == NULL ) {
            return false;
            }
        else {
            return isLeader( leader, inPossibleLeaderID );
            }
        }
    }



// does inViewer see inTarget as exiled?
char isExiled( LiveObject *inViewer, LiveObject *inTarget ) {
    for( int e=0; e< inTarget->exiledByIDs.size(); e++ ) {
        int exilerID = inTarget->exiledByIDs.getElementDirect( e );
                    
        // they have exiled us, or the person who exiled
        // us is their leader
        if( exilerID == inViewer->id 
            ||
            isLeader( inViewer, 
                      exilerID ) ) {
            return true;
            }
        }

    return false;
    }




static int countFollowers( LiveObject *inLeader ) {
    int count = 0;
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( isLeader( p, inLeader->id ) &&
            ! isExiled( inLeader, p ) ) {
            count++;
            }
        }
    
    return count;
    }




// people under player's top leader who don't see player as exiled
static int countAllies( LiveObject *inPlayer, 
                        GridPos inPos, double inRadius ) {    
    int count = 0;

    int thisTopID = getTopLeader( inPlayer );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( p != inPlayer && 
            distance( inPos, getPlayerPos( p ) ) <= inRadius ) {
            int otherTopID = getTopLeader( p );
            
            if( otherTopID == thisTopID ) {
                // they are in our ally tree
                
                if( ! isExiled( p, inPlayer ) ) {
                    // they don't view us as exiled
                    count ++;
                    }
                }
            }
        }
    
    return count;
    }



// people who see this player as exiled
// Note that this does NOT count 
static int countEnemies( LiveObject *inPlayer, 
                        GridPos inPos, double inRadius ) {
    int count = 0;

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( distance( inPos, getPlayerPos( p ) ) <= inRadius ) {
            
            if( isExiled( p, inPlayer ) ) {
                // they view us as exiled
                count ++;
                }
            }
        }

    return count;
    }




static char isEveWindow() {
    
    if( players.size() <=
        SettingsManager::getIntSetting( "minActivePlayersForEveWindow", 15 ) ) {
        // not enough players
        // always Eve window
        
        // new window starts if we ever get enough players again
        eveWindowStart = 0;
        eveWindowOver = false;
        
        return true;
        }

    if( eveWindowStart == 0 ) {
        // start window now
        eveWindowStart = Time::getCurrentTime();
        eveWindowOver = false;
        return true;
        }
    else {
        double secSinceStart = Time::getCurrentTime() - eveWindowStart;
        
        if( secSinceStart >
            SettingsManager::getIntSetting( "eveWindowSeconds", 3600 ) ) {
            
            if( ! eveWindowOver ) {
                // eveWindow just ended

                restockPostWindowFamilies();
                }
            
            eveWindowOver = true;
            return false;
            }

        eveWindowOver = false;
        return true;
        }
    }



static void triggerApocalypseNow( const char *inMessage ) {
    AppLog::infoF( "Local apocalypse triggered:  %s\n", inMessage );
    
    apocalypseTriggered = true;
    }



static void setupToolSlots( LiveObject *inPlayer ) {
    int min = SettingsManager::getIntSetting( "baseToolSlotsPerPlayer", 6 );
    int max = SettingsManager::getIntSetting( "maxToolSlotsPerPlayer", 12 );
    
    int minActive = 
        SettingsManager::getIntSetting( "minActivePlayersForToolSlots", 15 );
    
    if( countLivingPlayers() < minActive ) {
        // low-pop players know all tools
        getAllToolSets( &( inPlayer->learnedTools ) );
        
        // slots don't matter
        inPlayer->numToolSlots = min;
        
        return;
        }
    


    // for capped scores in 0..60
    
    // this hockey stick curve looks good for tool slots between 5 and 12

    // plot 7 * ( 0.7 * (x/60)**10 + 0.3 * x / 60 ) + 5

    // this is a weighted average between the linear part and the curve
    // with weights B and C, exponent D (for how steep the curve is)

    // A and E are determined by range settings

    // A * ( B * (x/60)**D + C * (x/60) ) + E

    double A = max - min;
    double E = min;
    
    double B = 0.7;
    double C = 0.3;
    
    double D = 7;
    
    
    // when this is called, we already have a valid fitness score between 0..60
    double s = inPlayer->fitnessScore;
    int slots = 
        lrint( A * ( B * pow( (s/60), D ) + C * (s/60) ) + E );


    const char *slotWord = "SLOTS";
        
    if( abs( slots - min ) == 1 ) {
        slotWord = "SLOT";
        }


    const char *slotTotalWord = "SLOTS";
        
    if( slots == 1 ) {
        slotTotalWord = "SLOT";
        }
    
    char *message = autoSprintf( "YOUR GENETIC FITNESS SCORE IS %.1lf**"
                                 "YOU GET %d BONUS TOOL %s, "
                                 "FOR A TOTAL OF %d %s.",
                                 inPlayer->fitnessScore,
                                 slots - min, slotWord, 
                                 slots, slotTotalWord );
    
    sendGlobalMessage( message, inPlayer );
    
    delete [] message;
    

    inPlayer->numToolSlots = slots;

    if( inPlayer->isTutorial && inPlayer->learnedTools.size() == 0 ) {
        // tutorial players know all tools
        getAllToolSets( &( inPlayer->learnedTools ) );
        }
    }


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



// strings in outRecordToFill destroyed by caller
char getForceSpawn( char *inEmail, ForceSpawnRecord *outRecordToFill ) {
    char *cont = SettingsManager::getSettingContents( "forceSpawnAccounts" );
    
    if( cont == NULL ) {
        return false;
        }
    int numParts;
    char **lines = split( cont, "\n", &numParts );

    delete [] cont;
    
    char found = false;

    for( int i=0; i<numParts; i++ ) {
        
        if( strstr( lines[i], inEmail ) == lines[i] ) {
            // matches email

            char emailBuff[100];
            
            int on = 0;
            
            sscanf( lines[i],
                    "%99s %d", emailBuff, &on );

            if( on == 1 ) {
                
                outRecordToFill->firstName = new char[20];
                outRecordToFill->lastName = new char[20];
                

                int numRead = sscanf( 
                    lines[i],
                    "%99s %d %d,%d %lf %19s %19s %d %d %d %d %d %d", 
                    emailBuff, &on,
                    &outRecordToFill->pos.x,
                    &outRecordToFill->pos.y,
                    &outRecordToFill->age,
                    outRecordToFill->firstName,
                    outRecordToFill->lastName,
                    &outRecordToFill->displayID,
                    &outRecordToFill->hatID,
                    &outRecordToFill->tunicID,
                    &outRecordToFill->bottomID,
                    &outRecordToFill->frontShoeID,
                    &outRecordToFill->backShoeID );
                
                if( numRead == 13 ) {
                    found = true;
                    }
                else {
                    delete [] outRecordToFill->firstName;
                    delete [] outRecordToFill->lastName;
                    }
                }
            break;
            }
        }


    for( int i=0; i<numParts; i++ ) {
        delete [] lines[i];
        }
    delete [] lines;
    
    return found;
    }




static void makeOffspringSayMarker( int inPlayerID, int inIDToSkip ) {
    
    LiveObject *playerO = getLiveObject( inPlayerID );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id != inPlayerID && o->id != inIDToSkip ) {
            
            if( o->ancestorIDs->getElementIndex( inPlayerID ) != -1 ) {
                // this player is an ancestor of this other
                
                // make other say +FAMILY+, but only so this player can hear it

                char *message = autoSprintf( "PS\n"
                                             "%d/0 +FAMILY+\n#",
                                             o->id );
                sendMessageToPlayer( playerO, message, strlen( message ) );
                delete [] message;
                }
            }
        }
    }



static int countLivingChildren( int inMotherID ) {
    int count = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->parentID == inMotherID && ! o->error ) {
            count ++;
            }
        }
    return count;
    }



int getUnusedLeadershipColor();



static double killDelayTime = 12.0;

static double posseDelayReductionFactor = 2.0;



// for placement of tutorials out of the way 
static int maxPlacementX = 5000000;

// tutorial is alwasy placed 400,000 to East of furthest birth/Eve
// location
static int tutorialOffsetX = 400000;


// each subsequent tutorial gets put in a diferent place
static int tutorialCount = 0;



// fill this with emails that should also affect lineage ban
// if any twin in group is banned, all should be
static SimpleVector<char*> tempTwinEmails;

static char nextLogInTwin = false;

static int firstTwinID = -1;


// inAllowOrForceReconnect is 0 for forbidden reconnect, 1 to allow, 
// 2 to require
// returns ID of new player,
// or -1 if this player reconnected to an existing ID
int processLoggedInPlayer( int inAllowOrForceReconnect,
                           Socket *inSock,
                           SimpleVector<char> *inSockBuffer,
                           char *inEmail,
                           int inTutorialNumber,
                           CurseStatus inCurseStatus,
                           PastLifeStats inLifeStats,
                           float inFitnessScore,
                           // set to -2 to force Eve
                           int inForceParentID = -1,
                           int inForceDisplayID = -1,
                           GridPos *inForcePlayerPos = NULL ) {
    

    usePersonalCurses = SettingsManager::getIntSetting( "usePersonalCurses",
                                                        0 );
    
    if( usePersonalCurses ) {
        // ignore what old curse system said
        inCurseStatus.curseLevel = 0;
        inCurseStatus.excessPoints = 0;
        
        initPersonalCurseTest( inEmail );
        
        for( int p=0; p<players.size(); p++ ) {
            LiveObject *o = players.getElement( p );
        
            if( ! o->error && 
                ! o->isTutorial &&
                o->curseStatus.curseLevel == 0 &&
                strcmp( o->email, inEmail ) != 0 ) {

                // non-tutorial, non-cursed, non-us player
                addPersonToPersonalCurseTest( o->email, inEmail,
                                              getPlayerPos( o ) );
                }
            }
        }
    


    // new behavior:
    // allow this new connection from same
    // email (most likely a re-connect
    // by same person, when the old connection
    // hasn't broken on our end yet)
    
    // to make it work, force-mark
    // the old connection as broken
    for( int p=0; p<players.size(); p++ ) {
        LiveObject *o = players.getElement( p );
        
        if( ! o->error && 
            o->connected && 
            strcmp( o->email, inEmail ) == 0 ) {
            
            setPlayerDisconnected( o, "Authentic reconnect received" );
            
            break;
            }
        }



    // see if player was previously disconnected
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error && ! o->connected &&
            strcmp( o->email, inEmail ) == 0 ) {

            if( ! inAllowOrForceReconnect ) {
                // trigger an error for them, so they die and are removed
                o->error = true;
                o->errorCauseString = "Reconnected as twin";
                break;
                }
            
            // else allow them to reconnect to existing life

            // give them this new socket and buffer
            if( o->sock != NULL ) {
                delete o->sock;
                o->sock = NULL;
                }
            if( o->sockBuffer != NULL ) {
                delete o->sockBuffer;
                o->sockBuffer = NULL;
                }
            
            o->sock = inSock;
            o->sockBuffer = inSockBuffer;
            
            // they are connecting again, need to send them everything again
            o->firstMapSent = false;
            o->firstMessageSent = false;
            o->inFlight = false;

            o->foodUpdate = true;
            
            o->connected = true;
            o->cravingKnown = false;
            
            if( o->heldByOther ) {
                // they're held, so they may have moved far away from their
                // original location
                
                // their first PU on reconnect should give an estimate of this
                // new location
                
                LiveObject *holdingPlayer = 
                    getLiveObject( o->heldByOtherID );
                
                if( holdingPlayer != NULL ) {
                    o->xd = holdingPlayer->xd;
                    o->yd = holdingPlayer->yd;
                    
                    o->xs = holdingPlayer->xs;
                    o->ys = holdingPlayer->ys;
                    }
                }
            
            AppLog::infoF( "Player %d (%s) has reconnected.",
                           o->id, o->email );

            delete [] inEmail;
            
            return -1;
            }
        }
    

    if( inAllowOrForceReconnect == 2 ) {
        // wanted a reconnect, but got here and found no life for this player
        
        AppLog::infoF( 
            "Player (%s) has attempted RLOGIN reconnect, but no life found.",
            inEmail );

        
        refundLifeToken( inEmail );
        

        // send a REJECTED message here
        const char *message = "REJECTED\n#";
        inSock->send( (unsigned char*)message,
                      strlen( message ), 
                      false, false );

        // then shut and clean up connection immediately

        // this may result in the REJECTED message not getting through

        // but we don't have other infrastructure in the code to handle
        // this case.

        delete [] inEmail;
        delete inSock;
        delete inSockBuffer;

        return -1;
        }
    


    // a baby needs to be born
    

    // reload these settings every time someone new connects
    // thus, they can be changed without restarting the server
    minFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );
    
    maxFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

    foodScaleFactor = 
        SettingsManager::getFloatSetting( "foodScaleFactor", 1.0 );

    foodScaleFactorFloor = 
        SettingsManager::getFloatSetting( "foodScaleFactorFloor", 0.5 );
    foodScaleFactorHalfLife = 
        SettingsManager::getFloatSetting( "foodScaleFactorHalfLife", 50 );

    foodScaleFactorGamma = 
        SettingsManager::getFloatSetting( "foodScaleFactorGamma", 1.5 );

    newPlayerFoodEatingBonus = 
        SettingsManager::getIntSetting( "newPlayerFoodEatingBonus", 5 );
    newPlayerFoodDecrementSecondsBonus =
        SettingsManager::getFloatSetting( "newPlayerFoodDecrementSecondsBonus",
                                          8 );
    newPlayerFoodBonusHalfLifeSeconds =
        SettingsManager::getFloatSetting( "newPlayerFoodBonusHalfLifeSeconds",
                                          36000 );

    babyBirthFoodDecrement = 
        SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );

    nurseCost =
        SettingsManager::getIntSetting( "nurseCost", 1 );

    babyFeedingLevel =
        SettingsManager::getIntSetting( "babyFeedingLevel", 2 );
    

    indoorFoodDecrementSecondsBonus = SettingsManager::getFloatSetting( 
        "indoorFoodDecrementSecondsBonus", 20 );


    eatBonus = 
        SettingsManager::getIntSetting( "eatBonus", 0 );
    eatBonusFloor = 
        SettingsManager::getIntSetting( "eatBonusFloor", 0 );
    eatBonusHalfLife = 
        SettingsManager::getFloatSetting( "eatBonusHalfLife", 50 );

    eatCostMax =
        SettingsManager::getFloatSetting( "eatCostMax", 5 );
    eatCostGrowthRate =
        SettingsManager::getFloatSetting( "eatCostGrowthRate", 0.1 );

    minActivePlayersForLanguages =
        SettingsManager::getIntSetting( "minActivePlayersForLanguages", 15 );

    SimpleVector<double> *multiplierList = 
        SettingsManager::getDoubleSettingMulti( "posseSpeedMultipliers" );
    
    for( int i=0; i<multiplierList->size() && i < 4; i++ ) {
        posseSizeSpeedMultipliers[i] = multiplierList->getElementDirect( i );
        }
    delete multiplierList;


    
    
    killDelayTime = 
        SettingsManager::getFloatSetting( "killDelayTime", 12.0 );
    
    
    posseDelayReductionFactor = 
        SettingsManager::getFloatSetting( "posseDelayReductionFactor", 2.0 );

    
    killerVulnerableSeconds =
        SettingsManager::getFloatSetting( "killerVulnerableSeconds", 60 );
    

    cursesUseSenderEmail = 
        SettingsManager::getIntSetting( "cursesUseSenderEmail", 0 );

    useCurseWords = 
        SettingsManager::getIntSetting( "useCurseWords", 1 );
    

    minPosseFraction = 
        SettingsManager::getFloatSetting( "minPosseFraction", 0.5 );
    minPosseCap =
        SettingsManager::getIntSetting( "minPosseCap", 3 );

    possePopulationRadius = 
        SettingsManager::getFloatSetting( "possePopulationRadius", 30 );
    

    recentScoreWindowForPickingEve = 
        SettingsManager::getIntSetting( "recentScoreWindowForPickingEve", 10 );
    
    shortLifeAge = SettingsManager::getFloatSetting( "shortLifeAge", 10 );

    canYumChainBreak = SettingsManager::getIntSetting( "canYumChainBreak", 0 );
    
    yumBonusCap = SettingsManager::getIntSetting( "yumBonusCap", -1 );

    
    minAgeForCravings = SettingsManager::getDoubleSetting( "minAgeForCravings",
                                                           10 );
    

    numConnections ++;
                
    LiveObject newObject;

    newObject.email = inEmail;
    newObject.origEmail = NULL;
    
    newObject.lastSidsBabyEmail = NULL;

    newObject.lastBabyEmail = NULL;

    newObject.everHomesick = false;

    newObject.lastGateVisitorNoticeTime = 0;
    newObject.lastNewBabyNoticeTime = 0;

    newObject.cravingFood = noCraving;
    newObject.cravingFoodYumIncrement = 0;
    newObject.cravingKnown = false;
    
    newObject.id = nextID;
    nextID++;


    if( nextLogInTwin ) {
        newObject.isTwin = true;
        }
    else {
        newObject.isTwin = false;
        }
    
    newObject.isLastLifeShort = isShortLife( inEmail );

    


    if( familyDataLogFile != NULL ) {
        int eveCount = 0;
        
        double ageSum = 0;
        int ageSumCount = 0;
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *o = players.getElement( i );
        
            if( ! o->error && o->connected ) {
                if( o->parentID == -1 ) {
                    eveCount++;
                    }
                ageSum += computeAge( o );
                ageSumCount++;
                }
            }
        
        double averageAge = 0;
        if( ageSumCount > 0 ) {
            averageAge = ageSum / ageSumCount;
            }
        
        int cFam = countFamilies();
        int cM = countFertileMothers();
        int cB = countHelplessBabies();
        fprintf( familyDataLogFile,
                 "%.2f nid:%d fam:%d mom:%d bb:%d plr:%d eve:%d "
                 "avAge:%.2f\n",
                 Time::getCurrentTime(), newObject.id, 
                 cFam, cM, cB,
                 players.size(),
                 eveCount,
                 averageAge );
        }


    
    newObject.fitnessScore = inFitnessScore;
    



    SettingsManager::setSetting( "nextPlayerID",
                                 (int)nextID );


    newObject.responsiblePlayerID = -1;
    
    newObject.killPosseSize = 0;

    newObject.displayID = getRandomPersonObject();
    
    newObject.isEve = false;
    
    newObject.isTutorial = false;
    
    if( inTutorialNumber > 0 ) {
        newObject.isTutorial = true;
        }

    newObject.trueStartTimeSeconds = Time::getCurrentTime();
    newObject.lifeStartTimeSeconds = newObject.trueStartTimeSeconds;
                            

    newObject.lastSayTimeSeconds = Time::getCurrentTime();
    newObject.firstEmoteTimeSeconds = Time::getCurrentTime();
    
    newObject.emoteCountInWindow = 0;
    newObject.emoteCooldown = false;
    

    newObject.heldByOther = false;
    newObject.everHeldByParent = false;
    
    newObject.followingID = -1;
    newObject.leadingColorIndex = -1;

    // everyone should hear about who this player is following
    newObject.followingUpdate = true;
    
    newObject.exileUpdate = false;
    
    newObject.currentOrderNumber = -1;
    newObject.currentOrderOriginatorID = -1;
    newObject.currentOrder = NULL;
    

    int numPlayers = players.size();

    primeLineageTest( numPlayers );

    SimpleVector<LiveObject*> parentChoices;
    
    int numBirthLocationsCurseBlocked = 0;

    int numOfAge = 0;

    int maxLivingChildrenPerMother = 
        SettingsManager::getIntSetting( "maxLivingChildrenPerMother", 4 );
    

    // first, find all mothers that could possibly have us

    // three passes, once with birth cooldown limit and lineage limits on, 
    // then more passes with them off (if needed)
    char checkCooldown = true;
    char checkLineageLimit = true;
    
    int forceOffspringLineageID = getOffspringLineageID( newObject.email );
    clearOffspringLineageID( newObject.email );
    

    for( int p=0; p<3; p++ ) {
    
        for( int i=0; i<numPlayers; i++ ) {
            LiveObject *player = players.getElement( i );
            
            if( player->error ) {
                continue;
                }
            
            if( player->isTutorial ) {
                continue;
                }
            
            if( player->vogMode ) {
                continue;
                }

            GridPos motherPos = getPlayerPos( player );
            int homeStatus = isBirthland( motherPos.x, motherPos.y,
                                          player->lineageEveID,
                                          player->displayID );
            
            if( homeStatus == -1 ||
                ( homeStatus == 0 &&
                  player->everHomesick ) ) {
                // mother can't have babies here
                continue;
                }
                
            
            if( player->lastSidsBabyEmail != NULL &&
                strcmp( player->lastSidsBabyEmail,
                        newObject.email ) == 0 ) {
                // this baby JUST committed SIDS for this mother
                // skip her
                // (don't ever send SIDS baby to same mother twice in a row)
                continue;
                }

            if( isFertileAge( player ) ) {
                numOfAge ++;
                
                if( checkCooldown &&
                    ( Time::timeSec() < player->birthCoolDown 
                      ||
                      countLivingChildren( player->id ) >= 
                      maxLivingChildrenPerMother ) ) {    
                    continue;
                    }
                
                GridPos motherPos = getPlayerPos( player );
                
                if( checkLineageLimit &&
                    forceOffspringLineageID != -1 ) {
                    // we're trying to force this player to be born
                    // to their descendants
                    if( player->lineage->getElementIndex( 
                            forceOffspringLineageID ) == -1 ) {
                        // this possible mother is NOT
                        // a descendant
                        // skip
                        continue;
                        }
                    }
                // otherwise, respect normal lineage ban
                else if( checkLineageLimit &&
                    ! isLinePermitted( newObject.email, motherPos ) ) {
                    // this line forbidden for new player
                    continue;
                    }

                if( usePersonalCurses &&
                    isBirthLocationCurseBlocked( newObject.email, 
                                                 motherPos ) ) {
                    // this spot forbidden
                    // because someone nearby cursed new player
                    numBirthLocationsCurseBlocked++;
                    continue;
                    }
            
                // test any twins also
                char twinBanned = false;
                for( int s=0; s<tempTwinEmails.size(); s++ ) {
                    if( checkLineageLimit &&
                        ! isLinePermitted( tempTwinEmails.getElementDirect( s ),
                                           motherPos ) ) {
                        twinBanned = true;
                        break;
                        }

                    if( usePersonalCurses &&
                        // non-cached version for twin emails
                        // (otherwise, we interfere with caching done
                        //  for our email)
                        isBirthLocationCurseBlockedNoCache( 
                            tempTwinEmails.getElementDirect( s ), 
                            motherPos ) ) {
                        twinBanned = true;
                        
                        numBirthLocationsCurseBlocked++;
                        
                        break;
                        }
                    }
                
                if( twinBanned ) {
                    continue;
                    }
                
            
                if( ( inCurseStatus.curseLevel <= 0 && 
                      player->curseStatus.curseLevel <= 0 ) 
                    || 
                    ( inCurseStatus.curseLevel > 0 && 
                      player->curseStatus.curseLevel > 0 ) ) {
                    // cursed babies only born to cursed mothers
                    // non-cursed babies never born to cursed mothers
                    parentChoices.push_back( player );
                    }
                }
            }
        
        

        if( p == 0 ) {
            if( parentChoices.size() > 0 || numOfAge == 0 ) {
                // found some mothers off-cool-down, 
                // or there are none at all
                // skip second pass
                break;
                }
            
            // else found no mothers (but some on cool-down?)
            // start over with cooldowns off
            
            AppLog::infoF( 
                "Trying to place new baby %s, out of %d fertile moms, "
                "all are on cooldown, lineage banned, or curse blocked.  "
                "Trying again ignoring cooldowns.", newObject.email, numOfAge );
            
            checkCooldown = false;
            numBirthLocationsCurseBlocked = 0;
            numOfAge = 0;
            }
        else if( p == 1 ) {
            if( parentChoices.size() == 0 && numOfAge > 0 ) {
            
                // found no mothers (but some on lineage limit?)
                // start over with lineage limit off
            
                AppLog::infoF( 
                    "Trying to place new baby %s, out of %d fertile moms, "
                    "none are not on lineage limit or curse blocked.  "
                    "Trying again ignoring lineage limit.", 
                    newObject.email, numOfAge );
                
                checkLineageLimit = false;
                numBirthLocationsCurseBlocked = 0;
                numOfAge = 0;
                }
            }
        
        
        if( p == 1 || p == 2 ) {
            // had to resort to second/third pass with no cool-downs
            
            if( parentChoices.size() > 0 &&
                forceOffspringLineageID != -1 ) {
                // we're forcing a baby on an overwhelmed mother (on cooldown)
                // BUT, this is an offspring placement, so we should assume
                // the baby is wanted
                // don't consider spawning this one as Eve
                }
            else if( parentChoices.size() > 0 ) {
                // we're about to force a baby on an overwhelmed mother
                // how overwhelmed are they?
                
                int totalBabies = countHelplessBabies();
                
                int totalAdults = countLivingPlayers() - totalBabies;
                
                double ratio = (double)totalBabies / (double)totalAdults;


                double maxRatio = 
                    SettingsManager::getDoubleSetting( "maxBabyAdultRatio",
                                                       2.0 );
                AppLog::infoF( 
                    "Counting %d babies for %d adults, ratio %f (max %f)",
                    totalBabies, totalAdults, ratio, maxRatio );
                
                
                if( ratio >= maxRatio ) {
                    // to many members of the population are helpless babies 
                    // force an Eve spawn to compensate for this condition
                    
                    AppLog::infoF( 
                        "%d babies for %d adults, forcing Eve.",
                        totalBabies, totalAdults );
                    
                    // this player wasn't cursed out of all
                    // possible birth locations
                    numBirthLocationsCurseBlocked = 0;
                    parentChoices.deleteAll();
                    }
                else {
                    // catch edge case where there are lots of adults around
                    // but only a few isolated moms left
                    // they may not have adults around to help, so we don't
                    // want to send all the babies to them by accident
                    int totalMoms = countFertileMothers();
                    ratio = (double)totalBabies / (double)totalMoms;

                    maxRatio =
                        SettingsManager::getDoubleSetting( "maxBabyMomRatio",
                                                           2.0 );
                    AppLog::infoF( 
                        "Counting %d babies for %d moms, ratio %f (max %f)",
                        totalBabies, totalMoms, ratio, maxRatio );

                    if( ratio >= maxRatio ) {
                        // too many babies per mom
                        AppLog::infoF( 
                            "%d babies for %d moms, forcing Eve.",
                            totalBabies, totalMoms );
                    
                        // this player wasn't cursed out of all
                        // possible birth locations
                        numBirthLocationsCurseBlocked = 0;
                        parentChoices.deleteAll();
                        }
                    }
                
                break;
                }
            else if( p == 2 ) {
                AppLog::infoF( 
                        "Even after ignoring cooldowns and lineage limits, "
                        "no available mothers." );
                }
            }
        }
    
    
    if( parentChoices.size() > 1 ) {
        // filter them so that we avoid mothers who WE have curse-blocked
        // (only if we have a choice)
        SimpleVector<LiveObject*> oldParentList;
        oldParentList.push_back_other( &parentChoices );
        
        for( int i=0; i<parentChoices.size(); i++ ) {
            LiveObject *mom = parentChoices.getElementDirect( i );
            
            if( isCursed( newObject.email, mom->email ) ) {
                parentChoices.deleteElement( i );
                i--;
                }
            }
        if( parentChoices.size() == 0 ) {
            // restore original list, we have them all curse-blocked
            parentChoices.push_back_other( &oldParentList );
            }
        }
    else if( parentChoices.size() == 1 && countFertileMothers() == 1 ) {
        // only one possible mom

        // make sure WE don't have THEM curse blocked
        LiveObject *mom = parentChoices.getElementDirect( 0 );
        
        if( isCursed( newObject.email, mom->email ) ) {
            AppLog::info( 
                "We have only fertile mom on server curse-blocked.  "
                "Avoiding her, and not going to d-town, spawning new Eve" );
            parentChoices.deleteAll();
            
            // don't send ourselves to d-town in this case
            numBirthLocationsCurseBlocked = 0;
            }
        }
    else if( parentChoices.size() == 0 && 
             numBirthLocationsCurseBlocked > 0 &&
             countFertileMothers() == 1 ) {
        // there's only one mom on the server, and we're curse-blocked from her
        // don't send us to d-town in this case
        numBirthLocationsCurseBlocked = 0;
        
        AppLog::info( 
                "Only fertile mom on server has us curse-blocked.  "
                "Avoiding her, and not going to d-town, spawning new Eve" );   
        }

    
    if( parentChoices.size() > 0 &&
        SettingsManager::getIntSetting( "propUpWeakestRace", 1 ) ) {
        // next, filter mothers by weakest race amoung them
        int preFilterCount = parentChoices.size();
        
        SimpleVector<int> parentRaces;
        SimpleVector<int> parentRaceFertileCounts;
    
        
        for( int i=0; i<parentChoices.size(); i++ ) {
            LiveObject *player = parentChoices.getElementDirect( i );
            
            int race = getObject( player->displayID )->race;
            
            if( parentRaces.getElementIndex( race ) == -1 ) {
                parentRaces.push_back( race );
                
                parentRaceFertileCounts.push_back(
                    countFertileMothers( -1, race ) +
                    countGirls( -1, race ) );
                }
            }        


        int weakRace = -1;
        int weakRaceCount = 2 * players.size();
        
        for( int i=0; i<parentRaces.size(); i++ ) {
            int c = parentRaceFertileCounts.getElementDirect( i );
            
            if( c < weakRaceCount ) {
                weakRaceCount = c;
                weakRace = parentRaces.getElementDirect( i );
                }
            }

        for( int i=0; i<parentChoices.size(); i++ ) {
            int r = 
                getObject( 
                    parentChoices.getElementDirect( i )->displayID )->race;
            
            if( r != weakRace ) {
                parentChoices.deleteElement( i );
                i--;
                }
            }

        AppLog::infoF( "Filtering %d mothers for weakest race %d, and %d match",
                       preFilterCount, weakRace, parentChoices.size() );
        }


    
    if( parentChoices.size() > 0 &&
        SettingsManager::getIntSetting( "propUpWeakestFamily", 1 ) ) {
        int preFilterCount = parentChoices.size();
        
        // next, filter mothers by weakest family amoung them
        SimpleVector<int> parentFams;
        SimpleVector<int> parentFamFertileCounts;    
        
        for( int i=0; i<parentChoices.size(); i++ ) {
            LiveObject *player = parentChoices.getElementDirect( i );
            
            int fam = player->lineageEveID;
            
            if( parentFams.getElementIndex( fam ) == -1 ) {
                parentFams.push_back( fam );
                
                parentFamFertileCounts.push_back(
                    countFertileMothers( -1, fam ) +
                    countGirls( -1, fam ) );
                }
            }        


        int weakFam = -1;
        int weakFamCount = 2 * players.size();
        
        for( int i=0; i<parentFams.size(); i++ ) {
            int c = parentFamFertileCounts.getElementDirect( i );
            
            if( c < weakFamCount ) {
                weakFamCount = c;
                weakFam = parentFams.getElementDirect( i );
                }
            }

        for( int i=0; i<parentChoices.size(); i++ ) {
            int f = parentChoices.getElementDirect( i )->lineageEveID;
            
            if( f != weakFam ) {
                parentChoices.deleteElement( i );
                i--;
                }
            }
        AppLog::infoF( "Filtering %d mothers for weakest fam %d, and %d match",
                       preFilterCount, weakFam, parentChoices.size() );
        }



    if( parentChoices.size() == 0 && numBirthLocationsCurseBlocked > 0 ) {
        // they are blocked from being born EVERYWHERE by curses

        AppLog::infoF( "No available mothers, and %d are curse blocked, "
                       "sending a new Eve to donkeytown",
                       numBirthLocationsCurseBlocked );

        // d-town
        inCurseStatus.curseLevel = 1;
        inCurseStatus.excessPoints = 1;
        }

    
    float maxRecentScore = getHighestRecentScore();
    
    AppLog::infoF( "%d recent scores in log, %d max window size, "
                   "this player's score is %f, max score in window is %f",
                   recentScoresForPickingEve.size(),
                   recentScoreWindowForPickingEve,
                   inFitnessScore, maxRecentScore );

    
    // don't consider forced-spawn offspring as candidates for a needed Eve
    if( forceOffspringLineageID == -1 )
    if( inFitnessScore >= maxRecentScore )
    if( parentChoices.size() > 0 ) {
        // make sure one race isn't entirely extinct
        
        if( numPlayers >= 
            SettingsManager::getIntSetting( 
                "minActivePlayersForSpecialBiomes", 15 ) ) {

            int numRaces;
            int *races = getRaces( &numRaces );
        
            for( int i=0; i<numRaces; i++ ) {
                if( countGirls( -1, races[i] ) +
                    countFertileMothers( -1, races[i] ) == 0 ) {
                    AppLog::infoF( 
                        "Race %d has no potentially fertile females, "
                        "AND player's score (%f) beats/ties "
                        "max score of last %d players (%f), forcing Eve.",
                        races[i],
                        inFitnessScore,
                        recentScoreWindowForPickingEve,
                        maxRecentScore );
                
                    parentChoices.deleteAll();
                    break;
                    }
                }
            delete [] races;
            }
        }


    
    if( forceOffspringLineageID == -1 )
    if( inFitnessScore >= maxRecentScore )
    if( parentChoices.size() > 0 ) {
        int generationNumber =
            SettingsManager::getIntSetting( "forceEveAfterGenerationNumber",
                                            40 );
        
        int minGen = generationNumber + 1;
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *o = players.getElement( i );
            
            if( isPlayerCountable( o ) ) {
                
                if( o->parentChainLength < minGen ) {
                    minGen = o->parentChainLength;
                    }
                }
            }

        if( minGen > generationNumber ) {
            AppLog::infoF( 
                        "Youngest player generation on server is %d, "
                        "which is above our trigger level %d, "
                        "AND player's score (%f) beats/ties "
                        "max score of last %d players (%f), forcing Eve.",
                        minGen, generationNumber,
                        inFitnessScore,
                        recentScoreWindowForPickingEve,
                        maxRecentScore );    
            parentChoices.deleteAll();
            }
        
        }
        
    
    




    if( inTutorialNumber > 0 ) {
        // Tutorial always played full-grown
        parentChoices.deleteAll();
        }

    if( inForceParentID == -2 ) {
        // force eve
        parentChoices.deleteAll();
        }
    else if( inForceParentID > -1 ) {
        // force parent choice
        parentChoices.deleteAll();
        
        LiveObject *forcedParent = getLiveObject( inForceParentID );
        
        if( forcedParent != NULL ) {
            parentChoices.push_back( forcedParent );
            }
        }
    
    
    char forceSpawn = false;
    ForceSpawnRecord forceSpawnInfo;
    
    if( SettingsManager::getIntSetting( "forceAllPlayersEve", 0 ) ) {
        parentChoices.deleteAll();
        }
    else {
        forceSpawn = getForceSpawn( inEmail, &forceSpawnInfo );
    
        if( forceSpawn ) {
            parentChoices.deleteAll();
            }
        }




    newObject.parentChainLength = 1;

    if( parentChoices.size() == 0 ) {
        // new Eve
        // she starts almost full grown

        newObject.isEve = true;
        newObject.lineageEveID = newObject.id;
        
        newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );
        
        // she starts off craving a food right away
        newObject.cravingFood = getCravedFood( newObject.lineageEveID,
                                               newObject.parentChainLength );
        // initilize increment
        newObject.cravingFoodYumIncrement = 1;

        
        // when placing eve, pick a race that is not currently
        // represented
        int numRaces = 0;
        int *races = getRaces( &numRaces );
        
        int *counts = new int[ numRaces ];
        
        int foundMin = -1;
        int minFem = 999999;
        
        // first, shuffle races
        for( int r=0; r<numRaces; r++ ) {
            int other = randSource.getRandomBoundedInt( 0, numRaces - 1 );
            int temp = races[r];
            races[r] = races[other];
            races[other] = temp;
            }

        for( int r=0; r<numRaces; r++ ) {
            counts[r] = 0;
            for( int i=0; i<numPlayers; i++ ) {
                LiveObject *player = players.getElement( i );
            
                if( isPlayerCountable( player ) && isFertileAge( player ) ) {
                    ObjectRecord *d = getObject( player->displayID );
                    
                    if( d->race == races[r] ) {
                        counts[r] ++;
                        }
                    }
                }
            if( counts[r] == 0 &&
                getRaceSize( races[r] ) >= 2 ) {
                foundMin = races[r];
                break;
                }
            else if( counts[r] > 0 && counts[r] < minFem ) {
                minFem = counts[r];
                foundMin = races[r];
                }
            }
        
        delete [] races;
        delete [] counts;
        

        int femaleID = -1;
        
        if( foundMin != -1 ) {
            femaleID = getRandomPersonObjectOfRace( foundMin );
            
            int tryCount = 0;
            while( getObject( femaleID )->male && tryCount < 10 ) {
                femaleID = getRandomPersonObjectOfRace( foundMin );
                tryCount++;
                }
            if( getObject( femaleID )->male ) {
                femaleID = -1;
                }
            }

        if( femaleID == -1 ) {       
            // all races present, or couldn't find female character
            // to use in weakest race
            femaleID = getRandomFemalePersonObject();
            }
        
        if( femaleID != -1 ) {
            newObject.displayID = femaleID;
            }
        }    
         
       
    // else player starts as newborn
                

    newObject.foodCapModifier = 1.0;

    newObject.fever = 0;

    // start full up to capacity with food
    newObject.foodStore = computeFoodCapacity( &newObject );

    newObject.drunkenness = 0;
    

    if( ! newObject.isEve ) {
        // babies start out almost starving
        newObject.foodStore = 2;
        }
    
    if( newObject.isTutorial && newObject.foodStore > 10 ) {
        // so they can practice eating at the beginning of the tutorial
        newObject.foodStore -= 6;
        }
    
    double currentTime = Time::getCurrentTime();
    

    newObject.envHeat = targetHeat;
    newObject.bodyHeat = targetHeat;
    newObject.biomeHeat = targetHeat;
    newObject.lastBiomeHeat = targetHeat;
    newObject.heat = 0.5;
    newObject.heatUpdate = false;
    newObject.lastHeatUpdate = currentTime;
    newObject.isIndoors = false;
    
    newObject.foodDrainTime = 0;
    newObject.indoorBonusTime = 0;
    newObject.indoorBonusFraction = 0;

    
    
                
    newObject.foodUpdate = true;
    newObject.lastAteID = 0;
    newObject.lastAteFillMax = 0;
    newObject.justAte = false;
    newObject.justAteID = 0;
    
    newObject.yummyBonusStore = 0;

    newObject.lastReportedFoodCapacity = 0;

    newObject.clothing = getEmptyClothingSet();

    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        newObject.clothingEtaDecay[c] = 0;
        }
    
    newObject.xs = 0;
    newObject.ys = 0;
    newObject.xd = 0;
    newObject.yd = 0;
    
    newObject.facingLeft = 0;
    newObject.lastFlipTime = currentTime;
    

    newObject.mapChunkPathCheckedDest.x = 0;
    newObject.mapChunkPathCheckedDest.y = 0;
    

    newObject.lastRegionLookTime = 0;
    newObject.playerCrossingCheckTime = 0;
    
    
    LiveObject *parent = NULL;

    char placed = false;

    char forceGirl = false;
    
    
    if( parentChoices.size() > 0 ) {
        placed = true;
        
        if( newObject.isEve ) {
            // spawned next to random existing player
            int parentIndex = 
                randSource.getRandomBoundedInt( 0,
                                                parentChoices.size() - 1 );
            
            parent = parentChoices.getElementDirect( parentIndex );
            }
        else {
            // baby

            // pick random mother from a weighted distribution based on 
            // each mother's temperature
            
            // AND each mother's current YUM multiplier
            
            int maxYumMult = 1;

            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );
                
                int yumMult = p->yummyFoodChain.size() - 1;
                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }
                
                if( yumMult > maxYumMult ) {
                    maxYumMult = yumMult;
                    }
                }
            
            // 0.5 temp is worth .5 weight
            // 1.0 temp and 0 are worth 0 weight
            
            // max YumMult worth same that perfect temp is worth (0.5 weight)


            

            double totalWeight = 0;
            
            SimpleVector<double> parentChoiceWeights;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );

                // temp part of weight
                double thisMotherWeight = 0.5 - fabs( p->heat - 0.5 );
                

                int yumMult = p->yummyFoodChain.size() - 1;
                                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }

                // yum mult part of weight
                thisMotherWeight += 0.5 * yumMult / (double) maxYumMult;
                
                parentChoiceWeights.push_back( thisMotherWeight );
                
                totalWeight += thisMotherWeight;
                }

            double choice = 
                randSource.getRandomBoundedDouble( 0, totalWeight );
            
            
            totalWeight = 0;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );
                
                totalWeight += 
                    parentChoiceWeights.getElementDirect( i );

                if( totalWeight >= choice ) {
                    parent = p;
                    break;
                    }                
                }

            if( parent != NULL ) {
                // check if this family has too few potentially fertile
                // females
                // If so, force a girl baby.
                // Do this regardless of whether Eve window is in effect, etc.
                int min = SettingsManager::getIntSetting( 
                    "minPotentialFertileFemalesPerFamily", 3 );
                int famMothers = countFertileMothers( parent->lineageEveID );
                int famGirls = countGirls( parent->lineageEveID );
                if( famMothers + famGirls < min ) {
                    forceGirl = true;
                    }
                }
            }
        

        
        if( ! newObject.isEve ) {
            // mother giving birth to baby
            // take a ton out of her food store

            int min = 4;
            if( parent->foodStore < min ) {
                min = parent->foodStore;
                }
            parent->foodStore -= babyBirthFoodDecrement;
            if( parent->foodStore < min ) {
                parent->foodStore = min;
                }

            parent->foodDecrementETASeconds +=
                computeFoodDecrementTimeSeconds( parent );
            
            parent->foodUpdate = true;
            

            // only set race if the spawn-near player is our mother
            // otherwise, we are a new Eve spawning next to a baby
            
            timeSec_t curTime = Time::timeSec();
            
            parent->babyBirthTimes->push_back( curTime );
            parent->babyIDs->push_back( newObject.id );
            
            if( parent->lastBabyEmail != NULL ) {
                delete [] parent->lastBabyEmail;
                }
            parent->lastBabyEmail = stringDuplicate( newObject.email );
            

            // set cool-down time before this worman can have another baby
            parent->birthCoolDown = pickBirthCooldownSeconds() + curTime;

            ObjectRecord *parentObject = getObject( parent->displayID );

            // pick race of child
            int numRaces;
            int *races = getRaces( &numRaces );
        
            int parentRaceIndex = -1;
            
            for( int i=0; i<numRaces; i++ ) {
                if( parentObject->race == races[i] ) {
                    parentRaceIndex = i;
                    break;
                    }
                }
            

            if( parentRaceIndex != -1 ) {
                
                int childRace = parentObject->race;
                
                char forceDifferentRace = false;

                if( getRaceSize( parentObject->race ) < 3 ) {
                    // no room in race for diverse family members
                    
                    // pick a different race for child to ensure village 
                    // diversity
                    // (otherwise, almost everyone is going to look the same)
                    forceDifferentRace = true;
                    }
                
                // everyone has a small chance of having a neighboring-race
                // baby, even if not forced by parent's small race size
                if( forceDifferentRace ||
                    randSource.getRandomDouble() > 
                    childSameRaceLikelihood ) {
                    
                    // different race than parent
                    
                    int offset = 1;
                    
                    if( randSource.getRandomBoolean() ) {
                        offset = -1;
                        }
                    int childRaceIndex = parentRaceIndex + offset;
                    
                    // don't wrap around
                    // but push in other direction instead
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 2;
                        }
                    if( childRaceIndex < 0 ) {
                        childRaceIndex = 1;
                        }
                    
                    // stay in bounds
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 1;
                        }
                    

                    childRace = races[ childRaceIndex ];
                    }
                
                if( childRace == parentObject->race ) {
                    newObject.displayID = getRandomFamilyMember( 
                        parentObject->race, parent->displayID, familySpan,
                        forceGirl );
                    }
                else {
                    newObject.displayID = 
                        getRandomPersonObjectOfRace( childRace );
                    }
            
                }
        
            delete [] races;
            }
        
        if( parent->xs == parent->xd && 
            parent->ys == parent->yd ) {
                        
            // stationary parent
            newObject.xs = parent->xs;
            newObject.ys = parent->ys;
                        
            newObject.xd = parent->xs;
            newObject.yd = parent->ys;
            }
        else {
            // find where parent is along path
            GridPos cPos = computePartialMoveSpot( parent );
                        
            newObject.xs = cPos.x;
            newObject.ys = cPos.y;
                        
            newObject.xd = cPos.x;
            newObject.yd = cPos.y;
            }
        
        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    else if( inTutorialNumber > 0 ) {
        
        // different tutorials go in different x blocks, far apart
        int startX = maxPlacementX + tutorialOffsetX * inTutorialNumber;
        int startY = tutorialCount * 40;

        if( inTutorialNumber > 1 ) {
            // everything beyond tutorial 1 is placed randomly dispersed
            // in a big square
            int randX = randSource.getRandomBoundedInt( 0, tutorialOffsetX );
            int randY = randSource.getRandomBoundedInt( 0, tutorialOffsetX );
            
            startX += randX;
            startY += randY;
            }
        

        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        char *mapFileName = autoSprintf( "tutorial%d.txt", inTutorialNumber );
        
        placed = loadTutorialStart( &( newObject.tutorialLoad ),
                                    mapFileName, startX, startY );
        
        delete [] mapFileName;

        tutorialCount ++;

        int maxPlayers = 
            SettingsManager::getIntSetting( "maxPlayers", 200 );

        if( tutorialCount > maxPlayers ) {
            // wrap back to 0 so we don't keep getting farther
            // and farther away on map if server runs for a long time.

            // The earlier-placed tutorials are over by now, because
            // we can't have more than maxPlayers tutorials running at once
            
            tutorialCount = 0;
            }
        }
    
    if( inForcePlayerPos != NULL ) {
        placed = true;

        int startX = inForcePlayerPos->x;
        int startY = inForcePlayerPos->y;
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;
        }
    
    
    if( !placed ) {
        // tutorial didn't happen if not placed
        newObject.isTutorial = false;
        
        char allowEveRespawn = true;
        
        if( numOfAge >= 4 ) {
            // there are at least 4 fertile females on the server
            // why is this player spawning as Eve?
            // they must be on lineage ban everywhere OR a forced Eve injection
            // (and they are NOT a solo player on an empty server)
            // don't allow them to spawn back at their last old-age Eve death
            // location.
            allowEveRespawn = false;            
            }
       
        if( SettingsManager::getIntSetting( "blockEveRespawn", 0 ) ) {
            // this server has Eve respawn blocked
            allowEveRespawn = false;
            }

        

        // else starts at civ outskirts (lone Eve)
        
        SimpleVector<GridPos> otherPeoplePos( numPlayers );


        // consider players to be near Eve location that match
        // Eve's curse status
        char seekingCursed = false;
        
        if( inCurseStatus.curseLevel > 0 ) {
            seekingCursed = true;
            }
        

        for( int i=0; i<numPlayers; i++ ) {
            LiveObject *player = players.getElement( i );
            
            if( player->error || 
                ! player->connected ||
                player->isTutorial ||
                player->vogMode ) {
                continue;
                }

            if( seekingCursed && player->curseStatus.curseLevel <= 0 ) {
                continue;
                }
            else if( ! seekingCursed &&
                     player->curseStatus.curseLevel > 0 ) {
                continue;
                }

            GridPos p = { player->xs, player->ys };
            otherPeoplePos.push_back( p );
            }
        

        char incrementEvePlacement = true;        

        // don't increment Eve placement if this is a cursed player
        if( inCurseStatus.curseLevel > 0 ) {
            incrementEvePlacement = false;
            }

        int startX, startY;
        getEvePosition( newObject.email, 
                        newObject.id, &startX, &startY, 
                        &otherPeoplePos, allowEveRespawn, 
                        incrementEvePlacement );

        
        if( players.size() >= 
            SettingsManager::getIntSetting( "minActivePlayersForBirthlands", 
                                            15 )
            &&
            SettingsManager::getIntSetting( "specialBiomeBandMode", 0 ) ) {
            
            // shift Eve y position into center of her biome band
            startY = getSpecialBiomeBandYCenterForRace( 
                getObject( newObject.displayID )->race );
            }
        

        if( inCurseStatus.curseLevel > 0 ) {
            // keep cursed players away

            // 20K away in X and 20K away in Y, pushing out away from 0
            // in both directions

            if( startX > 0 )
                startX += 20000;
            else
                startX -= 20000;
            
            if( startY > 0 )
                startY += 20000;
            else
                startY -= 20000;
            }
        

        if( SettingsManager::getIntSetting( "forceEveLocation", 0 ) ) {

            startX = 
                SettingsManager::getIntSetting( "forceEveLocationX", 0 );
            startY = 
                SettingsManager::getIntSetting( "forceEveLocationY", 0 );
            }
        
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    

    if( inForceDisplayID != -1 ) {
        newObject.displayID = inForceDisplayID;
        }


    
    if( parent == NULL ) {
        // Eve
        int forceID = SettingsManager::getIntSetting( "forceEveObject", 0 );
    
        if( forceID > 0 ) {
            newObject.displayID = forceID;
            }
        
        
        float forceAge = SettingsManager::getFloatSetting( "forceEveAge", 0.0 );
        
        if( forceAge > 0 ) {
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - forceAge * ( 1.0 / getAgeRate() );
            }
        }
    

    newObject.holdingID = 0;


    if( areTriggersEnabled() ) {
        int id = getTriggerPlayerDisplayID( inEmail );
        
        if( id != -1 ) {
            newObject.displayID = id;
            
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - 
                getTriggerPlayerAge( inEmail ) * ( 1.0 / getAgeRate() );
        
            GridPos pos = getTriggerPlayerPos( inEmail );
            
            newObject.xd = pos.x;
            newObject.yd = pos.y;
            newObject.xs = pos.x;
            newObject.ys = pos.y;
            newObject.xd = pos.x;
            
            newObject.holdingID = getTriggerPlayerHolding( inEmail );
            newObject.clothing = getTriggerPlayerClothing( inEmail );
            }
        }
    
    
    newObject.lineage = new SimpleVector<int>();
    
    newObject.name = NULL;
    newObject.familyName = NULL;
    
    newObject.nameHasSuffix = false;
    newObject.lastSay = NULL;
    newObject.curseStatus = inCurseStatus;
    newObject.lifeStats = inLifeStats;
    

    if( newObject.curseStatus.curseLevel == 0 &&
        ! newObject.isTwin &&
        hasCurseToken( inEmail ) ) {
        newObject.curseTokenCount = 1;
        }
    else {
        newObject.curseTokenCount = 0;
        }

    newObject.curseTokenUpdate = true;

    
    newObject.pathLength = 0;
    newObject.pathToDest = NULL;
    newObject.pathTruncated = 0;
    newObject.firstMapSent = false;
    newObject.lastSentMapX = 0;
    newObject.lastSentMapY = 0;
    newObject.moveStartTime = Time::getCurrentTime();
    newObject.moveTotalSeconds = 0;
    newObject.facingOverride = 0;
    newObject.actionAttempt = 0;
    newObject.actionTarget.x = 0;
    newObject.actionTarget.y = 0;
    newObject.holdingEtaDecay = 0;
    newObject.heldOriginValid = 0;
    newObject.heldOriginX = 0;
    newObject.heldOriginY = 0;

    newObject.heldGraveOriginX = 0;
    newObject.heldGraveOriginY = 0;
    newObject.heldGravePlayerID = 0;
    
    newObject.heldTransitionSourceID = -1;
    newObject.numContained = 0;
    newObject.containedIDs = NULL;
    newObject.containedEtaDecays = NULL;
    newObject.subContainedIDs = NULL;
    newObject.subContainedEtaDecays = NULL;
    newObject.embeddedWeaponID = 0;
    newObject.embeddedWeaponEtaDecay = 0;
    newObject.murderSourceID = 0;
    newObject.holdingWound = false;
    newObject.holdingBiomeSickness = false;
    
    newObject.murderPerpID = 0;
    newObject.murderPerpEmail = NULL;
    
    newObject.deathSourceID = 0;
    
    newObject.everKilledAnyone = false;
    newObject.suicide = false;
    
    newObject.lastKillTime = 0;
    

    newObject.sock = inSock;
    newObject.sockBuffer = inSockBuffer;
    
    newObject.gotPartOfThisFrame = false;
    
    newObject.isNew = true;
    newObject.isNewCursed = false;
    newObject.firstMessageSent = false;
    newObject.inFlight = false;
    
    newObject.dying = false;
    newObject.dyingETA = 0;
    
    newObject.emotFrozen = false;
    newObject.emotUnfreezeETA = 0;
    newObject.emotFrozenIndex = 0;
    
    newObject.starving = false;

    newObject.connected = true;
    newObject.error = false;
    newObject.errorCauseString = "";
    
    newObject.customGraveID = -1;
    newObject.deathReason = NULL;
    
    newObject.deleteSent = false;
    newObject.deathLogged = false;
    newObject.newMove = false;
    
    newObject.posForced = false;
    newObject.waitingForForceResponse = false;
    
    // first move that player sends will be 2
    newObject.lastMoveSequenceNumber = 1;

    newObject.needsUpdate = false;
    newObject.updateSent = false;
    newObject.updateGlobal = false;
    
    newObject.wiggleUpdate = false;

    newObject.babyBirthTimes = new SimpleVector<timeSec_t>();
    newObject.babyIDs = new SimpleVector<int>();
    
    newObject.birthCoolDown = 0;
    
    newObject.monumentPosSet = false;
    newObject.monumentPosSent = true;
    
    newObject.monumentPosInherited = false;

    newObject.holdingFlightObject = false;

    newObject.vogMode = false;
    newObject.postVogMode = false;
    newObject.vogJumpIndex = 0;
    
    newObject.forceSpawn = false;

    newObject.forceFlightDestSetTime = 0;
                
    for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
        newObject.heatMap[i] = 0;
        }

    
    newObject.parentID = -1;
    char *parentEmail = NULL;

    if( parent != NULL && isFertileAge( parent ) ) {
        // do not log babies that new Eve spawns next to as parents
        newObject.parentID = parent->id;
        parentEmail = parent->email;

        if( parent->familyName != NULL ) {
            newObject.familyName = stringDuplicate( parent->familyName );
            }

        newObject.lineageEveID = parent->lineageEveID;

        // child inherits mother's leader
        newObject.followingID = parent->followingID;
        
        if( newObject.followingID == -1 ) {
            // boostrap the whole thing by having leaderless mothers
            // get their children as automatic followers.
            newObject.followingID = parent->id;
            
            // set mother's leadership color
            if( parent->leadingColorIndex == -1 ) {
                parent->leadingColorIndex = getUnusedLeadershipColor();
                }
            }


        newObject.parentChainLength = parent->parentChainLength + 1;

        // mother
        newObject.lineage->push_back( newObject.parentID );

        
        // inherit mother's craving at time of birth
        newObject.cravingFood = parent->cravingFood;
        
        // increment for next generation
        newObject.cravingFoodYumIncrement = parent->cravingFoodYumIncrement + 1;
        

        // inherit last heard monument, if any, from parent

        // but only if parent heard the monument call directly, not if THEY
        // also inherited it
        // Don't keep propagating across multiple generations.
        if( ! parent->monumentPosInherited ) {
            newObject.monumentPosSet = parent->monumentPosSet;
            newObject.lastMonumentPos = parent->lastMonumentPos;
            newObject.lastMonumentID = parent->lastMonumentID;
            if( newObject.monumentPosSet ) {
                newObject.monumentPosSent = false;
                newObject.monumentPosInherited = true;
                }
            }
        
        
        for( int i=0; 
             i < parent->lineage->size() && 
                 i < maxLineageTracked - 1;
             i++ ) {
            
            newObject.lineage->push_back( 
                parent->lineage->getElementDirect( i ) );
            }

        if( strstr( newObject.email, "paxkiosk" ) ) {
            // whoa, this baby is a PAX player!
            // let the mother know
            sendGlobalMessage( 
                (char*)"YOUR BABY IS A NEW PLAYER FROM THE PAX EXPO BOOTH.**"
                "PLEASE HELP THEM LEARN THE GAME.  THANKS!  -JASON",
                parent );
            }
        else if( isUsingStatsServer() && 
                 ! newObject.lifeStats.error &&
                 ( newObject.lifeStats.lifeCount < 
                   SettingsManager::getIntSetting( "newPlayerLifeCount", 5 ) ||
                   newObject.lifeStats.lifeTotalSeconds < 
                   SettingsManager::getIntSetting( "newPlayerLifeTotalSeconds",
                                                   7200 ) ) ) {
            // a new player (not at a PAX kiosk)
            // let mother know
            char *motherMessage =  
                SettingsManager::getSettingContents( 
                    "newPlayerMessageForMother", "" );
            
            if( strcmp( motherMessage, "" ) != 0 ) {
                sendGlobalMessage( motherMessage, parent );
                }
            
            delete [] motherMessage;
            }
        }

    newObject.personalEatBonus = 0;
    newObject.personalFoodDecrementSecondsBonus = 0;

    if( ! newObject.isTutorial &&
        isUsingStatsServer() &&
        ! newObject.lifeStats.error ) {
        
        int sec = newObject.lifeStats.lifeTotalSeconds;

        double halfLifeFactor = 
            pow( 0.5, sec / newPlayerFoodBonusHalfLifeSeconds );
        

        newObject.personalEatBonus = 
            lrint( halfLifeFactor * newPlayerFoodEatingBonus );
        
        newObject.personalFoodDecrementSecondsBonus =
            lrint( halfLifeFactor * newPlayerFoodDecrementSecondsBonus );
        }
    
    newObject.foodDecrementETASeconds =
        currentTime + 
        computeFoodDecrementTimeSeconds( &newObject );

        
    if( forceSpawn ) {
        newObject.forceSpawn = true;
        newObject.xs = forceSpawnInfo.pos.x;
        newObject.ys = forceSpawnInfo.pos.y;
        newObject.xd = forceSpawnInfo.pos.x;
        newObject.yd = forceSpawnInfo.pos.y;
        
        newObject.birthPos = forceSpawnInfo.pos;
        
        newObject.lifeStartTimeSeconds = 
            Time::getCurrentTime() -
            forceSpawnInfo.age * ( 1.0 / getAgeRate() );
        
        newObject.name = autoSprintf( "%s %s", 
                                      forceSpawnInfo.firstName,
                                      forceSpawnInfo.lastName );
        newObject.displayID = forceSpawnInfo.displayID;
        
        newObject.clothing.hat = getObject( forceSpawnInfo.hatID, true );
        newObject.clothing.tunic = getObject( forceSpawnInfo.tunicID, true );
        newObject.clothing.bottom = getObject( forceSpawnInfo.bottomID, true );
        newObject.clothing.frontShoe = 
            getObject( forceSpawnInfo.frontShoeID, true );
        newObject.clothing.backShoe = 
            getObject( forceSpawnInfo.backShoeID, true );

        delete [] forceSpawnInfo.firstName;
        delete [] forceSpawnInfo.lastName;
        }
    

    newObject.lastGlobalMessageTime = 0;
    

    newObject.birthPos.x = newObject.xd;
    newObject.birthPos.y = newObject.yd;
    
    newObject.originalBirthPos = newObject.birthPos;
    

    newObject.heldOriginX = newObject.xd;
    newObject.heldOriginY = newObject.yd;
    
    newObject.actionTarget = newObject.birthPos;



    newObject.ancestorIDs = new SimpleVector<int>();
    newObject.ancestorEmails = new SimpleVector<char*>();
    newObject.ancestorRelNames = new SimpleVector<char*>();
    newObject.ancestorLifeStartTimeSeconds = new SimpleVector<double>();
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        
        if( otherPlayer->error ) {
            continue;
            }
        
        // a living other player
        
        // consider all men here
        // and any childless women (they are counted as aunts
        // for any children born before they themselves have children
        // or after all their own children die)
        if( newObject.parentID != otherPlayer->id 
            &&
            ( ! getFemale( otherPlayer ) ||
              countLivingChildren( otherPlayer->id ) == 0 ) ) {
            
            // check if his mother is an ancestor
            // (then he's an uncle, or she's a childless aunt)
            if( otherPlayer->parentID > 0 ) {
                
                // look at lineage above parent
                // don't count brothers, only uncles
                for( int i=1; i<newObject.lineage->size(); i++ ) {
                    
                    if( newObject.lineage->getElementDirect( i ) ==
                        otherPlayer->parentID ) {
                        
                        newObject.ancestorIDs->push_back( otherPlayer->id );

                        newObject.ancestorEmails->push_back( 
                            stringDuplicate( otherPlayer->email ) );

                        // i tells us how many greats
                        SimpleVector<char> workingName;
                        
                        for( int g=2; g<=i; g++ ) {
                            workingName.appendElementString( "Great_" );
                            }
                        if( ! getFemale( &newObject ) ) {
                            workingName.appendElementString( "Nephew" );
                            }
                        else {
                            workingName.appendElementString( "Niece" );
                            }

                        newObject.ancestorRelNames->push_back(
                            workingName.getElementString() );
                        
                        newObject.ancestorLifeStartTimeSeconds->push_back(
                            otherPlayer->lifeStartTimeSeconds );
                        
                        break;
                        }
                    }
                }
            }
        else {
            // females, look for direct ancestry

            for( int i=0; i<newObject.lineage->size(); i++ ) {
                    
                if( newObject.lineage->getElementDirect( i ) ==
                    otherPlayer->id ) {
                        
                    newObject.ancestorIDs->push_back( otherPlayer->id );

                    newObject.ancestorEmails->push_back( 
                        stringDuplicate( otherPlayer->email ) );

                    // i tells us how many greats and grands
                    SimpleVector<char> workingName;
                    SimpleVector<char> workingMotherName;
                    
                    for( int g=1; g<=i; g++ ) {
                        if( g == i ) {
                            workingName.appendElementString( "Grand" );
                            workingMotherName.appendElementString( "Grand" );
                            }
                        else {
                            workingName.appendElementString( "Great_" );
                            workingMotherName.appendElementString( "Great_" );
                            }
                        }
                    
                    
                    if( i != 0 ) {
                        if( ! getFemale( &newObject ) ) {
                            workingName.appendElementString( "son" );
                            }
                        else {
                            workingName.appendElementString( "daughter" );
                            }
                        workingMotherName.appendElementString( "mother" );
                        }
                    else {
                        // no "Grand"
                        if( ! getFemale( &newObject ) ) {
                                workingName.appendElementString( "Son" );
                            }
                        else {
                            workingName.appendElementString( "Daughter" );
                            }
                        workingMotherName.appendElementString( "Mother" );
                        }
                    
                    
                    newObject.ancestorRelNames->push_back(
                        workingName.getElementString() );
                    
                    newObject.ancestorLifeStartTimeSeconds->push_back(
                            otherPlayer->lifeStartTimeSeconds );
                    
                    // this is the only case of bi-directionality
                    // players should try to prevent their mothers, gma,
                    // ggma, etc from dying

                    otherPlayer->ancestorIDs->push_back( newObject.id );
                    otherPlayer->ancestorEmails->push_back( 
                        stringDuplicate( newObject.email ) );
                    otherPlayer->ancestorRelNames->push_back( 
                        workingMotherName.getElementString() );
                    otherPlayer->ancestorLifeStartTimeSeconds->push_back(
                        newObject.lifeStartTimeSeconds );
                    
                    break;
                    }
                }
            }
        
        // if we got here, they aren't our mother, g-ma, g-g-ma, etc.
        // nor are they our uncle

        
        // are they our sibling?
        // note that this is only uni-directional
        // (we're checking here for this new baby born)
        // so only our OLDER sibs count as our ancestors (and thus
        // they care about protecting us).

        // this is a little weird, but it does make some sense
        // you are more protective of little sibs

        // anyway, the point of this is to close the "just care about yourself
        // and avoid having kids" exploit.  If your mother has kids after you
        // (which is totally out of your control), then their survival
        // will affect your score.
        
        if( newObject.parentID > 0 &&
            newObject.parentID == otherPlayer->parentID &&
            ( firstTwinID == -1 || otherPlayer->id < firstTwinID ) ) {
            
            // sibs, but NOT twins
            // only consider sibs that joined before the firstTwinID in
            // our twin set
            
            newObject.ancestorIDs->push_back( otherPlayer->id );

            newObject.ancestorEmails->push_back( 
                stringDuplicate( otherPlayer->email ) );

            const char *relName;
            
            if( ! getFemale( &newObject ) ) {
                relName = "Little_Brother";
                }
            else {
                relName = "Little_Sister";
                }

            newObject.ancestorRelNames->push_back( stringDuplicate( relName ) );
            
            newObject.ancestorLifeStartTimeSeconds->push_back(
                otherPlayer->lifeStartTimeSeconds );
                        
            continue;
            }
        }
    

    

    
    // parent pointer possibly no longer valid after push_back, which
    // can resize the vector
    parent = NULL;

    newObject.numToolSlots = -1;
    

    if( newObject.isTutorial ) {
        AppLog::infoF( "New player %s pending tutorial load (tutorial=%d)",
                       newObject.email,
                       inTutorialNumber );

        // holding bay for loading tutorial maps incrementally
        tutorialLoadingPlayers.push_back( newObject );
        }
    else {
        players.push_back( newObject );            
        }

    if( newObject.isEve ) {
        addEveLanguage( newObject.id );
        }
    else {
        incrementLanguageCount( newObject.lineageEveID );
        }
    

    addRecentScore( newObject.email, inFitnessScore );
    

    if( ! newObject.isTutorial )        
    logBirth( newObject.id,
              newObject.email,
              newObject.parentID,
              parentEmail,
              ! getFemale( &newObject ),
              newObject.xd,
              newObject.yd,
              players.size(),
              newObject.parentChainLength );
    
    AppLog::infoF( "New player %s connected as player %d (tutorial=%d) (%d,%d)"
                   " (maxPlacementX=%d)",
                   newObject.email, newObject.id,
                   inTutorialNumber, newObject.xs, newObject.ys,
                   maxPlacementX );

    // generate log line whenever a new baby is born
    logFamilyCounts();


    double curTime = Time::getCurrentTime();

    // tell non-mother ancestors about this baby
    for( int i=0; i<newObject.ancestorIDs->size(); i++ ) {
        int id = newObject.ancestorIDs->getElementDirect( i );
        
        if( id == newObject.id ) {
            continue;
            }
        
        // skip this baby when saying +FAMILY+
        // we are already getting a pointer to them, probably
        makeOffspringSayMarker( id, newObject.id );
        

        if( id == newObject.parentID ) {
            continue;
            }
        
        LiveObject *o = getLiveObject( id );
        
        if( o != NULL && ! o->error && o->connected &&
            computeAge( o ) >= defaultActionAge &&
            // at most one new baby notice per minute
            curTime - o->lastNewBabyNoticeTime > 60 ) {
            
            char *message = autoSprintf( "PS\n"
                                         "%d/0 A NEW OFFSPRING BABY "
                                         "*baby %d *map %d %d\n#",
                                         id,
                                         newObject.id,
                                         newObject.xs - o->birthPos.x,
                                         newObject.ys - o->birthPos.y );
            sendMessageToPlayer( o, message, strlen( message ) );
            delete [] message;
            
            o->lastNewBabyNoticeTime = curTime;
            }
        }
    

    logHomelandBirth( newObject.xs, newObject.ys,
                      newObject.lineageEveID );
    
    return newObject.id;
    }






static char isMapSpotBlockingForPlayer( LiveObject *inPlayer,
                                        int inX, int inY ) {
    
    if( isMapSpotBlocking( inX, inY ) ) {
        return true;
        }
    int oID = getMapObject( inX, inY );
    
    if( oID <= 0 ) {
        return false;
        }

    ObjectRecord *o = getObject( oID );
    
    if( o->isOwned && o->blocksNonFollower ) {
        
        if( isMapSpotLeaderOwned( inPlayer, inX, inY ) ) {
            return false;
            }
        return true;
        }
    
    return false;    
    }




static void processWaitingTwinConnection( FreshConnection inConnection ) {
    AppLog::infoF( "Player %s waiting for twin party of %d", 
                   inConnection.email,
                   inConnection.twinCount );
    waitingForTwinConnections.push_back( inConnection );
    
    CurseStatus anyTwinCurseLevel = inConnection.curseStatus;
    

    // count how many match twin code from inConnection
    // is this the last one to join the party?
    SimpleVector<FreshConnection*> twinConnections;
    

    for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
        FreshConnection *nextConnection = 
            waitingForTwinConnections.getElement( i );
        
        if( nextConnection->error ) {
            continue;
            }
        
        if( nextConnection->twinCode != NULL
            &&
            strcmp( inConnection.twinCode, nextConnection->twinCode ) == 0 
            &&
            inConnection.twinCount == nextConnection->twinCount ) {
            
            if( strcmp( inConnection.email, nextConnection->email ) == 0 ) {
                // don't count this connection itself
                continue;
                }

            if( nextConnection->curseStatus.curseLevel > 
                anyTwinCurseLevel.curseLevel ) {
                anyTwinCurseLevel = nextConnection->curseStatus;
                }
            
            twinConnections.push_back( nextConnection );
            }
        }

    
    if( twinConnections.size() + 1 >= inConnection.twinCount ) {
        // everyone connected and ready in twin party

        AppLog::infoF( "Found %d other people waiting for twin party of %s, "
                       "ready", 
                       twinConnections.size(), inConnection.email );
        
        char *emailCopy = stringDuplicate( inConnection.email );
        
        // set up twin emails for lineage ban
        for( int i=0; i<twinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                twinConnections.getElementDirect( i );
        
            tempTwinEmails.push_back( nextConnection->email );
            }
        
        nextLogInTwin = true;
        firstTwinID = -1;
        
        int newID = processLoggedInPlayer( false,
                                           inConnection.sock,
                                           inConnection.sockBuffer,
                                           inConnection.email,
                                           inConnection.tutorialNumber,
                                           anyTwinCurseLevel,
                                           inConnection.lifeStats,
                                           inConnection.fitnessScore );
        tempTwinEmails.deleteAll();
        
        if( newID == -1 ) {
            AppLog::infoF( "%s reconnected to existing life, not triggering "
                           "fellow twins to spawn now.",
                           emailCopy );

            // take them out of waiting list too
            for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
                if( waitingForTwinConnections.getElement( i )->sock ==
                    inConnection.sock ) {
                    // found
                    
                    waitingForTwinConnections.deleteElement( i );
                    break;
                    }
                }

            delete [] emailCopy;

            if( inConnection.twinCode != NULL ) {
                delete [] inConnection.twinCode;
                inConnection.twinCode = NULL;
                }
            nextLogInTwin = false;
            return;
            }

        delete [] emailCopy;
        
        firstTwinID = newID;
        
        LiveObject *newPlayer = NULL;

        if( inConnection.tutorialNumber == 0 ) {
            newPlayer = getLiveObject( newID );
            }
        else {
            newPlayer = tutorialLoadingPlayers.getElement(
                tutorialLoadingPlayers.size() - 1 );
            }


        int parent = newPlayer->parentID;
        int displayID = newPlayer->displayID;
        GridPos playerPos = { newPlayer->xd, newPlayer->yd };
        
        GridPos *forcedEvePos = NULL;
        
        if( parent == -1 ) {
            // first twin placed was Eve
            // others are identical Eves
            forcedEvePos = &playerPos;
            // trigger forced Eve placement
            parent = -2;
            }


        char usePersonalCurses = 
            SettingsManager::getIntSetting( "usePersonalCurses", 0 );
    


        // save these out here, because newPlayer points into 
        // tutorialLoadingPlayers, which may expand during this loop,
        // invalidating that pointer
        char isTutorial = newPlayer->isTutorial;
        TutorialLoadProgress sharedTutorialLoad = newPlayer->tutorialLoad;

        for( int i=0; i<twinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                twinConnections.getElementDirect( i );
            
            processLoggedInPlayer( false, 
                                   nextConnection->sock,
                                   nextConnection->sockBuffer,
                                   nextConnection->email,
                                   // ignore tutorial number of all but
                                   // first player
                                   0,
                                   anyTwinCurseLevel,
                                   nextConnection->lifeStats,
                                   nextConnection->fitnessScore,
                                   parent,
                                   displayID,
                                   forcedEvePos );
            
            // just added is always last object in list
            
            if( usePersonalCurses ) {
                // curse level not known until after first twin logs in
                // their curse level is set based on blockage caused
                // by any of the other twins in the party
                // pass it on.
                LiveObject *newTwinPlayer = 
                    players.getElement( players.size() - 1 );
                newTwinPlayer->curseStatus = newPlayer->curseStatus;
                }



            LiveObject newTwinPlayer = 
                players.getElementDirect( players.size() - 1 );

            if( isTutorial ) {
                // force this one to wait for same tutorial map load
                newTwinPlayer.tutorialLoad = sharedTutorialLoad;

                // flag them as a tutorial player too, so they can't have
                // babies in the tutorial, and they won't be remembered
                // as a long-lineage position at shutdown
                newTwinPlayer.isTutorial = true;

                players.deleteElement( players.size() - 1 );
                
                tutorialLoadingPlayers.push_back( newTwinPlayer );
                }
            }
        
        firstTwinID = -1;

        char *twinCode = stringDuplicate( inConnection.twinCode );
        
        for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                waitingForTwinConnections.getElement( i );
            
            if( nextConnection->error ) {
                continue;
                }
            
            if( nextConnection->twinCode != NULL 
                &&
                nextConnection->twinCount == inConnection.twinCount
                &&
                strcmp( nextConnection->twinCode, twinCode ) == 0 ) {
                
                delete [] nextConnection->twinCode;
                waitingForTwinConnections.deleteElement( i );
                i--;
                }
            }
        
        delete [] twinCode;
        
        nextLogInTwin = false;
        }
    }




// doesn't check whether dest itself is blocked
static char directLineBlocked( LiveObject *inShooter,
                               GridPos inSource, GridPos inDest ) {
    // line algorithm from here
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    
    double deltaX = inDest.x - inSource.x;
    
    double deltaY = inDest.y - inSource.y;
    

    int xStep = 1;
    if( deltaX < 0 ) {
        xStep = -1;
        }
    
    int yStep = 1;
    if( deltaY < 0 ) {
        yStep = -1;
        }
    

    if( deltaX == 0 ) {
        // vertical line
        
        // just walk through y
        for( int y=inSource.y; y != inDest.y; y += yStep ) {
            if( isMapSpotBlockingForPlayer( inShooter, inSource.x, y ) ) {
                return true;
                }
            }
        }
    else {
        double deltaErr = fabs( deltaY / (double)deltaX );
        
        double error = 0;
        
        int y = inSource.y;
        for( int x=inSource.x; x != inDest.x || y != inDest.y; x += xStep ) {
            if( isMapSpotBlockingForPlayer( inShooter, x, y ) ) {
                return true;
                }
            error += deltaErr;
            
            if( error >= 0.5 ) {
                y += yStep;
                error -= 1.0;
                }
            
            // we may need to take multiple steps in y
            // if line is vertically oriented
            while( error >= 0.5 ) {
                if( isMapSpotBlockingForPlayer( inShooter, x, y ) ) {
                    return true;
                    }

                y += yStep;
                error -= 1.0;
                }
            }
        }

    return false;
    }



char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber );




// find index of spot on container held item can swap with, or -1 if none found
static int getContainerSwapIndex( LiveObject *inPlayer,
                                  int idToAdd,
                                  int inStillHeld,
                                  int inSearchLimit,
                                  int inContX, int inContY ) {
    // take what's on bottom of container, but only if it's different
    // from what's in our hand
    // AND we are old enough to take it
    double playerAge = computeAge( inPlayer );
    
    // if we find a same object on bottom, keep going up until
    // we find a non-same one to swap
    for( int botInd = 0; botInd < inSearchLimit; botInd ++ ) {
        
        char same = false;
        
        int bottomItem = 
            getContained( inContX, inContY, botInd, 0 );
        
        if( bottomItem > 0 &&
            ! canPickup( bottomItem, playerAge ) ) {
            // too young to hold!
            same = true;
            }
        else if( bottomItem == idToAdd ) {
            if( bottomItem > 0 ) {
                // not sub conts
                same = true;
                }
            else {
                // they must contain same stuff to be same
                int bottomNum = getNumContained( inContX, inContY,
                                                 botInd + 1 );
                int topNum;

                if( inStillHeld ) {
                    topNum = inPlayer->numContained;
                    }
                else {
                    // already in the container
                    topNum =  getNumContained( inContX, inContY,
                                               inSearchLimit + 1 );
                    }
                
                if( bottomNum != topNum ) {
                    same = false;
                    }
                else {
                    same = true;
                    for( int b=0; b<bottomNum; b++ ) {
                        int subB = getContained( inContX, inContY,
                                                 b, botInd + 1 );
                        int subT;

                        if( inStillHeld ) {
                            subT = inPlayer->containedIDs[b];
                            }
                        else {
                            subT = getContained( inContX, inContY,
                                                 b, inSearchLimit + 1 );
                            }
                        
                                
                        if( subB != subT ) {
                            same = false;
                            break;
                            }
                        }
                    }
                }
            }
        if( !same ) {
            return botInd;
            }
        }
    
    return -1;
    }



// checks for granular +cont containment limitations
// assumes that container size limitation and 
// containable property checked elsewhere
static char containmentPermitted( int inContainerID, int inContainedID ) {
    ObjectRecord *containedO = getObject( inContainedID );
    
    char *contLoc = strstr( containedO->description, "+cont" );
    
    if( contLoc == NULL ) {
        // not a limited containable object
        return true;
        }
    
    char *limitNameLoc = &( contLoc[5] );
    
    if( limitNameLoc[0] != ' ' &&
        limitNameLoc[0] != '\0' ) {

        // there's something after +cont
        // scan the whole thing, including +cont

        char tag[100];
        
        int numRead = sscanf( contLoc, "%99s", tag );
        
        if( numRead == 1 ) {
            
            // clean up # character that might delimit end of string
            int tagLen = strlen( tag );
            
            for( int i=0; i<tagLen; i++ ) {
                if( tag[i] == '#' ) {
                    tag[i] = '\0';
                    tagLen = i;
                    break;
                    }
                }

            char *locInContainerName =
                strstr( getObject( inContainerID )->description, tag );
            
            if( locInContainerName != NULL ) {
                // skip to end of tag
                // and make sure tag isn't a sub-tag of container tag
                // don't want contained to be +contHot
                // and contaienr to be +contHotPlates
                
                char end = locInContainerName[ tagLen ];
                
                if( end == ' ' ||
                    end == '\0'||
                    end == '#' ) {
                    return true;
                    }
                }
            return false;
            }
        }
    
    // +cont with nothing after it, no limit
    return true;
    }

        




// swap indicates that we want to put the held item at the bottom
// of the container and take the top one
// returns true if added
static char addHeldToContainer( LiveObject *inPlayer,
                                int inTargetID,
                                int inContX, int inContY,
                                char inSwap = false ) {
    
    int target = inTargetID;
        
    int targetSlots = 
        getNumContainerSlots( target );
                                        
    ObjectRecord *targetObj =
        getObject( target );
    
    if( isGrave( target ) ) {
        return false;
        }
    if( targetObj->slotsLocked ) {
        return false;
        }

    float slotSize =
        targetObj->slotSize;
    
    float containSize =
        getObject( 
            inPlayer->holdingID )->
        containSize;

    int numIn = 
        getNumContained( inContX, inContY );

    
    int isRoom = false;
    

    if( numIn < targetSlots ) {
        isRoom = true;
        }
    else {
        // container full
        // but check if swap is possible

        if( inSwap ) {
            
            int idToAdd = inPlayer->holdingID;
            TransRecord *r = getPTrans( idToAdd, -1 );

            if( r != NULL && r->newActor == 0 && r->newTarget > 0 ) {
                idToAdd = r->newTarget;
                }
            
            
            if( inPlayer->numContained > 0 ) {
                // negative to indicate sub-container
                idToAdd *= -1;
                }

            int swapInd = getContainerSwapIndex ( inPlayer,
                                                  idToAdd,
                                                  true,
                                                  numIn,
                                                  inContX, inContY );
            if( swapInd != -1 ) {
                isRoom = true;
                }
            }
        }
    


    
    if( isRoom &&
        isContainable( 
            inPlayer->holdingID ) &&
        containSize <= slotSize &&
        containmentPermitted( inTargetID, inPlayer->holdingID ) ) {
        
        // add to container
        
        setResponsiblePlayer( 
            inPlayer->id );
        

        // adding something to a container acts like a drop
        // but some non-permanent held objects are supposed to go through 
        // a transition when they drop (example:  held wild piglet is
        // non-permanent, so it can be containable, but it supposed to
        // switch to a moving wild piglet when dropped.... we should
        // switch to this other wild piglet when putting it into a container
        // too)

        // "set-down" type bare ground
        // trans exists for what we're 
        // holding?
        TransRecord *r = getPTrans( inPlayer->holdingID, -1 );

        if( r != NULL && r->newActor == 0 && r->newTarget > 0 ) {
                                            
            // only applies if the 
            // bare-ground
            // trans leaves nothing in
            // our hand
            
            // first, change what they
            // are holding to this 
            // newTarget
            

            handleHoldingChange( 
                inPlayer,
                r->newTarget );
            }


        int idToAdd = inPlayer->holdingID;


        float stretch = getObject( idToAdd )->slotTimeStretch;
                    
                    

        if( inPlayer->numContained > 0 ) {
            // negative to indicate sub-container
            idToAdd *= -1;
            }

        

        addContained( 
            inContX, inContY,
            idToAdd,
            inPlayer->holdingEtaDecay );

        if( inPlayer->numContained > 0 ) {
            timeSec_t curTime = Time::timeSec();
            
            for( int c=0; c<inPlayer->numContained; c++ ) {
                
                // undo decay stretch before adding
                // (stretch applied by adding)
                if( stretch != 1.0 &&
                    inPlayer->containedEtaDecays[c] != 0 ) {
                
                    timeSec_t offset = 
                        inPlayer->containedEtaDecays[c] - curTime;
                    
                    offset = offset * stretch;
                    
                    inPlayer->containedEtaDecays[c] = curTime + offset;
                    }


                addContained( inContX, inContY, inPlayer->containedIDs[c],
                              inPlayer->containedEtaDecays[c],
                              numIn + 1 );
                }
            
            clearPlayerHeldContained( inPlayer );
            }
        

        
        setResponsiblePlayer( -1 );
        
        inPlayer->holdingID = 0;
        inPlayer->holdingEtaDecay = 0;
        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;

        int numInNow = getNumContained( inContX, inContY );
        
        if( inSwap &&  numInNow > 1 ) {
            
            int swapInd = getContainerSwapIndex( inPlayer, 
                                                 idToAdd,
                                                 false,
                                                 // don't consider top slot
                                                 // where we just put this
                                                 // new item
                                                 numInNow - 1,
                                                 inContX, inContY );
            if( swapInd != -1 ) {
                // found one to swap
                removeFromContainerToHold( inPlayer, inContX, inContY, 
                                           swapInd );
                }
            // if we didn't remove one, it means whole container is full
            // of identical items.
            // the swap action doesn't work, so we just let it
            // behave like an add action instead.
            }

        return true;
        }

    return false;
    }



// returns true if succeeded
char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber ) {
    inPlayer->heldOriginValid = 0;
    inPlayer->heldOriginX = 0;
    inPlayer->heldOriginY = 0;                        
    inPlayer->heldTransitionSourceID = -1;
    

    if( isGridAdjacent( inContX, inContY,
                        inPlayer->xd, 
                        inPlayer->yd ) 
        ||
        ( inContX == inPlayer->xd &&
          inContY == inPlayer->yd ) ) {
                            
        inPlayer->actionAttempt = 1;
        inPlayer->actionTarget.x = inContX;
        inPlayer->actionTarget.y = inContY;
                            
        if( inContX > inPlayer->xd ) {
            inPlayer->facingOverride = 1;
            }
        else if( inContX < inPlayer->xd ) {
            inPlayer->facingOverride = -1;
            }

        // can only use on targets next to us for now,
        // no diags
                            
        int target = getMapObject( inContX, inContY );
                            
        if( target != 0 ) {
                            
            if( target > 0 && getObject( target )->slotsLocked ) {
                return false;
                }

            int numIn = 
                getNumContained( inContX, inContY );
                                
            int toRemoveID = 0;
            
            double playerAge = computeAge( inPlayer );

            
            if( inSlotNumber < 0 ) {
                inSlotNumber = numIn - 1;
                
                // no slot specified
                // find top-most object that they can actually pick up

                int toRemoveID = getContained( 
                    inContX, inContY,
                    inSlotNumber );
                
                if( toRemoveID < 0 ) {
                    toRemoveID *= -1;
                    }
                
                while( inSlotNumber > 0 &&
                       ! canPickup( toRemoveID, playerAge ) )  {
            
                    inSlotNumber--;
                    
                    toRemoveID = getContained( 
                        inContX, inContY,
                        inSlotNumber );
                
                    if( toRemoveID < 0 ) {
                        toRemoveID *= -1;
                        }
                    }
                }
            


                                
            if( numIn > 0 ) {
                toRemoveID = getContained( inContX, inContY, inSlotNumber );
                }
            
            char subContain = false;
            
            if( toRemoveID < 0 ) {
                toRemoveID *= -1;
                subContain = true;
                }

            
            if( toRemoveID == 0 ) {
                // this should never happen, except due to map corruption
                
                // clear container, to be safe
                clearAllContained( inContX, inContY );
                return false;
                }


            if( inPlayer->holdingID == 0 && 
                numIn > 0 &&
                // old enough to handle it
                canPickup( toRemoveID, computeAge( inPlayer ) ) ) {
                // get from container


                if( subContain ) {
                    int subSlotNumber = inSlotNumber;
                    
                    if( subSlotNumber == -1 ) {
                        subSlotNumber = numIn - 1;
                        }

                    inPlayer->containedIDs =
                        getContained( inContX, inContY, 
                                      &( inPlayer->numContained ), 
                                      subSlotNumber + 1 );
                    inPlayer->containedEtaDecays =
                        getContainedEtaDecay( inContX, inContY, 
                                              &( inPlayer->numContained ), 
                                              subSlotNumber + 1 );

                    // these will be cleared when removeContained is called
                    // for this slot below, so just get them now without clearing

                    // empty vectors... there are no sub-sub containers
                    inPlayer->subContainedIDs = 
                        new SimpleVector<int>[ inPlayer->numContained ];
                    inPlayer->subContainedEtaDecays = 
                        new SimpleVector<timeSec_t>[ inPlayer->numContained ];
                
                    }
                
                
                setResponsiblePlayer( - inPlayer->id );
                
                inPlayer->holdingID =
                    removeContained( 
                        inContX, inContY, inSlotNumber,
                        &( inPlayer->holdingEtaDecay ) );
                

                // does bare-hand action apply to this newly-held object
                // one that results in something new in the hand and
                // nothing on the ground?

                // if so, it is a pick-up action, and it should apply here

                TransRecord *pickupTrans = getPTrans( 0, inPlayer->holdingID );
                
                if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
                    pickupTrans->newTarget == 0 ) {
                    
                    handleHoldingChange( inPlayer, pickupTrans->newActor );
                    }
                else {
                    holdingSomethingNew( inPlayer );
                    }
                
                setResponsiblePlayer( -1 );

                if( inPlayer->holdingID < 0 ) {
                    // sub-contained
                    
                    inPlayer->holdingID *= -1;    
                    }
                
                // contained objects aren't animating
                // in a way that needs to be smooth
                // transitioned on client
                inPlayer->heldOriginValid = 0;
                inPlayer->heldOriginX = 0;
                inPlayer->heldOriginY = 0;

                return true;
                }
            }
        }        
    
    return false;
    }



// outCouldHaveGoneIn, if non-NULL, is set to TRUE if clothing
// could potentialy contain what we're holding (even if clothing too full
// to contain it)
static char addHeldToClothingContainer( LiveObject *inPlayer, 
                                        int inC,
                                        // true if we should over-pack
                                        // container in anticipation of a swap
                                        char inWillSwap = false,
                                        char *outCouldHaveGoneIn = NULL ) {    
    // drop into own clothing
    ObjectRecord *cObj = 
        clothingByIndex( 
            inPlayer->clothing,
            inC );
                                    
    if( cObj != NULL &&
        isContainable( 
            inPlayer->holdingID ) ) {
                                        
        int oldNum =
            inPlayer->
            clothingContained[inC].size();
                                        
        float slotSize =
            cObj->slotSize;
                                        
        float containSize =
            getObject( inPlayer->holdingID )->
            containSize;
    
        char permitted = false;
        
        if( containSize <= slotSize &&
            cObj->numSlots > 0 &&
            containmentPermitted( cObj->id, inPlayer->holdingID ) ) {
            permitted = true;
            }
        
        if( containSize <= slotSize &&
            cObj->numSlots > 0 &&
            permitted &&
            outCouldHaveGoneIn != NULL ) {
            *outCouldHaveGoneIn = true;
            }

        if( ( oldNum < cObj->numSlots
              || ( oldNum == cObj->numSlots && inWillSwap ) )
            &&
            containSize <= slotSize &&
            permitted ) {
            // room (or will swap, so we can over-pack it)
            inPlayer->clothingContained[inC].
                push_back( 
                    inPlayer->holdingID );

            if( inPlayer->
                holdingEtaDecay != 0 ) {
                                                
                timeSec_t curTime = 
                    Time::timeSec();
                                            
                timeSec_t offset = 
                    inPlayer->
                    holdingEtaDecay - 
                    curTime;
                                            
                offset = 
                    offset / 
                    cObj->
                    slotTimeStretch;
                                                
                inPlayer->holdingEtaDecay =
                    curTime + offset;
                }
                                            
            inPlayer->
                clothingContainedEtaDecays[inC].
                push_back( inPlayer->
                           holdingEtaDecay );
                                        
            inPlayer->holdingID = 0;
            inPlayer->holdingEtaDecay = 0;
            inPlayer->heldOriginValid = 0;
            inPlayer->heldOriginX = 0;
            inPlayer->heldOriginY = 0;
            inPlayer->heldTransitionSourceID =
                -1;

            return true;
            }
        }

    return false;
    }



static void changeContained( int inX, int inY, int inSlotNumber, 
                             int inNewObjectID ) {
    
    int numContained = 0;
    int *contained = getContained( inX, inY, &numContained );

    timeSec_t *containedETA = 
        getContainedEtaDecay( inX, inY, &numContained );
    
    timeSec_t curTimeSec = Time::timeSec();
    
    if( contained != NULL && containedETA != NULL &&
        numContained > inSlotNumber ) {
    
        int oldObjectID = contained[ inSlotNumber ];
        timeSec_t oldETA = containedETA[ inSlotNumber ];
        
        if( oldObjectID > 0 ) {
            
            TransRecord *oldDecayTrans = getTrans( -1, oldObjectID );

            TransRecord *newDecayTrans = getTrans( -1, inNewObjectID );
            

            timeSec_t newETA = 0;
            
            if( newDecayTrans != NULL ) {
                newETA = curTimeSec + newDecayTrans->autoDecaySeconds;
                }
            
            if( oldDecayTrans != NULL && newDecayTrans != NULL &&
                oldDecayTrans->autoDecaySeconds == 
                newDecayTrans->autoDecaySeconds ) {
                // preserve remaining seconds from old object
                newETA = oldETA;
                }
            
            contained[ inSlotNumber ] = inNewObjectID;
            containedETA[ inSlotNumber ] = newETA;

            setContained( inX, inY, numContained, contained );
            setContainedEtaDecay( inX, inY, numContained, containedETA );
            }
        }

    if( contained != NULL ) {
        delete [] contained;
        }
    if( containedETA != NULL ) {
        delete [] containedETA;
        }
    }



static void setHeldGraveOrigin( LiveObject *inPlayer, int inX, int inY,
                                int inNewTarget ) {
    // make sure that there is nothing left there
    // for now, transitions that remove graves leave nothing behind
    if( inNewTarget == 0 ) {
        
        // make sure that that there was a grave there before
        int gravePlayerID = getGravePlayerID( inX, inY );
        
        // clear it
        inPlayer->heldGravePlayerID = 0;
            

        if( gravePlayerID > 0 ) {
            
            // player action actually picked up this grave
            
            if( inPlayer->holdingID > 0 &&
                strstr( getObject( inPlayer->holdingID )->description, 
                        "origGrave" ) != NULL ) {
                
                inPlayer->heldGraveOriginX = inX;
                inPlayer->heldGraveOriginY = inY;
                
                inPlayer->heldGravePlayerID = getGravePlayerID( inX, inY );
                }
            
            // clear it from ground
            setGravePlayerID( inX, inY, 0 );
            }
        }
    
    }



static void pickupToHold( LiveObject *inPlayer, int inX, int inY, 
                          int inTargetID ) {
    inPlayer->holdingEtaDecay = 
        getEtaDecay( inX, inY );
    
    FullMapContained f =
        getFullMapContained( inX, inY );
    
    setContained( inPlayer, f );
    
    clearAllContained( inX, inY );
    
    setResponsiblePlayer( - inPlayer->id );
    setMapObject( inX, inY, 0 );
    setResponsiblePlayer( -1 );
    
    inPlayer->holdingID = inTargetID;
    holdingSomethingNew( inPlayer );

    setHeldGraveOrigin( inPlayer, inX, inY, 0 );
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inX;
    inPlayer->heldOriginY = inY;
    inPlayer->heldTransitionSourceID = -1;
    }


// returns true if it worked
static char removeFromClothingContainerToHold( LiveObject *inPlayer,
                                               int inC,
                                               int inI = -1 ) {    
    
    ObjectRecord *cObj = 
        clothingByIndex( inPlayer->clothing, 
                         inC );
                                
    float stretch = 1.0f;
    
    if( cObj != NULL ) {
        stretch = cObj->slotTimeStretch;
        }
    
    int oldNumContained = 
        inPlayer->clothingContained[inC].size();

    int slotToRemove = inI;

    double playerAge = computeAge( inPlayer );

                                
    if( slotToRemove < 0 ) {
        slotToRemove = oldNumContained - 1;

        // no slot specified
        // find top-most object that they can actually pick up

        while( slotToRemove > 0 &&
               ! canPickup( inPlayer->clothingContained[inC].
                            getElementDirect( slotToRemove ), 
                            playerAge ) ) {
            
            slotToRemove --;
            }
        }
                                
    int toRemoveID = -1;
                                
    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 ) {
                                    
        toRemoveID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        }

    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 &&
        // old enough to handle it
        canPickup( toRemoveID, playerAge ) ) {
                                    

        inPlayer->holdingID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        holdingSomethingNew( inPlayer );

        inPlayer->holdingEtaDecay = 
            inPlayer->
            clothingContainedEtaDecays[inC].
            getElementDirect( slotToRemove );
                                    
        timeSec_t curTime = Time::timeSec();

        if( inPlayer->holdingEtaDecay != 0 ) {
                                        
            timeSec_t offset = 
                inPlayer->holdingEtaDecay
                - curTime;
            offset = offset * stretch;
            inPlayer->holdingEtaDecay =
                curTime + offset;
            }

        inPlayer->clothingContained[inC].
            deleteElement( slotToRemove );
        inPlayer->clothingContainedEtaDecays[inC].
            deleteElement( slotToRemove );

        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;
        return true;
        }
    
    return false;
    }



static ObjectRecord **getClothingSlot( LiveObject *targetPlayer, int inIndex ) {
    
    ObjectRecord **clothingSlot = NULL;    


    if( inIndex == 2 &&
        targetPlayer->clothing.frontShoe != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.frontShoe );
        }
    else if( inIndex == 3 &&
             targetPlayer->clothing.backShoe 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.backShoe );
        }
    else if( inIndex == 0 && 
             targetPlayer->clothing.hat != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.hat );
        }
    else if( inIndex == 1 &&
             targetPlayer->clothing.tunic 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.tunic );
        }
    else if( inIndex == 4 &&
             targetPlayer->clothing.bottom 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.bottom );
        }
    else if( inIndex == 5 &&
             targetPlayer->
             clothing.backpack != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.backpack );
        }
    
    return clothingSlot;
    }

    

static void removeClothingToHold( LiveObject *nextPlayer, 
                                  LiveObject *targetPlayer,
                                  ObjectRecord **clothingSlot,
                                  int clothingSlotIndex ) {
    int ind = clothingSlotIndex;
    
    nextPlayer->holdingID =
        ( *clothingSlot )->id;
    holdingSomethingNew( nextPlayer );
    
    *clothingSlot = NULL;
    nextPlayer->holdingEtaDecay =
        targetPlayer->clothingEtaDecay[ind];
    targetPlayer->clothingEtaDecay[ind] = 0;
    
    nextPlayer->numContained =
        targetPlayer->
        clothingContained[ind].size();
    
    freePlayerContainedArrays( nextPlayer );
    
    nextPlayer->containedIDs =
        targetPlayer->
        clothingContained[ind].
        getElementArray();
    
    targetPlayer->clothingContained[ind].
        deleteAll();
    
    nextPlayer->containedEtaDecays =
        targetPlayer->
        clothingContainedEtaDecays[ind].
        getElementArray();
    
    targetPlayer->
        clothingContainedEtaDecays[ind].
        deleteAll();
    
    // empty sub contained in clothing
    nextPlayer->subContainedIDs =
        new SimpleVector<int>[
            nextPlayer->numContained ];
    
    nextPlayer->subContainedEtaDecays =
        new SimpleVector<timeSec_t>[
            nextPlayer->numContained ];
    
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    }



static TransRecord *getBareHandClothingTrans( LiveObject *nextPlayer,
                                              ObjectRecord **clothingSlot ) {
    TransRecord *bareHandClothingTrans = NULL;
    
    if( clothingSlot != NULL ) {
        bareHandClothingTrans =
            getPTrans( 0, ( *clothingSlot )->id );
                                    
        if( bareHandClothingTrans != NULL ) {
            int na =
                bareHandClothingTrans->newActor;
            
            if( na > 0 &&
                ! canPickup( na, computeAge( nextPlayer ) ) ) {
                // too young for trans
                bareHandClothingTrans = NULL;
                }
            
            if( bareHandClothingTrans != NULL ) {
                int nt = 
                    bareHandClothingTrans->
                    newTarget;
                
                if( nt > 0 &&
                    getObject( nt )->clothing 
                    == 'n' ) {
                    // don't allow transitions
                    // that leave a non-wearable
                    // item on your body
                    bareHandClothingTrans = NULL;
                    }
                }
            }
        }
    
    return bareHandClothingTrans;
    }




// change held as the result of a transition
static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID ) {
    
    LiveObject *nextPlayer = inPlayer;

    int oldHolding = nextPlayer->holdingID;
    
    int oldContained = 
        nextPlayer->numContained;
    
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    
    // can newly changed container hold
    // less than what it could contain
    // before?
    
    int newHeldSlots = getNumContainerSlots( inNewHeldID );
    
    if( newHeldSlots < oldContained ) {
        // new container can hold less
        // truncate
                            
        GridPos dropPos = 
            getPlayerPos( inPlayer );
                            
        // offset to counter-act offsets built into
        // drop code
        dropPos.x += 1;
        dropPos.y += 1;
        
        char found = false;
        GridPos spot;
        
        if( getMapObject( dropPos.x, dropPos.y ) == 0 ) {
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
            
            // throw it on map temporarily
            handleDrop( 
                spot.x, spot.y, 
                inPlayer,
                // only temporary, don't worry about blocking players
                // with this drop
                NULL );
                                

            // responsible player for stuff thrown on map by shrink
            setResponsiblePlayer( inPlayer->id );

            // shrink contianer on map
            shrinkContainer( spot.x, spot.y, 
                             newHeldSlots );
            
            setResponsiblePlayer( -1 );
            

            // pick it back up
            nextPlayer->holdingEtaDecay = 
                getEtaDecay( spot.x, spot.y );
                                    
            FullMapContained f =
                getFullMapContained( spot.x, spot.y );

            setContained( inPlayer, f );
            
            clearAllContained( spot.x, spot.y );
            setMapObject( spot.x, spot.y, 0 );
            }
        else {
            // no spot to throw it
            // cannot leverage map's container-shrinking
            // just truncate held container directly
            
            // truncated contained items will be lost
            inPlayer->numContained = newHeldSlots;
            }
        }

    nextPlayer->holdingID = inNewHeldID;
    holdingSomethingNew( inPlayer, oldHolding );

    if( newHeldSlots > 0 && 
        oldHolding != 0 ) {
                                        
        restretchDecays( 
            newHeldSlots,
            nextPlayer->containedEtaDecays,
            oldHolding,
            nextPlayer->holdingID );
        }
    
    
    if( oldHolding != inNewHeldID ) {
            
        char kept = false;

        // keep old decay timeer going...
        // if they both decay to the same thing in the same time
        if( oldHolding > 0 && inNewHeldID > 0 ) {
            
            TransRecord *oldDecayT = getMetaTrans( -1, oldHolding );
            TransRecord *newDecayT = getMetaTrans( -1, inNewHeldID );
            
            if( oldDecayT != NULL && newDecayT != NULL ) {
                if( oldDecayT->autoDecaySeconds == newDecayT->autoDecaySeconds
                    && 
                    oldDecayT->newTarget == newDecayT->newTarget ) {
                    
                    kept = true;
                    }
                }
            }
        if( !kept ) {
            setFreshEtaDecayForHeld( nextPlayer );
            }
        }

    }



static unsigned char *makeCompressedMessage( char *inMessage, int inLength,
                                             int *outLength ) {
    
    int compressedSize;
    unsigned char *compressedData =
        zipCompress( (unsigned char*)inMessage, inLength, &compressedSize );



    char *header = autoSprintf( "CM\n%d %d\n#", 
                                inLength,
                                compressedSize );
    int headerLength = strlen( header );
    int fullLength = headerLength + compressedSize;
    
    unsigned char *fullMessage = new unsigned char[ fullLength ];
    
    memcpy( fullMessage, (unsigned char*)header, headerLength );
    
    memcpy( &( fullMessage[ headerLength ] ), compressedData, compressedSize );

    delete [] compressedData;
    
    *outLength = fullLength;
    
    delete [] header;
    
    return fullMessage;
    }



static int maxUncompressedSize = 256;


void sendMessageToPlayer( LiveObject *inPlayer, 
                          char *inMessage, int inLength ) {
    if( ! inPlayer->connected ) {
        // stop sending messages to disconnected players
        return;
        }
    
    
    unsigned char *message = (unsigned char*)inMessage;
    int len = inLength;
    
    char deleteMessage = false;

    if( inLength > maxUncompressedSize ) {
        message = makeCompressedMessage( inMessage, inLength, &len );
        deleteMessage = true;
        }

    int numSent = 
        inPlayer->sock->send( message, 
                              len, 
                              false, false );
        
    if( numSent != len ) {
        setPlayerDisconnected( inPlayer, "Socket write failed" );
        }

    inPlayer->gotPartOfThisFrame = true;
    
    if( deleteMessage ) {
        delete [] message;
        }
    }



// result destroyed by caller
static char *getWarReportMessage() {
    SimpleVector<char> workingMessage;
    
    SimpleVector<int> lineageEveIDs;
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->error ) {
            continue;
            }
        
        if( lineageEveIDs.getElementIndex( o->lineageEveID ) == -1 ) {
            lineageEveIDs.push_back( o->lineageEveID );
            }
        }

    workingMessage.appendElementString( "WR\n" );

    // check each unique pair of families
    for( int a=0; a<lineageEveIDs.size(); a++ ) {
        int linA = lineageEveIDs.getElementDirect( a );
        for( int b=a+1; b<lineageEveIDs.size(); b++ ) {
            int linB = lineageEveIDs.getElementDirect( b );
            
            char *line = NULL;
            if( isWarState( linA, linB ) ) {
                line = autoSprintf( "%d %d war\n", linA, linB );
                }
            else if( isPeaceTreaty( linA, linB ) ) {
                line = autoSprintf( "%d %d peace\n", linA, linB );
                }
            // no line if neutral
            if( line != NULL ) {
                workingMessage.appendElementString( line );
                delete [] line;
                }
            }
        }

    workingMessage.appendElementString( "#" );

    return workingMessage.getElementString();
    }



void sendWarReportToAll() {
    char *w = getWarReportMessage();
    int len = strlen( w );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error && o->connected ) {
            sendMessageToPlayer( o, w, len );
            }
        }
    delete [] w;
    }



static void sendWarReportToOne( LiveObject *inO ) {
    char *w = getWarReportMessage();
    int len = strlen( w );
    sendMessageToPlayer( inO, w, len );
    delete [] w;
    }
    


void readPhrases( const char *inSettingsName, 
                  SimpleVector<char*> *inList ) {
    char *cont = SettingsManager::getSettingContents( inSettingsName, "" );
    
    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        return;    
        }
    
    int numParts;
    char **parts = split( cont, "\n", &numParts );
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            inList->push_back( stringToUpperCase( parts[i] ) );
            }
        delete [] parts[i];
        }
    delete [] parts;
    }



// returns pointer to name in string
char *isNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList ) {
    char *saidString = inSaidString;
    
    if( saidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return NULL;
        }
    
    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        if( strstr( inSaidString, testString ) == saidString ) {
            // hit
            int phraseLen = strlen( testString );
            // skip spaces after
            while( saidString[ phraseLen ] == ' ' ) {
                phraseLen++;
                }
            return &( saidString[ phraseLen ] );
            }
        }
    return NULL;
    }


// returns newly allocated name, or NULL
// looks for phrases that start with a name
char *isReverseNamingSay( char *inSaidString, 
                          SimpleVector<char*> *inPhraseList ) {
    
    if( inSaidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return NULL;
        }

    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        char *hitLoc = strstr( inSaidString, testString );

        if( hitLoc != NULL ) {

            char *saidDupe = stringDuplicate( inSaidString );

            hitLoc = strstr( saidDupe, testString );

            // back one, to exclude space from name
            if( hitLoc != saidDupe ) {
                hitLoc[-1] = '\0';
                return saidDupe;
                }
            else {
                delete [] saidDupe;
                return NULL;
                }
            }
        }
    return NULL;
    }



char *isBabyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &nameGivingPhrases );
    }

char *isFamilyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &familyNameGivingPhrases );
    }

char *isEveNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &eveNameGivingPhrases );
    }

char *isCurseNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &cursingPhrases );
    }

char *isNamedGivingSay( char *inSaidString ) {
    return isReverseNamingSay( inSaidString, &namedGivingPhrases );
    }



static char isWildcardGivingSay( char *inSaidString,
                                 SimpleVector<char*> *inPhrases ) {
    if( inSaidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return false;
        }

    for( int i=0; i<inPhrases->size(); i++ ) {
        char *testString = inPhrases->getElementDirect( i );
        
        if( strcmp( inSaidString, testString ) == 0 ) {
            return true;
            }
        }
    return false;
    }



char isYouGivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youGivingPhrases );
    }

char isFamilyGivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &familyGivingPhrases );
    }

char isOffspringGivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &offspringGivingPhrases );
    }

char isPosseJoiningSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &posseJoiningPhrases );
    }


char isYouFollowSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youFollowPhrases );
    }

// returns pointer into inSaidString
char *isNamedFollowSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &namedFollowPhrases );
    }


char isYouExileSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youExilePhrases );
    }

// returns pointer into inSaidString
char *isNamedExileSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &namedExilePhrases );
    }


char isYouRedeemSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youRedeemPhrases );
    }

// returns pointer into inSaidString
char *isNamedRedeemSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &namedRedeemPhrases );
    }



char isYouKillSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youKillPhrases );
    }


// returns newly allocated string
char *isNamedKillSay( char *inSaidString ) {

    char *name = isReverseNamingSay( inSaidString, &namedAfterKillPhrases );

    if( name != NULL ) {
        return name;
        }
    
    name = isNamingSay( inSaidString, &namedKillPhrases );
    
    if( name != NULL ) {
        return stringDuplicate( name );
        }
    
    return NULL;
    }


char isYouForgivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youForgivingPhrases );
    }

// returns pointer into inSaidString
char *isNamedForgivingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &forgivingPhrases );
    }




LiveObject *getClosestOtherPlayer( LiveObject *inThisPlayer,
                                   double inMinAge = 0,
                                   char inNameMustBeNULL = false ) {
    GridPos thisPos = getPlayerPos( inThisPlayer );

    // don't consider anyone who is too far away
    double closestDist = 20;
    LiveObject *closestOther = NULL;
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = 
            players.getElement(j);
        
        if( otherPlayer != inThisPlayer &&
            ! otherPlayer->error &&
            computeAge( otherPlayer ) >= inMinAge &&
            ( ! inNameMustBeNULL || otherPlayer->name == NULL ) ) {
                                        
            GridPos otherPos = 
                getPlayerPos( otherPlayer );
            
            double dist =
                distance( thisPos, otherPos );
            
            if( dist < closestDist ) {
                closestDist = dist;
                closestOther = otherPlayer;
                }
            }
        }
    return closestOther;
    }



int readIntFromFile( const char *inFileName, int inDefaultValue ) {
    FILE *f = fopen( inFileName, "r" );
    
    if( f == NULL ) {
        return inDefaultValue;
        }
    
    int val = inDefaultValue;
    
    fscanf( f, "%d", &val );

    fclose( f );

    return val;
    }




typedef struct KillState {
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


SimpleVector<KillState> activeKillStates;




void apocalypseStep() {
    
    double curTime = Time::getCurrentTime();

    if( !apocalypseTriggered ) {
        
        if( apocalypseRequest == NULL &&
            curTime - lastRemoteApocalypseCheckTime > 
            remoteApocalypseCheckInterval ) {

            lastRemoteApocalypseCheckTime = curTime;

            // don't actually send request to reflector if apocalypse
            // not possible locally
            // or if broadcast mode disabled
            if( SettingsManager::getIntSetting( "remoteReport", 0 ) &&
                SettingsManager::getIntSetting( "apocalypsePossible", 0 ) &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) ) {

                printf( "Checking for remote apocalypse\n" );
            
                char *url = autoSprintf( "%s?action=check_apocalypse", 
                                         reflectorURL );
        
                apocalypseRequest =
                    new WebRequest( "GET", url, NULL );
            
                delete [] url;
                }
            }
        else if( apocalypseRequest != NULL ) {
            int result = apocalypseRequest->step();

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse check:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse check:  Bad response from reflector:  %s.",
                        webResult );
                    }
                else {
                    int newApocalypseNumber = lastApocalypseNumber;
                    
                    sscanf( webResult, "%d\n", &newApocalypseNumber );
                
                    if( newApocalypseNumber > lastApocalypseNumber ) {
                        lastApocalypseNumber = newApocalypseNumber;
                        apocalypseTriggered = true;
                        apocalypseRemote = true;
                        AppLog::infoF( 
                            "Apocalypse check:  New remote apocalypse:  %d.",
                            lastApocalypseNumber );
                        SettingsManager::setSetting( "lastApocalypseNumber",
                                                     lastApocalypseNumber );
                        }
                    }
                    
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }
        }
        


    if( apocalypseTriggered ) {

        if( !apocalypseStarted ) {
            apocalypsePossible = 
                SettingsManager::getIntSetting( "apocalypsePossible", 0 );

            if( !apocalypsePossible ) {
                // settings change since we last looked at it
                apocalypseTriggered = false;
                return;
                }
            
            AppLog::info( "Apocalypse triggerered, starting it" );

            // restart Eve window, and let this player be the
            // first new Eve
            eveWindowStart = 0;
    
            // reset other apocalypse trigger
            lastBabyPassedThresholdTime = 0;
            
            // repopulate this list later when next Eve window ends
            familyNamesAfterEveWindow.deallocateStringElements();
            familyLineageEveIDsAfterEveWindow.deleteAll();
            familyCountsAfterEveWindow.deleteAll();
            nextBabyFamilyIndex = 0;
            
            if( postWindowFamilyLogFile != NULL ) {
                fclose( postWindowFamilyLogFile );
                postWindowFamilyLogFile = NULL;
                }


            reportArcEnd();
            

            // only broadcast to reflector if apocalypseBroadcast set
            if( !apocalypseRemote &&
                SettingsManager::getIntSetting( "remoteReport", 0 ) &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) &&
                apocalypseRequest == NULL && reflectorURL != NULL ) {
                
                AppLog::info( "Apocalypse broadcast set, telling reflector" );

                
                char *reflectorSharedSecret = 
                    SettingsManager::
                    getStringSetting( "reflectorSharedSecret" );
                
                if( reflectorSharedSecret != NULL ) {
                    lastApocalypseNumber++;

                    AppLog::infoF( 
                        "Apocalypse trigger:  New local apocalypse:  %d.",
                        lastApocalypseNumber );

                    SettingsManager::setSetting( "lastApocalypseNumber",
                                                 lastApocalypseNumber );

                    int closestPlayerIndex = -1;
                    double closestDist = 999999999;
                    
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error ) {
                            
                            double dist = 
                                abs( nextPlayer->xd - apocalypseLocation.x ) +
                                abs( nextPlayer->yd - apocalypseLocation.y );
                            if( dist < closestDist ) {
                                closestPlayerIndex = i;
                                closestDist = dist;
                                }
                            }
                        
                        }
                    char *name = NULL;
                    if( closestPlayerIndex != -1 ) {
                        name = 
                            players.getElement( closestPlayerIndex )->
                            name;
                        }
                    
                    if( name == NULL ) {
                        name = stringDuplicate( "UNKNOWN" );
                        }
                    
                    char *idString = autoSprintf( "%d", lastApocalypseNumber );
                    
                    char *hash = hmac_sha1( reflectorSharedSecret, idString );

                    delete [] idString;

                    char *url = autoSprintf( 
                        "%s?action=trigger_apocalypse"
                        "&id=%d&id_hash=%s&name=%s",
                        reflectorURL, lastApocalypseNumber, hash, name );

                    delete [] hash;
                    delete [] name;
                    
                    printf( "Starting new web request for %s\n", url );
                    
                    apocalypseRequest =
                        new WebRequest( "GET", url, NULL );
                                
                    delete [] url;
                    delete [] reflectorSharedSecret;
                    }
                }


            // send all players the AP message
            const char *message = "AP\n#";
            int messageLength = strlen( message );
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                if( !nextPlayer->error && nextPlayer->connected ) {
                    
                    int numSent = 
                        nextPlayer->sock->send( 
                            (unsigned char*)message, 
                            messageLength,
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != messageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }
                }
            
            apocalypseStartTime = Time::getCurrentTime();
            apocalypseStarted = true;
            postApocalypseStarted = false;
            }

        if( apocalypseRequest != NULL ) {
            
            int result = apocalypseRequest->step();
                

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse trigger:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                printf( "Apocalypse trigger:  "
                        "Got web result:  '%s'\n", webResult );
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse trigger:  "
                        "Bad response from reflector:  %s.",
                        webResult );
                    }
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }

        if( apocalypseRequest == NULL &&
            Time::getCurrentTime() - apocalypseStartTime >= 8 ) {
            
            if( ! postApocalypseStarted  ) {
                AppLog::infoF( "Enough warning time, %d players still alive",
                               players.size() );
                
                
                double startTime = Time::getCurrentTime();
                
                if( familyDataLogFile != NULL ) {
                    fprintf( familyDataLogFile, "%.2f apocalypse triggered\n",
                             startTime );
                    }
    

                // clear map
                freeMap( true );

                AppLog::infoF( "Apocalypse freeMap took %f sec",
                               Time::getCurrentTime() - startTime );
                wipeMapFiles();

                AppLog::infoF( "Apocalypse wipeMapFiles took %f sec",
                               Time::getCurrentTime() - startTime );
                
                initMap();

                reseedMap( true );
                
                AppLog::infoF( "Apocalypse initMap took %f sec",
                               Time::getCurrentTime() - startTime );
                
                clearTapoutCounts();

                peaceTreaties.deleteAll();
                warStates.deleteAll();
                warPeaceRecords.deleteAll();
                activeKillStates.deleteAll();

                lastRemoteApocalypseCheckTime = curTime;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    backToBasics( nextPlayer );
                    }
                
                // send everyone update about everyone
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    if( nextPlayer->connected ) {    
                        nextPlayer->firstMessageSent = false;
                        nextPlayer->firstMapSent = false;
                        nextPlayer->inFlight = false;
                        }
                    // clear monument pos post-apoc
                    // so we don't keep passing the stale info on to
                    // our offspring
                    nextPlayer->monumentPosSet = false;
                    }

                postApocalypseStarted = true;
                }
            else {
                // make sure all players have gotten map and update
                char allMapAndUpdate = true;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    if( nextPlayer->connected && ! nextPlayer->firstMapSent ) {
                        allMapAndUpdate = false;
                        break;
                        }
                    }
                
                if( allMapAndUpdate ) {
                    
                    // send all players the AD message
                    const char *message = "AD\n#";
                    int messageLength = strlen( message );
            
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error && nextPlayer->connected ) {
                    
                            int numSent = 
                                nextPlayer->sock->send( 
                                    (unsigned char*)message, 
                                    messageLength,
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                    
                            if( numSent != messageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }

                    // totally done
                    apocalypseStarted = false;
                    apocalypseTriggered = false;
                    apocalypseRemote = false;
                    postApocalypseStarted = false;
                    }
                }    
            }
        }
    }




void monumentStep() {
    if( monumentCallPending ) {
        
        // send to all players
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            // remember it to tell babies about it
            nextPlayer->monumentPosSet = true;
            nextPlayer->lastMonumentPos.x = monumentCallX;
            nextPlayer->lastMonumentPos.y = monumentCallY;
            nextPlayer->lastMonumentID = monumentCallID;
            nextPlayer->monumentPosSent = true;
            
            if( !nextPlayer->error && nextPlayer->connected ) {
                
                char *message = autoSprintf( "MN\n%d %d %d\n#", 
                                             monumentCallX -
                                             nextPlayer->birthPos.x, 
                                             monumentCallY -
                                             nextPlayer->birthPos.y,
                                             hideIDForClient( 
                                                 monumentCallID ) );
                int messageLength = strlen( message );


                int numSent = 
                    nextPlayer->sock->send( 
                        (unsigned char*)message, 
                        messageLength,
                        false, false );
                
                nextPlayer->gotPartOfThisFrame = true;
                
                delete [] message;

                if( numSent != messageLength ) {
                    setPlayerDisconnected( nextPlayer, "Socket write failed" );
                    }
                }
            }

        monumentCallPending = false;
        }
    }




// inPlayerName may be destroyed inside this function
// returns a uniquified name, sometimes newly allocated.
// return value destroyed by caller
char *getUniqueCursableName( char *inPlayerName, char *outSuffixAdded,
                             char inIsEve, char inFemale ) {
    
    char dup = isNameDuplicateForCurses( inPlayerName );
    
    if( ! dup ) {
        *outSuffixAdded = false;

        if( inIsEve ) {
            // make sure Eve doesn't have same last name as any living person
            char firstName[99];
            char lastName[99];
            
            sscanf( inPlayerName, "%s %s", firstName, lastName );
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *o = players.getElement( i );
                
                if( ! o->error && o->familyName != NULL &&
                    strcmp( o->familyName, lastName ) == 0 ) {
                    
                    dup = true;
                    break;
                    }
                }
            }
        

        return inPlayerName;
        }    
    
    
    if( false ) {
        // old code, add suffix to make unique

        *outSuffixAdded = true;

        int targetPersonNumber = 1;
        
        char *fullName = stringDuplicate( inPlayerName );

        while( dup ) {
            // try next roman numeral
            targetPersonNumber++;
            
            int personNumber = targetPersonNumber;            
        
            SimpleVector<char> romanNumeralList;
        
            while( personNumber >= 100 ) {
                romanNumeralList.push_back( 'C' );
                personNumber -= 100;
                }
            while( personNumber >= 50 ) {
                romanNumeralList.push_back( 'L' );
                personNumber -= 50;
                }
            while( personNumber >= 40 ) {
                romanNumeralList.push_back( 'X' );
                romanNumeralList.push_back( 'L' );
                personNumber -= 40;
                }
            while( personNumber >= 10 ) {
                romanNumeralList.push_back( 'X' );
                personNumber -= 10;
                }
            while( personNumber >= 9 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'X' );
                personNumber -= 9;
                }
            while( personNumber >= 5 ) {
                romanNumeralList.push_back( 'V' );
                personNumber -= 5;
                }
            while( personNumber >= 4 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'V' );
                personNumber -= 4;
                }
            while( personNumber >= 1 ) {
                romanNumeralList.push_back( 'I' );
                personNumber -= 1;
                }
            
            char *romanString = romanNumeralList.getElementString();

            delete [] fullName;
            
            fullName = autoSprintf( "%s %s", inPlayerName, romanString );
            delete [] romanString;
            
            dup = isNameDuplicateForCurses( fullName );
            }
        
        delete [] inPlayerName;
        
        return fullName;
        }
    else {
        // new code:
        // make name unique by finding close matching name that hasn't been
        // used recently
        
        *outSuffixAdded = false;

        char firstName[99];
        char lastName[99];
        
        int numNames = sscanf( inPlayerName, "%s %s", firstName, lastName );
        
        if( numNames == 1 ) {
            // special case, find a totally unique first name for them
            
            int i = getFirstNameIndex( firstName, inFemale );

            while( dup ) {

                int nextI;
                
                dup = isNameDuplicateForCurses( getFirstName( i, &nextI, 
                                                              inFemale ) );
            
                if( dup ) {
                    i = nextI;
                    }
                }
            
            if( dup ) {
                // ran out of names, yikes
                return inPlayerName;
                }
            else {
                delete [] inPlayerName;
                int nextI;
                return stringDuplicate( getFirstName( i, &nextI, inFemale ) );
                }
            }
        else if( numNames == 2 ) {
            if( inIsEve ) {
                // cycle last names until we find one not used by any
                // family
                
                int i = getLastNameIndex( lastName );
            
                const char *tempLastName = "";
                
                while( dup ) {
                    
                    int nextI;
                    tempLastName = getLastName( i, &nextI );
                    
                    dup = false;

                    for( int j=0; j<players.size(); j++ ) {
                        LiveObject *o = players.getElement( j );
                        
                        if( ! o->error && 
                            o->familyName != NULL &&
                            strcmp( o->familyName, tempLastName ) == 0 ) {
                    
                            dup = true;
                            break;
                            }
                        }
                    
                    if( dup ) {
                        i = nextI;
                        }
                    }
            
                if( dup ) {
                    // ran out of names, yikes
                    return inPlayerName;
                    }
                else {
                    delete [] inPlayerName;
                    return autoSprintf( "%s %s", firstName, tempLastName );
                    }
                }
            else {
                // cycle first names until we find one
                int i = getFirstNameIndex( firstName, inFemale );
            
                char *tempName = NULL;
                
                while( dup ) {                    
                    if( tempName != NULL ) {
                        delete [] tempName;
                        }
                    
                    int nextI;
                    tempName = autoSprintf( "%s %s", getFirstName( i, &nextI,
                                                                   inFemale ),
                                            lastName );
                    

                    dup = isNameDuplicateForCurses( tempName );
                    if( dup ) {
                        i = nextI;
                        }
                    }
            
                if( dup ) {
                    // ran out of names, yikes
                    if( tempName != NULL ) {
                        delete [] tempName;
                        }
                    return inPlayerName;
                    }
                else {
                    delete [] inPlayerName;
                    return tempName;
                    }
                }
            }
        else {
            // weird case, name doesn't even have two string parts, give up
            return inPlayerName;
            }
        }
    
    }




typedef struct ForcedEffects {
        // -1 if no emot specified
        int emotIndex;
        int ttlSec;
        
        char foodModifierSet;
        double foodCapModifier;
        
        char feverSet;
        float fever;
    } ForcedEffects;
        


ForcedEffects checkForForcedEffects( int inHeldObjectID ) {
    ForcedEffects e = { -1, 0, false, 1.0, false, 0.0f };
    
    ObjectRecord *o = getObject( inHeldObjectID );
    
    if( o != NULL ) {
        char *emotPos = strstr( o->description, "emot_" );
        
        if( emotPos != NULL ) {
            sscanf( emotPos, "emot_%d_%d", 
                    &( e.emotIndex ), &( e.ttlSec ) );
            }

        char *foodPos = strstr( o->description, "food_" );
        
        if( foodPos != NULL ) {
            int numRead = sscanf( foodPos, "food_%lf", 
                                  &( e.foodCapModifier ) );
            if( numRead == 1 ) {
                e.foodModifierSet = true;
                }
            }

        char *feverPos = strstr( o->description, "fever_" );
        
        if( feverPos != NULL ) {
            int numRead = sscanf( feverPos, "fever_%f", 
                                  &( e.fever ) );
            if( numRead == 1 ) {
                e.feverSet = true;
                }
            }
        }
    
    
    return e;
    }




void setNoLongerDying( LiveObject *inPlayer, 
                       SimpleVector<int> *inPlayerIndicesToSendHealingAbout ) {
    inPlayer->dying = false;
    inPlayer->murderSourceID = 0;
    inPlayer->murderPerpID = 0;
    if( inPlayer->murderPerpEmail != 
        NULL ) {
        delete [] 
            inPlayer->murderPerpEmail;
        inPlayer->murderPerpEmail =
            NULL;
        }
    
    inPlayer->deathSourceID = 0;
    inPlayer->holdingWound = false;
    inPlayer->customGraveID = -1;
    
    inPlayer->emotFrozen = false;
    inPlayer->emotUnfreezeETA = 0;
    
    inPlayer->foodCapModifier = 1.0;
    inPlayer->foodUpdate = true;

    inPlayer->fever = 0;

    if( inPlayer->deathReason 
        != NULL ) {
        delete [] inPlayer->deathReason;
        inPlayer->deathReason = NULL;
        }
                                        
    inPlayerIndicesToSendHealingAbout->
        push_back( 
            getLiveObjectIndex( 
                inPlayer->id ) );
    }



static void checkSickStaggerTime( LiveObject *inPlayer ) {
    ObjectRecord *heldObj = NULL;
    
    if( inPlayer->holdingID > 0 ) {
        heldObj = getObject( inPlayer->holdingID );
        }
    else {
        return;
        }

    
    char isSick = false;
    
    if( strstr(
            heldObj->
            description,
            "sick" ) != NULL ) {
        isSick = true;
        
        // sicknesses override basic death-stagger
        // time.  The person can live forever
        // if they are taken care of until
        // the sickness passes
        
        int staggerTime = 
            SettingsManager::getIntSetting(
                "deathStaggerTime", 20 );
        
        double currentTime = 
            Time::getCurrentTime();
        
        // 10x base stagger time should
        // give them enough time to either heal
        // from the disease or die from its
        // side-effects
        inPlayer->dyingETA = 
            currentTime + 10 * staggerTime;
        }
    
    if( isSick ) {
        // what they have will heal on its own 
        // with time.  Sickness, not wound.
        
        // death source is sickness, not
        // source
        inPlayer->deathSourceID = 
            inPlayer->holdingID;
        
        setDeathReason( inPlayer, 
                        "succumbed",
                        inPlayer->holdingID );
        }
    }



typedef struct FlightDest {
        int playerID;
        GridPos destPos;
    } FlightDest;
        



SimpleVector<int> killStatePosseChangedPlayerIDs;


static int countPosseSize( LiveObject *inTarget, 
                           int *outMinPosseSizeForKill = NULL,
                           char *outFullForceSoloPosse = NULL ) {
    int p = 0;
    
    int uncounted = 0;

    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        if( s->targetID == inTarget->id ) {

            LiveObject *killerO = getLiveObject( s->killerID );
            
            if( killerO != NULL ) {
                
                // twins don't count toward posse size
                // people who lived short life last life don't count either
                // they may be die-cycling to find their IRL friends and
                // gang up
                if( ! killerO->isTwin && ! killerO->isLastLifeShort ) {
                    p++;

                    if( p == 1 ) {
                        // set these based on state for posse leader only
                        if( outMinPosseSizeForKill != NULL ) {
                            *outMinPosseSizeForKill = s->minPosseSizeForKill;
                            }
                        if( outFullForceSoloPosse != NULL ) {
                            *outFullForceSoloPosse = s->fullForceSoloPosse;
                            }
                        }
                    }
                else {
                    uncounted ++;
                    }
                }
            }
        }
    
    if( p == 0 &&
        uncounted > 0 ) {
        // if twin (or other uncounted person) is only one in posse, 
        // count as a posse of 1
        p = 1;
        }

    return p;
    }



static void updatePosseSize( LiveObject *inTarget, 
                             LiveObject *inRemovedKiller = NULL ) {
    
    int p = countPosseSize( inTarget );
    
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        
        if( s->targetID == inTarget->id ) {
            int killerID = s->killerID;

            s->posseSize = p;
            
            LiveObject *o = getLiveObject( killerID );
            
            if( o != NULL ) {
                int oldSize = o->killPosseSize;
                o->killPosseSize = p;
                
                if( oldSize != p ) {
                    killStatePosseChangedPlayerIDs.push_back( killerID );
                    }
                }
            }
        }

    if( inRemovedKiller != NULL ) {
        int oldSize = inRemovedKiller->killPosseSize;
        
        inRemovedKiller->killPosseSize = 0;
        if( oldSize != 0 ) {
            killStatePosseChangedPlayerIDs.push_back( 
                inRemovedKiller->id );
            }
        }
    }





// inEatenID = 0 for nursing
static void checkForFoodEatingEmot( LiveObject *inPlayer,
                                    int inEatenID ) {
    
    char wasStarving = inPlayer->starving;
    inPlayer->starving = false;

    
    if( inEatenID > 0 ) {
        
        ObjectRecord *o = getObject( inEatenID );
        
        if( o != NULL ) {
            char *emotPos = strstr( o->description, "emotEat_" );
            
            if( emotPos != NULL ) {
                int e, t;
                int numRead = sscanf( emotPos, "emotEat_%d_%d", &e, &t );
                
                if( numRead == 2 ) {
                    inPlayer->emotFrozen = true;
                    inPlayer->emotFrozenIndex = e;
                    
                    inPlayer->emotUnfreezeETA = Time::getCurrentTime() + t;
                    
                    newEmotPlayerIDs.push_back( inPlayer->id );
                    newEmotIndices.push_back( e );
                    newEmotTTLs.push_back( t );
                    return;
                    }
                }
            }
        }

    // no food emot found
    if( wasStarving ) {
        // clear their starving emot
        newEmotPlayerIDs.push_back( inPlayer->id );
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }
                
    }



static char isNoWaitWeapon( int inObjectID ) {
    return strstr( getObject( inObjectID )->description,
                   "+noWait" ) != NULL;
    }



char *getLeadershipName( LiveObject *nextPlayer, 
                         char inNoName = false );
    


// return true if it worked
char addKillState( LiveObject *inKiller, LiveObject *inTarget,
                   char inInfiniteRange = false ) {
    char found = false;
    
    GridPos killerPos = getPlayerPos( inKiller );
    GridPos targetPos = getPlayerPos( inTarget );
    
    if( ! inInfiniteRange && 
        distance( killerPos, targetPos )
        > 8 ) {
        // out of range
        return false;
        }
    
    

    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        
        if( s->killerID == inKiller->id ) {
            found = true;
            s->killerWeaponID = inKiller->holdingID;
            s->targetID = inTarget->id;

            double curTime = Time::getCurrentTime();
            s->emotStartTime = curTime;
            s->killStartTime = curTime;
            
            if( isNoWaitWeapon( inKiller->holdingID ) ) {
                // allow it to happen right now
                s->killStartTime -= killDelayTime;
                }

            s->emotRefreshSeconds = 30;
            break;
            }
        }
    
    if( !found ) {
        // add new


        // new, based on allies and enemies
        // allies protect, enemies reduce protection
        
        int allyCount = countAllies( inTarget, targetPos, 
                                     possePopulationRadius );
        int enemyCount = countEnemies( inTarget, targetPos, 
                                       possePopulationRadius );        
        
        if( ! isExiled( inKiller, inTarget ) ) {
            // killer doesn't currently see target as exiled
            // count them as a temporary enemy, because they are trying
            // to kill target
            enemyCount += 1;
            }
        

        int minPosseSizeForKill = 1;
        char fullForceSoloPosse = false;

        if( allyCount >= enemyCount ) {
            // use standard posse size calculation based on settings
            // and population size
            
            // before age 9, they can't say "I JOIN YOU"
            double possePossibleAge = 9;
        

            // count population around victim, not around killer
            int regionalPop = countNonHelpless( targetPos, 
                                                possePopulationRadius,
                                                possePossibleAge );
            
            minPosseSizeForKill = ceil( minPosseFraction * regionalPop );
            }
        else {
            // more enemies than allies
            // solo killing becomes possible
            minPosseSizeForKill = 1;
            
            if( enemyCount > 1 ) {
                // target isn't being solo targeted in wilderness
                // enable force-dropping of what they are holding
                fullForceSoloPosse = true;
                }
            }


        char recentKiller = false;
        
        if( ! fullForceSoloPosse &&
            Time::getCurrentTime() - inTarget->lastKillTime < 
            killerVulnerableSeconds ) {
            
            // trying to kill a recent killer
            recentKiller = true;            
            minPosseSizeForKill = 1;
            }

        if( minPosseSizeForKill > minPosseCap ) {
            minPosseSizeForKill = minPosseCap;
            }

        char noWaitWeapon = false;
        
        if( isNoWaitWeapon( inKiller->holdingID ) ) {
            // no posse required for non-deadly weapons (snowballs, tattoos)
            minPosseSizeForKill = 1;
            noWaitWeapon = true;
            }
        
        int thisMinPosseSizeForKill = minPosseSizeForKill;
        

        char joiningExisting = false;
        
        // dupe existing min posse size
        // if other player started posse
        for( int i=0; i<activeKillStates.size(); i++ ) {
            KillState *s = activeKillStates.getElement( i );
            
            if( s->targetID == inTarget->id ) {
                
                // don't copy small posse size from no-wait weapons
                if( ! isNoWaitWeapon( s->killerWeaponID ) ) {
                    
                    minPosseSizeForKill = s->minPosseSizeForKill;
                    }
                else if( minPosseSizeForKill > 1 ) {
                    // earlier kill state is no-wait weapon
                    // override it's small posse size with this new size
                    s->minPosseSizeForKill = minPosseSizeForKill;
                    }
                
                joiningExisting = true;
                break;
                }
            }

        
        if( joiningExisting && ! noWaitWeapon &&
            thisMinPosseSizeForKill < minPosseSizeForKill &&
            thisMinPosseSizeForKill <= 1 ) {
            
            // something has changed since posse was formed
            // maybe target has been exiled?

            // update existing posse records to reflect this.

            minPosseSizeForKill = thisMinPosseSizeForKill;
            
            for( int i=0; i<activeKillStates.size(); i++ ) {
                KillState *s = activeKillStates.getElement( i );
                
                if( s->targetID == inTarget->id ) {
                    s->minPosseSizeForKill = minPosseSizeForKill;
                    }
                }
            }


        if( ! joiningExisting && minPosseSizeForKill > 1 ) {
            // this is the founder of the posse
            
            // let them know what the requirements are

            const char *allyWord = "ALLIES";
            if( allyCount == 1 ) {
                allyWord = "ALLY";
                }
            const char *enemyWord = "ENEMIES";
            if( enemyCount == 1 ) {
                enemyWord = "ENEMY";
                }
            
            char *message = 
                autoSprintf( 
                    "TARGET HAS %d %s, %d %s, NEED POSSE OF %d TO KILL.**"
                    "OTHERS JOIN BY HOLDING OBJECTS AND SAYING:  I JOIN YOU", 
                    allyCount, allyWord, enemyCount, enemyWord, 
                    minPosseSizeForKill );            
            
        
            sendGlobalMessage( message, inKiller );
            delete [] message;


            LiveObject *topLeaderO = 
                getLiveObject( getTopLeader( inTarget ) );

            const char *pronoun = "HIM";
            if( getFemale( inTarget ) ) {
                pronoun = "HER";
                }
            
            if( topLeaderO == inKiller ) {
                // killer has power to exile them directly
    
                char *psMessage = 
                    autoSprintf( "PS\n"
                                 "%d/0 I CAN EXILE %s MYSELF\n#",
                                 inKiller->id, pronoun );
                
                sendMessageToPlayer( inKiller, 
                                     psMessage, strlen( psMessage ) );
                delete [] psMessage;
                }
            else if( topLeaderO == inTarget ) {
                // we're targeting a leader
                
                pronoun = "HE";
                if( getFemale( inTarget ) ) {
                    pronoun = "SHE";
                    }
                
                char *psMessage = 
                    autoSprintf( "PS\n"
                                 "%d/0 %s IS A TOP LEADER\n#",
                                 inKiller->id, pronoun );
                
                sendMessageToPlayer( inKiller, 
                                     psMessage, strlen( psMessage ) );
                delete [] psMessage;
                }
            else if( topLeaderO != NULL ) {

                char *topLeaderName = getLeadershipName( topLeaderO );
                
                GridPos lPos = getPlayerPos( topLeaderO );
                
                char *psMessage = 
                    autoSprintf( "PS\n"
                                 "%d/0 %s CAN EXILE %s "
                                 "*leader %d *map %d %d\n#",
                                 inKiller->id,
                                 topLeaderName,
                                 pronoun,
                                 topLeaderO->id,
                                 lPos.x - inKiller->birthPos.x,
                                 lPos.y - inKiller->birthPos.y );
                
                delete [] topLeaderName;
            
                sendMessageToPlayer( inKiller, 
                                     psMessage, strlen( psMessage ) );
                delete [] psMessage;
                }
            }
        else if( ! joiningExisting && minPosseSizeForKill <= 1 &&
                 recentKiller ) {
            // solo killing okay, because it's a recent killer
            const char *pronoun = "HE";
            if( getFemale( inTarget ) ) {
                pronoun = "SHE";
                }
            char *message = 
                autoSprintf( 
                    "TARGET HAS KILLED RECENTLY, SO %s CAN BE KILLED SOLO.", 
                    pronoun );

            sendGlobalMessage( message, inKiller );
            delete [] message;
            }
        else if( ! joiningExisting && minPosseSizeForKill <= 1 ) {
            // solo killing okay!
            
            // let them know about it

            const char *allyWord = "ALLIES";
            if( allyCount == 1 ) {
                allyWord = "ALLY";
                }
            const char *enemyWord = "ENEMIES";
            if( enemyCount == 1 ) {
                enemyWord = "ENEMY";
                }
            
            const char *pronoun = "HE";
            if( getFemale( inTarget ) ) {
                pronoun = "SHE";
                }

            char *message = 
                autoSprintf( 
                    "TARGET HAS %d %s, %d %s, SO %s CAN BE KILLED SOLO.", 
                    allyCount, allyWord, enemyCount, enemyWord,
                    pronoun );            
            
        
            sendGlobalMessage( message, inKiller );
            delete [] message;
            }
        

        
        double curTime = Time::getCurrentTime();
        KillState s = { inKiller->id, 
                        inKiller->holdingID,
                        inTarget->id, 
                        curTime,
                        curTime,
                        30,
                        1,
                        minPosseSizeForKill,
                        fullForceSoloPosse };
        
        if( ! joiningExisting && isNoWaitWeapon( inKiller->holdingID ) ) {
                // allow it to happen right now
            s.killStartTime -= killDelayTime;
            }

        activeKillStates.push_back( s );

        // force target to gasp
        makePlayerSay( inTarget, (char*)"[GASP]" );
        }

    if( inTarget != NULL ) {
        char *message = autoSprintf( "PJ\n%d %d\n#", 
                                     inKiller->id, inTarget->id );
        sendMessageToPlayer( inTarget, message, strlen( message ) );
        delete [] message;
        }
    
    updatePosseSize( inTarget );
    
    return true;
    }



static void removeKillState( LiveObject *inKiller, LiveObject *inTarget ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
    
        if( s->killerID == inKiller->id &&
            s->targetID == inTarget->id ) {
            activeKillStates.deleteElement( i );
            
            updatePosseSize( inTarget, inKiller );
            break;
            }
        }

    if( inKiller != NULL ) {
        // clear their emot
        inKiller->emotFrozen = false;
        inKiller->emotUnfreezeETA = 0;
        
        newEmotPlayerIDs.push_back( inKiller->id );
        
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }

    int newPosseSize = 0;
    if( inTarget != NULL ) {
        newPosseSize = countPosseSize( inTarget );
        }
    
    if( newPosseSize == 0 &&
        inTarget != NULL &&
        inTarget->emotFrozen &&
        ( inTarget->emotFrozenIndex == victimEmotionIndex ||
          inTarget->emotFrozenIndex == victimTerrifiedEmotionIndex ) ) {
        
        // inTarget's emot hasn't been replaced, end it
        inTarget->emotFrozen = false;
        inTarget->emotUnfreezeETA = 0;
        
        newEmotPlayerIDs.push_back( inTarget->id );
        
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }

    // killer has left posse
    if( inTarget != NULL ) {
        char *message = autoSprintf( "PJ\n%d 0\n#", 
                                     inKiller->id );
        sendMessageToPlayer( inTarget, message, strlen( message ) );
        delete [] message;
        }
    
    }



static void removeAnyKillState( LiveObject *inKiller ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
    
        if( s->killerID == inKiller->id ) {
            
            LiveObject *target = getLiveObject( s->targetID );
            
            if( target != NULL ) {
                removeKillState( inKiller, target );
                i--;
                }
            }
        }
    }



static KillState *getKillState( LiveObject *inKiller ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
    
        if( s->killerID == inKiller->id ) {
            LiveObject *target = getLiveObject( s->targetID );
            
            if( target != NULL ) {
                return s;
                }
            else {
                return NULL;
                }
            }
        }
    return NULL;
    }



static char isAlreadyInKillState( LiveObject *inKiller ) {
    KillState *s = getKillState( inKiller );
    
    if( s != NULL ) {
        return true;
        }
    return false;
    }

            



static void interruptAnyKillEmots( int inPlayerID, 
                                   int inInterruptingTTL ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        
        if( s->killerID == inPlayerID ) {
            s->emotStartTime = Time::getCurrentTime();
            s->emotRefreshSeconds = inInterruptingTTL;
            break;
            }
        }
    }    



static void setPerpetratorHoldingAfterKill( LiveObject *nextPlayer,
                                            TransRecord *woundHit,
                                            TransRecord *rHit,
                                            TransRecord *r ) {

    int oldHolding = nextPlayer->holdingID;


    if( rHit != NULL ) {
        // if hit trans exist
        // leave bloody knife or
        // whatever in hand
        nextPlayer->holdingID = rHit->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
    else if( woundHit != NULL ) {
        // result of hit on held weapon 
        // could also be
        // specified in wound trans
        nextPlayer->holdingID = 
            woundHit->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
    else if( r != NULL ) {
        nextPlayer->holdingID = r->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
                        
    if( r != NULL || rHit != NULL || woundHit != NULL ) {
        
        nextPlayer->heldTransitionSourceID = 0;
        
        if( oldHolding != 
            nextPlayer->holdingID ) {
            
            setFreshEtaDecayForHeld( 
                nextPlayer );
            }
        }
    }



/*
static void printPath( LiveObject *inPlayer ) {
    printf( "Path: " );
    for( int i=0; i<inPlayer->pathLength; i++ ) {
        printf( "(%d,%d) ", inPlayer->pathToDest[i].x,
                inPlayer->pathToDest[i].y );
        }
    printf( "\n" );
    }
*/


static FILE *getKillLogFile( char outTimeStamp[16] ) {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];

    File logDir( NULL, "killHitLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory killHitLog is in the way" );
        return NULL;
        }

    strftime( fileName, 99, "%Y_%m%B_%d_%A.txt", timeStruct );
    
    
    strftime( outTimeStamp, 15, "%H:%M:%S -- ", timeStruct );
    

    File *newFile = logDir.getChildFile( fileName );
    
    char *newFileName = newFile->getFullFileName();
    delete newFile;
    
    FILE *logFile = fopen( newFileName, "a" );

    delete [] newFileName;

    return logFile;
    }



static void logKillHit( LiveObject *inVictim, LiveObject *inKiller ) {
    char timeStamp[16];
    
    FILE *logFile = getKillLogFile( timeStamp );

    if( logFile == NULL ) {
        return;
        }
    
    char *weaponName = getObject( inKiller->holdingID )->description;


    SimpleVector<LiveObject *> posseMembers;

    double killStateDuration = 0;
    int minPosseSizeForKill = 0;
    
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        if( s->targetID == inVictim->id ) {

            LiveObject *killerO = getLiveObject( s->killerID );
            
            if( killerO->id != inKiller->id ) {
                posseMembers.push_back( killerO );
                }
            else {
                killStateDuration = Time::getCurrentTime() - s->killStartTime;
                minPosseSizeForKill = s->minPosseSizeForKill;
                }
            }
        }
    
    fprintf( logFile, "%s", timeStamp );
    
    fprintf( logFile, "Kill hit landed on %d (%s), ",
             inVictim->id, inVictim->email );
    
    fprintf( logFile, "attacker is %d (%s), "
             "been in kill state for %.2f seconds, using weapon %d (%s), ",
             inKiller->id, inKiller->email, 
             killStateDuration, inKiller->holdingID, weaponName );
    
    fprintf( logFile, "min posse size for kill is %d, ",
             minPosseSizeForKill );

    if( posseMembers.size() == 0 ) {
        fprintf( logFile, "solo kill" );
        }
    else {
        fprintf( logFile, "group kill, posse of %d member(s) including:  ", 
                 posseMembers.size() );
    
        for( int i=0; i<posseMembers.size(); i++ ) {
            LiveObject *pm = posseMembers.getElementDirect( i );
            fprintf( logFile, "%d: %d (%s), ", i + 1, pm->id, pm->email );
            }
        }
    
    fprintf( logFile, "\n\n" );

    fclose( logFile );
    }



void logHealOfKill( LiveObject *inVictim, LiveObject *inHealer ) {
    char timeStamp[16];
        
    FILE *logFile = getKillLogFile( timeStamp );

    if( logFile == NULL ) {
        return;
        }


    fprintf( logFile, "%s", timeStamp );
    
    fprintf( logFile, "Kill hit healed on %d (%s), ",
             inVictim->id, inVictim->email );
    
    fprintf( logFile, "healer is %d (%s)",
             inHealer->id, inHealer->email );

    fprintf( logFile, "\n\n" );

    fclose( logFile );
    }




void executeKillAction( int inKillerIndex,
                        int inTargetIndex,
                        SimpleVector<int> *playerIndicesToSendUpdatesAbout,
                        SimpleVector<int> *playerIndicesToSendDyingAbout,
                        SimpleVector<int> *newEmotPlayerIDs,
                        SimpleVector<int> *newEmotIndices,
                        SimpleVector<int> *newEmotTTLs ) {
    int i = inKillerIndex;
    LiveObject *nextPlayer = players.getElement( inKillerIndex );    

    LiveObject *hitPlayer = players.getElement( inTargetIndex );

    GridPos targetPos = getPlayerPos( hitPlayer );


    // send update even if action fails (to let them
    // know that action is over)
    playerIndicesToSendUpdatesAbout->push_back( i );
                        
    if( nextPlayer->holdingID > 0 ) {
                            
        nextPlayer->actionAttempt = 1;
        nextPlayer->actionTarget.x = targetPos.x;
        nextPlayer->actionTarget.y = targetPos.y;
                            
        if( nextPlayer->actionTarget.x > nextPlayer->xd ) {
            nextPlayer->facingOverride = 1;
            }
        else if( nextPlayer->actionTarget.x < nextPlayer->xd ) {
            nextPlayer->facingOverride = -1;
            }

        // holding something
        ObjectRecord *heldObj = 
            getObject( nextPlayer->holdingID );
                            
        if( heldObj->deadlyDistance > 0 ) {
            // it's deadly

            GridPos playerPos = getPlayerPos( nextPlayer );
                                
            double d = distance( targetPos,
                                 playerPos );
                                
            if( heldObj->deadlyDistance >= d &&
                ! directLineBlocked( nextPlayer, playerPos, 
                                     targetPos ) ) {
                // target is close enough
                // and no blocking objects along the way                

                char someoneHit = false;


                if( hitPlayer != NULL &&
                    strstr( heldObj->description,
                            "otherFamilyOnly" ) ) {
                    // make sure victim is in
                    // different family
                    // and no treaty
                                        
                    if( hitPlayer->lineageEveID ==
                        nextPlayer->lineageEveID
                        || 
                        isPeaceTreaty( hitPlayer->lineageEveID,
                                       nextPlayer->lineageEveID )
                        ||
                        ! isWarState( hitPlayer->lineageEveID,
                                      nextPlayer->lineageEveID ) ) {      
                        hitPlayer = NULL;
                        }
                    }
                

                // special case:
                // non-lethal no_replace ends up in victim's hand
                // they aren't dying, but they may have an emot
                // effect only
                if( hitPlayer != NULL ) {

                    TransRecord *woundHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, true, false );

                    if( woundHit != NULL && woundHit->newTarget > 0 &&
                        strstr( getObject( woundHit->newTarget )->description,
                                "no_replace" ) != NULL ) {
                        
                        
                        TransRecord *rHit = 
                            getPTrans( nextPlayer->holdingID, 0, false, true );
                        
                        TransRecord *r = 
                            getPTrans( nextPlayer->holdingID, 0 );

                        setPerpetratorHoldingAfterKill( nextPlayer,
                                                        woundHit, rHit, r );
                        
                        ForcedEffects e = 
                            checkForForcedEffects( woundHit->newTarget );
                            
                        // emote-effect only for no_replace
                        // no fever or food effect
                        if( e.emotIndex != -1 ) {
                            hitPlayer->emotFrozen = 
                                true;
                            hitPlayer->emotFrozenIndex = e.emotIndex;
                            
                            hitPlayer->emotUnfreezeETA =
                                Time::getCurrentTime() + e.ttlSec;
                            
                            newEmotPlayerIDs->push_back( 
                                hitPlayer->id );
                            newEmotIndices->push_back( 
                                e.emotIndex );
                            newEmotTTLs->push_back( 
                                e.ttlSec );

                            interruptAnyKillEmots( hitPlayer->id,
                                                   e.ttlSec );
                            }
                        return;
                        }
                    }
                

                if( hitPlayer != NULL ) {
                    someoneHit = true;
                    // break the connection with 
                    // them, eventually
                    // let them stagger a bit first

                    hitPlayer->murderSourceID =
                        nextPlayer->holdingID;
                                        
                    hitPlayer->murderPerpID =
                        nextPlayer->id;
                                        
                    // brand this player as a murderer
                    nextPlayer->everKilledAnyone = true;

                    if( hitPlayer->murderPerpEmail 
                        != NULL ) {
                        delete [] 
                            hitPlayer->murderPerpEmail;
                        }
                                        
                    hitPlayer->murderPerpEmail =
                        stringDuplicate( 
                            nextPlayer->email );
                                        

                    setDeathReason( hitPlayer, 
                                    "killed",
                                    nextPlayer->holdingID );

                    // if not already dying
                    if( ! hitPlayer->dying ) {
                        int staggerTime = 
                            SettingsManager::getIntSetting(
                                "deathStaggerTime", 20 );
                                            
                        double currentTime = 
                            Time::getCurrentTime();
                                            
                        hitPlayer->dying = true;
                        hitPlayer->dyingETA = 
                            currentTime + staggerTime;

                        playerIndicesToSendDyingAbout->
                            push_back( 
                                getLiveObjectIndex( 
                                    hitPlayer->id ) );
                                        
                        hitPlayer->errorCauseString =
                            "Player killed by other player";
                        }
                    else {
                        // already dying, 
                        // and getting attacked again
                        
                        // halve their remaining 
                        // stagger time
                        double currentTime = 
                            Time::getCurrentTime();
                                             
                        double staggerTimeLeft = 
                            hitPlayer->dyingETA - 
                            currentTime;
                        
                        if( staggerTimeLeft > 0 ) {
                            staggerTimeLeft /= 2;
                            hitPlayer->dyingETA = 
                                currentTime + 
                                staggerTimeLeft;
                            }
                        }
                    }
                                    
                                    
                // a player either hit or not
                // in either case, weapon was used
                                    
                // check for a transition for weapon

                // 0 is generic "on person" target
                TransRecord *r = 
                    getPTrans( nextPlayer->holdingID, 
                               0 );

                TransRecord *rHit = NULL;
                TransRecord *woundHit = NULL;
                                    
                if( someoneHit ) {
                    
                    logKillHit( hitPlayer, nextPlayer );
                    
                    nextPlayer->lastKillTime = Time::getCurrentTime();
                    

                    // last use on target specifies
                    // grave and weapon change on hit
                    // non-last use (r above) specifies
                    // what projectile ends up in grave
                    // or on ground
                    rHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, false, true );
                                        
                    if( rHit != NULL &&
                        rHit->newTarget > 0 ) {
                        hitPlayer->customGraveID = 
                            rHit->newTarget;
                        }
                                        
                    char wasSick = false;
                                        
                    if( hitPlayer->holdingID > 0 &&
                        strstr(
                            getObject( 
                                hitPlayer->holdingID )->
                            description,
                            "sick" ) != NULL ) {
                        wasSick = true;
                        }

                    // last use on actor specifies
                    // what is left in victim's hand
                    woundHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, true, false );
                                        
                    if( woundHit != NULL &&
                        woundHit->newTarget > 0 ) {
                                            
                        // don't drop their wound
                        if( hitPlayer->holdingID != 0 &&
                            ! hitPlayer->holdingWound &&
                            ! hitPlayer->holdingBiomeSickness ) {
                            handleDrop( 
                                targetPos.x, targetPos.y, 
                                hitPlayer,
                                playerIndicesToSendUpdatesAbout );
                            }

                        // give them a new wound
                        // if they don't already have
                        // one, but never replace their
                        // original wound.  That allows
                        // a healing exploit where you
                        // intentionally give someone
                        // an easier-to-treat wound
                        // to replace their hard-to-treat
                        // wound

                        // however, do let wounds replace
                        // sickness
                        char woundChange = false;
                                            
                        if( ! hitPlayer->holdingWound ||
                            wasSick ) {
                            woundChange = true;
                                                
                            hitPlayer->holdingID = 
                                woundHit->newTarget;
                            holdingSomethingNew( 
                                hitPlayer );
                            setFreshEtaDecayForHeld( 
                                hitPlayer );
                            }
                                            
                                            
                        hitPlayer->holdingWound = true;
                        hitPlayer->holdingBiomeSickness = false;
                        
                        if( woundChange ) {
                                                
                            ForcedEffects e = 
                                checkForForcedEffects( 
                                    hitPlayer->holdingID );
                            
                            if( e.emotIndex != -1 ) {
                                hitPlayer->emotFrozen = 
                                    true;
                                hitPlayer->emotFrozenIndex = e.emotIndex;
                                
                                newEmotPlayerIDs->push_back( 
                                    hitPlayer->id );
                                newEmotIndices->push_back( 
                                    e.emotIndex );
                                newEmotTTLs->push_back( 
                                    e.ttlSec );
                                interruptAnyKillEmots( hitPlayer->id,
                                                       e.ttlSec );
                                }
                                            
                            if( e.foodModifierSet && 
                                e.foodCapModifier != 1 ) {
                                hitPlayer->yummyBonusStore = 0;
                                hitPlayer->
                                    foodCapModifier = 
                                    e.foodCapModifier;
                                hitPlayer->foodUpdate = 
                                    true;
                                }
                                                
                            if( e.feverSet ) {
                                hitPlayer->fever = e.fever;
                                }

                            checkSickStaggerTime( 
                                hitPlayer );
                                                
                            playerIndicesToSendUpdatesAbout->
                                push_back( 
                                    getLiveObjectIndex( 
                                        hitPlayer->id ) );
                            }   
                        }
                    }
                                    

                int oldHolding = nextPlayer->holdingID;

                setPerpetratorHoldingAfterKill( nextPlayer, 
                                                woundHit, rHit, r );

                // if they are moving, end their move NOW
                // (this allows their move speed to get updated
                //  with the murder weapon before their next move)
                // Otherwise, if their move continues, they might walk
                // at the wrong speed with the changed weapon
                

                endAnyMove( nextPlayer );
                

                timeSec_t oldEtaDecay = 
                    nextPlayer->holdingEtaDecay;
                                    

                if( r != NULL ) {
                                    
                    if( hitPlayer != NULL &&
                        r->newTarget != 0 ) {
                                        
                        hitPlayer->embeddedWeaponID = 
                            r->newTarget;
                                        
                        if( oldHolding == r->newTarget ) {
                            // what we are holding
                            // is now embedded in them
                            // keep old decay
                            hitPlayer->
                                embeddedWeaponEtaDecay =
                                oldEtaDecay;
                            }
                        else {
                                            
                            TransRecord *newDecayT = 
                                getMetaTrans( 
                                    -1, 
                                    r->newTarget );
                    
                            if( newDecayT != NULL ) {
                                hitPlayer->
                                    embeddedWeaponEtaDecay = 
                                    Time::timeSec() + 
                                    newDecayT->
                                    autoDecaySeconds;
                                }
                            else {
                                // no further decay
                                hitPlayer->
                                    embeddedWeaponEtaDecay 
                                    = 0;
                                }
                            }
                        }
                    else if( hitPlayer == NULL &&
                             isMapSpotEmpty( targetPos.x, 
                                             targetPos.y ) ) {
                        // this is old code, and probably never gets executed
                        
                        // no player hit, and target ground
                        // spot is empty
                        setMapObject( targetPos.x, targetPos.y, 
                                      r->newTarget );
                                        
                        // if we're thowing a weapon
                        // target is same as what we
                        // were holding
                        if( oldHolding == r->newTarget ) {
                            // preserve old decay time 
                            // of what we were holding
                            setEtaDecay( targetPos.x, targetPos.y,
                                         oldEtaDecay );
                            }
                        }
                    // else new target, post-kill-attempt
                    // is lost
                    }
                }
            }
        }
    }




static void nameEve( LiveObject *nextPlayer, char *name ) {
    
    const char *close = findCloseLastName( name );
    nextPlayer->name = autoSprintf( "%s %s", eveName, close );
    
                                
    nextPlayer->name = getUniqueCursableName( 
        nextPlayer->name, 
        &( nextPlayer->nameHasSuffix ),
        true,
        getFemale( nextPlayer ) );
                                
    char firstName[99];
    char lastName[99];
    char suffix[99];
    
    if( nextPlayer->nameHasSuffix ) {
        
        sscanf( nextPlayer->name, 
                "%s %s %s", 
                firstName, lastName, suffix );
        }
    else {
        sscanf( nextPlayer->name, 
                "%s %s", 
                firstName, lastName );
        }
    
    nextPlayer->familyName = 
        stringDuplicate( lastName );
    
    
    if( ! nextPlayer->isTutorial ) {    
        logName( nextPlayer->id,
                 nextPlayer->email,
                 nextPlayer->name,
                 nextPlayer->lineageEveID );
        }
    }

                                


void nameBaby( LiveObject *inNamer, LiveObject *inBaby, char *inName,
               SimpleVector<int> *playerIndicesToSendNamesAbout ) {    

    LiveObject *nextPlayer = inNamer;
    LiveObject *babyO = inBaby;
    
    char *name = inName;
    

    // NEW:  keep the baby's family name at all costs, even in case
    // of adoption
    // (if baby has no family name, then take mother's family name as last
    // name)
    
    const char *lastName = "";
    

    // note that we skip this case now, in favor of keeping baby's family name
    if( false && nextPlayer->name != NULL ) {
        lastName = strstr( nextPlayer->name, 
                           " " );
                                        
        if( lastName != NULL ) {
            // skip space
            lastName = &( lastName[1] );
            }

        if( lastName == NULL ) {
            lastName = "";

            if( nextPlayer->familyName != 
                NULL ) {
                lastName = 
                    nextPlayer->familyName;
                }    
            }
        else if( nextPlayer->nameHasSuffix ) {
            // only keep last name
            // if it contains another
            // space (the suffix is after
            // the last name).  Otherwise
            // we are probably confused,
            // and what we think
            // is the last name IS the suffix.
                                            
            char *suffixPos =
                strstr( (char*)lastName, " " );
                                            
            if( suffixPos == NULL ) {
                // last name is suffix, actually
                // don't pass suffix on to baby
                lastName = "";
                }
            else {
                // last name plus suffix
                // okay to pass to baby
                // because we strip off
                // third part of name
                // (suffix) below.
                }
            }
        }
    else if( babyO->familyName != NULL ) {
        lastName = babyO->familyName;
        }
    else if( nextPlayer->familyName != NULL ) {
        lastName = nextPlayer->familyName;
        }
                                    


    const char *close = 
        findCloseFirstName( name, getFemale( inBaby ) );

    if( strcmp( lastName, "" ) != 0 ) {    
        babyO->name = autoSprintf( "%s %s",
                                   close, 
                                   lastName );
        }
    else {
        babyO->name = stringDuplicate( close );
        }
    
    
    if( babyO->familyName == NULL &&
        nextPlayer->familyName != NULL ) {
        // mother didn't have a family 
        // name set when baby was born
        // now she does
        // or whatever player named 
        // this orphaned baby does
        babyO->familyName = 
            stringDuplicate( 
                nextPlayer->familyName );
        }
                                    

    int spaceCount = 0;
    int lastSpaceIndex = -1;

    int nameLen = strlen( babyO->name );
    for( int s=0; s<nameLen; s++ ) {
        if( babyO->name[s] == ' ' ) {
            lastSpaceIndex = s;
            spaceCount++;
            }
        }
                                    
    if( spaceCount > 1 ) {
        // remove suffix from end
        babyO->name[ lastSpaceIndex ] = '\0';
        }
                                    
    babyO->name = getUniqueCursableName( 
        babyO->name, 
        &( babyO->nameHasSuffix ), false,
        getFemale( babyO ) );
                                    
    logName( babyO->id,
             babyO->email,
             babyO->name,
             babyO->lineageEveID );
                                    
    playerIndicesToSendNamesAbout->push_back( 
        getLiveObjectIndex( babyO->id ) );
    }



// after person has been named, use this to filter phrase itself
// destroys inSaidPhrase and replaces it
void replaceNameInSaidPhrase( char *inSaidName, char **inSaidPhrase,
                              LiveObject *inNamedPerson, 
                              char inForceBoth = false ) {
    char *trueName;
    if( inForceBoth || strstr( inSaidName, " " ) != NULL ) {
        // multi-word said name
        // assume first and last name
        trueName = stringDuplicate( inNamedPerson->name );
        }
    else {
        // single-word said name
        trueName = stringDuplicate( inNamedPerson->name );
        // trim off last name, if there is one
        char *spacePos = strstr( trueName, " " );
        if( spacePos != NULL ) {
            spacePos[0] = '\0';
            }
        }
    char found = false;
    char *newPhrase = replaceOnce( *inSaidPhrase, inSaidName, trueName,
                                   &found );
    delete [] trueName;
    
    delete [] (*inSaidPhrase);

    *inSaidPhrase = newPhrase;
    }




void getLineageLineForPlayer( LiveObject *inPlayer,
                              SimpleVector<char> *inVector ) {
    
    char *pID = autoSprintf( "%d", inPlayer->id );
    inVector->appendElementString( pID );
    delete [] pID;
    
    for( int j=0; j<inPlayer->lineage->size(); j++ ) {
        char *mID = 
            autoSprintf( 
                " %d",
                inPlayer->lineage->getElementDirect( j ) );
        inVector->appendElementString( mID );
        delete [] mID;
        }        
    // include eve tag at end
    char *eveTag = autoSprintf( " eve=%d",
                                inPlayer->lineageEveID );
    inVector->appendElementString( eveTag );
    delete [] eveTag;
    
    inVector->push_back( '\n' );            
    }



// result NOT destroyed by caller
// can be NULL if not found
static char *getLineageLastName( int inLineageEveID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( p->lineageEveID == inLineageEveID &&
            p->familyName != NULL ) {
            return p->familyName;
            }
        }
    return NULL;
    }




static void sendHomelandMessage( LiveObject *nextPlayer,
                                 int homeLineageEveID,
                                 GridPos homeCenter ) {    
    const char *famName = "0";
    
    if( homeLineageEveID != -1 ) {
        char *realFamName =
            getLineageLastName( 
                homeLineageEveID );
        if( realFamName != NULL ) {
            famName = realFamName;
            }
        else {
            famName = "UNNAMED";
            }
        }
    
    char *message = 
        autoSprintf( 
            "HL\n"
            "%d %d %s\n#",
            homeCenter.x -
            nextPlayer->birthPos.x,
            homeCenter.y -
            nextPlayer->birthPos.y,
            famName );
    sendMessageToPlayer( 
        nextPlayer, 
        message, 
        strlen( message ) );
    delete [] message;
    }



static void endBiomeSickness( 
    LiveObject *nextPlayer,
    int i,
    SimpleVector<int> *playerIndicesToSendUpdatesAbout ) {
    
    int oldSickness = -1;
    
    if( ! nextPlayer->holdingWound ) {
        // back to holding nothing
        oldSickness = nextPlayer->holdingID;
                                        
        nextPlayer->holdingID = 0;
                                        
        playerIndicesToSendUpdatesAbout->
            push_back( i );
        }
                                    
    nextPlayer->holdingBiomeSickness = 
        false;

    // relief emot
    nextPlayer->emotFrozen = false;
    nextPlayer->emotUnfreezeETA = 0;
    nextPlayer->emotFrozenIndex = 0;
    
    newEmotPlayerIDs.push_back( 
        nextPlayer->id );
        
    int newEmot = 
        getBiomeReliefEmot( oldSickness );
                                    
    if( newEmot != -1 ) {
        newEmotIndices.push_back( newEmot );
        // 3 sec
        newEmotTTLs.push_back( 3 );
        }
    else {
        // clear
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }
    }






void logFitnessDeath( LiveObject *nextPlayer ) {
    
    // log this death for fitness purposes,
    // for both tutorial and non    


    // if this person themselves died before their mom, gma, etc.
    // remove them from the "ancestor" list of everyone who is older than they
    // are and still alive

    // You only get genetic points for ma, gma, and other older ancestors
    // if you are alive when they die.

    // This ends an exploit where people suicide as a baby (or young person)
    // yet reap genetic benefit from their mother living a long life
    // (your mother, gma, etc count for your genetic score if you yourself
    //  live beyond 3, so it is in your interest to protect them)
    double deadPersonAge = computeAge( nextPlayer );
    if( deadPersonAge < forceDeathAge ) {
        for( int i=0; i<players.size(); i++ ) {
                
            LiveObject *o = players.getElement( i );
            
            if( o->error ||
                o->isTutorial ||
                o->id == nextPlayer->id ) {
                continue;
                }
            
            if( computeAge( o ) < deadPersonAge ) {
                // this person was born after the dead person
                // thus, there's no way they are their ma, gma, etc.
                continue;
                }

            for( int e=0; e< o->ancestorIDs->size(); e++ ) {
                if( o->ancestorIDs->getElementDirect( e ) == nextPlayer->id ) {
                    o->ancestorIDs->deleteElement( e );
                    
                    delete [] o->ancestorEmails->getElementDirect( e );
                    o->ancestorEmails->deleteElement( e );
                
                    delete [] o->ancestorRelNames->getElementDirect( e );
                    o->ancestorRelNames->deleteElement( e );
                    
                    o->ancestorLifeStartTimeSeconds->deleteElement( e );

                    break;
                    }
                }
            }
        }


    SimpleVector<int> emptyAncestorIDs;
    SimpleVector<char*> emptyAncestorEmails;
    SimpleVector<char*> emptyAncestorRelNames;
    SimpleVector<double> emptyAncestorLifeStartTimeSeconds;
    

    SimpleVector<int> *ancestorIDs = nextPlayer->ancestorIDs;
    SimpleVector<char*> *ancestorEmails = nextPlayer->ancestorEmails;
    SimpleVector<char*> *ancestorRelNames = nextPlayer->ancestorRelNames;
    SimpleVector<double> *ancestorLifeStartTimeSeconds = 
        nextPlayer->ancestorLifeStartTimeSeconds;
    

    if( nextPlayer->suicide ) {
        // don't let this suicide death affect scores of any ancestors
        ancestorIDs = &emptyAncestorIDs;
        ancestorEmails = &emptyAncestorEmails;
        ancestorRelNames = &emptyAncestorRelNames;
        ancestorLifeStartTimeSeconds = &emptyAncestorLifeStartTimeSeconds;
        }
    else {
        // any that never made it to age 3+ by the time this person died
        // should not be counted.  What could they have done to keep us alive
        // Note that this misses one case... an older sib that died at age 2.5
        // and then we died at age 10 or whatever.  They are age "12.5" right
        // now, even though they are dead.  We're not still tracking them,
        // though, so we don't know.
        double curTime = Time::getCurrentTime();
        
        double ageRate = getAgeRate();
        
        for( int i=0; i<ancestorEmails->size(); i++ ) {
            double startTime = 
                ancestorLifeStartTimeSeconds->getElementDirect( i );
            
            if( ageRate * ( curTime - startTime ) < defaultActionAge ) {
                // too young to have taken action to help this person
                ancestorIDs->deleteElement( i );
                
                delete [] ancestorEmails->getElementDirect( i );
                ancestorEmails->deleteElement( i );
                
                delete [] ancestorRelNames->getElementDirect( i );
                ancestorRelNames->deleteElement( i );
                
                ancestorLifeStartTimeSeconds->deleteElement( i );
                
                i--;
                }
            }
        
        }    


    logFitnessDeath( players.size(),
                     nextPlayer->email, 
                     nextPlayer->name, nextPlayer->displayID,
                     computeAge( nextPlayer ),
                     ancestorEmails, 
                     ancestorRelNames );
    }




static void logClientTag( FreshConnection *inConnection ) {
    const char *tagToLog = "no_tag";
    
    if( inConnection->clientTag != NULL ) {
        tagToLog = inConnection->clientTag;
        }
    
    FILE *log = fopen( "clientTagLog.txt", "a" );
    
    if( log != NULL ) {
        fprintf( log, "%.0f %s %s\n", Time::getCurrentTime(),
                 inConnection->email, tagToLog );
        
        fclose( log );
        }
    }



static void sendLearnedToolMessage( LiveObject *inPlayer,
                                    SimpleVector<int> *inNewToolSets ) {
    SimpleVector<int> setList;
    
    for( int i=0; i < inNewToolSets->size(); i++ ) {
        getToolSetMembership( inNewToolSets->getElementDirect(i), 
                              &( setList ) );
        }

    // send LR message to let client know that these tools are learned now
    SimpleVector<char> messageWorking;
    
    messageWorking.appendElementString( "LR\n" );
    for( int i=0; i<setList.size(); i++ ) {
        if( i > 0 ) {
            messageWorking.appendElementString( " " );
            }
        char *idString = autoSprintf( "%d", 
                                      setList.getElementDirect( i ) );
        messageWorking.appendElementString( idString );
        delete [] idString;
        }
    messageWorking.appendElementString( "\n#" );
    char *lrMessage = messageWorking.getElementString();
    
    sendMessageToPlayer( inPlayer, lrMessage, strlen( lrMessage ) );
    delete [] lrMessage;


    char *tsMessage = autoSprintf( "TS\n%d %d\n#", 
                                   inPlayer->learnedTools.size(),
                                   inPlayer->numToolSlots );
    
    sendMessageToPlayer( inPlayer, tsMessage, strlen( tsMessage ) );
    delete [] tsMessage;
    }



static char *getToolSetDescription( ObjectRecord *inToolO ) {
    const char *article = "THE ";
        
    char *des = stringToUpperCase( inToolO->description );

    
    
    // if it's a group of tools, like +toolSterile_Technique
    // show the group name instead of the individual tool
    
    char *toolPos = strstr( des, "+TOOL" );
    
    char isToolGroup = false;
    
    if( toolPos != NULL ) {
        char *tagPos = &( toolPos[5] );
        
        // use dummies can have # immediately after +TOOL tag
        if( tagPos[0] != '\0' && tagPos[0] != ' ' && tagPos[0] != '#' ) {
            int tagLen = strlen( tagPos );
            for( int i=0; i<tagLen; i++ ) {
                if( tagPos[i] == ' ' || tagPos[i] == '#' ) {
                    tagPos[i] = '\0';
                    break;
                    }
                }
            // now replace any _ with ' '
            tagLen = strlen( tagPos );
            for( int i=0; i<tagLen; i++ ) {
                if( tagPos[i] == '_' ) {
                    tagPos[i] = ' ';
                    }
                }
            char *newDes = stringDuplicate( tagPos );
            delete [] des;
            des = newDes;
            isToolGroup = true;
            }
        }
    
        
    stripDescriptionComment( des );
    
    int desLen = strlen( des );
    if( isToolGroup ||
        ( desLen > 0 && des[ desLen - 1 ] == 'S' ) ||
        ( desLen > 2 && des[ desLen - 1 ] == 'G'
          && des[ desLen - 2 ] == 'N' 
          && des[ desLen - 3 ] == 'I' ) ) {
        // use THE for singular tools like YOU LEARNED THE AXE
        // no article for plural tools like YOU LEARNED KNITTING NEEDLES
        // no article for activities (usually tool groups) like SEWING
        article = "";
        }
    
    char *finalDes = autoSprintf( "%s%s", article, des );
    delete [] des;
    
    return finalDes;
    }

    


static char learnTool( LiveObject *inPlayer, int inToolID ) {
    ObjectRecord *toolO = getObject( inToolID );
                                    
    // is it a marked tool?
    int toolSet = toolO->toolSetIndex;
    
    if( toolSet != -1 &&
        inPlayer->learnedTools.getElementIndex( toolSet ) == -1 &&
        inPlayer->numToolSlots > inPlayer->learnedTools.size() ) {

        
        

        if( inPlayer->partiallyLearnedTools.getElementIndex( toolSet ) == 
            -1  ) {
        
            int numLeft =
                inPlayer->numToolSlots - inPlayer->learnedTools.size();
            
            // they can try this tool once for free, without learning it
            inPlayer->partiallyLearnedTools.push_back( toolSet );
            
            char *des = getToolSetDescription( toolO );

            char *message;
            if( numLeft > 1 ) {
                message = autoSprintf( 
                    "YOU ALMOST LEARNED %s.**"
                    "REPEAT THAT ACTION TO SPEND ONE YOUR %d "
                    "REMAINING TOOL SLOTS.", 
                    des,
                    numLeft );
                }
            else {
                message = autoSprintf( 
                    "YOU ALMOST LEARNED %s.**"
                    "REPEAT THAT ACTION TO SPEND YOUR LAST "
                    "REMAINING TOOL SLOT.", 
                    des );
                }
            sendGlobalMessage( message, inPlayer );

            delete [] message;
            delete [] des;

            return true;
            }
        


        
        inPlayer->learnedTools.push_back( toolSet );
        
        SimpleVector<int> newToolSets;
        newToolSets.push_back( toolSet );
        
        sendLearnedToolMessage( inPlayer, &newToolSets );
        
        
        // now send DING message
        char *des = getToolSetDescription( toolO );
        

        char *message;
        
        int numLeft = inPlayer->numToolSlots - inPlayer->learnedTools.size();
        
        if( numLeft > 0 ) {
            message = autoSprintf( "YOU LEARNED %s.**"
                                   "%d OF %d TOOL SLOTS ARE LEFT.", 
                                   des,
                                   numLeft,
                                   inPlayer->numToolSlots );
            }
        else {
            message = autoSprintf( "YOU LEARNED %s.**"
                                   "ALL OF YOUR TOOL SLOTS HAVE BEEN USED.", 
                                   des );            
            }
        
        sendGlobalMessage( message, inPlayer );
        
        delete [] des;
        delete [] message;


        return true;
        }
    return false;
    }


static char canPlayerUseOrLearnTool( LiveObject *inPlayer, int inToolID ) {
    if( ! canPlayerUseTool( inPlayer, inToolID ) ) {
        return learnTool( inPlayer, inToolID );
        }
    return true;
    }



char isBiomeAllowedForPlayer( LiveObject *inPlayer, int inX, int inY,
                              char inIgnoreFloor ) {
    if( inPlayer->vogMode ||
        inPlayer->forceSpawn ||
        inPlayer->isTutorial ) {
        return true;
        }

    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *heldO = getObject( inPlayer->holdingID );
        if( heldO->permanent &&
            heldO->speedMult == 0 ) {
            // what they're holding is stuck stuck stuck, and they can't
            // move at all.
            
            // is there some way for them to drop it?
            // this prevents us from mistakenly dropping wounds that
            // don't let you move or whatever
            TransRecord *bareGroundT = getPTrans( inPlayer->holdingID, -1 );
            
            if( bareGroundT != NULL && bareGroundT->newTarget > 0 ) {
                // Don't block them from dropping this object
                return true;
                }
            }
        }

    return isBiomeAllowed( inPlayer->displayID, inX, inY, inIgnoreFloor );
    }



static char isPlayerBlockedFromHoldingByPosse( LiveObject *inPlayer ) {
    int minPosseSizeForKill = 0;
    char fullForceSoloPosse = false;
    
    int posseSize = countPosseSize( 
        inPlayer,
        &minPosseSizeForKill,
        &fullForceSoloPosse );
                            
    // deadly solo posses in wilderness don't block victim from holding stuff
    if( ( fullForceSoloPosse && posseSize >= 1 )
        || 
        ( posseSize >= minPosseSizeForKill && posseSize > 1 ) ) {
        return true;
        }
    return false;
    }



static char heldNeverDrop( LiveObject *inPlayer ) {
    if( inPlayer->holdingID > 0 ) {        
        ObjectRecord *o = getObject( inPlayer->holdingID );
        if( strstr( o->description, "+neverDrop" ) != NULL ) {
            return true;
            }
        }
    return false;
    }



// does not force-drop of player holding wound, sickness, etc.
static void tryToForceDropHeld( 
    LiveObject *inTargetPlayer, 
    SimpleVector<int> *playerIndicesToSendUpdatesAbout ) {
    
    if( inTargetPlayer->holdingID != 0 &&
        ! inTargetPlayer->holdingWound &&
        ! inTargetPlayer->holdingBiomeSickness &&
        ! heldNeverDrop( inTargetPlayer ) ) {
                                    
        GridPos p = getPlayerPos( inTargetPlayer );
        
        int pi = getLiveObjectIndex( inTargetPlayer->id );
                                    
        playerIndicesToSendUpdatesAbout->push_back( pi );
                                    
        handleDrop( p.x, p.y, inTargetPlayer,
                    playerIndicesToSendUpdatesAbout );
        }
    }

    


// access blocked b/c of access direction or ownership?
static char isAccessBlocked( LiveObject *inPlayer, 
                             int inTargetX, int inTargetY,
                             int inTargetID,
                             int inHoldingID = 0 ) {
    int target = inTargetID;
    
    int x = inTargetX;
    int y = inTargetY;
    

    char wrongSide = false;
    char ownershipBlocked = false;
    
    if( target > 0 ) {
        ObjectRecord *targetObj = getObject( target );

        if( isGridAdjacent( x, y,
                            inPlayer->xd, 
                            inPlayer->yd ) ) {
            
            if( targetObj->sideAccess ) {
                
                if( y > inPlayer->yd ||
                    y < inPlayer->yd ) {
                    // access from N or S
                    wrongSide = true;
                    }
                }
            else if( targetObj->noBackAccess ) {
                if( y < inPlayer->yd ) {
                    // access from N
                    wrongSide = true;
                    }
                }
            }
        if( targetObj->isOwned ) {
            // make sure player owns this pos
            // (or is part of ally pool that can access it)
            ownershipBlocked = 
                ! isOwnedOrAllyOwned( inPlayer, targetObj, x, y );
            
            if( ownershipBlocked && 
                targetObj->isTempOwned &&
                inHoldingID > 0 ) {
                // if they are attempting a transition that leads to a
                // non-owned object, let it through, because ownership
                // of the target object is on shaky footing
                TransRecord *t = getTrans( inHoldingID, inTargetID );
                if( t != NULL && 
                    ( t->newTarget <= 0 ||
                      ! getObject( t->newTarget )->isOwned ) ) {
                    ownershipBlocked = false;
                    }
                }
            else if( ! ownershipBlocked &&
                     ! targetObj->isTempOwned &&
                     inHoldingID > 0 &&
                     ! isOwned( inPlayer, x, y ) ) {
                // they're not blocked, but they don't own it directly
                // their leader must own it
                // make sure this action isn't going to destroy it
                // (only a direct owner, not a follower, can destroy owned
                //  property and convert it into a non-owned object)
                TransRecord *t = getTrans( inHoldingID, inTargetID );
                if( t != NULL && 
                    ( t->newTarget <= 0 ||
                      ! getObject( t->newTarget )->isOwned ) ) {
                    ownershipBlocked = true;
                    }
                }
            
            

            if( ownershipBlocked ) {
                GridPos ourPos = getPlayerPos( inPlayer );

                // find closest owner
                int closeID = -1;
                LiveObject *closePlayer = NULL;
                GridPos closePos;
                double closeDist = DBL_MAX;
                
                for( int j=0; j<players.size(); j++ ) {
                    LiveObject *otherPlayer = players.getElement( j );
                    if( ! otherPlayer->error &&
                        isOwned( otherPlayer, x, y ) ) {
                        
                        GridPos p = getPlayerPos( otherPlayer );
                        
                        double d = distance( ourPos, p );
                        
                        if( d < closeDist ) {
                            closeDist = d;
                            closePos = p;
                            closeID = otherPlayer->id;
                            closePlayer = otherPlayer;
                            }
                        }
                    }            
                
                if( closeID != -1 ) {

                    char *message = autoSprintf( "PS\n"
                                                 "%d/0 CLOSEST OWNER "
                                                 "*owner %d *map %d %d\n#",
                                                 inPlayer->id,
                                                 closeID,
                                                 closePos.x - 
                                                 inPlayer->birthPos.x,
                                                 closePos.y - 
                                                 inPlayer->birthPos.y );
                    sendMessageToPlayer( inPlayer, message, strlen( message ) );
                    delete [] message;

                    if( closeDist > getMaxChunkDimension() ) {
                        // closest owner is out of range
                        
                        // send the owner a VISITOR message to let them
                        // know that they are needed back at home
                    
                        // but don't bug them about this too often
                        // not more than once a minute
                        
                        double curTime = Time::getCurrentTime();
                        
                        if( curTime - 
                            closePlayer->lastGateVisitorNoticeTime > 60 ) {

                            message = autoSprintf( "PS\n"
                                                   "%d/0 A GATE VISITOR "
                                                   "*visitor %d *map %d %d\n#",
                                                   closePlayer->id,
                                                   inPlayer->id,
                                                   ourPos.x - 
                                                   closePlayer->birthPos.x,
                                                   ourPos.y - 
                                                   closePlayer->birthPos.y );
                            sendMessageToPlayer( 
                                closePlayer, message, strlen( message ) );
                            delete [] message;
                            
                            closePlayer->lastGateVisitorNoticeTime = curTime;
                            }
                        }
                    }
                }
            }
        }
    return wrongSide || ownershipBlocked;
    }



void sendHungryWorkSpeech( LiveObject *inPlayer ) {
    // tell player about it with private speech
    char *message = autoSprintf( 
        "PS\n"
        "%d/0 +MORE FOOD+\n#",
        inPlayer->id );
    
    sendMessageToPlayer( 
        inPlayer, 
        message, 
        strlen( message ) );
    delete [] message;
    }



// cost set to 0 unless hungry work not blocked
char isHungryWorkBlocked( LiveObject *inPlayer, 
                          int inNewTarget, int *outCost ) {          
    *outCost = 0;
    
    char *des =
        getObject( inNewTarget )->description;
                                    
    char *desPos =
        strstr( des, "+hungryWork" );
    
    if( desPos != NULL ) {
                                        
        int cost = 0;
        
        sscanf( desPos,
                "+hungryWork%d", 
                &cost );
        
        if( inPlayer->foodStore + 
            inPlayer->yummyBonusStore < 
            cost + 4 ) {
            // block hungry work,
            // not enough food to have a
            // "safe" buffer after
            return true;
            }
        
        // can do work
        *outCost = cost;
        return false;
        }

    // not hungry work at all
    return false;
    }



// returns NULL if not found
static LiveObject *getPlayerByName( char *inName, 
                                    LiveObject *inPlayerSayingName ) {
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            otherPlayer != inPlayerSayingName &&
            otherPlayer->name != NULL &&
            strcmp( otherPlayer->name, inName ) == 0 ) {
            
            return otherPlayer;
            }
        }
    
    // no exact match.

    // does name contain no space?
    char *spacePos = strstr( inName, " " );

    if( spacePos == NULL ) {
        // try again, using just the first name for each potential target

        // stick a space at the end to forbid matching prefix of someone's name
        char *firstName = autoSprintf( "%s ", inName );

        LiveObject *matchingPlayer = NULL;
        double matchingDistance = DBL_MAX;
        
        GridPos playerPos = getPlayerPos( inPlayerSayingName );
        

        for( int j=0; j<players.size(); j++ ) {
            LiveObject *otherPlayer = players.getElement( j );
            if( ! otherPlayer->error &&
                otherPlayer != inPlayerSayingName &&
                otherPlayer->name != NULL &&
                // does their name start with firstName
                strstr( otherPlayer->name, firstName ) == otherPlayer->name ) {
                
                GridPos pos = getPlayerPos( otherPlayer );            
                double d = distance( pos, playerPos );
                
                if( d < matchingDistance ) {
                    matchingDistance = d;
                    matchingPlayer = otherPlayer;
                    }
                }
            }
        
        delete [] firstName;

        return matchingPlayer;
        }
        

    return NULL;
    }




static void findExpertForPlayer( LiveObject *inPlayer, 
                                 ObjectRecord *inTouchedObject ) {
    
    if( ! specialBiomesActive() ) {
        return;
        }

    int race = getSpecialistRace( inTouchedObject );
    

    char polylingual = false;
    if( getObject( inPlayer->displayID )->race  == race ) {
        // they ARE this expert themselves
        
        // point them toward polylingual race instead
        race = getPolylingualRace();
        polylingual = true;
        }

    
    if( race == -1 ) {
        return;
        }

    
    GridPos playerPos = getPlayerPos( inPlayer );

    double minDist = DBL_MAX;
    LiveObject *closestExpert = NULL;
    
    int chunkD = getMaxChunkDimension();
    

    // two passes
    // first pass, look for experts that are very close or standing in homelands
    // second pass, look for any expert
    for( int pass=0; pass<2; pass++ ) {

        for( int i=0; i<players.size(); i++ ) {
            LiveObject *p = players.getElement( i );
            
            if( getObject( p->displayID )->race == race ) {
                GridPos pos = getPlayerPos( p );            
                double d = distance( pos, playerPos );
                
                
                if( d < minDist ) {
                    if( pass == 1 || 
                        d < chunkD || 
                        isHomeland( pos.x, pos.y, p->lineageEveID ) == 1 ) {
                        
                        // player has no homeland or they are standing in their
                        // homeland
                        
                        // OR they are super-close to us
                        // (if there's a very nearby expert, don't send player
                        //  off to find homeland).
                        
                        // OR we're on second pass
                        // in that case, consider distant experts that
                        // are outside their homelands

                        minDist = d;
                        closestExpert = p;
                        }
                    }
                }
            }
        
        if( closestExpert != NULL ) {
            // found one on first pass
            break;
            }
        // else continue to second pass
        }

    const char *biomeName = getBadBiomeName( inTouchedObject );
    
    char *bName = NULL;
    
    if( polylingual ) {
        if( bName != NULL ) {
            delete [] bName;
            }
        bName = stringDuplicate( "OTHER LANGUAGES" );
        }
    else if( biomeName != NULL ) {
        char found;
        bName = replaceAll( biomeName, "_", " ", &found );
        }
    else {
        bName = stringDuplicate( "UNKNOWN BIOME" );
        }

    char *message = NULL;
    
    if( closestExpert != NULL ) {
        GridPos ePos = getPlayerPos( closestExpert );
        
        int eID = closestExpert->id;

        const char *phrase = "EXPERT";
        
        if( minDist > chunkD &&
            isHomeland( ePos.x, ePos.y, closestExpert->lineageEveID ) == 1 ) {
            // expert is far away.
            // they will probably move by the time we get to them
            
            // point to center of homeland instead, if we have one
            GridPos homeCenter;
            int homeEve = 0;

            if( getHomelandCenter( ePos.x, ePos.y, &homeCenter, &homeEve ) ) {
                
                if( homeEve == closestExpert->lineageEveID ) {
                    // leave 0 for person ID
                    // because we lead them to well location instead of person
                    eID = 0;
                    ePos = homeCenter;
                    phrase = "EXPERT VILLAGE";
                    }
                }
            }
        
        
        message = autoSprintf( "PS\n"
                               "%d/0 %s FOR %s "
                               "*expert %d *map %d %d\n#",
                               inPlayer->id,
                               phrase,
                               bName,
                               eID,
                               ePos.x - inPlayer->birthPos.x,
                               ePos.y - inPlayer->birthPos.y );
        }
    else {
        message = autoSprintf( "PS\n"
                               "%d/0 NO EXPERTS EXIST FOR %s\n#",
                               inPlayer->id,
                               bName );
        }
    
    delete [] bName;
    

    sendMessageToPlayer( inPlayer, message, strlen( message ) );
    delete [] message;
    }




// if inAll, generates info for all players, and doesn't touch 
//           followingUpdate flags
// returns NULL if no following message
static unsigned char *getFollowingMessage( char inAll, int *outLength ) {
    unsigned char *followingMessage = NULL;
    int followingMessageLength = 0;
        
    SimpleVector<char> followingWorking;
    followingWorking.appendElementString( "FW\n" );
            
    int numAdded = 0;
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        if( nextPlayer->error ) {
            continue;
            }
        
        if( nextPlayer->followingUpdate || inAll ) {

            int colorIndex = -1;
            
            if( nextPlayer->followingID > 0 ) {
                LiveObject *l = getLiveObject( nextPlayer->followingID );
                
                if( l != NULL ) {
                    colorIndex = l->leadingColorIndex;
                    }
                }

            char *line = autoSprintf( "%d %d %d\n", 
                                      nextPlayer->id,
                                      nextPlayer->followingID,
                                      colorIndex );
                
            followingWorking.appendElementString( line );
            delete [] line;
            numAdded++;

            if( ! inAll ) {
                nextPlayer->followingUpdate = false;
                }
            }
        }
            
    if( numAdded > 0 ) {
        followingWorking.push_back( '#' );
            
        if( numAdded > 0 ) {

            char *followingMessageText = 
                followingWorking.getElementString();
                
            followingMessageLength = strlen( followingMessageText );
                
            if( followingMessageLength < maxUncompressedSize ) {
                followingMessage = (unsigned char*)followingMessageText;
                }
            else {
                // compress for all players once here
                followingMessage = makeCompressedMessage( 
                    followingMessageText, 
                    followingMessageLength, &followingMessageLength );
                    
                delete [] followingMessageText;
                }
            }
        }

    *outLength = followingMessageLength;
    return followingMessage;
    }



// if inAll, generates info for all players, and doesn't touch exileUpdate flags
// returns NULL if no exile message
static unsigned char *getExileMessage( char inAll, int *outLength ) {
    unsigned char *exileMessage = NULL;
    int exileMessageLength = 0;
    

    SimpleVector<char> exileWorking;
    exileWorking.appendElementString( "EX\n" );
    
    int numAdded = 0;
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        if( nextPlayer->error ) {
            continue;
            }
        if( nextPlayer->exileUpdate || inAll ) {

            if( nextPlayer->exiledByIDs.size() > 0 ||
                ( !inAll && nextPlayer->exileUpdate ) ) {
                // send preface line for this player
                // they have some lines coming OR we have a force-update
                // for a player with no exile status (client-side list should
                // be cleared)
                char *line = autoSprintf( "%d -1\n", nextPlayer->id  );
                
                exileWorking.appendElementString( line );
                delete [] line;
                numAdded++;
                }
            
            for( int e=0; e< nextPlayer->exiledByIDs.size(); e++ ) {
                
                char *line = autoSprintf( 
                    "%d %d\n", 
                    nextPlayer->id,
                    nextPlayer->exiledByIDs.getElementDirect( e ) );
                
                exileWorking.appendElementString( line );
                delete [] line;
                numAdded++;
                }

            if( ! inAll ) {
                nextPlayer->exileUpdate = false;
                }
            }
        }
    
    if( numAdded > 0 ) {
        exileWorking.push_back( '#' );
        
        if( numAdded > 0 ) {
            
            char *exileMessageText = 
                exileWorking.getElementString();
            
            exileMessageLength = strlen( exileMessageText );
            
            if( exileMessageLength < maxUncompressedSize ) {
                exileMessage = (unsigned char*)exileMessageText;
                }
            else {
                // compress for all players once here
                exileMessage = makeCompressedMessage( 
                    exileMessageText, 
                    exileMessageLength, &exileMessageLength );
                
                delete [] exileMessageText;
                }
            }
        }

    *outLength = exileMessageLength;
    return exileMessage;
    }



// Recursively walks up leader tree to find out if inLeader is a leader
static char isFollower( LiveObject *inLeader, LiveObject *inTestFollower ) {
    int nextID = inTestFollower->followingID;
    
    if( nextID > 0 ) {
        if( nextID == inLeader->id ) {
            return true;
            }

        LiveObject *next = getLiveObject( nextID );
        
        if( next == NULL ) {
            return false;
            }
        return isFollower( inLeader, next );
        }
    return false;
    }
    





// any followers switch to following the leader of this leader
// exiles are passed down to followers
static void leaderDied( LiveObject *inLeader ) {
    char *leaderName = getLeadershipName( inLeader );
    


    SimpleVector<LiveObject*> oldFollowers;
    SimpleVector<LiveObject*> directFollowers;

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *otherPlayer = players.getElement( i );
        if( otherPlayer != inLeader &&
            ! otherPlayer->error ) {
        
            if( isFollower( inLeader, otherPlayer ) ) {
                oldFollowers.push_back( otherPlayer );
                }
            if( otherPlayer->followingID == inLeader->id ) {
                directFollowers.push_back( otherPlayer );
                }
            }
        }


    // somewhere else in code, we have a condition that sometimes
    // allows someone to keep following a dead and removed player
    // this condition can cause a crash when we go to look up that leader's
    // name
    // 
    // check for that here and correct for it, so we don't pass that stale
    // leader on to others (or cause a crash by looking up their name).
    if( inLeader->followingID != -1 ) {
        LiveObject *higherLeader = getLiveObject( inLeader->followingID );
        
        if( higherLeader == NULL ||
            higherLeader->error ) {
            
            inLeader->followingID = -1;
            }
        }


    // if leader is following no one (they haven't picked an heir to take over)
    // have them follow their fittest direct follower now automatically
    // ignore followers that this leader sees as exiled
    // (unless we have no choice, and they're all exiled)

    if( inLeader->followingID == -1 &&
        directFollowers.size() > 0 ) {
        
        LiveObject *fittestFollower = directFollowers.getElementDirect( 0 );
        // -1, because lowest possible score is 0
        // we will find a non-exiled follower this way
        double fittestFitness = -1;
        
        for( int i=0; i<directFollowers.size(); i++ ) {
            LiveObject *otherPlayer = directFollowers.getElementDirect( i );
            
            if( otherPlayer->fitnessScore > fittestFitness &&
                ! isExiled( inLeader, otherPlayer ) ) {
                
                fittestFitness = otherPlayer->fitnessScore;
                fittestFollower = otherPlayer;
                }
            }
        
        // if all are exiled, then we find fittest follower
        if( fittestFitness == -1 ) {
            for( int i=0; i<directFollowers.size(); i++ ) {
                LiveObject *otherPlayer = directFollowers.getElementDirect( i );
            
                // ignore exile status this time
                if( otherPlayer->fitnessScore > fittestFitness ) {
                
                    fittestFitness = otherPlayer->fitnessScore;
                    fittestFollower = otherPlayer;
                    }
                }
            }
        

        inLeader->followingID = fittestFollower->id;
        
        // they become top of tree, following no one
        fittestFollower->followingID = -1;
        fittestFollower->followingUpdate = true;

        // inform them differently, instead of as part of
        // group of followers below
        oldFollowers.deleteElementEqualTo( fittestFollower );
        
        const char *pronoun = "HIS";
        
        if( getFemale( inLeader ) ) {
            pronoun = "HER";
            }
        
        char *message =
            autoSprintf( "YOUR %s HAS DIED.**"
                         "YOU HAVE INHERITED %s POSITION.",
                         leaderName, pronoun );
        
        sendGlobalMessage( message, fittestFollower );
        delete [] message;
        }



    // get list of people that this leader has exiled directly
    // and clear this leader off their exiled-by lists
    SimpleVector<LiveObject*> exiledByThisLeader;
    
    for( int i=0; i<players.size(); i++ ) {
        
        LiveObject *otherPlayer = players.getElement( i );
        
        if( otherPlayer != inLeader &&
            ! otherPlayer->error ) {
            
            int exileIndex = otherPlayer->
                exiledByIDs.getElementIndex( inLeader->id );
            
            if( exileIndex != -1 ) {
                
                // we have this other exiled
                exiledByThisLeader.push_back( otherPlayer );
                
                // take ourselves off their list, we're dead
                otherPlayer->exiledByIDs.deleteElement( exileIndex );
                otherPlayer->exileUpdate = true;
                }
            }
        }


    
        
    for( int i=0; i<players.size(); i++ ) {
        
        LiveObject *otherPlayer = players.getElement( i );
        
        if( otherPlayer != inLeader &&
            ! otherPlayer->error ) {
            
            if( otherPlayer->followingID == inLeader->id ) {
                // they were following us

                
                // now they follow our leader
                // (or no leader, if we had none)
                otherPlayer->followingID = inLeader->followingID;
                otherPlayer->followingUpdate = true;
                
                int oID = otherPlayer->id;
                
                // have them exile whoever we were exiling
                for( int e=0; e<exiledByThisLeader.size(); e++ ) {
                    LiveObject *eO = 
                        exiledByThisLeader.getElementDirect( e );
                    
                    if( eO != NULL &&
                        // never have them exile themselves
                        eO->id != oID &&
                        eO->exiledByIDs.getElementIndex( oID ) == -1 ) {
                        // this follower is not already exiling this person
                        eO->exiledByIDs.push_back( oID );
                        eO->exileUpdate = true;
                        }
                    }
                } 
            }
        }



    if( leaderName == NULL ) {
        // no followers to inform
        return;
        }


    char *newLeaderExplain = NULL;
    LiveObject *newLeaderO = NULL;
    
    if( inLeader->followingID != -1 ) {
        newLeaderO = getLiveObject( inLeader->followingID );
        
        char *newLeaderName = getLeadershipName( newLeaderO );
        newLeaderExplain = autoSprintf( "YOU NOW FOLLOW YOUR %s.", 
                                        newLeaderName );
        delete [] newLeaderName;
        }

    
    if( newLeaderO != NULL ) {
        // have a new leader
        // before removeAllOwnership happens externally (which passes property
        // to children)
        // pass +leaderInherit owned stuff to this new leader now

        for( int i=0; i<inLeader->ownedPositions.size(); i++ ) {
            GridPos *p = inLeader->ownedPositions.getElement( i );

            int oID = getMapObject( p->x, p->y );
            
            if( oID <= 0 ) {
                continue;
                }

            ObjectRecord *o = getObject( oID );
            
            if( strstr( o->description, "+leaderInherit" ) != NULL ) {
                recentlyRemovedOwnerPos.push_back( *p );
                newOwnerPos.push_back( *p );
                newLeaderO->ownedPositions.push_back( *p );

                inLeader->ownedPositions.deleteElement( i );
                }
            }
        }
        


    // tell followers about our death
    for( int i=0; i<oldFollowers.size(); i++ ) {
        LiveObject *otherPlayer = oldFollowers.getElementDirect( i );
        
        
        char *secondLine;
        
        if( newLeaderExplain != NULL ) {
            secondLine = stringDuplicate( newLeaderExplain );
            }
        else {
            // no heir for this position.

            // who is their prime leader?
            int primeID = otherPlayer->followingID;
            while( primeID != -1 ) {
                LiveObject *primeO = getLiveObject( primeID );
                if( primeO != NULL ) {
                    if( primeO->followingID != -1 ) {
                        primeID = primeO->followingID;
                        }
                    else {
                        break;
                        }
                    }
                }
            newLeaderO = NULL;
            if( primeID != -1 ) {
                newLeaderO = getLiveObject( primeID );
                }
            if( newLeaderO != NULL ) {
                char *primeName = getLeadershipName( newLeaderO );
                secondLine = autoSprintf( "YOUR PRIME LEADER IS NOW YOUR %s.",
                                          primeName );
                delete [] primeName;
                }
            else {
                secondLine = autoSprintf( "YOU NOW HAVE NO LEADER." );
                }
                
            }

        char *message =
            autoSprintf( "YOUR %s HAS DIED.**"
                         "%s",
                         leaderName,
                         secondLine );
        
        delete [] secondLine;
                                
        sendGlobalMessage( message, otherPlayer );
        delete [] message;
        

        if( newLeaderO != NULL ) {
            // give them an arrow to their new leader
            char *newLeaderName = getLeadershipName( newLeaderO );
            
            GridPos lPos = getPlayerPos( newLeaderO );

            char *psMessage = 
                autoSprintf( "PS\n"
                             "%d/0 MY %s "
                             "*leader %d *map %d %d\n#",
                             otherPlayer->id,
                             newLeaderName,
                             newLeaderO->id,
                             lPos.x - otherPlayer->birthPos.x,
                             lPos.y - otherPlayer->birthPos.y );
            
            delete [] newLeaderName;
            
            sendMessageToPlayer( otherPlayer, psMessage, strlen( psMessage ) );
            delete [] psMessage;
            }
        }

    
    delete [] newLeaderExplain;
    delete [] leaderName;
    }




static LiveObject *getClosestFollower( LiveObject *inLeader ) {
    GridPos leaderPos = getPlayerPos( inLeader );
    
    double minDist = DBL_MAX;
    LiveObject *closestFollower = NULL;
    
    for( int i=0; i<players.size(); i++ ) {
        
        LiveObject *otherPlayer = players.getElement( i );
        
        if( otherPlayer != inLeader &&
            ! otherPlayer->error ) {
            
            if( isFollower( inLeader, otherPlayer ) &&
                ! isExiled( inLeader, otherPlayer ) ) {
                
                GridPos fPos = getPlayerPos( otherPlayer );
                
                double d = distance( leaderPos, fPos );
                
                if( d < minDist ) {
                    minDist = d;
                    closestFollower = otherPlayer;
                    }
                }
            }
        }
    return closestFollower;
    }




static void tryToStartKill( LiveObject *nextPlayer, int inTargetID,
                            SimpleVector<int> *playerIndicesToSendUpdatesAbout,
                            char inInfiniteRange = false ) {
    if( inTargetID > 0 && 
        nextPlayer->holdingID > 0 ) {
                            
        ObjectRecord *heldObj = 
            getObject( nextPlayer->holdingID );
                            
                            
        if( heldObj->deadlyDistance > 0 ) {
            
            // player transitioning into kill state?
                            
            LiveObject *targetPlayer =
                getLiveObject( inTargetID );
                            
            if( targetPlayer != NULL ) {
                                    
                // block intra-family kills with
                // otherFamilyOnly weapons
                char weaponBlocked = false;
                                    
                if( strstr( heldObj->description,
                            "otherFamilyOnly" ) ) {
                    // make sure victim is in
                    // different family
                    // AND that there's no peace treaty
                    if( targetPlayer->lineageEveID ==
                        nextPlayer->lineageEveID
                        ||
                        isPeaceTreaty( 
                            targetPlayer->lineageEveID,
                            nextPlayer->lineageEveID )
                        ||
                        ! isWarState( 
                            targetPlayer->lineageEveID,
                            nextPlayer->lineageEveID ) ) {
                                            
                        weaponBlocked = true;
                        }
                    }
                                    
                if( ! weaponBlocked  &&
                    ! isAlreadyInKillState( nextPlayer ) &&
                    canPlayerUseOrLearnTool( nextPlayer,
                                             nextPlayer->holdingID ) ) {
                    // they aren't already in one
                    // and they can learn the weapon they're holding
                                        
                    removeAnyKillState( nextPlayer );
                                        
                    char enteredState =
                        addKillState( nextPlayer,
                                      targetPlayer,
                                      inInfiniteRange );
                                        
                    if( enteredState && 
                        ! isNoWaitWeapon( 
                            nextPlayer->holdingID ) ) {
                                            
                        // no killer emote for no-wait
                        // weapons (these aren't
                        // actually weapons, like
                        // tattoo needles and snowballs)

                        nextPlayer->emotFrozen = true;
                        nextPlayer->emotFrozenIndex = 
                            killEmotionIndex;
                                            
                        newEmotPlayerIDs.push_back( 
                            nextPlayer->id );
                        newEmotIndices.push_back( 
                            killEmotionIndex );
                        newEmotTTLs.push_back( 120 );
                                            
                        if( ! targetPlayer->emotFrozen ) {
                            int minPosseSizeForKill = 0;
                            char fullForceSoloPosse = false;
                            
                            int posseSize = countPosseSize( 
                                targetPlayer,
                                &minPosseSizeForKill,
                                &fullForceSoloPosse );
                            
                            int emotIndex = victimEmotionIndex;
                            
                            if( posseSize >= minPosseSizeForKill ) {
                                emotIndex = victimTerrifiedEmotionIndex;
                                // force target player to drop what they are
                                // holding
                                // 
                                // but NOT for wilderness solo posses
                                // The point of solo posses is to tell someone
                                // to scram or else, not to force them to get
                                // off their horse.
                                if( fullForceSoloPosse || posseSize > 1 ) {
                                    tryToForceDropHeld( 
                                        targetPlayer, 
                                        playerIndicesToSendUpdatesAbout );
                                    }
                                }

                            targetPlayer->emotFrozen = true;
                            targetPlayer->emotFrozenIndex =
                                emotIndex;
                                                
                            newEmotPlayerIDs.push_back( 
                                targetPlayer->id );
                            newEmotIndices.push_back( 
                                emotIndex );
                            newEmotTTLs.push_back( 120 );
                            }
                        }
                    }
                }
            }
        }
    }



int getUnusedLeadershipColor() {
    // look for next unused

    int usedCounts[ NUM_BADGE_COLORS ];
    memset( usedCounts, 0, NUM_BADGE_COLORS * sizeof( int ) );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );

        if( o->leadingColorIndex != -1 ) {
            usedCounts[ o->leadingColorIndex ] ++;
            }
        }
    
    int minUsedCount = players.size();
    int minUsedIndex = -1;
    
    for( int c=0; c<NUM_BADGE_COLORS; c++ ) {
        if( usedCounts[c] < minUsedCount ) {
            minUsedCount = usedCounts[c];
            minUsedIndex = c;
            }
        }

    return minUsedIndex;
    }



#define NUM_LEADERSHIP_NAMES 8
static const char *
leadershipNames[NUM_LEADERSHIP_NAMES][2] = { { "LORD",
                                               "LADY" },
                                             { "BARON",
                                               "BARONESS" },
                                             { "COUNT",
                                               "COUNTESS" },
                                             { "DUKE",
                                               "DUCHESS" },
                                             { "KING",
                                               "QUEEN" },
                                             { "EMPEROR",
                                               "EMPRESS" },
                                             { "HIGH EMPEROR",
                                               "HIGH EMPRESS" },
                                             { "SUPREME EMPEROR",
                                               "SUPREME EMPRESS" } };


char *getLeadershipName( LiveObject *nextPlayer, 
                         char inNoName ) {
    
    int level = 0;
    

    SimpleVector<LiveObject *>possibleLeaders;
    possibleLeaders.push_back( nextPlayer );
    
    SimpleVector<LiveObject *>nextLeaders;

    while( possibleLeaders.size() > 0 ) {
        for( int p=0; p<possibleLeaders.size(); p++ ) {
            LiveObject *possibleL = possibleLeaders.getElementDirect( p );

            for( int i=0; i<players.size(); i++ ) {
                LiveObject *o = players.getElement( i );
            
                if( o->error ) {
                    continue;
                    }
                if( o->followingID == possibleL->id ) {
                    nextLeaders.push_back( o );
                    }
                }
            }
        possibleLeaders.deleteAll();
        
        if( nextLeaders.size() > 0 ) {
            level ++;
            
            possibleLeaders.push_back_other( &nextLeaders );
            nextLeaders.deleteAll();
            }
        }

    if( level == 0 ) {
        // no followers
        return NULL;
        }
    
    int gender = 0;
    if( getFemale( nextPlayer ) ) {
        gender = 1;
        }
    
    if( level > NUM_LEADERSHIP_NAMES ) {
        level = NUM_LEADERSHIP_NAMES;
        }
    
    int index = level - 1;
    const char *title = leadershipNames[index][gender];
    
    if( nextPlayer->name == NULL || inNoName ) {
        return stringDuplicate( title );
        }
    else {
        return autoSprintf( "%s %s", title, nextPlayer->name );
        } 
    }



static void replaceOrder( LiveObject *nextPlayer, char *inFormattedOrder,
                          int inOrderNumber, int inOriginatorID ) {
    if( nextPlayer->currentOrderNumber < inOrderNumber ) {
        nextPlayer->currentOrderNumber = inOrderNumber;
        nextPlayer->currentOrderOriginatorID = inOriginatorID;
        
        if( nextPlayer->currentOrder != NULL ) {
            delete [] nextPlayer->currentOrder;
            }
        nextPlayer->currentOrder = stringDuplicate( inFormattedOrder );
        }
    }




// speaker can be NULL sometimes
static char *translatePhraseFromSpeaker( char *inPhrase,
                                         LiveObject *speakerObj,
                                         LiveObject *listenerObj ) {
    char *trimmedPhrase = inPhrase;
    LiveObject *nextPlayer = listenerObj;
    
    
    // skip language filtering in some cases
    // VOG can talk to anyone
    // so can force spawns
    // also, skip in on very low pop servers
    // (just let everyone talk together)
    // also in case where speach is server-forced
    // sound representations (like [GASP])
    // but NOT for reading written words
    if( speakerObj == NULL ||
        nextPlayer->vogMode || 
        nextPlayer->forceSpawn || 
        ( speakerObj != NULL &&
          speakerObj->vogMode ) ||
        ( speakerObj != NULL &&
          speakerObj->forceSpawn ) ||
        players.size() < 
        minActivePlayersForLanguages ||
        strlen( trimmedPhrase ) == 0 ||
        trimmedPhrase[0] == '[' ||
        isPolylingual( nextPlayer->displayID ) ||
        ( speakerObj != NULL &&
          isPolylingual( 
              speakerObj->displayID ) ) ) {
        
        return stringDuplicate( trimmedPhrase );
        }
    else {
        int speakerDrunkenness = speakerObj->drunkenness;
        
        return mapLanguagePhrase( 
            trimmedPhrase,
            speakerObj->lineageEveID,
            nextPlayer->lineageEveID,
            speakerObj->id,
            nextPlayer->id,
            computeAge( speakerObj ),
            computeAge( nextPlayer ),
            speakerObj->parentID,
            nextPlayer->parentID,
            speakerDrunkenness / 10.0 );
        }
    }



static int orderDistance = 10;


static void checkOrderPropagation() {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->error ) {
            continue;
            }

        if( o->followingID != -1 ) {
            LiveObject *l = getLiveObject( o->followingID );
            
            if( l != NULL && l->currentOrder != NULL && 
                l->currentOrderNumber > o->currentOrderNumber) {
                
                // our leader has a new order that we don't have

                // are we close enough to them?
                
                GridPos ourPos = getPlayerPos( o );
                GridPos theirPos = getPlayerPos( l );
                                
                double d = distance( ourPos, theirPos );

                if( d <= orderDistance ) {

                    // close!

                    // make leader doesn't see us as exiled
                    
                    // are they, or any of their leaders, on our exile list?
                    
                    LiveObject *nextToCheck = l;
                    
                    char exiled = false;
                    while( nextToCheck != NULL ) {
                        if( o->exiledByIDs.getElementIndex( nextToCheck->id )
                            != -1 ) {
                            exiled = true;
                            break;
                            }
                        if( nextToCheck->followingID != -1 ) {
                            nextToCheck = 
                                getLiveObject( nextToCheck->followingID );
                            }
                        else {
                            nextToCheck = NULL;
                            }
                        }

                    // replace order even if exiled, so we don't have to keep
                    // checking them later
                    replaceOrder( o, l->currentOrder, 
                                  l->currentOrderNumber,
                                  l->currentOrderOriginatorID );

                    // but don't actually deliver message to them if exiled
                    if( ! exiled ) {
                        
                        // everything after first ** should be translated
                        
                        char *fullOrder = stringDuplicate( l->currentOrder );
                        
                        char *messageStart = strstr( fullOrder, "**" );
                        
                        if( messageStart != NULL ) {
                            // terminate here
                            messageStart[0] = '\0';
                            
                            messageStart = &( messageStart[2] );
                            
                            char *transOrder = translatePhraseFromSpeaker( 
                                messageStart,
                                getLiveObject( l->currentOrderOriginatorID ),
                                o );
                            
                            char *fullTransOrder = autoSprintf( "%s**%s",
                                                                fullOrder,
                                                                transOrder );
                            delete [] transOrder;
                            delete [] fullOrder;
                            fullOrder = fullTransOrder;
                            }
                                

                        sendGlobalMessage( fullOrder, o );
                        delete [] fullOrder;

                        LiveObject *leaderO = 
                            getLiveObject( l->currentOrderOriginatorID );
                        
                        if( leaderO != NULL ) {
                            // arrow to leader
                            GridPos leaderPos = getPlayerPos( leaderO );
                            
                            char *leadershipName = 
                                getLeadershipName( leaderO, true );
                            
                            char *message = 
                                autoSprintf( "PS\n"
                                             "%d/0 MY %s "
                                             "*leader %d *map %d %d\n#",
                                             o->id,
                                             leadershipName,
                                             l->currentOrderOriginatorID,
                                             leaderPos.x - o->birthPos.x,
                                             leaderPos.y - o->birthPos.y );
                            
                            delete [] leadershipName;

                            sendMessageToPlayer( o, message, 
                                                 strlen( message ) );
                            delete [] message;
                            }
                        }
                    }
                }
            } 
        }
    
    }






void makePlayerBiomeSick( LiveObject *nextPlayer, 
                          int sicknessObjectID ) {
    nextPlayer->holdingID = 
        sicknessObjectID;

    nextPlayer->holdingBiomeSickness = true;

    ForcedEffects e = 
        checkForForcedEffects( 
            nextPlayer->holdingID );
                            
    if( e.emotIndex != -1 ) {
        nextPlayer->emotFrozen = true;
        nextPlayer->emotFrozenIndex =
            e.emotIndex;
        newEmotPlayerIDs.push_back( 
            nextPlayer->id );
        newEmotIndices.push_back( 
            e.emotIndex );
        newEmotTTLs.push_back( e.ttlSec );
        interruptAnyKillEmots( 
            nextPlayer->id, e.ttlSec );
        }
    }




static void handleHeldDecay( 
    LiveObject *nextPlayer, int i,
    SimpleVector<int> *playerIndicesToSendUpdatesAbout,
    SimpleVector<int> *playerIndicesToSendHealingAbout ) {
    
    int oldID = nextPlayer->holdingID;
    
    TransRecord *t = getPTrans( -1, oldID );
    
    if( t != NULL ) {
        
        int newID = t->newTarget;
        
        handleHoldingChange( nextPlayer, newID );
        
        if( newID == 0 &&
            nextPlayer->holdingWound &&
            nextPlayer->dying ) {
            
            // wound decayed naturally, count as healed
            setNoLongerDying( 
                nextPlayer,
                playerIndicesToSendHealingAbout );            
            }
        
        
        nextPlayer->heldTransitionSourceID = -1;
        
        ObjectRecord *newObj = getObject( newID );
        ObjectRecord *oldObj = getObject( oldID );
        
        
        if( newObj != NULL && newObj->permanent &&
            oldObj != NULL && ! oldObj->permanent &&
            ! nextPlayer->holdingWound &&
            ! nextPlayer->holdingBiomeSickness ) {
            // object decayed into a permanent
            // force drop
            GridPos dropPos = 
                getPlayerPos( nextPlayer );
            
            handleDrop( 
                dropPos.x, dropPos.y, 
                nextPlayer,
                playerIndicesToSendUpdatesAbout );
            }
        
        
        playerIndicesToSendUpdatesAbout->push_back( i );
        }
    else {
        // no decay transition exists
        // clear it
        setFreshEtaDecayForHeld( nextPlayer );
        }
    }



// check if target has a 1-second
// decay specified
// if so, make it happen NOW and set in map
// return new target id
static int checkTargetInstantDecay( int inTarget, int inX, int inY ) {
    int newTarget = inTarget;
    
    TransRecord *targetDecay = getPTrans( -1, inTarget );
    
    // do NOT auto-apply movement transitions here
    // don't want result of move to repace this object in place, because
    // moving object might leave something behind, or require something to
    // land on in its destination (like a moving cart leaving one track
    // and landing on another)

    if( targetDecay != NULL &&
        targetDecay->autoDecaySeconds == 1  &&
        targetDecay->newTarget > 0 &&
        targetDecay->move == 0 ) {
                                            
        newTarget = targetDecay->newTarget;
                                            
        setMapObject( inX, inY, newTarget );
        }
    
    return newTarget;
    }


static void setRefuseFoodEmote( LiveObject *hitPlayer ) {
    if( hitPlayer->emotFrozen ) {
        return;
        }
    
    int newEmotIndex =
        SettingsManager::
        getIntSetting( 
            "refuseFoodEmotionIndex",
            -1 );
    if( newEmotIndex != -1 ) {
        newEmotPlayerIDs.push_back( 
            hitPlayer->id );
        
        newEmotIndices.push_back( 
            newEmotIndex );
        // 5 sec
        newEmotTTLs.push_back( 5 );
        }
    }




static void sendCraving( LiveObject *inPlayer ) {
    // they earn the normal YUM multiplier increase (+1) PLUS the bonus
    // increase, so send them the total.
    char *message = autoSprintf( "CR\n%d %d\n#", 
                                 inPlayer->cravingFood.foodID,
                                 inPlayer->cravingFoodYumIncrement + 1 );
    sendMessageToPlayer( inPlayer, message, strlen( message ) );
    delete [] message;

    inPlayer->cravingKnown = true;
    }




    // implement null versions of these to allow a headless build
    // we never call drawObject, but we need to use other objectBank functions


    void *getSprite( int ) {
	return NULL;
}

char *getSpriteTag( int ) {
	return NULL;
}

char isSpriteBankLoaded() {
	return false;
}

char markSpriteLive( int ) {
	return false;
}

void stepSpriteBank() {
}

void drawSprite( void*, doublePair, double, double, char ) {
}

void setDrawColor( float inR, float inG, float inB, float inA ) {
}

void setDrawColor( FloatColor inColor ) {
}

void setDrawFade( float ) {
}

float getTotalGlobalFade() {
	return 1.0f;
}

void toggleAdditiveTextureColoring( char inAdditive ) {
}

void toggleAdditiveBlend( char ) {
}

void drawSquare( doublePair, double ) {
}

void startAddingToStencil( char, char, float ) {
}

void startDrawingThroughStencil( char ) {
}

void stopStencil() {
}





// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {
	return NULL;
}

#include "../gameSource/soundBank.h"
void checkIfSoundStillNeeded( int inID ) {
}



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
	return false;
}


char getUsesMultiplicativeBlending( int inID ) {
	return false;
}



char getNoFlip( int inID ) {
	return false;
}



void toggleMultiplicativeBlend( char inMultiplicative ) {
}


void countLiveUse( SoundUsage inUsage ) {
}

void unCountLiveUse( SoundUsage inUsage ) {
}



// animation bank calls these only if lip sync hack is enabled, which
// it never is for server
void *loadSpriteBase( const char*, char ) {
	return NULL;
}

void freeSprite( void* ) {
}

void startOutputAllFrames() {
}

void stopOutputAllFrames() {
}


char realSpriteBank() {
	return false;
}

#ifndef ENABLE_TEST_MODE
#include "src/server/main.cpp"
#endif