//
// Created by olivier on 19/10/2021.
//

#include "socket.h"

#include "OneLife/gameSource/LivingLifePage.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"//TODO: set value outside
#include "minorGems/formats/encodingUtils.h"

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
int pendingCompressedChunkSize;
char serverFrameReady;
char *pendingMapChunkMessage = NULL;
char pendingCMData = false;
int pendingCMCompressedSize = 0;
int pendingCMDecompressedSize = 0;
SimpleVector<char*> readyPendingReceivedMessages;
double lastServerMessageReceiveTime = 0;
double largestPendingMessageTimeGap = 0;// while player action pending, measure largest gap between sequential // server messages// This is an approximation of our outtage time.
char waitForFrameMessages = false;
SimpleVector<char*> serverFrameMessages;
int messagesInCount = 0;



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

messageType getMessageType( char *inMessage )
{
	char *copy = stringDuplicate( inMessage );

	char *firstBreak = strstr( copy, "\n" );

	if( firstBreak == NULL ) {
		delete [] copy;
		return UNKNOWN;
	}

	firstBreak[0] = '\0';

	messageType returnValue = UNKNOWN;

	if( strcmp( copy, "CM" ) == 0 ) {
		returnValue = COMPRESSED_MESSAGE;
	}
	else if( strcmp( copy, "MC" ) == 0 ) {
		returnValue = MAP_CHUNK;
	}
	else if( strcmp( copy, "MX" ) == 0 ) {
		returnValue = MAP_CHANGE;
	}
	else if( strcmp( copy, "PU" ) == 0 ) {
		returnValue = PLAYER_UPDATE;
	}
	else if( strcmp( copy, "PM" ) == 0 ) {
		returnValue = PLAYER_MOVES_START;
	}
	else if( strcmp( copy, "PO" ) == 0 ) {
		returnValue = PLAYER_OUT_OF_RANGE;
	}
	else if( strcmp( copy, "BW" ) == 0 ) {
		returnValue = BABY_WIGGLE;
	}
	else if( strcmp( copy, "PS" ) == 0 ) {
		returnValue = PLAYER_SAYS;
	}
	else if( strcmp( copy, "LS" ) == 0 ) {
		returnValue = LOCATION_SAYS;
	}
	else if( strcmp( copy, "PE" ) == 0 ) {
		returnValue = PLAYER_EMOT;
	}
	else if( strcmp( copy, "FX" ) == 0 ) {
		returnValue = FOOD_CHANGE;
	}
	else if( strcmp( copy, "HX" ) == 0 ) {
		returnValue = HEAT_CHANGE;
	}
	else if( strcmp( copy, "LN" ) == 0 ) {
		returnValue = LINEAGE;
	}
	else if( strcmp( copy, "CU" ) == 0 ) {
		returnValue = CURSED;
	}
	else if( strcmp( copy, "CX" ) == 0 ) {
		returnValue = CURSE_TOKEN_CHANGE;
	}
	else if( strcmp( copy, "CS" ) == 0 ) {
		returnValue = CURSE_SCORE;
	}
	else if( strcmp( copy, "NM" ) == 0 ) {
		returnValue = NAMES;
	}
	else if( strcmp( copy, "AP" ) == 0 ) {
		returnValue = APOCALYPSE;
	}
	else if( strcmp( copy, "AD" ) == 0 ) {
		returnValue = APOCALYPSE_DONE;
	}
	else if( strcmp( copy, "DY" ) == 0 ) {
		returnValue = DYING;
	}
	else if( strcmp( copy, "HE" ) == 0 ) {
		returnValue = HEALED;
	}
	else if( strcmp( copy, "PJ" ) == 0 ) {
		returnValue = POSSE_JOIN;
	}
	else if( strcmp( copy, "MN" ) == 0 ) {
		returnValue = MONUMENT_CALL;
	}
	else if( strcmp( copy, "GV" ) == 0 ) {
		returnValue = GRAVE;
	}
	else if( strcmp( copy, "GM" ) == 0 ) {
		returnValue = GRAVE_MOVE;
	}
	else if( strcmp( copy, "GO" ) == 0 ) {
		returnValue = GRAVE_OLD;
	}
	else if( strcmp( copy, "OW" ) == 0 ) {
		returnValue = OWNER;
	}
	else if( strcmp( copy, "FW" ) == 0 ) {
		returnValue = FOLLOWING;
	}
	else if( strcmp( copy, "EX" ) == 0 ) {
		returnValue = EXILED;
	}
	else if( strcmp( copy, "VS" ) == 0 ) {
		returnValue = VALLEY_SPACING;
	}
	else if( strcmp( copy, "FD" ) == 0 ) {
		returnValue = FLIGHT_DEST;
	}
	else if( strcmp( copy, "BB" ) == 0 ) {
		returnValue = BAD_BIOMES;
	}
	else if( strcmp( copy, "VU" ) == 0 ) {
		returnValue = VOG_UPDATE;
	}
	else if( strcmp( copy, "PH" ) == 0 ) {
		returnValue = PHOTO_SIGNATURE;
	}
	else if( strcmp( copy, "PONG" ) == 0 ) {
		returnValue = PONG;
	}
	else if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
		returnValue = SHUTDOWN;
	}
	else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
		returnValue = SERVER_FULL;
	}
	else if( strcmp( copy, "SN" ) == 0 ) {
		returnValue = SEQUENCE_NUMBER;
	}
	else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
		returnValue = ACCEPTED;
	}
	else if( strcmp( copy, "REJECTED" ) == 0 ) {
		returnValue = REJECTED;
	}
	else if( strcmp( copy, "NO_LIFE_TOKENS" ) == 0 ) {
		returnValue = NO_LIFE_TOKENS;
	}
	else if( strcmp( copy, "SD" ) == 0 ) {
		returnValue = FORCED_SHUTDOWN;
	}
	else if( strcmp( copy, "MS" ) == 0 ) {
		returnValue = GLOBAL_MESSAGE;
	}
	else if( strcmp( copy, "WR" ) == 0 ) {
		returnValue = WAR_REPORT;
	}
	else if( strcmp( copy, "LR" ) == 0 ) {
		returnValue = LEARNED_TOOL_REPORT;
	}
	else if( strcmp( copy, "TE" ) == 0 ) {
		returnValue = TOOL_EXPERTS;
	}
	else if( strcmp( copy, "TS" ) == 0 ) {
		returnValue = TOOL_SLOTS;
	}
	else if( strcmp( copy, "HL" ) == 0 ) {
		returnValue = HOMELAND;
	}
	else if( strcmp( copy, "FL" ) == 0 ) {
		returnValue = FLIP;
	}
	else if( strcmp( copy, "CR" ) == 0 ) {
		returnValue = CRAVING;
	}

	delete [] copy;
	return returnValue;
}

// NULL if there's no full message available
char *getNextServerMessageRaw()
{

	if( pendingMapChunkMessage != NULL ) {
		// wait for full binary data chunk to arrive completely
		// after message before we report that the message is ready

		if( serverSocketBuffer.size() >= pendingCompressedChunkSize ) {
			char *returnMessage = pendingMapChunkMessage;
			pendingMapChunkMessage = NULL;

			messagesInCount++;

			return returnMessage;
		}
		else {
			// wait for more data to arrive before saying this MC message
			// is ready
			return NULL;
		}
	}

	if( pendingCMData ) {
		if( serverSocketBuffer.size() >= pendingCMCompressedSize ) {
			pendingCMData = false;

			unsigned char *compressedData =
					new unsigned char[ pendingCMCompressedSize ];

			for( int i=0; i<pendingCMCompressedSize; i++ ) {
				compressedData[i] = serverSocketBuffer.getElementDirect( i );
			}
			serverSocketBuffer.deleteStartElements( pendingCMCompressedSize );

			unsigned char *decompressedMessage =
					zipDecompress( compressedData,
								   pendingCMCompressedSize,
								   pendingCMDecompressedSize );

			delete [] compressedData;

			if( decompressedMessage == NULL ) {
				printf( "Decompressing CM message failed\n" );
				return NULL;
			}
			else {
				char *textMessage = new char[ pendingCMDecompressedSize + 1 ];
				memcpy( textMessage, decompressedMessage,
						pendingCMDecompressedSize );
				textMessage[ pendingCMDecompressedSize ] = '\0';

				delete [] decompressedMessage;

				messagesInCount++;
				return textMessage;
			}
		}
		else {
			// wait for more data to arrive
			return NULL;
		}
	}



	// find first terminal character #

	int index = serverSocketBuffer.getElementIndex( '#' );

	if( index == -1 ) {
		return NULL;
	}

	// terminal character means message arrived

	double curTime = game_getCurrentTime();

	double gap = curTime - lastServerMessageReceiveTime;

	if( gap > largestPendingMessageTimeGap ) {
		largestPendingMessageTimeGap = gap;
	}

	lastServerMessageReceiveTime = curTime;



	char *message = new char[ index + 1 ];

	for( int i=0; i<index; i++ ) {
		message[i] = (char)( serverSocketBuffer.getElementDirect( i ) );
	}
	// delete message and terminal character
	serverSocketBuffer.deleteStartElements( index + 1 );

	message[ index ] = '\0';

	if( getMessageType( message ) == MAP_CHUNK ) {
		pendingMapChunkMessage = message;

		int sizeX, sizeY, x, y, binarySize;
		sscanf( message, "MC\n%d %d %d %d\n%d %d\n",
				&sizeX, &sizeY,
				&x, &y, &binarySize, &pendingCompressedChunkSize );


		return getNextServerMessageRaw();
	}
	else if( getMessageType( message ) == COMPRESSED_MESSAGE ) {
		pendingCMData = true;

		printf( "Got compressed message header:\n%s\n\n", message );

		sscanf( message, "CM\n%d %d\n",
				&pendingCMDecompressedSize, &pendingCMCompressedSize );

		delete [] message;
		return NULL;
	}
	else {
		messagesInCount++;
		return message;
	}
}

// either returns a pending recieved message (one that was received earlier
// or held back
//
// or receives the next message from the server socket (if we are not waiting
// for full frames of messages)
//
// or returns NULL until a full frame of messages is available, and
// then returns the first message from the frame
char *getNextServerMessage()
{

	if( readyPendingReceivedMessages.size() > 0 ) {
		char *message = readyPendingReceivedMessages.getElementDirect( 0 );
		readyPendingReceivedMessages.deleteElement( 0 );
		printf( "Playing a held pending message\n" );
		return message;
	}

	if( !waitForFrameMessages ) {
		return getNextServerMessageRaw();
	}
	else {
		if( !serverFrameReady ) {
			// read more and look for end of frame

			char *message = getNextServerMessageRaw();

			while( message != NULL ) {
				messageType t = getMessageType( message );

				if( strstr( message, "FM" ) == message ) {
					// end of frame, discard the marker message
					delete [] message;

					if( serverFrameMessages.size() > 0 ) {
						serverFrameReady = true;
						// see end of frame, stop reading more messages
						// for now (they are part of next frame)
						// and start returning message to caller from
						// this frame
						break;
					}
				}
				else if( t == MAP_CHUNK ||
						 t == PONG ||
						 t == FLIGHT_DEST ||
						 t == PHOTO_SIGNATURE ) {
					// map chunks are followed by compressed data
					// they cannot be queued

					// PONG messages should be returned instantly

					// FLIGHT_DEST messages also should be returned instantly
					// otherwise, they will be queued and seen by
					// the client after the corresponding MC message
					// for the new location.
					// which will invalidate the map around player's old
					// location
					return message;
				}
				else {
					// some other message in the middle of the frame
					// keep it
					serverFrameMessages.push_back( message );
				}

				// keep reading messages, until we either see the
				// end of the frame or read all available messages
				message = getNextServerMessageRaw();
			}
		}

		if( serverFrameReady ) {
			char *message = serverFrameMessages.getElementDirect( 0 );

			serverFrameMessages.deleteElement( 0 );

			if( serverFrameMessages.size() == 0 ) {
				serverFrameReady = false;
			}
			return message;
		}
		else {
			return NULL;
		}
	}
}
