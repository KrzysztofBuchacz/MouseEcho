#include "stdafx.h"
#include "MouseEcho.h"

HINSTANCE hInst;
HICON hIcon;
HHOOK hMouseHook;
WCHAR szWindowClass[] = L"2C91E8BF-43EF-4604-9056-5991FAB6FB2E";
NOTIFYICONDATA trayIconData = { sizeof(trayIconData) };
#define COLOR_TRANSPARENT_BKG RGB(128, 128, 128)
#define TIMER_EVENT 1
#define ICO_SIZE 32
#define MAX_THREADS 10

POINT  vPoints [MAX_THREADS];
HANDLE vThreads[MAX_THREADS];

DWORD WINAPI ThreadFun(LPVOID lpParam)
{
  POINT* pt = (POINT*)lpParam;
  HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, szWindowClass,
    NULL, 0, pt->x - ICO_SIZE/2, pt->y - ICO_SIZE/2, ICO_SIZE, ICO_SIZE, nullptr, nullptr, nullptr, nullptr);
  SetWindowLong(hWnd, GWL_STYLE, WS_DISABLED);
  SetLayeredWindowAttributes(hWnd, COLOR_TRANSPARENT_BKG, 0, LWA_COLORKEY);
  SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
  SetTimer(hWnd, TIMER_EVENT, 150, NULL);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  BOOL ok = DestroyWindow(hWnd);
  _ASSERT(ok);
  return 0;
}

int FreeHandle()
{
  int ret = -1;
  for (int i = 0; i < MAX_THREADS; i++)
  {
    if (vThreads[i] != NULL)
    {
      if (WaitForSingleObject(vThreads[i], 0) == WAIT_OBJECT_0)
      {
        CloseHandle(vThreads[i]);
        vThreads[i] = NULL;
      }
    }
    if (vThreads[i] == NULL)
      ret = i;
  }
  return ret;
}

static LRESULT CALLBACK MouseProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
  if (nCode == HC_ACTION)
  {
    switch (wParam)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
      int i = FreeHandle();
      if (i >= 0)
      {
        MOUSEHOOKSTRUCT* p = (MOUSEHOOKSTRUCT*)lParam;
        vPoints[i] = p->pt;
        vThreads[i] = CreateThread(NULL, 0, ThreadFun, &vPoints[i], 0, NULL);
      }
      break;
    }
  }
  return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  hInst = hInstance; // Store instance handle in our global variable

  hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSEECHO));

  trayIconData.hWnd = CreateWindow(szWindowClass, NULL, 0, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);

  trayIconData.uID = IDI_MOUSEECHO;
  trayIconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAY));
  trayIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  trayIconData.uCallbackMessage = RegisterWindowMessage(L"MouseEchoCallbackMessage1");
  lstrcpy(trayIconData.szTip, L"MouseEcho");

  // Set the tray icon
  if (!Shell_NotifyIcon(NIM_ADD, &trayIconData))
  {
    return FALSE;
  }

  trayIconData.uVersion = 3;
  if (!Shell_NotifyIcon(NIM_SETVERSION, &trayIconData))
  {
    return FALSE;
  }

  if (!trayIconData.hWnd)
  {
    return FALSE;
  }

  hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

  if (!hMouseHook)
  {
    return FALSE;
  }

  return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (trayIconData.uCallbackMessage == message)
  {
    if (LOWORD(lParam) == WM_RBUTTONUP)
    {
      HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_MOUSEECHO));
      POINT pos = { 0 };
      GetCursorPos(&pos);
      TrackPopupMenu(GetSubMenu(hMenu, 0), 0, pos.x, pos.y, 0, hWnd, NULL);
      DestroyMenu(hMenu);
    }
    return 0;
  }
  switch (message)
  {
  case WM_COMMAND:
  {
    int wmId = LOWORD(wParam);
    switch (wmId)
    {
    case IDM_EXIT:
      if (hWnd == trayIconData.hWnd)
        DestroyWindow(hWnd);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  }
  break;
  case WM_TIMER:
    if (wParam == TIMER_EVENT)
    {
      BOOL ok = KillTimer(hWnd, TIMER_EVENT);
      _ASSERT(ok);
      PostQuitMessage(0);
    }
    break;
  case WM_NCCALCSIZE:
    if (wParam)
    {
      LPNCCALCSIZE_PARAMS pNcCalcSizeParam = (LPNCCALCSIZE_PARAMS)lParam;
      pNcCalcSizeParam->rgrc[0] = pNcCalcSizeParam->rgrc[1];
      return TRUE;
    }
    break;
  case WM_ERASEBKGND:
    return TRUE;
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT r;
    GetClientRect(hWnd, &r);
    HBRUSH hbr = CreateSolidBrush(COLOR_TRANSPARENT_BKG);
    FillRect(hdc, &r, hbr);
    DrawIconEx(hdc, 0, 0, hIcon, ICO_SIZE, ICO_SIZE, 0, NULL, DI_NORMAL);
    DeleteObject(hbr);
    EndPaint(hWnd, &ps);
  }
  break;
  case WM_ACTIVATEAPP:
    if (hWnd != trayIconData.hWnd)
    {
      return MA_NOACTIVATEANDEAT;
    }
    break;
  case WM_DESTROY:
    if (hWnd == trayIconData.hWnd)
    {
      trayIconData.uFlags = 0;
      Shell_NotifyIcon(NIM_DELETE, &trayIconData);
      UnhookWindowsHookEx(hMouseHook);
      PostQuitMessage(0);
    }
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = szWindowClass;
  wcex.hbrBackground = (HBRUSH)NULL_BRUSH;

  RegisterClassEx(&wcex);

  if (!InitInstance(hInstance, nCmdShow))
  {
    return FALSE;
  }

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  FreeHandle();

  return (int)msg.wParam;
}
