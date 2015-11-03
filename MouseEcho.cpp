#include "stdafx.h"
#include "MouseEcho.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HHOOK hMouseHook;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
NOTIFYICONDATA trayIconData = { sizeof(trayIconData) };
#define COLOR_TRANSPARENT_BKG RGB(128,128,128)

#define TIMER_EVENT 1
#define ICO_SIZE 32
#define MAX_THREADS 10

POINT  vPoints[MAX_THREADS];
HANDLE vThreads[MAX_THREADS];

// Forward declarations of functions included in this code module:
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI ThreadFun(LPVOID lpParam)
{
  POINT* pt = (POINT*)lpParam;
  HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, szWindowClass, NULL, 0, pt->x - ICO_SIZE/2, pt->y - ICO_SIZE/2, ICO_SIZE, ICO_SIZE, nullptr, nullptr, nullptr, nullptr);
  SetWindowLong(hWnd, GWL_STYLE, 0);
  SetLayeredWindowAttributes(hWnd, COLOR_TRANSPARENT_BKG, 0, LWA_COLORKEY);
  ShowWindow(hWnd, SW_SHOW);
  InvalidateRect(hWnd, NULL, TRUE);
  UpdateWindow(hWnd);
  SetTimer(hWnd, TIMER_EVENT, 150, NULL);
  MSG msg;
  while (GetMessage(&msg, hWnd, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
}

int FreeHandles()
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
  if (nCode < 0)
  {
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
  }
  if (nCode == HC_ACTION)
  {
    switch (wParam)
    {
    case WM_LBUTTONDOWN:
      int i = FreeHandles();
      if (i >= 0)
      {
        MOUSEHOOKSTRUCT* p = (MOUSEHOOKSTRUCT*)lParam;
        vPoints[i] = p->pt;
        vThreads[i] = CreateThread(NULL, 0, ThreadFun, &vPoints[i], 0, NULL);
      }
      break;
    }
  }
  return 0;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR    lpCmdLine,
  _In_ int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // TODO: Place code here.

  // Initialize global strings
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_MOUSEECHO, szWindowClass, MAX_LOADSTRING);

  WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = szWindowClass;
  wcex.hbrBackground = (HBRUSH)NULL_BRUSH;

  RegisterClassEx(&wcex);

  // Perform application initialization:
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

  FreeHandles();

  return (int)msg.wParam;
}



//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  hInst = hInstance; // Store instance handle in our global variable

  trayIconData.hWnd = CreateWindow(szWindowClass, szTitle, 0, 100, 100, 100, 100, nullptr, nullptr, hInstance, nullptr);

  trayIconData.uID = IDI_MOUSEECHO;
  trayIconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSEECHO));
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

  // show balloonh
  lstrcpy(trayIconData.szInfoTitle, L"A");
  lstrcpy(trayIconData.szInfo, L"S");
  trayIconData.uTimeout = 15000;
  trayIconData.uFlags = NIF_INFO;
  trayIconData.dwInfoFlags = NIF_INFO;
  Shell_NotifyIcon(NIM_MODIFY, &trayIconData);

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
    // Parse the menu selections:
    switch (wmId)
    {
    case IDM_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      break;
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
    DrawIconEx(hdc, 0, 0, trayIconData.hIcon, ICO_SIZE, ICO_SIZE, 0, NULL, DI_NORMAL);
    DeleteObject(hbr);
    EndPaint(hWnd, &ps);
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

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  switch (message)
  {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}
