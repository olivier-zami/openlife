//
// Created by olivier on 19/10/2021.
//

#ifndef OPENLIFE_CLIENT_SOCKET_H
#define OPENLIFE_CLIENT_SOCKET_H

namespace openLife::client
{
	class Socket
	{
		public:
			Socket();
			~Socket();
	};
}

typedef enum messageType {
	SHUTDOWN,
	SERVER_FULL,
	SEQUENCE_NUMBER,
	ACCEPTED,
	NO_LIFE_TOKENS,
	REJECTED,
	MAP_CHUNK,
	MAP_CHANGE,
	PLAYER_UPDATE,
	PLAYER_MOVES_START,
	PLAYER_OUT_OF_RANGE,
	BABY_WIGGLE,
	PLAYER_SAYS,
	LOCATION_SAYS,
	PLAYER_EMOT,
	FOOD_CHANGE,
	HEAT_CHANGE,
	LINEAGE,
	CURSED,
	CURSE_TOKEN_CHANGE,
	CURSE_SCORE,
	NAMES,
	APOCALYPSE,
	APOCALYPSE_DONE,
	DYING,
	HEALED,
	POSSE_JOIN,
	MONUMENT_CALL,
	GRAVE,
	GRAVE_MOVE,
	GRAVE_OLD,
	OWNER,
	FOLLOWING,
	EXILED,
	VALLEY_SPACING,
	FLIGHT_DEST,
	BAD_BIOMES,
	VOG_UPDATE,
	PHOTO_SIGNATURE,
	FORCED_SHUTDOWN,
	GLOBAL_MESSAGE,
	WAR_REPORT,
	LEARNED_TOOL_REPORT,
	TOOL_EXPERTS,
	TOOL_SLOTS,
	HOMELAND,
	FLIP,
	CRAVING,
	PONG,
	COMPRESSED_MESSAGE,
	UNKNOWN
} messageType;

char readServerSocketFull( int inServerSocket );
char *getNextServerMessageRaw();
char *getNextServerMessage();
messageType getMessageType( char *inMessage );

#endif //OPENLIFE_CLIENT_SOCKET_H