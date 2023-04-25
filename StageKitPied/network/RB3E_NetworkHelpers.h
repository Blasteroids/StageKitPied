#ifndef _RB3E_NETWORKHELPERS_H_
#define _RB3E_NETWORKHELPERS_H_

#define RB3E_NETWORK_MAGICKEY 0x52423345

typedef struct _RB3E_EventHeader
{
    uint32_t ProtocolMagic; // big endian, do a ntohl
    uint8_t  ProtocolVersion;
    uint8_t  PacketType;
    uint8_t  PacketSize;
    uint8_t  Platform;
} RB3E_EventHeader;

typedef struct _RB3E_EventPacket
{
    RB3E_EventHeader Header;
    uint8_t Data[ 0xFF ];
} RB3E_EventPacket;

typedef enum _RB3E_Events_PacketTypes
{
    RB3E_EVENT_ALIVE,          // content is a string of the RB3E_BUILDTAG value
    RB3E_EVENT_STATE,          // content is a char - 00=menus, 01=ingame
    RB3E_EVENT_SONG_NAME,      // content is a string of the current song name
    RB3E_EVENT_SONG_ARTIST,    // content is a string of the current song artist
    RB3E_EVENT_SONG_SHORTNAME, // content is a string of the current shortname
    RB3E_EVENT_SCORE,          // content is a RB3E_EventScore struct with score info
    RB3E_EVENT_STAGEKIT,       // content is a RB3E_EventStagekit struct with stagekit info
    RB3E_EVENT_BAND_INFO       // content is a RB3E_EventBandInfo struct with band info
} RB3E_Events_EventTypes;

typedef struct _RB3E_EventStagekit
{
    uint8_t LeftChannel;
    uint8_t RightChannel;
} RB3E_EventStagekit;

typedef struct _RB3E_EventScore
{
    uint32_t TotalScore;
    uint32_t MemberScores[4];
    uint8_t  Stars;
} RB3E_EventScore;

typedef struct _RB3E_EventBandInfo
{
    uint8_t MemberExists[4];
    uint8_t Difficulty[4];
    uint8_t TrackType[4];
} RB3E_EventBandInfo;

#endif
