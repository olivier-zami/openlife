//
// Created by olivier on 19/10/2021.
//

#include "socket.h"

#include "OneLife/gameSource/LivingLifePage.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"//TODO: set value outside

extern int showBugMessage;//from LivingLifePage.cpp

char *lastMessageSentToServer = NULL;
SimpleVector<unsigned char> serverSocketBuffer;
char serverSocketConnected = false;
char serverSocketHardFail = false;
float connectionMessageFade = 1.0f;
double connectedTime = 0;
char forceDisconnect = false;
double timeLastMessageSent = 0;
char *nextActionMessageToSend = NULL;// if user clicks to initiate an action while still moving, we// queue it here
int numServerBytesRead = 0;
int numServerBytesSent = 0;
int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;
int messagesOutCount = 0;
int bytesOutCount = 0;
char userReconnect = false;
int bytesInCount = 0;

// destroyed internally if not NULL
static void replaceLastMessageSent( char *inNewMessage )
{
	if( lastMessageSentToServer != NULL )
	{
		delete [] lastMessageSentToServer;
	}
	lastMessageSentToServer = inNewMessage;
}

void LivingLifePage::sendBugReport( int inBugNumber )
{
	char *bugString = stringDuplicate( "" );

	if( lastMessageSentToServer != NULL ) {
		char *temp = bugString;
		bugString = autoSprintf( "%s   Just sent: [%s]",
								 temp, lastMessageSentToServer );
		delete [] temp;
	}
	if( nextActionMessageToSend != NULL ) {
		char *temp = bugString;
		bugString = autoSprintf( "%s   Waiting to send: [%s]",
								 temp, nextActionMessageToSend );
		delete [] temp;
	}

	// clear # terminators from message
	char *spot = strstr( bugString, "#" );

	while( spot != NULL ) {
		spot[0] = ' ';
		spot = strstr( bugString, "#" );
	}


	char *bugMessage = autoSprintf( "BUG %d %s#", inBugNumber, bugString );

	delete [] bugString;

	sendToServerSocket( bugMessage );
	delete [] bugMessage;

	if( ! SettingsManager::getIntSetting( "reportWildBugToUser", 1 ) ) {
		return;
	}

	FILE *f = fopen( "stdout.txt", "r" );

	int recordGame = SettingsManager::getIntSetting( "recordGame", 0 );

	if( f != NULL ) {
		// stdout.txt exists

		printf( "Bug report sent, telling user to email files to us.\n" );

		fclose( f );

		showBugMessage = 3;

		if( recordGame ) {
			showBugMessage = 2;
		}
	}
	else if( recordGame ) {
		printf( "Bug report sent, telling user to email files to us.\n" );
		showBugMessage = 1;
	}
}

// reads all waiting data from socket and stores it in buffer
// returns false on socket error
char readServerSocketFull( int inServerSocket )
{

	if( forceDisconnect ) {
		forceDisconnect = false;
		return false;
	}


	unsigned char buffer[512];

	int numRead = readFromSocket( inServerSocket, buffer, 512 );


	while( numRead > 0 ) {
		if( ! serverSocketConnected ) {
			serverSocketConnected = true;
			connectedTime = game_getCurrentTime();
		}

		serverSocketBuffer.appendArray( buffer, numRead );
		numServerBytesRead += numRead;
		bytesInCount += numRead;

		numRead = readFromSocket( inServerSocket, buffer, 512 );
	}

	if( numRead == -1 ) {
		printf( "Failed to read from server socket at time %f\n",
				game_getCurrentTime() );
		return false;
	}

	return true;
}

void LivingLifePage::sendToServerSocket( char *inMessage )
{
	timeLastMessageSent = game_getCurrentTime();

	printf( "Sending message to server: %s\n", inMessage );

	replaceLastMessageSent( stringDuplicate( inMessage ) );

	int len = strlen( inMessage );

	int numSent = sendToSocket( mServerSocket, (unsigned char*)inMessage, len );

	if( numSent == len ) {
		numServerBytesSent += len;
		overheadServerBytesSent += 52;

		messagesOutCount++;
		bytesOutCount += len;
	}
	else {
		printf( "Failed to send message to server socket "
				"at time %f "
				"(tried to send %d, but numSent=%d)\n",
				game_getCurrentTime(), len, numSent );
		closeSocket( mServerSocket );
		mServerSocket = -1;

		if( mFirstServerMessagesReceived  ) {

			if( mDeathReason != NULL ) {
				delete [] mDeathReason;
			}
			mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );

			handleOurDeath( true );
		}
		else {
			setWaiting( false );

			if( userReconnect ) {
				setSignal( "reconnectFailed" );
			}
			else {
				setSignal( "loginFailed" );
			}
		}
	}

}
