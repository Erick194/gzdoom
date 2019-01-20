/*
** thingdef_data.cpp
**
** DECORATE data tables
**
**---------------------------------------------------------------------------
** Copyright 2002-2008 Christoph Oelckers
** Copyright 2004-2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "thingdef.h"
#include "actor.h"
#include "d_player.h"
#include "p_effect.h"
#include "gi.h"
#include "p_terrain.h"
#include "gstrings.h"
#include "g_levellocals.h"
#include "p_checkposition.h"
#include "v_font.h"
#include "menu/menu.h"
#include "teaminfo.h"
#include "r_data/sprites.h"
#include "serializer.h"
#include "wi_stuff.h"
#include "a_dynlight.h"
#include "types.h"

static TArray<FPropertyInfo*> properties;
static TArray<AFuncDesc> AFTable;
static TArray<FieldDesc> FieldTable;
extern int				BackbuttonTime;
extern float			BackbuttonAlpha;

//==========================================================================
//
// List of all flags
//
//==========================================================================

// [RH] Keep GCC quiet by not using offsetof on Actor types.
#define DEFINE_FLAG(prefix, name, type, variable) { (unsigned int)prefix##_##name, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native }
#define DEFINE_PROTECTED_FLAG(prefix, name, type, variable) { (unsigned int)prefix##_##name, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native|VARF_ReadOnly|VARF_InternalAccess }
#define DEFINE_FLAG2(symbol, name, type, variable) { (unsigned int)symbol, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native }
#define DEFINE_FLAG2_DEPRECATED(symbol, name, type, variable) { (unsigned int)symbol, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native|VARF_Deprecated }
#define DEFINE_DEPRECATED_FLAG(name) { DEPF_##name, #name, -1, 0, true }
#define DEFINE_DUMMY_FLAG(name, deprec) { DEPF_UNUSED, #name, -1, 0, deprec? VARF_Deprecated:0 }

// internal flags. These do not get exposed to actor definitions but scripts need to be able to access them as variables.
static FFlagDef InternalActorFlagDefs[]=
{
	DEFINE_FLAG(MF, INCHASE, AActor, flags),
	DEFINE_FLAG(MF, UNMORPHED, AActor, flags),
	DEFINE_FLAG(MF2, FLY, AActor, flags2),
	DEFINE_FLAG(MF2, ONMOBJ, AActor, flags2),
	DEFINE_FLAG(MF2, ARGSDEFINED, AActor, flags2),
	DEFINE_FLAG(MF3, NOSIGHTCHECK, AActor, flags3),
	DEFINE_FLAG(MF3, CRASHED, AActor, flags3),
	DEFINE_FLAG(MF3, WARNBOT, AActor, flags3),
	DEFINE_FLAG(MF3, HUNTPLAYERS, AActor, flags3),
	DEFINE_FLAG(MF4, NOHATEPLAYERS, AActor, flags4),
	DEFINE_FLAG(MF4, SCROLLMOVE, AActor, flags4),
	DEFINE_FLAG(MF4, VFRICTION, AActor, flags4),
	DEFINE_FLAG(MF4, BOSSSPAWNED, AActor, flags4),
	DEFINE_FLAG(MF5, AVOIDINGDROPOFF, AActor, flags5),
	DEFINE_FLAG(MF5, CHASEGOAL, AActor, flags5),
	DEFINE_FLAG(MF5, INCONVERSATION, AActor, flags5),
	DEFINE_FLAG(MF6, ARMED, AActor, flags6),
	DEFINE_FLAG(MF6, FALLING, AActor, flags6),
	DEFINE_FLAG(MF6, LINEDONE, AActor, flags6),
	DEFINE_FLAG(MF6, SHATTERING, AActor, flags6),
	DEFINE_FLAG(MF6, KILLED, AActor, flags6),
	DEFINE_FLAG(MF6, BOSSCUBE, AActor, flags6),
	DEFINE_FLAG(MF6, INTRYMOVE, AActor, flags6),
	DEFINE_FLAG(MF7, HANDLENODELAY, AActor, flags7),
	DEFINE_FLAG(MF7, FLYCHEAT, AActor, flags7),
	DEFINE_FLAG(FX, RESPAWNINVUL, AActor, effects),
};


static FFlagDef ActorFlagDefs[]=
{
	DEFINE_FLAG(MF, PICKUP, APlayerPawn, flags),
	DEFINE_FLAG(MF, SPECIAL, APlayerPawn, flags),
	DEFINE_FLAG(MF, SOLID, AActor, flags),
	DEFINE_FLAG(MF, SHOOTABLE, AActor, flags),
	DEFINE_PROTECTED_FLAG(MF, NOSECTOR, AActor, flags),
	DEFINE_PROTECTED_FLAG(MF, NOBLOCKMAP, AActor, flags),
	DEFINE_FLAG(MF, AMBUSH, AActor, flags),
	DEFINE_FLAG(MF, JUSTHIT, AActor, flags),
	DEFINE_FLAG(MF, JUSTATTACKED, AActor, flags),
	DEFINE_FLAG(MF, SPAWNCEILING, AActor, flags),
	DEFINE_FLAG(MF, NOGRAVITY, AActor, flags),
	DEFINE_FLAG(MF, DROPOFF, AActor, flags),
	DEFINE_FLAG(MF, NOCLIP, AActor, flags),
	DEFINE_FLAG(MF, FLOAT, AActor, flags),
	DEFINE_FLAG(MF, TELEPORT, AActor, flags),
	DEFINE_FLAG(MF, MISSILE, AActor, flags),
	DEFINE_FLAG(MF, DROPPED, AActor, flags),
	DEFINE_FLAG(MF, SHADOW, AActor, flags),
	DEFINE_FLAG(MF, NOBLOOD, AActor, flags),
	DEFINE_FLAG(MF, CORPSE, AActor, flags),
	DEFINE_FLAG(MF, INFLOAT, AActor, flags),
	DEFINE_FLAG(MF, COUNTKILL, AActor, flags),
	DEFINE_FLAG(MF, COUNTITEM, AActor, flags),
	DEFINE_FLAG(MF, SKULLFLY, AActor, flags),
	DEFINE_FLAG(MF, NOTDMATCH, AActor, flags),
	DEFINE_FLAG(MF, SPAWNSOUNDSOURCE, AActor, flags),
	DEFINE_FLAG(MF, FRIENDLY, AActor, flags),
	DEFINE_FLAG(MF, NOLIFTDROP, AActor, flags),
	DEFINE_FLAG(MF, STEALTH, AActor, flags),
	DEFINE_FLAG(MF, ICECORPSE, AActor, flags),

	DEFINE_FLAG(MF2, DONTREFLECT, AActor, flags2),
	DEFINE_FLAG(MF2, WINDTHRUST, AActor, flags2),
	DEFINE_FLAG(MF2, DONTSEEKINVISIBLE, AActor, flags2),
	DEFINE_FLAG(MF2, BLASTED, AActor, flags2),
	DEFINE_FLAG(MF2, FLOORCLIP, AActor, flags2),
	DEFINE_FLAG(MF2, SPAWNFLOAT, AActor, flags2),
	DEFINE_FLAG(MF2, NOTELEPORT, AActor, flags2),
	DEFINE_FLAG2(MF2_RIP, RIPPER, AActor, flags2),
	DEFINE_FLAG(MF2, PUSHABLE, AActor, flags2),
	DEFINE_FLAG2(MF2_SLIDE, SLIDESONWALLS, AActor, flags2),
	DEFINE_FLAG2(MF2_PASSMOBJ, CANPASS, AActor, flags2),
	DEFINE_FLAG(MF2, CANNOTPUSH, AActor, flags2),
	DEFINE_FLAG(MF2, THRUGHOST, AActor, flags2),
	DEFINE_FLAG(MF2, BOSS, AActor, flags2),
	DEFINE_FLAG2(MF2_NODMGTHRUST, NODAMAGETHRUST, AActor, flags2),
	DEFINE_FLAG(MF2, DONTTRANSLATE, AActor, flags2),
	DEFINE_FLAG(MF2, TELESTOMP, AActor, flags2),
	DEFINE_FLAG(MF2, FLOATBOB, AActor, flags2),
	DEFINE_FLAG(MF2, THRUACTORS, AActor, flags2),
	DEFINE_FLAG2(MF2_IMPACT, ACTIVATEIMPACT, AActor, flags2),
	DEFINE_FLAG2(MF2_PUSHWALL, CANPUSHWALLS, AActor, flags2),
	DEFINE_FLAG2(MF2_MCROSS, ACTIVATEMCROSS, AActor, flags2),
	DEFINE_FLAG2(MF2_PCROSS, ACTIVATEPCROSS, AActor, flags2),
	DEFINE_FLAG(MF2, CANTLEAVEFLOORPIC, AActor, flags2),
	DEFINE_FLAG(MF2, NONSHOOTABLE, AActor, flags2),
	DEFINE_FLAG(MF2, INVULNERABLE, AActor, flags2),
	DEFINE_FLAG(MF2, DORMANT, AActor, flags2),
	DEFINE_FLAG(MF2, SEEKERMISSILE, AActor, flags2),
	DEFINE_FLAG(MF2, REFLECTIVE, AActor, flags2),

	DEFINE_FLAG(MF3, FLOORHUGGER, AActor, flags3),
	DEFINE_FLAG(MF3, CEILINGHUGGER, AActor, flags3),
	DEFINE_FLAG(MF3, NORADIUSDMG, AActor, flags3),
	DEFINE_FLAG(MF3, GHOST, AActor, flags3),
	DEFINE_FLAG(MF3, SPECIALFLOORCLIP, AActor, flags3),
	DEFINE_FLAG(MF3, ALWAYSPUFF, AActor, flags3),
	DEFINE_FLAG(MF3, DONTSPLASH, AActor, flags3),
	DEFINE_FLAG(MF3, DONTOVERLAP, AActor, flags3),
	DEFINE_FLAG(MF3, DONTMORPH, AActor, flags3),
	DEFINE_FLAG(MF3, DONTSQUASH, AActor, flags3),
	DEFINE_FLAG(MF3, EXPLOCOUNT, AActor, flags3),
	DEFINE_FLAG(MF3, FULLVOLACTIVE, AActor, flags3),
	DEFINE_FLAG(MF3, ISMONSTER, AActor, flags3),
	DEFINE_FLAG(MF3, SKYEXPLODE, AActor, flags3),
	DEFINE_FLAG(MF3, STAYMORPHED, AActor, flags3),
	DEFINE_FLAG(MF3, DONTBLAST, AActor, flags3),
	DEFINE_FLAG(MF3, CANBLAST, AActor, flags3),
	DEFINE_FLAG(MF3, NOTARGET, AActor, flags3),
	DEFINE_FLAG(MF3, DONTGIB, AActor, flags3),
	DEFINE_FLAG(MF3, NOBLOCKMONST, AActor, flags3),
	DEFINE_FLAG(MF3, FULLVOLDEATH, AActor, flags3),
	DEFINE_FLAG(MF3, AVOIDMELEE, AActor, flags3),
	DEFINE_FLAG(MF3, SCREENSEEKER, AActor, flags3),
	DEFINE_FLAG(MF3, FOILINVUL, AActor, flags3),
	DEFINE_FLAG(MF3, NOTELEOTHER, AActor, flags3),
	DEFINE_FLAG(MF3, BLOODLESSIMPACT, AActor, flags3),
	DEFINE_FLAG(MF3, NOEXPLODEFLOOR, AActor, flags3),
	DEFINE_FLAG(MF3, PUFFONACTORS, AActor, flags3),

	DEFINE_FLAG(MF4, QUICKTORETALIATE, AActor, flags4),
	DEFINE_FLAG(MF4, NOICEDEATH, AActor, flags4),
	DEFINE_FLAG(MF4, RANDOMIZE, AActor, flags4),
	DEFINE_FLAG(MF4, FIXMAPTHINGPOS , AActor, flags4),
	DEFINE_FLAG(MF4, ACTLIKEBRIDGE, AActor, flags4),
	DEFINE_FLAG(MF4, STRIFEDAMAGE, AActor, flags4),
	DEFINE_FLAG(MF4, CANUSEWALLS, AActor, flags4),
	DEFINE_FLAG(MF4, MISSILEMORE, AActor, flags4),
	DEFINE_FLAG(MF4, MISSILEEVENMORE, AActor, flags4),
	DEFINE_FLAG(MF4, FORCERADIUSDMG, AActor, flags4),
	DEFINE_FLAG(MF4, DONTFALL, AActor, flags4),
	DEFINE_FLAG(MF4, SEESDAGGERS, AActor, flags4),
	DEFINE_FLAG(MF4, INCOMBAT, AActor, flags4),
	DEFINE_FLAG(MF4, LOOKALLAROUND, AActor, flags4),
	DEFINE_FLAG(MF4, STANDSTILL, AActor, flags4),
	DEFINE_FLAG(MF4, SPECTRAL, AActor, flags4),
	DEFINE_FLAG(MF4, NOSPLASHALERT, AActor, flags4),
	DEFINE_FLAG(MF4, SYNCHRONIZED, AActor, flags4),
	DEFINE_FLAG(MF4, NOTARGETSWITCH, AActor, flags4),
	DEFINE_FLAG(MF4, DONTHARMCLASS, AActor, flags4),
	DEFINE_FLAG(MF4, SHIELDREFLECT, AActor, flags4),
	DEFINE_FLAG(MF4, DEFLECT, AActor, flags4),
	DEFINE_FLAG(MF4, ALLOWPARTICLES, AActor, flags4),
	DEFINE_FLAG(MF4, EXTREMEDEATH, AActor, flags4),
	DEFINE_FLAG(MF4, NOEXTREMEDEATH, AActor, flags4),
	DEFINE_FLAG(MF4, FRIGHTENED, AActor, flags4),
	DEFINE_FLAG(MF4, NOSKIN, AActor, flags4),
	DEFINE_FLAG(MF4, BOSSDEATH, AActor, flags4),

	DEFINE_FLAG(MF5, DONTDRAIN, AActor, flags5),
	DEFINE_FLAG(MF5, GETOWNER, AActor, flags5),
	DEFINE_FLAG(MF5, NODROPOFF, AActor, flags5),
	DEFINE_FLAG(MF5, NOFORWARDFALL, AActor, flags5),
	DEFINE_FLAG(MF5, COUNTSECRET, AActor, flags5),
	DEFINE_FLAG(MF5, NODAMAGE, AActor, flags5),
	DEFINE_FLAG(MF5, BLOODSPLATTER, AActor, flags5),
	DEFINE_FLAG(MF5, OLDRADIUSDMG, AActor, flags5),
	DEFINE_FLAG(MF5, DEHEXPLOSION, AActor, flags5),
	DEFINE_FLAG(MF5, PIERCEARMOR, AActor, flags5),
	DEFINE_FLAG(MF5, NOBLOODDECALS, AActor, flags5),
	DEFINE_FLAG(MF5, USESPECIAL, AActor, flags5),
	DEFINE_FLAG(MF5, NOPAIN, AActor, flags5),
	DEFINE_FLAG(MF5, ALWAYSFAST, AActor, flags5),
	DEFINE_FLAG(MF5, NEVERFAST, AActor, flags5),
	DEFINE_FLAG(MF5, ALWAYSRESPAWN, AActor, flags5),
	DEFINE_FLAG(MF5, NEVERRESPAWN, AActor, flags5),
	DEFINE_FLAG(MF5, DONTRIP, AActor, flags5),
	DEFINE_FLAG(MF5, NOINFIGHTING, AActor, flags5),
	DEFINE_FLAG(MF5, NOINTERACTION, AActor, flags5),
	DEFINE_FLAG(MF5, NOTIMEFREEZE, AActor, flags5),
	DEFINE_FLAG(MF5, PUFFGETSOWNER, AActor, flags5), // [BB] added PUFFGETSOWNER
	DEFINE_FLAG(MF5, SPECIALFIREDAMAGE, AActor, flags5),
	DEFINE_FLAG(MF5, SUMMONEDMONSTER, AActor, flags5),
	DEFINE_FLAG(MF5, NOVERTICALMELEERANGE, AActor, flags5),
	DEFINE_FLAG(MF5, BRIGHT, AActor, flags5),
	DEFINE_FLAG(MF5, CANTSEEK, AActor, flags5),
	DEFINE_FLAG(MF5, PAINLESS, AActor, flags5),
	DEFINE_FLAG(MF5, MOVEWITHSECTOR, AActor, flags5),

	DEFINE_FLAG(MF6, NOBOSSRIP, AActor, flags6),
	DEFINE_FLAG(MF6, THRUSPECIES, AActor, flags6),
	DEFINE_FLAG(MF6, MTHRUSPECIES, AActor, flags6),
	DEFINE_FLAG(MF6, FORCEPAIN, AActor, flags6),
	DEFINE_FLAG(MF6, NOFEAR, AActor, flags6),
	DEFINE_FLAG(MF6, BUMPSPECIAL, AActor, flags6),
	DEFINE_FLAG(MF6, DONTHARMSPECIES, AActor, flags6),
	DEFINE_FLAG(MF6, STEPMISSILE, AActor, flags6),
	DEFINE_FLAG(MF6, NOTELEFRAG, AActor, flags6),
	DEFINE_FLAG(MF6, TOUCHY, AActor, flags6),
	DEFINE_FLAG(MF6, CANJUMP, AActor, flags6),
	DEFINE_FLAG(MF6, JUMPDOWN, AActor, flags6),
	DEFINE_FLAG(MF6, VULNERABLE, AActor, flags6),
	DEFINE_FLAG(MF6, NOTRIGGER, AActor, flags6),
	DEFINE_FLAG(MF6, ADDITIVEPOISONDAMAGE, AActor, flags6),
	DEFINE_FLAG(MF6, ADDITIVEPOISONDURATION, AActor, flags6),
	DEFINE_FLAG(MF6, BLOCKEDBYSOLIDACTORS, AActor, flags6),
	DEFINE_FLAG(MF6, NOMENU, AActor, flags6),
	DEFINE_FLAG(MF6, SEEINVISIBLE, AActor, flags6),
	DEFINE_FLAG(MF6, DONTCORPSE, AActor, flags6),
	DEFINE_FLAG(MF6, DOHARMSPECIES, AActor, flags6),
	DEFINE_FLAG(MF6, POISONALWAYS, AActor, flags6),
	DEFINE_FLAG(MF6, NOTAUTOAIMED, AActor, flags6),
	DEFINE_FLAG(MF6, NOTONAUTOMAP, AActor, flags6),
	DEFINE_FLAG(MF6, RELATIVETOFLOOR, AActor, flags6),

	DEFINE_FLAG(MF7, NEVERTARGET, AActor, flags7),
	DEFINE_FLAG(MF7, NOTELESTOMP, AActor, flags7),
	DEFINE_FLAG(MF7, ALWAYSTELEFRAG, AActor, flags7),
	DEFINE_FLAG(MF7, WEAPONSPAWN, AActor, flags7),
	DEFINE_FLAG(MF7, HARMFRIENDS, AActor, flags7),
	DEFINE_FLAG(MF7, BUDDHA, AActor, flags7),
	DEFINE_FLAG(MF7, FOILBUDDHA, AActor, flags7),
	DEFINE_FLAG(MF7, DONTTHRUST, AActor, flags7),
	DEFINE_FLAG(MF7, ALLOWPAIN, AActor, flags7),
	DEFINE_FLAG(MF7, CAUSEPAIN, AActor, flags7),
	DEFINE_FLAG(MF7, THRUREFLECT, AActor, flags7),
	DEFINE_FLAG(MF7, MIRRORREFLECT, AActor, flags7),
	DEFINE_FLAG(MF7, AIMREFLECT, AActor, flags7),
	DEFINE_FLAG(MF7, HITTARGET, AActor, flags7),
	DEFINE_FLAG(MF7, HITMASTER, AActor, flags7),
	DEFINE_FLAG(MF7, HITTRACER, AActor, flags7),
	DEFINE_FLAG(MF7, NODECAL, AActor, flags7),		// [ZK] Decal flags
	DEFINE_FLAG(MF7, FORCEDECAL, AActor, flags7),
	DEFINE_FLAG(MF7, LAXTELEFRAGDMG, AActor, flags7),
	DEFINE_FLAG(MF7, ICESHATTER, AActor, flags7),
	DEFINE_FLAG(MF7, ALLOWTHRUFLAGS, AActor, flags7),
	DEFINE_FLAG(MF7, USEKILLSCRIPTS, AActor, flags7),
	DEFINE_FLAG(MF7, NOKILLSCRIPTS, AActor, flags7),
	DEFINE_FLAG(MF7, SPRITEANGLE, AActor, flags7),
	DEFINE_FLAG(MF7, SMASHABLE, AActor, flags7),
	DEFINE_FLAG(MF7, NOSHIELDREFLECT, AActor, flags7),
	DEFINE_FLAG(MF7, FORCEZERORADIUSDMG, AActor, flags7),
	DEFINE_FLAG(MF7, NOINFIGHTSPECIES, AActor, flags7),
	DEFINE_FLAG(MF7, FORCEINFIGHTING, AActor, flags7),

	DEFINE_FLAG(MF8, FRIGHTENING, AActor, flags8),
	DEFINE_FLAG(MF8, BLOCKASPLAYER, AActor, flags8),
	DEFINE_FLAG(MF8, DONTFACETALKER, AActor, flags8),
	DEFINE_FLAG(MF8, HITOWNER, AActor, flags8),
	DEFINE_FLAG(MF8, NOFRICTION, AActor, flags8),
	DEFINE_FLAG(MF8, NOFRICTIONBOUNCE, AActor, flags8),

	// Effect flags
	DEFINE_FLAG(FX, VISIBILITYPULSE, AActor, effects),
	DEFINE_FLAG2(FX_ROCKET, ROCKETTRAIL, AActor, effects),
	DEFINE_FLAG2(FX_GRENADE, GRENADETRAIL, AActor, effects),
	DEFINE_FLAG(RF, INVISIBLE, AActor, renderflags),
	DEFINE_FLAG(RF, FORCEYBILLBOARD, AActor, renderflags),
	DEFINE_FLAG(RF, FORCEXYBILLBOARD, AActor, renderflags),
	DEFINE_FLAG(RF, ROLLSPRITE, AActor, renderflags), // [marrub] roll the sprite billboard
			// [fgsfds] Flat sprites
	DEFINE_FLAG(RF, FLATSPRITE, AActor, renderflags),
	DEFINE_FLAG(RF, WALLSPRITE, AActor, renderflags),
	DEFINE_FLAG(RF, DONTFLIP, AActor, renderflags),
	DEFINE_FLAG(RF, ROLLCENTER, AActor, renderflags),
	DEFINE_FLAG(RF, MASKROTATION, AActor, renderflags),
	DEFINE_FLAG(RF, ABSMASKANGLE, AActor, renderflags),
	DEFINE_FLAG(RF, ABSMASKPITCH, AActor, renderflags),
	DEFINE_FLAG(RF, XFLIP, AActor, renderflags),
	DEFINE_FLAG(RF, YFLIP, AActor, renderflags),
	DEFINE_FLAG(RF, INTERPOLATEANGLES, AActor, renderflags),
	DEFINE_FLAG(RF, DONTINTERPOLATE, AActor, renderflags),
	DEFINE_FLAG(RF, SPRITEFLIP, AActor, renderflags),
	DEFINE_FLAG(RF, ZDOOMTRANS, AActor, renderflags),

	// Bounce flags
	DEFINE_FLAG2(BOUNCE_Walls, BOUNCEONWALLS, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_Floors, BOUNCEONFLOORS, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_Ceilings, BOUNCEONCEILINGS, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_Actors, ALLOWBOUNCEONACTORS, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_AutoOff, BOUNCEAUTOOFF, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_HereticType, BOUNCELIKEHERETIC, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_CanBounceWater, CANBOUNCEWATER, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_NoWallSound, NOWALLBOUNCESND, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_Quiet, NOBOUNCESOUND, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_AllActors, BOUNCEONACTORS, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_ExplodeOnWater, EXPLODEONWATER, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_MBF, MBFBOUNCER, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_AutoOffFloorOnly, BOUNCEAUTOOFFFLOORONLY, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_UseBounceState, USEBOUNCESTATE, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_NotOnShootables, DONTBOUNCEONSHOOTABLES, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_BounceOnUnrips, BOUNCEONUNRIPPABLES, AActor, BounceFlags),
	DEFINE_FLAG2(BOUNCE_NotOnSky, DONTBOUNCEONSKY, AActor, BounceFlags),
};

// These won't be accessible through bitfield variables
static FFlagDef MoreFlagDefs[] =
{

	// Deprecated flags. Handling must be performed in HandleDeprecatedFlags
	DEFINE_DEPRECATED_FLAG(FIREDAMAGE),
	DEFINE_DEPRECATED_FLAG(ICEDAMAGE),
	DEFINE_DEPRECATED_FLAG(LOWGRAVITY),
	DEFINE_DEPRECATED_FLAG(SHORTMISSILERANGE),
	DEFINE_DEPRECATED_FLAG(LONGMELEERANGE),
	DEFINE_DEPRECATED_FLAG(QUARTERGRAVITY),
	DEFINE_DEPRECATED_FLAG(FIRERESIST),
	DEFINE_DEPRECATED_FLAG(HERETICBOUNCE),
	DEFINE_DEPRECATED_FLAG(HEXENBOUNCE),
	DEFINE_DEPRECATED_FLAG(DOOMBOUNCE),

	// Deprecated flags with no more existing functionality.
	DEFINE_DUMMY_FLAG(FASTER, true),				// obsolete, replaced by 'Fast' state flag
	DEFINE_DUMMY_FLAG(FASTMELEE, true),			// obsolete, replaced by 'Fast' state flag

	// Deprecated name as an alias
	DEFINE_FLAG2_DEPRECATED(MF4_DONTHARMCLASS, DONTHURTSPECIES, AActor, flags4),

	// Various Skulltag flags that are quite irrelevant to ZDoom
	// [BC] New DECORATE flag defines here.
	DEFINE_DUMMY_FLAG(BLUETEAM, false),
	DEFINE_DUMMY_FLAG(REDTEAM, false),
	DEFINE_DUMMY_FLAG(USESPECIAL, false),
	DEFINE_DUMMY_FLAG(BASEHEALTH, false),
	DEFINE_DUMMY_FLAG(SUPERHEALTH, false),
	DEFINE_DUMMY_FLAG(BASEARMOR, false),
	DEFINE_DUMMY_FLAG(SUPERARMOR, false),
	DEFINE_DUMMY_FLAG(SCOREPILLAR, false),
	DEFINE_DUMMY_FLAG(NODE, false),
	DEFINE_DUMMY_FLAG(USESTBOUNCESOUND, false),
	DEFINE_DUMMY_FLAG(EXPLODEONDEATH, true),
	DEFINE_DUMMY_FLAG(DONTIDENTIFYTARGET, false), // [CK]

	// Skulltag netcode-based flags.
	// [BB] New DECORATE network related flag defines here.
	DEFINE_DUMMY_FLAG(NONETID, false),
	DEFINE_DUMMY_FLAG(ALLOWCLIENTSPAWN, false),
	DEFINE_DUMMY_FLAG(CLIENTSIDEONLY, false),
	DEFINE_DUMMY_FLAG(SERVERSIDEONLY, false),
};

static FFlagDef PlayerPawnFlagDefs[] =
{
	// PlayerPawn flags
	DEFINE_FLAG(PPF, NOTHRUSTWHENINVUL, APlayerPawn, PlayerFlags),
	DEFINE_FLAG(PPF, CANSUPERMORPH, APlayerPawn, PlayerFlags),
	DEFINE_FLAG(PPF, CROUCHABLEMORPH, APlayerPawn, PlayerFlags),
};

static FFlagDef DynLightFlagDefs[] =
{
	// PlayerPawn flags
	DEFINE_FLAG(LF, SUBTRACTIVE, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, ADDITIVE, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, DONTLIGHTSELF, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, ATTENUATE, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, NOSHADOWMAP, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, DONTLIGHTACTORS, ADynamicLight, lightflags),
	DEFINE_FLAG(LF, SPOT, ADynamicLight, lightflags),
};

static const struct FFlagList { const PClass * const *Type; FFlagDef *Defs; int NumDefs; int Use; } FlagLists[] =
{
	{ &RUNTIME_CLASS_CASTLESS(AActor), 		ActorFlagDefs,		countof(ActorFlagDefs), 3 },	// -1 to account for the terminator
	{ &RUNTIME_CLASS_CASTLESS(AActor), 		MoreFlagDefs,		countof(MoreFlagDefs), 1 },
	{ &RUNTIME_CLASS_CASTLESS(AActor), 	InternalActorFlagDefs,	countof(InternalActorFlagDefs), 2 },
	{ &RUNTIME_CLASS_CASTLESS(APlayerPawn),	PlayerPawnFlagDefs,	countof(PlayerPawnFlagDefs), 3 },
	{ &RUNTIME_CLASS_CASTLESS(ADynamicLight),DynLightFlagDefs,	countof(DynLightFlagDefs), 3 },
};
#define NUM_FLAG_LISTS (countof(FlagLists))

static FFlagDef forInternalFlags;

//==========================================================================
//
// Find a flag by name using a binary search
//
//==========================================================================
static FFlagDef *FindFlag (FFlagDef *flags, int numflags, const char *flag)
{
	int min = 0, max = numflags - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = stricmp (flag, flags[mid].name);
		if (lexval == 0)
		{
			return &flags[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// Finds a flag that may have a qualified name
//
//==========================================================================

FFlagDef *FindFlag (const PClass *type, const char *part1, const char *part2, bool strict)
{

	if (part2 == nullptr)
	{
		FStringf internalname("@flagdef@%s", part1);
		FName name(internalname, true);
		if (name != NAME_None)
		{
			auto field = dyn_cast<PPropFlag>(type->FindSymbol(name, true));
			if (field != nullptr && (!strict || !field->decorateOnly))
			{
				forInternalFlags.fieldsize = 4;
				forInternalFlags.name = "";
				forInternalFlags.flagbit = field->Offset? 1 << field->bitval : field->bitval;
				forInternalFlags.structoffset = field->Offset? (int)field->Offset->Offset : -1;
				forInternalFlags.varflags = field->Offset == nullptr && field->bitval > 0? VARF_Deprecated : 0;
				return &forInternalFlags;
			}
		}
	}
	else
	{
		FStringf internalname("@flagdef@%s.%s", part1, part2);
		FName name(internalname, true);
		if (name != NAME_None)
		{
			auto field = dyn_cast<PPropFlag>(type->FindSymbol(name, true));
			if (field != nullptr)
			{
				forInternalFlags.fieldsize = 4;
				forInternalFlags.name = "";
				forInternalFlags.flagbit = field->Offset ? 1 << field->bitval : field->bitval;
				forInternalFlags.structoffset = field->Offset ? (int)field->Offset->Offset : -1;
				forInternalFlags.varflags = field->Offset == nullptr && field->bitval > 0? VARF_Deprecated : 0;
				return &forInternalFlags;
			}
		}
	}

	// Not found. Try the internal flag definitions.


	FFlagDef *def;

	if (part2 == NULL)
	{ // Search all lists
		int max = strict ? 2 : NUM_FLAG_LISTS;
		for (int i = 0; i < max; ++i)
		{
			if ((FlagLists[i].Use & 1) && type->IsDescendantOf (*FlagLists[i].Type))
			{
				def = FindFlag (FlagLists[i].Defs, FlagLists[i].NumDefs, part1);
				if (def != NULL)
				{
					return def;
				}
			}
		}
	}
	else
	{ // Search just the named list
		for (size_t i = 0; i < NUM_FLAG_LISTS; ++i)
		{
			if (stricmp ((*FlagLists[i].Type)->TypeName.GetChars(), part1) == 0)
			{
				if (type->IsDescendantOf (*FlagLists[i].Type))
				{
					return FindFlag (FlagLists[i].Defs, FlagLists[i].NumDefs, part2);
				}
				else
				{
					return NULL;
				}
			}
		}
	}

	return NULL;
}


//==========================================================================
//
// Gets the name of an actor flag
//
//==========================================================================

const char *GetFlagName(unsigned int flagnum, int flagoffset)
{
	for(size_t i = 0; i < countof(ActorFlagDefs); i++)
	{
		if (ActorFlagDefs[i].flagbit == flagnum && ActorFlagDefs[i].structoffset == flagoffset)
		{
			return ActorFlagDefs[i].name;
		}
	}
	return "(unknown)";	// return something printable
}

//==========================================================================
//
// Find a property by name using a binary search
//
//==========================================================================

FPropertyInfo *FindProperty(const char * string)
{
	int min = 0, max = properties.Size()-1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = stricmp (string, properties[mid]->name);
		if (lexval == 0)
		{
			return properties[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
//
//
//==========================================================================

template <typename Desc>
static int CompareClassNames(const char* const aname, const Desc& b)
{
	// ++ to get past the prefix letter of the native class name, which gets omitted by the FName for the class.
	const char* bname = b.ClassName;
	if ('\0' != *bname) ++bname;
	return stricmp(aname, bname);
}

template <typename Desc>
static int CompareClassNames(const Desc& a, const Desc& b)
{
	// ++ to get past the prefix letter of the native class name, which gets omitted by the FName for the class.
	const char* aname = a.ClassName;
	if ('\0' != *aname) ++aname;
	return CompareClassNames(aname, b);
}

//==========================================================================
//
// Find a function by name using a binary search
//
//==========================================================================

AFuncDesc *FindFunction(PContainerType *cls, const char * string)
{
	int min = 0, max = AFTable.Size() - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = CompareClassNames(cls->TypeName.GetChars(), AFTable[mid]);
		if (lexval == 0) lexval = stricmp(string, AFTable[mid].FuncName);
		if (lexval == 0)
		{
			return &AFTable[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return nullptr;
}

//==========================================================================
//
// Find a function by name using a binary search
//
//==========================================================================

FieldDesc *FindField(PContainerType *cls, const char * string)
{
	int min = 0, max = FieldTable.Size() - 1;
	const char * cname = cls ? cls->TypeName.GetChars() : "";

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = CompareClassNames(cname, FieldTable[mid]);
		if (lexval == 0) lexval = stricmp(string, FieldTable[mid].FieldName);
		if (lexval == 0)
		{
			return &FieldTable[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return nullptr;
}


//==========================================================================
//
// Find an action function in AActor's table
//
//==========================================================================

VMFunction *FindVMFunction(PClass *cls, const char *name)
{
	auto f = dyn_cast<PFunction>(cls->FindSymbol(name, true));
	return f == nullptr ? nullptr : f->Variants[0].Implementation;
}


//==========================================================================
//
// Sorting helpers
//
//==========================================================================

static int flagcmp (const void * a, const void * b)
{
	return stricmp( ((FFlagDef*)a)->name, ((FFlagDef*)b)->name);
}

static int propcmp(const void * a, const void * b)
{
	return stricmp( (*(FPropertyInfo**)a)->name, (*(FPropertyInfo**)b)->name);
}

static int funccmp(const void * a, const void * b)
{
	int res = CompareClassNames(*(AFuncDesc*)a, *(AFuncDesc*)b);
	if (res == 0) res = stricmp(((AFuncDesc*)a)->FuncName, ((AFuncDesc*)b)->FuncName);
	return res;
}

static int fieldcmp(const void * a, const void * b)
{
	int res = CompareClassNames(*(FieldDesc*)a, *(FieldDesc*)b);
	if (res == 0) res = stricmp(((FieldDesc*)a)->FieldName, ((FieldDesc*)b)->FieldName);
	return res;
}

//==========================================================================
//
// Initialization
//
//==========================================================================

void InitThingdef()
{
	// Some native types need size and serialization information added before the scripts get compiled.
	auto secplanestruct = NewStruct("Secplane", nullptr, true);
	secplanestruct->Size = sizeof(secplane_t);
	secplanestruct->Align = alignof(secplane_t);

	auto sectorstruct = NewStruct("Sector", nullptr, true);
	sectorstruct->Size = sizeof(sector_t);
	sectorstruct->Align = alignof(sector_t);
	NewPointer(sectorstruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(sector_t **)addr);
		},
		[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<sector_t>(ar, key, *(sector_t **)addr, nullptr);
			return true;
		}
	);

	auto linestruct = NewStruct("Line", nullptr, true);
	linestruct->Size = sizeof(line_t);
	linestruct->Align = alignof(line_t);
	NewPointer(linestruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(line_t **)addr);
		},
		[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<line_t>(ar, key, *(line_t **)addr, nullptr);
			return true;
		}
	);

	auto sidestruct = NewStruct("Side", nullptr, true);
	sidestruct->Size = sizeof(side_t);
	sidestruct->Align = alignof(side_t);
	NewPointer(sidestruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(side_t **)addr);
		},
			[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<side_t>(ar, key, *(side_t **)addr, nullptr);
			return true;
		}
	);

	auto vertstruct = NewStruct("Vertex", nullptr, true);
	vertstruct->Size = sizeof(vertex_t);
	vertstruct->Align = alignof(vertex_t);
	NewPointer(vertstruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(vertex_t **)addr);
		},
		[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<vertex_t>(ar, key, *(vertex_t **)addr, nullptr);
			return true;
		}
	);

	auto sectorportalstruct = NewStruct("SectorPortal", nullptr, true);
	sectorportalstruct->Size = sizeof(FSectorPortal);
	sectorportalstruct->Align = alignof(FSectorPortal);

	auto playerclassstruct = NewStruct("PlayerClass", nullptr, true);
	playerclassstruct->Size = sizeof(FPlayerClass);
	playerclassstruct->Align = alignof(FPlayerClass);

	auto playerskinstruct = NewStruct("PlayerSkin", nullptr, true);
	playerskinstruct->Size = sizeof(FPlayerSkin);
	playerskinstruct->Align = alignof(FPlayerSkin);

	auto teamstruct = NewStruct("Team", nullptr, true);
	teamstruct->Size = sizeof(FTeam);
	teamstruct->Align = alignof(FTeam);

	PStruct *pstruct = NewStruct("PlayerInfo", nullptr, true);
	pstruct->Size = sizeof(player_t);
	pstruct->Align = alignof(player_t);
	NewPointer(pstruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(player_t **)addr);
		},
			[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<player_t>(ar, key, *(player_t **)addr, nullptr);
			return true;
		}
	);

	auto fontstruct = NewStruct("FFont", nullptr, true);
	fontstruct->Size = sizeof(FFont);
	fontstruct->Align = alignof(FFont);
	NewPointer(fontstruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(FFont **)addr);
		},
			[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<FFont>(ar, key, *(FFont **)addr, nullptr);
			return true;
		}
	);

	auto wbplayerstruct = NewStruct("WBPlayerStruct", nullptr, true);
	wbplayerstruct->Size = sizeof(wbplayerstruct_t);
	wbplayerstruct->Align = alignof(wbplayerstruct_t);

	FAutoSegIterator probe(CRegHead, CRegTail);

	while (*++probe != NULL)
	{
		if (((ClassReg *)*probe)->InitNatives)
			((ClassReg *)*probe)->InitNatives();
	}

	// Sort the flag lists
	for (size_t i = 0; i < NUM_FLAG_LISTS; ++i)
	{
		qsort (FlagLists[i].Defs, FlagLists[i].NumDefs, sizeof(FFlagDef), flagcmp);
	}

	// Create a sorted list of properties
	if (properties.Size() == 0)
	{
		FAutoSegIterator probe(GRegHead, GRegTail);

		while (*++probe != NULL)
		{
			properties.Push((FPropertyInfo *)*probe);
		}
		properties.ShrinkToFit();
		qsort(&properties[0], properties.Size(), sizeof(properties[0]), propcmp);
	}

	// Create a sorted list of native action functions
	AFTable.Clear();
	if (AFTable.Size() == 0)
	{
		FAutoSegIterator probe(ARegHead, ARegTail);

		while (*++probe != NULL)
		{
			AFuncDesc *afunc = (AFuncDesc *)*probe;
			assert(afunc->VMPointer != NULL);
			*(afunc->VMPointer) = new VMNativeFunction(afunc->Function, afunc->FuncName);
			(*(afunc->VMPointer))->PrintableName.Format("%s.%s [Native]", afunc->ClassName+1, afunc->FuncName);
			(*(afunc->VMPointer))->DirectNativeCall = afunc->DirectNative;
			AFTable.Push(*afunc);
		}
		AFTable.ShrinkToFit();
		qsort(&AFTable[0], AFTable.Size(), sizeof(AFTable[0]), funccmp);
	}

	// Add the constructor and destructor to FCheckPosition.
	auto fcp = NewStruct("FCheckPosition", nullptr);
	fcp->mConstructor = *FindFunction(fcp, "_Constructor")->VMPointer;
	fcp->mDestructor = *FindFunction(fcp, "_Destructor")->VMPointer;
	static const uint8_t reguse[] = { REGT_POINTER };
	fcp->mConstructor->RegTypes = fcp->mDestructor->RegTypes = reguse;
	fcp->Size = sizeof(FCheckPosition);
	fcp->Align = alignof(FCheckPosition);

	//This must also have its size set.
	auto frp = NewStruct("FRailParams", nullptr);
	frp->Size = sizeof(FRailParams);
	frp->Align = alignof(FRailParams);


	FieldTable.Clear();
	if (FieldTable.Size() == 0)
	{
		FAutoSegIterator probe(FRegHead, FRegTail);

		while (*++probe != NULL)
		{
			FieldDesc *afield = (FieldDesc *)*probe;
			FieldTable.Push(*afield);
		}
		FieldTable.ShrinkToFit();
		qsort(&FieldTable[0], FieldTable.Size(), sizeof(FieldTable[0]), fieldcmp);
	}
}

void SynthesizeFlagFields()
{
	// synthesize a symbol for each flag from the flag name tables to avoid redundant declaration of them.
	for (auto &fl : FlagLists)
	{
		auto cls = const_cast<PClass*>(*fl.Type);
		if (fl.Use & 2)
		{
			for (int i = 0; i < fl.NumDefs; i++)
			{
				if (fl.Defs[i].structoffset > 0) // skip the deprecated entries in this list
				{
					cls->VMType->AddNativeField(FStringf("b%s", fl.Defs[i].name), (fl.Defs[i].fieldsize == 4 ? TypeSInt32 : TypeSInt16), fl.Defs[i].structoffset, fl.Defs[i].varflags, fl.Defs[i].flagbit);
				}
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(DObject, BAM)
{
	PARAM_PROLOGUE;
	PARAM_FLOAT(ang);
	ACTION_RETURN_INT(DAngle(ang).BAMs());
}

FString FStringFormat(VM_ARGS, int offset)
{
	PARAM_VA_POINTER(va_reginfo)	// Get the hidden type information array
	assert(va_reginfo[offset] == REGT_STRING);

	FString fmtstring = param[offset].s().GetChars();

	param += offset;
	numparam -= offset;
	va_reginfo += offset;

	// note: we don't need a real printf format parser.
	//       enough to simply find the subtitution tokens and feed them to the real printf after checking types.
	//       https://en.wikipedia.org/wiki/Printf_format_string#Format_placeholder_specification
	FString output;
	bool in_fmt = false;
	FString fmt_current;
	int argnum = 1;
	int argauto = 1;
	// % = starts
	//  [0-9], -, +, \s, 0, #, . continue
	//  %, s, d, i, u, fF, eE, gG, xX, o, c, p, aA terminate
	// various type flags are not supported. not like stuff like 'hh' modifier is to be used in the VM.
	// the only combination that is parsed locally is %n$...
	bool haveargnums = false;
	for (size_t i = 0; i < fmtstring.Len(); i++)
	{
		char c = fmtstring[i];
		if (in_fmt)
		{
			if (c == '*' && (fmt_current.Len() == 1 || (fmt_current.Len() == 2 && fmt_current[1] == '0')))
			{
				fmt_current += c;
			}
			else if ((c >= '0' && c <= '9') ||
				c == '-' || c == '+' || (c == ' ' && fmt_current.Back() != ' ') || c == '#' || c == '.')
			{
				fmt_current += c;
			}
			else if (c == '$') // %number$format
			{
				if (!haveargnums && argauto > 1)
					ThrowAbortException(X_FORMAT_ERROR, "Cannot mix explicit and implicit arguments.");
				FString argnumstr = fmt_current.Mid(1);
				if (!argnumstr.IsInt()) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for argument number, got '%s'.", argnumstr.GetChars());
				argnum = argnumstr.ToLong();
				if (argnum < 1 || argnum >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format (tried to access argument %d, %d total).", argnum, numparam);
				fmt_current = "%";
				haveargnums = true;
			}
			else
			{
				fmt_current += c;

				switch (c)
				{
					// string
				case 's':
				{
					if (argnum < 0 && haveargnums)
						ThrowAbortException(X_FORMAT_ERROR, "Cannot mix explicit and implicit arguments.");
					in_fmt = false;
					// fail if something was found, but it's not a string
					if (argnum >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
					if (va_reginfo[argnum] != REGT_STRING) ThrowAbortException(X_FORMAT_ERROR, "Expected a string for format %s.", fmt_current.GetChars());
					// append
					output.AppendFormat(fmt_current.GetChars(), param[argnum].s().GetChars());
					if (!haveargnums) argnum = ++argauto;
					else argnum = -1;
					break;
				}

				// pointer
				case 'p':
				{
					if (argnum < 0 && haveargnums)
						ThrowAbortException(X_FORMAT_ERROR, "Cannot mix explicit and implicit arguments.");
					in_fmt = false;
					// fail if something was found, but it's not a string
					if (argnum >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
					if (va_reginfo[argnum] != REGT_POINTER) ThrowAbortException(X_FORMAT_ERROR, "Expected a pointer for format %s.", fmt_current.GetChars());
					// append
					output.AppendFormat(fmt_current.GetChars(), param[argnum].a);
					if (!haveargnums) argnum = ++argauto;
					else argnum = -1;
					break;
				}

				// int formats (including char)
				case 'd':
				case 'i':
				case 'u':
				case 'x':
				case 'X':
				case 'o':
				case 'c':
				case 'B':
				{
					if (argnum < 0 && haveargnums)
						ThrowAbortException(X_FORMAT_ERROR, "Cannot mix explicit and implicit arguments.");
					in_fmt = false;
					// append
					if (fmt_current[1] == '*' || fmt_current[2] == '*')
					{
						// fail if something was found, but it's not an int
						if (argnum+1 >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
						if (va_reginfo[argnum] != REGT_INT &&
							va_reginfo[argnum] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());
						if (va_reginfo[argnum+1] != REGT_INT &&
							va_reginfo[argnum+1] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());

						output.AppendFormat(fmt_current.GetChars(), param[argnum].ToInt(va_reginfo[argnum]), param[argnum + 1].ToInt(va_reginfo[argnum + 1]));
						argauto++;
					}
					else
					{
						// fail if something was found, but it's not an int
						if (argnum >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
						if (va_reginfo[argnum] != REGT_INT &&
							va_reginfo[argnum] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());
						output.AppendFormat(fmt_current.GetChars(), param[argnum].ToInt(va_reginfo[argnum]));
					}
					if (!haveargnums) argnum = ++argauto;
					else argnum = -1;
					break;
				}

				// double formats
				case 'f':
				case 'F':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
				{
					if (argnum < 0 && haveargnums)
						ThrowAbortException(X_FORMAT_ERROR, "Cannot mix explicit and implicit arguments.");
					in_fmt = false;
					if (fmt_current[1] == '*' || fmt_current[2] == '*')
					{
						// fail if something was found, but it's not an int
						if (argnum + 1 >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
						if (va_reginfo[argnum] != REGT_INT &&
							va_reginfo[argnum] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());
						if (va_reginfo[argnum + 1] != REGT_INT &&
							va_reginfo[argnum + 1] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());

						output.AppendFormat(fmt_current.GetChars(), param[argnum].ToInt(va_reginfo[argnum]), param[argnum + 1].ToDouble(va_reginfo[argnum + 1]));
						argauto++;
					}
					else
					{
						// fail if something was found, but it's not a float
						if (argnum >= numparam) ThrowAbortException(X_FORMAT_ERROR, "Not enough arguments for format.");
						if (va_reginfo[argnum] != REGT_INT &&
							va_reginfo[argnum] != REGT_FLOAT) ThrowAbortException(X_FORMAT_ERROR, "Expected a numeric value for format %s.", fmt_current.GetChars());
						// append
						output.AppendFormat(fmt_current.GetChars(), param[argnum].ToDouble(va_reginfo[argnum]));
					}
					if (!haveargnums) argnum = ++argauto;
					else argnum = -1;
					break;
				}

				default:
					// invalid character
					output += fmt_current;
					in_fmt = false;
					break;
				}
			}
		}
		else
		{
			if (c == '%')
			{
				if (i + 1 < fmtstring.Len() && fmtstring[i + 1] == '%')
				{
					output += '%';
					i++;
				}
				else
				{
					in_fmt = true;
					fmt_current = "%";
				}
			}
			else
			{
				output += c;
			}
		}
	}

	return output;
}

DEFINE_ACTION_FUNCTION(FStringStruct, Format)
{
	PARAM_PROLOGUE;
	FString s = FStringFormat(VM_ARGS_NAMES);
	ACTION_RETURN_STRING(s);
}

DEFINE_ACTION_FUNCTION(FStringStruct, AppendFormat)
{
	PARAM_SELF_STRUCT_PROLOGUE(FString);
	// first parameter is the self pointer
	FString s = FStringFormat(VM_ARGS_NAMES, 1);
	(*self) += s;
	return 0;
}
