// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
#include <iostream>
#include "bass.h"
#include "basshls.h"
#include "SimpleIni/SimpleIni.h"
#include "SimpleIni/ConvertUTF.h"
#include <sstream>
#include <thread>
using namespace std;

#pragma warning(disable:4996)

string* CH;
size_t CH_count;
float val[3];
float volume, legacyvolume;
int state;
HSTREAM BASSstream;
bool threadstop;


#ifdef _DEBUG
const bool debugmode = true;
#else
	bool debugmode;
#endif

void printDebug(string str) {
	if (debugmode == true)
		cout << str << endl;
}

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
		const char* CH_countS = ini.GetValue("Radio", "CHcount", "9");
		CH_count = atoi(CH_countS);

		CH = new string[CH_count+1];
		
		CH[0] = ini.GetValue("Radio", "TAPE", "NONE");
		legacyvolume = atof(ini.GetValue("Radio", "legacyvolume","0.15"));
#ifndef _DEBUG
		debugmode = ini.GetBoolValue("Radio", "debugmode",false);
		if (debugmode)
			initConsole();
#endif

		for (int i = 1; i <= CH_count; i++) {
			stringstream chstr;
			chstr << "CH";
			chstr << i;
			CH[i] = ini.GetValue("Radio", chstr.str().c_str(),"NONE");
		}
		stringstream counts;
		counts << "Loaded URLs: ";
		counts << CH_countS;
		printDebug(counts.str());
	}


}

wchar_t* chartow(const char* str, int codePage = GetACP())
{
	int size_needed = MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), NULL, 0);
	wchar_t *wstrTo = new wchar_t[size_needed];
	MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), wstrTo, size_needed);
	return wstrTo;
}

void __stdcall PluginStart(HWND aOwner)
{
#ifdef _DEBUG
	initConsole();
#endif
	bool ok = BASS_Init(-1, 44100, 0, aOwner, NULL);
	if (!ok) {
		stringstream i;
		i << "Fail to init BASS. Error Code: ";
		i << BASS_ErrorGetCode();
		printDebug(i.str());
	}else
		printDebug("BASS library loaded successfully");
	loadconfigfile();

}

void StopMusic() {
	if (state != 0) {
		BASS_ChannelStop(BASSstream);
		BASS_StreamFree(BASSstream);
		state = 0;
		printDebug("Channel stopped");
	}
}

void PlayURL(string URL) {
	if (URL == "NONE")
		return;
	BASSstream = BASS_StreamCreateURL(URL.c_str(), 0, BASS_STREAM_AUTOFREE, NULL, NULL);

	if (state > 0)
		printDebug(CH[state]);
	else
		printDebug(CH[0]);

	if (BASS_ErrorGetCode() != 0) {
		printDebug("Fail, retry in HLS mode");
		BASSstream = BASS_HLS_StreamCreateURL(URL.c_str(), BASS_STREAM_AUTOFREE, NULL, NULL);
	}
	if (BASS_ErrorGetCode() != 0)
		printDebug("Fail, channel not started");
	else
		BASS_ChannelPlay(BASSstream, true);
}

void StartMusic() {
	if (state == -1) {
		PlayURL(CH[0]);
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, legacyvolume);
		printDebug("Channel started (legacy mode)");
	}
	else if (state > 0) {
		PlayURL(CH[state]);
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, volume);
		printDebug("Channel started");
	}
}

void musicSwitcher()
{
	float prevvol = val[2];
	if (prevvol != volume) {
		volume = prevvol;
		BASS_ChannelSetAttribute(BASSstream, BASS_ATTRIB_VOL, volume);
	}

	float state_tapemode = val[0];
	int state_chx = min(val[1],CH_count);

	if (state_tapemode > 0)
		state_chx = -1;

	if (state_chx != state) {
		StopMusic();
		state = state_chx;
		StartMusic();
	}
}

void musicThread() {
	while (!threadstop)
		musicSwitcher();
}


void __stdcall AccessVariable(unsigned short varindex, float* value, bool* write)
{
	val[varindex] = (float)*value;
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
	delete[] CH;
#ifdef _DEBUG
	FreeConsole();
#endif
}
