// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
#include <iostream>
#include "bass.h"
#include "basshls.h"
#include "SimpleIni/SimpleIni.h"
#include "SimpleIni/ConvertUTF.h"
using namespace std;

#pragma warning(disable:4996)

string CH[10];
float legacyvolume, valprevvol;
float val[10];
int started, legacy, valprev, tapemode;
HSTREAM streamprev;

void loadconfigfile() {
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(".\\plugins\\SuperRadio.opl");
	if (rc >= 0) {
		CH[0] = ini.GetValue("Radio", "TAPE");
		CH[1] = ini.GetValue("Radio", "CH1");
		CH[2] = ini.GetValue("Radio", "CH2");
		CH[3] = ini.GetValue("Radio", "CH3");
		CH[4] = ini.GetValue("Radio", "CH4");
		CH[5] = ini.GetValue("Radio", "CH5");
		CH[6] = ini.GetValue("Radio", "CH6");
		CH[7] = ini.GetValue("Radio", "CH7");
		CH[8] = ini.GetValue("Radio", "CH8");
		CH[9] = ini.GetValue("Radio", "CH9");
		legacyvolume = atof(ini.GetValue("Radio", "legacyvolume"));
	}


}


wchar_t* chartow(const char* str, int codePage = GetACP())
{
	int size_needed = MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), NULL, 0);
	wchar_t *wstrTo = new wchar_t[size_needed];
	MultiByteToWideChar(codePage, 0, str, (int)sizeof(str), wstrTo, size_needed);
	return wstrTo;
}

void play_music(char* URL, float volume, bool local = false) {
	BASS_ChannelStop(streamprev);
	HSTREAM stream = NULL;
	if (local) {
		/*
		stream = BASS_StreamCreateFile(false, URL, 0, 0, BASS_STREAM_AUTOFREE);
#ifdef _DEBUG
		cout << "Local" << endl;
		cout << URL << endl;
		if (stream == 0) {
			cout << BASS_ErrorGetCode() << endl;
		}
#endif
*/
	} else {
		stream = BASS_StreamCreateURL(URL, 0, BASS_STREAM_AUTOFREE, NULL, NULL);
#ifdef _DEBUG
		cout << URL << endl;
#endif
		if (stream == 0) {
			if (BASS_ErrorGetCode() == BASS_ERROR_FILEFORM)
				stream = BASS_HLS_StreamCreateURL(URL, BASS_STREAM_AUTOFREE, NULL, NULL);
		}
	}
	streamprev = stream;
	BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, volume);
	BASS_ChannelPlay(stream, 1);
	started = 1;
}

	void __stdcall PluginStart(HWND aOwner)
	{
		loadconfigfile();
		BASS_Init(-1, 44100, 0, aOwner, NULL);
#ifdef _DEBUG
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
#endif
	}


	void __stdcall AccessVariable(unsigned short varindex, float* value, bool* write)
	{
		val[varindex] = (float)*value;
		
		if (val[2] != valprevvol) {
			valprevvol = val[2];
			BASS_ChannelSetAttribute(streamprev, BASS_ATTRIB_VOL, val[2]);
		}

		if (val[0] < 1 && val[1] < 1 || val[1] != valprev || val[3] != tapemode) {
#ifdef _DEBUG
			if(started)
				cout << "Channel stopped" << endl;
#endif
			if (started)
				BASS_ChannelStop(streamprev); started = 0; legacy = 0; valprev = val[1]; tapemode = val[3];
		}else {
			if (val[0] == 1 && started == 0) {
				legacy = 1;
				tapemode = 0;
				play_music((char*)CH[0].c_str(),legacyvolume);
#ifdef _DEBUG
				cout << "Channel started (legacy mode)" << endl;
#endif
			}
			else if (val[1] >= 1 && started == 0 && tapemode < 1) {
				legacy = 0;
				play_music((char*)CH[(int)val[1]].c_str(), val[2]);
#ifdef _DEBUG
				cout << "Channel started" << endl;
#endif
			}
			else if (val[1] >= 1 && val[3] >= 1 && tapemode >= 1) {
				legacy = 0;
#ifdef _DEBUG
				//cout << "Tapemode enabled" << endl;
#endif
				play_music((char*)CH[0].c_str(), val[2],true);
			}
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



	void __stdcall PluginFinalize()
	{
#ifdef _DEBUG
		FreeConsole();
#endif
	}
