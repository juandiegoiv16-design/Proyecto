#include <windows.h>

// Para MSVC: manifiesto de Common Controls (opcional)
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' \
 name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Globals
HWND hEditBox, hPregunta, hBtnSi, hBtnNo;
HFONT hFont = NULL;
HBRUSH hBrush = NULL;

int pasoActual = 0;
#define TOTAL_PASOS 10

const wchar_t* textos[TOTAL_PASOS] = {
    L"Paso 1...",
    L"Paso 2...",
    L"Paso 3...",
    L"Paso 4...",
    L"Paso 5...",
    L"Paso 6...",
    L"Paso 7...",
    L"Paso 8...",
    L"Paso 9...",
    L"Paso 10..."
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"VentanaSecuencial";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // dibujamos el fondo con la brocha en WM_CTLCOLOR...
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        MessageBoxA(NULL, "Error: RegisterClassW failed", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Proyecto de algoritmos GAES 1",
        (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 320,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Fuente y brocha (una sola vez)
        hFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI"
        );

        hBrush = CreateSolidBrush(RGB(60, 60, 60)); // fondo gris del recuadro

        // Caja de texto (EDIT multilínea, read-only) -> hace wrap automáticamente
        hEditBox = CreateWindowW(
            L"EDIT",
            textos[pasoActual],
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            40, 20, 440, 160,
            hwnd, NULL, NULL, NULL
        );

        // Pregunta y botones
        hPregunta = CreateWindowW(L"STATIC", L"¿Te funcionó?",
                                  WS_CHILD | WS_VISIBLE | SS_CENTER,
                                  200, 200, 120, 24, hwnd, NULL, NULL, NULL);

        hBtnSi = CreateWindowW(L"BUTTON", L"Sí",
                               WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                               160, 240, 90, 34, hwnd, (HMENU)1, NULL, NULL);

        hBtnNo = CreateWindowW(L"BUTTON", L"No",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               280, 240, 90, 34, hwnd, (HMENU)2, NULL, NULL);

        // Aplicar fuente
        if (hFont) {
            SendMessageW(hEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hPregunta, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hBtnSi, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hBtnNo, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Sí
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == 2) { // No
            pasoActual++;
            if (pasoActual < TOTAL_PASOS) {
                SetWindowTextW(hEditBox, textos[pasoActual]);
            } else {
                MessageBoxW(hwnd, L"No hay más pasos disponibles.", L"Fin", MB_OK | MB_ICONINFORMATION);
                PostQuitMessage(0);
            }
        }
        break;
    
    // color de fondo
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255,255,255));
        return (LRESULT)hBrush;
    }

    case WM_CTLCOLOREDIT: { // para el control EDIT (texto)
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(60,60,60));    // fondo gris para el edit
        SetTextColor(hdc, RGB(255,255,255));// texto blanco
        return (LRESULT)hBrush;
    }

    case WM_DESTROY:
        if (hFont) DeleteObject(hFont);
        if (hBrush) DeleteObject(hBrush);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


