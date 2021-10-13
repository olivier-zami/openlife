#ifndef OPENLIFE_LEGACY_DEMOCODEPANEL_H
#define OPENLIFE_LEGACY_DEMOCODEPANEL_H

#include "minorGems/graphics/openGL/gui/GUIPanelGL.h"
#include "minorGems/graphics/openGL/gui/GUITranslatorGL.h"
#include "minorGems/graphics/openGL/gui/TextGL.h"
#include "minorGems/graphics/openGL/gui/TextFieldGL.h"
#include "minorGems/graphics/openGL/gui/LabelGL.h"
#include "minorGems/graphics/Color.h"
#include "minorGems/util/SimpleVector.h"
#include "src/third_party/jason_rohrer/minorGems/util/log/AppLog.h"
#include "minorGems/game/game.h"
#include "DemoCodeChecker.h"
#include "minorGems/util/TranslationManager.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/SettingsManager.h"
#include "src/client/screen.h"

// shows panel until a correct code has been entered
// assumes font TGA in "graphics" folder
void showDemoCodePanel( ScreenGL *inScreen, const char *inFontTGAFileName,
						int inWidth, int inHeight );

// use this to check if demo checking done
char isDemoCodePanelShowing();

// call if app exits while panel still showing
void freeDemoCodePanel();


void setLabelString( LabelGL *inLabel,
                            const char *inTranslationString,
                            double inScaleFactor = 1 );

LabelGL *createLabel(
    double inGuiY, 
    const char *inTranslationString,
    TextGL *inText = NULL );


class DemoCodePanelKeyboardHandler : public KeyboardHandlerGL {
    public:
        void keyPressed( unsigned char inKey, int inX, int inY );


        char isFocused() {
            // always focused
            return true;
            }
        
		void specialKeyPressed( int inKey, int inX, int inY ) {
            }

		void keyReleased( unsigned char inKey, int inX, int inY ) {
            }

		void specialKeyReleased( int inKey, int inX, int inY ) {
            }
        
    };


// common init for both demo code panel and write failed message
void panelCommonInit( ScreenGL *inScreen, const char *inFontTGAFileName,
                             int inWidth, int inHeight ) ;

char isDemoCodePanelShowing();

void panelCommonFree();

void showWriteFailedPanel( ScreenGL *inScreen, const char *inFontTGAFileName,
                           int inWidth, int inHeight );

void freeWriteFailedPanel();

#endif//OPENLIFE_LEGACY_DEMOCODEPANEL_H