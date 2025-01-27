#pragma once
#include "Engine/Engine.h"
#include "Path.h"
#include "Timer/Timer.h"
#include "Window/Window.h"

#include <filesystem>

class OConfigReader;
class OTest;
class OApplication
{
public:
	static OApplication* Get();
	void Destory();

	shared_ptr<OWindow> CreateWindow() const;

	void DestroyWindow(unique_ptr<OWindow> Window)
	{
		Window.reset();
	}

	void Quit(int ExitCode);
	void InitApplication(HINSTANCE hInstance);

	int Run();

	inline static wchar_t WindowClassName[] = L"DXRendererClass";

	HINSTANCE GetAppInstance() const;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void SetAppPaused(bool bPaused);
	string GetConfigPath(const string& Key) const;
	wstring GetResourcePath(const wstring& Resource) const;
	wstring GetModelsPath(const wstring& Resource) const;
	static wstring GetTexturesPath(const wstring& PathToObj, const wstring& PathToTex);
	vector<wstring> GetShaderFolders() const;
	wstring GetRootShaderFolder() const;
	wstring GetRaytracingShaderFolder() const;

private:
	OApplication();
	void InitWindowClass() const;
	void CalculateFrameStats();

	inline static OApplication* Application = nullptr;
	HINSTANCE AppInstance = nullptr;

	OEngine* Engine = nullptr;
	vector<shared_ptr<OTest>> Tests;

	STimer Timer;
	bool bIsAppPaused = false;
	bool bIsAppMinimized = false;
	bool bIsAppMaximized = false;
	bool bIsResizing = false;

	SWindowInfo DefaultWindowInfo = { false, L"Window", 1300, 900, false, 45.f };

	SPath RootDirPath;
	SPath CurrentPath;

	unique_ptr<OConfigReader> ConfigReader;
	vector<wstring> ShaderFolders;
};

inline int OApplication::Run()
{
	Engine->InitScene();

	Timer.Reset();
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Timer.Tick();
			CalculateFrameStats();
			if (bIsAppPaused)
			{
				Sleep(100);
			}
			else
			{
				UpdateEventArgs args(Timer, Engine->GetWindow().lock()->GetHWND());
				Engine->Draw(args);
			}
		}
	}
	Engine->OnEnd();
	return static_cast<int>(msg.wParam);
}
