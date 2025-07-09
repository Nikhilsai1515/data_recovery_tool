#include "gui.h"
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_deviceContext = nullptr;
static IDXGISwapChain* g_swapchain = nullptr;
static bool g_swapchainoccluded = false;
static UINT g_resizewidth = 0, g_resizeheight = 0;
static ID3D11RenderTargetView* g_mainRenderTarget = nullptr;
HWND gui::window = nullptr;
static WNDCLASSEXW windowClass;
bool gui::quit = false;
UINT gui::width = 853;
UINT gui::height = 480;
void gui::CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer;
	g_swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_device->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTarget);
	pBackBuffer->Release();
}	 
void gui::CleanupRenderTarget() {
	if (g_mainRenderTarget) {
		g_mainRenderTarget->Release();
		g_mainRenderTarget = nullptr;
	}
}	 
bool gui::createDeviceD3D(HWND hwnd) {
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createdeviceflags = 0;
	D3D_FEATURE_LEVEL featurelevel;
	const D3D_FEATURE_LEVEL featurelevelarray[2] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_0 };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createdeviceflags, featurelevelarray, 2, D3D11_SDK_VERSION, &sd, &g_swapchain, &g_device, &featurelevel, &g_deviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED)
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createdeviceflags, featurelevelarray, 2, D3D11_SDK_VERSION, &sd, &g_swapchain, &g_device, &featurelevel, &g_deviceContext);
	if (res != S_OK)
		return false;
	CreateRenderTarget();
	return true;
}
void gui::CleanupDevice() {
	gui::CleanupRenderTarget();
	if (g_swapchain) { g_swapchain->Release(); g_swapchain = nullptr; }
	if (g_deviceContext) { g_deviceContext->Release(); g_deviceContext = nullptr; }
	if (g_device) { g_device->Release(); g_device = nullptr; }
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_resizewidth = (UINT)LOWORD(lParam); // Queue resize
		g_resizeheight = (UINT)HIWORD(lParam);
		gui::width = g_resizewidth;
		gui::height = g_resizeheight;
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		freeImage();
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
bool gui::changeWindowStyle(HWND window, DWORD addStyle, DWORD removeStyle, int pos) {
	LONG_PTR curr_style = GetWindowLongPtrW(window, GWL_STYLE);
		if (curr_style == 0) return false;

		curr_style &= ~removeStyle;
		curr_style |= addStyle;

		SetWindowLongPtrW(window, GWL_STYLE, curr_style);
		ShowWindow(window, pos);
		return true;

}
bool gui::CreateAppwindow(const wchar_t* Wname, const wchar_t* Wclass) {
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = Wclass;
	windowClass.hIconSm = 0;
	RegisterClassExW(&windowClass);
	window = CreateWindowExW(0L, Wclass, Wname,WS_BORDER|WS_POPUPWINDOW|WS_CAPTION, 100, 100, width, height, nullptr, nullptr, windowClass.hInstance, nullptr);
	UINT dark = 0;
	DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &dark, sizeof(dark));
	if (!gui::createDeviceD3D(window)) {
		gui::CleanupDevice();
		UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
		return false;
	}
	ShowWindow(window, SW_SHOWNORMAL);
	UpdateWindow(window);
	return true;
}
void gui::DestroyAppwindow() {
	gui::CleanupDevice();
	DestroyWindow(window);
	UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);

}
ImGuiIO* gui::createImgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(g_device, g_deviceContext);
	ImGui::StyleColorsLight();
	return &io;
}
void gui::DestroyImgui() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
bool gui::BeginRender() {
	MSG msg;
	while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		if (msg.message == WM_QUIT)
			quit = true;
	}
	if (g_swapchainoccluded && g_swapchain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
	{
		::Sleep(10);
		return false;
	}
	g_swapchainoccluded = false;

	// Handle window resize (we don't resize directly in the WM_SIZE handler)
	if (g_resizewidth != 0 && g_resizeheight != 0)
	{
		CleanupRenderTarget();
		g_swapchain->ResizeBuffers(0, g_resizewidth, g_resizeheight, DXGI_FORMAT_UNKNOWN, 0);
		g_resizewidth = g_resizeheight = 0;
		CreateRenderTarget();
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	return true;
}
void gui::endRender() {
	ImGui::Render();
	const FLOAT clear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_deviceContext->OMSetRenderTargets(1, &g_mainRenderTarget, nullptr);
	g_deviceContext->ClearRenderTargetView(g_mainRenderTarget, clear);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	HRESULT hr = g_swapchain->Present(1, 0);
	g_swapchainoccluded = (hr == DXGI_STATUS_OCCLUDED);
}
bool Utf16toUtf8(const wchar_t* wide,char* utf8,int utf8size) {
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
	if (sizeNeeded > (int)utf8size) {
		return false;
	}

	WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeNeeded, nullptr, nullptr);
	return true;
}
bool Utf8ToUtf16(const char* utf8, wchar_t* wide, int wideSize) {
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
	if (sizeNeeded > wideSize) {
		return false;
	}

	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, sizeNeeded);
	return true;
}
bool pathexists(const char* path) {
	wchar_t file[MAX_PATH];
	memset(file, 0, MAX_PATH);
	Utf8ToUtf16(path, file, MAX_PATH);
	HANDLE hFile = CreateFileW(file, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_REPARSE_POINT, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}
	else {
		CloseHandle(hFile);
		return true;
	}
}
bool IsRunningAsAdmin() {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

	if (AllocateAndInitializeSid(&ntAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&adminGroup)) {
		CheckTokenMembership(NULL, adminGroup, &isAdmin);
		FreeSid(adminGroup);
	}

	return isAdmin;
}


bool RelaunchAsAdmin(const char* args) {
	char exePath[MAX_PATH];
	if (!GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
		return false;
	}

	SHELLEXECUTEINFOA sei = { sizeof(sei) };
	sei.lpVerb = "runas";
	sei.lpFile = exePath;
	sei.lpParameters = args;
	sei.hwnd = gui::window;
	sei.nShow = SW_NORMAL;

	if (!ShellExecuteExA(&sei)) {
		DWORD err = GetLastError();
		if (err == ERROR_CANCELLED) {
			MessageBoxW(NULL, L"UAC elevation cancelled by user.", L"Cancelled", MB_ICONWARNING);
		}
		return false;
	}

	return true;
}

void gui::BrowseFolder(char* path,int size)
{
	wchar_t lpath[MAX_PATH* 2];
	BROWSEINFO bi = { 0 };
	bi.lpszTitle = L"Select Folder";
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != nullptr) {
		SHGetPathFromIDList(pidl, lpath);
		CoTaskMemFree(pidl);
	}
	Utf16toUtf8(lpath, path, size);
}

bool gui::ShowFileOpenDialog(HWND owner, char* outPath, DWORD outPathSize) {
	wchar_t widePath[MAX_PATH] = L"";

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = widePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.lpstrTitle = L"Select a File";

	if (!GetOpenFileNameW(&ofn)) {
		outPath[0] = '\0';  
		return false;       
	}


	Utf16toUtf8(widePath, outPath, outPathSize);
	return true;
}

disk_t* ListPhysicalDrives(uint16_t& size) {
	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
		&GUID_DEVINTERFACE_DISK,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
	);
	size = 0;
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to get device info set.\n";
		return NULL;
	}
	uint64_t capacity = 4;
	disk_t* drives = (disk_t*)malloc(sizeof(disk_t) * capacity);
	if (drives == NULL) {
		SetupDiDestroyDeviceInfoList(deviceInfoSet);
		return NULL;
	}
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DWORD index = 0;
	while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DEVINTERFACE_DISK, index, &deviceInterfaceData)) {
		if (index >= capacity) {
			drives = (disk_t*)realloc(drives, capacity * 2 * sizeof(disk_t));
		}
		size++;
		DWORD requiredSize = 0;
		SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

		PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
		if (!deviceDetail) {
			std::cerr << "Failed to allocate memory for device detail.\n";
			break;
		}

		deviceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		SP_DEVINFO_DATA devInfoData = {};
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, deviceDetail, requiredSize, NULL, &devInfoData)) {
			std::wstring devicePath = deviceDetail->DevicePath;

			HANDLE hDevice = CreateFileW(
				devicePath.c_str(),
				0,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL
			);

			if (hDevice != INVALID_HANDLE_VALUE) {
				// Get \\.\PhysicalDriveX
				STORAGE_DEVICE_NUMBER devNumber = {};
				DWORD bytesReturned = 0;
				if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
					&devNumber, sizeof(devNumber), &bytesReturned, NULL)) {
					drives[index].drivenumber = devNumber.DeviceNumber;
				
				}
				else {
					std::cerr << "Failed to get device number.\n";
				}

				// Get model and vendor
				STORAGE_PROPERTY_QUERY query = {};
				query.PropertyId = StorageDeviceProperty;
				query.QueryType = PropertyStandardQuery;

				BYTE buffer[1024] = {};
				if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
					&query, sizeof(query),
					&buffer, sizeof(buffer),
					&bytesReturned, NULL)) {
					STORAGE_DEVICE_DESCRIPTOR* descriptor = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
					const char* vendor = descriptor->VendorIdOffset ? (char*)buffer + descriptor->VendorIdOffset : "Unknown";
					const char* model = descriptor->ProductIdOffset ? (char*)buffer + descriptor->ProductIdOffset : "Unknown";
					drives[index].model = (char*)calloc( strlen(vendor)+strlen(model) + 2, sizeof(char));
					strcpy(drives[index].model, model);
					strcat(drives[index].model, "\n");
					strcat(drives[index].model, vendor);
				}
				else {
					std::cerr << "Failed to query device property.\n";
				}

				CloseHandle(hDevice);
			}
			else {
				std::wcerr << "Failed to open device: " << devicePath << std::endl;
			}
		}

		free(deviceDetail);
		index++;
	}
	SetupDiDestroyDeviceInfoList(deviceInfoSet);
	return drives;
}
