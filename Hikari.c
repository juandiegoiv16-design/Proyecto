// Hikari.c  -- versión completa integrada
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <stdbool.h>
#include <wchar.h>
#include <stdio.h>

// ---------------- CONFIG ----------------
#define NUM_BUTTONS 11
#define BUTTON_ID_BASE 1000
#define POPUP_ID_YES 2001
#define POPUP_ID_NO  2002

// Pon aquí tu BMP (o copia el BMP junto al .exe y deja "fondo.bmp")
#define BACKGROUND_BMP_PATH L"Logo-Hikari.bmp"

// ---------------- HELPERS ----------------
#define CLAMP255(x) (((x) > 255) ? 255 : ((x) < 0 ? 0 : (x)))

// ---------------- GLOBALS ----------------
HINSTANCE g_hInst;
HWND g_hMain = NULL;
HBITMAP g_hBmp = NULL;
HWND g_hButtons[NUM_BUTTONS];
bool g_disabled[NUM_BUTTONS];
int g_hoverIndex = -1;
bool g_isFullscreen = false;
RECT g_prevRect;

// paleta de colores
COLORREF g_colors[NUM_BUTTONS] = {
    RGB(210,245,255), RGB(190,235,245), RGB(160,220,240), RGB(130,205,235),
    RGB(90,185,230),  RGB(60,165,215),  RGB(30,135,200),  RGB(20,120,180),
    RGB(10,100,160),  RGB(5,  70, 120), RGB(2,  40,  80)
};

// forward
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcPopup(HWND, UINT, WPARAM, LPARAM);
void CreateSkillButtons(HWND);
void ToggleFullscreen(HWND);
void OpenPopupForSkill(int skillIndex);
void DisableSkill(int idx);
static void CenterWindow(HWND hWnd);

// ---------------- Implementation ----------------

static void CenterWindow(HWND hWnd) {
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hWnd, NULL, (sw - w) / 2, (sh - h) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// ---------------- MAIN ----------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SkillsMainClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    g_hMain = CreateWindowEx(
        0, wc.lpszClassName, L"Habilidades - Hikari",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
        NULL, NULL, hInstance, NULL
    );
    if (!g_hMain) return 0;

    // Intentar cargar BMP de fondo (si falla, mostramos código de error pero continuamos)
    g_hBmp = (HBITMAP)LoadImageW(NULL, BACKGROUND_BMP_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!g_hBmp) {
        wchar_t buf[256];
        swprintf(buf, L"No se pudo cargar el BMP de fondo.\nRuta: %s\nGetLastError() = %lu",
                 BACKGROUND_BMP_PATH, GetLastError());
        MessageBoxW(NULL, buf, L"Advertencia - fondo no cargado", MB_OK | MB_ICONWARNING);
        // seguimos; se usará fondo sólido
    }

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    // Iniciar timer para detectar hover (25Hz)
    SetTimer(g_hMain, 1, 40, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hBmp) DeleteObject(g_hBmp);
    return (int)msg.wParam;
}

// ---------------- Botones (creación) ----------------
void CreateSkillButtons(HWND hWnd) {
    // Creamos botones owner-draw; posición inicial (será re-posicionada en WM_SIZE)
    int btn_w = 200, btn_h = 60;
    int spacing_x = 60, spacing_y = 30;
    int margin_x = 40, margin_y = 30;

    POINT positions[NUM_BUTTONS];
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        int col = i % 4;
        int row = i / 4;
        positions[i].x = margin_x + col * (btn_w + spacing_x);
        positions[i].y = margin_y + row * (btn_h + spacing_y);
    }

    for (int i = 0; i < NUM_BUTTONS; ++i) {
        g_disabled[i] = false;
        HWND hb = CreateWindowEx(
            0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            positions[i].x, positions[i].y, btn_w, btn_h,
            hWnd, (HMENU)(INT_PTR)(BUTTON_ID_BASE + i),
            g_hInst, NULL
        );
        g_hButtons[i] = hb;
    }

    // Force initial layout by sending WM_SIZE
    RECT rc;
    GetClientRect(hWnd, &rc);
    SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
}

// ---------------- Fullscreen ----------------
void ToggleFullscreen(HWND hWnd) {
    if (!g_isFullscreen) {
        GetWindowRect(hWnd, &g_prevRect);
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                     SWP_FRAMECHANGED);
        g_isFullscreen = true;
    } else {
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, g_prevRect.left, g_prevRect.top,
                     g_prevRect.right - g_prevRect.left, g_prevRect.bottom - g_prevRect.top,
                     SWP_FRAMECHANGED);
        g_isFullscreen = false;
    }
}

// ---------------- Popup WndProc ----------------
LRESULT CALLBACK WndProcPopup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == POPUP_ID_YES) {
            // cerrar popup y cerrar principal
            DestroyWindow(hWnd);
            PostMessage(g_hMain, WM_CLOSE, 0, 0);
            return 0;
        } else if (id == POPUP_ID_NO) {
            // deshabilitar habilidad asociada y cerrar popup
            int idx = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (idx >= 0) DisableSkill(idx);
            DestroyWindow(hWnd);
            EnableWindow(g_hMain, TRUE);
            SetForegroundWindow(g_hMain);
            return 0;
        }
    } break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        // re-habilitar main al destruir popup (por si se cerró con la X)
        if (g_hMain) {
            EnableWindow(g_hMain, TRUE);
            SetForegroundWindow(g_hMain);
        }
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------- Open Popup (modal behavior) ----------------
void OpenPopupForSkill(int skillIndex) {
    // deshabilitar main (efecto modal)
    EnableWindow(g_hMain, FALSE);

    // registrar clase del popup solo 1 vez
    static bool popupRegistered = false;
    if (!popupRegistered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = WndProcPopup;
        wc.hInstance = g_hInst;
        wc.lpszClassName = L"PopupSkillClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&wc);
        popupRegistered = true;
    }

    HWND hPopup = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"PopupSkillClass", L"Habilidad",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 320,
        g_hMain, NULL, g_hInst, NULL
    );
    if (!hPopup) {
        EnableWindow(g_hMain, TRUE);
        return;
    }

    CenterWindow(hPopup);

    // texto
    wchar_t textbuf[256];
    swprintf(textbuf, L"Texto de habilidad (%d)\nDescribe lo que debe hacer...", skillIndex + 1);

    CreateWindowEx(0, L"STATIC", textbuf,
                   WS_CHILD | WS_VISIBLE | SS_CENTER,
                   20, 20, 560, 180,
                   hPopup, NULL, g_hInst, NULL);

    // botones Si / No
    CreateWindowEx(0, L"BUTTON", L"Si",
                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                   120, 220, 140, 50,
                   hPopup, (HMENU)POPUP_ID_YES, g_hInst, NULL);
    CreateWindowEx(0, L"BUTTON", L"No",
                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                   340, 220, 140, 50,
                   hPopup, (HMENU)POPUP_ID_NO, g_hInst, NULL);

    // guardar índice en userdata (para usar en NO)
    SetWindowLongPtr(hPopup, GWLP_USERDATA, (LONG_PTR)skillIndex);

    // asegurar focus
    SetForegroundWindow(hPopup);
    SetActiveWindow(hPopup);
}

// ---------------- Disable skill ----------------
void DisableSkill(int idx) {
    if (idx < 0 || idx >= NUM_BUTTONS) return;
    g_disabled[idx] = true;
    EnableWindow(g_hButtons[idx], FALSE);
    InvalidateRect(g_hButtons[idx], NULL, TRUE);
}

// ---------------- WndProc principal ----------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateSkillButtons(hWnd);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_F11) ToggleFullscreen(hWnd);
        if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_TIMER: {
        if (wParam == 1) {
            // calcular hover por posicion del mouse
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hWnd, &p);
            int newHover = -1;
            for (int i = 0; i < NUM_BUTTONS; ++i) {
                if (!g_hButtons[i]) continue;
                RECT r;
                GetWindowRect(g_hButtons[i], &r);
                POINT topLeft = { r.left, r.top };
                ScreenToClient(hWnd, &topLeft);
                RECT rcClient;
                rcClient.left = topLeft.x;
                rcClient.top = topLeft.y;
                rcClient.right = rcClient.left + (r.right - r.left);
                rcClient.bottom = rcClient.top + (r.bottom - r.top);
                if (PtInRect(&rcClient, p) && !g_disabled[i]) { newHover = i; break; }
            }
            if (newHover != g_hoverIndex) {
                int old = g_hoverIndex;
                g_hoverIndex = newHover;
                if (old != -1 && g_hButtons[old]) InvalidateRect(g_hButtons[old], NULL, TRUE);
                if (g_hoverIndex != -1 && g_hButtons[g_hoverIndex]) InvalidateRect(g_hButtons[g_hoverIndex], NULL, TRUE);
            }
        }
        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        HWND hItem = pdis->hwndItem;
        int idx = -1;
        for (int i = 0; i < NUM_BUTTONS; ++i) {
            if (g_hButtons[i] == hItem) { idx = i; break; }
        }
        if (idx == -1) break;

        COLORREF clr = g_colors[idx];
        if (g_disabled[idx]) {
            clr = RGB(180,180,180);
        } else if (idx == g_hoverIndex) {
            int r = CLAMP255(GetRValue(clr) + 30);
            int g = CLAMP255(GetGValue(clr) + 30);
            int b = CLAMP255(GetBValue(clr) + 30);
            clr = RGB(r,g,b);
        }

        // dibujar fondo
        HBRUSH hBr = CreateSolidBrush(clr);
        FillRect(pdis->hDC, &pdis->rcItem, hBr);
        DeleteObject(hBr);

        // borde
        FrameRect(pdis->hDC, &pdis->rcItem, GetStockObject(BLACK_BRUSH));

        // texto
        wchar_t buf[64];
        swprintf(buf, L"Habilidad #%d", idx + 1);
        SetBkMode(pdis->hDC, TRANSPARENT);
        SetTextColor(pdis->hDC, g_disabled[idx] ? RGB(110,110,110) : RGB(255,255,255));
        DrawTextW(pdis->hDC, buf, -1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= BUTTON_ID_BASE && id < BUTTON_ID_BASE + NUM_BUTTONS) {
            int idx = id - BUTTON_ID_BASE;
            if (!g_disabled[idx]) {
                OpenPopupForSkill(idx);
            }
        }
        return 0;
    }

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        // layout dinámico tipo grid centrado
        int cols = 4;
        int spacing_x = 20;
        int spacing_y = 20;
        int btn_w = (width - (cols + 1) * spacing_x) / cols;
        if (btn_w > 260) btn_w = 260;
        if (btn_w < 100) btn_w = 100;
        int btn_h = 60;

        int rows = (NUM_BUTTONS + cols - 1) / cols;
        int totalWidth = cols * btn_w + (cols + 1) * spacing_x;
        int totalHeight = rows * btn_h + (rows + 1) * spacing_y;
        int startX = (width - totalWidth) / 2 + spacing_x;
        int startY = (height - totalHeight) / 2 + spacing_y;

        for (int i = 0; i < NUM_BUTTONS; ++i) {
            int row = i / cols;
            int col = i % cols;
            int x = startX + col * (btn_w + spacing_x);
            int y = startY + row * (btn_h + spacing_y);
            if (g_hButtons[i]) MoveWindow(g_hButtons[i], x, y, btn_w, btn_h, TRUE);
        }

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);

        if (g_hBmp) {
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(mem, g_hBmp);
            BITMAP bm;
            GetObject(g_hBmp, sizeof(bm), &bm);

            // Escalar el BMP a client rect
            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                       mem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            SelectObject(mem, old);
            DeleteDC(mem);
        } else {
            // color de fondo fallback
            HBRUSH hb = CreateSolidBrush(RGB(250,245,240));
            FillRect(hdc, &rc, hb);
            DeleteObject(hb);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        KillTimer(hWnd, 1);
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    } // switch

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


