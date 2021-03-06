/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>
#include <game/generated/nethash.cpp>
#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif

bool CheckClientID(int ClientID);
char* TimerType(int TimerType);

void CGameContext::ConRescue(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	char aBuf[128];
	if (!g_Config.m_SvRescue)
		return;
	if(!pPlayer)
		return;
	if(pPlayer->GetTeam()!=TEAM_SPECTATORS)
	{
		CCharacter* pChr = pPlayer->GetCharacter();
		if(pChr && pChr->m_SavedPos && pChr->m_FreezeTime)
		{
			if(!(pChr->m_SavedPos == vec2(0,0)) && pChr->m_FreezeTime!=0)
			{
				pChr->m_PrevPos = pChr->m_SavedPos;//TIGROW edit
				pChr->Core()->m_Pos = pChr->m_SavedPos;
				pChr->m_RescueUnfreeze = 1;
				pSelf->CreatePlayerSpawn(pChr->Core()->m_Pos);
				pChr->UnFreeze();

			}
		}
		else pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rescue", "You are not freezed!");
	}
	//rescue for dummy. TODO: good coders never write so much
	if (pSelf ->m_apPlayers[ClientID]->m_HasDummy &&
		pSelf ->m_apPlayers[DummyID] && 
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true &&
		(pSelf ->m_apPlayers[DummyID]->m_DummyUnderControl || 
		pSelf ->m_apPlayers[DummyID]->m_DummyCopyMove))
	{
		if (pSelf->m_apPlayers[DummyID]->GetCharacter() &&
			pSelf->m_apPlayers[DummyID]->GetCharacter()->m_SavedPos &&
			pSelf->m_apPlayers[DummyID]->GetCharacter()->m_FreezeTime)
			if(!(pSelf->m_apPlayers[DummyID]->GetCharacter()->m_SavedPos == vec2(0,0)) && 
				pSelf->m_apPlayers[DummyID]->GetCharacter()->m_FreezeTime!=0)
			{
				pSelf->m_apPlayers[DummyID]->GetCharacter()->m_PrevPos = pSelf->m_apPlayers[DummyID]->GetCharacter()->m_SavedPos;//TIGROW edit
				pSelf->m_apPlayers[DummyID]->GetCharacter()->Core()->m_Pos = pSelf->m_apPlayers[DummyID]->GetCharacter()->m_SavedPos;
				pSelf->m_apPlayers[DummyID]->GetCharacter()->m_RescueUnfreeze = 1;
				pSelf->CreatePlayerSpawn(pSelf->m_apPlayers[DummyID]->GetCharacter()->Core()->m_Pos);
				pSelf->m_apPlayers[DummyID]->GetCharacter()->UnFreeze();
			}
	}
}
void CGameContext::ConDummy(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if(!pPlayer)
		return;
	//if client is on slot for dummies when dummies are on
	if (g_Config.m_SvDummies && ClientID >= 8)
	{
		pSelf ->OnClientDrop(ClientID, "Clearing free slots for dummies");
		return;
	}

	if (pPlayer->m_HasDummy) 
	{
		if(!g_Config.m_SvDummy)
			return;
		CCharacter* pChr = pPlayer->GetCharacter();
		if(pPlayer->GetTeam()!=TEAM_SPECTATORS &&
		pPlayer->m_Paused == CPlayer::PAUSED_NONE &&
		pChr && pSelf ->m_apPlayers[DummyID] && 
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true &&
		pSelf ->m_apPlayers[DummyID]->GetTeam()!=TEAM_SPECTATORS &&
		pSelf ->m_apPlayers[DummyID]->m_Paused == CPlayer::PAUSED_NONE &&
		((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(ClientID) == ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(DummyID))
		{
			if(pPlayer->m_Last_Dummy + pSelf->Server()->TickSpeed() * g_Config.m_SvDummyDelay/2 <= pSelf->Server()->Tick()) 
			{
				pSelf->CreatePlayerSpawn(pSelf->GetPlayerChar(DummyID)->Core()->m_Pos);
				pSelf->GetPlayerChar(DummyID)->m_PrevPos = pSelf->m_apPlayers[ClientID]->m_ViewPos;//TIGROW edit
				pSelf->GetPlayerChar(DummyID)->Core()->m_Pos = pSelf->m_apPlayers[ClientID]->m_ViewPos;
				pPlayer->m_Last_Dummy = pSelf->Server()->Tick();
				pSelf->GetPlayerChar(DummyID)->m_DDRaceState = DDRACE_STARTED; //important
			}
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dummy", "You can\'t /dummy that often.");			
		}
	}
	else
	{
		if(!g_Config.m_SvDummies)
			return;
		if(pSelf ->m_apPlayers[DummyID])
		{
			pSelf ->OnClientDrop(DummyID, "Cleared slot for dummy");
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit", "Sorry, there are some problems. Retry in a few seconds.");
			return;
		}
		pSelf ->m_apPlayers[ClientID]->m_HasDummy = true;
		pSelf ->OnClientConnected(DummyID);
		pSelf ->m_apPlayers[DummyID]->m_IsDummy = true;
		//let's set name for dummy
		char buf[512];
		str_format(buf, sizeof(buf), "[D] %s", pSelf ->Server()->ClientName(ClientID));
		str_copy(pSelf ->m_apPlayers[DummyID]->m_DummyName, buf, MAX_NAME_LENGTH);
		char chatmsgbuf[512];
		str_format(chatmsgbuf, sizeof(chatmsgbuf), "%s called dummy.", pSelf ->Server()->ClientName(ClientID));
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, chatmsgbuf);
	}
}
void CGameContext::ConDummyDelete(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
    CCharacter* pChr = pSelf->GetPlayerChar(ClientID); 
 
    if (!pSelf ->m_apPlayers[ClientID]->m_HasDummy || !pSelf ->m_apPlayers[DummyID])
            return;
    
	pSelf ->m_apPlayers[DummyID]->m_IsDummy = false;
	pSelf ->OnClientDrop(DummyID, "Dummy deleted");
	g_Config.m_SvReservedSlots = 0;
    pSelf ->m_apPlayers[ClientID]->m_HasDummy = false;
}
void CGameContext::ConDummyChange(IConsole::IResult *pResult, void *pUserData)
{
	if (!g_Config.m_SvDummyChange)
		return;

	CGameContext *m_pGameServer;
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if(!pPlayer)
		return;   
	if (pPlayer->m_HasDummy && 
		pPlayer->GetTeam()!=TEAM_SPECTATORS && 
		pPlayer->m_Paused == CPlayer::PAUSED_NONE)
	{
		CCharacter* pChr = pPlayer->GetCharacter();
		if(pChr && pSelf ->m_apPlayers[DummyID] && 
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true &&
		pSelf ->m_apPlayers[DummyID]->GetTeam()!=TEAM_SPECTATORS &&
		pSelf ->m_apPlayers[DummyID]->m_Paused == CPlayer::PAUSED_NONE &&
		((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(ClientID) == ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(DummyID))
		{
			CCharacter* pDumChr = pSelf->m_apPlayers[DummyID]->GetCharacter();
			if(pDumChr->m_TileIndex == TILE_END || pDumChr->m_TileFIndex == TILE_END)
				return;
			if(pPlayer->m_Last_DummyChange + pSelf->Server()->TickSpeed() * g_Config.m_SvDummyChangeDelay/2 <= pSelf->Server()->Tick()) 
			{
				if(pDumChr->m_TileFIndex == TILE_FREEZE || pDumChr->m_TileIndex == TILE_FREEZE)
				{
					pChr->m_FreezeTime = pDumChr->m_FreezeTime;
				}

				//if ddrace not started and we swap, it can mean that player want to pass start without time
				if(pChr->m_DDRaceState != DDRACE_STARTED)
				{
					pChr->m_SavedPos = vec2(0,0); //so we clear his /r saved pos
					pPlayer->m_DisableTeams = true; //and not allow him enter teams (prevent cheating)
				}
				pChr->m_ChangePos = pSelf->m_apPlayers[DummyID]->m_ViewPos;
				pSelf->CreatePlayerSpawn(pDumChr->Core()->m_Pos);
				pDumChr->m_PrevPos = pChr->Core()->m_Pos;//TIGROW edit
				pDumChr->Core()->m_Pos = pSelf->m_apPlayers[ClientID]->m_ViewPos;
				pSelf->CreatePlayerSpawn(pChr->Core()->m_Pos);
				pChr->m_PrevPos = pChr->m_ChangePos;//TIGROW edit
				pSelf->GetPlayerChar(ClientID)->Core()->m_Pos = pChr->m_ChangePos;
				pPlayer->m_Last_DummyChange = pSelf->Server()->Tick();
				pDumChr->m_DDRaceState = DDRACE_STARTED; //important
			}
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dummy change", "You can\'t /dummy_change that often.");
		}
	}
}
void CGameContext::ConDummyHammer(IConsole::IResult *pResult, void *pUserData)
{
	if (!g_Config.m_SvDummyHammer)
		return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
	if (pSelf ->m_apPlayers[ClientID]->m_HasDummy &&
		pSelf ->m_apPlayers[DummyID] && 
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true)
	{
		if (pSelf->GetPlayerChar(DummyID)->DoHammerFly)
		{
			pSelf->GetPlayerChar(DummyID)->DoHammerFly = false;
		}
		else
		{
			pSelf->GetPlayerChar(DummyID)->DoHammerFly = true;
		}
	}
	else 
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dummy hammer", "You haven't got Dummy yet! Write /dummy to get it.");
	}
}
void CGameContext::ConDummyControl(IConsole::IResult *pResult, void *pUserData)
{
	if (!g_Config.m_SvControlDummy)
		return;
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;
	char aBuf[128];

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (pSelf ->m_apPlayers[ClientID]->m_HasDummy &&
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true)
	{
		if(pPlayer->m_Paused == CPlayer::PAUSED_PAUSED)
		{
			pSelf ->m_apPlayers[DummyID]->m_DummyUnderControl = false;
			pPlayer->m_Paused = CPlayer::PAUSED_NONE;
		}
		if(pPlayer->m_Paused == CPlayer::PAUSED_NONE)
		{
			pSelf ->m_apPlayers[DummyID]->m_DummyUnderControl = true;
			pSelf->GetPlayerChar(DummyID)->m_DDRaceState = DDRACE_STARTED; //important
			pPlayer->m_SpectatorID = (DummyID);
			pPlayer->m_Paused = CPlayer::PAUSED_PAUSED;
		}
	}
	else 
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "control dummy", "You haven't got Dummy yet! Write /dummy to get it.");
	}
	
}
void CGameContext::ConDummyCopyMove(IConsole::IResult *pResult, void *pUserData)
{
	if (!g_Config.m_SvDummyCopyMove)
		return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	int ClientID = pResult->m_ClientID;
	int DummyID = 15 - ClientID;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	char aBuf[128];
	if (pSelf ->m_apPlayers[ClientID]->m_HasDummy &&
		pSelf->GetPlayerChar(DummyID) &&
		pSelf->GetPlayerChar(DummyID)->DummyIsReady == true &&
		((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(ClientID) == ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(DummyID))
	{
		if (pSelf ->m_apPlayers[DummyID]->m_DummyUnderControl)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dcm", "You are controlling dummy right now. Write /control_dummy again to be able to use /dummy_copy_move.");
		}
		else if(pPlayer->GetTeam()==TEAM_SPECTATORS || pPlayer->m_Paused != CPlayer::PAUSED_NONE)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dcm", "You are not in game! Esc -> Join game.");
		}
		else if (pSelf ->m_apPlayers[DummyID]->m_DummyCopyMove)
			pSelf ->m_apPlayers[DummyID]->m_DummyCopyMove = false;
		else
			pSelf ->m_apPlayers[DummyID]->m_DummyCopyMove = true;
	}	
	else 
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dcm", "You haven't got Dummy yet! Write /dummy to get it.");
	}
}

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Teeworlds Team takes most of the credits also");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"This mod was originally created by \'3DA\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Now it is maintained & re-coded by:");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"\'[Egypt]GreYFoX@GTi\' and \'[BlackTee]den\'");
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"credit",
			"Others Helping on the code: \'heinrich5991\', \'ravomavain\', \'Trust o_0 Aeeeh ?!\', \'noother\', \'<3 fisted <3\' & \'LemonFace\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Documentation: Zeta-Hoernchen & Learath2, Entities: Fisico");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Code (in the past): \'3DA\' and \'Fluxid\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Please check the changelog on DDRace.info.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Also the commit log on github.com/GreYFoX/teeworlds .");
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"DDRace Mod. Version: " GAME_VERSION);
#if defined( GIT_SHORTREV_HASH )
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Git revision hash: " GIT_SHORTREV_HASH);
#endif
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Official site: DDRace.info");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"For more Info /cmdlist");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"iDDRace edit: iDDRace.iPod-Clan.com");
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/cmdlist will show a list of all chat commands");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/help + any command will show you the help for this command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"Example /help settings will display the help about ");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		const IConsole::CCommandInfo *pCmdInfo =
				pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER, false);
		if (pCmdInfo && pCmdInfo->m_pHelp)
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
					pCmdInfo->m_pHelp);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"help",
					"Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConSettings(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"setting",
				"to check a server setting say /settings and setting's name, setting names are:");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"teams, cheats, collision, hooking, endlesshooking, me, ");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"hitting, oldlaser, timeout, votes, pause and scores");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		char aBuf[256];
		float ColTemp;
		float HookTemp;
		pSelf->m_Tuning.Get("player_collision", &ColTemp);
		pSelf->m_Tuning.Get("player_hooking", &HookTemp);
		if (str_comp(pArg, "teams") == 0)
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"%s %s",
					g_Config.m_SvTeam == 1 ?
							"Teams are available on this server" :
							!g_Config.m_SvTeam ?
									"Teams are not available on this server" :
									"You have to be in a team to play on this server", /*g_Config.m_SvTeamStrict ? "and if you die in a team all of you die" : */
									"and if you die in a team only you die");
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "collision") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					ColTemp ?
							"Players can collide on this server" :
							"Players Can't collide on this server");
		}
		else if (str_comp(pArg, "hooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					HookTemp ?
							"Players can hook each other on this server" :
							"Players Can't hook each other on this server");
		}
		else if (str_comp(pArg, "endlesshooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvEndlessDrag ?
							"Players can hook time is unlimited" :
							"Players can hook time is limited");
		}
		else if (str_comp(pArg, "hitting") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHit ?
							"Player's weapons affect each other" :
							"Player's weapons has no affect on each other");
		}
		else if (str_comp(pArg, "oldlaser") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvOldLaser ?
							"Lasers can hit you if you shot them and that they pull you towards the bounce origin (Like DDRace Beta)" :
							"Lasers can't hit you if you shot them, and they pull others towards the shooter");
		}
		else if (str_comp(pArg, "me") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvSlashMe ?
							"Players can use /me commands the famous IRC Command" :
							"Players Can't use the /me command");
		}
		else if (str_comp(pArg, "timeout") == 0)
		{
			str_format(aBuf, sizeof(aBuf),
					"The Server Timeout is currently set to %d",
					g_Config.m_ConnTimeout);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "votes") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKick ?
							"Players can use Callvote menu tab to kick offenders" :
							"Players Can't use the Callvote menu tab to kick offenders");
			if (g_Config.m_SvVoteKick)
				str_format(
						aBuf,
						sizeof(aBuf),
						"Players are banned for %d second(s) if they get voted off",
						g_Config.m_SvVoteKickBantime);
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKickBantime ?
							aBuf :
							"Players are just kicked and not banned if they get voted off");
		}
		else if (str_comp(pArg, "pause") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvPauseable ?
							g_Config.m_SvPauseTime ?
									"/pause is available on this server and it pauses your time too" :
									"/pause is available on this server but it doesn't pause your time"
									:"/pause is NOT available on this server");
		}
		else if (str_comp(pArg, "scores") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHideScore ?
							"Scores are private on this server" :
							"Scores are public on this server");
		}
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	bool Printed = false;
	if (g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No blocking.");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No insulting / spamming.");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No fun voting / vote spamming.");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"rules",
				"Breaking any of these rules will result in a penalty, decided by server admins.");
		Printed = true;
	}
	if (g_Config.m_SvRulesLine1[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine1);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine2[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine2);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine3[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine3);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine4[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine4);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine5[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine5);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine6[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine6);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine7[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine7);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine8[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine8);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine9[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine9);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine10[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine10);
		Printed = true;
	}
	if (!Printed)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No Rules Defined, Kill em all!!");
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	char aBuf[128];

	if(!g_Config.m_SvPauseable)
	{
		ConToggleSpec(pResult, pUserData);
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause",
	"You can't pause while you are dead/a spectator.");
	return;
	}
	if (pPlayer->m_Paused == CPlayer::PAUSED_SPEC && g_Config.m_SvPauseable)
	{
		ConToggleSpec(pResult, pUserData);
		return;
	}

	if (pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-paused. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_PAUSED) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_PAUSED;
}

void CGameContext::ConToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	char aBuf[128];

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec",
	"You can't spec while you are dead/a spectator.");
	return;
	}

	if(pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-paused. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_SPEC) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_SPEC;
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "top5",
				"Showing the top 5 is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0 && pResult->GetInteger(0) >= 0)
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

#if defined(CONF_SQL)
void CGameContext::ConTimes(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckClientID(pResult->m_ClientID)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if(g_Config.m_SvUseSQL)
	{
		CSqlScore *pScore = (CSqlScore *)pSelf->Score();
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if(!pPlayer)
			return;

		if(pResult->NumArguments() == 0)
		{
			pScore->ShowTimes(pPlayer->GetCID(),1);
			return;
		}

		else if(pResult->NumArguments() < 3)
		{
			if (pResult->NumArguments() == 1)
			{
				if(pResult->GetInteger(0) != 0)
					pScore->ShowTimes(pPlayer->GetCID(),pResult->GetInteger(0));
				else
					pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),1);
				return;
			}
			else if (pResult->GetInteger(1) != 0)
			{
				pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),pResult->GetInteger(1));
				return;
			}
		}

		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "/times needs 0, 1 or 2 parameter. 1. = name, 2. = start number");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Example: /times, /times me, /times Hans, /times \"Papa Smurf\" 5");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Bad: /times Papa Smurf 5 # Good: /times \"Papa Smurf\" 5 ");

#if defined(CONF_SQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
	}
}
#endif

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowRank(pResult->m_ClientID, pResult->GetString(0),
					true);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"rank",
					"Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];

	if (pSelf->m_VoteCloseTime && pSelf->m_VoteCreator == pResult->m_ClientID)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You are running a vote please try again after the vote is done!");
		return;
	}
	else if (g_Config.m_SvTeam == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
				"Admin has disabled teams");
		return;
	}
	else if (g_Config.m_SvTeam == 2 && pResult->GetInteger(0) == 0 && pPlayer->GetCharacter()->m_LastStartWarning < pSelf->Server()->Tick() - 3 * pSelf->Server()->TickSpeed())
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You must join a team and play with somebody or else you can\'t play");
		pPlayer->GetCharacter()->m_LastStartWarning = pSelf->Server()->Tick();
	}

	if (pResult->NumArguments() > 0)
	{
		if (pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					"You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if (pPlayer->m_Last_Team
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvTeamChangeDelay
					> pSelf->Server()->Tick())
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You can\'t change teams that fast!");
			}
			else if(pPlayer->m_DisableTeams == true)
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"Are you going to cheat with time? :P");
			}
			else if (((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.SetCharacterTeam(
					pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d",
						pSelf->Server()->ClientName(pPlayer->GetCID()),
						pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				pPlayer->m_Last_Team = pSelf->Server()->Tick();
			}
			else
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You cannot join this team at this time");
			}
		}
	}
	else
	{
		char aBuf[512];
		if (!pPlayer->IsPlaying())
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"join",
					"You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"You are in team %d",
					((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(
							pResult->m_ClientID));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					aBuf);
		}
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "'%s' %s",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			pResult->GetString(0));
	if (g_Config.m_SvSlashMe)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, pResult->m_ClientID);
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"me",
				"/me is disabled on this server, admin can enable it by using sv_slash_me");
}

void CGameContext::ConSetEyeEmote(IConsole::IResult *pResult,
		void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				(pPlayer->m_EyeEmote) ?
						"You can now use the preset eye emotes." :
						"You don't have any eye emotes, remember to bind some. (until you die)");
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "on") == 0)
		pPlayer->m_EyeEmote = true;
	else if(str_comp_nocase(pResult->GetString(0), "off") == 0)
		pPlayer->m_EyeEmote = false;
	else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		pPlayer->m_EyeEmote = !pPlayer->m_EyeEmote;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"emote",
			(pPlayer->m_EyeEmote) ?
					"You can now use the preset eye emotes." :
					"You don't have any eye emotes, remember to bind some. (until you die)");
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (g_Config.m_SvEmotionalTees == -1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "emote",
				"Server admin disabled emotes.");
		return;
	}

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else
	{
			if(pPlayer->m_LastEyeEmote + g_Config.m_SvEyeEmoteChangeDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;

			if (!str_comp(pResult->GetString(0), "angry"))
				pPlayer->m_DefEmote = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
				pPlayer->m_DefEmote = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
				pPlayer->m_DefEmote = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
				pPlayer->m_DefEmote = EMOTE_SURPRISE;
			else if (!str_comp(pResult->GetString(0), "normal"))
				pPlayer->m_DefEmote = EMOTE_NORMAL;
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,
						"emote", "Unknown emote... Say /emote");

			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);

			pPlayer->m_DefEmoteReset = pSelf->Server()->Tick()
							+ Duration * pSelf->Server()->TickSpeed();
			pPlayer->m_LastEyeEmote = pSelf->Server()->Tick();
	}
}

void CGameContext::ConShowOthers(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (g_Config.m_SvShowOthers)
	{
		if (pPlayer->m_IsUsingDDRaceClient)
		{
			if (pResult->NumArguments())
				pPlayer->m_ShowOthers = pResult->GetInteger(0);
			else
				pPlayer->m_ShowOthers = !pPlayer->m_ShowOthers;
		}
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"showotherschat",
					"Showing players from other teams is only available with DDRace Client, http://DDRace.info");
	}
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"showotherschat",
				"Showing players from other teams is disabled by the server admin");
}

bool CheckClientID(int ClientID)
{
	dbg_assert(ClientID >= 0 || ClientID < MAX_CLIENTS,
			"The Client ID is wrong");
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	return true;
}

char* TimerType(int TimerType)
{
	char msg[3][128] = {"game/round timer.", "broadcast.", "both game/round timer and broadcast."};
	return msg[TimerType];
}
void CGameContext::ConSayTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your Time is %s%d:%s%d",
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "time", aBuftime);
}

void CGameContext::ConSayTimeAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime),
			"%s\'s current race time is %s%d:%s%d",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuftime, pResult->m_ClientID);
}

void CGameContext::ConTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your Time is %s%d:%s%d",
				((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
				((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendBroadcast(aBuftime, pResult->m_ClientID);
}

void CGameContext::ConSetTimerType(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[128];
	if(pPlayer->m_TimerType <= 2 && pPlayer->m_TimerType >= 0)
		str_format(aBuf, sizeof(aBuf), "Timer is displayed in", TimerType(pPlayer->m_TimerType));
	else if(pPlayer->m_TimerType == 3)
		str_format(aBuf, sizeof(aBuf), "Timer isn't displayed.");

	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "gametimer") == 0) {
		pSelf->SendBroadcast("", pResult->m_ClientID);
		pPlayer->m_TimerType = 0;
	}
	else if(str_comp_nocase(pResult->GetString(0), "broadcast") == 0)
			pPlayer->m_TimerType = 1;
	else if(str_comp_nocase(pResult->GetString(0), "both") == 0)
			pPlayer->m_TimerType = 2;
	else if(str_comp_nocase(pResult->GetString(0), "none") == 0)
			pPlayer->m_TimerType = 3;
	else if(str_comp_nocase(pResult->GetString(0), "cycle") == 0) {
		if(pPlayer->m_TimerType < 3)
			pPlayer->m_TimerType++;
		else if(pPlayer->m_TimerType == 3)
			pPlayer->m_TimerType = 0;
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
}

