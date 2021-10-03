/*
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
#include "minorGems/network/SocketPoll.h"
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
*/

#ifdef WIN_32
#include <windows.h>
BOOL WINAPI ctrlHandler( DWORD dwCtrlType )
{
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

/*
char *isCurseNamingSay( char *inSaidString );
char canPlayerUseTool( LiveObject *inPlayer, int inToolID );
void makePlayerBiomeSick( LiveObject *nextPlayer, int sicknessObjectID );
int getUnusedLeadershipColor();
char removeFromContainerToHold( LiveObject *inPlayer, int inContX, int inContY, int inSlotNumber );
char *getLeadershipName( LiveObject *nextPlayer, char inNoName = false );
void sendWarReportToAll();
void sendPeaceWarMessage( const char *inPeaceOrWar, char inWar, int inLineageAEveID, int inLineageBEveID );
double computeAge( LiveObject *inPlayer );
char isExiled( LiveObject *inViewer, LiveObject *inTarget );
LiveObject *getLiveObject( int inID );
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
*/

/*
extern SimpleVector<KillState> activeKillStates;
extern SimpleVector<int> newEmotTTLs;
extern SimpleVector<int> newEmotPlayerIDs;
extern SimpleVector<int> newEmotIndices;
*/

/*
#ifndef ENABLE_TEST_MODE
#include "src/server/main.cpp"
#endif
*/