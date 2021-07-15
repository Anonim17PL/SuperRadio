// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
#include "pch.h"
#include <iostream>
#include "bass.h"
#include "SimpleIni/SimpleIni.h"
#include "SimpleIni/ConvertUTF.h"
#include <codecvt>
#include <vector>
using namespace std;

// HLS definitions (copied from BASSHLS.H)
#define BASS_SYNC_HLS_SEGMENT	0x10300
#define BASS_TAG_HLS_EXTINF		0x14000

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

string CH[10];
float legacyvolume, valprevvol;
float val[999];
int started, legacy, valprev;
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



void play_music(char* URL, float volume) {
	BASS_ChannelStop(streamprev);
	HSTREAM stream = BASS_StreamCreateURL(URL, 0, 0, NULL, 0);
	streamprev = stream;
	BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, volume);
	BASS_ChannelPlay(stream, 1);
	started = 1;
}

std::string encode(const std::wstring& wstr, int codePage = GetACP())
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(codePage, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(codePage, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wstring decode(const std::string& str, int codePage = GetACP())
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(codePage, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(codePage, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

	void __stdcall PluginStart(void* aOwner)
	{
		loadconfigfile();
		BASS_Init(-1, 44100, 0, 0, NULL);
	}


	void __stdcall AccessVariable(unsigned short varindex, float* value, bool* write)
	{
		val[varindex] = (float)*value;
		
		if (val[2] != valprevvol) {
			valprevvol = val[2];
			BASS_ChannelSetAttribute(streamprev, BASS_ATTRIB_VOL, val[2]);
		}

		if (val[0] < 1 && val[1] < 1 || val[1] != valprev) {
			BASS_ChannelStop(streamprev); started = 0; legacy = 0; valprev = val[1];
		}else {
			if (val[0] == 1 && started == 0) {
				legacy = 1;
				play_music((char*)CH[0].c_str(),legacyvolume);
			}
			else if (val[1] >= 1 && started == 0) {
				legacy = 0;
				play_music((char*)CH[(int)val[1]].c_str(), val[2]);
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

	}
