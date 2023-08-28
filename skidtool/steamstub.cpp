#include <stdio.h>
#include <string>

#include <steam/steam_api.h>

#include "steamstub.h"

/*
#define BBDECL extern "C" 

class SteamEvent;

BBDECL int OpenSteam(int gameID);
BBDECL SteamEvent *GetSteamEvent();
BBDECL void CloseSteam();

BBDECL void FindNumberOfCurrentPlayers();

BBDECL int GetSteamStat(const char *id);
BBDECL void SetSteamStat(const char *id,int32 value);
BBDECL int GetSteamAchievement(char *id);
BBDECL void SetSteamAchievement(const char *id);
BBDECL void ClearSteamAchievement(const char *id);
BBDECL int GetSteamAchievementIcon(const char *pchName);
BBDECL const char *GetAchievementDisplayAttribute( const char *pchName, const char *pchKey );
BBDECL int IndicateAchievementProgress( const char *pchName, uint32 nCurProgress, uint32 nMaxProgress );
BBDECL void FindOrCreateLeaderboard( char *name, ELeaderboardSortMethod sortmethod, ELeaderboardDisplayType displaytype );
BBDECL void FindLeaderboard( char *name );
BBDECL void UploadLeaderboardScore( ELeaderboardUploadScoreMethod method, int32 score, char *details );
BBDECL void DownloadLeaderboardEntries( ELeaderboardDataRequest request, int start, int end );
*/

extern "C" long long __umoddi3( long long x, long long y) ;
extern "C" long long __udivdi3( long long x, long long y) ;

long long __umoddi3( long long x, long long y) { 
	printf("bleh");
	fflush(stdout);
	return x%y; 
} 

long long __udivdi3( long long x, long long y) { 
	printf("bleh");
	fflush(stdout);
	return x/y; 
} 

// _declspec(dllexport)

SteamAPICall_t call = 0;
SteamLeaderboard_t handle=0;
std::string steamlog;

class SteamStub
{
public:
	CGameID m_GameID;

	SteamStub( int32 gameID );

	STEAM_CALLBACK( SteamStub, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
	STEAM_CALLBACK( SteamStub, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
	STEAM_CALLBACK( SteamStub, OnAchievementStored, UserAchievementStored_t, m_CallbackAchievementStored );
	STEAM_CALLBACK( SteamStub, OnFindScores, LeaderboardFindResult_t, m_CallbackFindScores );
	STEAM_CALLBACK( SteamStub, OnDownload, LeaderboardScoresDownloaded_t, m_CallbackDownloaded );
	STEAM_CALLBACK( SteamStub, OnUpload, LeaderboardScoreUploaded_t, m_CallbackUploaded );
	STEAM_CALLBACK( SteamStub, OnCount, NumberOfCurrentPlayers_t, m_CallbackCount );

	// Called when SteamUserStats()->FindOrCreateLeaderboard() returns asynchronously
	// Called when SteamUserStats()->UploadLeaderboardScore() returns asynchronously

	void OnFindLeaderboard( LeaderboardFindResult_t *pFindLearderboardResult, bool bIOFailure );
	void OnUploadScore( LeaderboardScoreUploaded_t *pFindLearderboardResult, bool bIOFailure );

	CCallResult<SteamStub, LeaderboardFindResult_t> m_SteamCallResultCreateLeaderboard;
	CCallResult<SteamStub, LeaderboardScoreUploaded_t> m_SteamCallResultUploadScore;
};

SteamStub::SteamStub( int32 gameID ):
	m_CallbackUserStatsReceived( this, &SteamStub::OnUserStatsReceived ),
	m_CallbackUserStatsStored( this, &SteamStub::OnUserStatsStored ),
	m_CallbackAchievementStored( this, &SteamStub::OnAchievementStored ),
	
	m_CallbackFindScores( this, &SteamStub::OnFindScores ),
	m_CallbackDownloaded( this, &SteamStub::OnDownload ),
	m_CallbackUploaded( this, &SteamStub::OnUpload ),
	m_CallbackCount( this, &SteamStub::OnCount ),
	m_GameID( gameID )
{
//	m_GameID.Set( uint32 unAccountID, EUniverse eUniverse, EAccountType eAccountType )
}

#ifdef nothanks

class SteamEvent : public Object
{	
public:
	enum SteamEventType{LOG};

	SteamEvent(SteamEventType _type,int _value,const char *_text):Object(){
		type=_type;
		value=_value;
		if(_text){	
			text=String(_text);
		}else{
			text=String("");
		}
	}

	virtual ~SteamEvent(){
		printf("streamEvent destructor"); 
	}

	String GetText(){
		return text;
	}
	
private:
	String text;
	SteamEventType type;
	int value;
};
#endif

void SteamStub::OnDownload( LeaderboardScoresDownloaded_t *download ){

	LeaderboardEntry_t entry;
	int32 details[256];
	char buffer[128];
	int n;

	_snprintf( buffer, 128, "download=%d!", download->m_cEntryCount );
	steamlog.append(buffer);
	for ( int index = 0; index < download->m_cEntryCount; index++ ){
		SteamUserStats()->GetDownloadedLeaderboardEntry( 
			download->m_hSteamLeaderboardEntries, 
			index, 
			&entry, 
			details,
			256
		);
		n=_snprintf( buffer, 128, "%d:%d:%d:", entry.m_nScore,entry.m_nGlobalRank,entry.m_steamIDUser);
		steamlog.append(buffer);
		for( int i=0;i<entry.m_cDetails;i++){
			steamlog.push_back((char)details[i]);
		}
		steamlog.append("!");
	}
}

void SteamStub::OnUpload( LeaderboardScoreUploaded_t *up ){
	char buffer[128];
	if(up->m_bSuccess && up->m_bScoreChanged){
		_snprintf( buffer, 128, "upload=%d:%d:%d!", up->m_nScore,up->m_nGlobalRankNew,up->m_nGlobalRankPrevious );
		steamlog.append(buffer);
	}
}

void SteamStub::OnFindScores( LeaderboardFindResult_t *t ){
	char buffer[128];
	if(t->m_bLeaderboardFound){
		handle=t->m_hSteamLeaderboard;
		_snprintf( buffer, 128, "found=%s:%d!", 
			SteamUserStats()->GetLeaderboardName(t->m_hSteamLeaderboard),
			SteamUserStats()->GetLeaderboardEntryCount(t->m_hSteamLeaderboard));
		steamlog.append(buffer);
	}
}


void SteamStub::OnFindLeaderboard( LeaderboardFindResult_t *pFindLearderboardResult, bool bIOFailure ){
	printf("onfindleaderboard");fflush(stdout);
}

void SteamStub::OnUploadScore( LeaderboardScoreUploaded_t *pFindLearderboardResult, bool bIOFailure ){
	printf("onfindleaderboard");fflush(stdout);
}


void SteamStub::OnCount(NumberOfCurrentPlayers_t *n){
	char buffer[128];
	_snprintf( buffer, 128, "playercount=%d!", n->m_cPlayers);
	steamlog.append(buffer);
}

void SteamStub::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			steamlog.append("stats received!");
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "stats failed %d!", pCallback->m_eResult );
			steamlog.append( buffer );
		}
	}
}


void SteamStub::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			steamlog.append("stats stored!");
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "stats stored failed %d!", pCallback->m_eResult );
			steamlog.append( buffer );
		}
	}
}

void SteamStub::OnAchievementStored( UserAchievementStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( 0 == pCallback->m_nMaxProgress )
		{
			char buffer[128];
			_snprintf( buffer, 128, "Achievement '%s' unlocked!", pCallback->m_rgchAchievementName );
			steamlog.append(buffer);
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "Achievement '%s' progress %d:%d!", 
				pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress );
			steamlog.append( buffer );
		}
	}
}





#define MAXPLAYERFETCH 256
#define BUFFERSIZE 4096+MAXPLAYERFETCH*512

char rbuffer[BUFFERSIZE];

BBDECL char *ReadSteam(){
	SteamAPI_RunCallbacks();
	_snprintf(rbuffer,BUFFERSIZE,"%s",steamlog.c_str());
	steamlog="";
	return rbuffer;
}

SteamStub *stub;

BBDECL void StoreSteamStats(){
  SteamUserStats()->StoreStats();
}

// stats 

BBDECL int GetSteamStat(const char *id){
  int32 res;
  if (!SteamUserStats()->GetStat(id,&res )){
	  return -1;
  }
  return res;
}

BBDECL void SetSteamStat(const char *id,int32 value){
	SteamUserStats()->SetStat(id,value);
}

// achieves

BBDECL int GetSteamAchievement(char *id){
  bool res;
  SteamUserStats()->GetAchievement(id,&res );
  return res?1:0;
}

BBDECL void SetSteamAchievement(const char *id){
  SteamUserStats()->SetAchievement(id);
}

BBDECL void ClearSteamAchievement(const char *id){
  SteamUserStats()->ClearAchievement(id);
}

BBDECL int GetSteamAchievementIcon(const char *pchName){
  return SteamUserStats()->GetAchievementIcon( pchName );
}
	// Get general attributes (display name / text, etc) for an Achievement
BBDECL const char *GetAchievementDisplayAttribute( const char *pchName, const char *pchKey ){
	return SteamUserStats()->GetAchievementDisplayAttribute( pchName, pchKey );
}

	// Achievement progress - triggers an AchievementProgress callback, that is all.
	// Calling this w/ N out of N progress will NOT set the achievement, the game must still do that.
	
BBDECL int IndicateAchievementProgress( const char *pchName, uint32 nCurProgress, uint32 nMaxProgress ){
	return SteamUserStats()->IndicateAchievementProgress( pchName, nCurProgress, nMaxProgress )?1:0;
}

char leadname[256];

BBDECL void FindOrCreateLeaderboard( char *name, ELeaderboardSortMethod sortmethod, ELeaderboardDisplayType displaytype ){
	handle=0;
	sprintf(leadname,name);
	call=SteamUserStats()->FindOrCreateLeaderboard(leadname,sortmethod,displaytype);
	if (call!=0){
		stub->m_SteamCallResultCreateLeaderboard.Set( call, stub, &SteamStub::OnFindLeaderboard );
	}
}

BBDECL void FindLeaderboard( char *name ){
	handle=0;
	sprintf(leadname,name);
	call=SteamUserStats()->FindLeaderboard(leadname);
	if (call!=0){
		stub->m_SteamCallResultCreateLeaderboard.Set( call, stub, &SteamStub::OnFindLeaderboard );
	}
}

BBDECL void UploadLeaderboardScore( ELeaderboardUploadScoreMethod method, int32 score, char *details ) {
	static int data[256];
	int i;
	if(handle==0){
		steamlog.append( "no leaderboard open!" );
		return;
	}

	for(i=0;i<255;i++){
		if ((data[i]=details[i])==0){
			break;
		}
	}

	data[i++]=0;
	SteamUserStats()->UploadLeaderboardScore(handle,method,score,(const int32 *)data,i);
}

BBDECL void FindNumberOfCurrentPlayers() {
	SteamUserStats()->GetNumberOfCurrentPlayers();
}

BBDECL void DownloadLeaderboardEntries( ELeaderboardDataRequest request, int start, int end ) {
	if(handle==0){
		steamlog.append( "no leaderboard open!" );
		return;
	}
	if(end-start>MAXPLAYERFETCH){
		end=start+MAXPLAYERFETCH;
	}
	SteamUserStats()->DownloadLeaderboardEntries(handle,request,start,end);
}


BBDECL int OpenSteam(int gameID){

	if(!SteamAPI_Init()){
		return -1;
	}

	stub=new SteamStub( gameID );

	CSteamID steamid;
	int loggedon;

	steamid=SteamUser()->GetSteamID();
	
	loggedon=SteamUser()->BLoggedOn();

	SteamUserStats()->RequestCurrentStats();
//	SteamUserStats()->SetStat("Player hatched",50);
//	SteamUserStats()->StoreStats();

	return loggedon?1:0;
}

BBDECL void CloseSteam(){
	SteamAPI_Shutdown();
}

#ifdef nothanks
BBDECL SteamEvent *GetSteamEvent(){

	SteamEvent *result=NULL;

	SteamAPI_RunCallbacks();

	if(steamlog.length()>0)
	{
		const char *text=steamlog.c_str();
		result=new SteamEvent(SteamEvent::LOG,0,text);
		steamlog.erase();
	}

	return result;
}
#endif

/*
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
*/