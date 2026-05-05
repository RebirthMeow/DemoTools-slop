#include <cwchar>   // wchar_t wide characters
#include "client/client.h"
#include "demo_common.h"
#include <map>
#include <string>

#if (defined _MSC_VER)
#include <Windows.h>

// Convert a wide Unicode string to an UTF8 string
static const char *utf8_encode( const wchar_t *wstr, int wstrSize )
{
	int size_needed = WideCharToMultiByte( CP_UTF8, 0, wstr, wstrSize, NULL, 0, NULL, NULL );
	char *strTo = (char *) malloc( size_needed + 1 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[0], wstrSize, strTo, size_needed, NULL, NULL );
	strTo[size_needed] = '\0';
	return strTo;
}

wchar_t wtmpBuf[MAX_STRING_CHARS];
const wchar_t *utf8BytesToString( const char *utf8 ) {
	int size_needed = MultiByteToWideChar( CP_UTF8, 0, utf8, -1, 0, 0 );
	wchar_t *strTo = (wchar_t *) malloc( size_needed * sizeof( wchar_t ) );
	MultiByteToWideChar( CP_UTF8, 0, utf8, -1, strTo, size_needed );
	Q_strncpyz( (char *) wtmpBuf, (char *) strTo, sizeof( wtmpBuf ) * sizeof( *wtmpBuf ) );
	free( strTo );
	return wtmpBuf;
}

char tmpBuf[MAX_STRING_CHARS];
const char *cp1252toUTF8( const char *cp1252 )
{
	int size_needed = MultiByteToWideChar( CP_ACP, 0, cp1252, -1, 0, 0 );
	wchar_t *strTo = (wchar_t *) malloc( size_needed * sizeof( wchar_t ) );
	MultiByteToWideChar( CP_ACP, 0, cp1252, -1, strTo, size_needed );
	const char *result = utf8_encode( strTo, size_needed );
	free( strTo );
	Q_strncpyz( tmpBuf, result, sizeof( tmpBuf ) );
	free( ( void *) result );
	return tmpBuf;
}
#else
#include <locale.h>

// Convert a wide Unicode string to an UTF8 string
static const char *utf8_encode( const wchar_t *wstr, int wstrSize )
{
	setlocale( LC_CTYPE, "en_US.UTF-8" );
	mbstate_t ps;
	memset( &ps, 0, sizeof( ps ) );
	size_t size_needed = wcsrtombs( NULL, &wstr, 0, &ps );
	if ( size_needed == (size_t) -1 ) {
		fprintf( stderr, "failure at: %S\n", wstr );
		perror( "utf8 conversion failed" );
		return NULL;
	}
	char *strTo = (char *) malloc( size_needed + 1 );
	memset( &ps, 0, sizeof( ps ) );
	wcsrtombs( strTo, &wstr, size_needed, &ps );
	strTo[size_needed] = '\0';
	return strTo;
}

wchar_t wtmpBuf[MAX_STRING_CHARS];
const wchar_t *utf8BytesToString( const char *utf8 ) {
	setlocale( LC_CTYPE, "en_US.UTF-8" );
	mbstate_t ps;
	memset( &ps, 0, sizeof( ps ) );
	size_t size_needed = mbsrtowcs( NULL, &utf8, 0, &ps );
	if ( size_needed == (size_t) -1 ) {
		perror( "utf8 conversion failed" );
		return NULL;
	}
	wchar_t *strTo = (wchar_t *) malloc( size_needed * sizeof( wchar_t ) );
	mbsrtowcs( strTo, &utf8, size_needed, &ps );
	Q_strncpyz( (char *) wtmpBuf, (char *) strTo, sizeof( wtmpBuf ) * sizeof( *wtmpBuf ) );
	free( strTo );
	return wtmpBuf;
}

char tmpBuf[MAX_STRING_CHARS];
const char *cp1252toUTF8( const char *cp1252 )
{
	setlocale( LC_CTYPE, "en_US.CP1252" );
	mbstate_t ps;
	memset( &ps, 0, sizeof( ps ) );
	size_t size_needed = mbsrtowcs( NULL, &cp1252, 0, &ps );
	if ( size_needed == (size_t) -1 ) {
		fprintf( stderr, "failure at: %s\n", cp1252 );
		perror( "cp1252 conversion failed" );
		//return NULL;
		char *ret = strdup( cp1252 ), *out = ret;
		for ( const char *in = cp1252; *in; ) {
			if (*in > 0) {
				*out = *in;
				out++;
			}
			in++;
		}
		*out = 0;
		return ret;
	}
	wchar_t *strTo = (wchar_t *) malloc( (size_needed + 1) * sizeof( wchar_t ) );
	memset( &ps, 0, sizeof( ps ) );
	mbsrtowcs( strTo, &cp1252, size_needed, &ps );
	strTo[size_needed] = '\0';
	const char *result = utf8_encode( strTo, size_needed );
	free( strTo );
	Q_strncpyz( tmpBuf, result, sizeof( tmpBuf ) );
	free( ( void *) result );
	return tmpBuf;
}
#endif

#if (defined _MSC_VER)
#define strtoull _strtoui64
#endif

typedef struct clientCache_s {
	int lastOffset;
	char name[MAX_NETNAME];
	char nameUTF8[MAX_NETNAME * 3]; // UTF-8 can be up to 3x longer
	int skill;
	uint64_t uniqueId;
	char cid[MAX_STRING_CHARS];
	team_t team;
} clientCache_t;

static clientCache_t clientCache[MAX_CLIENTS];
static bool clientCacheInitialized = false;

static void updateClientCache( int playerIdx ) {
	if ( !clientCacheInitialized ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			clientCache[i].lastOffset = -1;
		}
		clientCacheInitialized = true;
	}

	int currentOffset = ctx->cl.gameState.stringOffsets[CS_PLAYERS + playerIdx];
	if ( clientCache[playerIdx].lastOffset == currentOffset ) {
		return;
	}

	const char *cs = ctx->cl.gameState.stringData + currentOffset;
	if ( !*cs ) {
		clientCache[playerIdx].lastOffset = currentOffset;
		clientCache[playerIdx].name[0] = 0;
		clientCache[playerIdx].nameUTF8[0] = 0;
		clientCache[playerIdx].skill = -1;
		clientCache[playerIdx].uniqueId = 0;
		clientCache[playerIdx].cid[0] = 0;
		clientCache[playerIdx].team = TEAM_FREE;
		return;
	}

	clientCache[playerIdx].lastOffset = currentOffset;
	
	const char *n = Info_ValueForKey( cs, "n" );
	Q_strncpyz( clientCache[playerIdx].name, n ? n : "UNKNOWN", sizeof( clientCache[playerIdx].name ) );
	Q_strncpyz( clientCache[playerIdx].nameUTF8, cp1252toUTF8( clientCache[playerIdx].name ), sizeof( clientCache[playerIdx].nameUTF8 ) );

	const char *skillStr = Info_ValueForKey( cs, "skill" );
	clientCache[playerIdx].skill = ( skillStr && skillStr[0] ) ? atoi( skillStr ) : -1;

	const char *idStr = Info_ValueForKey( cs, "id" );
	if ( idStr && *idStr ) {
		char *p = NULL;
		clientCache[playerIdx].uniqueId = strtoull( idStr, &p, 10 );
		if ( errno == ERANGE || *p != 0 || p == idStr ) {
			clientCache[playerIdx].uniqueId = 0;
		}
	} else {
		clientCache[playerIdx].uniqueId = 0;
	}

	const char *cidStr = Info_ValueForKey( cs, "cid" );
	Q_strncpyz( clientCache[playerIdx].cid, cidStr ? cidStr : "", sizeof( clientCache[playerIdx].cid ) );

	clientCache[playerIdx].team = (team_t) atoi( Info_ValueForKey( cs, "t" ) );
}

const char *getPlayerName( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return "UNKNOWN";
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].name;
}

const char *getPlayerNameUTF8( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return "UNKNOWN";
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].nameUTF8;
}

int playerSkill( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return -1;
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].skill;
}

uint64_t getUniqueId( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return 0;
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].uniqueId;
}

const char* getNewmodId( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return 0;
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].cid[0] ? clientCache[playerIdx].cid : 0;
}

const char *CG_TeamName(team_t team)
{
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

team_t OtherTeam(team_t team) {
	if (team==TEAM_RED)
		return TEAM_BLUE;
	else if (team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *getPlayerTeamName( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return "UNKNOWN";
	}
	updateClientCache( playerIdx );
	return CG_TeamName( clientCache[playerIdx].team );
}

qboolean playerActive( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return qfalse;
	}
	// player's configstring is set to emptystring if they are not connected
	return *(ctx->cl.gameState.stringData + ctx->cl.gameState.stringOffsets[ CS_PLAYERS + playerIdx ]) != 0 ? qtrue : qfalse;
}

team_t getPlayerTeam( int playerIdx ) {
	if ( playerIdx < 0 || playerIdx >= MAX_CLIENTS ) {
		return TEAM_FREE;
	}
	updateClientCache( playerIdx );
	return clientCache[playerIdx].team;
}

static gametype_t cachedGameType = (gametype_t)0;
static int cachedGameTypeOffset = -1;

gametype_t getGameType() {
	int currentOffset = ctx->cl.gameState.stringOffsets[ CS_SERVERINFO ];
	if ( currentOffset != cachedGameTypeOffset ) {
		cachedGameType = (gametype_t) atoi( Info_ValueForKey( ctx->cl.gameState.stringData + currentOffset, "g_gametype" ) );
		cachedGameTypeOffset = currentOffset;
	}
	return cachedGameType;
}


clSnapshot_t *previousSnap() {
	int curSnap = ctx->cl.snap.messageNum & PACKET_MASK;
	for ( int snapIdx = ( curSnap - 1 ) & PACKET_MASK; snapIdx != curSnap; snapIdx = ( snapIdx - 1 ) & PACKET_MASK ) {
		if ( ctx->cl.snapshots[ snapIdx ].valid ) {
			return &ctx->cl.snapshots[ snapIdx ];
		}
	}
	return NULL;
}

static int cachedLevelStartTime = 0;
static int cachedLevelStartTimeOffset = -1;

long getCurrentTime() {
	int currentOffset = ctx->cl.gameState.stringOffsets[ CS_LEVEL_START_TIME ];
	if ( currentOffset != cachedLevelStartTimeOffset ) {
		cachedLevelStartTime = atoi( ctx->cl.gameState.stringData + currentOffset );
		cachedLevelStartTimeOffset = currentOffset;
	}
	return ctx->cl.snap.serverTime - cachedLevelStartTime;
}
