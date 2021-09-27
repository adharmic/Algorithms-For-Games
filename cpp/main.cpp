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
#include "HullMath.cpp"

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

enum Algo
{
    QHull,
    MinkSum,
    MinkDiff,
    PointHull,
    GJK
};

Algo current_alg = QHull;

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


//Base window
class MainWindow : public BaseWindow<MainWindow>
{
    enum Mode {};
    HCURSOR                 hCursor;

    ID2D1Factory            *pFactory;
    ID2D1HwndRenderTarget   *pRenderTarget;
    ID2D1SolidColorBrush    *pBrush;
    D2D1_POINT_2F           ptMouse;

    Mode                    mode;
    size_t                  nextColor;


    /*MyEllipse PointFarthestFromEdge(MyEllipse a, MyEllipse b, list<shared_ptr<MyEllipse>> p);
    bool    Contains(list<MyEllipse> points, MyEllipse to_be_found);*/

    //void    SetMode(Mode m);
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    Resize();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    //void    OnLButtonUp();

public:
    void    OnPaint();

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
        //, selection(ellipses.end())
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
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
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);


        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

        /*for (auto i = ellipses.begin(); i != ellipses.end(); ++i)
        {
            (*i)->Draw(pRenderTarget, pBrush);
        }

        if (Selection())
        {
            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            pRenderTarget->DrawEllipse(Selection()->ellipse, pBrush, 2.0f);
        }*/

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
    
    
}

//Algorithm window

class AlgorithmWindow : public BaseWindow<AlgorithmWindow>
{
    enum Mode
    {
        //Change to only one mode?
        DragMode,
        SelectMode,
        DrawMode
    };

    HCURSOR                 hCursor;

    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    D2D1_POINT_2F           ptMouse;
    D2D1_POINT_2F           click_pos;

    Mode                    mode;
    size_t                  nextColor;

    list<shared_ptr<MyEllipse>>             ellipses;
    list<shared_ptr<MyEllipse>>::iterator   selection;

    // Don't worry about these two, Will - they're for QuickHull and Point Convex, which will not be moved.
    vector<D2D1_ELLIPSE> big_points;
    vector<D2D1_ELLIPSE> small_points;

    // These are the movable hulls!
    vector<D2D1_ELLIPSE> hull1;
    vector<D2D1_ELLIPSE> hull2;

    vector<D2D1_ELLIPSE> static_hull;

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
    //HRESULT InsertEllipse(float x, float y);

    /*MyEllipse PointFarthestFromEdge(MyEllipse a, MyEllipse b, list<shared_ptr<MyEllipse>> p);
    bool    Contains(list<MyEllipse> points, MyEllipse to_be_found);*/

    BOOL    HitTest(float x, float y);
    void    SetMode(Mode m);
    void    MoveSelection(float x, float y);
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    Resize();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();
    void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
    void    OnKeyDown(UINT vkey);
    void    OnPaint();
    void    RenderEdges(vector<D2D1_ELLIPSE> points);
    void    DrawAxes();
    void    UpdateEllipses();
    D2D1_ELLIPSE point_convex;

    // Hull being moved
    vector<D2D1_ELLIPSE> moving_hull;

    // Index of moved hull
    int move_ind;

    vector<D2D1_ELLIPSE> AlgorithmWindow::Translate(vector<D2D1_ELLIPSE> points, int dir);

public:

    AlgorithmWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0), selection(ellipses.end())
    {
    }

    HRESULT InsertEllipse(float x, float y);
    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void AlgorithmWindow::DrawAxes() {
    D2D1_POINT_2F p1 = D2D1::Point2F(0, pRenderTarget->GetSize().height / 2);
    D2D1_POINT_2F q1 = D2D1::Point2F(pRenderTarget->GetSize().width, pRenderTarget->GetSize().height / 2);
    D2D1_POINT_2F p2 = D2D1::Point2F(pRenderTarget->GetSize().width / 2, 0);
    D2D1_POINT_2F q2 = D2D1::Point2F(pRenderTarget->GetSize().width / 2, pRenderTarget->GetSize().height);
    pRenderTarget->DrawLine(p1, q1, pBrush);
    pRenderTarget->DrawLine(p2, q2, pBrush);
}


vector<D2D1_ELLIPSE> FirstHull(vector<D2D1_ELLIPSE> points, int start, int end) {
    vector<D2D1_ELLIPSE> hull;
    for (size_t i = start; i < end; i++) {
        points[i].point.x = rand() % 400 + 10;
        points[i].point.y = rand() % 100 + 10;
        hull.push_back(points[i]);
    }
    return hull;
}

vector<D2D1_ELLIPSE> SecondHull(vector<D2D1_ELLIPSE> points, int start, int end) {
    vector<D2D1_ELLIPSE> hull;
    for (size_t i = start; i < end; i++) {
        points[i].point.x = rand() % 300 + 500;
        points[i].point.y = rand() % 100 + 200;
        hull.push_back(points[i]);
    }
    return hull;
}

HRESULT AlgorithmWindow::CreateGraphicsResources()
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
            for (size_t i = 0; i < 10; i++)
            {
                D2D1_ELLIPSE adding = D2D1::Ellipse(D2D1::Point2F(rand() % 780 + 10, rand() % 260 + 10), 10.0f, 10.0f);
                InsertEllipse(adding.point.x, adding.point.y);
                big_points.push_back(adding);
            }
            hull1 = FirstHull(big_points, 0, 5);
            hull2 = SecondHull(big_points, 5, 10);
            for each (D2D1_ELLIPSE var in hull1) {
                small_points.push_back(var);
            }
            for each (D2D1_ELLIPSE var in hull2) {
                small_points.push_back(var);
            }
            point_convex.point.x = pRenderTarget->GetSize().width / 2;
            point_convex.point.y = pRenderTarget->GetSize().height / 2;
            InsertEllipse(point_convex.point.x, point_convex.point.y);

        }
    }
    return hr;
}

void AlgorithmWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

int ComparePoints(const void* vp1, const void* vp2) {
    D2D1_ELLIPSE* p1 = (D2D1_ELLIPSE*)vp1;
    D2D1_ELLIPSE* p2 = (D2D1_ELLIPSE*)vp2;

    int ori = HullMath::PointOri(first_point, *p1, *p2);
    if (ori == 0) {
        return (HullMath::PointDistance(first_point, *p2) >= HullMath::PointDistance(first_point, *p1)) ? -1 : 1;
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

vector<D2D1_ELLIPSE> AlgorithmWindow::Translate(vector<D2D1_ELLIPSE> points, int dir) {
    vector<D2D1_ELLIPSE> translated;
    for each (D2D1_ELLIPSE var in points) {
        float x_co = var.point.x + ((pRenderTarget->GetSize().width / 2) * dir);
        float y_co = var.point.y + ((pRenderTarget->GetSize().height / 2) * dir);
        translated.push_back(D2D1::Ellipse(D2D1::Point2F(x_co, y_co), 10.0f, 10.0f));
    }
    return translated;
}

void AlgorithmWindow::RenderEdges(vector<D2D1_ELLIPSE> points) {
    for (size_t i = 0; i < points.size() - 1; i++) {
        D2D1_POINT_2F point_a = D2D1::Point2F(points[i].point.x, points[i].point.y);
        D2D1_POINT_2F point_b = D2D1::Point2F(points[i+1].point.x, points[i+1].point.y);
        pRenderTarget->DrawLine(point_a, point_b, pBrush);

    }

    D2D1_POINT_2F point_a = D2D1::Point2F(points[points.size()-1].point.x, points[points.size()-1].point.y);
    D2D1_POINT_2F point_b = D2D1::Point2F(points[0].point.x, points[0].point.y);
    pRenderTarget->DrawLine(point_a, point_b, pBrush);
}

void AlgorithmWindow::UpdateEllipses() {
    int index = 0;
    if (current_alg == QHull || current_alg == PointHull) {
        for (auto i = ellipses.begin(); i != ellipses.end(); ++i) {
            if (index == 10)
                break;
            (*i)->ellipse.point.x = big_points[index].point.x;
            (*i)->ellipse.point.y = big_points[index].point.y;
            index++;
        }
        return;
    }
    for (auto i = ellipses.begin(); i != ellipses.end(); ++i) {
        if (index == 10)
            break;
        (*i)->ellipse.point.x = small_points[index].point.x;
        (*i)->ellipse.point.y = small_points[index].point.y;
        index++;
    }
}

void AlgorithmWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));


        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        vector<D2D1_ELLIPSE> hullpoints;

        int index = 0;
        for (auto i = ellipses.begin(); i != ellipses.end(); ++i)
        {
            if (index != 10) {
                (hullpoints).push_back((*i)->ellipse);
                pRenderTarget->DrawEllipse((*i)->ellipse, pBrush);
            }
            index++;
        }

        UpdateEllipses();

        if (current_alg == MinkDiff || current_alg == MinkSum || current_alg == GJK) {

            DrawAxes();
            
            for (int i = 0; i < 5; i++) {
                hull1[i] = small_points[i];
            }
            for (int i = 0; i < 5; i++) {
                hull2[i] = small_points[i+5];
            }
            
            vector<D2D1_ELLIPSE> hull3;

            if (current_alg == MinkDiff || current_alg == GJK) {
                hull3 = Translate(HullMath::MinkowskiDiff(Translate(hull1, -1), Translate(hull2, -1)), 1);
            }

            if (current_alg == MinkSum) {
                hull3 = Translate(HullMath::MinkowskiSum(Translate(hull1, -1), Translate(hull2, -1)), 1);
            }

            QuickHull* qhull1 = new QuickHull(hull1);
            QuickHull* qhull2 = new QuickHull(hull2);
            QuickHull* qhull3 = new QuickHull(hull3);

            vector<D2D1_ELLIPSE> sorted_hull1 = SortPoints(qhull1->GetConvexHull());
            vector<D2D1_ELLIPSE> sorted_hull2 = SortPoints(qhull2->GetConvexHull());
            vector<D2D1_ELLIPSE> sorted_hull3 = SortPoints(qhull3->GetConvexHull());


            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            RenderEdges(sorted_hull1);

            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
            RenderEdges(sorted_hull2);


            if (current_alg == GJK && HullMath::HullsIntersecting(sorted_hull1, sorted_hull2)) {
                pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
            }
            else {
                pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Magenta));
            }
            RenderEdges(sorted_hull3);
        }
        // RENDERING QUICKHULL
        if (current_alg == QHull) {


            QuickHull* qhull = new QuickHull(hullpoints);

            vector<D2D1_ELLIPSE> q_points = (qhull)->GetConvexHull();

            vector<D2D1_ELLIPSE> quick_hull_points = SortPoints(q_points);

            RenderEdges(quick_hull_points);
        }

        if (current_alg == PointHull) {
            QuickHull* qhull = new QuickHull(hullpoints);

            vector<D2D1_ELLIPSE> q_points = (qhull)->GetConvexHull();

            vector<D2D1_ELLIPSE> quick_hull_points = SortPoints(q_points);

            D2D1_ELLIPSE point_check;

            RenderEdges(quick_hull_points);

            int index = 0;
            for (auto i = ellipses.begin(); i != ellipses.end(); i++) {
                if (index == 10) {
                    point_check = (*i)->ellipse;
                    if (HullMath::ContainsPoint(quick_hull_points, point_check)) {
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
                    }
                    else {
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
                    }
                    pRenderTarget->FillEllipse((*i)->ellipse, pBrush);
                }
                else {
                    pRenderTarget->DrawEllipse((*i)->ellipse, pBrush);
                }
                index++;
            }
        }

        if (Selection())
        {
            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange));
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

void AlgorithmWindow::Resize()
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

void AlgorithmWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);
    click_pos.x = dipX;
    click_pos.y = dipY;
    D2D1_ELLIPSE pos;
    pos.point.x = dipX;
    pos.point.y = dipY;

    if (current_alg == MinkDiff || current_alg == MinkSum || current_alg == GJK) {
        QuickHull* qhull1 = new QuickHull(hull1);
        QuickHull* qhull2 = new QuickHull(hull2);

        vector<D2D1_ELLIPSE> sorted_hull1 = SortPoints(qhull1->GetConvexHull());
        vector<D2D1_ELLIPSE> sorted_hull2 = SortPoints(qhull2->GetConvexHull());

        if (HullMath::ContainsPoint(sorted_hull1, pos)) {
            moving_hull = hull1;
            move_ind = 1;
        }
        if (HullMath::ContainsPoint(sorted_hull2, pos)) {
            moving_hull = hull2;
            move_ind = 2;
        }
    }
    else {
        QuickHull* qhull1 = new QuickHull(big_points);

        vector<D2D1_ELLIPSE> sorted_hull1 = SortPoints(qhull1->GetConvexHull());

        if (HullMath::ContainsPoint(sorted_hull1, pos)) {
            moving_hull = big_points;
            move_ind = 3;
        }

    }

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

void AlgorithmWindow::OnLButtonUp()
{
    move_ind = 0;
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

void AlgorithmWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);

    if ((flags & MK_LBUTTON) && Selection())
    {
        if (mode == DragMode)
        {
            // Move the ellipse.
            vector<D2D1_ELLIPSE> new_points;
            Selection()->ellipse.point.x = dipX + ptMouse.x;
            Selection()->ellipse.point.y = dipY + ptMouse.y;

            for (auto i = ellipses.begin(); i != ellipses.end(); i++) {
                D2D1_ELLIPSE point1 = D2D1::Ellipse(D2D1::Point2F((*i)->ellipse.point.x, (*i)->ellipse.point.y), 10.0f, 10.0f);
                new_points.push_back(point1);
            }
            if (current_alg == QHull || current_alg == PointHull) {
                big_points = new_points;
            }
            else {
                small_points = new_points;
            }

        }
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    else if (flags && MK_LBUTTON && move_ind != 0) {
        int index = 0;
        vector<D2D1_ELLIPSE> new_points;
        switch (move_ind) {
        case 1:
            for (auto i = ellipses.begin(); i != ellipses.end(); i++) {
                if (index < 5) {
                    (*i)->ellipse.point.x = moving_hull[index].point.x + (dipX - click_pos.x);
                    (*i)->ellipse.point.y = moving_hull[index].point.y + (dipY - click_pos.y);
                }
                D2D1_ELLIPSE point1 = D2D1::Ellipse(D2D1::Point2F((*i)->ellipse.point.x, (*i)->ellipse.point.y), 10.0f, 10.0f);
                new_points.push_back(point1);
                index++;
            }
            small_points = new_points;
            break;
        case 2:
            for (auto i = ellipses.begin(); i != ellipses.end(); i++) {
                if (index > 4 && index < 10) {
                    (*i)->ellipse.point.x = moving_hull[index-5].point.x + (dipX - click_pos.x);
                    (*i)->ellipse.point.y = moving_hull[index-5].point.y + (dipY - click_pos.y);
                }
                D2D1_ELLIPSE point1 = D2D1::Ellipse(D2D1::Point2F((*i)->ellipse.point.x, (*i)->ellipse.point.y), 10.0f, 10.0f);
                new_points.push_back(point1);
                index++;
            }
            small_points = new_points;
            break;
        case 3:
            for (auto i = ellipses.begin(); i != ellipses.end(); i++) {
                if (index < 10) {
                    (*i)->ellipse.point.x = moving_hull[index].point.x + (dipX - click_pos.x);
                    (*i)->ellipse.point.y = moving_hull[index].point.y + (dipY - click_pos.y);
                    D2D1_ELLIPSE point1 = D2D1::Ellipse(D2D1::Point2F((*i)->ellipse.point.x, (*i)->ellipse.point.y), 10.0f, 10.0f);
                    new_points.push_back(point1);
                }
                index++;
            }
            big_points = new_points;
            break;
        default:
            break;
        }
    }
}

void AlgorithmWindow::OnKeyDown(UINT vkey)
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

HRESULT AlgorithmWindow::InsertEllipse(float x, float y)
{
    try
    {
        selection = ellipses.insert(
            ellipses.end(),
            shared_ptr<MyEllipse>(new MyEllipse()));

        Selection()->ellipse.point = ptMouse = D2D1::Point2F(x, y);
        Selection()->ellipse.radiusX = Selection()->ellipse.radiusY = 10.0f;
        Selection()->color = D2D1::ColorF(colors[nextColor]);

        nextColor = (nextColor + 1) % ARRAYSIZE(colors);
    }
    catch (std::bad_alloc)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

BOOL AlgorithmWindow::HitTest(float x, float y)
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

void AlgorithmWindow::MoveSelection(float x, float y)
{
    if (Selection())
    {
        Selection()->ellipse.point.x += x;
        Selection()->ellipse.point.y += y;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void AlgorithmWindow::SetMode(Mode m)
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

AlgorithmWindow algwin;

//Button 1 window
class Button1Window : public BaseWindow<Button1Window>
{
    enum Mode
    {
        SelectMode
    };

    HCURSOR                 hCursor;

    ID2D1Factory            *pFactory;
    ID2D1HwndRenderTarget   *pRenderTarget;
    ID2D1SolidColorBrush    *pBrush;
    D2D1_POINT_2F           ptMouse;

    Mode                    mode;
    size_t                  nextColor;

    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    SetMode(Mode m);
    void    OnPaint();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();

public:

    Button1Window() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT Button1Window::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        EndPaint(m_hwnd, &ps);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void Button1Window::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void Button1Window::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    current_alg = MinkDiff;
    SendMessage(algwin.Window(), WM_PAINT, 0, 0);
}

void Button1Window::OnLButtonUp()
{
    /*HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }*/
}

void Button1Window::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    TextOut(hdc,
        // Location of the text
        0,
        5,
        // Text to print
        L"Minkowski",
        // Size of the text, my function gets this for us
        sizeof("Minkowski"));

    TextOut(hdc,
        // Location of the text
        0,
        25,
        // Text to print
        L"Difference",
        // Size of the text, my function gets this for us
        sizeof("Difference"));

    EndPaint(m_hwnd, &ps);
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {        
     
    }
}

void Button1Window::SetMode(Mode m)
{
    mode = m;

    LPWSTR cursor;
    cursor = IDC_HAND;
    
    hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}

//Button 2 window
class Button2Window : public BaseWindow<Button2Window>
{

    HCURSOR                 hCursor;

    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    D2D1_POINT_2F           ptMouse;

    size_t                  nextColor;


    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();

public:

    Button2Window() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT Button2Window::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        EndPaint(m_hwnd, &ps);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void Button2Window::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void Button2Window::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    current_alg = MinkSum;
    SendMessage(algwin.Window(), WM_PAINT, 0, 0);
}

void Button2Window::OnLButtonUp()
{
}

void Button2Window::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    TextOut(hdc,
        // Location of the text
        0,
        5,
        // Text to print
        L"Minkowski",
        // Size of the text, my function gets this for us
        sizeof("Minkowski"));

    TextOut(hdc,
        // Location of the text
        0,
        25,
        // Text to print
        L"Sum",
        // Size of the text, my function gets this for us
        sizeof("Sum"));

    EndPaint(m_hwnd, &ps);
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {

    }
}

//Button 3 window
class Button3Window : public BaseWindow<Button3Window>
{

    HCURSOR                 hCursor;

    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    D2D1_POINT_2F           ptMouse;

    size_t                  nextColor;


    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();

public:

    Button3Window() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT Button3Window::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        EndPaint(m_hwnd, &ps);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void Button3Window::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void Button3Window::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    current_alg = QHull;
    SendMessage(algwin.Window(), WM_PAINT, 0, 0);
}

void Button3Window::OnLButtonUp()
{
}

void Button3Window::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    TextOut(hdc,
        // Location of the text
        0,
        15,
        // Text to print
        L"Quickhull",
        // Size of the text, my function gets this for us
        sizeof("Quickhull"));

    EndPaint(m_hwnd, &ps);
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {

    }
}

//Button 4 window
class Button4Window : public BaseWindow<Button4Window>
{

    HCURSOR                 hCursor;

    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    D2D1_POINT_2F           ptMouse;

    size_t                  nextColor;


    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();

public:

    Button4Window() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT Button4Window::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        EndPaint(m_hwnd, &ps);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void Button4Window::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void Button4Window::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    current_alg = PointHull;
    SendMessage(algwin.Window(), WM_PAINT, 0, 0);
}

void Button4Window::OnLButtonUp()
{
}

void Button4Window::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    TextOut(hdc,
        // Location of the text
        0,
        5,
        // Text to print
        L"Point Convex",
        // Size of the text, my function gets this for us
        sizeof("Point Convex"));

    TextOut(hdc,
        // Location of the text
        0,
        25,
        // Text to print
        L"Hull",
        // Size of the text, my function gets this for us
        sizeof("Hull"));

    EndPaint(m_hwnd, &ps);
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {

    }
}

//Button 5 window
class Button5Window : public BaseWindow<Button5Window>
{

    HCURSOR                 hCursor;

    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    D2D1_POINT_2F           ptMouse;

    size_t                  nextColor;


    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();

public:

    Button5Window() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0)
    {
    }

    PCWSTR  ClassName() const { return L"Button Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT Button5Window::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        EndPaint(m_hwnd, &ps);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void Button5Window::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void Button5Window::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    current_alg = GJK;
    SendMessage(algwin.Window(), WM_PAINT, 0, 0);
}

void Button5Window::OnLButtonUp()
{
}

void Button5Window::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    TextOut(hdc,
        // Location of the text
        0,
        25,
        // Text to print
        L"GJK",
        // Size of the text, my function gets this for us
        sizeof("GJK"));

    EndPaint(m_hwnd, &ps);
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {

    }
}





//MAIN PROGRAM

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Gaming algorithms", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN))
    {
        return 0;
    }

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));

    ShowWindow(win.Window(), nCmdShow);

    // Creating buttons
    // But 1
    Button1Window button1;

    if (!button1.Create(L"Button 1", WS_VISIBLE | WS_CHILD | WS_BORDER,
        10, 30, 100, 50, win.Window()))
    {
        return 0;
    }

    ShowWindow(button1.Window(), nCmdShow);
    
    // But2
    Button2Window button2;

    if (!button2.Create(L"Button 2", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        10, 110, 100, 50, win.Window()))
    {
        return 0;
    }

    ShowWindow(button2.Window(), nCmdShow);
    
    // But3
    Button3Window button3;

    if (!button3.Create(L"Button 3", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        10, 190, 100, 50, win.Window()))
    {
        return 0;
    }

    ShowWindow(button3.Window(), nCmdShow);
    
    // But4
    Button4Window button4;

    if (!button4.Create(L"Button 4", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        10, 270, 100, 50, win.Window()))
    {
        return 0;
    }

    ShowWindow(button4.Window(), nCmdShow);
    
    // But5
    Button5Window button5;

    if (!button5.Create(L"Button 5", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        10, 350, 100, 50, win.Window()))
    {
        return 0;
    }

    ShowWindow(button5.Window(), nCmdShow);
    
    // Algorithm Window
    AlgorithmWindow algwin;
    if (!algwin.Create(L"Algorithms", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        120, 30, 800, 370, win.Window()))
    {
        return 0;
    }

    ShowWindow(algwin.Window(), nCmdShow);

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

LRESULT AlgorithmWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        Resize();
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

LRESULT Button1Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        SetMode(SelectMode); 
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_LBUTTONDOWN:
        
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;
    
    case WM_LBUTTONUP:
        OnLButtonUp();
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

LRESULT Button2Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp();
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

LRESULT Button3Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp();
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

LRESULT Button4Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp();
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

LRESULT Button5Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp();
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
        //SetMode(SelectMode);        
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
    {
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;
    }
    /*case WM_LBUTTONUP:
        OnLButtonUp();
        return 0;*/

    /*case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;*/

    /*case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(hCursor);
            return TRUE;
        }
        break;*/

    /*case WM_KEYDOWN:
        OnKeyDown((UINT)wParam);
        return 0;*/

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
