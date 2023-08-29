#pragma once

#define BBDECL extern "C" 

BBDECL int OpenSteam(int gameID);
BBDECL void *GetSteamEvent();
BBDECL void CloseSteam();

BBDECL char* ReadSteam();

BBDECL void FindNumberOfCurrentPlayers();

BBDECL int GetSteamStat(const char *id);
BBDECL void SetSteamStat(const char *id,int value);
BBDECL int GetSteamAchievement(char *id);
BBDECL void SetSteamAchievement(const char *id);
BBDECL void ClearSteamAchievement(const char *id);
BBDECL int GetSteamAchievementIcon(const char *pchName);
BBDECL const char *GetAchievementDisplayAttribute( const char *pchName, const char *pchKey );

/*
* 
BBDECL int IndicateAchievementProgress( const char *pchName, uint32 nCurProgress, uint32 nMaxProgress );
BBDECL void FindOrCreateLeaderboard( char *name, ELeaderboardSortMethod sortmethod, ELeaderboardDisplayType displaytype );
BBDECL void FindLeaderboard( char *name );
BBDECL void UploadLeaderboardScore( ELeaderboardUploadScoreMethod method, int32 score, char *details );
BBDECL void DownloadLeaderboardEntries( ELeaderboardDataRequest request, int start, int end );
*/