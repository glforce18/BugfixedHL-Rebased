#ifndef AGTR_ANTICHEAT_H
#define AGTR_ANTICHEAT_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define HSPRITE WINDOWS_HSPRITE
#include <windows.h>
#undef HSPRITE
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#define AGTR_SCAN_INTERVAL 60.0f
#define AGTR_INITIAL_DELAY 5.0f
#define AGTR_USERINFO_KEY "_agtrac"

static const char* g_SusProcesses[] = {
    "cheatengine", "artmoney", "ollydbg", "x64dbg", "processhacker",
    "injector", "trainer", nullptr
};

static const char* g_SusDLLs[] = { "opengl32.dll", "d3d9.dll", "dinput8.dll", nullptr };

static const char* g_SusKeywords[] = {
    "hack", "cheat", "aimbot", "wallhack", "esp", "inject",
    "ssw", "plwh", "ogc", "radoom", nullptr
};

class CAGTRAntiCheat
{
public:
    bool m_bInit = false, m_bPassed = false;
    float m_flNext = 0;
    int m_iCount = 0;
    std::string m_szToken, m_szDir;

    void Initialize() {
        if (m_bInit) return;
        
        gEngfuncs.Con_Printf("[AGTR] Anti-Cheat Initializing...\n");
        
#ifdef _WIN32
        char p[MAX_PATH]; 
        GetModuleFileNameA(NULL, p, MAX_PATH);
        char* s = strrchr(p, '\\'); 
        if (s) *s = 0;
        m_szDir = p;
        gEngfuncs.Con_Printf("[AGTR] Game Dir: %s\n", m_szDir.c_str());
#endif
        m_flNext = gEngfuncs.GetClientTime() + AGTR_INITIAL_DELAY;
        m_bInit = true;
        gEngfuncs.Con_Printf("[AGTR] Initialized! First scan in %.0f seconds\n", AGTR_INITIAL_DELAY);
    }

    void Shutdown() { 
        m_bInit = false; 
        gEngfuncs.Con_Printf("[AGTR] Shutdown\n");
    }

    void Think() {
        if (!m_bInit) return;
        
        float t = gEngfuncs.GetClientTime();
        if (m_flNext > 0 && t >= m_flNext) { 
            gEngfuncs.Con_Printf("[AGTR] Running scan #%d...\n", m_iCount + 1);
            DoScan(); 
            m_flNext = t + AGTR_SCAN_INTERVAL; 
            m_iCount++; 
        }
    }

    void DoScan() {
#ifdef _WIN32
        int sus = 0;
        
        // Process scan
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(pe);
            if (Process32First(snap, &pe)) {
                do {
                    std::string n = pe.szExeFile;
                    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    for (int i = 0; g_SusProcesses[i]; i++) {
                        if (n.find(g_SusProcesses[i]) != std::string::npos) { 
                            gEngfuncs.Con_Printf("[AGTR] Suspicious process: %s\n", pe.szExeFile);
                            sus++; 
                            break; 
                        }
                    }
                } while (Process32Next(snap, &pe));
            }
            CloseHandle(snap);
        }
        
        // File scan
        WIN32_FIND_DATAA fd;
        std::string searchPath = m_szDir + "\\*.dll";
        HANDLE hf = FindFirstFileA(searchPath.c_str(), &fd);
        if (hf != INVALID_HANDLE_VALUE) {
            do {
                std::string fn = fd.cFileName;
                std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
                for (int i = 0; g_SusDLLs[i]; i++) {
                    if (fn == g_SusDLLs[i]) {
                        gEngfuncs.Con_Printf("[AGTR] Suspicious DLL: %s\n", fd.cFileName);
                        sus++;
                    }
                }
                for (int i = 0; g_SusKeywords[i]; i++) {
                    if (fn.find(g_SusKeywords[i]) != std::string::npos) { 
                        gEngfuncs.Con_Printf("[AGTR] Suspicious file: %s\n", fd.cFileName);
                        sus++; 
                        break; 
                    }
                }
            } while (FindNextFileA(hf, &fd));
            FindClose(hf);
        }
        
        m_bPassed = (sus == 0);
        
        char tok[17]; 
        snprintf(tok, 17, "%08lX%08lX", GetTickCount(), GetCurrentProcessId());
        m_szToken = tok;
        
        char cmd[256];
        snprintf(cmd, 256, "setinfo %s \"%s|%d|%d\"", AGTR_USERINFO_KEY, tok, m_bPassed?1:0, sus);
        
        gEngfuncs.Con_Printf("[AGTR] Scan complete: %s (suspicious: %d)\n", m_bPassed ? "CLEAN" : "SUSPICIOUS", sus);
        gEngfuncs.Con_Printf("[AGTR] Command: %s\n", cmd);
        
        gEngfuncs.pfnClientCmd(cmd);
#endif
    }
};

static CAGTRAntiCheat g_AGTRAntiCheat;

inline void AGTR_AntiCheat_Init() { g_AGTRAntiCheat.Initialize(); }
inline void AGTR_AntiCheat_Shutdown() { g_AGTRAntiCheat.Shutdown(); }
inline void AGTR_AntiCheat_Think() { g_AGTRAntiCheat.Think(); }

#endif
