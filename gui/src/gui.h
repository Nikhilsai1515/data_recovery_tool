#pragma once
#define INITGUID
#include<imgui/imgui.h>
#include<imgui/imgui_impl_win32.h>
#include<imgui/imgui_impl_dx11.h>
#include<d3d11.h>
#include<Windows.h>
#include<ShlObj.h>
#include<setupapi.h>
#include<dwmapi.h>
#include<iostream>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef struct {
	char* model;
	DWORD drivenumber;
}disk_t;
namespace gui {
	void CreateRenderTarget();
	void CleanupRenderTarget();
	bool createDeviceD3D(HWND hwnd);
	void CleanupDevice();
	bool CreateAppwindow(const wchar_t* Wname, const wchar_t* Wclass);
	void DestroyAppwindow();
	ImGuiIO* createImgui();
	void DestroyImgui();
	bool BeginRender();
	void endRender();
	extern bool quit;
	extern UINT width;
	extern UINT height;
	extern HWND window;
	bool ShowFileOpenDialog(HWND owner, char* outPath, DWORD outPathSize);
	void BrowseFolder(char* path, int size);
bool changeWindowStyle(HWND window, DWORD addStyle, DWORD removeStyle = 0, int pos=1);
}
bool IsRunningAsAdmin();
extern void freeImage();
bool RelaunchAsAdmin(const char* args);
bool Utf16toUtf8(const wchar_t* wide, char* utf8, int utf8size);
bool Utf8ToUtf16(const char* utf8, wchar_t* wide, int wideSize); 
bool pathexists(const char* path);
disk_t* ListPhysicalDrives(uint16_t& size);
