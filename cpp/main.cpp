#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>

#include <algorithm>
#include <list>
#include <memory>
using namespace std;

#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "resource.h"
#include "QuickHull.cpp"

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class DPIScale
{
    static float scaleX;
    static float scaleY;

public:
    static void Initialize(ID2D1Factory *pFactory)
    {
        FLOAT dpiX, dpiY;
        pFactory->GetDesktopDpi(&dpiX, &dpiY);
        scaleX = dpiX/96.0f;
        scaleY = dpiY/96.0f;
    }

    template <typename T>
    static float PixelsToDipsX(T x)
    {
        return static_cast<float>(x) / scaleX;
    }

    template <typename T>
    static float PixelsToDipsY(T y)
    {
        return static_cast<float>(y) / scaleY;
    }
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
D2D1_ELLIPSE first_point;

struct MyEllipse
{
    D2D1_ELLIPSE    ellipse;
    D2D1_COLOR_F    color;

    void Draw(ID2D1RenderTarget *pRT, ID2D1SolidColorBrush *pBrush)
    {
        pBrush->SetColor(color);
        pRT->FillEllipse(ellipse, pBrush);
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        pRT->DrawEllipse(ellipse, pBrush, 1.0f);
    }

    BOOL HitTest(float x, float y)
    {
        const float a = ellipse.radiusX;
        const float b = ellipse.radiusY;
        const float x1 = x - ellipse.point.x;
        const float y1 = y - ellipse.point.y;
        const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
        return d <= 1.0f;
    }
};

D2D1::ColorF::Enum colors[] = { D2D1::ColorF::Yellow, D2D1::ColorF::Salmon, D2D1::ColorF::LimeGreen };


class MainWindow : public BaseWindow<MainWindow>
{
    enum Mode
    {
        DrawMode,
        SelectMode,
        DragMode
    };

    HCURSOR                 hCursor;

    ID2D1Factory            *pFactory;
    ID2D1HwndRenderTarget   *pRenderTarget;
    ID2D1SolidColorBrush    *pBrush;
    D2D1_POINT_2F           ptMouse;

    Mode                    mode;
    size_t                  nextColor;

    list<shared_ptr<MyEllipse>>             ellipses;
    list<shared_ptr<MyEllipse>>::iterator   selection;
     
    shared_ptr<MyEllipse> Selection() 
    { 
        if (selection == ellipses.end()) 
        { 
            return nullptr;
        }
        else
        {
            return (*selection);
        }
    }

    void    ClearSelection() { selection = ellipses.end(); }
    HRESULT InsertEllipse(float x, float y);

    /*MyEllipse PointFarthestFromEdge(MyEllipse a, MyEllipse b, list<shared_ptr<MyEllipse>> p);
    bool    Contains(list<MyEllipse> points, MyEllipse to_be_found);*/

    BOOL    HitTest(float x, float y);
    void    SetMode(Mode m);
    void    MoveSelection(float x, float y);
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    Resize();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();
    void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
    void    OnKeyDown(UINT vkey);

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL), 
        ptMouse(D2D1::Point2F()), nextColor(0), selection(ellipses.end())
    {
    }

    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            // Change based on mode selected
            for (size_t i = 0; i < 10; i++)
            {
                InsertEllipse(rand() % 1100 + 100, rand() % 400 + 100);
            }
        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

int PointOri(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p3) {
    int value = (p2.point.y - p1.point.y) * (p3.point.x - p2.point.x) - (p2.point.x - p1.point.x) * (p3.point.y - p2.point.y);

    if (value == 0) {
        return 0;
    }
    return (value > 0) ? 1 : 2;
}

int PointDistance(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2) {
    return (p1.point.x - p2.point.x) * (p1.point.x - p2.point.x) - (p1.point.y - p2.point.y) * (p1.point.y - p2.point.y);
}

int ComparePoints(const void* vp1, const void* vp2) {
    D2D1_ELLIPSE* p1 = (D2D1_ELLIPSE*)vp1;
    D2D1_ELLIPSE* p2 = (D2D1_ELLIPSE*)vp2;

    int ori = PointOri(first_point, *p1, *p2);
    if (ori == 0) {
        return (PointDistance(first_point, *p2) >= PointDistance(first_point, *p1)) ? -1 : 1;
    }
    return (ori == 2) ? -1 : 1;

}

vector<D2D1_ELLIPSE> SortPoints(vector<D2D1_ELLIPSE> points) {
    int min_y = points[0].point.y;
    int min = 0;

    for (int i = 0; i < points.size(); i++) {
        int y = points[i].point.y;

        if ((y < min_y) || (min_y == y && points[i].point.x < points[min].point.x)) {
            min_y = points[i].point.y;
            min = i;
        }
    }

    D2D1_ELLIPSE temp = points[0];
    points[0] = points[min];
    points[min] = temp;

    first_point = points[0];

    qsort(&points[1], points.size() - 1, sizeof(D2D1_ELLIPSE), ComparePoints);

    return points;
}

void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        vector<D2D1_ELLIPSE> *hullpoints = new vector<D2D1_ELLIPSE>;
     
        pRenderTarget->BeginDraw();

        pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::SkyBlue) );

        for (auto i = ellipses.begin(); i != ellipses.end(); ++i)
        {
            (*hullpoints).push_back((*i)->ellipse);
            (*i)->Draw(pRenderTarget, pBrush);
        }

        QuickHull *qhull = new QuickHull(*hullpoints);

        vector<D2D1_ELLIPSE> q_points = (qhull)->GetConvexHull();

        vector<D2D1_ELLIPSE> quick_hull_points = SortPoints(q_points);

        printf("%d\n", quick_hull_points.size());

        for (size_t i = 0; i < quick_hull_points.size() - 1; i++) {
            auto hull_a = std::next(quick_hull_points.begin(), i);
            auto hull_b = std::next(quick_hull_points.begin(), i+1);
            D2D1_POINT_2F point_a = D2D1::Point2F((*hull_a).point.x, (*hull_a).point.y);
            D2D1_POINT_2F point_b = D2D1::Point2F((*hull_b).point.x, (*hull_b).point.y);
            pRenderTarget->DrawLine(point_a, point_b, pBrush);

        }

        auto hull_a = std::next(quick_hull_points.begin(), quick_hull_points.size() - 1);
        auto hull_b = std::next(quick_hull_points.begin(), 0);
        D2D1_POINT_2F point_a = D2D1::Point2F((*hull_a).point.x, (*hull_a).point.y);
        D2D1_POINT_2F point_b = D2D1::Point2F((*hull_b).point.x, (*hull_b).point.y);
        pRenderTarget->DrawLine(point_a, point_b, pBrush);

        if (Selection())
        {
            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            pRenderTarget->DrawEllipse(Selection()->ellipse, pBrush, 2.0f);
        }

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }
}

void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);

        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);

    //if (mode == DrawMode)
    //{
    //    POINT pt = { pixelX, pixelY };

    //    if (DragDetect(m_hwnd, pt))
    //    {
    //        SetCapture(m_hwnd);
    //    
    //        // Start a new ellipse.
    //        InsertEllipse(dipX, dipY);
    //    }
    //}
    ClearSelection();

    if (HitTest(dipX, dipY))
    {
        SetCapture(m_hwnd);

        ptMouse = Selection()->ellipse.point;
        ptMouse.x -= dipX;
        ptMouse.y -= dipY;

        SetMode(DragMode);
    }
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::OnLButtonUp()
{
    if ((mode == DrawMode) && Selection())
    {
        ClearSelection();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    else if (mode == DragMode)
    {
        SetMode(SelectMode);
    }
    ReleaseCapture(); 
}


void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);

    if ((flags & MK_LBUTTON) && Selection())
    { 
        if (mode == DrawMode)
        {
            // Resize the ellipse.
            const float width = (dipX - ptMouse.x) / 2;
            const float height = (dipY - ptMouse.y) / 2;
            const float x1 = ptMouse.x + width;
            const float y1 = ptMouse.y + height;

            Selection()->ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);
        }
        else if (mode == DragMode)
        {
            // Move the ellipse.
            Selection()->ellipse.point.x = dipX + ptMouse.x;
            Selection()->ellipse.point.y = dipY + ptMouse.y;
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}


void MainWindow::OnKeyDown(UINT vkey)
{
    switch (vkey)
    {
    case VK_BACK:
    /*case VK_DELETE:
        if (Selection())
        {
            ellipses.erase(selection);
            ClearSelection();
            SetMode(SelectMode);
            InvalidateRect(m_hwnd, NULL, FALSE);
        };
        break;*/
        break;

    case VK_LEFT:
        MoveSelection(-1, 0);
        break;

    case VK_RIGHT:
        MoveSelection(1, 0);
        break;

    case VK_UP:
        MoveSelection(0, -1);
        break;

    case VK_DOWN:
        MoveSelection(0, 1);
        break;
    }
}

HRESULT MainWindow::InsertEllipse(float x, float y)
{
    try
    {
        selection = ellipses.insert(
            ellipses.end(), 
            shared_ptr<MyEllipse>(new MyEllipse()));

        Selection()->ellipse.point = ptMouse = D2D1::Point2F(x, y);
        Selection()->ellipse.radiusX = Selection()->ellipse.radiusY = 10.0f; 
        Selection()->color = D2D1::ColorF( colors[nextColor] );

        nextColor = (nextColor + 1) % ARRAYSIZE(colors);
    }
    catch (std::bad_alloc)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}




BOOL MainWindow::HitTest(float x, float y)
{
    for (auto i = ellipses.rbegin(); i != ellipses.rend(); ++i)
    {
        if ((*i)->HitTest(x, y))
        {
            selection = (++i).base();
            return TRUE;
        }
    }
    return FALSE;
}

void MainWindow::MoveSelection(float x, float y)
{
    if (Selection())
    {
        Selection()->ellipse.point.x += x;
        Selection()->ellipse.point.y += y;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::SetMode(Mode m)
{
    mode = m;

    LPWSTR cursor;
    cursor = IDC_HAND;
    /*switch (mode)
    {
    case DrawMode:
        cursor = IDC_HAND;
        break;

    case SelectMode:
        cursor = IDC_HAND;
        break;

    case DragMode:
        cursor = IDC_SIZEALL;
        break;
    }*/

    hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Draw Circles", WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));

    ShowWindow(win.Window(), nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(win.Window(), hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        DPIScale::Initialize(pFactory);
        SetMode(DrawMode);
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        Resize();
        return 0;

    case WM_LBUTTONDOWN: 
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP: 
        OnLButtonUp();
        return 0;

    case WM_MOUSEMOVE: 
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(hCursor);
            return TRUE;
        }
        break;

    case WM_KEYDOWN:
        OnKeyDown((UINT)wParam);
        return 0;

    case WM_COMMAND:
        /*switch (LOWORD(wParam))
        {
        case ID_DRAW_MODE:
            SetMode(DrawMode);
            break;

        case ID_SELECT_MODE:
            SetMode(SelectMode);
            break;

        case ID_TOGGLE_MODE:
            if (mode == DrawMode)
            {
                SetMode(SelectMode);
            }
            else
            {
                SetMode(DrawMode);
            }
            break;
        }*/
        break;
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
