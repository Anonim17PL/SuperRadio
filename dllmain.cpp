// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
#include "bass.h"
#include "basshls.h"
#include "SimpleIni/SimpleIni.h"
#include "SimpleIni/ConvertUTF.h"
#include <thread>
#include <vector>
using namespace std;

#pragma warning(disable:4996)

char** CH;
char* Playlist_reldirpath;
vector <char*> Playlist_files;
size_t CH_count;
float val[4], pluginSetPlaylist;
float volume, legacyvolume;
int state, playlistSetState;
float playlistState;
HSTREAM BASSstream;
bool threadstop, playlistMode, writeMode;


#ifdef _DEBUG
const bool debugmode = true;
#else
	bool debugmode;
#endif

void initConsole() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	SetConsoleTitle(L"SuperRadio Debug Log");
}


void loadconfigfile() {
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(".\\plugins\\SuperRadio.opl");
	if (rc >= 0) {

#ifndef _DEBUG
		debugmode = ini.GetBoolValue("Radio", "debugmode", false);
		if (debugmode)
			initConsole();
#endif

		CH_count = ini.GetLongValue("Radio", "CHcount", 9);
		legacyvolume = atof(ini.GetValue("Radio", "legacyvolume", "0.15"));

		CH = new char*[CH_count+1];
		
		const char* ch0 = ini.GetValue("Radio", "TAPE", "NONE");
		size_t ch0Size = strlen(ch0);
		CH[0] = new char[ch0Size + 1]{0};
		memcpy(CH[0], ch0, ch0Size);

		for (size_t i = 1; i <= CH_count; i++) {
			char chstr[10];
			sprintf(chstr, "CH%i", i);
			const char* churl = ini.GetValue("Radio", chstr, "NONE");
			size_t churlSize = strlen(churl);
			CH[i] = new char[churlSize+1]{0};
			memcpy(CH[i], churl, churlSize);
		}

		//char counts[100];
		//sprintf(counts, "Loaded URLs: %i", CH_count);
		printf("Loaded URLs: %i\n", CH_count);
	}


}

/*
wchar_t* chartow(const char* str, int codePage = GetACP())
{
	int size_needed = MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), NULL, 0);
	wchar_t *wstrTo = new wchar_t[size_needed];
	MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), wstrTo, size_needed);
	return wstrTo;
}
*/

void __stdcall PluginStart(HWND aOwner)
{
#ifdef _DEBUG
	initConsole();
#endif
	bool ok = BASS_Init(-1, 44100, 0, aOwner, NULL);
	if (!ok) {
		printf("BASS failed to initialize. Error code: %i\n", BASS_ErrorGetCode());
	}else
		printf("BASS library loaded successfully\n");
	loadconfigfile();

}

void StopMusic() {
	if (state != 0) {
		playlistMode = false;
		BASS_ChannelStop(BASSstream);
		BASS_StreamFree(BASSstream);
		state = 0;
		printf("Channel stopped\n");
	}
}

void cleanupPlaylist() {
	size_t count = Playlist_files.size();
	for (size_t i = 0; i < count; i++) {
		delete[] Playlist_files[i];
	}
	Playlist_files.clear();
}

void parsePlaylist(string data) {
	size_t pos = 0;
	size_t len = data.length();
	while (pos < len) {
		size_t fnLength = data.find('\n', pos);
		if (fnLength == string::npos)
			fnLength = data.length();
		fnLength -= pos;
		char* filename = new char[fnLength + 1]{ 0 };
		memcpy(filename, data.substr(pos, fnLength).c_str(), fnLength);
		size_t commPos = strchr(filename, '#') - filename;
		int BOMPos = strchr(filename, '\xEF') - filename;
		int diskPos = strchr(filename, ':') - filename;
		if (commPos != 0 && BOMPos != 0) {
			if (diskPos > 0)
				Playlist_files.push_back(filename);
			else {
				size_t paths = strlen(Playlist_reldirpath);
				size_t newstrs = paths + fnLength + 1;
				char* filename2 = new char[newstrs] {0};
				memcpy(filename2, Playlist_reldirpath, paths);
				memcpy(filename2 + paths, filename, fnLength);
				delete[] filename;
				Playlist_files.push_back(filename2);
			}
		}
		pos += fnLength;
		pos += 1;

	}
}

void PlayURL(const char* URL) {
	if (strcmp(URL, "NONE") == 0)
		return;

	bool isfile = (strstr(URL,"f:") == URL || strstr(URL, "F:") == URL) ? true : false;
	bool isplaylist = (strstr(URL,"p:") == URL || strstr(URL, "P:") == URL) ? true : false;

	if (isplaylist) {
		printf("Local playlist detected\n");

		size_t substrLength = strrchr(URL, '\\') - URL - 1;
		delete[] Playlist_reldirpath;
		Playlist_reldirpath = new char[substrLength + 1]{ 0 };
		memcpy(Playlist_reldirpath, URL + 2, substrLength);

		const char* playlistPath = URL + 2;

		FILE* playlist;
		errno_t errf = fopen_s(&playlist, playlistPath, "r");
		if (errf != 0) {
			printf("Failed to open playlist %s\n", URL + 2);
			return;
		}

		string buff;
		size_t fcount = 100;
		size_t flen = 100;


		while (flen > 0) {
			char fbuff[101] = { 0 };
			flen = fread(fbuff, sizeof(char), fcount, playlist);
			buff += fbuff;
		}
		fclose(playlist);
		cleanupPlaylist();
		parsePlaylist(buff);

		printf("Playing channel %i: %s\n", state, URL + 2);
		playlistMode = true;
		playlistSetState = -1;
		playlistState = 1;
		return;
	}else
	if (isfile) {
		printf("Local file detected\n");
		BASSstream = BASS_StreamCreateFile(false, URL + 2, 0, 0, BASS_STREAM_AUTOFREE);
		printf("Playing channel %i: %s\n", state, URL + 2);
		BASS_ChannelPlay(BASSstream, true);
		printf("Channel started\n");
		return;
	}
	else
		BASSstream = BASS_StreamCreateURL(URL, 0, BASS_STREAM_AUTOFREE, NULL, NULL);

	if (state > 0)
		printf("Playing channel %i: %s\n",state, CH[state]);
	else
		printf("Playing channel %i: %s\n", 0, CH[0]);

	int err = 0;

	if (err = BASS_ErrorGetCode() != 0) {
		if (isfile || isplaylist) {
			printf("Fail, channel not started. Error code: %i\n",err);
			return;
		}
		printf("Fail, retry in HLS mode. Error code: %i\n", err);
		BASSstream = BASS_HLS_StreamCreateURL(URL, BASS_STREAM_AUTOFREE, NULL, NULL);
	}
	if (err = BASS_ErrorGetCode() != 0)
		printf("Fail, channel not started. Error code: %i\n",err);
	else {
		BASS_ChannelPlay(BASSstream, true);
		printf("Channel started\n");
	}
}

void StartMusic() {
	if (state == -1) {
		PlayURL(CH[0]);
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, legacyvolume);
	}
	else if (state > 0) {
		PlayURL(CH[state]);
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, volume);
	}
}

void playlistFunc() {
	DWORD num = BASS_ChannelIsActive(BASSstream);
	if ((num == BASS_ACTIVE_STALLED || num == BASS_ACTIVE_STOPPED) && !writeMode)
		playlistSetState++;

	if (val[3] != pluginSetPlaylist) {
		pluginSetPlaylist = val[3];
		playlistSetState = val[3];
	}

	int countf = Playlist_files.size();
	if (playlistSetState >= countf)
		playlistSetState = 0;
	
	if (playlistSetState < 0)
		playlistSetState = countf-1;

	if (playlistSetState != (int)playlistState) {
		BASS_ChannelStop(BASSstream);
		playlistState = playlistSetState;
		writeMode = true;
		printf("Playing file from playlist: %s\n", Playlist_files[playlistSetState]);
		BASSstream = BASS_StreamCreateFile(false, Playlist_files[playlistSetState], 0, 0, BASS_STREAM_AUTOFREE);
		BASS_ChannelPlay(BASSstream,true);
	}
}

void musicSwitcher()
{
	float setVolume = val[2];
	float volume; BASS_ChannelGetAttribute(BASSstream, BASS_ATTRIB_VOL, &volume);

	if (setVolume != volume)
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, setVolume);

	float state_tapemode = val[0];
	int state_chx = min(val[1],CH_count);

	if (state_tapemode > 0)
		state_chx = -1;

	if (state_chx != state) {
		StopMusic();
		state = state_chx;
		StartMusic();
	}

	if (playlistMode)
		playlistFunc();
}

void musicThread() {
	while (!threadstop)
		musicSwitcher();
}


void __stdcall AccessVariable(unsigned short varindex, float* value, bool* write)
{
	if (varindex < 3)
		val[varindex] = (float)*value;
	if (varindex == 3) 
		val[varindex] = (float)*value;
	if (varindex == 3 && writeMode) {
		*value = playlistState;
		*write = true;
		writeMode = false;
	}
}


void __stdcall AccessStringVariable(unsigned short varindex, wchar_t* value, bool* write)
{
}

void __stdcall AccessTrigger(unsigned short triggerindex, bool* active)
{

}

void __stdcall AccessSystemVariable(unsigned short varindex, float* value, bool* write)
{

}


std::thread mth(musicThread);

void __stdcall PluginFinalize()
{
	threadstop = true;
	mth.join();
	StopMusic();
	BASS_Free();
	cleanupPlaylist();
	for (size_t i = 0; i<=CH_count; i++)
		delete[] CH[i];
	delete[] CH;
	delete[] Playlist_reldirpath;
#ifdef _DEBUG
	FreeConsole();
#endif
}
