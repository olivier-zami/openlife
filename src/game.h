//
// Created by olivier on 22/09/2021.
//

#ifndef OPENLIFE_CLIENT_GAME_H
#define OPENLIFE_CLIENT_GAME_H

#include "src/client/component/bank/sprite.h"

namespace client
{
	class Game
	{
		public:
			Game();
			~Game();

			client::component::bank::Sprite* getSprites();

		private:
			client::component::bank::Sprite* spriteBank;
	};
}

#include "minorGems/game/game.h"//Uint8

int mainFunction( int inArgCount, char **inArgs );
void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill );
void cleanUpAtExit();


#include "minorGems/ui/GUIComponent.h"
#include "src/client/screen.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/graphics/openGL/RedrawListenerGL.h"
#include "minorGems/ui/event/ActionListener.h"
class GameSceneHandler :
		public SceneHandlerGL, public MouseHandlerGL, public KeyboardHandlerGL,
		public RedrawListenerGL, public ActionListener  {

public:

	/**
	 * Constructs a sceen handler.
	 *
	 * @param inScreen the screen to interact with.
	 *   Must be destroyed by caller after this class is destroyed.
	 */
	GameSceneHandler( ScreenGL *inScreen );

	virtual ~GameSceneHandler();



	/**
	 * Executes necessary init code that reads from files.
	 *
	 * Must be called before using a newly-constructed GameSceneHandler.
	 *
	 * This call assumes that the needed files are in the current working
	 * directory.
	 */
	void initFromFiles();



	ScreenGL *mScreen;








	// implements the SceneHandlerGL interface
	virtual void drawScene();

	// implements the MouseHandlerGL interface
	virtual void mouseMoved( int inX, int inY );
	virtual void mouseDragged( int inX, int inY );
	virtual void mousePressed( int inX, int inY );
	virtual void mouseReleased( int inX, int inY );

	// implements the KeyboardHandlerGL interface
	virtual char isFocused() {
		// always focused
		return true;
	}
	virtual void keyPressed( unsigned char inKey, int inX, int inY );
	virtual void specialKeyPressed( int inKey, int inX, int inY );
	virtual void keyReleased( unsigned char inKey, int inX, int inY );
	virtual void specialKeyReleased( int inKey, int inX, int inY );

	// implements the RedrawListener interface
	virtual void fireRedraw();


	// implements the ActionListener interface
	virtual void actionPerformed( GUIComponent *inTarget );


	char mPaused;
	char mPausedDuringFrameBatch;
	char mLoadingDuringFrameBatch;

	// reduce sleep time when user hits keys to restore responsiveness
	unsigned int mPausedSleepTime;


	char mBlockQuitting;

	double mLastFrameRate;


protected:

	timeSec_t mStartTimeSeconds;


	char mPrintFrameRate;
	unsigned long mNumFrames;
	unsigned long mFrameBatchSize;
	double mFrameBatchStartTimeSeconds;



	Color mBackgroundColor;


};

#endif //OPENLIFE_CLIENT_GAME_H
