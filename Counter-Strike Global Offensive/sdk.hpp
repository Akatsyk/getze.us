#pragma once

#include <algorithm>
#include "optional.hpp"

#include "horizon.hpp"

#include "vector2d.hpp"
#include "vector.hpp"
#include "vector4d.hpp"
#include "qangle.hpp"
#include "matrix.hpp"
#include "CUtlVector.hpp"
#include "CUtlString.hpp"
#include "music_player.h"
//#include "math.hpp"

#pragma once

#include <array>
// ReSharper disable once CppUnusedIncludeDirective

namespace random {
	constexpr auto time = __TIME__;
	constexpr auto seed = static_cast<unsigned>(time[7]) + static_cast<unsigned>(time[6]) * 10 + static_cast<unsigned>(time[4]) * 60 + static_cast<unsigned>(time[3]) * 600 + static_cast<unsigned>(time[1]) * 3600 + static_cast<unsigned>(time[0]) * 36000;

	template <int n>
	struct gen {
	private:
		static constexpr unsigned a = 16807;
		static constexpr unsigned m = 2147483647;

		static constexpr unsigned s = gen<n - 1>::value;
		static constexpr unsigned lo = a * (s & 0xFFFFu);
		static constexpr unsigned hi = a * (s >> 16u);
		static constexpr unsigned lo2 = lo + ((hi & 0x7FFFu) << 16u);
		static constexpr unsigned hi2 = hi >> 15u;
		static constexpr unsigned lo3 = lo2 + hi;

	public:
		static constexpr unsigned max = m;
		static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
	};

	template <>
	struct gen<0> {
		static constexpr unsigned value = seed;
	};

	template <int n, int m>
	struct _int {
		static constexpr auto value = gen<n + 1>::value % m;
	};

	template <int n>
	struct _char {
		static const char value = static_cast<char>(1 + _int<n, 0x7F - 1>::value);
	};
}

template <size_t n, char k>
struct xorstr
{
private:
	static constexpr char enc(const char c)
	{
		return c ^ k;
	}

public:
	template <size_t... s>
	constexpr __forceinline xorstr(const char* str, std::index_sequence<s...>) : encrypted{ enc(str[s])... } { }

	__forceinline std::string dec()
	{
		std::string dec;
		dec.resize(n);

		for (auto i = 0; i < n; i++)
			dec[i] = encrypted[i] ^ k;

		return dec;
	}

	__forceinline std::string ot(bool decrypt = true)
	{
		std::string dec;
		dec.resize(n);

		for (auto i = 0; i < n; i++)
		{
			dec[i] = decrypt ? (encrypted[i] ^ k) : encrypted[i];
			encrypted[i] = '\0';
		}

		return dec;
	}

	std::array<char, n> encrypted{};
};

#define _(s) xorstr<sizeof(s), random::_char<__COUNTER__>::value>(s, std::make_index_sequence<sizeof(s)>()).dec().c_str()
#define _ot(s) xorstr<sizeof(s), random::_char<__COUNTER__>::value>(s, std::make_index_sequence<sizeof(s)>()).ot().c_str()
#define __(s) []() -> std::pair<std::string, char> { \
	constexpr auto key = random::_char<__COUNTER__>::value; \
	return std::make_pair(xorstr<sizeof(s), key>(s, std::make_index_sequence<sizeof(s)>()).ot(false), key); \
}()
#define _rt(n, s) auto (n) = reinterpret_cast<char*>(alloca(((s).first.size() + 1) * sizeof(char))); \
	for (size_t i = 0; i < (s).first.size(); i++) \
        (n)[i] = (s).first[i] ^ (s).second; \
    (n)[(s).first.size()] = '\0'


using namespace Horizon;
#pragma warning( disable : 4244 )  

// 
// macros
// 
#pragma region decl_macros

#define RandInt(min, max) (rand() % (max - min + 1) + min)

#define FLOW_OUTGOING 0		
#define FLOW_INCOMING 1
#define MAX_FLOWS 2		// in & out

#define END_OF_FREE_LIST -1
#define ENTRY_IN_USE -2

#define MAX_AREA_STATE_BYTES		32
#define MAX_AREA_PORTAL_STATE_BYTES 24

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define PAD(size) byte MACRO_CONCAT(_pad, __COUNTER__)[size];

#define MULTIPLAYER_BACKUP 150

#define FL_ONGROUND ( 1 << 0 )
#define FL_DUCKING ( 1 << 1 )

#define IN_ATTACK ( 1 << 0 )
#define IN_JUMP ( 1 << 1 )
#define IN_DUCK ( 1 << 2 )
#define IN_FORWARD ( 1 << 3 )
#define IN_BACK ( 1 << 4 )
#define IN_USE ( 1 << 5 )
#define IN_CANCEL ( 1 << 6 )
#define IN_LEFT ( 1 << 7 )
#define IN_RIGHT ( 1 << 8 )
#define IN_MOVELEFT ( 1 << 9 )
#define IN_MOVERIGHT ( 1 << 10 )
#define IN_ATTACK2 ( 1 << 11 )
#define IN_RUN ( 1 << 12 )
#define IN_RELOAD ( 1 << 13 )
#define IN_ALT1 ( 1 << 14 )
#define IN_ALT2 ( 1 << 15 )
#define IN_SCORE ( 1 << 16 )
#define IN_SPEED ( 1 << 17 )
#define IN_WALK ( 1 << 18 )
#define IN_ZOOM ( 1 << 19 )
#define IN_WEAPON1 ( 1 << 20 )
#define IN_WEAPON2 ( 1 << 21 )
#define IN_BULLRUSH ( 1 << 22 )
#define IN_GRENADE1 ( 1 << 23 )
#define IN_GRENADE2 ( 1 << 24 )


#define M_PI		3.14159265358979323846f
#define M_RADPI		57.295779513082f
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define MAX_SPLITSCREEN_CLIENT_BITS 2
// this should == MAX_JOYSTICKS in InputEnums.h
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4

// The default, no flags at all
#define FCVAR_NONE                0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_UNREGISTERED              (1<<0)  // If this is Set, don't add to linked list, etc.
#define FCVAR_DEVELOPMENTONLY           (1<<1)  // Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL                   (1<<2)  // defined by the game DLL
#define FCVAR_CLIENTDLL                 (1<<3)  // defined by the client DLL
#define FCVAR_HIDDEN                    (1<<4)  // Hidden. Doesn't appear in GetOffset or auto complete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only                                  
#define FCVAR_PROTECTED                 (1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY                    (1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define FCVAR_ARCHIVE                   (1<<7)  // Set to cause it to be saved to vars.rc
#define FCVAR_NOTIFY                    (1<<8)  // notifies players when changed
#define FCVAR_USERINFO                  (1<<9)  // changes the client's info string

#define FCVAR_PRINTABLEONLY             (1<<10) // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED                  (1<<11) // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_NEVER_AS_STRING           (1<<12) // never try to print that cvar
#define FCVAR_REPLICATED                (1<<13) // server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_CHEAT                     (1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats
#define FCVAR_SS                        (1<<15) // causes varnameN where N == 2 through max splitscreen slots for mod to be autogenerated
#define FCVAR_DEMO                      (1<<16) // record this cvar when starting a demo file
#define FCVAR_DONTRECORD                (1<<17) // don't record these command in demofiles
#define FCVAR_SS_ADDED                  (1<<18) // This is one of the "added" FCVAR_SS variables for the splitscreen players
#define FCVAR_RELEASE                   (1<<19) // Cvars tagged with this are the only cvars avaliable to customers
#define FCVAR_RELOAD_MATERIALS          (1<<20) // If this cvar changes, it forces a material reload
#define FCVAR_RELOAD_TEXTURES           (1<<21) // If this cvar changes, if forces a texture reload
#define FCVAR_NOT_CONNECTED             (1<<22) // cvar cannot be changed by a client that is connected to a server
#define FCVAR_MATERIAL_SYSTEM_THREAD    (1<<23) // Indicates this cvar is read from the material system thread
#define FCVAR_ARCHIVE_XBOX              (1<<24) // cvar written to config.cfg on the Xbox
#define FCVAR_ACCESSIBLE_FROM_THREADS   (1<<25) // used as a debugging tool necessary to check material system thread convars
//#define FCVAR_AVAILABLE               (1<<26)
//#define FCVAR_AVAILABLE               (1<<27)
#define FCVAR_SERVER_CAN_EXECUTE        (1<<28) // the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_SERVER_CANNOT_QUERY       (1<<29) // If this is Set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_CLIENTCMD_CAN_EXECUTE     (1<<30) // IVEngineClient::ClientCmd is allowed to execute this command. 
#define FCVAR_MEME_DLL                  (1<<31)

#define FCVAR_MATERIAL_THREAD_MASK ( FCVAR_RELOAD_MATERIALS | FCVAR_RELOAD_TEXTURES | FCVAR_MATERIAL_SYSTEM_THREAD )  

#define  Assert( _exp )										((void)0)
#define  AssertAligned( ptr )								((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg )							((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)

#define NDEBUG_PERSIST_TILL_NEXT_SERVER (0.01023f)
#define FMTFUNCTION( a, b )

typedef __int16					int16;
typedef unsigned __int16		uint16;
typedef __int32					int32;
typedef unsigned __int32		uint32;
typedef __int64					int64;
typedef unsigned __int64		uint64;

#define FLOAT32_NAN_BITS     (uint32)0x7FC00000	// not a number!
#define FLOAT32_NAN          unsigned long( FLOAT32_NAN_BITS )

#define VEC_T_NAN FLOAT32_NAN

#define DECL_ALIGN(x)			__declspec( align( x ) )

#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#define CHECK_VALID( _v ) 0

#define	CONTENTS_EMPTY			0		// No contents

#define	CONTENTS_SOLID			0x1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x2		// translucent, but not watery (glass)
#define	CONTENTS_AUX			0x4
#define	CONTENTS_GRATE			0x8		// alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
#define	CONTENTS_SLIME			0x10
#define	CONTENTS_WATER			0x20
#define	CONTENTS_BLOCKLOS		0x40	// block AI line of sight
#define CONTENTS_OPAQUE			0x80	// things that cannot be seen through (may be non-solid though)
#define	LAST_VISIBLE_CONTENTS	CONTENTS_OPAQUE

#define ALL_VISIBLE_CONTENTS (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

#define CONTENTS_TESTFOGVOLUME	0x100
#define CONTENTS_UNUSED			0x200	

// unused 
// NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
// if not visible, then grab from the bottom.
// CONTENTS_OPAQUE + SURF_NODRAW count as CONTENTS_OPAQUE (shadow-casting toolsblocklight textures)
#define CONTENTS_BLOCKLIGHT		0x400

#define CONTENTS_TEAM1			0x800	// per team contents used to differentiate collisions 
#define CONTENTS_TEAM2			0x1000	// between players and objects on different teams

// ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
#define CONTENTS_IGNORE_NODRAW_OPAQUE	0x2000

// hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MOVEABLE		0x4000

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEBRIS			0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000
#define CONTENTS_HITBOX			0x40000000	// use accurate hitboxes on trace

#define	MASK_SHOT	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)

// NOTE: These are stored in a short in the engine now.  Don't use more than 16 bits
#define	SURF_LIGHT		0x0001		// value will hold the light strength
#define	SURF_SKY2D		0x0002		// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS		0x0010
#define SURF_NOPORTAL	0x0020	// the surface can not have a portal placed on it
#define	SURF_TRIGGER	0x0040	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
#define	SURF_NODRAW		0x0080	// don't bother referencing the texture

#define	SURF_HINT		0x0100	// make a primary bsp splitter

#define	SURF_SKIP		0x0200	// completely ignore, allowing non-closed brushes
#define SURF_NOLIGHT	0x0400	// Don't calculate light
#define SURF_BUMPLIGHT	0x0800	// calculate three lightmaps for the surface for bumpmapping
#define SURF_NOSHADOWS	0x1000	// Don't receive shadows
#define SURF_NODECALS	0x2000	// Don't receive decals
#define SURF_NOPAINT	SURF_NODECALS	// the surface can not have paint placed on it
#define SURF_NOCHOP		0x4000	// Don't subdivide patches on this surface 
#define SURF_HITBOX		0x8000	// surface is part of a hitbox

#define	MASK_ALL					(0xFFFFFFFF)
// everything that is normally solid
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// everything that blocks player movement
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// blocks npc movement
#define	MASK_NPCSOLID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// blocks fluid movement
#define	MASK_NPCFLUID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
// water physics in these contents
#define	MASK_WATER					(CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)
// everything that blocks lighting
#define	MASK_OPAQUE					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)
// everything that blocks lighting, but with monsters added.
#define MASK_OPAQUE_AND_NPCS		(MASK_OPAQUE|CONTENTS_MONSTER)
// everything that blocks line of sight for AI
#define MASK_BLOCKLOS				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
// everything that blocks line of sight for AI plus NPCs
#define MASK_BLOCKLOS_AND_NPCS		(MASK_BLOCKLOS|CONTENTS_MONSTER)
// everything that blocks line of sight for players
#define	MASK_VISIBLE					(MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)
// everything that blocks line of sight for players, but with monsters added.
#define MASK_VISIBLE_AND_NPCS		(MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)
// bullets see these as solid
#define	MASK_SHOT					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
// bullets see these as solid, except monsters (world+brush only)
#define MASK_SHOT_BRUSHONLY			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_DEBRIS)
// non-raycasted weapons see this as solid (includes grates)
#define MASK_SHOT_HULL				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)
// hits solids (not grates) and passes through everything else
#define MASK_SHOT_PORTAL			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)
// everything normally solid, except monsters (world+brush only)
#define MASK_SOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)
// everything normally solid for player movement, except monsters (world+brush only)
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)
// everything normally solid for npc movement, except monsters (world+brush only)
#define MASK_NPCSOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC_FLUID	(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP)
// These are things that can split areaportals
#define MASK_SPLITAREAPORTAL		(CONTENTS_WATER|CONTENTS_SLIME)

// UNDONE: This is untested, any moving water
#define MASK_CURRENT				(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// everything that blocks corpse movement
// UNDONE: Not used yet / may be deleted
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE)

#pragma endregion

// 
// indixes
// 
#pragma region decl_indixes
namespace Index
{
	namespace IBaseClientDLL
	{
		enum
		{
			GetAllClasses = 8,
			CreateMove = 24,
			OverrideView = 18,
			GetViewModelFOV = 35, 
			FrameStageNotify = 37
		};
	}
	namespace IDirectX
	{
		enum
		{
			Reset = 16,
			EndScene = 42,
			BeginScene = 41,
		};
	}
	namespace IClientEntityList
	{
		enum
		{
			GetClientEntity = 3,
			GetClientEntityFromHandle = 4,
			GetHighestEntityIndex = 6,
		};
	}
	namespace IGameMovement
	{
		enum
		{
			ProcessMovement = 1,
			StartTrackPredictionErrors = 3,
			FinishTrackPredictionErrors = 4,
		};
	}
	namespace IPrediction
	{
		enum
		{
			InPrediction = 14,
			RunCommand = 19,
			SetupMove = 20,
			FinishMove = 21,
		};
	}
	namespace IMoveHelper
	{
		enum
		{
			SetHost = 1,
		};
	}
	namespace IInput
	{
		enum
		{
			GetUserCmd = 8,
		};
	}
	namespace IVEngineClient
	{
		enum
		{
			GetScreenSize = 5,
			GetPlayerInfo = 8,
			GetLocalPlayer = 12,
			Time = 14,
			GetViewAngles = 18,
			SetViewAngles = 19,
			GetMaxClients = 20,
			IsInGame = 26,
			IsConnected = 27,
			WorldToScreenMatrix = 37,
			ClientCmd_Unrestricted = 114,
		};
	}
	namespace IPanel
	{
		enum
		{
			GetName = 36,
			PaintTraverse = 41,
		};
	}
	namespace ISurface
	{
		enum
		{
			LockCursor = 67,
		};
	}
}
#pragma endregion

// 
// enums
// 
#pragma region decl_enums
enum SendPropType
{
	DPT_Int = 0,
	DPT_Float,
	DPT_Vector,
	DPT_VectorXY,
	DPT_String,
	DPT_Array,
	DPT_DataTable,
	DPT_NUMSendPropTypes,
};

enum MapLoadType_t
{
	MapLoad_NewGame = 0,
	MapLoad_LoadGame,
	MapLoad_Transition,
	MapLoad_Background,
};

enum ClientFrameStage_t
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END,
};

enum class_ids
{
	CTestTraceline = 223,
	CTEWorldDecal = 224,
	CTESpriteSpray = 221,
	CTESprite = 220,
	CTESparks = 219,
	CTESmoke = 218,
	CTEShowLine = 216,
	CTEProjectedDecal = 213,
	CFEPlayerDecal = 71,
	CTEPlayerDecal = 212,
	CTEPhysicsProp = 209,
	CTEParticleSystem = 208,
	CTEMuzzleFlash = 207,
	CTELargeFunnel = 205,
	CTEKillPlayerAttachments = 204,
	CTEImpact = 203,
	CTEGlowSprite = 202,
	CTEShatterSurface = 215,
	CTEFootprintDecal = 199,
	CTEFizz = 198,
	CTEExplosion = 196,
	CTEEnergySplash = 195,
	CTEEffectDispatch = 194,
	CTEDynamicLight = 193,
	CTEDecal = 191,
	CTEClientProjectile = 190,
	CTEBubbleTrail = 189,
	CTEBubbles = 188,
	CTEBSPDecal = 187,
	CTEBreakModel = 186,
	CTEBloodStream = 185,
	CTEBloodSprite = 184,
	CTEBeamSpline = 183,
	CTEBeamRingPoint = 182,
	CTEBeamRing = 181,
	CTEBeamPoints = 180,
	CTEBeamLaser = 179,
	CTEBeamFollow = 178,
	CTEBeamEnts = 177,
	CTEBeamEntPoint = 176,
	CTEBaseBeam = 175,
	CTEArmorRicochet = 174,
	CTEMetalSparks = 206,
	CSteamJet = 167,
	CSmokeStack = 157,
	DustTrail = 275,
	CFireTrail = 74,
	SporeTrail = 281,
	SporeExplosion = 280,
	RocketTrail = 278,
	SmokeTrail = 279,
	CPropVehicleDriveable = 144,
	ParticleSmokeGrenade = 277,
	CParticleFire = 116,
	MovieExplosion = 276,
	CTEGaussExplosion = 201,
	CEnvQuadraticBeam = 66,
	CEmbers = 55,
	CEnvWind = 70,
	CPrecipitation = 137,
	CPrecipitationBlocker = 138,
	CBaseTempEntity = 18,
	NextBotCombatCharacter = 0,
	CEconWearable = 54,
	CBaseAttributableItem = 4,
	CEconEntity = 53,
	CWeaponXM1014 = 272,
	CWeaponTaser = 267,
	CTablet = 171,
	CSnowball = 158,
	CSmokeGrenade = 155,
	CWeaponShield = 265,
	CWeaponSG552 = 263,
	CSensorGrenade = 151,
	CWeaponSawedoff = 259,
	CWeaponNOVA = 255,
	CIncendiaryGrenade = 99,
	CMolotovGrenade = 112,
	CMelee = 111,
	CWeaponM3 = 247,
	CKnifeGG = 108,
	CKnife = 107,
	CHEGrenade = 96,
	CFlashbang = 77,
	CFists = 76,
	CWeaponElite = 238,
	CDecoyGrenade = 47,
	CDEagle = 46,
	CWeaponUSP = 271,
	CWeaponM249 = 246,
	CWeaponUMP45 = 270,
	CWeaponTMP = 269,
	CWeaponTec9 = 268,
	CWeaponSSG08 = 266,
	CWeaponSG556 = 264,
	CWeaponSG550 = 262,
	CWeaponScout = 261,
	CWeaponSCAR20 = 260,
	CSCAR17 = 149,
	CWeaponP90 = 258,
	CWeaponP250 = 257,
	CWeaponP228 = 256,
	CWeaponNegev = 254,
	CWeaponMP9 = 253,
	CWeaponMP7 = 252,
	CWeaponMP5Navy = 251,
	CWeaponMag7 = 250,
	CWeaponMAC10 = 249,
	CWeaponM4A1 = 248,
	CWeaponHKP2000 = 245,
	CWeaponGlock = 244,
	CWeaponGalilAR = 243,
	CWeaponGalil = 242,
	CWeaponG3SG1 = 241,
	CWeaponFiveSeven = 240,
	CWeaponFamas = 239,
	CWeaponBizon = 234,
	CWeaponAWP = 232,
	CWeaponAug = 231,
	CAK47 = 1,
	CWeaponCSBaseGun = 236,
	CWeaponCSBase = 235,
	CC4 = 34,
	CBumpMine = 32,
	CBumpMineProjectile = 33,
	CBreachCharge = 28,
	CBreachChargeProjectile = 29,
	CWeaponBaseItem = 233,
	CBaseCSGrenade = 8,
	CSnowballProjectile = 160,
	CSnowballPile = 159,
	CSmokeGrenadeProjectile = 156,
	CSensorGrenadeProjectile = 152,
	CMolotovProjectile = 113,
	CItem_Healthshot = 104,
	CItemDogtags = 106,
	CDecoyProjectile = 48,
	CPhysPropRadarJammer = 126,
	CPhysPropWeaponUpgrade = 127,
	CPhysPropAmmoBox = 124,
	CPhysPropLootCrate = 125,
	CItemCash = 105,
	CEnvGasCanister = 63,
	CDronegun = 50,
	CParadropChopper = 115,
	CSurvivalSpawnChopper = 170,
	CBRC4Target = 27,
	CInfoMapRegion = 102,
	CFireCrackerBlast = 72,
	CInferno = 100,
	CChicken = 36,
	CDrone = 49,
	CFootstepControl = 79,
	CCSGameRulesProxy = 39,
	CWeaponCubemap = 0,
	CWeaponCycler = 237,
	CTEPlantBomb = 210,
	CTEFireBullets = 197,
	CTERadioIcon = 214,
	CPlantedC4 = 128,
	CCSTeam = 43,
	CCSPlayerResource = 41,
	CCSPlayer = 40,
	CPlayerPing = 130,
	CCSRagdoll = 42,
	CTEPlayerAnimEvent = 211,
	CHostage = 97,
	CHostageCarriableProp = 98,
	CBaseCSGrenadeProjectile = 9,
	CHandleTest = 95,
	CTeamplayRoundBasedRulesProxy = 173,
	CSpriteTrail = 165,
	CSpriteOriented = 164,
	CSprite = 163,
	CRagdollPropAttached = 147,
	CRagdollProp = 146,
	CPropCounter = 141,
	CPredictedViewModel = 139,
	CPoseController = 135,
	CGrassBurn = 94,
	CGameRulesProxy = 93,
	CInfoLadderDismount = 101,
	CFuncLadder = 85,
	CTEFoundryHelpers = 200,
	CEnvDetailController = 61,
	CDangerZone = 44,
	CDangerZoneController = 45,
	CWorldVguiText = 274,
	CWorld = 273,
	CWaterLODControl = 230,
	CWaterBullet = 229,
	CVoteController = 228,
	CVGuiScreen = 227,
	CPropJeep = 143,
	CPropVehicleChoreoGeneric = 0,
	CTriggerSoundOperator = 226,
	CBaseVPhysicsTrigger = 22,
	CTriggerPlayerMovement = 225,
	CBaseTrigger = 20,
	CTest_ProxyToggle_Networkable = 222,
	CTesla = 217,
	CBaseTeamObjectiveResource = 17,
	CTeam = 172,
	CSunlightShadowControl = 169,
	CSun = 168,
	CParticlePerformanceMonitor = 117,
	CSpotlightEnd = 162,
	CSpatialEntity = 161,
	CSlideshowDisplay = 154,
	CShadowControl = 153,
	CSceneEntity = 150,
	CRopeKeyframe = 148,
	CRagdollManager = 145,
	CPhysicsPropMultiplayer = 122,
	CPhysBoxMultiplayer = 120,
	CPropDoorRotating = 142,
	CBasePropDoor = 16,
	CDynamicProp = 52,
	CProp_Hallucination = 140,
	CPostProcessController = 136,
	CPointWorldText = 134,
	CPointCommentaryNode = 133,
	CPointCamera = 132,
	CPlayerResource = 131,
	CPlasma = 129,
	CPhysMagnet = 123,
	CPhysicsProp = 121,
	CStatueProp = 166,
	CPhysBox = 119,
	CParticleSystem = 118,
	CMovieDisplay = 114,
	CMaterialModifyControl = 110,
	CLightGlow = 109,
	CItemAssaultSuitUseable = 0,
	CItem = 0,
	CInfoOverlayAccessor = 103,
	CFuncTrackTrain = 92,
	CFuncSmokeVolume = 91,
	CFuncRotating = 90,
	CFuncReflectiveGlass = 89,
	CFuncOccluder = 88,
	CFuncMoveLinear = 87,
	CFuncMonitor = 86,
	CFunc_LOD = 81,
	CTEDust = 192,
	CFunc_Dust = 80,
	CFuncConveyor = 84,
	CFuncBrush = 83,
	CBreakableSurface = 31,
	CFuncAreaPortalWindow = 82,
	CFish = 75,
	CFireSmoke = 73,
	CEnvTonemapController = 69,
	CEnvScreenEffect = 67,
	CEnvScreenOverlay = 68,
	CEnvProjectedTexture = 65,
	CEnvParticleScript = 64,
	CFogController = 78,
	CEnvDOFController = 62,
	CCascadeLight = 35,
	CEnvAmbientLight = 60,
	CEntityParticleTrail = 59,
	CEntityFreezing = 58,
	CEntityFlame = 57,
	CEntityDissolve = 56,
	CDynamicLight = 51,
	CColorCorrectionVolume = 38,
	CColorCorrection = 37,
	CBreakableProp = 30,
	CBeamSpotlight = 25,
	CBaseButton = 5,
	CBaseToggle = 19,
	CBasePlayer = 15,
	CBaseFlex = 12,
	CBaseEntity = 11,
	CBaseDoor = 10,
	CBaseCombatCharacter = 6,
	CBaseAnimatingOverlay = 3,
	CBoneFollower = 26,
	CBaseAnimating = 2,
	CAI_BaseNPC = 0,
	CBeam = 24,
	CBaseViewModel = 21,
	CBaseParticleEntity = 14,
	CBaseGrenade = 13,
	CBaseCombatWeapon = 7,
	CBaseWeaponWorldModel = 23,
};

enum FontRenderFlag_t
{
	FONT_LEFT = 0,
	FONT_RIGHT = 1 << 1,
	FONT_CENTER = 1 << 2,
	FONT_OUTLINE = 1 << 3
};

enum FontFeature
{
	FONT_FEATURE_ANTIALIASED_FONTS = 1,
	FONT_FEATURE_DROPSHADOW_FONTS = 2,
	FONT_FEATURE_OUTLINE_FONTS = 6,
};

enum FontDrawType
{
	FONT_DRAW_DEFAULT = 0,
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,
	FONT_DRAW_TYPE_COUNT = 2,
};

enum FontFlags
{
	FONTFLAG_NONE,
	FONTFLAG_ITALIC = 0x001,
	FONTFLAG_UNDERLINE = 0x002,
	FONTFLAG_STRIKEOUT = 0x004,
	FONTFLAG_SYMBOL = 0x008,
	FONTFLAG_ANTIALIAS = 0x010,
	FONTFLAG_GAUSSIANBLUR = 0x020,
	FONTFLAG_ROTARY = 0x040,
	FONTFLAG_DROPSHADOW = 0x080,
	FONTFLAG_ADDITIVE = 0x100,
	FONTFLAG_OUTLINE = 0x200,
	FONTFLAG_CUSTOM = 0x400,
	FONTFLAG_BITMAP = 0x800,
};

enum
{
	MAX_JOYSTICKS = MAX_SPLITSCREEN_CLIENTS,
	MOUSE_BUTTON_COUNT = 5,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_MAX_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_POV_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_AXIS_BUTTON_COUNT - 1),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,
	KEY_XBUTTON_INACTIVE_START,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

enum MouseCodeState_t
{
	BUTTON_RELEASED = 0,
	BUTTON_PRESSED,
	BUTTON_DOUBLECLICKED,
};
#pragma endregion

// 
// unimplemented
// 
#pragma region decl_unimplemented
struct model_t;

class SendProp;
#pragma endregion

// 
// structs
// 
#pragma region decl_structs
struct string_t;
#pragma endregion

// 
// classes
// 
#pragma region decl_classes
class IHandleEntity;
class IClientUnknown;
class ICollideable;
class IClientNetworkable;
class IClientRenderable;
class IClientEntity;
class C_BaseEntity;
class C_BaseAnimating;
class C_BaseCombatCharacter;
class C_BasePlayer;
class C_CSPlayer;
class C_BaseCombatWeapon;
class C_WeaponCSBaseGun;

class DVariant;
class CRecvProxyData;
class RecvProp;
class RecvTable;

class ClientClass;
class CUserCmd;
class CVerifiedUserCmd;
class CGlobalVarsBase;
class CGlobalVars;
class CMoveData;
#pragma endregion

// 
// interfaces
// 
#pragma region decl_interfaces
class IBaseClientDLL;
class IClientEntityList;
class IGameMovement;
class IPrediction;
class IMoveHelper;
class IInput;

class IVEngineClient;

class IPanel;
#pragma endregion

// 
// types
// 
#pragma region decl_types
using CRC32_t = unsigned int;
using VPANEL = unsigned int;
using CBaseHandle = unsigned long;
#pragma endregion

// 
// function types
// 
#pragma region decl_function_types
using CreateInterfaceFn = void* ( * )( const char*, int* );
using RecvVarProxyFn = void ( * )( const CRecvProxyData*, void*, void* );
#pragma endregion


// 
// implementation
// 

#pragma region impl_structs
struct string_t
{
public:
	const char* ToCStr() const;
protected:
	const char* pszValue;
};
#pragma endregion

#pragma region impl_classes
class DVariant
{
public:
	union
	{
		float m_Float;
		int m_Int;
		const char* m_pString;
		void* m_pData;
		float m_Vector[3];
	};
	SendPropType m_Type;
};

class CRecvProxyData
{
public:
	const RecvProp*	m_pRecvProp;
private:
	std::uint8_t __pad[4];
public:
	DVariant m_Value;
	int m_iElement;
	int m_ObjectID;
};

class RecvProp
{
public:
	const char* m_pVarName;
	SendPropType m_RecvType;
	int m_Flags;
	int m_StringBufferSize;
	bool m_bInsideArray;
	const void* m_pExtraData;
	RecvProp* m_pArrayProp;
	void* m_ArrayLengthProxy;
	RecvVarProxyFn m_ProxyFn;
	void* m_DataTableProxyFn;
	RecvTable* m_pDataTable;
	int m_Offset;
	int m_ElementStride;
	int m_nElements;
	const char* m_pParentArrayPropName;
};

class RecvTable
{
public:
	RecvProp* m_pProps;
	int m_nProps;
	void* m_pDecoder;
	const char* m_pNetTableName;
	bool m_bInitialized;
	bool m_bInMainList;
};

typedef IClientNetworkable* (*CreateClientClassFn)(int entnum, int serialNum);
typedef IClientNetworkable* (*CreateEventFn)();

class ClientClass
{
public:
	CreateClientClassFn m_pCreateFn;
	CreateEventFn m_pCreateEventFn;
	const char* m_pNetworkName;
	RecvTable* m_pRecvTable;
	ClientClass* m_pNext;
	int m_ClassID;
};

class CUserCmd
{
public:
	CUserCmd();
	CUserCmd( const CUserCmd& cmd );
public:
	virtual ~CUserCmd();
public:
	void Reset();
	CRC32_t GetChecksum() const;
public:
	CUserCmd& operator = ( const CUserCmd& cmd );
public:
	int command_number = 0;
	int tick_count = 0;
	QAngle viewangles = { };
	Vector aimdirection = { };
	float forwardmove = 0.0f;
	float sidemove = 0.0f;
	float upmove = 0.0f;
	int buttons = 0;
	std::uint8_t impulse = 0u;
	int weaponselect = 0;
	int weaponsubtype = 0;
	int random_seed = 0;
	short mousedx = 0;
	short mousedy = 0;
	bool hasbeenpredicted = false;
	QAngle headangles = { };
	Vector headoffset = { };
};

class CVerifiedUserCmd
{
public:
	CUserCmd m_cmd = { };
	CRC32_t m_crc = 0u;
};

class CGlobalVarsBase
{
public:
	float realtime = 0.0f;
	int framecount = 0;
	float absoluteframetime = 0.0f;
	float absoluteframestarttimestddev = 0.0f;
	float curtime = 0.0f;
	float frametime = 0.0f;
	int maxClients = 0;
	int tickcount = 0;
	float interval_per_tick = 0.0f;
	float interpolation_amount = 0.0f;
	int simTicksThisFrame = 0;
	int network_protocol = 0;
	void* pSaveData = nullptr;
	bool m_bClient = false;
	int nTimestampNetworkingBase = 0;
	int nTimestampRandomizeWindow = 0;
};

class CGlobalVars : public CGlobalVarsBase
{
public:
	string_t mapname = { };
	string_t mapGroupName = { };
	int mapversion = 0;
	string_t startspot = { };
	MapLoadType_t eLoadType = MapLoad_NewGame;
	bool bMapLoadFailed = false;
	bool deathmatch = false;
	bool coop = false;
	bool teamplay = false;
	int maxEntities = 0;
	int serverCount = 0;
	void* pEdicts = nullptr;
};

class CMoveData
{
private:
	std::uint8_t __pad[1024] = { };
};
#pragma endregion

#pragma region impl_interfaces
class IBaseClientDLL
{
public:
	ClientClass* GetAllClasses();
	bool WriteUsercmdDeltaToBuffer(int nSlot, void * buf, int from, int to, bool isNewCmd);
	void DispatchUserMessage(int type, unsigned int a3, unsigned int length, const void *msg_data);
};

class IClientEntityList
{
public:
	virtual IClientNetworkable*	GetClientNetworkable(int entnum) = 0;
	virtual IClientNetworkable*	GetClientNetworkableFromHandle(CBaseHandle hEnt) = 0;
	virtual IClientUnknown*		GetClientUnknownFromHandle(CBaseHandle hEnt) = 0;
	 
	virtual C_BasePlayer*	GetClientEntity(int iIndex) = 0;
	virtual C_BasePlayer*	GetClientEntityFromHandle(CBaseHandle hHandle) = 0;
	virtual int				NumberOfEntities(bool bIncludeNonNetworkable) = 0;
	virtual int				GetHighestEntityIndex() = 0;
};

class IGameMovement
{
public:
	/*void ProcessMovement( C_BasePlayer* pPlayer, CMoveData* pMove );
	void StartTrackPredictionErrors( C_BasePlayer* pPlayer );
	void FinishTrackPredictionErrors( C_BasePlayer* pPlayer );*/
	virtual			~IGameMovement(void) {}

	virtual void	ProcessMovement(C_BasePlayer *pPlayer, CMoveData *pMove) = 0;
	virtual void	Reset(void) = 0;
	virtual void	StartTrackPredictionErrors(C_BasePlayer *pPlayer) = 0;
	virtual void	FinishTrackPredictionErrors(C_BasePlayer *pPlayer) = 0;
	virtual void	DiffPrint(char const *fmt, ...) = 0;

	virtual Vector const&	GetPlayerMins(bool ducked) const = 0;
	virtual Vector const&	GetPlayerMaxs(bool ducked) const = 0;
	virtual Vector const&   GetPlayerViewOffset(bool ducked) const = 0;

	virtual bool			IsMovingPlayerStuck(void) const = 0;
	virtual C_BasePlayer*	GetMovingPlayer(void) const = 0;
	virtual void			UnblockPusher(C_BasePlayer* pPlayer, C_BasePlayer *pPusher) = 0;

	virtual void SetupMovementBounds(CMoveData *pMove) = 0;
};

class IPrediction
{
//private:
//	virtual void UnknownVirtual0() = 0;
//	virtual void UnknownVirtual1() = 0;
//	virtual void UnknownVirtual2() = 0;
//	virtual void UnknownVirtual3() = 0;
//	virtual void UnknownVirtual4() = 0;
//	virtual void UnknownVirtual5() = 0;
//	virtual void UnknownVirtual6() = 0;
//	virtual void UnknownVirtual7() = 0;
//	virtual void UnknownVirtual8() = 0;
//	virtual void UnknownVirtual9() = 0;
//	virtual void UnknownVirtual10() = 0;
//	virtual void UnknownVirtual11() = 0;
//public:
//	virtual void GetLocalViewAngles(Vector& ang);// 12
//	virtual void SetLocalViewAngles(Vector& ang);// 13
//private:
//	virtual void UnknownVirtual14() = 0;
//	virtual void UnknownVirtual15() = 0;
//	virtual void UnknownVirtual16() = 0;
//	virtual void UnknownVirtual17() = 0;
//	virtual void UnknownVirtual18() = 0;
public:
//	//virtual void RunCommand(C_BasePlayer *, CUserCmd *, IMoveHelper *) = 0;
//	virtual void SetupMove(C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move) = 0; //20
//	virtual void FinishMove(C_BasePlayer* player, CUserCmd* ucmd, CMoveData* move) = 0;

	void Update(int startframe,            // World update ( un-modded ) most recently received
		bool validframe,           // Is frame data valid
		int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
		int outgoing_command       // Last command (most recent) sent to server (un-modded)
	)
	{
		typedef void(__thiscall* Fn)(void*, int startframe,	bool validframe, int incoming_acknowledged,	int outgoing_command);
		return ((Fn)Memory::VCall<Fn>(this, 3))(this, startframe, validframe,incoming_acknowledged,outgoing_command);
	}
	void SetLocalViewAngles(QAngle& Angles)
	{
		typedef void(__thiscall* Fn)(void*, QAngle&);
		return ((Fn)Memory::VCall<Fn>(this, 13))(this, Angles);
	}

	void GetLocalViewAngles(QAngle& angle)
	{
		typedef void(__thiscall* Fn)(void*, QAngle&);
		return ((Fn)Memory::VCall<Fn>(this, 12))(this, angle);
	}

	void SetLocalViewAngles(Vector& Angles)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)Memory::VCall<Fn>(this, 13))(this, Angles);
	}

	void GetLocalViewAngles(Vector& angle)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)Memory::VCall<Fn>(this, 12))(this, angle);
	}

	void RunCommand(C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper)
	{
		typedef void(__thiscall* oRunCommand)(void*, C_BasePlayer*, CUserCmd*, IMoveHelper*);
		return Memory::VCall<oRunCommand>(this, 19)(this, player, ucmd, moveHelper);
	}

	void SetupMove(C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper, void* pMoveData)
	{
		typedef void(__thiscall* oSetupMove)(void*, C_BasePlayer*, CUserCmd*, IMoveHelper*, void*);
		return Memory::VCall<oSetupMove>(this, 20)(this, player, ucmd, moveHelper, pMoveData);
	}

	void FinishMove(C_BasePlayer *player, CUserCmd *ucmd, void*pMoveData)
	{
		typedef void(__thiscall* oFinishMove)(void*, C_BasePlayer*, CUserCmd*, void*);
		return Memory::VCall<oFinishMove>(this, 21)(this, player, ucmd, pMoveData);
	}

	bool InPrediction()
	{
		typedef bool(__thiscall* oInPrediction)(void*);
		return Memory::VCall<oInPrediction>(this, 14)(this);
	}

	/*CGlobalVarsBase* get_unpredicted_globals() {
		if (*(BYTE*)(uintptr_t(Source::m_pPrediction) + 8))
			return reinterpret_cast<CGlobalVarsBase*>(uint32_t(this) + 0x4c);

		return Source::m_pGlobalVars;
	}*/
};

class IMoveHelper
{
public:
	bool m_bFirstRunOfFunctions : 1;
	bool m_bGameCodeMovedPlayer : 1;
	int m_nPlayerHandle; // edict index on server, client entity handle on client=
	int m_nImpulseCommand; // Impulse command issued.
	QAngle m_vecViewAngles; // Command view angles (local space)
	QAngle m_vecAbsViewAngles; // Command view angles (world space)
	int m_nButtons; // Attack buttons.
	int m_nOldButtons; // From host_client->oldbuttons;
	float m_flForwardMove;
	float m_flSideMove;
	float m_flUpMove;
	float m_flMaxSpeed;
	float m_flClientMaxSpeed;
	Vector m_vecVelocity; // edict::velocity // Current movement direction.
	QAngle m_vecAngles; // edict::angles
	QAngle m_vecOldAngles;
	float m_outStepHeight; // how much you climbed this move
	Vector m_outWishVel; // This is where you tried 
	Vector m_outJumpVel; // This is your jump velocity
	Vector m_vecConstraintCenter;
	float m_flConstraintRadius;
	float m_flConstraintWidth;
	float m_flConstraintSpeedFactor;
	float m_flUnknown[5];
	Vector m_vecAbsOrigin;

	virtual	void _vpad() = 0;
	virtual void SetHost(C_BasePlayer* host) = 0;
};

class IInput
{
public:
	void * pvftable; //0x0000 
	bool m_fTrackIRAvailable; //0x0004 
	bool m_fMouseInitialized; //0x0005 
	bool m_fMouseActive; //0x0006 
	bool m_fJoystickAdvancedInit; //0x0007 
	char pad_0x0008[0xA4]; //0x0008
	bool m_fCameraInterceptingMouse; //0x00AC 
	bool m_fCameraInThirdPerson; //0x00AD 
	bool m_fCameraMovingWithMouse;
	Vector m_vecCameraOffset;
	bool m_fCameraDistanceMove;
	int m_nCameraOldX;
	int m_nCameraOldY;
	int m_nCameraX;
	int m_nCameraY;
	bool m_CameraIsOrthographic;
	Vector m_angPreviousViewAngles;
	Vector m_angPreviousViewAnglesTilt;
	float m_flLastForwardMove; // 0xEC
	int m_nClearInputState;
	CUserCmd* m_pCommands; // 0xF4
	CVerifiedUserCmd* m_pVerifiedCommands;

	CUserCmd* GetUserCmd( int sequence_number );
	CVerifiedUserCmd* GetVerifiedUserCmd( int sequence_number );
//private:
//	std::uint8_t __pad[236];
//private:
//	CUserCmd* m_pCommands;
//	CVerifiedUserCmd* m_pVerifiedCommands;
};

struct player_info
{
	/*char __pad0[0x8];

	int xuidlow;
	int xuidhigh;

	char name[128];
	int userid;
	char guid[33];

	char __pad1[0x17B];*/
	__int64 unknown; //0x0000
	union {
		__int64 steamID64; //0x0008 - SteamID64
		struct
		{
			__int32 xuidlow;
			__int32 xuidhigh;
		};
	};
	char name[128];       //0x0010 - Player Name
	int  userid;             //0x0090 - Unique Server Identifier
	char guid[20];     //0x0094 - STEAM_X:Y:Z
	char pad_0x00A8[0x10];  //0x00A8
	unsigned long iSteamID; //0x00B8 - SteamID
	char szFriendsName[128];
	bool fakeplayer;
	bool ishltv;
	unsigned int customfiles[4];
	unsigned char filesdownloaded;
};

typedef struct InputContextHandle_t__ *InputContextHandle_t;
struct client_textmessage_t;
struct model_t;
class SurfInfo;
class IMaterial;
class CSentence;
class CAudioSource;
class AudioState_t;
class ISpatialQuery;
class IMaterialSystem;
class CPhysCollide;
class IAchievementMgr;
class ISPSharedMemory;
class CGamestatsData;

struct FileHandle_t;

class KeyValues
{
public:
	//	By default, the KeyValues class uses a string table for the key names that is
	//	limited to 4MB. The game will exit in error if this space is exhausted. In
	//	general this is preferable for game code for performance and memory fragmentation
	//	reasons.
	//
	//	If this is not acceptable, you can use this call to switch to a table that can grow
	//	arbitrarily. This call must be made before any KeyValues objects are allocated or it
	//	will result in undefined behavior. If you use the growable string table, you cannot
	//	share KeyValues pointers directly with any other module. You can serialize them across
	//	module boundaries. These limitations are acceptable in the Steam backend code 
	//	this option was written for, but may not be in other situations. Make sure to
	//	understand the implications before using this.
	static void SetUseGrowableStringTable(bool bUseGrowableTable);

	KeyValues(const char *setName) {}

	//
	// AutoDelete class to automatically free the keyvalues.
	// Simply construct it with the keyvalues you allocated and it will free them when falls out of scope.
	// When you decide that keyvalues shouldn't be deleted call Assign(NULL) on it.
	// If you constructed AutoDelete(NULL) you can later assign the keyvalues to be deleted with Assign(pKeyValues).
	// You can also pass temporary KeyValues object as an argument to a function by wrapping it into KeyValues::AutoDelete
	// instance:   call_my_function( KeyValues::AutoDelete( new KeyValues( "test" ) ) )
	//
	class AutoDelete
	{
	public:
		explicit inline AutoDelete(KeyValues *pKeyValues) : m_pKeyValues(pKeyValues) {}
		explicit inline AutoDelete(const char *pchKVName) : m_pKeyValues(new KeyValues(pchKVName)) {}
		inline ~AutoDelete(void) { if (m_pKeyValues) m_pKeyValues->deleteThis(); }
		inline void Assign(KeyValues *pKeyValues) { m_pKeyValues = pKeyValues; }
		KeyValues *operator->() { return m_pKeyValues; }
		operator KeyValues *() { return m_pKeyValues; }
	private:
		AutoDelete(AutoDelete const &x); // forbid
		AutoDelete & operator= (AutoDelete const &x); // forbid
		KeyValues *m_pKeyValues;
	};

	// Quick setup constructors
	KeyValues(const char *setName, const char *firstKey, const char *firstValue);
	KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue);

	// Section name
	const char *GetName() const;
	void SetName(const char *setName);

	// gets the name as a unique int
	int GetNameSymbol() const { return m_iKeyName; }

	// File access. Set UsesEscapeSequences true, if resource file/buffer uses Escape Sequences (eg \n, \t)
	void UsesEscapeSequences(bool state); // default false
	void UsesConditionals(bool state); // default true
	bool LoadFromFile(void *filesystem, const char *resourceName, const char *pathID = NULL);
	bool SaveToFile(void *filesystem, const char *resourceName, const char *pathID = NULL, bool sortKeys = false, bool bAllowEmptyString = false);

	// Read from a buffer...  Note that the buffer must be null terminated
	bool LoadFromBuffer(char const *resourceName, const char *pBuffer, void* pFileSystem = NULL, const char *pPathID = NULL);

	// Read from a utlbuffer...
	bool LoadFromBuffer(char const *resourceName, void*buf, void* pFileSystem = NULL, const char *pPathID = NULL);

	// Find a keyValue, create it if it is not found.
	// Set bCreate to true to create the key if it doesn't already exist (which ensures a valid pointer will be returned)
	KeyValues *FindKey(const char *keyName, bool bCreate = false);
	KeyValues *FindKey(int keySymbol) const;
	KeyValues *CreateNewKey();		// creates a new key, with an autogenerated name.  name is guaranteed to be an integer, of value 1 higher than the highest other integer key name
	void AddSubKey(KeyValues *pSubkey);	// Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
	void RemoveSubKey(KeyValues *subKey);	// removes a subkey from the list, DOES NOT DELETE IT

											// Key iteration.
											//
											// NOTE: GetFirstSubKey/GetNextKey will iterate keys AND values. Use the functions 
											// below if you want to iterate over just the keys or just the values.
											//
	KeyValues *GetFirstSubKey() { return m_pSub; }	// returns the first subkey in the list
	KeyValues *GetNextKey() { return m_pPeer; }		// returns the next subkey
	void SetNextKey(KeyValues * pDat);
	KeyValues *FindLastSubKey();	// returns the LAST subkey in the list.  This requires a linked list iteration to find the key.  Returns NULL if we don't have any children

									//
									// These functions can be used to treat it like a true key/values tree instead of 
									// confusing values with keys.
									//
									// So if you wanted to iterate all subkeys, then all values, it would look like this:
									//     for ( KeyValues *pKey = pRoot->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
									//     {
									//		   Msg( "Key name: %s\n", pKey->GetName() );
									//     }
									//     for ( KeyValues *pValue = pRoot->GetFirstValue(); pKey; pKey = pKey->GetNextValue() )
									//     {
									//         Msg( "Int value: %d\n", pValue->GetInt() );  // Assuming pValue->GetDataType() == TYPE_INT...
									//     }
	KeyValues* GetFirstTrueSubKey();
	KeyValues* GetNextTrueSubKey();

	KeyValues* GetFirstValue();	// When you get a value back, you can use GetX and pass in NULL to get the value.
	KeyValues* GetNextValue();


	// Data access
	int   GetInt(const char *keyName = NULL, int defaultValue = 0);
	uint64 GetUint64(const char *keyName = NULL, uint64 defaultValue = 0);
	float GetFloat(const char *keyName = NULL, float defaultValue = 0.0f);
	const char *GetString(const char *keyName = NULL, const char *defaultValue = "");
	const wchar_t *GetWString(const char *keyName = NULL, const wchar_t *defaultValue = L"");
	void *GetPtr(const char *keyName = NULL, void *defaultValue = (void*)0);
	bool GetBool(const char *keyName = NULL, bool defaultValue = false);
	float* GetColor(const char *keyName = NULL /* default value is all black */);
	bool  IsEmpty(const char *keyName = NULL);

	// Data access
	int   GetInt(int keySymbol, int defaultValue = 0);
	float GetFloat(int keySymbol, float defaultValue = 0.0f);
	const char *GetString(int keySymbol, const char *defaultValue = "");
	const wchar_t *GetWString(int keySymbol, const wchar_t *defaultValue = L"");
	void *GetPtr(int keySymbol, void *defaultValue = (void*)0);
	float* GetColor(int keySymbol /* default value is all black */);
	bool  IsEmpty(int keySymbol);

	// Key writing
	void SetWString(const char *keyName, const wchar_t *value);
	void SetString(const char *keyName, const char *value);
	void SetInt(const char *keyName, int value);
	void SetUint64(const char *keyName, uint64 value);
	void SetFloat(const char *keyName, float value);
	void SetPtr(const char *keyName, void *value);
	void SetColor(const char *keyName, float* value);
	void SetBool(const char *keyName, bool value) { SetInt(keyName, value ? 1 : 0); }

	// Adds a chain... if we don't find stuff in this keyvalue, we'll look
	// in the one we're chained to.
	void ChainKeyValue(KeyValues* pChain);

	void RecursiveSaveToFile(void* buf, int indentLevel, bool sortKeys = false, bool bAllowEmptyString = false);

	bool WriteAsBinary(void*buffer);
	bool ReadAsBinary(void*buffer, int nStackDepth = 0);

	// Allocate & create a new copy of the keys
	KeyValues *MakeCopy(void) const;

	// Make a new copy of all subkeys, add them all to the passed-in keyvalues
	void CopySubkeys(KeyValues *pParent) const;

	// Clear out all subkeys, and the current value
	void Clear(void);

	// Data type
	enum types_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
		TYPE_NUMTYPES,
	};
	types_t GetDataType(const char *keyName = NULL);

	// Virtual deletion function - ensures that KeyValues object is deleted from correct heap
	void deleteThis();

	void SetStringValue(char const *strValue);

	// unpack a key values list into a structure
	void UnpackIntoStructure(struct KeyValuesUnpackStructure const *pUnpackTable, void *pDest, size_t DestSizeInBytes);

	// Process conditional keys for widescreen support.
	bool ProcessResolutionKeys(const char *pResString);

	// Dump keyvalues recursively into a dump context
	bool Dump(class IKeyValuesDumpContext *pDump, int nIndentLevel = 0);

	// Merge in another KeyValues, keeping "our" settings
	void RecursiveMergeKeyValues(KeyValues *baseKV);

private:
	KeyValues(KeyValues&);	// prevent copy constructor being used

							// prevent delete being called except through deleteThis()
	~KeyValues();

	KeyValues* CreateKey(const char *keyName);

	/// Create a child key, given that we know which child is currently the last child.
	/// This avoids the O(N^2) behaviour when adding children in sequence to KV,
	/// when CreateKey() wil have to re-locate the end of the list each time.  This happens,
	/// for example, every time we load any KV file whatsoever.
	KeyValues* CreateKeyUsingKnownLastChild(const char *keyName, KeyValues *pLastChild);
	void AddSubkeyUsingKnownLastChild(KeyValues *pSubKey, KeyValues *pLastChild);

	void RecursiveCopyKeyValues(KeyValues& src);
	void RemoveEverything();
	//	void RecursiveSaveToFile( IBaseFileSystem *filesystem, void*buffer, int indentLevel );
	//	void WriteConvertedString( void*buffer, const char *pszString );

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void RecursiveSaveToFile(void *filesystem, FileHandle_t f, void *pBuf, int indentLevel, bool sortKeys, bool bAllowEmptyString);
	void SaveKeyToFile(KeyValues *dat, void *filesystem, FileHandle_t f, void *pBuf, int indentLevel, bool sortKeys, bool bAllowEmptyString);
	void WriteConvertedString(void *filesystem, FileHandle_t f, void *pBuf, const char *pszString);

	void RecursiveLoadFromBuffer(char const *resourceName, void*buf);

	// For handling #include "filename"
	void AppendIncludedKeys(void* includedKeys);
	void ParseIncludedKeys(char const *resourceName, const char *filetoinclude,
		void* pFileSystem, const char *pPathID, void* includedKeys);

	// For handling #base "filename"
	void MergeBaseKeys(void* baseKeys);

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void InternalWrite(void *filesystem, FileHandle_t f, void *pBuf, const void *pData, int len);

	void Init();
	const char * ReadToken(void*buf, bool &wasQuoted, bool &wasConditional);
	void WriteIndents(void *filesystem, FileHandle_t f, void *pBuf, int indentLevel);

	void FreeAllocatedValue();
	void AllocateValueBlock(int size);

	int m_iKeyName;	// keyname is a symbol defined in KeyValuesSystem

					// These are needed out of the union because the API returns string pointers
	char *m_sValue;
	wchar_t *m_wsValue;

	// we don't delete these
	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
		unsigned char m_Color[4];
	};

	char	   m_iDataType;
	char	   m_bHasEscapeSequences; // true, if while parsing this KeyValue, Escape Sequences are used (default false)
	char	   m_bEvaluateConditionals; // true, if while parsing this KeyValue, conditionals blocks are evaluated (default true)
	char	   unused[1];

	KeyValues *m_pPeer;	// pointer to next key in list
	KeyValues *m_pSub;	// pointer to Start of a new sub key list
	KeyValues *m_pChain;// Search here if it's not in our list

private:
	// Statics to implement the optional growable string table
	// Function pointers that will determine which mode we are in
	static int(*s_pfGetSymbolForString)(const char *name, bool bCreate);
	static const char *(*s_pfGetStringForSymbol)(int symbol);
	static void *s_pGrowableStringTable;

public:
	// Functions that invoke the default behavior
	static int GetSymbolForStringClassic(const char *name, bool bCreate = true);
	static const char *GetStringForSymbolClassic(int symbol);

	// Functions that use the growable string table
	static int GetSymbolForStringGrowable(const char *name, bool bCreate = true);
	static const char *GetStringForSymbolGrowable(int symbol);

	// Functions to get external access to whichever of the above functions we're going to call.
	static int CallGetSymbolForString(const char *name, bool bCreate = true) { return s_pfGetSymbolForString(name, bCreate); }
	static const char *CallGetStringForSymbol(int symbol) { return s_pfGetStringForSymbol(symbol); }
};

class CSteamAPIContext;
struct Frustum_t;
class pfnDemoCustomDataCallback;

class INetChannelInfo
{
public:

	enum
	{
		GENERIC = 0, // must be first and is default group
		LOCALPLAYER, // bytes for local player entity update
		OTHERPLAYERS, // bytes for other players update
		ENTITIES, // all other entity bytes
		SOUNDS, // game sounds
		EVENTS, // event messages
		USERMESSAGES, // user messages
		ENTMESSAGES, // entity messages
		VOICE, // voice data
		STRINGTABLE, // a stringtable update
		MOVE, // client move cmds
		STRINGCMD, // string command
		SIGNON, // various signondata
		TOTAL, // must be last and is not a real group
	};

	virtual const char* GetName(void) const = 0; // get channel name
	virtual const char* GetAddress(void) const = 0; // get channel IP address as string
	virtual float GetTime(void) const = 0; // current net time
	virtual float GetTimeConnected(void) const = 0; // get connection time in seconds
	virtual int GetBufferSize(void) const = 0; // netchannel packet history size
	virtual int GetDataRate(void) const = 0; // send data rate in byte/sec

	virtual bool IsLoopback(void) const = 0; // true if loopback channel
	virtual bool IsTimingOut(void) const = 0; // true if timing out
	virtual bool IsPlayback(void) const = 0; // true if demo playback

	virtual float GetLatency(int flow) const = 0; // current latency (RTT), more accurate but jittering
	virtual float GetAvgLatency(int flow) const = 0; // average packet latency in seconds
	virtual float GetAvgLoss(int flow) const = 0; // avg packet loss[0..1]
	virtual float GetAvgChoke(int flow) const = 0; // avg packet choke[0..1]
	virtual float GetAvgData(int flow) const = 0; // data flow in bytes/sec
	virtual float GetAvgPackets(int flow) const = 0; // avg packets/sec
	virtual int GetTotalData(int flow) const = 0; // total flow in/out in bytes
	virtual int GetSequenceNr(int flow) const = 0; // last send seq number
	virtual bool IsValidPacket(int flow, int frame_number) const = 0; // true if packet was not lost/dropped/chocked/flushed
	virtual float GetPacketTime(int flow, int frame_number) const = 0; // time when packet was send
	virtual int GetPacketBytes(int flow, int frame_number, int group) const = 0; // group size of this packet
	virtual bool GetStreamProgress(int flow, int* received, int* total) const = 0; // TCP progress if transmitting
	virtual float GetTimeSinceLastReceived(void) const = 0; // get time since last recieved packet in seconds
	virtual float GetCommandInterpolationAmount(int flow, int frame_number) const = 0;

	virtual void GetPacketResponseLatency(int flow, int frame_number, int* pnLatencyMsecs, int* pnChoke) const = 0;

	virtual void GetRemoteFramerate(float* pflFrameTime, float* pflFrameTimeStdDeviation) const = 0;

	virtual float GetTimeoutSeconds() const = 0;
};

class IVEngineClient
{
public:
	virtual int                   GetIntersectingSurfaces(const model_t *model, const Vector &vCenter, const float radius, const bool bOnlyVisibleSurfaces, SurfInfo *pInfos, const int nMaxInfos) = 0;
	virtual Vector                GetLightForPoint(const Vector &pos, bool bClamp) = 0;
	virtual IMaterial*            TraceLineMaterialAndLighting(const Vector &start, const Vector &end, Vector &diffuseLightColor, Vector& baseColor) = 0;
	virtual const char*           ParseFile(const char *data, char *token, int maxlen) = 0;
	virtual bool                  CopyFile(const char *source, const char *destination) = 0;
	virtual void                  GetScreenSize(int& width, int& height) = 0;
	virtual void                  ServerCmd(const char *szCmdString, bool bReliable = true) = 0;
	virtual void                  ClientCmd(const char *szCmdString) = 0;
	virtual bool                  GetPlayerInfo(int ent_num, player_info *pinfo) = 0;
	virtual int                   GetPlayerForUserID(int userID) = 0;
	virtual client_textmessage_t* TextMessageGet(const char *pName) = 0; // 10
	virtual bool                  Con_IsVisible(void) = 0;
	virtual int                   GetLocalPlayer(void) = 0;
	virtual const model_t*        LoadModel(const char *pName, bool bProp = false) = 0;
	virtual float                 GetLastTimeStamp(void) = 0;
	virtual CSentence*            GetSentence(CAudioSource *pAudioSource) = 0; // 15
	virtual float                 GetSentenceLength(CAudioSource *pAudioSource) = 0;
	virtual bool                  IsStreaming(CAudioSource *pAudioSource) const = 0;
	virtual void                  GetViewAngles(QAngle& va) = 0;
	virtual void                  SetViewAngles(QAngle& va) = 0;
	virtual int                   GetMaxClients(void) = 0; // 20
	virtual   const char*         Key_LookupBinding(const char *pBinding) = 0;
	virtual const char*           Key_BindingForKey(int &code) = 0;
	virtual void                  Key_SetBinding(int, char const*) = 0;
	virtual void                  StartKeyTrapMode(void) = 0;
	virtual bool                  CheckDoneKeyTrapping(int &code) = 0;
	virtual bool                  IsInGame(void) = 0;
	virtual bool                  IsConnected(void) = 0;
	virtual bool                  IsDrawingLoadingImage(void) = 0;
	virtual void                  HideLoadingPlaque(void) = 0;
	virtual void                  Con_NPrintf(int pos, const char *fmt, ...) = 0; // 30
	virtual void                  Con_NXPrintf(const struct con_nprint_s *info, const char *fmt, ...) = 0;
	virtual int                   IsBoxVisible(const Vector& mins, const Vector& maxs) = 0;
	virtual int                   IsBoxInViewCluster(const Vector& mins, const Vector& maxs) = 0;
	virtual bool                  CullBox(const Vector& mins, const Vector& maxs) = 0;
	virtual void                  Sound_ExtraUpdate(void) = 0;
	virtual const char*           GetGameDirectory(void) = 0;
	virtual const VMatrix&        WorldToScreenMatrix() = 0;
	virtual const VMatrix&        WorldToViewMatrix() = 0;
	virtual int                   GameLumpVersion(int lumpId) const = 0;
	virtual int                   GameLumpSize(int lumpId) const = 0; // 40
	virtual bool                  LoadGameLump(int lumpId, void* pBuffer, int size) = 0;
	virtual int                   LevelLeafCount() const = 0;
	virtual ISpatialQuery*        GetBSPTreeQuery() = 0;
	virtual void                  LinearToGamma(float* linear, float* gamma) = 0;
	virtual float                 LightStyleValue(int style) = 0; // 45
	virtual void                  ComputeDynamicLighting(const Vector& pt, const Vector* pNormal, Vector& color) = 0;
	virtual void                  GetAmbientLightColor(Vector& color) = 0;
	virtual int                   GetDXSupportLevel() = 0;
	virtual bool                  SupportsHDR() = 0;
	virtual void                  Mat_Stub(IMaterialSystem *pMatSys) = 0; // 50
	virtual void                  GetChapterName(char *pchBuff, int iMaxLength) = 0;
	virtual char const*           GetLevelName(void) = 0;
	virtual char const*           GetLevelNameShort(void) = 0;
	virtual char const*           GetMapGroupName(void) = 0;
	virtual struct IVoiceTweak_s* GetVoiceTweakAPI(void) = 0;
	virtual void                  SetVoiceCasterID(unsigned int someint) = 0; // 56
	virtual void                  EngineStats_BeginFrame(void) = 0;
	virtual void                  EngineStats_EndFrame(void) = 0;
	virtual void                  FireEvents() = 0;
	virtual int                   GetLeavesArea(unsigned short *pLeaves, int nLeaves) = 0;
	virtual bool                  DoesBoxTouchAreaFrustum(const Vector &mins, const Vector &maxs, int iArea) = 0; // 60
	virtual int                   GetFrustumList(Frustum_t **pList, int listMax) = 0;
	virtual bool                  ShouldUseAreaFrustum(int i) = 0;
	virtual void                  SetAudioState(const AudioState_t& state) = 0;
	virtual int                   SentenceGroupPick(int groupIndex, char *name, int nameBufLen) = 0;
	virtual int                   SentenceGroupPickSequential(int groupIndex, char *name, int nameBufLen, int sentenceIndex, int reset) = 0;
	virtual int                   SentenceIndexFromName(const char *pSentenceName) = 0;
	virtual const char*           SentenceNameFromIndex(int sentenceIndex) = 0;
	virtual int                   SentenceGroupIndexFromName(const char *pGroupName) = 0;
	virtual const char*           SentenceGroupNameFromIndex(int groupIndex) = 0;
	virtual float                 SentenceLength(int sentenceIndex) = 0;
	virtual void                  ComputeLighting(const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color, Vector *pBoxColors = NULL) = 0;
	virtual void                  ActivateOccluder(int nOccluderIndex, bool bActive) = 0;
	virtual bool                  IsOccluded(const Vector &vecAbsMins, const Vector &vecAbsMaxs) = 0; // 74
	virtual int                   GetOcclusionViewId(void) = 0;
	virtual void*                 SaveAllocMemory(size_t num, size_t size) = 0;
	virtual void                  SaveFreeMemory(void *pSaveMem) = 0;
	virtual INetChannelInfo*      GetNetChannelInfo(void) = 0;
	virtual void                  DebugDrawPhysCollide() = 0; //79
	virtual void                  CheckPoint(const char *pName) = 0; // 80
	virtual void                  DrawPortals() = 0;
	virtual bool                  IsPlayingDemo(void) = 0;
	virtual bool                  IsRecordingDemo(void) = 0;
	virtual bool                  IsPlayingTimeDemo(void) = 0;
	virtual int                   GetDemoRecordingTick(void) = 0;
	virtual int                   GetDemoPlaybackTick(void) = 0;
	virtual int                   GetDemoPlaybackStartTick(void) = 0;
	virtual float                 GetDemoPlaybackTimeScale(void) = 0;
	virtual int                   GetDemoPlaybackTotalTicks(void) = 0;
	virtual bool                  IsPaused(void) = 0; // 90
	virtual float                 GetTimescale(void) const = 0;
	virtual bool                  IsTakingScreenshot(void) = 0;
	virtual bool                  IsHLTV(void) = 0;
	virtual bool                  IsLevelMainMenuBackground(void) = 0;
	virtual void                  GetMainMenuBackgroundName(char *dest, int destlen) = 0;
	virtual void                  SetOcclusionParameters(const int /*OcclusionParams_t*/ &params) = 0; // 96
	virtual void                  GetUILanguage(char *dest, int destlen) = 0;
	virtual int                   IsSkyboxVisibleFromPoint(const Vector &vecPoint) = 0;
	virtual const char*           GetMapEntitiesString() = 0;
	virtual bool                  IsInEditMode(void) = 0; // 100
	virtual float                 GetScreenAspectRatio(int viewportWidth, int viewportHeight) = 0;
	virtual bool                  REMOVED_SteamRefreshLogin(const char *password, bool isSecure) = 0; // 100
	virtual bool                  REMOVED_SteamProcessCall(bool & finished) = 0;
	virtual unsigned int          GetEngineBuildNumber() = 0; // engines build
	virtual const char *          GetProductVersionString() = 0; // mods version number (steam.inf)
	virtual void                  GrabPreColorCorrectedFrame(int x, int y, int width, int height) = 0;
	virtual bool                  IsHammerRunning() const = 0;
	virtual void                  ExecuteClientCmd(const char *szCmdString) = 0; //108
	virtual bool                  MapHasHDRLighting(void) = 0;
	virtual bool                  MapHasLightMapAlphaData(void) = 0;
	virtual int                   GetAppID() = 0;
	virtual Vector                GetLightForPointFast(const Vector &pos, bool bClamp) = 0;
	virtual void                  ClientCmd_Unrestricted1(char  const*, int, bool);
	virtual void                  ClientCmd_Unrestricted(const char *szCmdString, const char *newFlag = 0) = 0; // 114, new flag, quick testing shows setting 0 seems to work, haven't looked into it.
																												//Forgot to add this line, but make sure to format all unrestricted calls now with an extra , 0
																												//Ex:
																												//	I::Engine->ClientCmd_Unrestricted( charenc( "cl_mouseenable 1" ) , 0);
																												//	I::Engine->ClientCmd_Unrestricted( charenc( "crosshair 1" ) , 0);
	virtual void                  SetRestrictServerCommands(bool bRestrict) = 0;
	virtual void                  SetRestrictClientCommands(bool bRestrict) = 0;
	virtual void                  SetOverlayBindProxy(int iOverlayID, void *pBindProxy) = 0;
	virtual bool                  CopyFrameBufferToMaterial(const char *pMaterialName) = 0;
	virtual void                  ReadConfiguration(const int iController, const bool readDefault) = 0;
	virtual void                  SetAchievementMgr(IAchievementMgr *pAchievementMgr) = 0;
	virtual IAchievementMgr*      GetAchievementMgr() = 0;
	virtual bool                  MapLoadFailed(void) = 0;
	virtual void                  SetMapLoadFailed(bool bState) = 0;
	virtual bool                  IsLowViolence() = 0;
	virtual const char*           GetMostRecentSaveGame(void) = 0;
	virtual void                  SetMostRecentSaveGame(const char *lpszFilename) = 0;
	virtual void                  StartXboxExitingProcess() = 0;
	virtual bool                  IsSaveInProgress() = 0;
	virtual bool                  IsAutoSaveDangerousInProgress(void) = 0;
	virtual unsigned int          OnStorageDeviceAttached(int iController) = 0;
	virtual void                  OnStorageDeviceDetached(int iController) = 0;
	virtual char* const           GetSaveDirName(void) = 0;
	virtual void                  WriteScreenshot(const char *pFilename) = 0;
	virtual void                  ResetDemoInterpolation(void) = 0;
	virtual int                   GetActiveSplitScreenPlayerSlot() = 0;
	virtual int                   SetActiveSplitScreenPlayerSlot(int slot) = 0;
	virtual bool                  SetLocalPlayerIsResolvable(char const *pchContext, int nLine, bool bResolvable) = 0;
	virtual bool                  IsLocalPlayerResolvable() = 0;
	virtual int                   GetSplitScreenPlayer(int nSlot) = 0;
	virtual bool                  IsSplitScreenActive() = 0;
	virtual bool                  IsValidSplitScreenSlot(int nSlot) = 0;
	virtual int                   FirstValidSplitScreenSlot() = 0; // -1 == invalid
	virtual int                   NextValidSplitScreenSlot(int nPreviousSlot) = 0; // -1 == invalid
	virtual ISPSharedMemory*      GetSinglePlayerSharedMemorySpace(const char *szName, int ent_num = (1 << 11)) = 0;
	virtual void                  ComputeLightingCube(const Vector& pt, bool bClamp, Vector *pBoxColors) = 0;
	virtual void                  RegisterDemoCustomDataCallback(const char* szCallbackSaveID, pfnDemoCustomDataCallback pCallback) = 0;
	virtual void                  RecordDemoCustomData(pfnDemoCustomDataCallback pCallback, const void *pData, size_t iDataLength) = 0;
	virtual void                  SetPitchScale(float flPitchScale) = 0;
	virtual float                 GetPitchScale(void) = 0;
	virtual bool                  LoadFilmmaker() = 0;
	virtual void                  UnloadFilmmaker() = 0;
	virtual void                  SetLeafFlag(int nLeafIndex, int nFlagBits) = 0;
	virtual void                  RecalculateBSPLeafFlags(void) = 0;
	virtual bool                  DSPGetCurrentDASRoomNew(void) = 0;
	virtual bool                  DSPGetCurrentDASRoomChanged(void) = 0;
	virtual bool                  DSPGetCurrentDASRoomSkyAbove(void) = 0;
	virtual float                 DSPGetCurrentDASRoomSkyPercent(void) = 0;
	virtual void                  SetMixGroupOfCurrentMixer(const char *szgroupname, const char *szparam, float val, int setMixerType) = 0;
	virtual int                   GetMixLayerIndex(const char *szmixlayername) = 0;
	virtual void                  SetMixLayerLevel(int index, float level) = 0;
	virtual int                   GetMixGroupIndex(char  const* groupname) = 0;
	virtual void                  SetMixLayerTriggerFactor(int i1, int i2, float fl) = 0;
	virtual void                  SetMixLayerTriggerFactor(char  const* char1, char  const* char2, float fl) = 0;
	virtual bool                  IsCreatingReslist() = 0;
	virtual bool                  IsCreatingXboxReslist() = 0;
	virtual void                  SetTimescale(float flTimescale) = 0;
	virtual void                  SetGamestatsData(CGamestatsData *pGamestatsData) = 0;
	virtual CGamestatsData*       GetGamestatsData() = 0;
	virtual void                  GetMouseDelta(int &dx, int &dy, bool b) = 0; // unknown
	virtual   const char*         Key_LookupBindingEx(const char *pBinding, int iUserId = -1, int iStartCount = 0, int iAllowJoystick = -1) = 0;
	virtual int                   Key_CodeForBinding(char  const*, int, int, int) = 0;
	virtual void                  UpdateDAndELights(void) = 0;
	virtual int                   GetBugSubmissionCount() const = 0;
	virtual void                  ClearBugSubmissionCount() = 0;
	virtual bool                  DoesLevelContainWater() const = 0;
	virtual float                 GetServerSimulationFrameTime() const = 0;
	virtual void                  SolidMoved(class IClientEntity *pSolidEnt, class ICollideable *pSolidCollide, const Vector* pPrevAbsOrigin, bool accurateBboxTriggerChecks) = 0;
	virtual void                  TriggerMoved(class IClientEntity *pTriggerEnt, bool accurateBboxTriggerChecks) = 0;
	virtual void                  ComputeLeavesConnected(const Vector &vecOrigin, int nCount, const int *pLeafIndices, bool *pIsConnected) = 0;
	virtual bool                  IsInCommentaryMode(void) = 0;
	virtual void                  SetBlurFade(float amount) = 0;
	virtual bool                  IsTransitioningToLoad() = 0;
	virtual void                  SearchPathsChangedAfterInstall() = 0;
	virtual void                  ConfigureSystemLevel(int nCPULevel, int nGPULevel) = 0;
	virtual void                  SetConnectionPassword(char const *pchCurrentPW) = 0;
	virtual CSteamAPIContext*     GetSteamAPIContext() = 0;
	virtual void                  SubmitStatRecord(char const *szMapName, unsigned int uiBlobVersion, unsigned int uiBlobSize, const void *pvBlob) = 0;
	virtual void                  ServerCmdKeyValues(KeyValues *pKeyValues) = 0; // 203
	virtual void                  SpherePaintSurface(const model_t* model, const Vector& location, unsigned char chr, float fl1, float fl2) = 0;
	virtual bool                  HasPaintmap(void) = 0;
	virtual void                  EnablePaintmapRender() = 0;
	//virtual void                TracePaintSurface( const model_t *model, const Vector& position, float radius, CUtlVector<Color>& surfColors ) = 0;
	virtual void                  SphereTracePaintSurface(const model_t* model, const Vector& position, const Vector &vec2, float radius, /*CUtlVector<unsigned char, CUtlMemory<unsigned char, int>>*/ int& utilVecShit) = 0;
	virtual void                  RemoveAllPaint() = 0;
	virtual void                  PaintAllSurfaces(unsigned char uchr) = 0;
	virtual void                  RemovePaint(const model_t* model) = 0;
	virtual bool                  IsActiveApp() = 0;
	virtual bool                  IsClientLocalToActiveServer() = 0;
	virtual void                  TickProgressBar() = 0;
	virtual InputContextHandle_t  GetInputContext(int /*EngineInputContextId_t*/ id) = 0;
	virtual void                  GetStartupImage(char* filename, int size) = 0;
	virtual bool                  IsUsingLocalNetworkBackdoor(void) = 0;
	virtual void                  SaveGame(const char*, bool, char*, int, char*, int) = 0;
	virtual void                  GetGenericMemoryStats(/* GenericMemoryStat_t */ void **) = 0;
	virtual bool                  GameHasShutdownAndFlushedMemory(void) = 0;
	virtual int                   GetLastAcknowledgedCommand(void) = 0;
	virtual void                  FinishContainerWrites(int i) = 0;
	virtual void                  FinishAsyncSave(void) = 0;
	virtual int                   GetServerTick(void) = 0;
	virtual const char*           GetModDirectory(void) = 0;
	virtual bool                  AudioLanguageChanged(void) = 0;
	virtual bool                  IsAutoSaveInProgress(void) = 0;
	virtual void                  StartLoadingScreenForCommand(const char* command) = 0;
	virtual void                  StartLoadingScreenForKeyValues(KeyValues* values) = 0;
	virtual void                  SOSSetOpvarFloat(const char*, float) = 0;
	virtual void                  SOSGetOpvarFloat(const char*, float &) = 0;
	virtual bool                  IsSubscribedMap(const char*, bool) = 0;
	virtual bool                  IsFeaturedMap(const char*, bool) = 0;
	virtual void                  GetDemoPlaybackParameters(void) = 0;
	virtual int                   GetClientVersion(void) = 0;
	virtual bool                  IsDemoSkipping(void) = 0;
	virtual void                  SetDemoImportantEventData(const KeyValues* values) = 0;
	virtual void                  ClearEvents(void) = 0;
	virtual int                   GetSafeZoneXMin(void) = 0;
	virtual bool                  IsVoiceRecording(void) = 0;
	virtual void                  ForceVoiceRecordOn(void) = 0;
	virtual bool                  IsReplay(void) = 0;

	void SetViewAngles(Vector& Angles)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)Memory::VCall<Fn>(this, 19))(this, Angles);
	}

	void GetViewAngles(Vector& angle)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)Memory::VCall<Fn>(this, 18))(this, angle);
	}
};

class IPanel
{
public:
	const char* GetName( VPANEL vguiPanel );
};

namespace vgui
{
	typedef unsigned long HFont;
	typedef unsigned int VPANEL;
};

struct IntRect
{
	int x0;
	int y0;
	int x1;
	int y1;
};

struct Vertex_t
{
	Vertex_t() {}
	Vertex_t(const Vector2D &pos, const Vector2D &coord = Vector2D(0, 0))
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	void Init(const Vector2D &pos, const Vector2D &coord = Vector2D(0, 0))
	{
		m_Position = pos;
		m_TexCoord = coord;
	}

	Vector2D m_Position;
	Vector2D m_TexCoord;
};

class Color
{
public:
	Color()
	{
		*((int *)this) = 0;
	}
	Color(int color32)
	{
		*((int *)this) = color32;
	}
	Color(int _r, int _g, int _b)
	{
		SetColor(_r, _g, _b, 255);
	}
	Color(int _r, int _g, int _b, int _a)
	{
		if (_a < 0) _a = 0;
		else if (_a > 255) _a = 255;

		if (_r < 0) _r = 0;
		else if (_r > 255) _r = 255;

		if (_g < 0) _g = 0;
		else if (_g > 255) _g = 255;

		if (_b < 0) _b = 0;
		else if (_b > 255) _b = 255;

		SetColor(_r, _g, _b, _a);
	}
	Color(uint32_t value)
	{
		_color[3] = (value >> 24) & 0xff;
		_color[0] = (value >> 16) & 0xff;
		_color[1] = (value >> 8) & 0xff;
		_color[2] = (value >> 0) & 0xff;
	}

	Color alpha(int _a)
	{
		if (_a > 255)
			_a = 255;
		else if (_a < 0)
			_a = 0;

		return Color(this->r(),this->g(),this->b(), (unsigned char)_a);
	}

	Color malpha(float _a)
	{
		if (_a > 1.0f)
			_a = 1.f;
		else if (_a < 0.f)
			_a = 0.f;

		return Color(this->r(), this->g(), this->b(), this->a() * _a);
	}

	void SetColor(int _r, int _g, int _b, int _a = 255)
	{
		_color[0] = (unsigned char)_r;
		_color[1] = (unsigned char)_g;
		_color[2] = (unsigned char)_b;
		_color[3] = (unsigned char)_a;
	}

	void GetColor(int &_r, int &_g, int &_b, int &_a) const
	{
		_r = _color[0];
		_g = _color[1];
		_b = _color[2];
		_a = _color[3];
	}

	void SetRawColor(int color32)
	{
		*((int*)this) = color32;
	}

	int GetRawColor() const
	{
		return *((int*)this);
	}

	int GetD3DColor() const
	{
		return ((int)((((_color[3]) & 0xff) << 24) | (((_color[0]) & 0xff) << 16) | (((_color[1]) & 0xff) << 8) | ((_color[2]) & 0xff)));
	}

	inline int r() const { return _color[0]; }
	inline int g() const { return _color[1]; }
	inline int b() const { return _color[2]; }
	inline int a() const { return _color[3]; }

	inline float rBase() const { return _color[0] / 255.0f; }
	inline float gBase() const { return _color[1] / 255.0f; }
	inline float bBase() const { return _color[2] / 255.0f; }
	inline float aBase() const { return _color[3] / 255.0f; }

	unsigned char &operator[](int index)
	{
		return _color[index];
	}

	const unsigned char &operator[](int index) const
	{
		return _color[index];
	}

	bool operator == (const Color &rhs) const
	{
		//return (*((int *)this) == *((int *)&rhs));

		return (_color[0] == rhs._color[0] && _color[1] == rhs._color[1] && _color[2] == rhs._color[2] && _color[3] == rhs._color[3]);
	}

	bool operator != (const Color &rhs) const
	{
		return !(operator==(rhs));
	}

	Color &operator=(const Color &rhs)
	{
		//SetRawColor(rhs.GetRawColor());
		SetColor(rhs.r(), rhs.g(), rhs.b(), rhs.a());
		return *this;
	}

	float* Base()
	{
		float clr[3];

		clr[0] = _color[0] / 255.0f;
		clr[1] = _color[1] / 255.0f;
		clr[2] = _color[2] / 255.0f;

		return &clr[0];
	}

	float* BaseAlpha()
	{
		float clr[4];

		clr[0] = _color[0] / 255.0f;
		clr[1] = _color[1] / 255.0f;
		clr[2] = _color[2] / 255.0f;
		clr[3] = _color[3] / 255.0f;

		return &clr[0];
	}

	float Hue() const
	{
		if (_color[0] == _color[1] && _color[1] == _color[2])
		{
			return 0.0f;
		}

		float r = _color[0] / 255.0f;
		float g = _color[1] / 255.0f;
		float b = _color[2] / 255.0f;

		float max = r > g ? r : g > b ? g : b,
			min = r < g ? r : g < b ? g : b;
		float delta = max - min;
		float hue = 0.0f;

		if (r == max)
		{
			hue = (g - b) / delta;
		}
		else if (g == max)
		{
			hue = 2 + (b - r) / delta;
		}
		else if (b == max)
		{
			hue = 4 + (r - g) / delta;
		}
		hue *= 60;

		if (hue < 0.0f)
		{
			hue += 360.0f;
		}
		return hue;
	}

	float Saturation() const
	{
		float r = _color[0] / 255.0f;
		float g = _color[1] / 255.0f;
		float b = _color[2] / 255.0f;

		float max = r > g ? r : g > b ? g : b,
			min = r < g ? r : g < b ? g : b;
		float l, s = 0;

		if (max != min)
		{
			l = (max + min) / 2;
			if (l <= 0.5f)
				s = (max - min) / (max + min);
			else
				s = (max - min) / (2 - max - min);
		}
		return s;
	}

	float Brightness() const
	{
		float r = _color[0] / 255.0f;
		float g = _color[1] / 255.0f;
		float b = _color[2] / 255.0f;

		float max = r > g ? r : g > b ? g : b,
			min = r < g ? r : g < b ? g : b;
		return (max + min) / 2;
	}

	static Color FromHSB(float hue, float saturation, float brightness)
	{
		float h = hue == 1.0f ? 0 : hue * 6.0f;
		float f = h - (int)h;
		float p = brightness * (1.0f - saturation);
		float q = brightness * (1.0f - saturation * f);
		float t = brightness * (1.0f - (saturation * (1.0f - f)));

		if (h < 1)
		{
			return Color(
				(unsigned char)(brightness * 255),
				(unsigned char)(t * 255),
				(unsigned char)(p * 255)
			);
		}
		else if (h < 2)
		{
			return Color(
				(unsigned char)(q * 255),
				(unsigned char)(brightness * 255),
				(unsigned char)(p * 255)
			);
		}
		else if (h < 3)
		{
			return Color(
				(unsigned char)(p * 255),
				(unsigned char)(brightness * 255),
				(unsigned char)(t * 255)
			);
		}
		else if (h < 4)
		{
			return Color(
				(unsigned char)(p * 255),
				(unsigned char)(q * 255),
				(unsigned char)(brightness * 255)
			);
		}
		else if (h < 5)
		{
			return Color(
				(unsigned char)(t * 255),
				(unsigned char)(p * 255),
				(unsigned char)(brightness * 255)
			);
		}
		else
		{
			return Color(
				(unsigned char)(brightness * 255),
				(unsigned char)(p * 255),
				(unsigned char)(q * 255)
			);
		}
	}

	static Color FromHSV(float h, float s, float v)
	{
		if (s == 0.0f)
		{
			// gray
			return Color(v,v,v);
		}

		h = fmodf(h, 1.0f) / (60.0f / 360.0f);
		int   i = (int)h;
		float f = h - (float)i;
		float p = v * (1.0f - s);
		float q = v * (1.0f - s * f);
		float t = v * (1.0f - s * (1.0f - f));

		switch (i)
		{
		case 0: return Color(v * 255, t * 255, p * 255); break;
		case 1: return Color(q * 255, v * 255, p * 255); break;
		case 2: return Color(p * 255, v * 255, t * 255); break;
		case 3: return Color(p * 255, q * 255, v * 255); break;
		case 4: return Color(t * 255, p * 255, v * 255); break;
		case 5: default: return Color(v * 255, p * 255, q * 255); break;
		}
	}

	static Color Red() { return Color(255, 0, 0); }
	static Color Green() { return Color(0, 255, 0); }
	static Color Blue() { return Color(0, 0, 255); }
	static Color LightBlue() { return Color(100, 100, 255); }
	static Color Grey() { return Color(128, 128, 128); }
	static Color DarkGrey() { return Color(45, 45, 45); }
	static Color Black() { return Color(0, 0, 0); }
	static Color White() { return Color(255, 255, 255); }
	static Color Purple() { return Color(220, 0, 220); }

	static Color Red(int alpha) { return Color(255, 0, 0, alpha); }
	static Color Green(int alpha) { return Color(0, 255, 0, alpha); }
	static Color Blue(int alpha) { return Color(0, 0, 255, alpha); }
	static Color LightBlue(int alpha) { return Color(100, 100, 255, alpha); }
	static Color Grey(int alpha) { return Color(128, 128, 128, alpha); }
	static Color DarkGrey(int alpha) { return Color(45, 45, 45, alpha); }
	static Color Black(int alpha) { return Color(0, 0, 0, alpha); }
	static Color White(int alpha) { return Color(255, 255, 255, alpha); }
	static Color Purple(int alpha) { return Color(220, 0, 220, alpha); }

	/*//Menu
	static Color Background() { return Color(55, 55, 55); }
	static Color FrameBorder() { return Color(80, 80, 80); }
	static Color MainText() { return Color(230, 230, 230); }
	static Color HeaderText() { return Color(49, 124, 230); }
	static Color CurrentTab() { return Color(55, 55, 55); }
	static Color Tabs() { return Color(23, 23, 23); }
	static Color Highlight() { return Color(49, 124, 230); }
	static Color ElementBorder() { return Color(0, 0, 0); }
	static Color SliderScroll() { return Color(78, 143, 230); }
	*/

	operator uint32_t() {
		return (_color[3] << 24) | (_color[0] << 16) | (_color[1] << 8) | (_color[2] << 0);
	}

	operator ImVec4() {
		return ImVec4(_color[0] / 255.f, _color[1] / 255.f, _color[2] / 255.f, _color[3] / 255.f);
	}

private:
	unsigned char _color[4];
};

typedef void* (*CreateInterfaceFn)(const char *pName, int *pReturnCode);
typedef void* (*InstantiateInterfaceFn)();

class IAppSystem
{
public:
	virtual bool                            Connect(CreateInterfaceFn factory) = 0;                                     // 0
	virtual void                            Disconnect() = 0;                                                           // 1
	virtual void*                           QueryInterface(const char *pInterfaceName) = 0;                             // 2
	virtual int /*InitReturnVal_t*/         Init() = 0;                                                                 // 3
	virtual void                            Shutdown() = 0;                                                             // 4
	virtual const void* /*AppSystemInfo_t*/ GetDependencies() = 0;                                                      // 5
	virtual int /*AppSystemTier_t*/         GetTier() = 0;                                                              // 6
	virtual void                            Reconnect(CreateInterfaceFn factory, const char *pInterfaceName) = 0;       // 7
	virtual void                            UnkFunc() = 0;                                                              // 8
};

class IEngineVGui
{
public:
	virtual					~IEngineVGui(void) { }

	virtual vgui::VPANEL	GetPanel(int type) = 0;

	virtual bool			IsGameUIVisible() = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class ISurface : IAppSystem
{
public:
	virtual void          RunFrame() = 0;
	virtual vgui::VPANEL  GetEmbeddedPanel() = 0;
	virtual void          SetEmbeddedPanel(vgui::VPANEL pPanel) = 0;
	virtual void          PushMakeCurrent(vgui::VPANEL panel, bool useInsets) = 0;
	virtual void          PopMakeCurrent(vgui::VPANEL panel) = 0;
	virtual void          DrawSetColor(int r, int g, int b, int a) = 0;
	virtual void          DrawSetColor(Color col) = 0;
	virtual void          DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawFilledRectArray(IntRect *pRects, int numRects) = 0;
	virtual void          DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawPolyLine(int *px, int *py, int numPoints) = 0;
	virtual void          DrawSetApparentDepth(float f) = 0;
	virtual void          DrawClearApparentDepth(void) = 0;
	virtual void          DrawSetTextFont(vgui::HFont font) = 0;
	virtual void          DrawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void          DrawSetTextColor(Color col) = 0;
	virtual void          DrawSetTextPos(int x, int y) = 0;
	virtual void          DrawGetTextPos(int& x, int& y) = 0;
	virtual void          DrawPrintText(const wchar_t *text, int textLen, FontDrawType drawType = FontDrawType::FONT_DRAW_DEFAULT) = 0;
	virtual void          DrawUnicodeChar(wchar_t wch, FontDrawType drawType = FontDrawType::FONT_DRAW_DEFAULT) = 0;
	virtual void          DrawFlushText() = 0;
	virtual void*         CreateHTMLWindow(void *events, vgui::VPANEL context) = 0;
	virtual void          PaintHTMLWindow(void *htmlwin) = 0;
	virtual void          DeleteHTMLWindow(void *htmlwin) = 0;
	virtual int           DrawGetTextureId(char const *filename) = 0;
	virtual bool          DrawGetTextureFile(int id, char *filename, int maxlen) = 0;
	virtual void          DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) = 0;
	virtual void          DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall) = 0;
	virtual void          DrawSetTexture(int id) = 0;
	virtual void          DeleteTextureByID(int id) = 0;
	virtual void          DrawGetTextureSize(int id, int &wide, int &tall) = 0;
	virtual void          DrawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual bool          IsTextureIDValid(int id) = 0;
	virtual int           CreateNewTextureID(bool procedural = false) = 0;
	virtual void          GetScreenSize(int &wide, int &tall) = 0;
	virtual void          SetAsTopMost(vgui::VPANEL panel, bool state) = 0;
	virtual void          BringToFront(vgui::VPANEL panel) = 0;
	virtual void          SetForegroundWindow(vgui::VPANEL panel) = 0;
	virtual void          SetPanelVisible(vgui::VPANEL panel, bool state) = 0;
	virtual void          SetMinimized(vgui::VPANEL panel, bool state) = 0;
	virtual bool          IsMinimized(vgui::VPANEL panel) = 0;
	virtual void          FlashWindow(vgui::VPANEL panel, bool state) = 0;
	virtual void          SetTitle(vgui::VPANEL panel, const wchar_t *title) = 0;
	virtual void          SetAsToolBar(vgui::VPANEL panel, bool state) = 0;
	virtual void          CreatePopup(vgui::VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true, bool kbInput = true) = 0;
	virtual void          SwapBuffers(vgui::VPANEL panel) = 0;
	virtual void          Invalidate(vgui::VPANEL panel) = 0;
	virtual void          SetCursor(unsigned long cursor) = 0;
	virtual bool          IsCursorVisible() = 0;
	virtual void          ApplyChanges() = 0;
	virtual bool          IsWithin(int x, int y) = 0;
	virtual bool          HasFocus() = 0;
	virtual bool          SupportsFeature(int /*SurfaceFeature_t*/ feature) = 0;
	virtual void          RestrictPaintToSinglePanel(vgui::VPANEL panel, bool bForceAllowNonModalSurface = false) = 0;
	virtual void          SetModalPanel(vgui::VPANEL) = 0;
	virtual vgui::VPANEL  GetModalPanel() = 0;
	virtual void          UnlockCursor() = 0; //66
	virtual void          LockCursor() = 0; //67
	virtual void          SetTranslateExtendedKeys(bool state) = 0;
	virtual vgui::VPANEL  GetTopmostPopup() = 0;
	virtual void          SetTopLevelFocus(vgui::VPANEL panel) = 0;
	virtual vgui::HFont   CreateFont_() = 0;
	virtual bool          SetFontGlyphSet(vgui::HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0) = 0;
	virtual bool          AddCustomFontFile(const char *fontFileName) = 0;
	virtual int           GetFontTall(vgui::HFont font) = 0;
	virtual int           GetFontAscent(vgui::HFont font, wchar_t wch) = 0;
	virtual bool          IsFontAdditive(vgui::HFont font) = 0;
	virtual void          GetCharABCwide(vgui::HFont font, int ch, int &a, int &b, int &c) = 0;
	virtual int           GetCharacterWidth(vgui::HFont font, int ch) = 0;
	virtual void          GetTextSize(vgui::HFont font, const wchar_t *text, int &wide, int &tall) = 0;
	virtual vgui::VPANEL  GetNotifyPanel() = 0;
	virtual void          SetNotifyIcon(vgui::VPANEL context, unsigned long icon, vgui::VPANEL panelToReceiveMessages, const char *text) = 0;
	virtual void          PlaySound_(const char *fileName) = 0;
	virtual int           GetPopupCount() = 0;
	virtual vgui::VPANEL  GetPopup(int index) = 0;
	virtual bool          ShouldPaintChildPanel(vgui::VPANEL childPanel) = 0;
	virtual bool          RecreateContext(vgui::VPANEL panel) = 0;
	virtual void          AddPanel(vgui::VPANEL panel) = 0;
	virtual void          ReleasePanel(vgui::VPANEL panel) = 0;
	virtual void          MovePopupToFront(vgui::VPANEL panel) = 0;
	virtual void          MovePopupToBack(vgui::VPANEL panel) = 0;
	virtual void          SolveTraverse(vgui::VPANEL panel, bool forceApplySchemeSettings = false) = 0;
	virtual void          PaintTraverse(vgui::VPANEL panel) = 0;
	virtual void          EnableMouseCapture(vgui::VPANEL panel, bool state) = 0;
	virtual void          GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) = 0;
	virtual void          GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) = 0;
	virtual void          GetProportionalBase(int &width, int &height) = 0;
	virtual void          CalculateMouseVisible() = 0;
	virtual bool          NeedKBInput() = 0;
	virtual bool          HasCursorPosFunctions() = 0;
	virtual void          SurfaceGetCursorPos(int &x, int &y) = 0;
	virtual void          SurfaceSetCursorPos(int x, int y) = 0;
	virtual void          DrawTexturedLine(const Vertex_t &a, const Vertex_t &b) = 0;
	virtual void          DrawOutlinedCircle(int x, int y, int radius, int segments) = 0;
	virtual void          DrawTexturedPolyLine(const Vertex_t *p, int n) = 0;
	virtual void          DrawTexturedSubRect(int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1) = 0;
	virtual void          DrawTexturedPolygon(int n, Vertex_t *pVertice, bool bClipVertices = true) = 0;
	virtual const wchar_t* GetTitle(VPANEL panel) = 0;
	virtual bool IsCursorLocked() const = 0;
	virtual void SetWorkspaceInsets(int left, int top, int right, int bottom) = 0;

	// squarish comic book word bubble with pointer, rect params specify the space inside the bubble
	virtual void DrawWordBubble(int x0, int y0, int x1, int y1, int nBorderThickness, Color rgbaBackground, Color rgbaBorder,
		bool bPointer = false, int nPointerX = 0, int nPointerY = 0, int nPointerBaseThickness = 16) = 0;


	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	virtual bool DrawGetUnicodeCharRenderInfo(wchar_t ch, char& info) = 0;
	virtual void DrawRenderCharFromInfo(const char& info) = 0;

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	virtual void DrawSetAlphaMultiplier(float alpha /* [0..1] */) = 0;
	virtual float DrawGetAlphaMultiplier() = 0;

	// web browser
	virtual void SetAllowHTMLJavaScript(bool state) = 0;

	// video mode changing
	virtual void OnScreenSizeChanged(int nOldWidth, int nOldHeight) = 0;

	virtual char CreateCursorFromFile(char const *curOrAniFile, char const *pPathID = 0) = 0;

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	virtual char* DrawGetTextureMatInfoFactory(int id) = 0;

	virtual void PaintTraverseEx(VPANEL panel, bool paintPopups = false) = 0;

	virtual float GetZPos() const = 0;

	// From the Xbox
	virtual void SetPanelForInput(VPANEL vpanel) = 0;
	virtual void DrawFilledRectFastFade(int x0, int y0, int x1, int y1, int fadeStartPt, int fadeEndPt, unsigned int alpha0, unsigned int alpha1, bool bHorizontal) = 0;
	virtual void DrawFilledRectFade(int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal) = 0; //123

	enum ImageFormat
	{
		IMAGE_FORMAT_UNKNOWN = -1,
		IMAGE_FORMAT_RGBA8888 = 0,
		IMAGE_FORMAT_ABGR8888,
		IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_BGR888,
		IMAGE_FORMAT_RGB565,
		IMAGE_FORMAT_I8,
		IMAGE_FORMAT_IA88,
		IMAGE_FORMAT_P8,
		IMAGE_FORMAT_A8,
		IMAGE_FORMAT_RGB888_BLUESCREEN,
		IMAGE_FORMAT_BGR888_BLUESCREEN,
		IMAGE_FORMAT_ARGB8888,
		IMAGE_FORMAT_BGRA8888,
		IMAGE_FORMAT_DXT1,
		IMAGE_FORMAT_DXT3,
		IMAGE_FORMAT_DXT5,
		IMAGE_FORMAT_BGRX8888,
		IMAGE_FORMAT_BGR565,
		IMAGE_FORMAT_BGRX5551,
		IMAGE_FORMAT_BGRA4444,
		IMAGE_FORMAT_DXT1_ONEBITALPHA,
		IMAGE_FORMAT_BGRA5551,
		IMAGE_FORMAT_UV88,
		IMAGE_FORMAT_UVWQ8888,
		IMAGE_FORMAT_RGBA16161616F,
		IMAGE_FORMAT_RGBA16161616,
		IMAGE_FORMAT_UVLX8888,
		IMAGE_FORMAT_R32F,// Single-channel 32-bit floating point
		IMAGE_FORMAT_RGB323232F,
		IMAGE_FORMAT_RGBA32323232F,

		// Depth-stencil texture formats for shadow depth mapping
		IMAGE_FORMAT_NV_DST16,// 
		IMAGE_FORMAT_NV_DST24,//
		IMAGE_FORMAT_NV_INTZ,// Vendor-specific depth-stencil texture
		IMAGE_FORMAT_NV_RAWZ,// formats for shadow depth mapping 
		IMAGE_FORMAT_ATI_DST16,// 
		IMAGE_FORMAT_ATI_DST24,//
		IMAGE_FORMAT_NV_NULL,// Dummy format which takes no video memory

							 // Compressed normal map formats
							 IMAGE_FORMAT_ATI2N,// One-surface ATI2N / DXN format
							 IMAGE_FORMAT_ATI1N,// Two-surface ATI1N format

							 NUM_IMAGE_FORMATS
	};

	virtual void DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat) = 0;
	virtual void DrawSetTextScale(float sx, float sy) = 0;
	virtual bool SetBitmapFontGlyphSet(vgui::HFont font, const char *windowsFontName, float scalex, float scaley, int flags) = 0;
	// adds a bitmap font file
	virtual bool AddBitmapFontFile(const char *fontFileName) = 0;
	// sets a symbol for the bitmap font
	virtual void SetBitmapFontName(const char *pName, const char *pFontFilename) = 0;
	// gets the bitmap font filename
	virtual const char* GetBitmapFontName(const char *pName) = 0;
	virtual void ClearTemporaryFontCache() = 0;

	virtual char* GetIconImageForFullPath(char const *pFullPath) = 0;
	virtual void DrawUnicodeString(const wchar_t *pwString, int drawType = FONT_DRAW_DEFAULT) = 0;
	virtual void PrecacheFontCharacters(vgui::HFont font, const wchar_t *pCharacters) = 0;
	// Console-only.  Get the string to use for the current video mode for layout files.
	//virtual const char *GetResolutionKey( void ) const = 0;

	virtual const char *GetFontName(vgui::HFont font) = 0;
	//virtual const char *GetFontFamilyName( HFont font ) = 0;

	virtual bool ForceScreenSizeOverride(bool bState, int wide, int tall) = 0;
	// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
	virtual bool ForceScreenPosOffset(bool bState, int x, int y) = 0;
	virtual void OffsetAbsPos(int &x, int &y) = 0;

	virtual void SetAbsPosForContext(int, int, int) = 0;
	virtual void GetAbsPosForContext(int, int &, int &) = 0;

	// Causes fonts to get reloaded, etc.
	virtual void ResetFontCaches() = 0;
};

class InputSystem
{
	template <typename T>
	T callvfunc(void *vTable, int iIndex) {
		return (*(T**)vTable)[iIndex];
	}

public:
	void EnableInput(bool bEnable)
	{
		typedef void(__thiscall* OriginalFn)(void*, bool);
		return callvfunc<OriginalFn>(this, 11)(this, bEnable);
	}

	void ResetInputState()
	{
		typedef void(__thiscall* OriginalFn)(void*);
		return callvfunc<OriginalFn>(this, 39)(this);
	}

	bool IsButtonDown(ButtonCode_t code)
	{
		typedef bool(__thiscall* OriginalFn)(void*, ButtonCode_t);
		return callvfunc<OriginalFn>(this, 15)(this, code);
	}

	void GetCursorPosition(int* m_pX, int* m_pY)
	{
		typedef void(__thiscall* OriginalFn)(void*, int*, int*);
		return callvfunc<OriginalFn>(this, 56)(this, m_pX, m_pY);
	}

	const char* ButtonCodeToString(ButtonCode_t ButtonCode)
	{
		typedef const char*(__thiscall* OriginalFn)(void*, ButtonCode_t);
		return callvfunc<OriginalFn>(this, 40)(this, ButtonCode);
	}

	ButtonCode_t VirtualKeyToButtonCode(int nVirtualKey)
	{
		typedef ButtonCode_t(__thiscall* OriginalFn)(void*, int);
		return callvfunc<OriginalFn>(this, 44)(this, nVirtualKey);
	}

	int ButtonCodeToVirtualKey(ButtonCode_t code)
	{
		typedef int(__thiscall* OriginalFn)(void*, ButtonCode_t);
		return callvfunc<OriginalFn>(this, 45)(this, code);
	}
};

class ILocalizeTextQuery
{
public:
	virtual int ComputeTextWidth(const wchar_t* pString) = 0;
};

class ILocalizationChangeCallback
{
public:
	virtual void OnLocalizationChanged() = 0;
};

using LocalizeStringIndex_t = unsigned;
class ILocalize : public IAppSystem
{
public:
	virtual bool					AddFile(const char* fileName, const char* pPathID = nullptr, bool bIncludeFallbackSearchPaths = false) = 0;
	virtual void					RemoveAll() = 0;
	virtual wchar_t*				find(const char* tokenName) = 0;
	virtual const wchar_t*			FindSafe(const char* tokenName) = 0;
	virtual int						ConvertANSIToUnicode(const char* ansi, wchar_t* unicode, int unicodeBufferSizeInBytes) = 0;
	virtual int						ConvertUnicodeToANSI(const wchar_t* unicode, char* ansi, int ansiBufferSize) = 0;
	virtual LocalizeStringIndex_t	FindIndex(const char* tokenName) = 0;
	virtual void					ConstructString(wchar_t* unicodeOuput, int unicodeBufferSizeInBytes, const wchar_t* formatString, int numFormatParameters, ...) = 0;
	virtual const char*				GetNameByIndex(LocalizeStringIndex_t index) = 0;
	virtual wchar_t*				GetValueByIndex(LocalizeStringIndex_t index) = 0;
};

class CCSGameRules
{
public:
	bool IsValveDS()
	{
		return *(bool*)((uintptr_t)this + 0x75);
	}

	bool IsBombDropped()
	{
		return *(bool*)((uintptr_t)this + 0x99C);
	}

	bool IsBombPlanted()
	{
		return *(bool*)((uintptr_t)this + 0x99D);
	}

	bool IsFreezeTime()
	{
		return *(bool*)((uintptr_t)this + 0x20);
	}
};

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IConVar;
class CCommand;
class ConCommandBase;
class ConCommand;
class ConVar;
typedef int CVarDLLIdentifier_t;

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
// NOTE: For FCVAR_NEVER_AS_STRING ConVars, pOldValue == NULL
//-----------------------------------------------------------------------------
typedef void(*FnChangeCallback_t)(ConVar *var, const char *pOldValue, float flOldValue);


//-----------------------------------------------------------------------------
// Abstract interface for ConVars
//-----------------------------------------------------------------------------
#define FORCEINLINE_CVAR inline
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CCommand;
class ConCommand;
class ConCommandBase;

struct characterset_t
{
	char Set[256];
};

struct matrix3x4_t
{
	matrix3x4_t() {}
	matrix3x4_t(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	void Init(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin)
	{
		m_flMatVal[0][0] = xAxis.x; m_flMatVal[0][1] = yAxis.x; m_flMatVal[0][2] = zAxis.x; m_flMatVal[0][3] = vecOrigin.x;
		m_flMatVal[1][0] = xAxis.y; m_flMatVal[1][1] = yAxis.y; m_flMatVal[1][2] = zAxis.y; m_flMatVal[1][3] = vecOrigin.y;
		m_flMatVal[2][0] = xAxis.z; m_flMatVal[2][1] = yAxis.z; m_flMatVal[2][2] = zAxis.z; m_flMatVal[2][3] = vecOrigin.z;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	matrix3x4_t(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin)
	{
		Init(xAxis, yAxis, zAxis, vecOrigin);
	}

	inline void SetOrigin(Vector const & p)
	{
		m_flMatVal[0][3] = p.x;
		m_flMatVal[1][3] = p.y;
		m_flMatVal[2][3] = p.z;
	}

	inline Vector GetOrigin()
	{
		return Vector(m_flMatVal[0][3],
			m_flMatVal[1][3],
			m_flMatVal[2][3]);
	}

	inline void Invalidate(void)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_flMatVal[i][j] = VEC_T_NAN;
			}
		}
	}

	inline bool IsValid()
	{
		if (this == nullptr)
			return false;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				if (m_flMatVal[i][j] == VEC_T_NAN)
					return false;
			}
		}
		return true;
	}

	void Clear()
	{
		m_flMatVal[0][0] = 0.f; m_flMatVal[0][1] = 0.f; m_flMatVal[0][2] = 0.f; m_flMatVal[0][3] = 0.f;
		m_flMatVal[1][0] = 0.f; m_flMatVal[1][1] = 0.f; m_flMatVal[1][2] = 0.f; m_flMatVal[1][3] = 0.f;
		m_flMatVal[2][0] = 0.f; m_flMatVal[2][1] = 0.f; m_flMatVal[2][2] = 0.f; m_flMatVal[2][3] = 0.f;
	}

	float *operator[](int i) {
		if ((i >= 0) && (i < 3) && m_flMatVal[i])
		{
			return m_flMatVal[i];
		}
		else return NULL;
	}
	const float *operator[](int i) const {
		if ((i >= 0) && (i < 3) && m_flMatVal[i])
		{
			return m_flMatVal[i];
		}
		else return NULL;
	}
	float *Base() { return &m_flMatVal[0][0]; }
	const float *Base() const { return &m_flMatVal[0][0]; }

	float m_flMatVal[3][4];
};

class ALIGN16 matrix3x4a_t : public matrix3x4_t
{
public:
	/*
	matrix3x4a_t() { if (((size_t)Base()) % 16 != 0) { Error( "matrix3x4a_t missaligned" ); } }
	*/
	matrix3x4a_t& operator=(const matrix3x4_t& src) { memcpy(Base(), src.Base(), sizeof(float) * 3 * 4); return *this; };
};

//class IClientUnknown;
//struct model_t;

struct mstudiobbox_t
{
	int		bone;
	int		group; // intersection group
	Vector	bbmin; // bounding box 
	Vector	bbmax;
	int		hitboxnameindex; // offset to the name of the hitbox.
	QAngle	rotation;
	float	radius;
	int		pad2[4];

	char* getHitboxName()
	{
		if (hitboxnameindex == 0)
			return "";

		return ((char*)this) + hitboxnameindex;
	}
};

class CCommand
{
public:
	CCommand();
	CCommand(int nArgC, const char **ppArgV);
	bool Tokenize(const char *pCommand, characterset_t *pBreakSet = NULL);
	void Reset();

	int ArgC() const;
	const char** ArgV() const;
	const char*  ArgS() const;					        // All args that occur after the 0th arg, in string form
	const char*  GetCommandString() const;		  // The entire command in string form, including the 0th arg
	const char*  operator[](int nIndex) const;	// Gets at arguments
	const char*  Arg(int nIndex) const;		      // Gets at arguments

												  // Helper functions to parse arguments to commands.
	const char* FindArg(const char *pName) const;
	int FindArgInt(const char *pName, int nDefaultVal) const;

	static int MaxCommandLength();
	static characterset_t* DefaultBreakSet();

private:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

	int		m_nArgc;
	int		m_nArgv0Size;
	char	m_pArgSBuffer[COMMAND_MAX_LENGTH];
	char	m_pArgvBuffer[COMMAND_MAX_LENGTH];
	const char*	m_ppArgv[COMMAND_MAX_ARGC];
};

inline int CCommand::MaxCommandLength()
{
	return COMMAND_MAX_LENGTH - 1;
}

inline int CCommand::ArgC() const
{
	return m_nArgc;
}

inline const char **CCommand::ArgV() const
{
	return m_nArgc ? (const char**)m_ppArgv : NULL;
}

inline const char *CCommand::ArgS() const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

inline const char *CCommand::GetCommandString() const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

inline const char *CCommand::Arg(int nIndex) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if (nIndex < 0 || nIndex >= m_nArgc)
		return "";
	return m_ppArgv[nIndex];
}

inline const char *CCommand::operator[](int nIndex) const
{
	return Arg(nIndex);
}

//-----------------------------------------------------------------------------
// Any executable that wants to use ConVars need to implement one of
// these to hook up access to console variables.
//-----------------------------------------------------------------------------
class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase(ConCommandBase *pVar) = 0;
};

//-----------------------------------------------------------------------------
// Called when a ConCommand needs to execute
//-----------------------------------------------------------------------------
typedef void(*FnCommandCallbackV1_t)(void);
typedef void(*FnCommandCallback_t)(const CCommand &command);

#define COMMAND_COMPLETION_MAXITEMS       64
#define COMMAND_COMPLETION_ITEM_LENGTH    64

//-----------------------------------------------------------------------------
// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
//-----------------------------------------------------------------------------
typedef int(*FnCommandCompletionCallback)(const char *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);


//-----------------------------------------------------------------------------
// Interface version
//-----------------------------------------------------------------------------
class ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand &command) = 0;
};

class ICommandCompletionCallback
{
public:
	virtual int  CommandCompletionCallback(const char *pPartial, CUtlVector<CUtlString> &commands) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
	friend class CCvar;
	friend class ConVar;
	friend class ConCommand;
	friend void ConVar_Register(int nCVarFlag, IConCommandBaseAccessor *pAccessor);

	// FIXME: Remove when ConVar changes are done
	friend class CDefaultCvar;

public:
	ConCommandBase(void);
	ConCommandBase(const char *pName, const char *pHelpString = 0, int flags = 0);

	virtual                     ~ConCommandBase(void);
	virtual bool                IsCommand(void) const;
	virtual bool                IsFlagSet(int flag) const;
	virtual void                AddFlags(int flags);
	virtual void                RemoveFlags(int flags);
	virtual int                 GetFlags() const;
	virtual const char*         GetName(void) const;
	virtual const char*         GetHelpText(void) const;
	const ConCommandBase*       GetNext(void) const;
	ConCommandBase*             GetNext(void);
	virtual bool                IsRegistered(void) const;
	virtual CVarDLLIdentifier_t GetDLLIdentifier() const;

	//protected:
	virtual void                Create(const char *pName, const char *pHelpString = 0, int flags = 0);
	virtual void                Init();
	void                        Shutdown();
	char*                       CopyString(const char *from);

	//private:
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase*             m_pNext;
	bool                        m_bRegistered;
	const char*                 m_pszName;
	const char*                 m_pszHelpString;
	int                         m_nFlags;

protected:
	// ConVars add themselves to this list for the executable. 
	// Then ConVar_Register runs through  all the console variables 
	// and registers them into a global list stored in vstdlib.dll
	static ConCommandBase* s_pConCommandBases;

	// ConVars in this executable use this 'global' to access values.
	static IConCommandBaseAccessor* s_pAccessor;

public:
	// This list will hold all the registered commands.
	// It is not from the official SDK. I've added this so that
	// we can parse all convars we have created if we want to
	// save them to a file later on, for example.
	static ConCommandBase* s_pRegisteredCommands;
};

//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
	friend class CCvar;

public:
	typedef ConCommandBase BaseClass;

	ConCommand(const char *pName, FnCommandCallbackV1_t callback,
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0);
	ConCommand(const char *pName, FnCommandCallback_t callback,
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0);
	ConCommand(const char *pName, ICommandCallback *pCallback,
		const char *pHelpString = 0, int flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0);

	virtual         ~ConCommand(void);
	virtual bool    IsCommand(void) const;
	virtual int     AutoCompleteSuggest(const char *partial, CUtlVector<CUtlString> &commands);
	virtual bool    CanAutoComplete(void);
	virtual void    Dispatch(const CCommand &command);

	//private:
	// NOTE: To maintain backward compat, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

	// Call this function when executing the command
	union
	{
		FnCommandCallbackV1_t       m_fnCommandCallbackV1;
		FnCommandCallback_t         m_fnCommandCallback;
		ICommandCallback*           m_pCommandCallback;
	};

	union
	{
		FnCommandCompletionCallback m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};

class IConVar
{
public:
	virtual void SetValue(const char *pValue) = 0;
	virtual void SetValue(float flValue) = 0;
	virtual void SetValue(int nValue) = 0;
	virtual void SetValue(Color value) = 0;
	virtual const char *GetName(void) const = 0;
	virtual const char *GetBaseName(void) const = 0;
	virtual bool IsFlagSet(int nFlag) const = 0;
	virtual int GetSplitScreenPlayerSlot() const = 0;
};

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase, public IConVar
{
	friend class CCvar;
	friend class ConVarRef;
	friend class SplitScreenConVarRef;

public:
	typedef ConCommandBase BaseClass;

	ConVar(const char *pName, const char *pDefaultValue, int flags = 0);

	ConVar(const char *pName, const char *pDefaultValue, int flags, const char *pHelpString);
	ConVar(const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax);
	ConVar(const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, FnChangeCallback_t callback);
	ConVar(const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback);

	virtual                     ~ConVar(void);
	virtual bool                IsFlagSet(int flag) const;
	virtual const char*         GetHelpText(void) const;
	virtual bool                IsRegistered(void) const;
	virtual const char*         GetName(void) const;
	virtual const char*         GetBaseName(void) const;
	virtual int                 GetSplitScreenPlayerSlot() const;

	virtual void                AddFlags(int flags);
	virtual int                 GetFlags() const;
	virtual bool                IsCommand(void) const;

	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke = true);
	void RemoveChangeCallback(FnChangeCallback_t callbackToRemove);

	int GetChangeCallbackCount() const { return m_pParent->m_fnChangeCallbacks.Count(); }
	FnChangeCallback_t GetChangeCallback(int slot) const { return m_pParent->m_fnChangeCallbacks[slot]; }

	// Retrieve value
	virtual float                   GetFloat(void) const;
	virtual int                     GetInt(void) const;
	FORCEINLINE_CVAR Color          GetColor(void) const;
	FORCEINLINE_CVAR bool           GetBool() const { return !!GetInt(); }
	FORCEINLINE_CVAR char const*    GetString(void) const;

	// Compiler driven selection for template use
	template <typename T> T Get(void) const;
	template <typename T> T Get(T *) const;

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.

	// These just call into the IConCommandBaseAccessor to check flags and Set the var (which ends up calling InternalSetValue).
	virtual void                    SetValue(const char *value);
	virtual void                    SetValue(float value);
	virtual void                    SetValue(int value);
	virtual void                    SetValue(Color value);

	// Reset to default value
	void                            Revert(void);
	bool                            HasMin() const;
	bool                            HasMax() const;
	bool                            GetMin(float& minVal) const;
	bool                            GetMax(float& maxVal) const;
	float                           GetMinValue() const;
	float                           GetMaxValue() const;
	const char*                     GetDefault(void) const;

	struct CVValue_t
	{
		char*   m_pszString;
		int     m_StringLength;
		float   m_fValue;
		int     m_nValue;
	};

	FORCEINLINE_CVAR CVValue_t &GetRawValue()
	{
		return m_Value;
	}
	FORCEINLINE_CVAR const CVValue_t &GetRawValue() const
	{
		return m_Value;
	}

	//private:
	bool                        InternalSetColorFromString(const char *value);
	virtual void                InternalSetValue(const char *value);
	virtual void                InternalSetFloatValue(float fNewValue);
	virtual void                InternalSetIntValue(int nValue);
	virtual void                InternalSetColorValue(Color value);
	virtual bool                ClampValue(float& value);
	virtual void                ChangeStringValue(const char *tempVal, float flOldValue);
	virtual void                Create(const char *pName, const char *pDefaultValue, int flags = 0, const char *pHelpString = 0, bool bMin = false, float fMin = 0.0, bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0);

	// Used internally by OneTimeInit to Initialize.
	virtual void                Init();

	//protected:
	ConVar*                     m_pParent;
	const char*                 m_pszDefaultValue;
	CVValue_t                   m_Value;
	bool                        m_bHasMin;
	float                       m_fMinVal;
	bool                        m_bHasMax;
	float                       m_fMaxVal;

	// Call this function when ConVar changes
	CUtlVector<FnChangeCallback_t> m_fnChangeCallbacks;
};

struct CEffectData
{
	Vector origin;
	Vector start;
	Vector normal;
	Vector angles;
	int flags;
	int entity;
	float scale;
	float magnitude;
	float radius;
	int attachmentIndex;
	short surfaceProp;
	int material;
	int damageType;
	int hitBox;
	int otherEntIndex;
	unsigned char color;
	int effectName;
};

typedef void(*ClientEffectCallback)(const CEffectData& data);

struct CClientEffectRegistration
{
	CClientEffectRegistration(const char* pEffectName, ClientEffectCallback fn);
	const char* effectName;
	ClientEffectCallback function;
	CClientEffectRegistration* next;
};

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVar::GetFloat(void) const
{
	uint32_t xored = *(uint32_t*)&m_pParent->m_Value.m_fValue ^ (uint32_t)this;
	return *(float*)&xored;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVar::GetInt(void) const
{
	return (int)(m_pParent->m_Value.m_nValue ^ (int)this);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color
// Output : Color
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR Color ConVar::GetColor(void) const
{
	int value = GetInt();
	unsigned char *pColorElement = ((unsigned char *)&value);
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}


//-----------------------------------------------------------------------------

template <> FORCEINLINE_CVAR float          ConVar::Get<float>(void) const { return GetFloat(); }
template <> FORCEINLINE_CVAR int            ConVar::Get<int>(void) const { return GetInt(); }
template <> FORCEINLINE_CVAR bool           ConVar::Get<bool>(void) const { return GetBool(); }
template <> FORCEINLINE_CVAR const char*    ConVar::Get<const char *>(void) const { return GetString(); }
template <> FORCEINLINE_CVAR float          ConVar::Get<float>(float *p) const { return (*p = GetFloat()); }
template <> FORCEINLINE_CVAR int            ConVar::Get<int>(int *p) const { return (*p = GetInt()); }
template <> FORCEINLINE_CVAR bool           ConVar::Get<bool>(bool *p) const { return (*p = GetBool()); }
template <> FORCEINLINE_CVAR const char*    ConVar::Get<const char *>(char const **p) const { return (*p = GetString()); }

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
// Output : const char *
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *ConVar::GetString(void) const
{
	if (m_nFlags & FCVAR_NEVER_AS_STRING)
		return "FCVAR_NEVER_AS_STRING";
	char const *str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommands with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register(int nCVarFlag = 0, IConCommandBaseAccessor *pAccessor = NULL);
void ConVar_Unregister();

class SpoofedConvar
{

public:

	SpoofedConvar();
	SpoofedConvar(const char* szCVar);
	SpoofedConvar(ConVar* pCVar);

	~SpoofedConvar();

	bool IsSpoofed();
	void Spoof();
	void Restore();

	void SetFlags(int flags);
	int  GetFlags();

	void SetBool(bool bValue);
	void SetInt(int iValue);
	void SetFloat(float flValue);
	void SetString(const char* szValue);

private:
	ConVar * m_pOriginalCVar = nullptr;
	ConVar *m_pDummyCVar = nullptr;

	char m_szDummyName[128];
	char m_szDummyValue[128];
	char m_szOriginalName[128];
	char m_szOriginalValue[128];
	int m_iOriginalFlags;
};

class IConsoleDisplayFunc
{
public:
	virtual void ColorPrint(const uint8_t* clr, const char *pMessage) = 0;
	virtual void Print(const char *pMessage) = 0;
	virtual void DPrint(const char *pMessage) = 0;
};

class ICvar : public IAppSystem
{
public:
	virtual CVarDLLIdentifier_t        AllocateDLLIdentifier() = 0; // 9
	virtual void                       RegisterConCommand(ConCommandBase *pCommandBase) = 0; //10
	virtual void                       UnregisterConCommand(ConCommandBase *pCommandBase) = 0;
	virtual void                       UnregisterConCommands(CVarDLLIdentifier_t id) = 0;
	virtual const char*                GetCommandLineValue(const char *pVariableName) = 0;
	virtual ConCommandBase*            FindCommandBase(const char *name) = 0;
	virtual const ConCommandBase*      FindCommandBase(const char *name) const = 0;
	virtual ConVar*                    FindVar(const char *var_name) = 0; //16
	virtual const ConVar*              FindVar(const char *var_name) const = 0;
	virtual ConCommand*                FindCommand(const char *name) = 0;
	virtual const ConCommand*          FindCommand(const char *name) const = 0;
	virtual void                       InstallGlobalChangeCallback(FnChangeCallback_t callback) = 0;
	virtual void                       RemoveGlobalChangeCallback(FnChangeCallback_t callback) = 0;
	virtual void                       CallGlobalChangeCallbacks(ConVar *var, const char *pOldString, float flOldValue) = 0;
	virtual void                       InstallConsoleDisplayFunc(IConsoleDisplayFunc* pDisplayFunc) = 0;
	virtual void                       RemoveConsoleDisplayFunc(IConsoleDisplayFunc* pDisplayFunc) = 0;
	virtual void                       ConsoleColorPrintf(const Color &clr, const char *pFormat, ...) const = 0;
	virtual void                       ConsolePrintf(const char *pFormat, ...) const = 0;
	virtual void                       ConsoleDPrintf(const char *pFormat, ...) const = 0;
	virtual void                       RevertFlaggedConVars(int nFlag) = 0;
protected:	class ICVarIteratorInternal;
public:
	/// Iteration over all cvars. 
	/// (THIS IS A SLOW OPERATION AND YOU SHOULD AVOID IT.)
	/// usage: 
	/// { ICVar::Iterator iter(g_pCVar); 
	///   for ( iter.SetFirst() ; iter.IsValid() ; iter.Next() )
	///   {  
	///       ConCommandBase *cmd = iter.Get();
	///   } 
	/// }
	/// The Iterator class actually wraps the internal factory methods
	/// so you don't need to worry about new/delete -- scope takes care
	//  of it.
	/// We need an iterator like this because we can't simply return a 
	/// pointer to the internal data type that contains the cvars -- 
	/// it's a custom, protected class with unusual semantics and is
	/// prone to change.
	class Iterator
	{
	public:
		inline Iterator(ICvar *icvar);
		inline ~Iterator(void);
		inline void		SetFirst(void);
		inline void		Next(void);
		inline bool		IsValid(void);
		inline ConCommandBase *Get(void);
	private:
		ICVarIteratorInternal * m_pIter;
	};

protected:
	// internals for  ICVarIterator
	class ICVarIteratorInternal
	{
	public:
		// warning: delete called on 'ICvar::ICVarIteratorInternal' that is abstract but has non-virtual destructor [-Wdelete-non-virtual-dtor]
		virtual void		SetFirst(void) = 0;
		virtual void		Next(void) = 0;
		virtual	bool		IsValid(void) = 0;
		virtual ConCommandBase *Get(void) = 0;
	};

	friend class Iterator;
};

inline ICvar::Iterator::Iterator(ICvar *icvar)
{
	m_pIter = Memory::VCall<ICVarIteratorInternal*(__thiscall*)(void*)>(icvar, 42)(icvar);
}

inline ICvar::Iterator::~Iterator(void)
{
	//delete m_pIter;
}

inline void ICvar::Iterator::SetFirst(void)
{
	m_pIter->SetFirst();
}

inline void ICvar::Iterator::Next(void)
{
	m_pIter->Next();
}

inline bool ICvar::Iterator::IsValid(void)
{
	return m_pIter->IsValid();
}

inline ConCommandBase * ICvar::Iterator::Get(void)
{
	return m_pIter->Get();
}

class CViewSetup
{
public:
	int			x, x_old; //0x4
	int			y, y_old;
	int			width, width_old;
	int			height, height_old;
	bool		m_bOrtho;
	float		m_OrthoLeft;
	float		m_OrthoTop;
	float		m_OrthoRight;
	float		m_OrthoBottom;
	bool		m_bCustomViewMatrix;
	matrix3x4_t	m_matCustomViewMatrix;
	char		pad_0x68[0x48];
	float		fov;
	float		fovViewmodel;
	Vector		origin;
	QAngle		angles;
	float		zNear;
	float		zFar;
	float		zNearViewmodel;
	float		zFarViewmodel;
	float		m_flAspectRatio;
	float		m_flNearBlurDepth;
	float		m_flNearFocusDepth;
	float		m_flFarFocusDepth;
	float		m_flFarBlurDepth;
	float		m_flNearBlurRadius;
	float		m_flFarBlurRadius;
	int			m_nDoFQuality;
	int			m_nMotionBlurMode;
	float		m_flShutterTime;
	Vector		m_vShutterOpenPosition;
	QAngle		m_shutterOpenAngles;
	Vector		m_vShutterClosePosition;
	QAngle		m_shutterCloseAngles;
	float		m_flOffCenterTop;
	float		m_flOffCenterBottom;
	float		m_flOffCenterLeft;
	float		m_flOffCenterRight;
	int			m_EdgeBlur;
};

#pragma once

enum Bone : int
{
	BONE_PELVIS = 0,
	LEAN_ROOT,
	CAM_DRIVER,
	BONE_HIP,
	BONE_LOWER_SPINAL_COLUMN,
	BONE_MIDDLE_SPINAL_COLUMN,
	BONE_UPPER_SPINAL_COLUMN,
	BONE_NECK,
	BONE_HEAD,
};


enum Hitboxes : int
{
	HITBOX_HEAD,
	HITBOX_NECK,
	HITBOX_PELVIS,
	HITBOX_BODY,
	HITBOX_THORAX,
	HITBOX_CHEST,
	HITBOX_UPPER_CHEST,
	HITBOX_RIGHT_THIGH,
	HITBOX_LEFT_THIGH,
	HITBOX_RIGHT_CALF,
	HITBOX_LEFT_CALF,
	HITBOX_RIGHT_FOOT,
	HITBOX_LEFT_FOOT,
	HITBOX_RIGHT_HAND,
	HITBOX_LEFT_HAND,
	HITBOX_RIGHT_UPPER_ARM,
	HITBOX_RIGHT_FOREARM,
	HITBOX_LEFT_UPPER_ARM,
	HITBOX_LEFT_FOREARM,
	HITBOX_MAX
};

enum CSWeaponType {
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN
};

class C_TEFireBullets
{
public:
	char pad[12];
	int		m_iPlayer; //12
	int _m_iItemDefinitionIndex;
	Vector	_m_vecOrigin;
	QAngle	m_vecAngles;
	int		_m_iWeaponID;
	int		m_iMode;
	int		m_iSeed;
	float	m_flSpread;

	/*
	char pad[16];
	int m_iPlayer; //0x10
	int m_iItemDefinitionIndex;
	Vector m_vecOrigin;
	QAngle m_vecAngles;
	int m_iWeapon;
	int m_iWeaponID;
	int m_iMode;
	int m_iSeed;
	float m_flInaccuracy;
	float m_flRecoilIndex;
	float m_flSpread;
	int m_iSoundType; //0x4C
	*/
};

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
struct vcollide_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
struct cplane_t;
typedef CGameTrace trace_t;
struct studiohdr_t;
struct virtualmodel_t;
typedef unsigned char byte;
struct virtualterrainparams_t;
class CPhysCollide;
typedef unsigned short MDLHandle_t;
class CUtlBuffer;
class IClientRenderable;
class Quaternion;
class mstudioautolayer_t;
class mstudioiklock_t;//mstudioactivitymodifier_t
class mstudioactivitymodifier_t;
struct mstudioanimdesc_t;
struct mstudioevent_t
{
	//DECLARE_BYTESWAP_DATADESC();
	float				cycle;
	int					event;
	int					type;
	inline const char * pszOptions(void) const { return options; }
	char				options[64];

	int					szeventindex;
	inline char * const pszEventName(void) const { return ((char *)this) + szeventindex; }
};

struct mstudioseqdesc_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int					baseptr;
	inline studiohdr_t	*pStudiohdr(void) const { return (studiohdr_t *)(((byte *)this) + baseptr); }

	int					szlabelindex;
	inline char * const pszLabel(void) const { return ((char *)this) + szlabelindex; }

	int					szactivitynameindex;
	inline char * const pszActivityName(void) const { return ((char *)this) + szactivitynameindex; }

	int					flags;		// looping/non-looping flags

	int					activity;	// initialized at loadtime to game DLL values
	int					actweight;

	int					numevents;
	int					eventindex;
	inline mstudioevent_t *pEvent(int i) const { Assert(i >= 0 && i < numevents); return (mstudioevent_t *)(((byte *)this) + eventindex) + i; };

	Vector				bbmin;		// per sequence bounding box
	Vector				bbmax;

	int					numblends;

	// Index into array of shorts which is groupsize[0] x groupsize[1] in length
	int					animindexindex;

	inline int			anim(int x, int y) const
	{
		if (x >= groupsize[0])
		{
			x = groupsize[0] - 1;
		}

		if (y >= groupsize[1])
		{
			y = groupsize[1] - 1;
		}

		int offset = y * groupsize[0] + x;
		short *blends = (short *)(((byte *)this) + animindexindex);
		int value = (int)blends[offset];
		return value;
	}

	int					movementindex;	// [blend] float array for blended movement
	int					groupsize[2];
	int					paramindex[2];	// X, Y, Z, XR, YR, ZR
	float				paramstart[2];	// local (0..1) starting value
	float				paramend[2];	// local (0..1) ending value
	int					paramparent;

	float				fadeintime;		// ideal cross fate in time (0.2 default)
	float				fadeouttime;	// ideal cross fade out time (0.2 default)

	int					localentrynode;		// transition node at entry
	int					localexitnode;		// transition node at exit
	int					nodeflags;		// transition rules

	float				entryphase;		// used to match entry gait
	float				exitphase;		// used to match exit gait

	float				lastframe;		// frame that should generation EndOfSequence

	int					nextseq;		// auto advancing sequences
	int					pose;			// index of delta animation between end and nextseq

	int					numikrules;

	int					numautolayers;	//
	int					autolayerindex;
	inline int *pAutolayer(int i) const { Assert(i >= 0 && i < numautolayers); return (int *)(((byte *)this) + autolayerindex) + i; };

	int					weightlistindex;
	inline float		*pBoneweight(int i) const { return ((float *)(((byte *)this) + weightlistindex) + i); };
	inline float		weight(int i) const { return *(pBoneweight(i)); };

	// FIXME: make this 2D instead of 2x1D arrays
	int					posekeyindex;
	float				*pPoseKey(int iParam, int iAnim) const { return (float *)(((byte *)this) + posekeyindex) + iParam * groupsize[0] + iAnim; }
	float				poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

	int					numiklocks;
	int					iklockindex;
	inline int *pIKLock(int i) const { Assert(i >= 0 && i < numiklocks); return (int*)(((byte *)this) + iklockindex) + i; };

	// Key values
	int					keyvalueindex;
	int					keyvaluesize;
	inline const char * KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

	int					cycleposeindex;		// index of pose parameter to use as cycle index

	int					activitymodifierindex;
	int					numactivitymodifiers;
	inline int *pActivityModifier(int i) const { Assert(i >= 0 && i < numactivitymodifiers); return activitymodifierindex != 0 ? (int *)(((byte *)this) + activitymodifierindex) + i : NULL; };

	int					unused[5];		// remove/add as appropriate (grow back to 8 ints on version change!)

	mstudioseqdesc_t() {}
private:
	// No copy constructors allowed
	mstudioseqdesc_t(const mstudioseqdesc_t& vOther);
};
struct mstudiobodyparts_t;
struct mstudiotexture_t;

class RadianEuler
{
public:
	inline RadianEuler(void) { }
	inline RadianEuler(float X, float Y, float Z) { x = X; y = Y; z = Z; }
	inline RadianEuler(Quaternion const &q);	// evil auto type promotion!!!
	inline RadianEuler(QAngle const &angles);	// evil auto type promotion!!!

												// Initialization
	inline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f) { x = ix; y = iy; z = iz; }

	//	conversion to qangle
	QAngle ToQAngle(void) const;
	bool IsValid() const;
	void Invalidate();

	inline float *Base() { return &x; }
	inline const float *Base() const { return &x; }

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	float x, y, z;
};

class Quaternion				// same data-layout as engine's vec4_t,
{								//		which is a float[4]
public:
	inline Quaternion(void) {}
	inline Quaternion(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) { }
	inline Quaternion(RadianEuler const &angle);	// evil auto type promotion!!!

	inline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f, float iw = 0.0f) { x = ix; y = iy; z = iz; w = iw; }

	bool IsValid() const;
	void Invalidate();

	bool operator==(const Quaternion &src) const;
	bool operator!=(const Quaternion &src) const;

	float* Base() { return (float*)this; }
	const float* Base() const { return (float*)this; }

	// array access...
	float operator[](int i) const;
	float& operator[](int i);

	float x, y, z, w;
};



struct mstudiohitboxset_t
{
	int sznameindex;

	inline char * const pszName(void) const
	{
		return ((char*)this) + sznameindex;
	}

	int numhitboxes;
	int hitboxindex;

	inline mstudiobbox_t* pHitbox(int i) const
	{
		return (mstudiobbox_t*)(((byte*)this) + hitboxindex) + i;
	}
};

struct mstudiobone_t
{
	int					sznameindex;
	inline char * const pszName(void) const { return ((char *)this) + sznameindex; }
	int		 			parent;		// parent bone
	int					bonecontroller[6];	// bone controller index, -1 == none

											// default values
	Vector				pos;
	Quaternion			quat;
	RadianEuler			rot;
	// compression scale
	Vector				posscale;
	Vector				rotscale;

	matrix3x4_t			poseToBone;
	Quaternion			qAlignment;
	int					flags;
	int					proctype;
	int					procindex;		// procedural rule
	mutable int			physicsbone;	// index into physically simulated bone
	inline void *pProcedure() const { if (procindex == 0) return NULL; else return  (void *)(((byte *)this) + procindex); };
	int					surfacepropidx;	// index into string tablefor property name
	inline char * const pszSurfaceProp(void) const { return ((char *)this) + surfacepropidx; }
	inline int			GetSurfaceProp(void) const { return surfacepropLookup; }

	int					contents;		// See BSPFlags.h for the contents flags
	int					surfacepropLookup;	// this index must be cached by the loader, not saved in the file
	int					unused[7];		// remove as appropriate
};

class virtualgroup_t
{
public:
	virtualgroup_t(void) { cache = NULL; };
	// tool dependant.  In engine this is a model_t, in tool it's a direct pointer
	void *cache;
	// converts cache entry into a usable studiohdr_t *
	const studiohdr_t *GetStudioHdr(void) const {};

	CUtlVector< int > boneMap;				// maps global bone to local bone
	CUtlVector< int > masterBone;			// maps local bone to global bone
	CUtlVector< int > masterSeq;			// maps local sequence to master sequence
	CUtlVector< int > masterAnim;			// maps local animation to master animation
	CUtlVector< int > masterAttachment;	// maps local attachment to global
	CUtlVector< int > masterPose;			// maps local pose parameter to global
	CUtlVector< int > masterNode;			// maps local transition nodes to global
};

struct virtualsequence_t
{
#ifdef _XBOX
	short flags;
	short activity;
	short group;
	short index;
#else
	int	flags;
	int activity;
	int group;
	int index;
#endif
};

struct virtualgeneric_t
{
#ifdef _XBOX
	short group;
	short index;
#else
	int group;
	int index;
#endif
};

struct mstudiobonecontroller_t
{
	int					bone;	// -1 == 0
	int					type;	// X, Y, Z, XR, YR, ZR, M
	float				start;
	float				end;
	int					rest;	// byte index value at rest
	int					inputfield;	// 0-3 user set controller, 4 mouth
	int					unused[8];
};

struct studiohdr_t
{
	int					id;
	int					version;

	long				checksum;		// this has to be the same in the phy and vtx files to load!

	inline const char *	pszName(void) const { return name; }
	char				name[64];

	int					length;

	Vector				eyeposition;	// ideal eye position

	Vector				illumposition;	// illumination center

	Vector				hull_min;		// ideal movement hull size
	Vector				hull_max;

	Vector				view_bbmin;		// clipping bounding box
	Vector				view_bbmax;

	int					flags;

	int					numbones;			// bones
	int					boneindex;
	inline mstudiobone_t *pBone(int i) const { Assert(i >= 0 && i < numbones); return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };
	int					RemapSeqBone(int iSequence, int iLocalBone) const;	// maps local sequence bone to global bone
	int					RemapAnimBone(int iAnim, int iLocalBone) const;		// maps local animations bone to global bone

	int					numbonecontrollers;		// bone controllers
	int					bonecontrollerindex;
	//inline mstudiobonecontroller_t *pBonecontroller( int i ) const { Assert( i >= 0 && i < numbonecontrollers ); return ( mstudiobonecontroller_t * )( ( ( byte * )this ) + bonecontrollerindex ) + i; };

	int					numhitboxsets;
	int					hitboxsetindex;

	// Look up hitbox set by index
	mstudiohitboxset_t	*pHitboxSet(int i) const
	{
		Assert(i >= 0 && i < numhitboxsets);
		return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex) + i;
	};

	// Calls through to hitbox to determine size of specified set
	inline mstudiobbox_t *pHitbox(int i, int set) const
	{
		mstudiohitboxset_t const *s = pHitboxSet(set);
		if (!s)
			return NULL;

		return s->pHitbox(i);
	};

	// Calls through to set to get hitbox count for set
	inline int			iHitboxCount(int set) const
	{
		mstudiohitboxset_t const *s = pHitboxSet(set);
		if (!s)
			return 0;

		return s->numhitboxes;
	};

	// file local animations? and sequences
	//private:
	int					numlocalanim;			// animations/poses
	int					localanimindex;		// animation descriptions
	inline mstudioanimdesc_t *pLocalAnimdesc(int i) const { return NULL; };

	int					numlocalseq;				// sequences
	int					localseqindex;
	inline mstudioseqdesc_t *pLocalSeqdesc(int i) const {
		if (i < 0 || i >= numlocalseq)
			i = 0;
		return (mstudioseqdesc_t *)(((byte *)this) + localseqindex) + i;

	};

	//public:
	bool				SequencesAvailable() const
	{
		return true;
	}

	int					GetNumSeq() const;
	mstudioanimdesc_t	&pAnimdesc(int i) const;
	inline mstudioseqdesc_t	&pSeqdesc(int i) const {
		int v2; // eax@2
		int v3; // ecx@2

		v2 = i;
		v3 = *(DWORD*)this;
		if (i < 0 || i >= *(int*)(v3 + 188))
			v2 = 0;

		return *(mstudioseqdesc_t*)(v3 + *(DWORD*)(v3 + 192) + 212 * v2);
	};
	int					iRelativeAnim(int baseseq, int relanim) const;	// maps seq local anim reference to global anim index
	int					iRelativeSeq(int baseseq, int relseq) const;		// maps seq local seq reference to global seq index
																			//private:
	mutable int			activitylistversion;	// initialization flag - have the sequences been indexed?
	mutable int			eventsindexed;
	//public:
	int					GetSequenceActivity(int iSequence);
	void				SetSequenceActivity(int iSequence, int iActivity);
	int					GetActivityListVersion(void);
	void				SetActivityListVersion(int version) const;
	int					GetEventListVersion(void);
	void				SetEventListVersion(int version);

	// raw textures
	int					numtextures;
	int					textureindex;
	inline mstudiotexture_t *pTexture(int i) const { return NULL; };


	// raw textures search paths
	int					numcdtextures;
	int					cdtextureindex;
	inline char			*pCdtexture(int i) const { return (((char *)this) + *((int *)(((byte *)this) + cdtextureindex) + i)); };

	// replaceable textures tables
	int					numskinref;
	int					numskinfamilies;
	int					skinindex;
	inline short		*pSkinref(int i) const { return (short *)(((byte *)this) + skinindex) + i; };

	int					numbodyparts;
	int					bodypartindex;
	inline mstudiobodyparts_t	*pBodypart(int i) const { return NULL; };
	// queryable attachable points
	//private:
	int					numlocalattachments;
	int					localattachmentindex;
	inline int	*pLocalAttachment(int i) const { Assert(i >= 0 && i < numlocalattachments); return (int *)(((byte *)this) + localattachmentindex) + i; };
	//public:
	int					GetNumAttachments(void) const;
	const void			*pAttachment(int i) const;
	int					GetAttachmentBone(int i);
	// used on my tools in hlmv, not persistant
	void				SetAttachmentBone(int iAttachment, int iBone);

	// animation node to animation node transition graph
	//private:
	int					numlocalnodes;
	int					localnodeindex;
	int					localnodenameindex;
	inline char			*pszLocalNodeName(int iNode) const { Assert(iNode >= 0 && iNode < numlocalnodes); return (((char *)this) + *((int *)(((byte *)this) + localnodenameindex) + iNode)); }
	inline byte			*pLocalTransition(int i) const { Assert(i >= 0 && i < (numlocalnodes * numlocalnodes)); return (byte *)(((byte *)this) + localnodeindex) + i; };

	//public:
	int					EntryNode(int iSequence);
	int					ExitNode(int iSequence);
	char				*pszNodeName(int iNode);
	int					GetTransition(int iFrom, int iTo) const;

	int					numflexdesc;
	int					flexdescindex;
	inline int *pFlexdesc(int i) const { Assert(i >= 0 && i < numflexdesc); return (int *)(((byte *)this) + flexdescindex) + i; };

	int					numflexcontrollers;
	int					flexcontrollerindex;
	inline int *pFlexcontroller(int i) const { Assert(i >= 0 && i < numflexcontrollers); return (int *)(((byte *)this) + flexcontrollerindex) + i; };

	int					numflexrules;
	int					flexruleindex;
	inline int *pFlexRule(int i) const { Assert(i >= 0 && i < numflexrules); return (int *)(((byte *)this) + flexruleindex) + i; };

	int					numikchains;
	int					ikchainindex;
	inline int *pIKChain(int i) const { Assert(i >= 0 && i < numikchains); return (int *)(((byte *)this) + ikchainindex) + i; };

	int					nummouths;
	int					mouthindex;
	inline int *pMouth(int i) const { Assert(i >= 0 && i < nummouths); return (int *)(((byte *)this) + mouthindex) + i; };

	//private:
	int					numlocalposeparameters;
	int					localposeparamindex;
	inline int *pLocalPoseParameter(int i) const { Assert(i >= 0 && i < numlocalposeparameters); return (int *)(((byte *)this) + localposeparamindex) + i; };
	//public:
	int					GetNumPoseParameters(void) const;
	const void *pPoseParameter(int i);
	int					GetSharedPoseParameter(int iSequence, int iLocalPose) const;

	int					surfacepropindex;
	inline char * const pszSurfaceProp(void) const { return ((char *)this) + surfacepropindex; }
	inline int			GetSurfaceProp() const { return NULL; }

	// Key values
	int					keyvalueindex;
	int					keyvaluesize;
	inline const char * KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

	int					numlocalikautoplaylocks;
	int					localikautoplaylockindex;
	inline int *pLocalIKAutoplayLock(int i) const { Assert(i >= 0 && i < numlocalikautoplaylocks); return (int *)(((byte *)this) + localikautoplaylockindex) + i; };

	int					GetNumIKAutoplayLocks(void) const;
	const mstudioiklock_t &pIKAutoplayLock(int i);
	int					CountAutoplaySequences() const;
	int					CopyAutoplaySequences(unsigned short *pOut, int outCount) const;
	int					GetAutoplayList(unsigned short **pOut) const;

	// The collision model mass that jay wanted
	float				mass;
	int					contents;

	// external animations, models, etc.
	int					numincludemodels;
	int					includemodelindex;
	inline int *pModelGroup(int i) const { Assert(i >= 0 && i < numincludemodels); return (int *)(((byte *)this) + includemodelindex) + i; };
	// implementation specific call to get a named model
	const studiohdr_t	*FindModel(void **cache, char const *modelname) const;

	// implementation specific back pointer to virtual data
	mutable void		*virtualModel;
	virtualmodel_t		*GetVirtualModel(void) const;

};

class CStudioHdr
{
public:
	studiohdr_t * m_pStudioHdr;
};

class CUtlVectorSimple
{
public:
	unsigned memory;
	char pad[8];
	unsigned int count;
	inline void* Retrieve(int index, unsigned sizeofdata)
	{
		return (void*)((*(unsigned*)this) + (sizeofdata * index));
	}
};

//-----------------------------------------------------------------------------
// Indicates the type of translucency of an unmodulated renderable
//-----------------------------------------------------------------------------
enum RenderableTranslucencyType_t
{
	RENDERABLE_IS_OPAQUE = 0,
	RENDERABLE_IS_TRANSLUCENT,
	RENDERABLE_IS_TWO_PASS,	// has both translucent and opaque sub-partsa
};

enum MDLCacheDataType_t
{
	// Callbacks to get called when data is loaded or unloaded for these:
	MDLCACHE_STUDIOHDR = 0,
	MDLCACHE_STUDIOHWDATA,
	MDLCACHE_VCOLLIDE,

	// Callbacks NOT called when data is loaded or unloaded for these:
	MDLCACHE_ANIMBLOCK,
	MDLCACHE_VIRTUALMODEL,
	MDLCACHE_VERTEXES,
	MDLCACHE_DECODEDANIMBLOCK
};

class IMDLCacheNotify
{
public:
	// Called right after the data is loaded
	virtual void OnDataLoaded(MDLCacheDataType_t type, MDLHandle_t handle) = 0;

	// Called right before the data is unloaded
	virtual void OnDataUnloaded(MDLCacheDataType_t type, MDLHandle_t handle) = 0;
};

enum MDLCacheFlush_t
{
	MDLCACHE_FLUSH_STUDIOHDR = 0x01,
	MDLCACHE_FLUSH_STUDIOHWDATA = 0x02,
	MDLCACHE_FLUSH_VCOLLIDE = 0x04,
	MDLCACHE_FLUSH_ANIMBLOCK = 0x08,
	MDLCACHE_FLUSH_VIRTUALMODEL = 0x10,
	MDLCACHE_FLUSH_AUTOPLAY = 0x20,
	MDLCACHE_FLUSH_VERTEXES = 0x40,

	MDLCACHE_FLUSH_IGNORELOCK = 0x80000000,
	MDLCACHE_FLUSH_ALL = 0xFFFFFFFF
};

//struct studiohdr_t;
struct studiohwdata_t;
//struct vcollide_t;
//struct virtualmodel_t;
struct vertexFileHeader_t;

namespace OptimizedModel
{
	struct FileHeader_t;
}

//-----------------------------------------------------------------------------
// Model info interface
//-----------------------------------------------------------------------------
class IMDLCache
{
public:
	// Used to install callbacks for when data is loaded + unloaded
	// Returns the prior notify
	virtual void SetCacheNotify(IMDLCacheNotify *pNotify) = 0;

	// NOTE: This assumes the "GAME" path if you don't use
	// the UNC method of specifying files. This will also increment
	// the reference count of the MDL
	virtual MDLHandle_t FindMDL(const char *pMDLRelativePath) = 0;

	// Reference counting
	virtual int AddRef(MDLHandle_t handle) = 0;
	virtual int Release(MDLHandle_t handle) = 0;
	virtual int GetRef(MDLHandle_t handle) = 0;

	// Gets at the various data associated with a MDL
	virtual studiohdr_t *GetStudioHdr(MDLHandle_t handle) = 0;
	virtual studiohwdata_t *GetHardwareData(MDLHandle_t handle) = 0;
	virtual vcollide_t *GetVCollide(MDLHandle_t handle) = 0;
	virtual unsigned char *GetAnimBlock(MDLHandle_t handle, int nBlock) = 0;
	virtual virtualmodel_t *GetVirtualModel(MDLHandle_t handle) = 0;
	virtual int GetAutoplayList(MDLHandle_t handle, unsigned short **pOut) = 0;
	virtual vertexFileHeader_t *GetVertexData(MDLHandle_t handle) = 0;

	// Brings all data associated with an MDL into memory
	virtual void TouchAllData(MDLHandle_t handle) = 0;

	// Gets/sets user data associated with the MDL
	virtual void SetUserData(MDLHandle_t handle, void* pData) = 0;
	virtual void *GetUserData(MDLHandle_t handle) = 0;

	// Is this MDL using the error model?
	virtual bool IsErrorModel(MDLHandle_t handle) = 0;

	// Flushes the cache, force a full discard
	virtual void Flush(MDLCacheFlush_t nFlushFlags = MDLCACHE_FLUSH_ALL) = 0;

	// Flushes a particular model out of memory
	virtual void Flush(MDLHandle_t handle, int nFlushFlags = MDLCACHE_FLUSH_ALL) = 0;

	// Returns the name of the model (its relative path)
	virtual const char *GetModelName(MDLHandle_t handle) = 0;

	// faster access when you already have the studiohdr
	virtual virtualmodel_t *GetVirtualModelFast(const studiohdr_t *pStudioHdr, MDLHandle_t handle) = 0;

	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	virtual void BeginLock() = 0;

	// reset all protected blocks to normal
	virtual void EndLock() = 0;

	// returns a pointer to a counter that is incremented every time the cache has been out of the locked state (EVIL)
	virtual int *GetFrameUnlockCounterPtrOLD() = 0;

	// Finish all pending async operations
	virtual void FinishPendingLoads() = 0;

	virtual vcollide_t *GetVCollideEx(MDLHandle_t handle, bool synchronousLoad = true) = 0;
	virtual bool GetVCollideSize(MDLHandle_t handle, int *pVCollideSize) = 0;

	virtual bool GetAsyncLoad(MDLCacheDataType_t type) = 0;
	virtual bool SetAsyncLoad(MDLCacheDataType_t type, bool bAsync) = 0;

	virtual void BeginMapLoad() = 0;
	virtual void EndMapLoad() = 0;
	virtual void MarkAsLoaded(MDLHandle_t handle) = 0;

	virtual void InitPreloadData(bool rebuild) = 0;
	virtual void ShutdownPreloadData() = 0;

	virtual bool IsDataLoaded(MDLHandle_t handle, MDLCacheDataType_t type) = 0;

	virtual int *GetFrameUnlockCounterPtr(MDLCacheDataType_t type) = 0;

	virtual studiohdr_t *LockStudioHdr(MDLHandle_t handle) = 0;
	virtual void UnlockStudioHdr(MDLHandle_t handle) = 0;

	virtual bool PreloadModel(MDLHandle_t handle) = 0;

	// Hammer uses this. If a model has an error loading in GetStudioHdr, then it is flagged
	// as an error model and any further attempts to load it will just get the error model.
	// That is, until you call this function. Then it will load the correct model.
	virtual void ResetErrorModelStatus(MDLHandle_t handle) = 0;

	virtual void MarkFrame() = 0;

	// Locking for things that we can lock over longer intervals than
	// resources locked by BeginLock/EndLock
	virtual void BeginCoarseLock() = 0;
	virtual void EndCoarseLock() = 0;

	virtual void ReloadVCollide(MDLHandle_t handle) = 0;
};


class IVModelInfo
{
public:
	virtual							~IVModelInfo(void) { }
	virtual const model_t			*GetModel(int modelindex) const = 0;
	// Returns index of model by name
	virtual int						GetModelIndex(const char *name) const = 0;
	// Returns name of model
	virtual const char				*GetModelName(const model_t *model) const = 0;
	virtual vcollide_t				*GetVCollide(const model_t *model) const = 0;
	virtual vcollide_t				*GetVCollide(int modelindex) const = 0;
	virtual vcollide_t				*new_GetVCollide(bool modelindex) const = 0;
	virtual vcollide_t				*new_GetVCollide(int modelindex) const = 0;
	virtual void					GetModelBounds(const model_t *model, Vector& mins, Vector& maxs) const = 0;
	virtual	void					GetModelRenderBounds(const model_t *model, Vector& mins, Vector& maxs) const = 0;
	virtual int						GetModelFrameCount(const model_t *model) const = 0;
	virtual int						GetModelType(const model_t *model) const = 0;
	virtual void					*GetModelExtraData(const model_t *model) = 0;
	virtual bool					ModelHasMaterialProxy(const model_t *model) const = 0;
	virtual bool					IsTranslucent(model_t const* model) const = 0;
	virtual bool					IsTranslucentTwoPass(const model_t *model) const = 0;
	virtual void					Unused0() {};
	virtual RenderableTranslucencyType_t ComputeTranslucencyType(const model_t *model, int nSkin, int nBody) = 0;
	virtual int						GetModelMaterialCount(const model_t* model) const = 0;
	virtual void					GetModelMaterials(const model_t *model, int count, IMaterial** ppMaterials) = 0;
	virtual bool					IsModelVertexLit(const model_t *model) const = 0;
	virtual const char				*GetModelKeyValueText(const model_t *model) = 0;
	virtual bool					GetModelKeyValue(const model_t *model, CUtlBuffer &buf) = 0; // supports keyvalue blocks in submodels
	virtual float					GetModelRadius(const model_t *model) = 0;

	virtual const studiohdr_t		*FindModel(const studiohdr_t *pStudioHdr, void **cache, const char *modelname) const = 0;
	virtual const studiohdr_t		*FindModel(void *cache) const = 0;
	virtual	virtualmodel_t			*GetVirtualModel(const studiohdr_t *pStudioHdr) const = 0;
	virtual byte					*GetAnimBlock(const studiohdr_t *pStudioHdr, int iBlock) const = 0;
	virtual bool					HasAnimBlockBeenPreloaded(studiohdr_t const*, int) const = 0;

	// Available on client only!!!
	virtual void					GetModelMaterialColorAndLighting(const model_t *model, Vector const& origin,
		QAngle const& angles, trace_t* pTrace,
		Vector& lighting, Vector& matColor) = 0;
	virtual void					GetIlluminationPoint(const model_t *model, IClientRenderable *pRenderable, Vector const& origin,
		QAngle const& angles, Vector* pLightingCenter) = 0;

	virtual int						GetModelContents(int modelIndex) const = 0;
	virtual studiohdr_t				*GetStudioModel(const model_t *mod) = 0;
	virtual int						GetModelSpriteWidth(const model_t *model) const = 0;
	virtual int						GetModelSpriteHeight(const model_t *model) const = 0;

	// Sets/gets a map-specified fade range (client only)
	virtual void					SetLevelScreenFadeRange(float flMinSize, float flMaxSize) = 0;
	virtual void					GetLevelScreenFadeRange(float *pMinArea, float *pMaxArea) const = 0;

	// Sets/gets a map-specified per-view fade range (client only)
	virtual void					SetViewScreenFadeRange(float flMinSize, float flMaxSize) = 0;

	// Computes fade alpha based on distance fade + screen fade (client only)
	virtual unsigned char			ComputeLevelScreenFade(const Vector &vecAbsOrigin, float flRadius, float flFadeScale) const = 0;
	virtual unsigned char			ComputeViewScreenFade(const Vector &vecAbsOrigin, float flRadius, float flFadeScale) const = 0;

	// both client and server
	virtual int						GetAutoplayList(const studiohdr_t *pStudioHdr, unsigned short **pAutoplayList) const = 0;

	// Gets a virtual terrain collision model (creates if necessary)
	// NOTE: This may return NULL if the terrain model cannot be virtualized
	virtual CPhysCollide			*GetCollideForVirtualTerrain(int index) = 0;
	virtual bool					IsUsingFBTexture(const model_t *model, int nSkin, int nBody, void /*IClientRenderable*/ *pClientRenderable) const = 0;
	virtual const model_t			*FindOrLoadModel(const char *name) const = 0;
	virtual MDLHandle_t				GetCacheHandle(const model_t *model) const = 0;
	// Returns planes of non-nodraw brush model surfaces
	virtual int						GetBrushModelPlaneCount(const model_t *model) const = 0;
	virtual void					GetBrushModelPlane(const model_t *model, int nIndex, cplane_t &plane, Vector *pOrigin) const = 0;
	virtual int						GetSurfacepropsForVirtualTerrain(int index) = 0;
	virtual bool					UsesEnvCubemap(const model_t *model) const = 0;
	virtual bool					UsesStaticLighting(const model_t *model) const = 0;
};

struct SurfacePhysicsParams_t {
	float friction;
	float elasticity;
	float density;
	float thickness;
	float dampening;
};

struct SurfaceAudioParams_t {
	float reflectivity; // like elasticity, but how much sound should be reflected by this surface
	float hardnessFactor; // like elasticity, but only affects impact sound choices
	float roughnessFactor; // like friction, but only affects scrape sound choices   
	float roughThreshold; // surface roughness > this causes "rough" scrapes, < this causes "smooth" scrapes
	float hardThreshold; // surface hardness > this causes "hard" impacts, < this causes "soft" impacts
	float hardVelocityThreshold; // collision velocity > this causes "hard" impacts, < this causes "soft" impacts   
	float highPitchOcclusion;
	//a value betweeen 0 and 100 where 0 is not occluded at all and 100 is silent (except for any additional reflected sound)
	float midPitchOcclusion;
	float lowPitchOcclusion;
};

struct SurfaceSoundNames_t {
	unsigned short walkstepleft;
	unsigned short walkstepright;
	unsigned short runstepleft;
	unsigned short runstepright;
	unsigned short impactsoft;
	unsigned short impacthard;
	unsigned short scrapesmooth;
	unsigned short scraperough;
	unsigned short bulletimpact;
	unsigned short rolling;
	unsigned short breaksound;
	unsigned short strainsound;
};

struct SurfaceSoundHandles_t {
	short walkstepleft;
	short walkstepright;
	short runstepleft;
	short runstepright;
	short impactsoft;
	short impacthard;
	short scrapesmooth;
	short scraperough;
	short bulletimpact;
	short rolling;
	short breaksound;
	short strainsound;
};

struct SurfaceGameProps_t {
	float maxspeedfactor;
	float jumpfactor;
	float penetrationmodifier;
	float damagemodifier;
	uint16_t material;
	uint8_t climbable;
};

struct surface_data_t {
	SurfacePhysicsParams_t physics;
	SurfaceAudioParams_t audio;
	SurfaceSoundNames_t sounds;
	SurfaceGameProps_t game;
};

class IPhysicsSurfaceProps
{
public:
	virtual ~IPhysicsSurfaceProps(void) {}

	// parses a text file containing surface prop keys
	virtual int		ParseSurfaceData(const char *pFilename, const char *pTextfile) = 0;
	// current number of entries in the database
	virtual int		SurfacePropCount(void) const = 0;

	virtual int		GetSurfaceIndex(const char *pSurfacePropName) const = 0;
	virtual void	GetPhysicsProperties(int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity) const = 0;

	virtual surface_data_t	*GetSurfaceData(int surfaceDataIndex) = 0;
	virtual const char		*GetString(unsigned short stringTableIndex) const = 0;


	virtual const char		*GetPropName(int surfaceDataIndex) const = 0;

	// sets the global index table for world materials
	// UNDONE: Make this per-CPhysCollide
	virtual void	SetWorldMaterialIndexTable(int *pMapArray, int mapSize) = 0;

	// NOTE: Same as GetPhysicsProperties, but maybe more convenient
	virtual void	GetPhysicsParameters(int surfaceDataIndex, SurfacePhysicsParams_t *pParamsOut) const = 0;
};

struct Ray_t;

struct virtualmeshlist_t;

class ITraceListData
{
public:
	virtual ~ITraceListData() {}

	virtual void Reset() = 0;
	virtual bool IsEmpty() = 0;
	// CanTraceRay will return true if the current volume encloses the ray
	// NOTE: The leaflist trace will NOT check this.  Traces are intersected
	// against the culled volume exclusively.
	virtual bool CanTraceRay(const Ray_t &ray) = 0;
};

class ITraceFilter;

class IEntityEnumerator
{
public:
	// This gets called with each handle
	virtual bool EnumEntity(IHandleEntity *pHandleEntity) = 0;
};

struct BrushSideInfo_t
{
	Vector plane;			// The plane of the brush side
	float planec;
	unsigned short bevel;	// Bevel plane?
	unsigned short thin;	// Thin?
};

enum DebugTraceCounterBehavior_t
{
	kTRACE_COUNTER_SET = 0,
	kTRACE_COUNTER_INC,
};

class IEngineTrace
{
public:
	// Returns the contents mask + entity at a particular world-space position
	virtual int		GetPointContents(const Vector &vecAbsPosition, int contentsMask = MASK_ALL, IHandleEntity** ppEntity = NULL) = 0;

	// Returns the contents mask of the world only @ the world-space position (static props are ignored)
	virtual int		GetPointContents_WorldOnly(const Vector &vecAbsPosition, int contentsMask = MASK_ALL) = 0;

	// Get the point contents, but only test the specific entity. This works
	// on static props and brush models.
	//
	// If the entity isn't a static prop or a brush model, it returns CONTENTS_EMPTY and sets
	// bFailed to true if bFailed is non-null.
	virtual int		GetPointContents_Collideable(ICollideable *pCollide, const Vector &vecAbsPosition) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToEntity(const Ray_t &ray, unsigned int fMask, IHandleEntity *pEnt, trace_t *pTrace) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToCollideable(const Ray_t &ray, unsigned int fMask, ICollideable *pCollide, trace_t *pTrace) = 0;

	// A version that simply accepts a ray (can work as a traceline or tracehull)
	virtual void	TraceRay(const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// A version that sets up the leaf and entity lists and allows you to pass those in for collision.
	virtual void	SetupLeafAndEntityListRay(const Ray_t &ray, ITraceListData *pTraceData) = 0;
	virtual void    SetupLeafAndEntityListBox(const Vector &vecBoxMin, const Vector &vecBoxMax, ITraceListData *pTraceData) = 0;
	virtual void	TraceRayAgainstLeafAndEntityList(const Ray_t &ray, ITraceListData *pTraceData, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// A version that sweeps a collideable through the world
	// abs start + abs end represents the collision origins you want to sweep the collideable through
	// vecAngles represents the collision angles of the collideable during the sweep
	virtual void	SweepCollideable(ICollideable *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd,
		const QAngle &vecAngles, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// Enumerates over all entities along a ray
	// If triggers == true, it enumerates all triggers along a ray
	virtual void	EnumerateEntities(const Ray_t &ray, bool triggers, IEntityEnumerator *pEnumerator) = 0;

	// Same thing, but enumerate entitys within a box
	virtual void	EnumerateEntities(const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator) = 0;

	// Convert a handle entity to a collideable.  Useful inside enumer
	virtual ICollideable *GetCollideable(IHandleEntity *pEntity) = 0;

	// HACKHACK: Temp for performance measurments
	virtual int GetStatByIndex(int index, bool bClear) = 0;

	//finds brushes in an AABB, prone to some false positives
	virtual void lolignorethisaswellrifk() = 0;

	//Creates a CPhysCollide out of all displacements wholly or partially contained in the specified AABB
	virtual void GetCollidableFromDisplacementsInAABB() = 0;

	// gets the number of displacements in the world
	virtual int GetNumDisplacements() = 0;

	// gets a specific diplacement mesh
	virtual void GetDisplacementMesh(int nIndex, virtualmeshlist_t *pMeshTriList) = 0;

	//retrieve brush planes and contents, returns true if data is being returned in the output pointers, false if the brush doesn't exist
	virtual bool lolignorethis() = 0;

	virtual bool PointOutsideWorld(const Vector &ptTest) = 0; //Tests a point to see if it's outside any playable area

															  // Walks bsp to find the leaf containing the specified point
	virtual int GetLeafContainingPoint(const Vector &ptTest) = 0;

	virtual ITraceListData *AllocTraceListData() = 0;
	virtual void FreeTraceListData(ITraceListData *) = 0;

	/// Used only in debugging: get/set/clear/increment the trace debug counter. See comment below for details.
	virtual int GetSetDebugTraceCounter(int value, DebugTraceCounterBehavior_t behavior) = 0; //23
};

class C_AnimationLayer
{
public:
	bool m_bClientBlend;
	float m_flBlendIn;
	void *m_pStudioHdr;
	int m_nDispatchSequence;
	int m_nDispathcSequence_2;
	int m_nOrder;
	int m_nSequence;
	float m_flPrevCycle;
	float m_flWeight;
	float m_flWeightDeltaRate;
	float m_flPlaybackRate;
	float m_flCycle;
	void *m_pOwner;
	char pad_0038[4];
};

//class CCSGOPlayerAnimState
//{
//	PAD(0x70)
//public:
//	int last_animation_update_tick;
//private:
//	PAD(0x10)
//public:
//	float goal_feet_yaw;
//private:
//	PAD(0x6C)
//public:
//	float velocity; //0xEC
//private:
//	PAD(0x1D)
//public:
//	unsigned char hit_ground_animation;
//private:
//	PAD(0xF)
//public:
//	float head_height_from_hitting_ground_anim;
//};
//Generated using ReClass 2013 Mod by CypherPresents

class CCSGOPlayerAnimState
{
public:
	//char _0x0000[8];
	//__int32 m_someTickCount; //0x0008 
	//char _0x000C[12];
	//float m_someTime; //0x0018 
	//float N01779500; //0x001C 
	//float N01779501; //0x0020 
	//char _0x0024[4];
	//float N01779503; //0x0028 
	//float m_someTime2; //0x002C 
	//float N01779505; //0x0030 
	//float N01779506; //0x0034 
	//char _0x0038[4];
	//float N0177A35A; //0x003C 
	//float N0177A35B; //0x0040 
	//float N017FBE7C; //0x0044 
	//float N017FBE7D; //0x0048 
	//float N0177A35E; //0x004C 
	//__int32 N0177B1DA; //0x0050 
	//char _0x0054[8];
	//float N0177B1D7; //0x005C 
	//char  pad[3];
	//char  bUnknown;
	//bool  m_invalid;
	//char  pad2[72];
	//int   m_model_index;
	//char  pad_[11];
	//void* m_BaseEntity; //0x0060 
	//float N0177B1D5; //0x0064 
	//float N0177B1D4; //0x0068 
	//float m_Last_Animations_Update_time; //0x006C 
	//__int32 m_Last_Animations_Update_Tick; //0x0070 
	//float m_Anim_Update_Delta; //0x0074 
	//float m_flEyeYaw; //0x0078 
	//float m_flPitch; //0x007C 
	//float m_flGoalFeetYaw; //0x0080 
	//float m_flCurrentFeetYaw; //0x0084 
	//float m_flCurrentTorsoYaw; //0x0088 
	//float m_flVelocityLean; //0x008C changes when moving/jumping/hitting ground
	//float m_flLean; //0x0090 
	//char _0x0094[4];
	//float m_flFeetCycle; //0x0098 
	//float m_flFeetYawRate; //0x009C 
	//char _0x00A0[4];
	//float m_flDuckAmount; //0x00A4 
	//float m_flLandingDuckAdditive; //0x00A8 
	//char _0x00AC[4];
	//Vector m_vecOrigin; //0x00B0 
	//Vector m_vecLastOrigin; //0x00BC 
	//Vector m_vecVelocity; //0x00C8 
	//float m_flUnknown1; //0x00D4 affected by movement/direction
	//float m_flUnknown2; //0x00D8   affected when jumping/moving
	//char _0x00DC[4];
	//Vector2D m_vecUnknown; //0x00E0 from -1 to 1 when moving and affected by direction
	//char _0x00E8[3];
	//float m_flSpeed2D; //0x00EC 
	//float m_flUpSpeed; //0x00F0 
	//float m_flSpeed2DNormalized; //0x00F4 
	//float m_flFeetSpeedForwardSideways; //0x00F8 from 0 to 2. something  is 1 when walking, 2.something when running, 0.653 when crouch walking
	//float m_flFeetSpeedUnknownForwardSideways; //0x00FC from 0 to 3. something
	//float m_flTimeSinceStartedMoving; //0x0100 
	//float m_flTimeSinceStoppedMoving; //0x0104 
	//__int8 m_bOnGround; //0x0108 
	//__int8 m_bInHitGroundAnimation; //0x0109 
	//float m_Time_Since_InAir; //0x0110
	//char _0x010A[6];
	//float m_flLastOriginZ; //0x0114 
	//float m_flHeadHeightOrOffsetFromHittingGroundAnimation; //0x0118 
	//float m_flStopToFullRunningFraction; //0x11C from 0 to 1, doesnt change when walking or crouching, only running
	//char pad8[4]; //NaN
	//float m_flUnknownFraction; //0x124 affected while jumping and running, or when just jumping, 0 to 1
	//char pad9[4]; //NaN
	//float m_flUnknown3;
	//char pad10[528];
public:
	char  pad[3];
	char  bUnknown;
	bool  m_invalid;
	char  pad2[72];
	int   m_model_index;
	char  pad_[11];
	void* ent; //0x0060 
	char _0x0064[8];
	float last_anim_upd_time; //0x006C 
	__int32 last_anim_upd_tick; //0x0070 
	float anim_update_delta; //0x0074 
	float eye_yaw; //0x0078 
	float eye_pitch; //0x007C 
	float abs_yaw; //0x0080 
	float old_abs_yaw; //0x0084 
	float torso_yaw; //0x0088 
	float vel_lean; //0x008C 
	float lean; //0x0090 
	char _0x0094[4];
	float feet_cycle; //0x0098 
	float feet_rate; //0x009C 
	char _0x00A0[4];
	float duck_amt; //0x00A4 
	float landing_duck; //0x00A8 
	char _0x00AC[4];
	Vector origin; //0x00B0 
	Vector prev_origin; //0x00BC 
	Vector velocity; //0x00C8 
	char _0x00D4[12];
	Vector2D velocity_norm_2d; //0x00E0 
	char _0x00E8[4];
	float speed_2d; //0x00EC 
	float speed_up; //0x00F0 
	float speed_norm; //0x00F4 
	float feet_speed; //0x00F8 
	float feet_shit; //0x00FC 
	float t_since_started_moving; //0x0100 
	float t_since_stopped_moving; //0x0104 
	bool on_ground; //0x0108 
	bool hitgr_anim; //0x0109 
	char _0x010A[4];
	__int8 N01F4649D; //0x010E 
	BYTE N01F52468; //0x010F 
	float time_since_inair; //0x0110 
	float last_origin_z; //0x0114 
	float time_to_land; //0x0118 
	float stop_to_full_run_frac; //0x011C 
	char _0x0120[1];
	BYTE N01EE3F18; //0x0121 
	char _0x0122[2];
	float unk_frac; //0x0124 
	char _0x0128[517];
	__int8 N01EE3F9B; //0x032D 
	char _0x032E[2];
	float min_yaw; //0x0330 
	float max_yaw; //0x0334 
	char _0x0338[385];

};//Size=0x04B9

enum MoveType_t
{
	MOVETYPE_NONE = 0,
	MOVETYPE_ISOMETRIC,
	MOVETYPE_WALK,
	MOVETYPE_STEP,
	MOVETYPE_FLY,
	MOVETYPE_FLYGRAVITY,
	MOVETYPE_VPHYSICS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_LADDER,
	MOVETYPE_OBSERVER,
	MOVETYPE_CUSTOM,
	MOVETYPE_LAST = MOVETYPE_CUSTOM,
	MOVETYPE_MAX_BITS = 4
};

class CClockDriftMgr
{
public:
	float m_ClockOffsets[17];   //0x0000
	uint32_t m_iCurClockOffset; //0x0044
	uint32_t m_nServerTick;     //0x0048
	uint32_t m_nClientTick;     //0x004C
}; //Size: 0x0050

class weapon_info
{
public:
	virtual ~weapon_info() {};
	char*		consoleName;			// 0x0004
	char		pad_0008[12];			// 0x0008
	int			max_clip;				// 0x0014
	int			iMaxClip2;				// 0x0018
	int			iDefaultClip1;			// 0x001C
	int			iDefaultClip2;			// 0x0020
	char		pad_0024[8];			// 0x0024
	char*		szWorldModel;			// 0x002C
	char*		szViewModel;			// 0x0030
	char*		szDroppedModel;			// 0x0034
	char		pad_0038[4];			// 0x0038
	char*		N0000023E;				// 0x003C
	char		pad_0040[56];			// 0x0040
	char*		szEmptySound;			// 0x0078
	char		pad_007C[4];			// 0x007C
	char*		szBulletType;			// 0x0080
	char		pad_0084[4];			// 0x0084
	char*		szHudName;				// 0x0088
	char*		weapon_name;			// 0x008C
	char		_0x0090[60];			// 0x0090
	int 		type;					// 0x00C8
	int			iWeaponPrice;			// 0x00CC
	int			iKillAward;				// 0x00D0
	char*		szAnimationPrefix;		// 0x00D4
	float		flCycleTime;			// 0x00D8
	float		flCycleTimeAlt;			// 0x00DC
	float		flTimeToIdle;			// 0x00E0
	float		flIdleInterval;			// 0x00E4
	bool		bFullAuto;				// 0x00E8
	char		pad_0x00E5[3];			// 0x00E9
	int			damage;					// 0x00EC
	float		armor_ratio;			// 0x00F0
	int			iBullets;				// 0x00F4
	float		penetration;			// 0x00F8
	float		flFlinchVelocityModifierLarge;	// 0x00FC
	float		flFlinchVelocityModifierSmall;	// 0x0100
	float		range;					// 0x0104
	float		range_modifier;			// 0x0108
	float		throw_velocity;			// 0x010C
	char		pad_0x010C[12];			// 0x0110
	bool		bHasSilencer;			// 0x011C
	char		pad_0x0119[3];			// 0x011D
	char*		pSilencerModel;			// 0x0120
	int			iCrosshairMinDistance;	// 0x0124
	int			iCrosshairDeltaDistance;// 0x0128 - iTeam?
	float		max_speed;				// 0x012C
	float		max_speed_alt;			// 0x0130
	float		flSpread;				// 0x0134
	float		flSpreadAlt;			// 0x0138
	float		flInaccuracyCrouch;		// 0x013C
	float		flInaccuracyCrouchAlt;	// 0x0140
	float		flInaccuracyStand;		// 0x0144
	float		flInaccuracyStandAlt;	// 0x0148
	float		flInaccuracyJumpInitial;// 0x014C
	float		flInaccuracyJump;		// 0x0150
	float		flInaccuracyJumpAlt;	// 0x0154
	float		flInaccuracyLand;		// 0x0158
	float		flInaccuracyLandAlt;	// 0x015C
	float		flInaccuracyLadder;		// 0x0160
	float		flInaccuracyLadderAlt;	// 0x0164
	float		flInaccuracyFire;		// 0x0168
	float		flInaccuracyFireAlt;	// 0x016C
	float		flInaccuracyMove;		// 0x0170
	float		flInaccuracyMoveAlt;	// 0x0174
	float		flInaccuracyReload;		// 0x0178
	int			iRecoilSeed;			// 0x017C
	float		flRecoilAngle;			// 0x0180
	float		flRecoilAngleAlt;		// 0x0184
	float		flRecoilAngleVariance;	// 0x0188
	float		flRecoilAngleVarianceAlt;	// 0x018C
	float		flRecoilMagnitude;		// 0x0190
	float		flRecoilMagnitudeAlt;	// 0x0194
	float		flRecoilMagnitudeVariance;	// 0x0198
	float		flRecoilMagnitudeVarianceAlt;	// 0x019C
	float		flRecoveryTimeCrouch;	// 0x01A0
	float		flRecoveryTimeStand;	// 0x01A4
	float		flRecoveryTimeCrouchFinal;	// 0x01A8
	float		flRecoveryTimeStandFinal;	// 0x01AC
	int			iRecoveryTransitionStartBullet;// 0x01B0 
	int			iRecoveryTransitionEndBullet;	// 0x01B4
	bool		bUnzoomAfterShot;		// 0x01B8
	bool		bHideViewModelZoomed;	// 0x01B9
	char		pad_0x01B5[2];			// 0x01BA
	char		iZoomLevels[3];			// 0x01BC
	int			iZoomFOV[2];			// 0x01C0
	float		fZoomTime[3];			// 0x01C4
	char*		szWeaponClass;			// 0x01D4
	float		flAddonScale;			// 0x01D8
	char		pad_0x01DC[4];			// 0x01DC
	char*		szEjectBrassEffect;		// 0x01E0
	char*		szTracerEffect;			// 0x01E4
	int			iTracerFrequency;		// 0x01E8
	int			iTracerFrequencyAlt;	// 0x01EC
	char*		szMuzzleFlashEffect_1stPerson; // 0x01F0
	char		pad_0x01F4[4];			 // 0x01F4
	char*		szMuzzleFlashEffect_3rdPerson; // 0x01F8
	char		pad_0x01FC[4];			// 0x01FC
	char*		szMuzzleSmokeEffect;	// 0x0200
	float		flHeatPerShot;			// 0x0204
	char*		szZoomInSound;			// 0x0208
	char*		szZoomOutSound;			// 0x020C
	float		flInaccuracyPitchShift;	// 0x0210
	float		flInaccuracySoundThreshold;	// 0x0214
	float		flBotAudibleRange;		// 0x0218
	char		pad_0x0218[8];			// 0x0220
	char*		pWrongTeamMsg;			// 0x0224
	bool		bHasBurstMode;			// 0x0228
	char		pad_0x0225[3];			// 0x0229
	bool		bIsRevolver;			// 0x022C
	bool		bCannotShootUnderwater;	// 0x0230
};

struct datamap_t;
class typedescription_t;

enum
{
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,

	// Must be last
	TD_OFFSET_COUNT,
};

class typedescription_t
{
public:
	int32_t fieldType; //0x0000
	char* fieldName; //0x0004
	int fieldOffset[TD_OFFSET_COUNT]; //0x0008
	int16_t fieldSize_UNKNWN; //0x0010
	int16_t flags_UNKWN; //0x0012
	char pad_0014[12]; //0x0014
	datamap_t* td; //0x0020
	char pad_0024[24]; //0x0024
}; //Size: 0x003C


   //-----------------------------------------------------------------------------
   // Purpose: stores the list of objects in the hierarchy
   //            used to iterate through an object's data descriptions
   //-----------------------------------------------------------------------------
struct datamap_t
{
	typedescription_t    *dataDesc;
	int                    dataNumFields;
	char const            *dataClassName;
	datamap_t            *baseMap;

	bool                chains_validated;
	// Have the "packed" offsets been computed
	bool                packed_offsets_computed;
	int                    packed_size;
};

struct inputdata_t;
typedef enum _fieldtypes
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t *

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

							// NOTE: Use float arrays for local transformations that don't need to be fixed up.
							FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
							FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

							FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
							FIELD_MODELINDEX,		// a model index
							FIELD_MATERIALINDEX,	// a material index (using the material precache string table)

							FIELD_VECTOR2D,			// 2 floats

							FIELD_TYPECOUNT,		// MUST BE LAST
} fieldtype_t;

class IRefCounted
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
};

class IWorldRenderList : public IRefCounted
{
};

struct VisibleFogVolumeInfo_t
{
	int		m_nVisibleFogVolume;
	int		m_nVisibleFogVolumeLeaf;
	bool	m_bEyeInFogVolume;
	float	m_flDistanceToWater;
	float	m_flWaterHeight;
	IMaterial *m_pFogVolumeMaterial;
};

typedef unsigned short LeafIndex_t;
enum
{
	INVALID_LEAF_INDEX = (LeafIndex_t)~0
};

struct WorldListLeafData_t
{
	LeafIndex_t	leafIndex;	// 16 bits
	int16	waterData;
	uint16 	firstTranslucentSurface;	// engine-internal list index
	uint16	translucentSurfaceCount;	// count of translucent surfaces+disps
};

struct WorldListInfo_t
{
	int		m_ViewFogVolume;
	int		m_LeafCount;
	bool	m_bHasWater;
	WorldListLeafData_t	*m_pLeafDataList;
};

struct VisOverrideData_t
{
	Vector		m_vecVisOrigin;					// The point to to use as the viewpoint for area portal backface cull checks.
	float		m_fDistToAreaPortalTolerance;	// The distance from an area portal before using the full screen as the viewable portion.
};

struct BrushVertex_t
{
	Vector		m_Pos;
	Vector		m_Normal;
	Vector		m_TangentS;
	Vector		m_TangentT;
	Vector2D	m_TexCoord;
	Vector2D	m_LightmapCoord;

private:
	BrushVertex_t(const BrushVertex_t& src);
};

class IBrushSurface
{
public:
	// Computes texture coordinates + lightmap coordinates given a world position
	virtual void ComputeTextureCoordinate(Vector const& worldPos, Vector2D& texCoord) = 0;
	virtual void ComputeLightmapCoordinate(Vector const& worldPos, Vector2D& lightmapCoord) = 0;

	// Gets the vertex data for this surface
	virtual int  GetVertexCount() const = 0;
	virtual void GetVertexData(BrushVertex_t* pVerts) = 0;

	// Gets at the material properties for this surface
	virtual IMaterial* GetMaterial() = 0;
};

class IBrushRenderer
{
public:
	// Draws the surface; returns true if decals should be rendered on this surface
	virtual bool RenderBrushModelSurface(C_BasePlayer* pBaseEntity, IBrushSurface* pBrushSurface) = 0;
};

struct colorVec
{
	unsigned r, g, b, a;
};

class IVRenderView
{
public:

	// Draw normal brush model.
	// If pMaterialOverride is non-null, then all the faces of the bmodel will
	// set this material rather than their regular material.
	virtual void			DrawBrushModel(C_BasePlayer *baseentity, model_t *model, const Vector& origin, const QAngle& angles, bool sort) = 0;

	// Draw brush model that has no origin/angles change ( uses identity transform )
	// FIXME, Material proxy IClientEntity *baseentity is unused right now, use DrawBrushModel for brushes with
	//  proxies for now.
	virtual void			DrawIdentityBrushModel(IWorldRenderList *pList, model_t *model) = 0;

	// Mark this dynamic light as having changed this frame ( so light maps affected will be recomputed )
	virtual void			TouchLight(struct dlight_t *light) = 0;
	// Draw 3D Overlays
	virtual void			Draw3DDebugOverlays(void) = 0;
	// Sets global blending fraction
	virtual void			SetBlend(float blend) = 0;
	virtual float			GetBlend(void) = 0;

	// Sets global color modulation
	virtual void			SetColorModulation(float const* blend) = 0;
	virtual void			GetColorModulation(float* blend) = 0;

	// Wrap entire scene drawing
	virtual void			SceneBegin(void) = 0;
	virtual void			SceneEnd(void) = 0;

	// Gets the fog volume for a particular point
	virtual void			GetVisibleFogVolume(const Vector& eyePoint, VisibleFogVolumeInfo_t *pInfo) = 0;

	// Wraps world drawing
	// If iForceViewLeaf is not -1, then it uses the specified leaf as your starting area for setting up area portal culling.
	// This is used by water since your reflected view origin is often in solid space, but we still want to treat it as though
	// the first portal we're looking out of is a water portal, so our view effectively originates under the water.
	virtual IWorldRenderList * CreateWorldList() = 0;

	virtual void			BuildWorldLists(IWorldRenderList *pList, WorldListInfo_t* pInfo, int iForceFViewLeaf, const VisOverrideData_t* pVisData = NULL, bool bShadowDepth = false, float *pReflectionWaterHeight = NULL) = 0;
	virtual void			DrawWorldLists(IWorldRenderList *pList, unsigned long flags, float waterZAdjust) = 0;
	virtual int				GetNumIndicesForWorldLists(IWorldRenderList *pList, unsigned long nFlags) = 0;

	// Optimization for top view
	virtual void			DrawTopView(bool enable) = 0;
	virtual void			TopViewBounds(Vector2D const& mins, Vector2D const& maxs) = 0;

	// Draw lights
	virtual void			DrawLights(void) = 0;
	// FIXME:  This function is a stub, doesn't do anything in the engine right now
	virtual void			DrawMaskEntities(void) = 0;

	// Draw surfaces with alpha, don't call in shadow depth pass
	virtual void			DrawTranslucentSurfaces(IWorldRenderList *pList, int *pSortList, int sortCount, unsigned long flags) = 0;

	// Draw Particles ( just draws the linefine for debugging map leaks )
	virtual void			DrawLineFile(void) = 0;
	// Draw lightmaps
	virtual void			DrawLightmaps(IWorldRenderList *pList, int pageId) = 0;
	// Wraps view render sequence, sets up a view
	virtual void			ViewSetupVis(bool novis, int numorigins, const Vector origin[]) = 0;

	// Return true if any of these leaves are visible in the current PVS.
	virtual bool			AreAnyLeavesVisible(int *leafList, int nLeaves) = 0;

	virtual	void			VguiPaint(void) = 0;
	// Sets up view fade parameters
	virtual void			ViewDrawFade(byte *color, IMaterial *pMaterial) = 0;
	// Sets up the projection matrix for the specified field of view
	virtual void			OLD_SetProjectionMatrix(float fov, float zNear, float zFar) = 0;
	// Determine lighting at specified position
	virtual colorVec		GetLightAtPoint(Vector& pos) = 0;
	// Whose eyes are we looking through?
	virtual int				GetViewEntity(void) = 0;
	virtual bool			IsViewEntity(int entindex) = 0;
	// Get engine field of view setting
	virtual float			GetFieldOfView(void) = 0;
	// 1 == ducking, 0 == not
	virtual unsigned char	**GetAreaBits(void) = 0;

	// Set up fog for a particular leaf
	virtual void			SetFogVolumeState(int nVisibleFogVolume, bool bUseHeightFog) = 0;

	// Installs a brush surface draw override method, null means use normal renderer
	virtual void			InstallBrushSurfaceRenderer(IBrushRenderer* pBrushRenderer) = 0;

	// Draw brush model shadow
	virtual void			DrawBrushModelShadow(IClientRenderable *pRenderable) = 0;

	// Does the leaf contain translucent surfaces?
	virtual	bool			LeafContainsTranslucentSurfaces(IWorldRenderList *pList, int sortIndex, unsigned long flags) = 0;

	virtual bool			DoesBoxIntersectWaterVolume(const Vector &mins, const Vector &maxs, int leafWaterDataID) = 0;

	virtual void			SetAreaState(unsigned char chAreaBits[MAX_AREA_STATE_BYTES], unsigned char chAreaPortalBits[MAX_AREA_PORTAL_STATE_BYTES]) = 0;

	// See i
	virtual void			VGui_Paint(int mode) = 0;
};

struct model_t
{
	char        name[255];
};
typedef unsigned short ModelInstanceHandle_t;


struct ModelRenderInfo_t
{
	Vector origin;
	QAngle angles;
	char pad[0x4];
	void *pRenderable;
	const model_t *pModel;
	const matrix3x4_t *pModelToWorld;
	const matrix3x4_t *pLightingOffset;
	const Vector *pLightingOrigin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	ModelInstanceHandle_t instance;
};

class IVModelRender
{
public:

	void ForcedMaterialOverride(IMaterial *mat)
	{
		if (!this)
			return;

		typedef void(__thiscall *OrigFn)(void *, IMaterial *, int, int);
		Memory::VCall<OrigFn>(this, 1)(this, mat, 0, 0);
	}

	bool IsForcedMaterialOverride()
	{
		if (!this)
			return 0;

		typedef bool(__thiscall *OrigFn)(void *);
		return Memory::VCall<OrigFn>(this, 2)(this);
	}

	int DrawModel(int flags,
		IClientRenderable *pRenderable,
		ModelInstanceHandle_t instance,
		int entity_index,
		const model_t *model,
		Vector const& origin,
		QAngle const& angles,
		int skin,
		int body,
		int hitboxset,
		const matrix3x4_t *modelToWorld = NULL,
		const matrix3x4_t *pLightingOffset = NULL)
	{
		if (!this)
			return 0 ;

		typedef int(__thiscall *OrigFn)(void*, int flags,
			IClientRenderable *pRenderable,
			ModelInstanceHandle_t instance,
			int entity_index,
			const model_t *model,
			Vector const& origin,
			QAngle const& angles,
			int skin,
			int body,
			int hitboxset,
			const matrix3x4_t *modelToWorld,
			const matrix3x4_t *pLightingOffset);

		return Memory::VCall<OrigFn>(this, 2)(this, flags, pRenderable, instance, entity_index, model, origin, angles, skin, body, hitboxset, modelToWorld, pLightingOffset);
	}

	int DrawModelEx(ModelRenderInfo_t &pInfo)
	{
		if (!this)
			return 0;

		typedef int(__thiscall *OrigFn)(void*, ModelRenderInfo_t &pInfo);

		return Memory::VCall<OrigFn>(this, 18)(this, pInfo);
	}

	int DrawModelExecute(void* ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
	{
		if (!this)
			return 0;

		typedef int(__thiscall* OriginalFn)(void*, void* ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld);
		return Memory::VCall<OriginalFn>(this, 21)(this, ctx, state, pInfo, pCustomBoneToWorld);
	}
};

typedef unsigned short ModelInstanceHandle_t;

enum
{
	MODEL_INSTANCE_INVALID = (ModelInstanceHandle_t)~0
};

struct GlowObjectDefinition_t
{
	GlowObjectDefinition_t() { memset(this, 0, sizeof(*this)); }

	C_BasePlayer* m_pEntity;    //0x0000
	union
	{
		Vector m_vGlowColor;           //0x0004
		struct
		{
			float   m_flRed;           //0x0004
			float   m_flGreen;         //0x0008
			float   m_flBlue;          //0x000C
		};
	};
	float   m_flAlpha;                 //0x0010
	uint8_t pad_0014[4];               //0x0014
	float   m_flSomeFloat;             //0x0018
	uint8_t pad_001C[4];               //0x001C
	float   m_flAnotherFloat;          //0x0020
	bool    m_bRenderWhenOccluded;     //0x0024
	bool    m_bRenderWhenUnoccluded;   //0x0025
	bool    m_bFullBloomRender;        //0x0026
	uint8_t pad_0027[5];               //0x0027
	int32_t m_nGlowStyle;              //0x002C
	int32_t m_nSplitScreenSlot;        //0x0030
	int32_t m_nNextFreeSlot;           //0x0034

	bool IsUnused() const { return m_nNextFreeSlot != ENTRY_IN_USE; }
}; //Size: 0x0038 (56)

enum activity: int
{
	ACT_RESET,
	ACT_IDLE,
	ACT_TRANSITION,
	ACT_COVER,
	ACT_COVER_MED,
	ACT_COVER_LOW,
	ACT_WALK,
	ACT_WALK_AIM,
	ACT_WALK_CROUCH,
	ACT_WALK_CROUCH_AIM,
	ACT_RUN,
	ACT_RUN_AIM,
	ACT_RUN_CROUCH,
	ACT_RUN_CROUCH_AIM,
	ACT_RUN_PROTECTED,
	ACT_SCRIPT_CUSTOM_MOVE,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_RANGE_ATTACK1_LOW,
	ACT_RANGE_ATTACK2_LOW,
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_DIERAGDOLL,
	ACT_FLY,
	ACT_HOVER,
	ACT_GLIDE,
	ACT_SWIM,
	ACT_JUMP,
	ACT_HOP,
	ACT_LEAP,
	ACT_LAND,
	ACT_CLIMB_UP,
	ACT_CLIMB_DOWN,
	ACT_CLIMB_DISMOUNT,
	ACT_SHIPLADDER_UP,
	ACT_SHIPLADDER_DOWN,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,
	ACT_ROLL_RIGHT,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_CROUCH,
	ACT_CROUCHIDLE,
	ACT_STAND,
	ACT_USE,
	ACT_ALIEN_BURROW_IDLE,
	ACT_ALIEN_BURROW_OUT,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_SIGNAL_ADVANCE,
	ACT_SIGNAL_FORWARD,
	ACT_SIGNAL_GROUP,
	ACT_SIGNAL_HALT,
	ACT_SIGNAL_LEFT,
	ACT_SIGNAL_RIGHT,
	ACT_SIGNAL_TAKECOVER,
	ACT_LOOKBACK_RIGHT,
	ACT_LOOKBACK_LEFT,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_RELOAD_START,
	ACT_RELOAD_FINISH,
	ACT_RELOAD_LOW,
	ACT_ARM,
	ACT_DISARM,
	ACT_DROP_WEAPON,
	ACT_DROP_WEAPON_SHOTGUN,
	ACT_PICKUP_GROUND,
	ACT_PICKUP_RACK,
	ACT_IDLE_ANGRY,
	ACT_IDLE_RELAXED,
	ACT_IDLE_STIMULATED,
	ACT_IDLE_AGITATED,
	ACT_IDLE_STEALTH,
	ACT_IDLE_HURT,
	ACT_WALK_RELAXED,
	ACT_WALK_STIMULATED,
	ACT_WALK_AGITATED,
	ACT_WALK_STEALTH,
	ACT_RUN_RELAXED,
	ACT_RUN_STIMULATED,
	ACT_RUN_AGITATED,
	ACT_RUN_STEALTH,
	ACT_IDLE_AIM_RELAXED,
	ACT_IDLE_AIM_STIMULATED,
	ACT_IDLE_AIM_AGITATED,
	ACT_IDLE_AIM_STEALTH,
	ACT_WALK_AIM_RELAXED,
	ACT_WALK_AIM_STIMULATED,
	ACT_WALK_AIM_AGITATED,
	ACT_WALK_AIM_STEALTH,
	ACT_RUN_AIM_RELAXED,
	ACT_RUN_AIM_STIMULATED,
	ACT_RUN_AIM_AGITATED,
	ACT_RUN_AIM_STEALTH,
	ACT_CROUCHIDLE_STIMULATED,
	ACT_CROUCHIDLE_AIM_STIMULATED,
	ACT_CROUCHIDLE_AGITATED,
	ACT_WALK_HURT,
	ACT_RUN_HURT,
	ACT_SPECIAL_ATTACK1,
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,
	ACT_DIE_HEADSHOT,
	ACT_DIE_CHESTSHOT,
	ACT_DIE_GUTSHOT,
	ACT_DIE_BACKSHOT,
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
	ACT_FLINCH_PHYSICS,
	ACT_FLINCH_HEAD_BACK,
	ACT_FLINCH_HEAD_LEFT,
	ACT_FLINCH_HEAD_RIGHT,
	ACT_FLINCH_CHEST_BACK,
	ACT_FLINCH_STOMACH_BACK,
	ACT_FLINCH_CROUCH_FRONT,
	ACT_FLINCH_CROUCH_BACK,
	ACT_FLINCH_CROUCH_LEFT,
	ACT_FLINCH_CROUCH_RIGHT,
	ACT_IDLE_ON_FIRE,
	ACT_WALK_ON_FIRE,
	ACT_RUN_ON_FIRE,
	ACT_RAPPEL_LOOP,
	ACT_180_LEFT,
	ACT_180_RIGHT,
	ACT_90_LEFT,
	ACT_90_RIGHT,
	ACT_STEP_LEFT,
	ACT_STEP_RIGHT,
	ACT_STEP_BACK,
	ACT_STEP_FORE,
	ACT_GESTURE_RANGE_ATTACK1,
	ACT_GESTURE_RANGE_ATTACK2,
	ACT_GESTURE_MELEE_ATTACK1,
	ACT_GESTURE_MELEE_ATTACK2,
	ACT_GESTURE_RANGE_ATTACK1_LOW,
	ACT_GESTURE_RANGE_ATTACK2_LOW,
	ACT_MELEE_ATTACK_SWING_GESTURE,
	ACT_GESTURE_SMALL_FLINCH,
	ACT_GESTURE_BIG_FLINCH,
	ACT_GESTURE_FLINCH_BLAST,
	ACT_GESTURE_FLINCH_BLAST_SHOTGUN,
	ACT_GESTURE_FLINCH_BLAST_DAMAGED,
	ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN,
	ACT_GESTURE_FLINCH_HEAD,
	ACT_GESTURE_FLINCH_CHEST,
	ACT_GESTURE_FLINCH_STOMACH,
	ACT_GESTURE_FLINCH_LEFTARM,
	ACT_GESTURE_FLINCH_RIGHTARM,
	ACT_GESTURE_FLINCH_LEFTLEG,
	ACT_GESTURE_FLINCH_RIGHTLEG,
	ACT_GESTURE_TURN_LEFT,
	ACT_GESTURE_TURN_RIGHT,
	ACT_GESTURE_TURN_LEFT45,
	ACT_GESTURE_TURN_RIGHT45,
	ACT_GESTURE_TURN_LEFT90,
	ACT_GESTURE_TURN_RIGHT90,
	ACT_GESTURE_TURN_LEFT45_FLAT,
	ACT_GESTURE_TURN_RIGHT45_FLAT,
	ACT_GESTURE_TURN_LEFT90_FLAT,
	ACT_GESTURE_TURN_RIGHT90_FLAT,
	ACT_BARNACLE_HIT,
	ACT_BARNACLE_PULL,
	ACT_BARNACLE_CHOMP,
	ACT_BARNACLE_CHEW,
	ACT_DO_NOT_DISTURB,
	ACT_SPECIFIC_SEQUENCE,
	ACT_VM_DRAW,
	ACT_VM_HOLSTER,
	ACT_VM_IDLE,
	ACT_VM_FIDGET,
	ACT_VM_PULLBACK,
	ACT_VM_PULLBACK_HIGH,
	ACT_VM_PULLBACK_LOW,
	ACT_VM_THROW,
	ACT_VM_PULLPIN,
	ACT_VM_PRIMARYATTACK,
	ACT_VM_SECONDARYATTACK,
	ACT_VM_RELOAD,
	ACT_VM_DRYFIRE,
	ACT_VM_HITLEFT,
	ACT_VM_HITLEFT2,
	ACT_VM_HITRIGHT,
	ACT_VM_HITRIGHT2,
	ACT_VM_HITCENTER,
	ACT_VM_HITCENTER2,
	ACT_VM_MISSLEFT,
	ACT_VM_MISSLEFT2,
	ACT_VM_MISSRIGHT,
	ACT_VM_MISSRIGHT2,
	ACT_VM_MISSCENTER,
	ACT_VM_MISSCENTER2,
	ACT_VM_HAULBACK,
	ACT_VM_SWINGHARD,
	ACT_VM_SWINGMISS,
	ACT_VM_SWINGHIT,
	ACT_VM_IDLE_TO_LOWERED,
	ACT_VM_IDLE_LOWERED,
	ACT_VM_LOWERED_TO_IDLE,
	ACT_VM_RECOIL1,
	ACT_VM_RECOIL2,
	ACT_VM_RECOIL3,
	ACT_VM_PICKUP,
	ACT_VM_RELEASE,
	ACT_VM_ATTACH_SILENCER,
	ACT_VM_DETACH_SILENCER,
	ACT_VM_EMPTY_FIRE,
	ACT_VM_EMPTY_RELOAD,
	ACT_VM_EMPTY_DRAW,
	ACT_VM_EMPTY_IDLE,
	ACT_SLAM_STICKWALL_IDLE,
	ACT_SLAM_STICKWALL_ND_IDLE,
	ACT_SLAM_STICKWALL_ATTACH,
	ACT_SLAM_STICKWALL_ATTACH2,
	ACT_SLAM_STICKWALL_ND_ATTACH,
	ACT_SLAM_STICKWALL_ND_ATTACH2,
	ACT_SLAM_STICKWALL_DETONATE,
	ACT_SLAM_STICKWALL_DETONATOR_HOLSTER,
	ACT_SLAM_STICKWALL_DRAW,
	ACT_SLAM_STICKWALL_ND_DRAW,
	ACT_SLAM_STICKWALL_TO_THROW,
	ACT_SLAM_STICKWALL_TO_THROW_ND,
	ACT_SLAM_STICKWALL_TO_TRIPMINE_ND,
	ACT_SLAM_THROW_IDLE,
	ACT_SLAM_THROW_ND_IDLE,
	ACT_SLAM_THROW_THROW,
	ACT_SLAM_THROW_THROW2,
	ACT_SLAM_THROW_THROW_ND,
	ACT_SLAM_THROW_THROW_ND2,
	ACT_SLAM_THROW_DRAW,
	ACT_SLAM_THROW_ND_DRAW,
	ACT_SLAM_THROW_TO_STICKWALL,
	ACT_SLAM_THROW_TO_STICKWALL_ND,
	ACT_SLAM_THROW_DETONATE,
	ACT_SLAM_THROW_DETONATOR_HOLSTER,
	ACT_SLAM_THROW_TO_TRIPMINE_ND,
	ACT_SLAM_TRIPMINE_IDLE,
	ACT_SLAM_TRIPMINE_DRAW,
	ACT_SLAM_TRIPMINE_ATTACH,
	ACT_SLAM_TRIPMINE_ATTACH2,
	ACT_SLAM_TRIPMINE_TO_STICKWALL_ND,
	ACT_SLAM_TRIPMINE_TO_THROW_ND,
	ACT_SLAM_DETONATOR_IDLE,
	ACT_SLAM_DETONATOR_DRAW,
	ACT_SLAM_DETONATOR_DETONATE,
	ACT_SLAM_DETONATOR_HOLSTER,
	ACT_SLAM_DETONATOR_STICKWALL_DRAW,
	ACT_SLAM_DETONATOR_THROW_DRAW,
	ACT_SHOTGUN_RELOAD_START,
	ACT_SHOTGUN_RELOAD_FINISH,
	ACT_SHOTGUN_PUMP,
	ACT_SMG2_IDLE2,
	ACT_SMG2_FIRE2,
	ACT_SMG2_DRAW2,
	ACT_SMG2_RELOAD2,
	ACT_SMG2_DRYFIRE2,
	ACT_SMG2_TOAUTO,
	ACT_SMG2_TOBURST,
	ACT_PHYSCANNON_UPGRADE,
	ACT_RANGE_ATTACK_AR1,
	ACT_RANGE_ATTACK_AR2,
	ACT_RANGE_ATTACK_AR2_LOW,
	ACT_RANGE_ATTACK_AR2_GRENADE,
	ACT_RANGE_ATTACK_HMG1,
	ACT_RANGE_ATTACK_ML,
	ACT_RANGE_ATTACK_SMG1,
	ACT_RANGE_ATTACK_SMG1_LOW,
	ACT_RANGE_ATTACK_SMG2,
	ACT_RANGE_ATTACK_SHOTGUN,
	ACT_RANGE_ATTACK_SHOTGUN_LOW,
	ACT_RANGE_ATTACK_PISTOL,
	ACT_RANGE_ATTACK_PISTOL_LOW,
	ACT_RANGE_ATTACK_SLAM,
	ACT_RANGE_ATTACK_TRIPWIRE,
	ACT_RANGE_ATTACK_THROW,
	ACT_RANGE_ATTACK_SNIPER_RIFLE,
	ACT_RANGE_ATTACK_RPG,
	ACT_MELEE_ATTACK_SWING,
	ACT_RANGE_AIM_LOW,
	ACT_RANGE_AIM_SMG1_LOW,
	ACT_RANGE_AIM_PISTOL_LOW,
	ACT_RANGE_AIM_AR2_LOW,
	ACT_COVER_PISTOL_LOW,
	ACT_COVER_SMG1_LOW,
	ACT_GESTURE_RANGE_ATTACK_AR1,
	ACT_GESTURE_RANGE_ATTACK_AR2,
	ACT_GESTURE_RANGE_ATTACK_AR2_GRENADE,
	ACT_GESTURE_RANGE_ATTACK_HMG1,
	ACT_GESTURE_RANGE_ATTACK_ML,
	ACT_GESTURE_RANGE_ATTACK_SMG1,
	ACT_GESTURE_RANGE_ATTACK_SMG1_LOW,
	ACT_GESTURE_RANGE_ATTACK_SMG2,
	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,
	ACT_GESTURE_RANGE_ATTACK_PISTOL,
	ACT_GESTURE_RANGE_ATTACK_PISTOL_LOW,
	ACT_GESTURE_RANGE_ATTACK_SLAM,
	ACT_GESTURE_RANGE_ATTACK_TRIPWIRE,
	ACT_GESTURE_RANGE_ATTACK_THROW,
	ACT_GESTURE_RANGE_ATTACK_SNIPER_RIFLE,
	ACT_GESTURE_MELEE_ATTACK_SWING,
	ACT_IDLE_RIFLE,
	ACT_IDLE_SMG1,
	ACT_IDLE_ANGRY_SMG1,
	ACT_IDLE_PISTOL,
	ACT_IDLE_ANGRY_PISTOL,
	ACT_IDLE_ANGRY_SHOTGUN,
	ACT_IDLE_STEALTH_PISTOL,
	ACT_IDLE_PACKAGE,
	ACT_WALK_PACKAGE,
	ACT_IDLE_SUITCASE,
	ACT_WALK_SUITCASE,
	ACT_IDLE_SMG1_RELAXED,
	ACT_IDLE_SMG1_STIMULATED,
	ACT_WALK_RIFLE_RELAXED,
	ACT_RUN_RIFLE_RELAXED,
	ACT_WALK_RIFLE_STIMULATED,
	ACT_RUN_RIFLE_STIMULATED,
	ACT_IDLE_AIM_RIFLE_STIMULATED,
	ACT_WALK_AIM_RIFLE_STIMULATED,
	ACT_RUN_AIM_RIFLE_STIMULATED,
	ACT_IDLE_SHOTGUN_RELAXED,
	ACT_IDLE_SHOTGUN_STIMULATED,
	ACT_IDLE_SHOTGUN_AGITATED,
	ACT_WALK_ANGRY,
	ACT_POLICE_HARASS1,
	ACT_POLICE_HARASS2,
	ACT_IDLE_MANNEDGUN,
	ACT_IDLE_MELEE,
	ACT_IDLE_ANGRY_MELEE,
	ACT_IDLE_RPG_RELAXED,
	ACT_IDLE_RPG,
	ACT_IDLE_ANGRY_RPG,
	ACT_COVER_LOW_RPG,
	ACT_WALK_RPG,
	ACT_RUN_RPG,
	ACT_WALK_CROUCH_RPG,
	ACT_RUN_CROUCH_RPG,
	ACT_WALK_RPG_RELAXED,
	ACT_RUN_RPG_RELAXED,
	ACT_WALK_RIFLE,
	ACT_WALK_AIM_RIFLE,
	ACT_WALK_CROUCH_RIFLE,
	ACT_WALK_CROUCH_AIM_RIFLE,
	ACT_RUN_RIFLE,
	ACT_RUN_AIM_RIFLE,
	ACT_RUN_CROUCH_RIFLE,
	ACT_RUN_CROUCH_AIM_RIFLE,
	ACT_RUN_STEALTH_PISTOL,
	ACT_WALK_AIM_SHOTGUN,
	ACT_RUN_AIM_SHOTGUN,
	ACT_WALK_PISTOL,
	ACT_RUN_PISTOL,
	ACT_WALK_AIM_PISTOL,
	ACT_RUN_AIM_PISTOL,
	ACT_WALK_STEALTH_PISTOL,
	ACT_WALK_AIM_STEALTH_PISTOL,
	ACT_RUN_AIM_STEALTH_PISTOL,
	ACT_RELOAD_PISTOL,
	ACT_RELOAD_PISTOL_LOW,
	ACT_RELOAD_SMG1,
	ACT_RELOAD_SMG1_LOW,
	ACT_RELOAD_SHOTGUN,
	ACT_RELOAD_SHOTGUN_LOW,
	ACT_GESTURE_RELOAD,
	ACT_GESTURE_RELOAD_PISTOL,
	ACT_GESTURE_RELOAD_SMG1,
	ACT_GESTURE_RELOAD_SHOTGUN,
	ACT_BUSY_LEAN_LEFT,
	ACT_BUSY_LEAN_LEFT_ENTRY,
	ACT_BUSY_LEAN_LEFT_EXIT,
	ACT_BUSY_LEAN_BACK,
	ACT_BUSY_LEAN_BACK_ENTRY,
	ACT_BUSY_LEAN_BACK_EXIT,
	ACT_BUSY_SIT_GROUND,
	ACT_BUSY_SIT_GROUND_ENTRY,
	ACT_BUSY_SIT_GROUND_EXIT,
	ACT_BUSY_SIT_CHAIR,
	ACT_BUSY_SIT_CHAIR_ENTRY,
	ACT_BUSY_SIT_CHAIR_EXIT,
	ACT_BUSY_STAND,
	ACT_BUSY_QUEUE,
	ACT_DUCK_DODGE,
	ACT_DIE_BARNACLE_SWALLOW,
	ACT_GESTURE_BARNACLE_STRANGLE,
	ACT_PHYSCANNON_DETACH,
	ACT_PHYSCANNON_ANIMATE,
	ACT_PHYSCANNON_ANIMATE_PRE,
	ACT_PHYSCANNON_ANIMATE_POST,
	ACT_DIE_FRONTSIDE,
	ACT_DIE_RIGHTSIDE,
	ACT_DIE_BACKSIDE,
	ACT_DIE_LEFTSIDE,
	ACT_DIE_CROUCH_FRONTSIDE,
	ACT_DIE_CROUCH_RIGHTSIDE,
	ACT_DIE_CROUCH_BACKSIDE,
	ACT_DIE_CROUCH_LEFTSIDE,
	ACT_OPEN_DOOR,
	ACT_DI_ALYX_ZOMBIE_MELEE,
	ACT_DI_ALYX_ZOMBIE_TORSO_MELEE,
	ACT_DI_ALYX_HEADCRAB_MELEE,
	ACT_DI_ALYX_ANTLION,
	ACT_DI_ALYX_ZOMBIE_SHOTGUN64,
	ACT_DI_ALYX_ZOMBIE_SHOTGUN26,
	ACT_READINESS_RELAXED_TO_STIMULATED,
	ACT_READINESS_RELAXED_TO_STIMULATED_WALK,
	ACT_READINESS_AGITATED_TO_STIMULATED,
	ACT_READINESS_STIMULATED_TO_RELAXED,
	ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED,
	ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK,
	ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED,
	ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED,
	ACT_IDLE_CARRY,
	ACT_WALK_CARRY,
	ACT_STARTDYING,
	ACT_DYINGLOOP,
	ACT_DYINGTODEAD,
	ACT_RIDE_MANNED_GUN,
	ACT_VM_SPRINT_ENTER,
	ACT_VM_SPRINT_IDLE,
	ACT_VM_SPRINT_LEAVE,
	ACT_FIRE_START,
	ACT_FIRE_LOOP,
	ACT_FIRE_END,
	ACT_CROUCHING_GRENADEIDLE,
	ACT_CROUCHING_GRENADEREADY,
	ACT_CROUCHING_PRIMARYATTACK,
	ACT_OVERLAY_GRENADEIDLE,
	ACT_OVERLAY_GRENADEREADY,
	ACT_OVERLAY_PRIMARYATTACK,
	ACT_OVERLAY_SHIELD_UP,
	ACT_OVERLAY_SHIELD_DOWN,
	ACT_OVERLAY_SHIELD_UP_IDLE,
	ACT_OVERLAY_SHIELD_ATTACK,
	ACT_OVERLAY_SHIELD_KNOCKBACK,
	ACT_SHIELD_UP,
	ACT_SHIELD_DOWN,
	ACT_SHIELD_UP_IDLE,
	ACT_SHIELD_ATTACK,
	ACT_SHIELD_KNOCKBACK,
	ACT_CROUCHING_SHIELD_UP,
	ACT_CROUCHING_SHIELD_DOWN,
	ACT_CROUCHING_SHIELD_UP_IDLE,
	ACT_CROUCHING_SHIELD_ATTACK,
	ACT_CROUCHING_SHIELD_KNOCKBACK,
	ACT_TURNRIGHT45,
	ACT_TURNLEFT45,
	ACT_TURN,
	ACT_OBJ_ASSEMBLING,
	ACT_OBJ_DISMANTLING,
	ACT_OBJ_STARTUP,
	ACT_OBJ_RUNNING,
	ACT_OBJ_IDLE,
	ACT_OBJ_PLACING,
	ACT_OBJ_DETERIORATING,
	ACT_OBJ_UPGRADING,
	ACT_DEPLOY,
	ACT_DEPLOY_IDLE,
	ACT_UNDEPLOY,
	ACT_CROSSBOW_DRAW_UNLOADED,
	ACT_GAUSS_SPINUP,
	ACT_GAUSS_SPINCYCLE,
	ACT_VM_PRIMARYATTACK_SILENCED,
	ACT_VM_RELOAD_SILENCED,
	ACT_VM_DRYFIRE_SILENCED,
	ACT_VM_IDLE_SILENCED,
	ACT_VM_DRAW_SILENCED,
	ACT_VM_IDLE_EMPTY_LEFT,
	ACT_VM_DRYFIRE_LEFT,
	ACT_VM_IS_DRAW,
	ACT_VM_IS_HOLSTER,
	ACT_VM_IS_IDLE,
	ACT_VM_IS_PRIMARYATTACK,
	ACT_PLAYER_IDLE_FIRE,
	ACT_PLAYER_CROUCH_FIRE,
	ACT_PLAYER_CROUCH_WALK_FIRE,
	ACT_PLAYER_WALK_FIRE,
	ACT_PLAYER_RUN_FIRE,
	ACT_IDLETORUN,
	ACT_RUNTOIDLE,
	ACT_VM_DRAW_DEPLOYED,
	ACT_HL2MP_IDLE_MELEE,
	ACT_HL2MP_RUN_MELEE,
	ACT_HL2MP_IDLE_CROUCH_MELEE,
	ACT_HL2MP_WALK_CROUCH_MELEE,
	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,
	ACT_HL2MP_GESTURE_RELOAD_MELEE,
	ACT_HL2MP_JUMP_MELEE,
	ACT_VM_FIZZLE,
	ACT_MP_STAND_IDLE,
	ACT_MP_CROUCH_IDLE,
	ACT_MP_CROUCH_DEPLOYED_IDLE,
	ACT_MP_CROUCH_DEPLOYED,
	ACT_MP_DEPLOYED_IDLE,
	ACT_MP_RUN,
	ACT_MP_WALK,
	ACT_MP_AIRWALK,
	ACT_MP_CROUCHWALK,
	ACT_MP_SPRINT,
	ACT_MP_JUMP,
	ACT_MP_JUMP_START,
	ACT_MP_JUMP_FLOAT,
	ACT_MP_JUMP_LAND,
	ACT_MP_JUMP_IMPACT_N,
	ACT_MP_JUMP_IMPACT_E,
	ACT_MP_JUMP_IMPACT_W,
	ACT_MP_JUMP_IMPACT_S,
	ACT_MP_JUMP_IMPACT_TOP,
	ACT_MP_DOUBLEJUMP,
	ACT_MP_SWIM,
	ACT_MP_DEPLOYED,
	ACT_MP_SWIM_DEPLOYED,
	ACT_MP_VCD,
	ACT_MP_ATTACK_STAND_PRIMARYFIRE,
	ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED,
	ACT_MP_ATTACK_STAND_SECONDARYFIRE,
	ACT_MP_ATTACK_STAND_GRENADE,
	ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,
	ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED,
	ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,
	ACT_MP_ATTACK_CROUCH_GRENADE,
	ACT_MP_ATTACK_SWIM_PRIMARYFIRE,
	ACT_MP_ATTACK_SWIM_SECONDARYFIRE,
	ACT_MP_ATTACK_SWIM_GRENADE,
	ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,
	ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,
	ACT_MP_ATTACK_AIRWALK_GRENADE,
	ACT_MP_RELOAD_STAND,
	ACT_MP_RELOAD_STAND_LOOP,
	ACT_MP_RELOAD_STAND_END,
	ACT_MP_RELOAD_CROUCH,
	ACT_MP_RELOAD_CROUCH_LOOP,
	ACT_MP_RELOAD_CROUCH_END,
	ACT_MP_RELOAD_SWIM,
	ACT_MP_RELOAD_SWIM_LOOP,
	ACT_MP_RELOAD_SWIM_END,
	ACT_MP_RELOAD_AIRWALK,
	ACT_MP_RELOAD_AIRWALK_LOOP,
	ACT_MP_RELOAD_AIRWALK_END,
	ACT_MP_ATTACK_STAND_PREFIRE,
	ACT_MP_ATTACK_STAND_POSTFIRE,
	ACT_MP_ATTACK_STAND_STARTFIRE,
	ACT_MP_ATTACK_CROUCH_PREFIRE,
	ACT_MP_ATTACK_CROUCH_POSTFIRE,
	ACT_MP_ATTACK_SWIM_PREFIRE,
	ACT_MP_ATTACK_SWIM_POSTFIRE,
	ACT_MP_STAND_PRIMARY,
	ACT_MP_CROUCH_PRIMARY,
	ACT_MP_RUN_PRIMARY,
	ACT_MP_WALK_PRIMARY,
	ACT_MP_AIRWALK_PRIMARY,
	ACT_MP_CROUCHWALK_PRIMARY,
	ACT_MP_JUMP_PRIMARY,
	ACT_MP_JUMP_START_PRIMARY,
	ACT_MP_JUMP_FLOAT_PRIMARY,
	ACT_MP_JUMP_LAND_PRIMARY,
	ACT_MP_SWIM_PRIMARY,
	ACT_MP_DEPLOYED_PRIMARY,
	ACT_MP_SWIM_DEPLOYED_PRIMARY,
	ACT_MP_ATTACK_STAND_PRIMARY,
	ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED,
	ACT_MP_ATTACK_CROUCH_PRIMARY,
	ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,
	ACT_MP_ATTACK_SWIM_PRIMARY,
	ACT_MP_ATTACK_AIRWALK_PRIMARY,
	ACT_MP_RELOAD_STAND_PRIMARY,
	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,
	ACT_MP_RELOAD_STAND_PRIMARY_END,
	ACT_MP_RELOAD_CROUCH_PRIMARY,
	ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,
	ACT_MP_RELOAD_CROUCH_PRIMARY_END,
	ACT_MP_RELOAD_SWIM_PRIMARY,
	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,
	ACT_MP_RELOAD_SWIM_PRIMARY_END,
	ACT_MP_RELOAD_AIRWALK_PRIMARY,
	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,
	ACT_MP_RELOAD_AIRWALK_PRIMARY_END,
	ACT_MP_ATTACK_STAND_GRENADE_PRIMARY,
	ACT_MP_ATTACK_CROUCH_GRENADE_PRIMARY,
	ACT_MP_ATTACK_SWIM_GRENADE_PRIMARY,
	ACT_MP_ATTACK_AIRWALK_GRENADE_PRIMARY,
	ACT_MP_STAND_SECONDARY,
	ACT_MP_CROUCH_SECONDARY,
	ACT_MP_RUN_SECONDARY,
	ACT_MP_WALK_SECONDARY,
	ACT_MP_AIRWALK_SECONDARY,
	ACT_MP_CROUCHWALK_SECONDARY,
	ACT_MP_JUMP_SECONDARY,
	ACT_MP_JUMP_START_SECONDARY,
	ACT_MP_JUMP_FLOAT_SECONDARY,
	ACT_MP_JUMP_LAND_SECONDARY,
	ACT_MP_SWIM_SECONDARY,
	ACT_MP_ATTACK_STAND_SECONDARY,
	ACT_MP_ATTACK_CROUCH_SECONDARY,
	ACT_MP_ATTACK_SWIM_SECONDARY,
	ACT_MP_ATTACK_AIRWALK_SECONDARY,
	ACT_MP_RELOAD_STAND_SECONDARY,
	ACT_MP_RELOAD_STAND_SECONDARY_LOOP,
	ACT_MP_RELOAD_STAND_SECONDARY_END,
	ACT_MP_RELOAD_CROUCH_SECONDARY,
	ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,
	ACT_MP_RELOAD_CROUCH_SECONDARY_END,
	ACT_MP_RELOAD_SWIM_SECONDARY,
	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,
	ACT_MP_RELOAD_SWIM_SECONDARY_END,
	ACT_MP_RELOAD_AIRWALK_SECONDARY,
	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,
	ACT_MP_RELOAD_AIRWALK_SECONDARY_END,
	ACT_MP_ATTACK_STAND_GRENADE_SECONDARY,
	ACT_MP_ATTACK_CROUCH_GRENADE_SECONDARY,
	ACT_MP_ATTACK_SWIM_GRENADE_SECONDARY,
	ACT_MP_ATTACK_AIRWALK_GRENADE_SECONDARY,
	ACT_MP_STAND_MELEE,
	ACT_MP_CROUCH_MELEE,
	ACT_MP_RUN_MELEE,
	ACT_MP_WALK_MELEE,
	ACT_MP_AIRWALK_MELEE,
	ACT_MP_CROUCHWALK_MELEE,
	ACT_MP_JUMP_MELEE,
	ACT_MP_JUMP_START_MELEE,
	ACT_MP_JUMP_FLOAT_MELEE,
	ACT_MP_JUMP_LAND_MELEE,
	ACT_MP_SWIM_MELEE,
	ACT_MP_ATTACK_STAND_MELEE,
	ACT_MP_ATTACK_STAND_MELEE_SECONDARY,
	ACT_MP_ATTACK_CROUCH_MELEE,
	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,
	ACT_MP_ATTACK_SWIM_MELEE,
	ACT_MP_ATTACK_AIRWALK_MELEE,
	ACT_MP_ATTACK_STAND_GRENADE_MELEE,
	ACT_MP_ATTACK_CROUCH_GRENADE_MELEE,
	ACT_MP_ATTACK_SWIM_GRENADE_MELEE,
	ACT_MP_ATTACK_AIRWALK_GRENADE_MELEE,
	ACT_MP_STAND_ITEM1,
	ACT_MP_CROUCH_ITEM1,
	ACT_MP_RUN_ITEM1,
	ACT_MP_WALK_ITEM1,
	ACT_MP_AIRWALK_ITEM1,
	ACT_MP_CROUCHWALK_ITEM1,
	ACT_MP_JUMP_ITEM1,
	ACT_MP_JUMP_START_ITEM1,
	ACT_MP_JUMP_FLOAT_ITEM1,
	ACT_MP_JUMP_LAND_ITEM1,
	ACT_MP_SWIM_ITEM1,
	ACT_MP_ATTACK_STAND_ITEM1,
	ACT_MP_ATTACK_STAND_ITEM1_SECONDARY,
	ACT_MP_ATTACK_CROUCH_ITEM1,
	ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY,
	ACT_MP_ATTACK_SWIM_ITEM1,
	ACT_MP_ATTACK_AIRWALK_ITEM1,
	ACT_MP_STAND_ITEM2,
	ACT_MP_CROUCH_ITEM2,
	ACT_MP_RUN_ITEM2,
	ACT_MP_WALK_ITEM2,
	ACT_MP_AIRWALK_ITEM2,
	ACT_MP_CROUCHWALK_ITEM2,
	ACT_MP_JUMP_ITEM2,
	ACT_MP_JUMP_START_ITEM2,
	ACT_MP_JUMP_FLOAT_ITEM2,
	ACT_MP_JUMP_LAND_ITEM2,
	ACT_MP_SWIM_ITEM2,
	ACT_MP_ATTACK_STAND_ITEM2,
	ACT_MP_ATTACK_STAND_ITEM2_SECONDARY,
	ACT_MP_ATTACK_CROUCH_ITEM2,
	ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY,
	ACT_MP_ATTACK_SWIM_ITEM2,
	ACT_MP_ATTACK_AIRWALK_ITEM2,
	ACT_MP_GESTURE_FLINCH,
	ACT_MP_GESTURE_FLINCH_PRIMARY,
	ACT_MP_GESTURE_FLINCH_SECONDARY,
	ACT_MP_GESTURE_FLINCH_MELEE,
	ACT_MP_GESTURE_FLINCH_ITEM1,
	ACT_MP_GESTURE_FLINCH_ITEM2,
	ACT_MP_GESTURE_FLINCH_HEAD,
	ACT_MP_GESTURE_FLINCH_CHEST,
	ACT_MP_GESTURE_FLINCH_STOMACH,
	ACT_MP_GESTURE_FLINCH_LEFTARM,
	ACT_MP_GESTURE_FLINCH_RIGHTARM,
	ACT_MP_GESTURE_FLINCH_LEFTLEG,
	ACT_MP_GESTURE_FLINCH_RIGHTLEG,
	ACT_MP_GRENADE1_DRAW,
	ACT_MP_GRENADE1_IDLE,
	ACT_MP_GRENADE1_ATTACK,
	ACT_MP_GRENADE2_DRAW,
	ACT_MP_GRENADE2_IDLE,
	ACT_MP_GRENADE2_ATTACK,
	ACT_MP_PRIMARY_GRENADE1_DRAW,
	ACT_MP_PRIMARY_GRENADE1_IDLE,
	ACT_MP_PRIMARY_GRENADE1_ATTACK,
	ACT_MP_PRIMARY_GRENADE2_DRAW,
	ACT_MP_PRIMARY_GRENADE2_IDLE,
	ACT_MP_PRIMARY_GRENADE2_ATTACK,
	ACT_MP_SECONDARY_GRENADE1_DRAW,
	ACT_MP_SECONDARY_GRENADE1_IDLE,
	ACT_MP_SECONDARY_GRENADE1_ATTACK,
	ACT_MP_SECONDARY_GRENADE2_DRAW,
	ACT_MP_SECONDARY_GRENADE2_IDLE,
	ACT_MP_SECONDARY_GRENADE2_ATTACK,
	ACT_MP_MELEE_GRENADE1_DRAW,
	ACT_MP_MELEE_GRENADE1_IDLE,
	ACT_MP_MELEE_GRENADE1_ATTACK,
	ACT_MP_MELEE_GRENADE2_DRAW,
	ACT_MP_MELEE_GRENADE2_IDLE,
	ACT_MP_MELEE_GRENADE2_ATTACK,
	ACT_MP_ITEM1_GRENADE1_DRAW,
	ACT_MP_ITEM1_GRENADE1_IDLE,
	ACT_MP_ITEM1_GRENADE1_ATTACK,
	ACT_MP_ITEM1_GRENADE2_DRAW,
	ACT_MP_ITEM1_GRENADE2_IDLE,
	ACT_MP_ITEM1_GRENADE2_ATTACK,
	ACT_MP_ITEM2_GRENADE1_DRAW,
	ACT_MP_ITEM2_GRENADE1_IDLE,
	ACT_MP_ITEM2_GRENADE1_ATTACK,
	ACT_MP_ITEM2_GRENADE2_DRAW,
	ACT_MP_ITEM2_GRENADE2_IDLE,
	ACT_MP_ITEM2_GRENADE2_ATTACK,
	ACT_MP_STAND_BUILDING,
	ACT_MP_CROUCH_BUILDING,
	ACT_MP_RUN_BUILDING,
	ACT_MP_WALK_BUILDING,
	ACT_MP_AIRWALK_BUILDING,
	ACT_MP_CROUCHWALK_BUILDING,
	ACT_MP_JUMP_BUILDING,
	ACT_MP_JUMP_START_BUILDING,
	ACT_MP_JUMP_FLOAT_BUILDING,
	ACT_MP_JUMP_LAND_BUILDING,
	ACT_MP_SWIM_BUILDING,
	ACT_MP_ATTACK_STAND_BUILDING,
	ACT_MP_ATTACK_CROUCH_BUILDING,
	ACT_MP_ATTACK_SWIM_BUILDING,
	ACT_MP_ATTACK_AIRWALK_BUILDING,
	ACT_MP_ATTACK_STAND_GRENADE_BUILDING,
	ACT_MP_ATTACK_CROUCH_GRENADE_BUILDING,
	ACT_MP_ATTACK_SWIM_GRENADE_BUILDING,
	ACT_MP_ATTACK_AIRWALK_GRENADE_BUILDING,
	ACT_MP_STAND_PDA,
	ACT_MP_CROUCH_PDA,
	ACT_MP_RUN_PDA,
	ACT_MP_WALK_PDA,
	ACT_MP_AIRWALK_PDA,
	ACT_MP_CROUCHWALK_PDA,
	ACT_MP_JUMP_PDA,
	ACT_MP_JUMP_START_PDA,
	ACT_MP_JUMP_FLOAT_PDA,
	ACT_MP_JUMP_LAND_PDA,
	ACT_MP_SWIM_PDA,
	ACT_MP_ATTACK_STAND_PDA,
	ACT_MP_ATTACK_SWIM_PDA,
	ACT_MP_GESTURE_VC_HANDMOUTH,
	ACT_MP_GESTURE_VC_FINGERPOINT,
	ACT_MP_GESTURE_VC_FISTPUMP,
	ACT_MP_GESTURE_VC_THUMBSUP,
	ACT_MP_GESTURE_VC_NODYES,
	ACT_MP_GESTURE_VC_NODNO,
	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,
	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,
	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,
	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,
	ACT_MP_GESTURE_VC_NODYES_PRIMARY,
	ACT_MP_GESTURE_VC_NODNO_PRIMARY,
	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,
	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,
	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,
	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,
	ACT_MP_GESTURE_VC_NODYES_SECONDARY,
	ACT_MP_GESTURE_VC_NODNO_SECONDARY,
	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,
	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,
	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,
	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,
	ACT_MP_GESTURE_VC_NODYES_MELEE,
	ACT_MP_GESTURE_VC_NODNO_MELEE,
	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1,
	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1,
	ACT_MP_GESTURE_VC_FISTPUMP_ITEM1,
	ACT_MP_GESTURE_VC_THUMBSUP_ITEM1,
	ACT_MP_GESTURE_VC_NODYES_ITEM1,
	ACT_MP_GESTURE_VC_NODNO_ITEM1,
	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2,
	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2,
	ACT_MP_GESTURE_VC_FISTPUMP_ITEM2,
	ACT_MP_GESTURE_VC_THUMBSUP_ITEM2,
	ACT_MP_GESTURE_VC_NODYES_ITEM2,
	ACT_MP_GESTURE_VC_NODNO_ITEM2,
	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,
	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,
	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,
	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,
	ACT_MP_GESTURE_VC_NODYES_BUILDING,
	ACT_MP_GESTURE_VC_NODNO_BUILDING,
	ACT_MP_GESTURE_VC_HANDMOUTH_PDA,
	ACT_MP_GESTURE_VC_FINGERPOINT_PDA,
	ACT_MP_GESTURE_VC_FISTPUMP_PDA,
	ACT_MP_GESTURE_VC_THUMBSUP_PDA,
	ACT_MP_GESTURE_VC_NODYES_PDA,
	ACT_MP_GESTURE_VC_NODNO_PDA,
	ACT_VM_UNUSABLE,
	ACT_VM_UNUSABLE_TO_USABLE,
	ACT_VM_USABLE_TO_UNUSABLE,
	ACT_PRIMARY_VM_DRAW,
	ACT_PRIMARY_VM_HOLSTER,
	ACT_PRIMARY_VM_IDLE,
	ACT_PRIMARY_VM_PULLBACK,
	ACT_PRIMARY_VM_PRIMARYATTACK,
	ACT_PRIMARY_VM_SECONDARYATTACK,
	ACT_PRIMARY_VM_RELOAD,
	ACT_PRIMARY_VM_DRYFIRE,
	ACT_PRIMARY_VM_IDLE_TO_LOWERED,
	ACT_PRIMARY_VM_IDLE_LOWERED,
	ACT_PRIMARY_VM_LOWERED_TO_IDLE,
	ACT_SECONDARY_VM_DRAW,
	ACT_SECONDARY_VM_HOLSTER,
	ACT_SECONDARY_VM_IDLE,
	ACT_SECONDARY_VM_PULLBACK,
	ACT_SECONDARY_VM_PRIMARYATTACK,
	ACT_SECONDARY_VM_SECONDARYATTACK,
	ACT_SECONDARY_VM_RELOAD,
	ACT_SECONDARY_VM_DRYFIRE,
	ACT_SECONDARY_VM_IDLE_TO_LOWERED,
	ACT_SECONDARY_VM_IDLE_LOWERED,
	ACT_SECONDARY_VM_LOWERED_TO_IDLE,
	ACT_MELEE_VM_DRAW,
	ACT_MELEE_VM_HOLSTER,
	ACT_MELEE_VM_IDLE,
	ACT_MELEE_VM_PULLBACK,
	ACT_MELEE_VM_PRIMARYATTACK,
	ACT_MELEE_VM_SECONDARYATTACK,
	ACT_MELEE_VM_RELOAD,
	ACT_MELEE_VM_DRYFIRE,
	ACT_MELEE_VM_IDLE_TO_LOWERED,
	ACT_MELEE_VM_IDLE_LOWERED,
	ACT_MELEE_VM_LOWERED_TO_IDLE,
	ACT_PDA_VM_DRAW,
	ACT_PDA_VM_HOLSTER,
	ACT_PDA_VM_IDLE,
	ACT_PDA_VM_PULLBACK,
	ACT_PDA_VM_PRIMARYATTACK,
	ACT_PDA_VM_SECONDARYATTACK,
	ACT_PDA_VM_RELOAD,
	ACT_PDA_VM_DRYFIRE,
	ACT_PDA_VM_IDLE_TO_LOWERED,
	ACT_PDA_VM_IDLE_LOWERED,
	ACT_PDA_VM_LOWERED_TO_IDLE,
	ACT_ITEM1_VM_DRAW,
	ACT_ITEM1_VM_HOLSTER,
	ACT_ITEM1_VM_IDLE,
	ACT_ITEM1_VM_PULLBACK,
	ACT_ITEM1_VM_PRIMARYATTACK,
	ACT_ITEM1_VM_SECONDARYATTACK,
	ACT_ITEM1_VM_RELOAD,
	ACT_ITEM1_VM_DRYFIRE,
	ACT_ITEM1_VM_IDLE_TO_LOWERED,
	ACT_ITEM1_VM_IDLE_LOWERED,
	ACT_ITEM1_VM_LOWERED_TO_IDLE,
	ACT_ITEM2_VM_DRAW,
	ACT_ITEM2_VM_HOLSTER,
	ACT_ITEM2_VM_IDLE,
	ACT_ITEM2_VM_PULLBACK,
	ACT_ITEM2_VM_PRIMARYATTACK,
	ACT_ITEM2_VM_SECONDARYATTACK,
	ACT_ITEM2_VM_RELOAD,
	ACT_ITEM2_VM_DRYFIRE,
	ACT_ITEM2_VM_IDLE_TO_LOWERED,
	ACT_ITEM2_VM_IDLE_LOWERED,
	ACT_ITEM2_VM_LOWERED_TO_IDLE,
	ACT_RELOAD_SUCCEED,
	ACT_RELOAD_FAIL,
	ACT_WALK_AIM_AUTOGUN,
	ACT_RUN_AIM_AUTOGUN,
	ACT_IDLE_AUTOGUN,
	ACT_IDLE_AIM_AUTOGUN,
	ACT_RELOAD_AUTOGUN,
	ACT_CROUCH_IDLE_AUTOGUN,
	ACT_RANGE_ATTACK_AUTOGUN,
	ACT_JUMP_AUTOGUN,
	ACT_IDLE_AIM_PISTOL,
	ACT_WALK_AIM_DUAL,
	ACT_RUN_AIM_DUAL,
	ACT_IDLE_DUAL,
	ACT_IDLE_AIM_DUAL,
	ACT_RELOAD_DUAL,
	ACT_CROUCH_IDLE_DUAL,
	ACT_RANGE_ATTACK_DUAL,
	ACT_JUMP_DUAL,
	ACT_IDLE_SHOTGUN,
	ACT_IDLE_AIM_SHOTGUN,
	ACT_CROUCH_IDLE_SHOTGUN,
	ACT_JUMP_SHOTGUN,
	ACT_IDLE_AIM_RIFLE,
	ACT_RELOAD_RIFLE,
	ACT_CROUCH_IDLE_RIFLE,
	ACT_RANGE_ATTACK_RIFLE,
	ACT_JUMP_RIFLE,
	ACT_SLEEP,
	ACT_WAKE,
	ACT_FLICK_LEFT,
	ACT_FLICK_LEFT_MIDDLE,
	ACT_FLICK_RIGHT_MIDDLE,
	ACT_FLICK_RIGHT,
	ACT_SPINAROUND,
	ACT_PREP_TO_FIRE,
	ACT_FIRE,
	ACT_FIRE_RECOVER,
	ACT_SPRAY,
	ACT_PREP_EXPLODE,
	ACT_EXPLODE,
	ACT_DOTA_IDLE,
	ACT_DOTA_RUN,
	ACT_DOTA_ATTACK,
	ACT_DOTA_ATTACK_EVENT,
	ACT_DOTA_DIE,
	ACT_DOTA_FLINCH,
	ACT_DOTA_DISABLED,
	ACT_DOTA_CAST_ABILITY_1,
	ACT_DOTA_CAST_ABILITY_2,
	ACT_DOTA_CAST_ABILITY_3,
	ACT_DOTA_CAST_ABILITY_4,
	ACT_DOTA_OVERRIDE_ABILITY_1,
	ACT_DOTA_OVERRIDE_ABILITY_2,
	ACT_DOTA_OVERRIDE_ABILITY_3,
	ACT_DOTA_OVERRIDE_ABILITY_4,
	ACT_DOTA_CHANNEL_ABILITY_1,
	ACT_DOTA_CHANNEL_ABILITY_2,
	ACT_DOTA_CHANNEL_ABILITY_3,
	ACT_DOTA_CHANNEL_ABILITY_4,
	ACT_DOTA_CHANNEL_END_ABILITY_1,
	ACT_DOTA_CHANNEL_END_ABILITY_2,
	ACT_DOTA_CHANNEL_END_ABILITY_3,
	ACT_DOTA_CHANNEL_END_ABILITY_4,
	ACT_MP_RUN_SPEEDPAINT,
	ACT_MP_LONG_FALL,
	ACT_MP_TRACTORBEAM_FLOAT,
	ACT_MP_DEATH_CRUSH,
	ACT_MP_RUN_SPEEDPAINT_PRIMARY,
	ACT_MP_DROWNING_PRIMARY,
	ACT_MP_LONG_FALL_PRIMARY,
	ACT_MP_TRACTORBEAM_FLOAT_PRIMARY,
	ACT_MP_DEATH_CRUSH_PRIMARY,
	ACT_DIE_STAND,
	ACT_DIE_STAND_HEADSHOT,
	ACT_DIE_CROUCH,
	ACT_DIE_CROUCH_HEADSHOT,
	ACT_CSGO_NULL,
	ACT_CSGO_DEFUSE,
	ACT_CSGO_DEFUSE_WITH_KIT,
	ACT_CSGO_FLASHBANG_REACTION,
	ACT_CSGO_FIRE_PRIMARY,
	ACT_CSGO_FIRE_PRIMARY_OPT_1,
	ACT_CSGO_FIRE_PRIMARY_OPT_2,
	ACT_CSGO_FIRE_SECONDARY,
	ACT_CSGO_FIRE_SECONDARY_OPT_1,
	ACT_CSGO_FIRE_SECONDARY_OPT_2,
	ACT_CSGO_RELOAD,
	ACT_CSGO_RELOAD_START,
	ACT_CSGO_RELOAD_LOOP,
	ACT_CSGO_RELOAD_END,
	ACT_CSGO_OPERATE,
	ACT_CSGO_DEPLOY,
	ACT_CSGO_CATCH,
	ACT_CSGO_SILENCER_DETACH,
	ACT_CSGO_SILENCER_ATTACH,
	ACT_CSGO_TWITCH,
	ACT_CSGO_TWITCH_BUYZONE,
	ACT_CSGO_PLANT_BOMB,
	ACT_CSGO_IDLE_TURN_BALANCEADJUST,
	ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING,
	ACT_CSGO_ALIVE_LOOP,
	ACT_CSGO_FLINCH,
	ACT_CSGO_FLINCH_HEAD,
	ACT_CSGO_FLINCH_MOLOTOV,
	ACT_CSGO_JUMP,
	ACT_CSGO_FALL,
	ACT_CSGO_CLIMB_LADDER,
	ACT_CSGO_LAND_LIGHT,
	ACT_CSGO_LAND_HEAVY,
	ACT_CSGO_EXIT_LADDER_TOP,
	ACT_CSGO_EXIT_LADDER_BOTTOM,
};


class CGlowObjectManager
{
public:
	int RegisterGlowObject(C_BasePlayer *pEntity, const Vector &vGlowColor, float flGlowAlpha, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded, int nSplitScreenSlot)
	{
		int nIndex;
		if (m_nFirstFreeSlot == END_OF_FREE_LIST) {
			nIndex = -1;
		}
		else {
			nIndex = m_nFirstFreeSlot;
			m_nFirstFreeSlot = m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot;
		}

		m_GlowObjectDefinitions[nIndex].m_pEntity = pEntity;
		m_GlowObjectDefinitions[nIndex].m_vGlowColor = vGlowColor;
		m_GlowObjectDefinitions[nIndex].m_flAlpha = flGlowAlpha;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
		m_GlowObjectDefinitions[nIndex].m_nSplitScreenSlot = nSplitScreenSlot;
		m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot = ENTRY_IN_USE;

		return nIndex;
	}

	void UnregisterGlowObject(int nGlowObjectHandle)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_pEntity = NULL;
		m_nFirstFreeSlot = nGlowObjectHandle;
	}

	void SetEntity(int nGlowObjectHandle, C_BasePlayer *pEntity)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_pEntity = pEntity;
	}

	void SetColor(int nGlowObjectHandle, const Vector &vGlowColor)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_vGlowColor = vGlowColor;
	}

	void SetAlpha(int nGlowObjectHandle, float flAlpha)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_flAlpha = flAlpha;
	}

	void SetRenderFlags(int nGlowObjectHandle, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
	}

	GlowObjectDefinition_t* m_GlowObjectDefinitions; //0x0000

	int GetSize()
	{
		return *reinterpret_cast<int*>(uintptr_t(this) + 0xC);
	}

	int m_nFirstFreeSlot;                              //0x000C
};

class OverlayText_t;
class Color;

class IVDebugOverlay
{
public:
	virtual void AddEntityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...) = 0;
	virtual void AddBoxOverlay(const Vector &origin, const Vector &mins, const Vector &max, Vector const &orientation, int r, int g, int b, int a, float duration) = 0;
	virtual void AddSphereOverlay(const Vector &vOrigin, float flRadius, int nTheta, int nPhi, int r, int g, int b, int a, float flDuration) = 0;
	virtual void AddTriangleOverlay(const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
	virtual void AddLineOverlay(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest, float duration) = 0;
	virtual void AddTextOverlay(const Vector &origin, float duration, const char *format, ...) = 0;
	virtual void AddTextOverlay(const Vector &origin, int line_offset, float duration, const char *format, ...) = 0;
	virtual void AddScreenTextOverlay(float flXPos, float flYPos, float flDuration, int r, int g, int b, int a, const char *text) = 0;
	virtual void AddSweptBoxOverlay(const Vector &start, const Vector &end, const Vector &mins, const Vector &max, const Vector &angles, int r, int g, int b, int a, float flDuration) = 0;
	virtual void AddGridOverlay(const Vector &origin) = 0;
	virtual void AddCoordFrameOverlay(const matrix3x4_t &frame, float flScale, int vColorTable[3][3] = NULL) = 0;
	virtual int ScreenPosition(const Vector &point, Vector &screen) = 0;
	virtual int ScreenPosition(float flXPos, float flYPos, Vector &screen) = 0;
	virtual OverlayText_t *GetFirst(void) = 0;
	virtual OverlayText_t *GetNext(OverlayText_t *current) = 0;
	virtual void ClearDeadOverlays(void) = 0;
	virtual void ClearAllOverlays(void) = 0;
	virtual void AddTextOverlayRGB(const Vector &origin, int line_offset, float duration, float r, float g, float b, float alpha, const char *format, ...) = 0;
	virtual void AddTextOverlayRGB(const Vector &origin, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...) = 0;
	virtual void AddLineOverlayAlpha(const Vector &origin, const Vector &dest, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
	virtual void AddBoxOverlay2(const Vector &origin, const Vector &mins, const Vector &max, Vector const &orientation, const Color faceColor, const Color edgeColor, float duration) = 0;
	virtual void AddLineOverlay(const Vector &origin, const Vector &dest, int r, int g, int b, int a, float, float) = 0;
	virtual void PurgeTextOverlays() = 0;
	virtual void AddCapsuleOverlay(const Vector& mins, const Vector& max, float& radius, int r, int g, int b, int a, float duration, char unk, char ignorez) = 0;
};

class IMemAlloc
{
public:
	virtual ~IMemAlloc();

	// Release versions
	virtual void *Alloc(size_t nSize) = 0;
	virtual void *Realloc(void *pMem, size_t nSize) = 0;
	virtual void Free(void *pMem) = 0;
	virtual void *Expand_NoLongerSupported(void *pMem, size_t nSize) = 0;
};

typedef void* FileNameHandle_t;

struct SndInfo_t
{
	// Sound Guid
	int			m_nGuid;
	FileNameHandle_t m_filenameHandle;		// filesystem filename handle - call IFilesystem to conver this to a string
	int			m_nSoundSource;
	int			m_nChannel;
	// If a sound is being played through a speaker entity (e.g., on a monitor,), this is the
	//  entity upon which to show the lips moving, if the sound has sentence data
	int			m_nSpeakerEntity;
	float		m_flVolume;
	float		m_flLastSpatializedVolume;
	// Radius of this sound effect (spatialization is different within the radius)
	float		m_flRadius;
	int			m_nPitch;
	Vector		*m_pOrigin;
	Vector		*m_pDirection;

	// if true, assume sound source can move and update according to entity
	bool		m_bUpdatePositions;
	// true if playing linked sentence
	bool		m_bIsSentence;
	// if true, bypass all dsp processing for this sound (ie: music)	
	bool		m_bDryMix;
	// true if sound is playing through in-game speaker entity.
	bool		m_bSpeaker;
	// for snd_show, networked sounds get colored differently than local sounds
	bool		m_bFromServer;
};

class bf_read {
public:
	const char* m_pDebugName;
	bool m_bOverflow;
	int m_nDataBits;
	unsigned int m_nDataBytes;
	unsigned int m_nInBufWord;
	int m_nBitsAvail;
	const unsigned int* m_pDataIn;
	const unsigned int* m_pBufferEnd;
	const unsigned int* m_pData;

	bf_read() = default;

	bf_read(const void* pData, int nBytes, int nBits = -1) {
		StartReading(pData, nBytes, 0, nBits);
	}

	void StartReading(const void* pData, int nBytes, int iStartBit, int nBits) {
		// Make sure it's dword aligned and padded.
		m_pData = (uint32_t*)pData;
		m_pDataIn = m_pData;
		m_nDataBytes = nBytes;

		if (nBits == -1) {
			m_nDataBits = nBytes << 3;
		}
		else {
			m_nDataBits = nBits;
		}
		m_bOverflow = false;
		m_pBufferEnd = reinterpret_cast< uint32_t const* >(reinterpret_cast< uint8_t const* >(m_pData) + nBytes);
		if (m_pData)
			Seek(iStartBit);
	}

	bool Seek(int nPosition) {
		bool bSucc = true;
		if (nPosition < 0 || nPosition > m_nDataBits) {
			m_bOverflow = true;
			bSucc = false;
			nPosition = m_nDataBits;
		}
		int nHead = m_nDataBytes & 3; // non-multiple-of-4 bytes at head of buffer. We put the "round off"
									  // at the head to make reading and detecting the end efficient.

		int nByteOfs = nPosition / 8;
		if ((m_nDataBytes < 4) || (nHead && (nByteOfs < nHead))) {
			// partial first dword
			uint8_t const* pPartial = (uint8_t const*)m_pData;
			if (m_pData) {
				m_nInBufWord = *(pPartial++);
				if (nHead > 1)
					m_nInBufWord |= (*pPartial++) << 8;
				if (nHead > 2)
					m_nInBufWord |= (*pPartial++) << 16;
			}
			m_pDataIn = (uint32_t const*)pPartial;
			m_nInBufWord >>= (nPosition & 31);
			m_nBitsAvail = (nHead << 3) - (nPosition & 31);
		}
		else {
			int nAdjPosition = nPosition - (nHead << 3);
			m_pDataIn = reinterpret_cast< uint32_t const* >(
				reinterpret_cast< uint8_t const* >(m_pData) + ((nAdjPosition / 32) << 2) + nHead);
			if (m_pData) {
				m_nBitsAvail = 32;
				GrabNextDWord();
			}
			else {
				m_nInBufWord = 0;
				m_nBitsAvail = 1;
			}
			m_nInBufWord >>= (nAdjPosition & 31);
			m_nBitsAvail = min(m_nBitsAvail, 32 - (nAdjPosition & 31)); // in case grabnextdword overflowed
		}
		return bSucc;
	}

	FORCEINLINE void GrabNextDWord(bool bOverFlowImmediately = false) {
		if (m_pDataIn == m_pBufferEnd) {
			m_nBitsAvail = 1; // so that next read will run out of words
			m_nInBufWord = 0;
			m_pDataIn++; // so seek count increments like old
			if (bOverFlowImmediately)
				m_bOverflow = true;
		}
		else if (m_pDataIn > m_pBufferEnd) {
			m_bOverflow = true;
			m_nInBufWord = 0;
		}
		else {
			m_nInBufWord = DWORD(*(m_pDataIn++));
		}
	}
};
//class bf_read
//{
//public:
//	const char* m_pDebugName;
//	bool m_bOverflow;
//	int m_nDataBits;
//	unsigned int m_nDataBytes;
//	unsigned int m_nInBufWord;
//	int m_nBitsAvail;
//	const unsigned int* m_pDataIn;
//	const unsigned int* m_pBufferEnd;
//	const unsigned int* m_pData;
//
//	uintptr_t base_address;
//	uintptr_t cur_offset;
//
//	bf_read()
//	{
//
//	};
//
//	bf_read::bf_read(uintptr_t addr)
//	{
//		base_address = addr;
//		cur_offset = 0;
//	}
//
//	void bf_read::SetOffset(uintptr_t offset)
//	{
//		cur_offset = offset;
//	}
//
//	void bf_read::Skip(uintptr_t length)
//	{
//		cur_offset += length;
//	}
//
//	int bf_read::ReadByte()
//	{
//		auto val = *reinterpret_cast<char*>(base_address + cur_offset);
//		++cur_offset;
//		return val;
//	}
//
//	bool bf_read::ReadBool()
//	{
//		auto val = *reinterpret_cast<bool*>(base_address + cur_offset);
//		++cur_offset;
//		return val;
//	}
//
//	std::string bf_read::ReadString()
//	{
//		char buffer[256];
//		auto str_length = *reinterpret_cast<char*>(base_address + cur_offset);
//		++cur_offset;
//		memcpy(buffer, reinterpret_cast<void*>(base_address + cur_offset), str_length > 255 ? 255 : str_length);
//		buffer[str_length > 255 ? 255 : str_length] = '\0';
//		cur_offset += str_length + 1;
//		return std::string(buffer);
//	}
//};

class bf_write {
public:
	unsigned char* m_pData;
	int m_nDataBytes;
	int m_nDataBits;
	int m_iCurBit;
	bool m_bOverflow;
	bool m_bAssertOnOverflow;
	const char* m_pDebugName;

	void StartWriting(void* pData, int nBytes, int iStartBit = 0, int nBits = -1) {
		// Make sure it's dword aligned and padded.
		// The writing code will overrun the end of the buffer if it isn't dword aligned, so truncate to force alignment
		nBytes &= ~3;

		m_pData = (unsigned char*)pData;
		m_nDataBytes = nBytes;

		if (nBits == -1) {
			m_nDataBits = nBytes << 3;
		}
		else {
			m_nDataBits = nBits;
		}

		m_iCurBit = iStartBit;
		m_bOverflow = false;
	}

	bf_write() {
		m_pData = NULL;
		m_nDataBytes = 0;
		m_nDataBits = -1; // set to -1 so we generate overflow on any operation
		m_iCurBit = 0;
		m_bOverflow = false;
		m_bAssertOnOverflow = true;
		m_pDebugName = NULL;
	}

	// nMaxBits can be used as the number of bits in the buffer.
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write(void* pData, int nBytes, int nBits = -1) {
		m_bAssertOnOverflow = true;
		m_pDebugName = NULL;
		StartWriting(pData, nBytes, 0, nBits);
	}

	bf_write(const char* pDebugName, void* pData, int nBytes, int nBits = -1) {
		m_bAssertOnOverflow = true;
		m_pDebugName = pDebugName;
		StartWriting(pData, nBytes, 0, nBits);
	}
};

class CLC_Move {
private:
	char __PAD0[0x8]; // 0x0 two vtables
public:
	int m_nBackupCommands; // 0x8
	int m_nNewCommands;    // 0xC
	std::string* m_data;   // 0x10 std::string
	bf_read m_DataIn;      // 0x14
	bf_write m_DataOut;    // 0x38
};                       // size: 0x50

template < typename T >
class CNetMessagePB : public INetMessage, public T {};
using CCLCMsg_Move_t = CNetMessagePB< CLC_Move >;

class IGameEvent
{
public:
	virtual ~IGameEvent() {};
	virtual const char*     GetName() const = 0;

	virtual bool            IsReliable() const = 0;
	virtual bool            IsLocal() const = 0;
	virtual bool            IsEmpty(const char *keyName = nullptr) = 0;

	virtual bool            GetBool(const char *keyName = nullptr, bool defaultValue = false) = 0;
	virtual int             GetInt(const char *keyName = nullptr, int defaultValue = 0) = 0;
	virtual uint64_t        GetUint64(const char *keyName = nullptr, unsigned long defaultValue = 0) = 0;
	virtual float           GetFloat(const char *keyName = nullptr, float defaultValue = 0.0f) = 0;
	virtual const char*     GetString(const char *keyName = nullptr, const char *defaultValue = "") = 0;
	virtual const wchar_t*  GetWString(const char *keyName, const wchar_t *defaultValue = L"") = 0;

	virtual void			lol() = 0;
	virtual void            SetBool(const char *keyName, bool value) = 0;
	virtual void            SetInt(const char *keyName, int value) = 0;
	virtual void            SetUint64(const char *keyName, unsigned long value) = 0;
	virtual void            SetFloat(const char *keyName, float value) = 0;
	virtual void            SetString(const char *keyName, const char *value) = 0;
	virtual void            SetWString(const char *keyName, const wchar_t *value) = 0;
};
class IGameEventListener {
public:
	virtual ~IGameEventListener() {}
	virtual void FireGameEvent(IGameEvent* Event) = 0;
	virtual int GetEventDebugID() { return 42; }
};
class IGameEventManager {
//public:
//	virtual int __Unknown_1(int* dwUnknown) = 0;
//
//	// load game event descriptions from a file eg "resource\gameevents.res"
//	virtual int LoadEventsFromFile(const char *filename) = 0;
//
//	// removes all and anything
//	virtual void Reset() = 0;
//
//	// adds a listener for a particular event
//	virtual bool AddListener(IGameEventListener *listener, const char *name, bool bServerSide) = 0;
//
//	// returns true if this listener is listens to given event
//	virtual bool FindListener(IGameEventListener *listener, const char *name) = 0;
//
//	// removes a listener 
//	virtual int RemoveListener(IGameEventListener *listener) = 0;
//
//	// create an event by name, but doesn't fire it. returns NULL is event is not
//	// known or no listener is registered for it. bForce forces the creation even if no listener is active
//	virtual IGameEvent *CreateEvent(const char *name, bool bForce, unsigned int dwUnknown) = 0;
//
//	// fires a server event created earlier, if bDontBroadcast is set, event is not send to clients
//	virtual bool FireEvent(IGameEvent *event, bool bDontBroadcast = false) = 0;
//
//	// fires an event for the local client only, should be used only by client code
//	virtual bool FireEventClientSide(IGameEvent *event) = 0;
//
//	// create a new copy of this event, must be free later
//	virtual IGameEvent *DuplicateEvent(IGameEvent *event) = 0;
//
//	// if an event was created but not fired for some reason, it has to bee freed, same UnserializeEvent
//	virtual void FreeEvent(IGameEvent *event) = 0;
//
//	// write/read event to/from bitbuffer
//	virtual bool SerializeEvent(IGameEvent *event, bf_write *buf) = 0;
//
//	// create new KeyValues, must be deleted
//	virtual IGameEvent *UnserializeEvent(bf_read *buf) = 0;
public:
	virtual             ~IGameEventManager() = 0;
	virtual int         LoadEventsFromFile(const char *filename) = 0;
	virtual void        Reset() = 0;
	virtual bool        AddListener(IGameEventListener *listener, const char *name, bool bServerSide) = 0;
	virtual bool        FindListener(IGameEventListener *listener, const char *name) = 0;
	virtual int         RemoveListener(IGameEventListener *listener) = 0;
	virtual IGameEvent* CreateEvent(const char *name, bool bForce, unsigned int dwUnknown) = 0;
	virtual bool        FireEvent(IGameEvent *event, bool bDontBroadcast = false) = 0;
	virtual bool        FireEventClientSide(IGameEvent *event) = 0;
	virtual IGameEvent* DuplicateEvent(IGameEvent *event) = 0;
	virtual void        FreeEvent(IGameEvent *event) = 0;
	virtual bool        SerializeEvent(IGameEvent *event, bf_write *buf) = 0;
	virtual IGameEvent* UnserializeEvent(bf_read *buf) = 0;
};

class CEventInfo {
public:
	uint16_t classID;          //0x0000 0 implies not in use
	char pad_0002[2];          //0x0002
	float fire_delay;          //0x0004 If non-zero, the delay time when the event should be fired ( fixed up on the client )
	char pad_0008[4];          //0x0008
	ClientClass* pClientClass; //0x000C
	void* pData;               //0x0010 Raw event data
	char pad_0014[36];         //0x0014
	CEventInfo* next;          //0x0038
	char pad_003C[8]; // 0x003C
};

class Netmsgbinder;
class INetChannel;

class INetChannelHandler
{
public:
	virtual	~INetChannelHandler(void) {};

	virtual void ConnectionStart(INetChannel *chan) = 0;	// called first time network channel is established

	virtual void ConnectionClosing(const char *reason) = 0; // network channel is being closed by remote site

	virtual void ConnectionCrashed(const char *reason) = 0; // network error occured

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0;	// called each time a new packet arrived

	virtual void PacketEnd(void) = 0; // all messages has been parsed

	virtual void FileRequested(const char *fileName, unsigned int transferID) = 0; // other side request a file for download

	virtual void FileReceived(const char *fileName, unsigned int transferID) = 0; // we received a file

	virtual void FileDenied(const char *fileName, unsigned int transferID) = 0;	// a file request was denied by other side

	virtual void FileSent(const char *fileName, unsigned int transferID) = 0;	// we sent a file
};

//#define member_func_args(...) (this, __VA_ARGS__ ); }
//#define vfunc(index, func, sig) auto func { return reinterpret_cast<sig>((*(uint32_t**)this)[index]) member_func_args

class c_net_msg
{
public:
	virtual	~c_net_msg() = default;

	virtual void set_net_channel(void* netchan) = 0;
	virtual void set_reliable(bool state) = 0;
	virtual bool process() = 0;
	virtual	bool read_from_buffer(void* buffer) = 0;
	virtual	bool write_to_buffer(void* buffer) = 0;
	virtual bool is_reliable() const = 0;
	virtual int	get_type() const = 0;
	virtual int	get_group() const = 0;
	virtual const char* get_name() const = 0;
	virtual void* get_net_channel() const = 0;
	virtual const char* to_string() const = 0;
};

class INetChannel
{
public:
	/*uint32_t* get_vtable()
	{

		static const auto table = reinterpret_cast<uint32_t>(Memory::Scan(cheat::main::enginedll, "68 ? ? ? ? C7 07")) + 7;
		return *reinterpret_cast<uint32_t**>(table);
	}*/

	float get_latency(const int flow)
	{
		return Memory::VCall<float(__thiscall*)(INetChannel*, int)>(this, 9)(this, flow);
	}

	bool send_netmsg(c_net_msg* msg, bool reliable, bool voice)
	{
		return Memory::VCall<bool(__thiscall*)(INetChannel*, c_net_msg*, bool, bool)>(this, 40)(this, msg, reliable, voice);
	}

	int send_datagram(void *lol = nullptr)
	{
		return Memory::VCall<int(__thiscall*)(INetChannel*, void*)>(this, 46)(this, lol);
	}

	bool has_pending_reliable_data()
	{
		return Memory::VCall<bool(__thiscall*)(INetChannel*)>(this, 59)(this);
	}

	/*vfunc(9, get_latency(const int flow), float(__thiscall*)(c_net_channel*, int))(flow)
	vfunc(40, send_netmsg(c_net_msg* msg, bool reliable, bool voice), bool(__thiscall*)(c_net_channel*, c_net_msg*, bool, bool))(msg, reliable, voice)
	vfunc(46, send_datagram(), int(__thiscall*)(c_net_channel*, void*))(nullptr)
	vfunc(59, has_pending_reliable_data(), bool(__thiscall*)(c_net_channel*))()*/

	char pad_0000[20];
	bool processing_messages;
	bool should_delete;
	char pad_0016[2];
	int out_sequence_nr;
	int in_sequence_nr;
	int out_sequence_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
	char pad_0030[1044];
	//__int32 vtable; //0x0000 
	//Netmsgbinder* msgbinder1; //0x0004 
	//Netmsgbinder* msgbinder2; //0x0008 
	//Netmsgbinder* msgbinder3; //0x000C 
	//Netmsgbinder* msgbinder4; //0x0010 
	//unsigned char m_bProcessingMessages; //0x0014 
	//unsigned char m_bShouldDelete; //0x0015 
	//char pad_0x0016[0x2]; //0x0016
	//__int32 m_nOutSequenceNr; //0x0018 
	//__int32 m_nInSequenceNr; //0x001C 
	//__int32 m_nOutSequenceNrAck; //0x0020 
	//__int32 m_nOutReliableState; //0x0024 
	//__int32 m_nInReliableState; //0x0028 
	//__int32 m_nChokedPackets; //0x002C 

	//char m_StreamReliable; //0x0030 
	///*CUtlMemory*/ char m_ReliableDataBuffer[12]; //0x0048 
	//char m_StreamUnreliable; //0x0054 
	///*CUtlMemory*/ char m_UnreliableDataBuffer[12]; //0x006C 
	//char m_StreamVoice; //0x0078 
	///*CUtlMemory*/char m_VoiceDataBuffer[12]; //0x0090 
	//__int32 m_Socket; //0x009C 
	//__int32 m_StreamSocket; //0x00A0 
	//__int32 m_MaxReliablePayloadSize; //0x00A4 
	//char pad_0x00A8[0x4]; //0x00A8
	//char remote_address; //0x00AC //netadr_t
	//char dylanpadding[84]; //padding added by dylan
	//float last_received; //0x00B8  //dylan found 0x10c
	//					 //char pad_0x00BC[0x4]; //0x00BC
	//double /*float*/ connect_time; //0x00C0 //dylan found 0x110
	//							   //char pad_0x00C4[0x4]; //0x00C4
	//__int32 m_Rate; //0x00C8  //dylan found 0x118
	///*float*/double m_fClearTime; //0x00CC  //dylan found 0x120
	//char pad_0x00D0[0x8]; //0x00D0
	//char m_WaitingList[48];
	////CUtlVector m_WaitingList[0]; //0x00D8 
	////CUtlVector m_WaitingList[1]; //0x00EC 
	////char pad_0x0100[0x4120]; //0x0100
	//char pad_0x0100[0x40F0]; //dylan changed
	//__int32 m_PacketDrop; //0x4220  //dylan found 0x4250
	//char m_Name[32]; //0x4224 
	//__int32 m_ChallengeNr; //0x4244 
	//float m_Timeout; //0x4248 //dylan found 0x4278
	//INetChannelHandler* m_MessageHandler; //0x424C 
	///*CUtlVector*/char m_NetMessages[14]; //0x4250 
	//__int32 dylanUnknown;
	//void* m_pDemoRecorder; //0x4264 
	//__int32 m_nQueuedPackets; //0x4268  //dylan found 0x4298
	//float m_flInterpolationAmount; //0x426C //dylan found 0x429c
	//float m_flRemoteFrameTime; //0x4270 //dylan found 0x42a0
	//float m_flRemoteFrameTimeStdDeviation; //0x4274  //dylan found 0x42a4
	//float m_flRemoteFrameTimeUnknown; //dylan found 0x42a8
	//__int32 m_nMaxRoutablePayloadSize; //0x4278  //dylan found 0x42ac
	//__int32 m_nSplitPacketSequence; //0x427C  //dylan found 0x42b0
	//char pad_0x4280[0x14]; //0x4280

};//Size=0x4294

class CClientState
{
public:
	char _0x0000[156];
	INetChannel* m_ptrNetChannel; //0x009C 
	__int32 m_iChallengeNr; //0x00A0 
	char _0x00A4[100];
	__int32 m_iSignonState; //0x0108 
	char _0x010C[8];
	float m_flNextCmdTime; //0x0114 
	__int32 m_iServerCount; //0x0118 
	__int32 m_iCurrentSequence; //0x011C 
	char _0x0120[4];
	__int32 m_iClockDriftMgr; //0x0124 
	char _0x0128[76];
	__int32 m_iDeltaTick; //0x0174 
	bool m_bPaused; //0x0178 
	char _0x017C[12];
	char m_szLevelName[260]; //0x0188 
	char m_szLevelNameShort[40]; //0x028C 
	char m_szGroupName[40]; //0x02B4 
	char m_szSecondName[32]; //0x02DC 
	char _0x02FC[140];
	__int32 m_nMaxClients; //0x0388 
	char _0x038C[18820];
	float m_flLastServerTickTime; //0x4D10 
	bool m_bInSimulation; //0x4D14 
	__int32 m_iOldTickCount; //0x4D18 
	float m_flTickRemainder; //0x4D1C 
	float m_flFrameTime; //0x4D20 
	__int32 m_iLastOutgoingCommand; //0x4D24 
	__int32 m_iChockedCommands; //0x4D28 
	__int32 m_iLastCommandAck; //0x4D2C 
	__int32 m_iCommandAck; //0x4D30 
	__int32 m_iSoundSequence; //0x4D34 
	char _0x4D38[80];
	Vector m_vecViewAngles; //0x4D88 
	char _0x4D94[55];
	CEventInfo* m_ptrEvents; //0x4E64  
};

enum MaterialPropertyTypes_t
{
	MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,					// bool
	MATERIAL_PROPERTY_OPACITY,								// int (enum MaterialPropertyOpacityTypes_t)
	MATERIAL_PROPERTY_REFLECTIVITY,							// vec3_t
	MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS				// bool
};

class CBoneAccessor
{
public:
	inline CBoneAccessor()
	{
		m_pAnimating = NULL;
		m_pBones = NULL;
		m_ReadableBones = m_WritableBones = 0;
	}

	inline CBoneAccessor::CBoneAccessor(matrix3x4_t *pBones) // This can be used to allow access to all bones.
	{
		m_pAnimating = NULL;
		m_pBones = pBones;
	}

	inline void Init(const C_BaseAnimating *pAnimating, matrix3x4_t *pBones)// Initialize.
	{
		m_pAnimating = pAnimating;
		m_pBones = pBones;
	}

	inline int CBoneAccessor::GetReadableBones()
	{
		return m_ReadableBones;
	}

	inline void CBoneAccessor::SetReadableBones(int flags)
	{
		m_ReadableBones = flags;
	}

	inline int CBoneAccessor::GetWritableBones()
	{
		return m_WritableBones;
	}

	inline void CBoneAccessor::SetWritableBones(int flags)
	{
		m_WritableBones = flags;
	}

	// Get bones for read or write access.
	inline const matrix3x4_t& CBoneAccessor::GetBone(int iBone) const
	{
		return m_pBones[iBone];
	}

	inline const matrix3x4_t& CBoneAccessor::operator[](int iBone) const
	{
		return m_pBones[iBone];
	}

	inline matrix3x4_t& CBoneAccessor::GetBoneForWrite(int iBone)
	{
		return m_pBones[iBone];
	}

	inline matrix3x4_t *CBoneAccessor::GetBoneArrayForWrite(void) const
	{
		return m_pBones;
	}

	const C_BaseAnimating *m_pAnimating;

	matrix3x4_t *m_pBones;

	int m_ReadableBones;        // Which bones can be read.
	int m_WritableBones;        // Which bones can be written.
};

class INetMessage {
public:
	virtual ~INetMessage() {};

	// Use these to setup who can hear whose voice.
	// Pass in client indices (which are their ent indices - 1).

	virtual void SetNetChannel(INetChannel* netchan) = 0; // netchannel this message is from/for
	virtual void SetReliable(bool state) = 0;             // set to true if it's a reliable message

	virtual bool Process(void) = 0; // calles the recently set handler to process this message

	virtual bool ReadFromBuffer(bf_read& buffer) = 0; // returns true if parsing was OK
	virtual bool WriteToBuffer(bf_write& buffer) = 0; // returns true if writing was OK

	virtual bool IsReliable(void) const = 0; // true, if message needs reliable handling

	virtual int GetType(void) const = 0;         // returns module specific header tag eg svc_serverinfo
	virtual int GetGroup(void) const = 0;        // returns net message group of this message
	virtual const char* GetName(void) const = 0; // returns network message name, eg "svc_serverinfo"
	virtual INetChannel* GetNetChannel(void) const = 0;
	virtual const char* ToString(void) const = 0; // returns a human readable string about message content
};

enum ImageFormat
{
	IMAGE_FORMAT_UNKNOWN = -1,
	IMAGE_FORMAT_RGBA8888 = 0,
	IMAGE_FORMAT_ABGR8888,
	IMAGE_FORMAT_RGB888,
	IMAGE_FORMAT_BGR888,
	IMAGE_FORMAT_RGB565,
	IMAGE_FORMAT_I8,
	IMAGE_FORMAT_IA88,
	IMAGE_FORMAT_P8,
	IMAGE_FORMAT_A8,
	IMAGE_FORMAT_RGB888_BLUESCREEN,
	IMAGE_FORMAT_BGR888_BLUESCREEN,
	IMAGE_FORMAT_ARGB8888,
	IMAGE_FORMAT_BGRA8888,
	IMAGE_FORMAT_DXT1,
	IMAGE_FORMAT_DXT3,
	IMAGE_FORMAT_DXT5,
	IMAGE_FORMAT_BGRX8888,
	IMAGE_FORMAT_BGR565,
	IMAGE_FORMAT_BGRX5551,
	IMAGE_FORMAT_BGRA4444,
	IMAGE_FORMAT_DXT1_ONEBITALPHA,
	IMAGE_FORMAT_BGRA5551,
	IMAGE_FORMAT_UV88,
	IMAGE_FORMAT_UVWQ8888,
	IMAGE_FORMAT_RGBA16161616F,
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,			// Single-channel 32-bit floating point
	IMAGE_FORMAT_RGB323232F,	// NOTE: D3D9 does not have this format
	IMAGE_FORMAT_RGBA32323232F,
	IMAGE_FORMAT_RG1616F,
	IMAGE_FORMAT_RG3232F,
	IMAGE_FORMAT_RGBX8888,

	IMAGE_FORMAT_NULL,			// Dummy format which takes no video memory

								// Compressed normal map formats
								IMAGE_FORMAT_ATI2N,			// One-surface ATI2N / DXN format
								IMAGE_FORMAT_ATI1N,			// Two-surface ATI1N format

								IMAGE_FORMAT_RGBA1010102,	// 10 bit-per component render targets
								IMAGE_FORMAT_BGRA1010102,
								IMAGE_FORMAT_R16F,			// 16 bit FP format

															// Depth-stencil texture formats
															IMAGE_FORMAT_D16,
															IMAGE_FORMAT_D15S1,
															IMAGE_FORMAT_D32,
															IMAGE_FORMAT_D24S8,
															IMAGE_FORMAT_LINEAR_D24S8,
															IMAGE_FORMAT_D24X8,
															IMAGE_FORMAT_D24X4S4,
															IMAGE_FORMAT_D24FS8,
															IMAGE_FORMAT_D16_SHADOW,	// Specific formats for shadow mapping
															IMAGE_FORMAT_D24X8_SHADOW,	// Specific formats for shadow mapping

																						// supporting these specific formats as non-tiled for procedural cpu access (360-specific)
																						IMAGE_FORMAT_LINEAR_BGRX8888,
																						IMAGE_FORMAT_LINEAR_RGBA8888,
																						IMAGE_FORMAT_LINEAR_ABGR8888,
																						IMAGE_FORMAT_LINEAR_ARGB8888,
																						IMAGE_FORMAT_LINEAR_BGRA8888,
																						IMAGE_FORMAT_LINEAR_RGB888,
																						IMAGE_FORMAT_LINEAR_BGR888,
																						IMAGE_FORMAT_LINEAR_BGRX5551,
																						IMAGE_FORMAT_LINEAR_I8,
																						IMAGE_FORMAT_LINEAR_RGBA16161616,

																						IMAGE_FORMAT_LE_BGRX8888,
																						IMAGE_FORMAT_LE_BGRA8888,

																						NUM_IMAGE_FORMATS
};


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class IMaterialVar;
class ITexture;
class IMaterialProxy;
class Vector;

typedef uint64 VertexFormat_t;

//-----------------------------------------------------------------------------
// Flags for GetVertexFormat
//-----------------------------------------------------------------------------
enum VertexFormatFlags_t
{
	// Indicates an uninitialized VertexFormat_t value
	VERTEX_FORMAT_INVALID = 0xFFFFFFFF,

	VERTEX_POSITION = 0x0001,
	VERTEX_NORMAL = 0x0002,
	VERTEX_COLOR = 0x0004,
	VERTEX_SPECULAR = 0x0008,

	VERTEX_TANGENT_S = 0x0010,
	VERTEX_TANGENT_T = 0x0020,
	VERTEX_TANGENT_SPACE = VERTEX_TANGENT_S | VERTEX_TANGENT_T,

	// Indicates we're using wrinkle
	VERTEX_WRINKLE = 0x0040,

	// Indicates we're using bone indices
	VERTEX_BONE_INDEX = 0x0080,

	// Indicates this expects a color stream on stream 1
	VERTEX_COLOR_STREAM_1 = 0x0100,

	// Indicates this format shouldn't be bloated to cache align it
	// (only used for VertexUsage)
	VERTEX_FORMAT_USE_EXACT_FORMAT = 0x0200,

	// Indicates that compressed vertex elements are to be used (see also VertexCompressionType_t)
	VERTEX_FORMAT_COMPRESSED = 0x400,

	// Position or normal (if present) should be 4D not 3D
	VERTEX_FORMAT_PAD_POS_NORM = 0x800,

	// Update this if you add or remove bits...
	VERTEX_LAST_BIT = 11,

	VERTEX_BONE_WEIGHT_BIT = VERTEX_LAST_BIT + 1,
	USER_DATA_SIZE_BIT = VERTEX_LAST_BIT + 4,
	TEX_COORD_SIZE_BIT = VERTEX_LAST_BIT + 7,

	VERTEX_BONE_WEIGHT_MASK = (0x7 << VERTEX_BONE_WEIGHT_BIT),
	USER_DATA_SIZE_MASK = (0x7 << USER_DATA_SIZE_BIT),

	VERTEX_FORMAT_FIELD_MASK = 0x0FF,

	// If everything is off, it's an unknown vertex format
	VERTEX_FORMAT_UNKNOWN = 0,
};

//-----------------------------------------------------------------------------
// Macros for construction..
//-----------------------------------------------------------------------------
#define VERTEX_BONEWEIGHT( _n )				((_n) << VERTEX_BONE_WEIGHT_BIT)
#define VERTEX_USERDATA_SIZE( _n )			((_n) << USER_DATA_SIZE_BIT)
#define VERTEX_TEXCOORD_MASK( _coord )		(( 0x7ULL ) << ( TEX_COORD_SIZE_BIT + 3 * (_coord) ))

inline VertexFormat_t VERTEX_TEXCOORD_SIZE(int nIndex, int nNumCoords)
{
	uint64 n64 = nNumCoords;
	uint64 nShift = TEX_COORD_SIZE_BIT + (3 * nIndex);
	return n64 << nShift;
}

//-----------------------------------------------------------------------------
// Gets at various vertex format info...
//-----------------------------------------------------------------------------
inline int VertexFlags(VertexFormat_t vertexFormat)
{
	return static_cast<int> (vertexFormat & ((1 << (VERTEX_LAST_BIT + 1)) - 1));
}

inline int NumBoneWeights(VertexFormat_t vertexFormat)
{
	return static_cast<int> ((vertexFormat >> VERTEX_BONE_WEIGHT_BIT) & 0x7);
}

inline int UserDataSize(VertexFormat_t vertexFormat)
{
	return static_cast<int> ((vertexFormat >> USER_DATA_SIZE_BIT) & 0x7);
}

inline int TexCoordSize(int nTexCoordIndex, VertexFormat_t vertexFormat)
{
	return static_cast<int> ((vertexFormat >> (TEX_COORD_SIZE_BIT + 3 * nTexCoordIndex)) & 0x7);
}


//-----------------------------------------------------------------------------
// VertexElement_t (enumerates all usable vertex elements)
//-----------------------------------------------------------------------------
// FIXME: unify this with VertexFormat_t (i.e. construct the lower bits of VertexFormat_t with "1 << (VertexElement_t)element")
enum VertexElement_t
{
	VERTEX_ELEMENT_NONE = -1,

	// Deliberately explicitly numbered so it's a pain in the ass to change, so you read this:
	// #!#!#NOTE#!#!# update GetVertexElementSize, VertexElementToDeclType and
	//                CVBAllocTracker (elementTable) when you update this!
	VERTEX_ELEMENT_POSITION = 0,
	VERTEX_ELEMENT_POSITION4D = 1,
	VERTEX_ELEMENT_NORMAL = 2,
	VERTEX_ELEMENT_NORMAL4D = 3,
	VERTEX_ELEMENT_COLOR = 4,
	VERTEX_ELEMENT_SPECULAR = 5,
	VERTEX_ELEMENT_TANGENT_S = 6,
	VERTEX_ELEMENT_TANGENT_T = 7,
	VERTEX_ELEMENT_WRINKLE = 8,
	VERTEX_ELEMENT_BONEINDEX = 9,
	VERTEX_ELEMENT_BONEWEIGHTS1 = 10,
	VERTEX_ELEMENT_BONEWEIGHTS2 = 11,
	VERTEX_ELEMENT_BONEWEIGHTS3 = 12,
	VERTEX_ELEMENT_BONEWEIGHTS4 = 13,
	VERTEX_ELEMENT_USERDATA1 = 14,
	VERTEX_ELEMENT_USERDATA2 = 15,
	VERTEX_ELEMENT_USERDATA3 = 16,
	VERTEX_ELEMENT_USERDATA4 = 17,
	VERTEX_ELEMENT_TEXCOORD1D_0 = 18,
	VERTEX_ELEMENT_TEXCOORD1D_1 = 19,
	VERTEX_ELEMENT_TEXCOORD1D_2 = 20,
	VERTEX_ELEMENT_TEXCOORD1D_3 = 21,
	VERTEX_ELEMENT_TEXCOORD1D_4 = 22,
	VERTEX_ELEMENT_TEXCOORD1D_5 = 23,
	VERTEX_ELEMENT_TEXCOORD1D_6 = 24,
	VERTEX_ELEMENT_TEXCOORD1D_7 = 25,
	VERTEX_ELEMENT_TEXCOORD2D_0 = 26,
	VERTEX_ELEMENT_TEXCOORD2D_1 = 27,
	VERTEX_ELEMENT_TEXCOORD2D_2 = 28,
	VERTEX_ELEMENT_TEXCOORD2D_3 = 29,
	VERTEX_ELEMENT_TEXCOORD2D_4 = 30,
	VERTEX_ELEMENT_TEXCOORD2D_5 = 31,
	VERTEX_ELEMENT_TEXCOORD2D_6 = 32,
	VERTEX_ELEMENT_TEXCOORD2D_7 = 33,
	VERTEX_ELEMENT_TEXCOORD3D_0 = 34,
	VERTEX_ELEMENT_TEXCOORD3D_1 = 35,
	VERTEX_ELEMENT_TEXCOORD3D_2 = 36,
	VERTEX_ELEMENT_TEXCOORD3D_3 = 37,
	VERTEX_ELEMENT_TEXCOORD3D_4 = 38,
	VERTEX_ELEMENT_TEXCOORD3D_5 = 39,
	VERTEX_ELEMENT_TEXCOORD3D_6 = 40,
	VERTEX_ELEMENT_TEXCOORD3D_7 = 41,
	VERTEX_ELEMENT_TEXCOORD4D_0 = 42,
	VERTEX_ELEMENT_TEXCOORD4D_1 = 43,
	VERTEX_ELEMENT_TEXCOORD4D_2 = 44,
	VERTEX_ELEMENT_TEXCOORD4D_3 = 45,
	VERTEX_ELEMENT_TEXCOORD4D_4 = 46,
	VERTEX_ELEMENT_TEXCOORD4D_5 = 47,
	VERTEX_ELEMENT_TEXCOORD4D_6 = 48,
	VERTEX_ELEMENT_TEXCOORD4D_7 = 49,

	VERTEX_ELEMENT_NUMELEMENTS = 50
};

inline void Detect_VertexElement_t_Changes(VertexElement_t element) // GREPs for VertexElement_t will hit this
{
	// Make it harder for someone to change VertexElement_t without noticing that dependent code
	// (GetVertexElementSize, VertexElementToDeclType, CVBAllocTracker) needs updating
	Assert(VERTEX_ELEMENT_NUMELEMENTS == 50);
	switch (element)
	{
	case VERTEX_ELEMENT_POSITION:		Assert(VERTEX_ELEMENT_POSITION == 0); break;
	case VERTEX_ELEMENT_POSITION4D:		Assert(VERTEX_ELEMENT_POSITION4D == 1); break;
	case VERTEX_ELEMENT_NORMAL:			Assert(VERTEX_ELEMENT_NORMAL == 2); break;
	case VERTEX_ELEMENT_NORMAL4D:		Assert(VERTEX_ELEMENT_NORMAL4D == 3); break;
	case VERTEX_ELEMENT_COLOR:			Assert(VERTEX_ELEMENT_COLOR == 4); break;
	case VERTEX_ELEMENT_SPECULAR:		Assert(VERTEX_ELEMENT_SPECULAR == 5); break;
	case VERTEX_ELEMENT_TANGENT_S:		Assert(VERTEX_ELEMENT_TANGENT_S == 6); break;
	case VERTEX_ELEMENT_TANGENT_T:		Assert(VERTEX_ELEMENT_TANGENT_T == 7); break;
	case VERTEX_ELEMENT_WRINKLE:		Assert(VERTEX_ELEMENT_WRINKLE == 8); break;
	case VERTEX_ELEMENT_BONEINDEX:		Assert(VERTEX_ELEMENT_BONEINDEX == 9); break;
	case VERTEX_ELEMENT_BONEWEIGHTS1:	Assert(VERTEX_ELEMENT_BONEWEIGHTS1 == 10); break;
	case VERTEX_ELEMENT_BONEWEIGHTS2:	Assert(VERTEX_ELEMENT_BONEWEIGHTS2 == 11); break;
	case VERTEX_ELEMENT_BONEWEIGHTS3:	Assert(VERTEX_ELEMENT_BONEWEIGHTS3 == 12); break;
	case VERTEX_ELEMENT_BONEWEIGHTS4:	Assert(VERTEX_ELEMENT_BONEWEIGHTS4 == 13); break;
	case VERTEX_ELEMENT_USERDATA1:		Assert(VERTEX_ELEMENT_USERDATA1 == 14); break;
	case VERTEX_ELEMENT_USERDATA2:		Assert(VERTEX_ELEMENT_USERDATA2 == 15); break;
	case VERTEX_ELEMENT_USERDATA3:		Assert(VERTEX_ELEMENT_USERDATA3 == 16); break;
	case VERTEX_ELEMENT_USERDATA4:		Assert(VERTEX_ELEMENT_USERDATA4 == 17); break;
	case VERTEX_ELEMENT_TEXCOORD1D_0:	Assert(VERTEX_ELEMENT_TEXCOORD1D_0 == 18); break;
	case VERTEX_ELEMENT_TEXCOORD1D_1:	Assert(VERTEX_ELEMENT_TEXCOORD1D_1 == 19); break;
	case VERTEX_ELEMENT_TEXCOORD1D_2:	Assert(VERTEX_ELEMENT_TEXCOORD1D_2 == 20); break;
	case VERTEX_ELEMENT_TEXCOORD1D_3:	Assert(VERTEX_ELEMENT_TEXCOORD1D_3 == 21); break;
	case VERTEX_ELEMENT_TEXCOORD1D_4:	Assert(VERTEX_ELEMENT_TEXCOORD1D_4 == 22); break;
	case VERTEX_ELEMENT_TEXCOORD1D_5:	Assert(VERTEX_ELEMENT_TEXCOORD1D_5 == 23); break;
	case VERTEX_ELEMENT_TEXCOORD1D_6:	Assert(VERTEX_ELEMENT_TEXCOORD1D_6 == 24); break;
	case VERTEX_ELEMENT_TEXCOORD1D_7:	Assert(VERTEX_ELEMENT_TEXCOORD1D_7 == 25); break;
	case VERTEX_ELEMENT_TEXCOORD2D_0:	Assert(VERTEX_ELEMENT_TEXCOORD2D_0 == 26); break;
	case VERTEX_ELEMENT_TEXCOORD2D_1:	Assert(VERTEX_ELEMENT_TEXCOORD2D_1 == 27); break;
	case VERTEX_ELEMENT_TEXCOORD2D_2:	Assert(VERTEX_ELEMENT_TEXCOORD2D_2 == 28); break;
	case VERTEX_ELEMENT_TEXCOORD2D_3:	Assert(VERTEX_ELEMENT_TEXCOORD2D_3 == 29); break;
	case VERTEX_ELEMENT_TEXCOORD2D_4:	Assert(VERTEX_ELEMENT_TEXCOORD2D_4 == 30); break;
	case VERTEX_ELEMENT_TEXCOORD2D_5:	Assert(VERTEX_ELEMENT_TEXCOORD2D_5 == 31); break;
	case VERTEX_ELEMENT_TEXCOORD2D_6:	Assert(VERTEX_ELEMENT_TEXCOORD2D_6 == 32); break;
	case VERTEX_ELEMENT_TEXCOORD2D_7:	Assert(VERTEX_ELEMENT_TEXCOORD2D_7 == 33); break;
	case VERTEX_ELEMENT_TEXCOORD3D_0:	Assert(VERTEX_ELEMENT_TEXCOORD3D_0 == 34); break;
	case VERTEX_ELEMENT_TEXCOORD3D_1:	Assert(VERTEX_ELEMENT_TEXCOORD3D_1 == 35); break;
	case VERTEX_ELEMENT_TEXCOORD3D_2:	Assert(VERTEX_ELEMENT_TEXCOORD3D_2 == 36); break;
	case VERTEX_ELEMENT_TEXCOORD3D_3:	Assert(VERTEX_ELEMENT_TEXCOORD3D_3 == 37); break;
	case VERTEX_ELEMENT_TEXCOORD3D_4:	Assert(VERTEX_ELEMENT_TEXCOORD3D_4 == 38); break;
	case VERTEX_ELEMENT_TEXCOORD3D_5:	Assert(VERTEX_ELEMENT_TEXCOORD3D_5 == 39); break;
	case VERTEX_ELEMENT_TEXCOORD3D_6:	Assert(VERTEX_ELEMENT_TEXCOORD3D_6 == 40); break;
	case VERTEX_ELEMENT_TEXCOORD3D_7:	Assert(VERTEX_ELEMENT_TEXCOORD3D_7 == 41); break;
	case VERTEX_ELEMENT_TEXCOORD4D_0:	Assert(VERTEX_ELEMENT_TEXCOORD4D_0 == 42); break;
	case VERTEX_ELEMENT_TEXCOORD4D_1:	Assert(VERTEX_ELEMENT_TEXCOORD4D_1 == 43); break;
	case VERTEX_ELEMENT_TEXCOORD4D_2:	Assert(VERTEX_ELEMENT_TEXCOORD4D_2 == 44); break;
	case VERTEX_ELEMENT_TEXCOORD4D_3:	Assert(VERTEX_ELEMENT_TEXCOORD4D_3 == 45); break;
	case VERTEX_ELEMENT_TEXCOORD4D_4:	Assert(VERTEX_ELEMENT_TEXCOORD4D_4 == 46); break;
	case VERTEX_ELEMENT_TEXCOORD4D_5:	Assert(VERTEX_ELEMENT_TEXCOORD4D_5 == 47); break;
	case VERTEX_ELEMENT_TEXCOORD4D_6:	Assert(VERTEX_ELEMENT_TEXCOORD4D_6 == 48); break;
	case VERTEX_ELEMENT_TEXCOORD4D_7:	Assert(VERTEX_ELEMENT_TEXCOORD4D_7 == 49); break;
	default:
		Assert(0); // Invalid input or VertexElement_t has definitely changed
		break;
	}
}

// We're testing 2 normal compression methods
// One compressed normals+tangents into a SHORT2 each (8 bytes total)
// The other compresses them together, into a single UBYTE4 (4 bytes total)
// FIXME: pick one or the other, compare lighting quality in important cases
#define COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2	0
#define COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4	1
//#define COMPRESSED_NORMALS_TYPE						COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2
#define COMPRESSED_NORMALS_TYPE						COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4


//-----------------------------------------------------------------------------
// Shader state flags can be read from the FLAGS materialvar
// Also can be read or written to with the Set/GetMaterialVarFlags() call
// Also make sure you add/remove a string associated with each flag below to CShaderSystem::ShaderStateString in ShaderSystem.cpp
//-----------------------------------------------------------------------------
enum MaterialVarFlags_t
{
	MATERIAL_VAR_DEBUG = (1 << 0),
	MATERIAL_VAR_NO_DEBUG_OVERRIDE = (1 << 1),
	MATERIAL_VAR_NO_DRAW = (1 << 2),
	MATERIAL_VAR_USE_IN_FILLRATE_MODE = (1 << 3),

	MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA = (1 << 5),
	MATERIAL_VAR_SELFILLUM = (1 << 6),
	MATERIAL_VAR_ADDITIVE = (1 << 7),
	MATERIAL_VAR_ALPHATEST = (1 << 8),
	//	MATERIAL_VAR_UNUSED					  = (1 << 9),
	MATERIAL_VAR_ZNEARER = (1 << 10),
	MATERIAL_VAR_MODEL = (1 << 11),
	MATERIAL_VAR_FLAT = (1 << 12),
	MATERIAL_VAR_NOCULL = (1 << 13),
	MATERIAL_VAR_NOFOG = (1 << 14),
	MATERIAL_VAR_IGNOREZ = (1 << 15),
	MATERIAL_VAR_DECAL = (1 << 16),
	MATERIAL_VAR_ENVMAPSPHERE = (1 << 17), // OBSOLETE
										   //	MATERIAL_VAR_UNUSED					  = (1 << 18),
										   MATERIAL_VAR_ENVMAPCAMERASPACE = (1 << 19), // OBSOLETE
										   MATERIAL_VAR_BASEALPHAENVMAPMASK = (1 << 20),
										   MATERIAL_VAR_TRANSLUCENT = (1 << 21),
										   MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
										   MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = (1 << 23), // OBSOLETE
										   MATERIAL_VAR_OPAQUETEXTURE = (1 << 24),
										   MATERIAL_VAR_ENVMAPMODE = (1 << 25), // OBSOLETE
										   MATERIAL_VAR_SUPPRESS_DECALS = (1 << 26),
										   MATERIAL_VAR_HALFLAMBERT = (1 << 27),
										   MATERIAL_VAR_WIREFRAME = (1 << 28),
										   MATERIAL_VAR_ALLOWALPHATOCOVERAGE = (1 << 29),
										   MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY = (1 << 30),
										   MATERIAL_VAR_VERTEXFOG = (1 << 31),

										   // NOTE: Only add flags here that either should be read from
										   // .vmts or can be set directly from client code. Other, internal
										   // flags should to into the flag enum in IMaterialInternal.h
};


//-----------------------------------------------------------------------------
// Internal flags not accessible from outside the material system. Stored in Flags2
//-----------------------------------------------------------------------------
enum MaterialVarFlags2_t
{
	// NOTE: These are for $flags2!!!!!
	//	UNUSED											= (1 << 0),

	MATERIAL_VAR2_LIGHTING_UNLIT = 0,
	MATERIAL_VAR2_LIGHTING_VERTEX_LIT = (1 << 1),
	MATERIAL_VAR2_LIGHTING_LIGHTMAP = (1 << 2),
	MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP = (1 << 3),
	MATERIAL_VAR2_LIGHTING_MASK =
	(MATERIAL_VAR2_LIGHTING_VERTEX_LIT |
		MATERIAL_VAR2_LIGHTING_LIGHTMAP |
		MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP),

	// FIXME: Should this be a part of the above lighting enums?
	MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL = (1 << 4),
	MATERIAL_VAR2_USES_ENV_CUBEMAP = (1 << 5),
	MATERIAL_VAR2_NEEDS_TANGENT_SPACES = (1 << 6),
	MATERIAL_VAR2_NEEDS_SOFTWARE_LIGHTING = (1 << 7),
	// GR - HDR path puts lightmap alpha in separate texture...
	MATERIAL_VAR2_BLEND_WITH_LIGHTMAP_ALPHA = (1 << 8),
	MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS = (1 << 9),
	MATERIAL_VAR2_USE_FLASHLIGHT = (1 << 10),
	MATERIAL_VAR2_USE_FIXED_FUNCTION_BAKED_LIGHTING = (1 << 11),
	MATERIAL_VAR2_NEEDS_FIXED_FUNCTION_FLASHLIGHT = (1 << 12),
	MATERIAL_VAR2_USE_EDITOR = (1 << 13),
	MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE = (1 << 14),
	MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE = (1 << 15),
	MATERIAL_VAR2_IS_SPRITECARD = (1 << 16),
	MATERIAL_VAR2_USES_VERTEXID = (1 << 17),
	MATERIAL_VAR2_SUPPORTS_HW_SKINNING = (1 << 18),
	MATERIAL_VAR2_SUPPORTS_FLASHLIGHT = (1 << 19),
	MATERIAL_VAR2_USE_GBUFFER0 = (1 << 20),
	MATERIAL_VAR2_USE_GBUFFER1 = (1 << 21),
	MATERIAL_VAR2_SELFILLUMMASK = (1 << 22),
	MATERIAL_VAR2_SUPPORTS_TESSELLATION = (1 << 23)
};


//-----------------------------------------------------------------------------
// Preview image return values
//-----------------------------------------------------------------------------
enum PreviewImageRetVal_t
{
	MATERIAL_PREVIEW_IMAGE_BAD = 0,
	MATERIAL_PREVIEW_IMAGE_OK,
	MATERIAL_NO_PREVIEW_IMAGE,
};


//-----------------------------------------------------------------------------
// material interface
//-----------------------------------------------------------------------------
class IMaterial
{
public:
	// Get the name of the material.  This is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	virtual const char* GetName() const = 0;

	virtual const char* GetTextureGroupName() const = 0;

	// Get the preferred size/bitDepth of a preview image of a material.
	// This is the sort of image that you would use for a thumbnail view
	// of a material, or in WorldCraft until it uses materials to render.
	// separate this for the tools maybe
	virtual PreviewImageRetVal_t GetPreviewImageProperties(int* width, int* height, ImageFormat* imageFormat, bool* isTranslucent) const = 0;

	// Get a preview image at the specified width/height and bitDepth.
	// Will do resampling if necessary.(not yet!!! :) )
	// Will do color format conversion. (works now.)
	virtual PreviewImageRetVal_t GetPreviewImage(unsigned char* data, int width, int height, ImageFormat imageFormat) const = 0;

	//
	virtual int GetMappingWidth() = 0;

	virtual int GetMappingHeight() = 0;

	virtual int GetNumAnimationFrames() = 0;

	// For material subrects (material pages).  Offset(u,v) and scale(u,v) are normalized to texture.
	virtual bool InMaterialPage(void) = 0;

	virtual void GetMaterialOffset(float* pOffset) = 0;

	virtual void GetMaterialScale(float* pScale) = 0;

	virtual IMaterial* GetMaterialPage(void) = 0;

	// find a vmt variable.
	// This is how game code affects how a material is rendered.
	// The game code must know about the params that are used by
	// the shader for the material that it is trying to affect.
	virtual IMaterialVar* FindVar(const char* varName, bool* found, bool complain = true) = 0;

	// The user never allocates or deallocates materials.  Reference counting is
	// used instead.  Garbage collection is done upon a call to 
	// IMaterialSystem::UncacheUnusedMaterials.
	virtual void IncrementReferenceCount(void) = 0;

	virtual void DecrementReferenceCount(void) = 0;

	inline void AddRef()
	{
		IncrementReferenceCount();
	}

	inline void Release()
	{
		DecrementReferenceCount();
	}

	// Each material is assigned a number that groups it with like materials
	// for sorting in the application.
	virtual int GetEnumerationID(void) const = 0;

	virtual void GetLowResColorSample(float s, float t, float* color) const = 0;

	// This computes the state snapshots for this material
	virtual void RecomputeStateSnapshots() = 0;

	// Are we translucent?
	virtual bool IsTranslucent() = 0;

	// Are we alphatested?
	virtual bool IsAlphaTested() = 0;

	// Are we vertex lit?
	virtual bool IsVertexLit() = 0;

	// Gets the vertex format
	virtual VertexFormat_t GetVertexFormat() const = 0;

	// returns true if this material uses a material proxy
	virtual bool HasProxy(void) const = 0;

	virtual bool UsesEnvCubemap(void) = 0;

	virtual bool NeedsTangentSpace(void) = 0;

	virtual bool NeedsPowerOfTwoFrameBufferTexture(bool bCheckSpecificToThisFrame = true) = 0;

	virtual bool NeedsFullFrameBufferTexture(bool bCheckSpecificToThisFrame = true) = 0;

	// returns true if the shader doesn't do skinning itself and requires
	// the data that is sent to it to be preskinned.
	virtual bool NeedsSoftwareSkinning(void) = 0;

	// Apply constant color or alpha modulation
	virtual void AlphaModulate(float alpha) = 0;

	virtual void ColorModulate(float r, float g, float b) = 0;

	// Material Var flags...
	virtual void SetMaterialVarFlag(MaterialVarFlags_t flag, bool on) = 0;

	virtual bool GetMaterialVarFlag(MaterialVarFlags_t flag) const = 0;

	// Gets material reflectivity
	virtual void GetReflectivity(Vector& reflect) = 0;

	// Gets material property flags
	virtual bool GetPropertyFlag(MaterialPropertyTypes_t type) = 0;

	// Is the material visible from both sides?
	virtual bool IsTwoSided() = 0;

	// Sets the shader associated with the material
	virtual void SetShader(const char* pShaderName) = 0;

	// Can't be const because the material might have to precache itself.
	virtual int GetNumPasses(void) = 0;

	// Can't be const because the material might have to precache itself.
	virtual int GetTextureMemoryBytes(void) = 0;

	// Meant to be used with materials created using CreateMaterial
	// It updates the materials to reflect the current values stored in the material vars
	virtual void Refresh() = 0;

	// GR - returns true is material uses lightmap alpha for blending
	virtual bool NeedsLightmapBlendAlpha(void) = 0;

	// returns true if the shader doesn't do lighting itself and requires
	// the data that is sent to it to be prelighted
	virtual bool NeedsSoftwareLighting(void) = 0;

	// Gets at the shader parameters
	virtual int ShaderParamCount() const = 0;

	virtual IMaterialVar** GetShaderParams(void) = 0;

	// Returns true if this is the error material you get back from IMaterialSystem::FindMaterial if
	// the material can't be found.
	virtual bool IsErrorMaterial() const = 0;

	virtual void Unused() = 0;

	// Gets the current alpha modulation
	virtual float GetAlphaModulation() = 0;

	virtual void GetColorModulation(float* r, float* g, float* b) = 0;

	// Is this translucent given a particular alpha modulation?
	virtual bool IsTranslucentUnderModulation(float fAlphaModulation = 1.0f) const = 0;

	// fast find that stores the index of the found var in the string table in local cache
	virtual IMaterialVar* FindVarFast(char const* pVarName, unsigned int* pToken) = 0;

	// Sets new VMT shader parameters for the material
	virtual void SetShaderAndParams(KeyValues* pKeyValues) = 0;

	virtual const char* GetShaderName() const = 0;

	virtual void DeleteIfUnreferenced() = 0;

	virtual bool IsSpriteCard() = 0;

	virtual void CallBindProxy(void* proxyData) = 0;

	virtual void RefreshPreservingMaterialVars() = 0;

	virtual bool WasReloadedFromWhitelist() = 0;

	virtual bool SetTempExcluded(bool bSet, int nExcludedDimensionLimit) = 0;

	virtual int GetReferenceCount() const = 0;
};

extern IMaterial* mat_vertex_in;
extern IMaterial* mat_vertex_out;
extern IMaterial* mat_unlit_in;
extern IMaterial* mat_unlit_out;
extern IMaterial* mat_outline;


inline bool IsErrorMaterial(IMaterial *pMat)
{
	return !pMat; //|| pMat->IsErrorMaterial();
}


//
// Vertex stream specifications
//

struct VertexStreamSpec_t
{
	enum StreamSpec_t
	{
		STREAM_DEFAULT,		// stream 0: with position, normal, etc.
		STREAM_SPECULAR1,	// stream 1: following specular vhv lighting
		STREAM_FLEXDELTA,	// stream 2: flex deltas
		STREAM_MORPH,		// stream 3: morph
		STREAM_UNIQUE_A,	// unique stream 4
		STREAM_UNIQUE_B,	// unique stream 5
		STREAM_UNIQUE_C,	// unique stream 6
		STREAM_UNIQUE_D,	// unique stream 7
		STREAM_SUBDQUADS,	// stream 8: quad buffer for subd's
	};

	enum
	{
		MAX_UNIQUE_STREAMS = 4
	};

	VertexFormatFlags_t iVertexDataElement;
	StreamSpec_t iStreamSpec;
};

inline VertexStreamSpec_t * FindVertexStreamSpec(VertexFormat_t iElement, VertexStreamSpec_t *arrVertexStreamSpec)
{
	for (; arrVertexStreamSpec &&
		arrVertexStreamSpec->iVertexDataElement != VERTEX_FORMAT_UNKNOWN;
		++arrVertexStreamSpec)
	{
		if (arrVertexStreamSpec->iVertexDataElement == iElement)
			return arrVertexStreamSpec;
	}
	return NULL;
}

struct mstudioposeparamdesc_t {
	int sznameindex;
	inline char* const pszName(void) const { return ((char*)this) + sznameindex; }
	int flags;   // ???? (  nice comment volvo )
	float start; // starting value
	float end;   // ending value
	float loop;  // looping range, 0 for no looping, 360 for rotations, etc.
};

typedef unsigned short MaterialHandle_t;

class IMaterialSystem
{
public:
	IMaterial * CreateMaterial(bool flat, bool ignorez, bool wireframed);
	IMaterial* FindMaterial(char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL);
	MaterialHandle_t FirstMaterial();
	MaterialHandle_t NextMaterial(MaterialHandle_t h);
	MaterialHandle_t InvalidMaterial();
	IMaterial * GetMaterial(MaterialHandle_t h);
};

class CCSGO_HudDeathNotice
{
public:
	char _0x0000[68];
	void* deathnotices; //0x0044 
	char _0x0048[4];
	float lifetime; //0x004C 
	float localplayer_lifetime_mod; //0x0050 
	float fade_out_time; //0x0054 
};

enum ECstrike15UserMessages
{
	CS_UM_VGUIMenu = 1,
	CS_UM_Geiger = 2,
	CS_UM_Train = 3,
	CS_UM_HudText = 4,
	CS_UM_SayText = 5,
	CS_UM_SayText2 = 6,
	CS_UM_TextMsg = 7,
	CS_UM_HudMsg = 8,
	CS_UM_ResetHud = 9,
	CS_UM_GameTitle = 10,
	CS_UM_Shake = 12,
	CS_UM_Fade = 13,			// fade HUD in/out
	CS_UM_Rumble = 14,
	CS_UM_CloseCaption = 15,
	CS_UM_CloseCaptionDirect = 16,
	CS_UM_SendAudio = 17,
	CS_UM_RawAudio = 18,
	CS_UM_VoiceMask = 19,
	CS_UM_RequestState = 20,
	CS_UM_Damage = 21,
	CS_UM_RadioText = 22,
	CS_UM_HintText = 23,
	CS_UM_KeyHintText = 24,
	CS_UM_ProcessSpottedEntityUpdate = 25,
	CS_UM_ReloadEffect = 26,
	CS_UM_AdjustMoney = 27,
	CS_UM_UpdateTeamMoney = 28,
	CS_UM_StopSpectatorMode = 29,
	CS_UM_KillCam = 30,
	CS_UM_DesiredTimescale = 31,
	CS_UM_CurrentTimescale = 32,
	CS_UM_AchievementEvent = 33,
	CS_UM_MatchEndConditions = 34,
	CS_UM_DisconnectToLobby = 35,
	CS_UM_PlayerStatsUpdate = 36,
	CS_UM_DisplayInventory = 37,
	CS_UM_WarmupHasEnded = 38,
	CS_UM_ClientInfo = 39,
	CS_UM_XRankGet = 40,					// Get ELO Rank Value from Client
	CS_UM_XRankUpd = 41,					// Update ELO Rank Value on Client
	CS_UM_SetPlayerEloDisplayBracket = 42,	// Sets the elo bracket to be displayed for the local user
	CS_UM_RequestEloBracketInfo = 43,		// Ask the client to send up the elo bracket info stored in title data.
	CS_UM_SetEloBracketInfo = 44,			// Server setting the elo bracket info for a client.
	CS_UM_CallVoteFailed = 45,
	CS_UM_VoteStart = 46,
	CS_UM_VotePass = 47,
	CS_UM_VoteFailed = 48,
	CS_UM_VoteSetup = 49,
	CS_UM_ServerRankRevealAll = 50,
	CS_UM_SendLastKillerDamageToClient = 51,
	CS_UM_ServerRankUpdate = 52,
	CS_UM_ItemPickup = 53,
	CS_UM_ShowMenu = 54,					// show hud menu
	CS_UM_BarTime = 55,						// For the C4 progress bar.
	CS_UM_AmmoDenied = 56,
	CS_UM_MarkAchievement = 57,
	CS_UM_MatchStatsUpdate = 58,
	CS_UM_ItemDrop = 59
};

class CCSUsrMsg_VGUIMenu
{ public:
	const char* name; //=1;
	bool show; //=2;

	class Subkey
	{ public:
		const char* name; //=1;
		const char* str; //=2;
	};

	Subkey subkeys; //=3;
};

class CCSUsrMsg_Geiger
{ public:
	int range; //=1;
};

class CCSUsrMsg_Train
{ public:
	int train; //=1;
};

class CCSUsrMsg_HudText
{ public:
	const char* text; //=1;
};

class CCSUsrMsg_SayText
{ public:
	int ent_idx; //=1;
	const char* text; //=2;
	bool chat; //=3;
};

class CCSUsrMsg_SayText2
{ public:
	int ent_idx; //=1;
	bool chat; //=2;
	const char* msg_name; //=3;
	const char* params; //=4;
};

class CCSUsrMsg_TextMsg
{ public:
	int msg_dst; //=1;
	const char* params; //=3;
};

class CCSUsrMsg_HudMsg
{ public:
	int channel; //=1;
	Vector2D pos; //=2;
	Color clr1; //=3;
	Color clr2; //=4;
	int effect; //=5;
	float fade_in_time; //=6;
	float fade_out_time; //=7;
	float hold_time; //=9;
	float fx_time; //=10;
	const char* text; //=11;
};

class CCSUsrMsg_Shake
{ public:
	int command; //=1;
	float local_amplitude; //=2;
	float frequency; //=3;
	float duration; //=4;
};

class CCSUsrMsg_Fade
{ public:
	int duration; //=1;
	int hold_time; //=2;
	int flags; //=3;		// fade type (in / out)
	Color clr; //=4;
};

class CCSUsrMsg_Rumble
{ public:
	int index; //=1;
	int data; //=2;
	int flags; //=3;
};

class CCSUsrMsg_CloseCaption
{ public:
	uint32_t hash; //=1;
	int duration; //=2;
	bool from_player; //=3;
};

class CCSUsrMsg_CloseCaptionDirect
{ public:
	uint32_t hash; //=1;
	int duration; //=2;
	bool from_player; //=3;
};

class CCSUsrMsg_SendAudio
{ public:
	const char* radio_sound; //=1;
};

class CCSUsrMsg_RawAudio
{ public:
	int pitch; //=1;
	int entidx; //=2;
	float duration; //=3;
	const char* voice_filename; //=4;
};

class CCSUsrMsg_VoiceMask
{ public:
	class PlayerMask
	{ public:
		int game_rules_mask; //=1;
		int ban_masks; //=2;
	};

	PlayerMask player_masks; //=1;
	bool player_mod_enable; //=2;
};

class CCSUsrMsg_Damage
{ public:
	int amount; //=1;
	Vector inflictor_world_pos; //=2;
};

class CCSUsrMsg_RadioText
{ public:
	int msg_dst; //=1;
	int client; //=2;
	const char* msg_name; //=3;
	const char* params; //=4;
};

class CCSUsrMsg_HintText
{ public:
	const char* text; //=1;
};

class CCSUsrMsg_KeyHintText
{ public:
	const char* hints; //=1;
};

// gurjeets - Message below is slightly bigger in size than the non-protobuf version,
// by around 8 bits. 
class CCSUsrMsg_ProcessSpottedEntityUpdate
{ public:
	bool new_update; //=1;

	class SpottedEntityUpdate
	{ public:
		int entity_idx; //=1;
		int class_id; //=2;
		int origin_x; //=3;
		int origin_y; //=4;
		int origin_z; //=5;
		int angle_y; //=6;
		bool defuser; //=7;
		bool player_has_defuser; //=8;
		bool player_has_c4; //=9;
	};

	SpottedEntityUpdate entity_updates; //=2;
};

class CCSUsrMsg_ReloadEffect
{ public:
	int entidx; //=1;
};

class CCSUsrMsg_AdjustMoney
{ public:
	int amount; //=1;
};

class CCSUsrMsg_KillCam
{ public:
	int obs_mode; //=1;
	int first_target; //=2;
	int second_target; //=3;
};

class CCSUsrMsg_DesiredTimescale
{ public:
	float desired_timescale; //=1;
	float duration_realtime_sec; //=2;
	int interpolator_type; //=3;
	float start_blend_time; //=4;
};

class CCSUsrMsg_CurrentTimescale
{ public:
	float cur_timescale; //=1;
};

class CCSUsrMsg_AchievementEvent
{ public:
	int achievement; //=1;
	int count; //=2;
	int user_id; //=3;
};


class CCSUsrMsg_MatchEndConditions
{ public:
	int fraglimit; //=1;
	int mp_maxrounds; //=2;
	int mp_winlimit; //=3;
	int mp_timelimit; //=4;
};

class CCSUsrMsg_PlayerStatsUpdate
{ public:
	int version; //=1;
	int official_server; //=2;

	class Stat
	{ public:
		int idx; //=1;
		int delta; //=2;
	};

	Stat stats; //=4;
	int user_id; //=5;
	int crc; //=6;
};

class CCSUsrMsg_DisplayInventory
{ public:
	bool display; //=1;
};

class CCSUsrMsg_XRankGet
{ public:
	int mode_idx; //=1;
	int controller; //=2;
};

class CCSUsrMsg_XRankUpd
{ public:
	int mode_idx; //=1;
	int controller; //=2;
	int ranking; //=3;
};

class CCSUsrMsg_SetPlayerEloDisplayBracket
{ public:
	int bracket; //=1;
};

class CCSUsrMsg_RequestEloBracketInfo
{ public:
	int bracket; //=1;
};

class CCSUsrMsg_SetEloBracketInfo
{ public:
	int game_mode; //=1;
	int display_bracket; //=2;
	int prev_bracket; //=3;
	int num_games_in_bracket; //=4;
};

class CCSUsrMsg_CallVoteFailed
{ public:
	int reason; //=1;
	int time; //=2;
};

class CCSUsrMsg_VoteStart
{ public:
	int team; //=1;
	int ent_idx; //=2;
	int vote_type; //=3;
	const char* disp_str; //=4;
	const char* details_str; //=5;
	const char* other_team_str; //=6;
	bool is_yes_no_vote; //=7;

};

class CCSUsrMsg_VotePass
{ public:
	int team; //=1;
	int vote_type; //=2;
	const char* disp_str; //=3;
	const char* details_str; //=4;
};

class CCSUsrMsg_VoteFailed
{ public:
	int team; //=1;
	int reason; //=2;
};

class CCSUsrMsg_VoteSetup
{ public:
	const char* potential_issues; //=1;
};

class CCSUsrMsg_SendLastKillerDamageToClient
{ public:
	int num_hits_given; //=1;
	int damage_given; //=2;
	int num_hits_taken; //=3;
	int damage_taken; //=4;
};

class CCSUsrMsg_ServerRankUpdate
{ public:
	class RankUpdate
	{ public:
		int account_id; //=1;
		int rank_old; //=2;
		int rank_new; //=3;
		int num_wins; //=4;
		float rank_change; //=5;
	};

	RankUpdate rank_update;
};

class CCSUsrMsg_ItemPickup
{ public:
	const char* item; //=1;
};

class CCSUsrMsg_ShowMenu
{ public:
	int bits_valid_slots; //=1;
	int display_time; //=2;
	const char* menu_string; //=3;
};

class CCSUsrMsg_BarTime
{ public:
	const char* time; //=1;
};

class CCSUsrMsg_AmmoDenied
{ public:
	int ammoIdx; //=1;
};

class CCSUsrMsg_MarkAchievement
{ public:
	const char* achievement; //=1;
};

class CCSUsrMsg_MatchStatsUpdate
{ public:
	const char* update; //=1;
};

class CCSUsrMsg_ItemDrop
{ public:
	int itemid; //=1;
	bool death; //=2;
};

#pragma once

class C_Beam;
class Beam_t;

//---------------------------------------------------------------------------�
// Purpose: Popcorn trail for Beam Follow rendering...
//---------------------------------------------------------------------------�
enum
{
	FBEAM_STARTENTITY = 0x00000001,
	FBEAM_ENDENTITY = 0x00000002,
	FBEAM_FADEIN = 0x00000004,
	FBEAM_FADEOUT = 0x00000008,
	FBEAM_SINENOISE = 0x00000010,
	FBEAM_SOLID = 0x00000020,
	FBEAM_SHADEIN = 0x00000040,
	FBEAM_SHADEOUT = 0x00000080,
	FBEAM_ONLYNOISEONCE = 0x00000100, // Only calculate our noise once
	FBEAM_NOTILE = 0x00000200,
	FBEAM_USE_HITBOXES = 0x00000400, // Attachment indices represent hitbox indices instead when this is set.
	FBEAM_STARTVISIBLE = 0x00000800, // Has this client actually seen this beam's start entity yet?
	FBEAM_ENDVISIBLE = 0x00001000, // Has this client actually seen this beam's end entity yet?
	FBEAM_ISACTIVE = 0x00002000,
	FBEAM_FOREVER = 0x00004000,
	FBEAM_HALOBEAM = 0x00008000, // When drawing a beam with a halo, don't ignore the segments and endwidth
	FBEAM_REVERSED = 0x00010000,
	NUM_BEAM_FLAGS = 17 // KEEP THIS UPDATED!
};
enum
{
	TE_BEAMPOINTS = 0x00, // beam effect between two points
	TE_SPRITE = 0x01, // additive sprite, plays 1 cycle
	TE_BEAMDISK = 0x02, // disk that expands to max radius over lifetime
	TE_BEAMCYLINDER = 0x03, // cylinder that expands to max radius over lifetime
	TE_BEAMFOLLOW = 0x04, // create a line of decaying beam segments until entity stops moving
	TE_BEAMRING = 0x05, // connect a beam ring to two entities
	TE_BEAMSPLINE = 0x06,
	TE_BEAMRINGPOINT = 0x07,
	TE_BEAMLASER = 0x08, // Fades according to viewpoint
	TE_BEAMTESLA = 0x09,
};

struct BeamTrail_t
{
	// NOTE: Don't add user defined fields except after these four fields.
	BeamTrail_t* next;
	float die;
	Vector org;
	Vector vel;
};

struct BeamInfo_t
{
	int m_nType;

	// Entities
	C_BasePlayer* m_pStartEnt;
	int m_nStartAttachment;
	C_BasePlayer* m_pEndEnt;
	int m_nEndAttachment;

	// Points
	Vector m_vecStart;
	Vector m_vecEnd;

	int m_nModelIndex;
	const char *m_pszModelName;

	int m_nHaloIndex;
	const char *m_pszHaloName;
	float m_flHaloScale;

	float m_flLife;
	float m_flWidth;
	float m_flEndWidth;
	float m_flFadeLength;
	float m_flAmplitude;

	float m_flBrightness;
	float m_flSpeed;

	int m_nStartFrame;
	float m_flFrameRate;

	float m_flRed;
	float m_flGreen;
	float m_flBlue;

	bool m_bRenderable;

	int m_nSegments;

	int m_nFlags;

	// Rings
	Vector m_vecCenter;
	float m_flStartRadius;
	float m_flEndRadius;

	BeamInfo_t()
	{
		m_nType = TE_BEAMPOINTS;
		m_nSegments = -1;
		m_pszModelName = NULL;
		m_pszHaloName = NULL;
		m_nModelIndex = -1;
		m_nHaloIndex = -1;
		m_bRenderable = true;
		m_nFlags = 0;
	}
};

class IRecipientFilter
{
public:
	virtual			~IRecipientFilter() {}

	virtual bool	IsReliable(void) const = 0;
	virtual bool	IsInitMessage(void) const = 0;

	virtual int		GetRecipientCount(void) const = 0;
	virtual int		GetRecipientIndex(int slot) const = 0;
};

//-----------------------------------------------------------------------------
#define	PITCH_NORM		100			// non-pitch shifted
#define PITCH_LOW		95			// other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH		120
//-----------------------------------------------------------------------------

enum soundlevel_t
{
	SNDLVL_NONE = 0,

	SNDLVL_20dB = 20,			// rustling leaves
	SNDLVL_25dB = 25,			// whispering
	SNDLVL_30dB = 30,			// library
	SNDLVL_35dB = 35,
	SNDLVL_40dB = 40,
	SNDLVL_45dB = 45,			// refrigerator

	SNDLVL_50dB = 50,	// 3.9	// average home
	SNDLVL_55dB = 55,	// 3.0

	SNDLVL_IDLE = 60,	// 2.0	
	SNDLVL_60dB = 60,	// 2.0	// normal conversation, clothes dryer

	SNDLVL_65dB = 65,	// 1.5	// washing machine, dishwasher
	SNDLVL_STATIC = 66,	// 1.25

	SNDLVL_70dB = 70,	// 1.0	// car, vacuum cleaner, mixer, electric sewing machine

	SNDLVL_NORM = 75,
	SNDLVL_75dB = 75,	// 0.8	// busy traffic

	SNDLVL_80dB = 80,	// 0.7	// mini-bike, alarm clock, noisy restaurant, office tabulator, outboard motor, passing snowmobile
	SNDLVL_TALKING = 80,	// 0.7
	SNDLVL_85dB = 85,	// 0.6	// average factory, electric shaver
	SNDLVL_90dB = 90,	// 0.5	// screaming child, passing motorcycle, convertible ride on frw
	SNDLVL_95dB = 95,
	SNDLVL_100dB = 100,	// 0.4	// subway train, diesel truck, woodworking shop, pneumatic drill, boiler shop, jackhammer
	SNDLVL_105dB = 105,			// helicopter, power mower
	SNDLVL_110dB = 110,			// snowmobile drvrs seat, inboard motorboat, sandblasting
	SNDLVL_120dB = 120,			// auto horn, propeller aircraft
	SNDLVL_130dB = 130,			// air raid siren

	SNDLVL_GUNFIRE = 140,	// 0.27	// THRESHOLD OF PAIN, gunshot, jet engine
	SNDLVL_140dB = 140,	// 0.2

	SNDLVL_150dB = 150,	// 0.2

	SNDLVL_180dB = 180,			// rocket launching

								// NOTE: Valid soundlevel_t values are 0-255.
								//       256-511 are reserved for sounds using goldsrc compatibility attenuation.
};


//class IEngineSound
//{
//public:
//	// Precache a particular sample
//	virtual bool PrecacheSound(const char *pSample, bool bPreload = false, bool bIsUISound = false) = 0;
//	virtual bool IsSoundPrecached(const char *pSample) = 0;
//	virtual void PrefetchSound(const char *pSample) = 0;
//	virtual bool IsLoopingSound(const char *pSample) = 0;
//
//	// Just loads the file header and checks for duration (not hooked up for .mp3's yet)
//	// Is accessible to server and client though
//	virtual float GetSoundDuration(const char *pSample) = 0;
//
//	// Pitch of 100 is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
//	// down to 1 is a lower pitch.   150 to 70 is the realistic range.
//	// EmitSound with pitch != 100 should be used sparingly, as it's not quite as
//	// fast (the pitchshift mixer is not native coded).
//
//	// NOTE: setting iEntIndex to -1 will cause the sound to be emitted from the local
//	// player (client-side only)
//	virtual int EmitSound(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample,
//		float flVolume, float flAttenuation, int nSeed, int iFlags = 0, int iPitch = PITCH_NORM,
//		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
//
//	virtual int EmitSound(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample,
//		float flVolume, soundlevel_t iSoundlevel, int nSeed, int iFlags = 0, int iPitch = PITCH_NORM,
//		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
//
//	virtual void EmitSentenceByIndex(IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex,
//		float flVolume, soundlevel_t iSoundlevel, int nSeed, int iFlags = 0, int iPitch = PITCH_NORM,
//		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
//
//	virtual void StopSound(int iEntIndex, int iChannel, const char *pSample, unsigned int nSoundEntryHash) = 0;
//
//	// stop all active sounds (client only)
//	virtual void StopAllSounds(bool bClearBuffers) = 0;
//
//	// Set the room type for a player (client only)
//	virtual void SetRoomType(IRecipientFilter& filter, int roomType) = 0;
//
//	// Set the dsp preset for a player (client only)
//	virtual void SetPlayerDSP(IRecipientFilter& filter, int dspType, bool fastReset) = 0;
//
//	// emit an "ambient" sound that isn't spatialized
//	// only available on the client, assert on server
//	virtual int EmitAmbientSound(const char *pSample, float flVolume, int iPitch = PITCH_NORM, int flags = 0, float soundtime = 0.0f) = 0;
//
//
//	//	virtual EntChannel_t	CreateEntChannel() = 0;
//
//	virtual float GetDistGainFromSoundLevel(soundlevel_t soundlevel, float dist) = 0;
//
//	// Client .dll only functions
//	virtual int		GetGuidForLastSoundEmitted() = 0;
//	virtual bool	IsSoundStillPlaying(int guid) = 0;
//	virtual void	StopSoundByGuid(int guid, bool bForceSync) = 0;
//	// Set's master volume (0.0->1.0)
//	virtual void	SetVolumeByGuid(int guid, float fvol) = 0;
//
//	// Retrieves list of all active sounds
//	virtual void	GetActiveSounds(CUtlVector< SndInfo_t >& sndlist) = 0;
//
//	virtual void	PrecacheSentenceGroup(const char *pGroupName) = 0;
//	virtual void	NotifyBeginMoviePlayback() = 0;
//	virtual void	NotifyEndMoviePlayback() = 0;
//
//	virtual bool	GetSoundChannelVolume(const char* sound, float &flVolumeLeft, float &flVolumeRight) = 0;
//
//	virtual float	GetElapsedTimeByGuid(int guid) = 0;
//
//};

class IEngineSound
{
public:
	void GetActiveSounds(CUtlVector<SndInfo_t> & sndlist)
	{
		//VirtualFunc(void)(PVOID, Engine::ValveSDK::CUtlVector<Engine::ValveSDK::SndInfo_t> &);
		//CallVirtual< OriginalFn >(this, 19)(this, sndlist);
		typedef void(__thiscall* OriginalFn)(PVOID, CUtlVector<SndInfo_t> &);
		return Memory::VCall<OriginalFn>(this, 19)(this, sndlist);
	}
};

class IViewRenderBeams
{
public:
	virtual void InitBeams(void) = 0;
	virtual void ShutdownBeams(void) = 0;
	virtual void ClearBeams(void) = 0;

	// Updates the state of the temp ent beams
	virtual void UpdateTempEntBeams() = 0;

	virtual void DrawBeam(C_Beam* pbeam, ITraceFilter *pEntityBeamTraceFilter = NULL) = 0;
	virtual void DrawBeam(Beam_t *pbeam) = 0;

	virtual void KillDeadBeams(C_BasePlayer *pEnt) = 0;

	// New interfaces!
	virtual Beam_t *CreateBeamEnts(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamEntPoint(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamPoints(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamRing(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamRingPoint(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamCirclePoints(BeamInfo_t &beamInfo) = 0;
	virtual Beam_t *CreateBeamFollow(BeamInfo_t &beamInfo) = 0;

	virtual void FreeBeam(Beam_t *pBeam) = 0;
	virtual void UpdateBeamInfo(Beam_t *pBeam, BeamInfo_t &beamInfo) = 0;

	// These will go away!
	virtual void CreateBeamEnts(int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b, int type = -1) = 0;
	virtual void CreateBeamEntPoint(int nStartEntity, const Vector *pStart, int nEndEntity, const Vector* pEnd, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b) = 0;
	virtual void CreateBeamPoints(Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b) = 0;
	virtual void CreateBeamRing(int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b, int flags = 0) = 0;
	virtual void CreateBeamRingPoint(const Vector& center, float start_radius, float end_radius, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b, int flags = 0) = 0;
	virtual void CreateBeamCirclePoints(int type, Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b) = 0;
	virtual void CreateBeamFollow(int startEnt, int modelIndex, int haloIndex, float haloScale, float life, float width, float m_nEndWidth, float m_nFadeLength, float r, float g, float b, float brightness) = 0;
};

#pragma endregion

#pragma region decl_functions
void RandomSeed( unsigned int seed );
float RandomFloat( float min, float max );
int RandomInt( int min, int max );

void CRC32_Init( CRC32_t* pulCRC );
void CRC32_ProcessBuffer( CRC32_t* pulCRC, const void* p, int len );
void CRC32_Final( CRC32_t* pulCRC );
CRC32_t CRC32_GetTableEntry( unsigned int slot );
#pragma endregion

struct clientanimating_t
{
	C_BaseAnimating *pAnimating;
	unsigned int	flags;
	clientanimating_t(C_BaseAnimating *_pAnim, unsigned int _flags) : pAnimating(_pAnim), flags(_flags) {}
};

class c_drawhack;
class c_variables;
class c_visuals;
class c_aimbot;
class c_lagcomp;
class c_antiaimbot; 
class c_resolver;
class c_autowall;
class c_player_records;
class CVars;
class CMenu;
class c_dormant_esp;
class c_misc;
class c_legitbot;
struct _shotinfo;

namespace cheat
{
	namespace game {
		extern bool	pressed_keys[256];
		extern int last_frame;
		extern bool get_key_press(int key, int zticks = 1);
		template <class T>
		T find_hud_element(const char* name)
		{
			static auto pThis = *reinterpret_cast<DWORD**>(Memory::Scan(cheat::main::clientdll, "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") + 1);

			static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(Memory::Scan(cheat::main::clientdll, "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));

			if (find_hud_element && pThis)
				return (T)find_hud_element(pThis, name);
			else
				return (T)nullptr;
		}
		extern void log(std::string text);
		extern void* update_hud_weapons;
		extern CUserCmd* last_cmd;
		extern CCSGO_HudDeathNotice* hud_death_notice;
		extern Vector2D screen_size;
	}
	namespace features {
		extern c_drawhack menu;
		extern c_visuals visuals;
		extern c_aimbot aimbot;
		extern c_lagcomp lagcomp;
		extern c_antiaimbot antiaimbot;
		extern c_resolver aaa;
		extern c_autowall autowall;
		extern c_music_player music;
		extern CUtlVector<clientanimating_t> *clientside_animlist;
		extern CClientEffectRegistration *effects_head;
		extern c_dormant_esp dormant;
		extern c_misc misc;
		extern c_legitbot legitbot;
	}
	namespace main {
		extern FORCEINLINE C_BasePlayer* local();
		extern float lerp_time;
		extern QAngle fix;
		extern CCSGOPlayerAnimState localstate;
		extern bool reset_local_animstate;
		extern bool fakewalking;
		extern bool updating_skins;
		extern int prev_fakelag_value;
		extern int side;
		extern int fside;
		extern int shots_fired[128];
		extern int shots_total[128];
		extern int history_hit[128];
		extern bool thirdperson;
		extern bool send_packet;
		extern bool updating_anims;
		extern bool called_chams_render;
		extern std::vector<Vector> points[64][19];
		extern _shotinfo fired_shot;
		extern std::vector<int> command_numbers;
		extern matrix3x4_t localplaya_matrix[128];
		extern float hit_time;
		extern bool jittering;
		extern std::string enginedll;
		extern std::string clientdll;
		extern float fov;
		extern bool fakeducking;
		extern int last_penetrated_count;
		extern int known_cmd_nr;
		extern bool setuping_bones;
		extern bool setuped_bones;
		extern std::wstring nickname;
		extern std::array<float, 24> fake_pose;
		extern int shots;
		extern int shots_hit;
		extern int shots_missed;
		extern float real_angle;
		extern float real_yaw;
		extern int fps;
		extern float last_kill_time;
		extern bool updating_resolver_anims;
		extern bool fast_autostop;
		extern float fake_angle;
		extern bool stand;
		extern int unpred_tickcount;
		extern bool updating_local_anims;
		extern std::array<float, 24> real_pose;
		extern bool should_update_local_anims;
		extern C_AnimationLayer local_server_anim_layers[13];
		extern QAngle local_eye_angles;
		extern Vector local_bone_origin_delta[128];
	}
	namespace convars {
		extern int sv_usercmd_custom_random_seed;
		extern float weapon_recoil_scale;
		extern int weapon_accuracy_nospread;
		extern int sv_clip_penetration_traces_to_players;
		extern float ff_damage_reduction_bullets;
		extern float ff_damage_bullet_penetration;
		extern int sv_penetration_type;

		//namespace spoofed {
		//	extern SpoofedConvar *viewmodel_offset_x;
		//	extern SpoofedConvar *viewmodel_offset_y;
		//	extern SpoofedConvar *viewmodel_offset_z;
		//	extern SpoofedConvar *sv_cheats;
		//}
	}
	extern c_variables settings;
	extern CVars Cvars;
	extern CMenu menu;
}