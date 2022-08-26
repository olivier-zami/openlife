
//
// Created by olivier on 18/03/2022.
//

/*
 * Modification History
 *
 * 2010-September-3  Jason Rohrer
 * Fixed mouse to world translation function.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL.h>

#include "../../minorGems/graphics/openGL/ScreenGL.h"
#include "../../minorGems/graphics/Color.h"
#include "../../minorGems/graphics/openGL/gui/GUIComponentGL.h"
#include "../../minorGems/network/web/WebRequest.h"
#include "../../minorGems/graphics/openGL/glInclude.h"
#include "../../minorGems/graphics/openGL/MouseHandlerGL.h"
#include "../../minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "../../minorGems/ui/event/ActionListener.h"
#include "../../minorGems/system/Time.h"
#include "../../minorGems/system/Thread.h"
#include "../../minorGems/io/file/File.h"
#include "../../minorGems/network/HostAddress.h"
#include "../../minorGems/network/Socket.h"
#include "../../minorGems/network/SocketClient.h"
#include "../../minorGems/network/upnp/portMapping.h"
#include "../../minorGems/util/SettingsManager.h"
#include "../../minorGems/util/TranslationManager.h"
#include "../../minorGems/util/stringUtils.h"
#include "../../minorGems/util/SimpleVector.h"
#include "../../minorGems/util/log/AppLog.h"
#include "../../minorGems/util/log/FileLog.h"
#include "../../minorGems/graphics/converters/TGAImageConverter.h"
#include "../../minorGems/io/file/FileInputStream.h"
#include "../../minorGems/util/ByteBufferInputStream.h"
#include "../../minorGems/sound/formats/aiff.h"
#include "../../minorGems/sound/audioNoClip.h"
#include "../../minorGems/game/game.h"
#include "../../minorGems/game/gameGraphics.h"
#include "../../minorGems/game/drawUtils.h"
#include "../../minorGems/game/diffBundle/client/diffBundleClient.h"
#include "../../minorGems/util/random/CustomRandomSource.h"

unsigned char lastKeyPressed = '\0';
int lastMouseX = 0;
int lastMouseY = 0;
int lastMouseDownX = 0;
int lastMouseDownY = 0;
char ignoreNextMouseEvent = false;
int xCoordToIgnore, yCoordToIgnore;
static double soundSpriteVolumeMax = 1.0;
static double soundSpriteVolumeMin = 1.0;
float soundLoudness = 1.0f;
float currentSoundLoudness = 0.0f;
float soundSpriteGlobalLoudness = 1.0f;
char soundSpritesFading = false;
float soundSpriteFadeIncrementPerSample = 0.0f;
double maxTotalSoundSpriteVolume = 1.0;
double soundSpriteCompressionFraction = 0.0;
double totalSoundSpriteNormalizeFactor = 1.0;
NoClip soundSpriteNoClip;

// need to track these separately from SDL_GetModState so that
// we replay isCommandKeyDown properly during recorded game playback
char rCtrlDown = false;
char lCtrlDown = false;
char rAltDown = false;
char lAltDown = false;
char rMetaDown = false;
char lMetaDown = false;
char lShiftDown = false;
char rShiftDown = false;

// static seed
static CustomRandomSource randSource( 34957197 );

static int nextAsyncFileHandle = 0;

typedef struct AsyncFileRecord {
	int handle;
	char *filePath;

	int dataLength;
	unsigned char *data;

	char doneReading;

} AsyncFileRecord;

#include "../../minorGems/system/StopSignalThread.h"
#include "../../minorGems/system/MutexLock.h"
#include "../../minorGems/system/BinarySemaphore.h"

static MutexLock asyncLock;
static BinarySemaphore newFileToReadSem;
static BinarySemaphore newFileDoneReadingSem;
static SimpleVector<AsyncFileRecord> asyncFiles;

class AsyncFileThread : public StopSignalThread {

public:

	AsyncFileThread() {
		start();
	}

	~AsyncFileThread() {
		stop();
		newFileToReadSem.signal();
		join();

		for( int i=0; i<asyncFiles.size(); i++ ) {

			AsyncFileRecord *r = asyncFiles.getElement( i );

			if( r->filePath != NULL ) {
				delete [] r->filePath;
			}

			if( r->data != NULL ) {
				delete [] r->data;
			}
		}
		asyncFiles.deleteAll();
	}


	virtual void run() {
		while( ! isStopped() ) {

			int handleToRead = -1;
			char *pathToRead = NULL;

			asyncLock.lock();

			for( int i=0; i<asyncFiles.size(); i++ ) {
				AsyncFileRecord *r = asyncFiles.getElement( i );

				if( ! r->doneReading ) {
					// can't trust pointer to record in vector
					// outside of lock, because vector storage may
					// change
					handleToRead = r->handle;
					pathToRead = r->filePath;
					break;
				}
			}

			asyncLock.unlock();

			if( handleToRead != -1 ) {
				// read file data

				File f( NULL, pathToRead );

				int dataLength;
				unsigned char *data = f.readFileContents( &dataLength );

				// re-lock vector, search for handle, and add it
				// cannot count on vector order or pointers to records
				// when we don't have it locked

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( r->handle == handleToRead ) {
						r->dataLength = dataLength;
						r->data = data;
						r->doneReading = true;
						break;
					}
				}

				asyncLock.unlock();

				// let anyone waiting for a new file to finish
				// reading (only matters in the case of playback, where
				// the file-done must happen on a specific frame)
				newFileDoneReadingSem.signal();
			}
			else {
				// wait on binary semaphore until something else added
				// for us to read
				newFileToReadSem.wait();
			}
		}
	};
};

static AsyncFileThread fileReadThread;
int cursorMode = 0;
double emulatedCursorScale = 1.0;
int gameWidth = 320;// size of game image
int gameHeight = 240;// size of game image
int screenWidth = 640;// size of screen for fullscreen mode
int screenHeight = 480;// size of screen for fullscreen mode
int soundSampleRate = 22050;//int soundSampleRate = 44100;
char soundRunning = false;
char hardToQuitMode = false;
char demoMode = false;
char writeFailed = false;
char frameDrawerInited = false;
char outputAllFrames = false;// should each and every frame be saved to disk?// useful for making videos
char blendOutputFramePairs = false;// should output only every other frame, and blend in dropped frames?
float blendOutputFrameFraction = 0;
char *webProxy = NULL;
unsigned char *lastFrame_rgbaBytes = NULL;
char recordAudio = false;
FILE *aiffOutFile = NULL;
char measureFrameRate = true;
char startMeasureTimeRecorded = false;
static double noWarningSecondsToMeasure = 1;// show measure screen longer if there's a vsync warning
static double secondsToMeasure = noWarningSecondsToMeasure;
char mouseWorldCoordinates = true;
char shouldTakeScreenshot = false;// should screenshot be taken at end of next redraw?
char manualScreenShot = false;
char *screenShotPrefix = NULL;
ScreenGL *screen;
int pixelZoomFactor;// how many pixels wide is each game pixel drawn as?

typedef struct WebRequestRecord {
	int handle;
	WebRequest *request;
} WebRequestRecord;

SimpleVector<WebRequestRecord> webRequestRecords;

typedef struct SocketConnectionRecord {
	int handle;
	Socket *sock;
} SocketConnectionRecord;

SimpleVector<SocketConnectionRecord> socketConnectionRecords;

void getScreenDimensions( int *outWidth, int *outHeight ) {
	*outWidth = screenWidth;
	*outHeight = screenHeight;
}

typedef struct SoundSprite {
	int handle;
	int numSamples;
	int samplesPlayed;

	// true for sound sprites that are marked to never use
	// pitch and volume variance
	char noVariance;

	// floating point position of next interpolated sameple to play
	// (for variable rate playback)
	double samplesPlayedF;


	Sint16 *samples;
} SoundSprite;

// don't worry about dynamic mallocs here
// because we don't need to lock audio thread before adding values
// (playing sound sprites is handled directly through SoundSprite* handles
//  without accessing this vector).
SimpleVector<SoundSprite*> soundSprites;

// audio thread is locked every time we touch this vector
// So, we want to avoid mallocs here
// Can we imagine more than 100 sound sprites ever playing at the same time?
SimpleVector<SoundSprite> playingSoundSprites( 100 );
double *soundSpriteMixingBufferL = NULL;
double *soundSpriteMixingBufferR = NULL;

// variable rate per sprite
// these are also touched with audio thread locked, so avoid mallocs
// we won't see more than 100 of these simultaneously either
SimpleVector<double> playingSoundSpriteRates( 100 );

extern SimpleVector<double> playingSoundSpriteVolumesR;
extern SimpleVector<double> playingSoundSpriteVolumesL;

static int nextSoundSpriteHandle;
static double soundSpriteRateMax = 1.0;
static double soundSpriteRateMin = 1.0;



void setSoundSpriteRateRange( double inMin, double inMax ) {
	soundSpriteRateMin = inMin;
	soundSpriteRateMax = inMax;
}

void setSoundSpriteVolumeRange( double inMin, double inMax ) {
	soundSpriteVolumeMin = inMin;
	soundSpriteVolumeMax = inMax;
}

void setSoundLoudness( float inLoudness ) {
	lockAudio();
	soundLoudness = inLoudness;
	currentSoundLoudness = inLoudness;
	unlockAudio();
}

void fadeSoundSprites( double inFadeSeconds ) {
	lockAudio();
	soundSpritesFading = true;

	soundSpriteFadeIncrementPerSample =
			1.0f / ( inFadeSeconds * soundSampleRate );

	unlockAudio();
}

void resumePlayingSoundSprites() {
	lockAudio();
	soundSpritesFading = false;
	soundSpriteGlobalLoudness = 1.0f;
	unlockAudio();
}

SoundSpriteHandle loadSoundSprite( const char *inAIFFFileName ) {
	return loadSoundSprite( "sounds", inAIFFFileName );
}

SoundSpriteHandle loadSoundSprite( const char *inFolderName,
								   const char *inAIFFFileName ) {

	File aiffFile( new Path( inFolderName ), inAIFFFileName );

	if( ! aiffFile.exists() ) {
		printf( "File does not exist in sounds folder: %s\n",
				inAIFFFileName );
		return NULL;
	}

	int numBytes;
	unsigned char *data = aiffFile.readFileContents( &numBytes );


	if( data == NULL ) {
		printf( "Failed to read sound file: %s\n", inAIFFFileName );
		return NULL;
	}


	int numSamples;
	int16_t *samples = readMono16AIFFData( data, numBytes, &numSamples );

	delete [] data;

	if( samples == NULL ) {
		printf( "Failed to parse AIFF sound file: %s\n", inAIFFFileName );
		return NULL;
	}

	SoundSpriteHandle s = setSoundSprite( samples, numSamples );

	delete [] samples;

	return s;
}

SoundSpriteHandle setSoundSprite( int16_t *inSamples, int inNumSamples ) {
	SoundSprite *s = new SoundSprite;

	s->handle = nextSoundSpriteHandle ++;
	s->numSamples = inNumSamples;

	s->noVariance = false;

	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;

	s->samples = new Sint16[ s->numSamples ];

	memcpy( s->samples, inSamples, inNumSamples * sizeof( int16_t ) );

	soundSprites.push_back( s );

	return (SoundSpriteHandle)s;
}

void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance ) {
	SoundSprite *s = (SoundSprite*)inHandle;
	s->noVariance = inNoVariance;
}

void setMaxTotalSoundSpriteVolume( double inMaxTotal,
								   double inCompressionFraction ) {
	lockAudio();

	maxTotalSoundSpriteVolume = inMaxTotal;
	soundSpriteCompressionFraction = inCompressionFraction;

	totalSoundSpriteNormalizeFactor =
			1.0 / ( 1.0 - soundSpriteCompressionFraction );

	soundSpriteNoClip =
			resetAudioNoClip( ( 1.0 - soundSpriteCompressionFraction ) *
							  maxTotalSoundSpriteVolume * 32767,
					// half second hold and release
							  soundSampleRate / 2, soundSampleRate / 2 );

	unlockAudio();
}


static int maxSimultaneousSoundSprites = -1;


void setMaxSimultaneousSoundSprites( int inMaxCount ) {
	maxSimultaneousSoundSprites = inMaxCount;
}

static double pickRandomRate() {
	if( soundSpriteRateMax != 1.0 ||
		soundSpriteRateMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteRateMin,
												  soundSpriteRateMax );
	}
	else {
		return 1.0;
	}
}

static double pickRandomVolume() {
	if( soundSpriteVolumeMax != 1.0 ||
		soundSpriteVolumeMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteVolumeMin,
												  soundSpriteVolumeMax );
	}
	else {
		return 1.0;
	}
}

// no locking
static void playSoundSpriteInternal(
		SoundSpriteHandle inHandle, double inVolumeTweak,
		double inStereoPosition,
		double inForceVolume = -1,
		double inForceRate = -1 ) {


	if( soundSpritesFading && soundSpriteGlobalLoudness == 0.0f ) {
		// don't play any new sound sprites
		return;
	}

	if( maxSimultaneousSoundSprites != -1 &&
		playingSoundSpriteVolumesR.size() >= maxSimultaneousSoundSprites ) {
		// cap would be exceeded
		// don't play this sound sprite at all
		return;
	}


	double volume = inVolumeTweak;

	SoundSprite *s = (SoundSprite*)inHandle;

	if( ! s->noVariance ) {

		if( inForceVolume == -1 ) {
			volume *= pickRandomVolume();
		}
		else {
			volume *= inForceVolume;
		}
	}






	// constant power rule
	double p = M_PI * inStereoPosition * 0.5;

	double rightVolume = volume * sin( p );
	double leftVolume = volume * cos( p );


	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;




	playingSoundSprites.push_back( *s );

	if( s->noVariance ) {
		playingSoundSpriteRates.push_back( 1.0 );
	}
	else {

		if( inForceRate != -1 ) {
			playingSoundSpriteRates.push_back( inForceRate );
		}
		else {
			playingSoundSpriteRates.push_back(  pickRandomRate() );
		}
	}



	playingSoundSpriteVolumesR.push_back( rightVolume );
	playingSoundSpriteVolumesL.push_back( leftVolume );
}

// locking
void playSoundSprite( SoundSpriteHandle inHandle, double inVolumeTweak,
					  double inStereoPosition ) {

	lockAudio();
	playSoundSpriteInternal( inHandle, inVolumeTweak, inStereoPosition );
	unlockAudio();
}

// multiple with single lock
void playSoundSprite( int inNumSprites, SoundSpriteHandle *inHandles,
					  double *inVolumeTweaks,
					  double *inStereoPositions ) {
	lockAudio();

	// one random volume and rate for whole batch
	double volume = pickRandomVolume();
	double rate = pickRandomRate();

	for( int i=0; i<inNumSprites; i++ ) {
		playSoundSpriteInternal( inHandles[i], inVolumeTweaks[i],
								 inStereoPositions[i], volume, rate );
	}
	unlockAudio();
}

void freeSoundSprite( SoundSpriteHandle inHandle ) {
	// make sure this sprite isn't playing
	lockAudio();

	SoundSprite *s = (SoundSprite*)inHandle;

	// find it in vector to remove it
	for( int i=playingSoundSprites.size()-1; i>=0; i-- ) {
		SoundSprite *s2 = playingSoundSprites.getElement( i );
		if( s2->handle == s->handle ) {
			// stop it abruptly
			playingSoundSprites.deleteElement( i );
			playingSoundSpriteRates.deleteElement( i );
			playingSoundSpriteVolumesL.deleteElement( i );
			playingSoundSpriteVolumesR.deleteElement( i );
		}
	}

	unlockAudio();


	for( int i=0; i<soundSprites.size(); i++ ) {
		SoundSprite *s2 = soundSprites.getElementDirect( i );
		if( s2->handle == s->handle ) {
			delete [] s2->samples;
			soundSprites.deleteElement( i );
			delete s2;
		}
	}
}

int getSampleRate() {
	return soundSampleRate;
}

void setSoundPlaying( char inPlaying ) {
	SDL_PauseAudio( !inPlaying );
}

void lockAudio() {
	SDL_LockAudio();
}

void unlockAudio() {
	SDL_UnlockAudio();
}

char isSoundRunning() {
	return soundRunning;
}


#ifdef __mac__
#include <unistd.h>
#include <stdarg.h>

// returns path to folder
static char *pickFolder( const char *inPrompt ) {

    const char *commandFormat =
        "osascript -e 'tell application \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to return POSIX path of "
        "(choose folder with prompt \"%s\")'";

    char *command = autoSprintf( commandFormat, inPrompt );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for picking a folder." );
        }
    else {
        char buffer[200];

        int numRead = fscanf( osascriptPipe, "%199s", buffer );

        if( numRead == 1 ) {
            return stringDuplicate( buffer );
            }

        pclose( osascriptPipe );
        }
    return NULL;
    }



static void showMessage( const char *inAppName,
                         const char *inTitle, const char *inMessage,
                         char inError = false ) {
    const char *iconName = "note";
    if( inError ) {
        // stop icon is broken in OS 10.12
        // always use note
        // iconName = "stop";
        }

    const char *commandFormat =
        "osascript -e 'tell app \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to display dialog \"%s\" "
        "with title \"%s:  %s\" buttons \"Ok\" "
        "with icon %s default button \"Ok\"' ";

    char *command = autoSprintf( commandFormat, inMessage, inAppName, inTitle,
                                 iconName );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for displaying GUI messages." );
        }
    else {
        pclose( osascriptPipe );
        }
    }


static char *getPrefFilePath() {
    return autoSprintf( "%s/Library/Preferences/%s_prefs.txt",
                        getenv( "HOME" ),
                        getAppName() );
    }


static char isSettingsFolderFound() {

    File settingsFolder( NULL, "settings" );

    if( settingsFolder.exists() && settingsFolder.isDirectory() ) {
        return true;
        }

    return false;
    }
#endif

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

float viewCenterX = 0;
float viewCenterY = 0;
// default -1 to +1
float viewSize = 2;
float visibleWidth = -1;
float visibleHeight = -1;
extern int totalLoadedTextureBytes;

unsigned int getRandSeed() {
	return screen->getRandSeed();
}

// returns true if we're currently executing a recorded game
char isGamePlayingBack() {
	return screen->isPlayingBack();
}

void mapKey( unsigned char inFromKey, unsigned char inToKey ) {
	screen->setKeyMapping( inFromKey, inToKey );
}

void toggleKeyMapping( char inMappingOn ) {
	screen->toggleKeyMapping( inMappingOn );
}

doublePair getViewCenterPosition()
{
	doublePair p = { viewCenterX, viewCenterY };

	return p;
}

void setLetterbox( float inVisibleWidth, float inVisibleHeight ) {
	visibleWidth = inVisibleWidth;
	visibleHeight = inVisibleHeight;
}

void setCursorVisible( char inIsVisible ) {
	if( inIsVisible ) {
		SDL_ShowCursor( SDL_ENABLE );
	}
	else {
		SDL_ShowCursor( SDL_DISABLE );
	}
}

void setCursorMode( int inMode ) {
	SettingsManager::setSetting( "cursorMode", inMode );
	cursorMode = inMode;

	switch( cursorMode ) {
		case 0:
		case 2:
			setCursorVisible( true );
			break;
		case 1:
			setCursorVisible( false );
			break;
		default:
			setCursorVisible( true );
			break;
	}
}

int getCursorMode() {
	return cursorMode;
}

void setEmulatedCursorScale( double inScale ) {
	SettingsManager::setDoubleSetting( "emulatedCursorScale", inScale );
	emulatedCursorScale = inScale;
}

double getEmulatedCursorScale() {
	return emulatedCursorScale;
}

void grabInput( char inGrabOn ) {
	if( inGrabOn ) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
	}
}

void setMouseReportingMode( char inWorldCoordinates ) {
	mouseWorldCoordinates = inWorldCoordinates;
}

static void warpMouseToScreenPos( int inX, int inY ) {
	if( inX == lastMouseX && inY == lastMouseY ) {
		// mouse already there, no need to warp
		// (and warping when already there may or may not generate
		//  an event on some platforms, which causes trouble when we
		//  try to ignore the event)
		return;
	}

	if( SDL_GetAppState() & SDL_APPINPUTFOCUS ) {

		if( frameDrawerInited ) {
			// not ignoring mouse events currently due to demo code panel
			// or loading message... frame drawer not inited yet
			ignoreNextMouseEvent = true;
			xCoordToIgnore = inX;
			yCoordToIgnore = inY;
		}

		SDL_WarpMouse( inX, inY );
	}
}

void warpMouseToCenter( int *outNewMouseX, int *outNewMouseY ) {
	*outNewMouseX = screenWidth / 2;
	*outNewMouseY = screenHeight / 2;

	warpMouseToScreenPos( *outNewMouseX, *outNewMouseY );
}

void warpMouseToWorldPos( float inX, float inY ) {
	int worldX, worldY;
	worldToScreen( inX, inY, &worldX, &worldY );
	warpMouseToScreenPos( worldX, worldY );
}

void screenToWorld( int inX, int inY, float *outX, float *outY ) {

	if( mouseWorldCoordinates ) {

		// relative to center,
		// viewSize spreads out across screenWidth only (a square on screen)
		float x = (float)( inX - (screenWidth/2) ) / (float)screenWidth;
		float y = -(float)( inY - (screenHeight/2) ) / (float)screenWidth;

		*outX = x * viewSize + viewCenterX;
		*outY = y * viewSize + viewCenterY;
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}

}

void worldToScreen( float inX, float inY, int *outX, int *outY ) {
	if( mouseWorldCoordinates ) {
		// inverse of screenToWorld
		inX -= viewCenterX;
		inX /= viewSize;

		inX *= screenWidth;
		inX += screenWidth/2;

		*outX = round( inX );


		inY -= viewCenterY;
		inY /= viewSize;

		inY *= -screenWidth;
		inY += screenHeight/2;

		*outY = round( inY );
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}
}

void getLastMouseScreenPos( int *outX, int *outY ) {
	*outX = lastMouseX;
	*outY = lastMouseY;
}

char isCommandKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_CTRL )
		||
		( modState & KMOD_ALT )
		||
		( modState & KMOD_META ) ) {

		return true;
	}

	if( screen->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rCtrlDown || lCtrlDown ||
			rAltDown || lAltDown ||
			rMetaDown || lMetaDown ) {
			return true;
		}
	}

	return false;
}

char isShiftKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_SHIFT ) ) {

		return true;
	}

	if( screen->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rShiftDown || lShiftDown ) {
			return true;
		}
	}

	return false;
}

char isLastMouseButtonRight() {
	return screen->isLastMouseButtonRight();
}

// FOVMOD NOTE:  Change 1/1 - Take these lines during the merge process
int getLastMouseButton() {
	return screen->getLastMouseButton();
}

void obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead ) {

	screen->obscureRecordedNumericTyping( inObscure, inCharToRecordInstead );
}


static Image *readTGAFile( File *inFile ) {

	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );

	TGAImageConverter converter;

	Image *result = converter.deformatImage( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}

Image *readTGAFile( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFile( &tgaFile );
}

Image *readTGAFileBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFile( &tgaFile );
}

static RawRGBAImage *readTGAFileRaw( InputStream *inStream ) {
	TGAImageConverter converter;

	RawRGBAImage *result = converter.deformatImageRaw( inStream );


	return result;
}

static RawRGBAImage *readTGAFileRaw( File *inFile ) {

	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );


	RawRGBAImage *result = readTGAFileRaw( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}

RawRGBAImage *readTGAFileRaw( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}

RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}

RawRGBAImage *readTGAFileRawFromBuffer( unsigned char *inBuffer, int inLength ) {

	ByteBufferInputStream tgaStream( inBuffer, inLength );

	return readTGAFileRaw( &tgaStream );
}

void writeTGAFile( const char *inTGAFileName, Image *inImage ) {
	File tgaFile( NULL, inTGAFileName );
	FileOutputStream tgaStream( &tgaFile );

	TGAImageConverter converter;

	return converter.formatImage( inImage, &tgaStream );
}

SpriteHandle fillSprite( RawRGBAImage *inRawImage ) {
	if( inRawImage->mNumChannels != 4 ) {
		printf( "Sprite not a 4-channel image, "
				"failed to load.\n" );

		return NULL;
	}

	return fillSprite( inRawImage->mRGBABytes,
					   inRawImage->mWidth,
					   inRawImage->mHeight );
}

SpriteHandle loadSprite( const char *inTGAFileName,
						 char inTransparentLowerLeftCorner ) {

	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRaw( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from graphics/%s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFile( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
										  inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}

SpriteHandle loadSpriteBase( const char *inTGAFileName,
							 char inTransparentLowerLeftCorner ) {
	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRawBase( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from %s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFileBase( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
										  inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}

const char *translate( const char *inTranslationKey ) {
	return TranslationManager::translate( inTranslationKey );
}

Image **screenShotImageDest = NULL;

void saveScreenShot( const char *inPrefix, Image **outImage ) {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( inPrefix );
	shouldTakeScreenshot = true;
	manualScreenShot = true;

	screenShotImageDest = outImage;
}

void startOutputAllFrames() {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( "frame" );

	outputAllFrames = true;
	shouldTakeScreenshot = true;
}

void stopOutputAllFrames() {
	outputAllFrames = false;
	shouldTakeScreenshot = false;
}

int nextWebRequestHandle = 0;

int startWebRequest( const char *inMethod, const char *inURL,
					 const char *inBody ) {

	WebRequestRecord r;

	r.handle = nextWebRequestHandle;
	nextWebRequestHandle ++;


	if( screen->isPlayingBack() ) {
		// stop here, don't actually start a real web request
		return r.handle;
	}


	r.request = new WebRequest( inMethod, inURL, inBody, webProxy );

	webRequestRecords.push_back( r );

	return r.handle;
}

static WebRequest *getRequestByHandle( int inHandle ) {
	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->request;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getRequestByHandle:  "
				   "Requested WebRequest handle not found\n" );
	return NULL;
}

int stepWebRequest( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// don't step request, because we're only simulating the response
		// of the server

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return 1;
		}
		else if( nextType == 1 ) {
			// recording said our result was ready
			// but it may not be the actual next result, due to timing
			// keep processing results until we see an actual 2 in the recording
			return 0;
		}
		else {
			return nextType;
		}
	}


	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {

		int stepResult = r->step();

		screen->registerWebEvent( inHandle, stepResult );

		return stepResult;
	}

	return -1;
}

// gets the response body as a \0-terminated string, destroyed by caller
char *getWebResult( int inHandle ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return screen->getWebEventResultBody( inHandle );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		char *result = r->getResult();

		if( result != NULL ) {
			screen->registerWebEvent( inHandle,
					// the type for "result" is 2
									  2,
									  result );
		}

		return result;
	}

	return NULL;
}

unsigned char *getWebResult( int inHandle, int *outSize ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return (unsigned char *)
					screen->getWebEventResultBody( inHandle, outSize );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		unsigned char *result = r->getResult( outSize );

		if( result != NULL ) {
			screen->registerWebEvent( inHandle,
					// the type for "result" is 2
									  2,
									  (char*)result,
									  *outSize );
		}

		return result;
	}

	return NULL;
}

int getWebProgressSize( int inHandle ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType > 2 ) {
			return nextType;
		}
		else {
			AppLog::error(
					"Expecting a web result progress event in playback file, "
					"but found none." );

			return 0;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		int progress = r->getProgressSize();
		if( progress > 2 ) {
			screen->registerWebEvent( inHandle,
					// the type for "progress" is
					// the actual size
									  progress );
			return progress;
		}
		else {
			// progress of 2 or less is returned as 0, to keep consistency
			// for recording and playback
			return 0;
		}
	}

	return 0;
}

// frees resources associated with a web request
// if request is not complete, this cancels it
// if hostname lookup is not complete, this call might block.
void clearWebRequest( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real request, do nothing
		return;
	}


	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->request;

			webRequestRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - clearWebRequest:  "
				   "Requested WebRequest handle not found\n" );
}

int nextSocketConnectionHandle = 0;

int openSocketConnection( const char *inNumericalAddress, int inPort ) {
	SocketConnectionRecord r;

	r.handle = nextSocketConnectionHandle;
	nextSocketConnectionHandle++;


	if( screen->isPlayingBack() ) {
		// stop here, don't actually open a real socket
		return r.handle;
	}

	HostAddress address( stringDuplicate( inNumericalAddress ), inPort );


	char timedOut;

	// non-blocking connet
	r.sock = SocketClient::connectToServer( &address, 0, &timedOut );

	if( r.sock != NULL ) {
		socketConnectionRecords.push_back( r );

		return r.handle;
	}
	else {
		return -1;
	}
}

static Socket *getSocketByHandle( int inHandle ) {
	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->sock;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getSocketByHandle:  "
				   "Requested Socket handle not found\n" );
	return NULL;
}

// non-blocking send
// returns number sent (maybe 0) on success, -1 on error
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength ) {
	if( screen->isPlayingBack() ) {
		// play back result of this send

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		while( nextType == 2 && nextNumBodyBytes == 0 ) {
			// skip over any lingering waiting-for-read events
			// sometimes there are extra in recording that aren't needed
			// on playback for some reason
			screen->getSocketEventTypeAndSize( inHandle,
											   &nextType, &nextNumBodyBytes );
		}


		if( nextType == 0 ) {
			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numSent = 0;


		if( sock->isConnected() ) {

			numSent = sock->send( inData, inDataLength, false, false );

			if( numSent == -2 ) {
				// would block
				numSent = 0;
			}
		}

		int type = 0;

		if( numSent == -1 ) {
			type = 1;
			numSent = 0;
		}

		screen->registerSocketEvent( inHandle, type, numSent, NULL );

		return numSent;
	}

	return -1;
}

// non-blocking read
// returns number of bytes read (maybe 0), -1 on error
int readFromSocket( int inHandle,
					unsigned char *inDataBuffer, int inBytesToRead ) {

	if( screen->isPlayingBack() ) {
		// play back result of this read

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		if( nextType == 2 ) {

			if( nextNumBodyBytes == 0 ) {
				return 0;
			}
			// else there are body bytes waiting

			if( nextNumBodyBytes > inBytesToRead ) {
				AppLog::errorF( "gameSDL - readFromSocket:  "
								"Expecting to read at most %d bytes, but "
								"recording has %d bytes waiting\n",
								inBytesToRead, nextNumBodyBytes );
				return -1;
			}

			unsigned char *bodyBytes =
					screen->getSocketEventBodyBytes( inHandle );

			memcpy( inDataBuffer, bodyBytes, nextNumBodyBytes );
			delete [] bodyBytes;


			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numRead = 0;

		if( sock->isConnected() ) {

			numRead = sock->receive( inDataBuffer, inBytesToRead, 0 );

			if( numRead == -2 ) {
				// would block
				numRead = 0;
			}
		}

		int type = 2;
		if( numRead == -1 ) {
			type = 3;
		}

		unsigned char *bodyBytes = NULL;
		if( numRead > 0 ) {
			bodyBytes = inDataBuffer;
		}

		screen->registerSocketEvent( inHandle, type, numRead, bodyBytes );

		return numRead;
	}

	return -1;
}

void closeSocket( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real socket, do nothing
		return;
	}

	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->sock;

			socketConnectionRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - closeSocket:  "
				   "Requested Socket handle not found\n" );
}

int startAsyncFileRead( const char *inFilePath ) {

	int handle = nextAsyncFileHandle;
	nextAsyncFileHandle ++;

	AsyncFileRecord r = {
			handle,
			stringDuplicate( inFilePath ),
			-1,
			NULL,
			false };

	asyncLock.lock();
	asyncFiles.push_back( r );
	asyncLock.unlock();

	newFileToReadSem.signal();

	return handle;
}

char checkAsyncFileReadDone( int inHandle ) {

	char ready = false;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle &&
			r->doneReading ) {

			ready = true;
			break;
		}
	}
	asyncLock.unlock();


	if( screen->isPlayingBack() ) {
		char playbackSaysReady = screen->getAsyncFileDone( inHandle );

		if( ready && playbackSaysReady ) {
			return true;
		}
		else if( ready && !playbackSaysReady ) {
			return false;
		}
		else if( ! ready && playbackSaysReady ) {
			// need to return ready before end of this frame
			// so behavior matches recording behavior

			// wait for read to finish, synchronously
			while( !ready ) {
				newFileDoneReadingSem.wait();

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( r->handle == inHandle &&
						r->doneReading ) {

						ready = true;
						break;
					}
				}
				asyncLock.unlock();
			}

			return true;
		}
	}
	else {
		if( ready ) {
			screen->registerAsyncFileDone( inHandle );
		}
	}


	return ready;
}

unsigned char *getAsyncFileData( int inHandle, int *outDataLength ) {

	unsigned char *data = NULL;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle ) {

			data = r->data;
			*outDataLength = r->dataLength;

			if( r->filePath != NULL ) {
				delete [] r->filePath;
			}

			asyncFiles.deleteElement( i );
			break;
		}
	}
	asyncLock.unlock();

	return data;
}

timeSec_t game_timeSec() {
	return screen->getTimeSec();
}

double game_getCurrentTime() {
	return screen->getCurrentTime();
}

char isHardToQuitMode() {
	return hardToQuitMode;
}

// platform-specific clipboard code
#ifdef LINUX
static char clipboardSupportKnown = false;
static char clipboardSupport = false;
#endif

char isClipboardSupported() {
#ifdef LINUX
	// only check once, since system forks a process each time
    if( !clipboardSupportKnown ) {

        if( system( "which xclip > /dev/null 2>&1" ) ) {
            // xclip not installed
            AppLog::error( "xclip must be installed for clipboard to work" );
            clipboardSupport = false;
            }
        else {
            clipboardSupport = true;
            }
        clipboardSupportKnown = true;
        }
    return clipboardSupport;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}

char isURLLaunchSupported() {
#ifdef LINUX
	return true;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}

#ifdef LINUX
// X windows clipboard tutorial found here
// http://michael.toren.net/mirrors/doc/X-copy+paste.txt

// X11 has it's own Time type
// avoid conflicts with our Time class from above by replacing the word
// (Trick found here:  http://stackoverflow.com/questions/8865744 )
#define Time X11_Time

#include <X11/Xlib.h>
#include <X11/Xatom.h>


char *getClipboardText() {

    FILE* pipe = popen( "xclip -silent -selection clipboard -o", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    SimpleVector<char> textVector;

    char buffer[512];
    char *line = fgets( buffer, sizeof( buffer ), pipe );

    while( line != NULL ) {
        textVector.appendElementString( buffer );
        line = fgets( buffer, sizeof( buffer ), pipe );
        }

    pclose( pipe );


    return textVector.getElementString();
    }


void setClipboardText( const char *inText  ) {
    // x copy paste is a MESS
    // after claiming ownership of the clipboard, application needs
    // to listen to x events forever to handle any consumers of the clipboard
    // data.  Yuck!

    // farm this out to xclip with -silent flag
    // it forks its own process and keeps it live as long as the clipboard
    // data is still needed (kills itself when the clipboard is claimed
    // by someone else with new data)

    FILE* pipe = popen( "xclip -silent -selection clipboard -i", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }


void launchURL( char *inURL ) {
    char *call = autoSprintf( "xdg-open \"%s\" &", inURL );
    system( call );
    delete [] call;
    }
#elif defined(__mac__)
// pbpaste command line trick found here:
// http://www.alecjacobson.com/weblog/?p=2376

char *getClipboardText() {
    FILE* pipe = popen( "pbpaste", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    char buffer[ 128 ];

    char *result = stringDuplicate( "" );

    // read until pipe closed
    while( ! feof( pipe ) ) {
        if( fgets( buffer, 128, pipe ) != NULL ) {
            char *newResult = concatonate( result, buffer );
            delete [] result;
            result = newResult;
            }
        }
    pclose( pipe );


    return result;
    }



void setClipboardText( const char *inText  ) {
    FILE* pipe = popen( "pbcopy", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }



void launchURL( char *inURL ) {
    char *call = autoSprintf( "open \"%s\"", inURL );
    system( call );
    delete [] call;
    }
#elif defined(WIN_32)
// simple windows clipboard solution found here:
// https://www.allegro.cc/forums/thread/606034

#include <windows.h>

char *getClipboardText() {
    char *fromClipboard = NULL;
    if( OpenClipboard( NULL ) ) {
        HANDLE hData = GetClipboardData( CF_TEXT );
        char *buffer = (char*)GlobalLock( hData );
        if( buffer != NULL ) {
            fromClipboard = stringDuplicate( buffer );
            }
        GlobalUnlock( hData );
        CloseClipboard();
        }

    if( fromClipboard == NULL ) {
        fromClipboard = stringDuplicate( "" );
        }

    return fromClipboard;
    }


void setClipboardText( const char *inText  ) {
    if (OpenClipboard(NULL)) {

        EmptyClipboard();
        HGLOBAL clipBuffer = GlobalAlloc( GMEM_DDESHARE, strlen(inText) + 1 );
        char *buffer = (char*)GlobalLock( clipBuffer );

        strcpy( buffer, inText );
        GlobalUnlock( clipBuffer );
        SetClipboardData( CF_TEXT, clipBuffer );
        CloseClipboard();
        }
    }


void launchURL( char *inURL ) {
    // for some reason, on Windows, need extra set of "" before quoted URL
    // found here:
    // https://stackoverflow.com/questions/3037088/
    //         how-to-open-the-default-web-browser-in-windows-in-c

    // the wmic method allows spawning a browser without it lingering as
    // a child process
    // https://steamcommunity.com/groups/steamworks/
    //         discussions/0/154645427521397803/
    char *call = autoSprintf(
        "wmic process call create 'cmd /c start \"\" \"%s\"'", inURL );
    system( call );
    delete [] call;
    }
#else
// unsupported platform
char *getClipboardText() {
	return stringDuplicate( "" );
}

void launchURL( char *inURL ) {
}
#endif

#define macLaunchExtension ".app"
#define winLaunchExtension ".exe"

#define steamGateClientName "steamGateClient"

#ifdef LINUX
#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "./%s", getLinuxAppName() );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }

char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "./%s", steamGateClientName );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from launching steamGateClient\n" );
    return true;
    }
#elif defined(__mac__)
#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    // Gatekeeper on 10.12 prevents relaunch from working
    // to be safe, just have user manually relaunch on Mac
    return false;

    /*
    char *launchTarget =
        autoSprintf( "%s_$d%s", getAppName(),
                     getAppVersion(), macLaunchExtension );
    AppLog::infoF( "Relaunching game %s", launchTarget );
    int forkValue = fork();
    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[4] = { (char*)"open", (char*)"-n",
                               launchTarget, NULL };
        execvp( "open", arguments );
        // we'll never return from this call
        // small memory leak here, but okay
        delete [] launchTarget;
        }
    delete [] launchTarget;
    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    */
    }


char runSteamGateClient() {
    // have never tested this on Mac, who knows?
    return false;
    }
#elif defined(WIN_32)
#include <windows.h>
#include <process.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "%s%s", getAppName(), winLaunchExtension );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }



char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "%s%s", steamGateClientName, winLaunchExtension );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from running steamGateClient\n" );
    return true;
    }
#else
// unsupported platform
char relaunchGame() {
	return false;
}

char runSteamGateClient() {
	return false;
}
#endif

void quitGame() {
	exit( 0 );
}

// true if platform supports sound recording, false otherwise
char isSoundRecordingSupported() {
#ifdef LINUX
	// check for arecord existence
    // The redirect to /dev/null ensures that your program does not produce
    // the output of these commands.
    // found here:
    // http://stackoverflow.com/questions/7222674/
    //             how-to-check-if-command-is-available-or-existant
    //
    int ret = system( "arecord --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
#elif defined(__mac__)
	return false;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}

#ifdef LINUX
static FILE *arecordPipe = NULL;
const char *arecordFileName = "inputSoundTemp.wav";
static int arecordSampleRate = 0;

// starts recording asynchronously
// keeps recording until stop called
char startRecording16BitMonoSound( int inSampleRate ) {
    if( arecordPipe != NULL ) {
        pclose( arecordPipe );
        arecordPipe = NULL;
        }

    arecordSampleRate = inSampleRate;

    char *arecordLine =
        autoSprintf( "arecord -f S16_LE -c1 -r%d %s",
                     inSampleRate, arecordFileName );

    arecordPipe = popen( arecordLine, "w" );

    delete [] arecordLine;

    if( arecordPipe == NULL ) {
        return false;
        }
    else {
        return true;
        }
    }



// returns array of samples destroyed by caller
int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    if( arecordPipe == NULL ) {
        return NULL;
        }

    // kill arecord to end the recording gracefully
    // this is reasonable to do because I can't imagine situations
    // where more than one arecord is running
    system( "pkill arecord" );

    pclose( arecordPipe );
    arecordPipe = NULL;

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }
#elif defined(__mac__)
const char *arecordFileName = "inputSound.wav";

// mac implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {
    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    *outNumSamples = 0;
    return NULL;
    }
#elif defined(WIN_32)
#include <mmsystem.h>

const char *arecordFileName = "inputSound.wav";
static int arecordSampleRate = 0;

// windows implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {

    arecordSampleRate = inSampleRate;

    if( mciSendString( "open new type waveaudio alias my_sound",
                       NULL, 0, 0 ) == 0 ) {

        char *settingsString =
            autoSprintf( "set my_sound alignment 2 bitspersample 16"
                         " samplespersec %d"
                         " channels 1"
                         " bytespersec %d"
                         " time format milliseconds format tag pcm",
                         inSampleRate,
                         ( 16 * inSampleRate ) / 8 );

        mciSendString( settingsString, NULL, 0, 0 );

        delete [] settingsString;

        mciSendString( "record my_sound", NULL, 0, 0 );
        return true;
        }

    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    mciSendString( "stop my_sound", NULL, 0, 0 );

    char *saveCommand = autoSprintf( "save my_sound %s", arecordFileName );

    mciSendString( saveCommand, NULL, 0, 0 );
    delete [] saveCommand;

    mciSendString( "close my_sound", NULL, 0, 0 );

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }
#else

const char *arecordFileName = "inputSound.wav";

// default implementation does nothing
char startRecording16BitMonoSound( int inSampleRate ) {
	return false;
}

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
	return NULL;
}

#endif

// same for all platforms
// load a .wav file
int16_t *load16BitMonoSound( int *outNumSamples, int *outSampleRate ) {

	File wavFile( NULL, arecordFileName );

	if( ! wavFile.exists() ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "File does not exist in game folder: %s\n",
						arecordFileName );
		return NULL;
	}

	char *fileName = wavFile.getFullFileName();

	FILE *file = fopen( fileName, "rb" );

	delete [] fileName;

	if( file == NULL ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to open sound file for reading: %s\n",
						arecordFileName );
		return NULL;
	}

	fseek( file, 0L, SEEK_END );
	int fileSize  = ftell( file );
	rewind( file );

	if( fileSize <= 44 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file too small to contain a WAV header: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	// skip 20 bytes of header to get to format flag
	fseek( file, 20, SEEK_SET );

	unsigned char readBuffer[4];

	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not in PCM format: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not  in mono: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}

	fread( readBuffer, 1, 4, file );

	// little endian
	*outSampleRate =
			(int)( readBuffer[3] << 24 |
				   readBuffer[2] << 16 |
				   readBuffer[1] << 8 |
				   readBuffer[0] );


	fseek( file, 34, SEEK_SET );


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 16 && readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not 16-bit: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}



	/*
	  // this is not reliable as arecord leaves this blank when
	  // recording a stream
	fseek( file, 40, SEEK_SET );
	fread( readBuffer, 1, 4, file );
	// little endian
	int numSampleBytes =
		(int)( readBuffer[3] << 24 |
			   readBuffer[2] << 16 |
			   readBuffer[1] << 8 |
			   readBuffer[0] );
	*/

	fseek( file, 44, SEEK_SET );


	int currentPos = ftell( file );

	int numSampleBytes = fileSize - currentPos;

	*outNumSamples = numSampleBytes / 2;

	int numSamples = *outNumSamples;


	unsigned char *rawSamples = new unsigned char[ 2 * numSamples ];

	int numRead = fread( rawSamples, 1, numSamples * 2, file );


	if( numRead != numSamples * 2 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to read %d samples from file: %s\n",
						numSamples, arecordFileName );

		delete [] rawSamples;
		fclose( file );
		return NULL;
	}


	int16_t *returnSamples = new int16_t[ numSamples ];

	int r = 0;
	for( int i=0; i<numSamples; i++ ) {
		// little endian
		returnSamples[i] =
				( rawSamples[r+1] << 8 ) |
				rawSamples[r];

		r += 2;
	}
	delete [] rawSamples;



	fclose( file );

	return returnSamples;
}

#ifdef LINUX
char isPrintingSupported() {
    int ret = system( "convert --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
    }


void printImage( Image *inImage, char inFullColor ) {
    const char *fileName = "printImage_temp.tga";

    writeTGAFile( fileName, inImage );

    const char *colorspaceFlag = "-colorspace gray";

    if( inFullColor ) {
        colorspaceFlag = "";
        }


    char *command =
        autoSprintf( "convert -density 72x72 "
                     " %s %s ps:- | lpr",
                     colorspaceFlag, fileName );


    system( command );

    delete [] command;

    // File file( NULL, fileName );
    // file.remove();
    }
#else
char isPrintingSupported() {
	return false;
}

void printImage( Image *inImage, char inFullColor ) {
}
#endif