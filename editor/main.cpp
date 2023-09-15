#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>
#include "../common/math.hpp"
#include "../common/crn.hpp"
#include "../common/string.hpp"
#include <vector>
#include <sstream>
#include <memory>
#include <Shlwapi.h>
#include "../common/file.hpp"
#include "resource.h"
#include <queue>

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

#define APP_NAME TEXT("CRN エディタ")

#define DEBUG_LOG(...)                                                \
    {                                                                 \
        StringStream ss;                                              \
        ss << __VA_ARGS__;                                            \
        MessageBox(NULL, ss.str().c_str(), TEXT("DEBUG_LOG"), MB_OK); \
    }

#define XY(point) static_cast<Gdiplus::REAL>((point).x), static_cast<Gdiplus::REAL>((point).y)
#define CR2RECT(center, radius) static_cast<Gdiplus::REAL>((center).x - (radius)), \
                                static_cast<Gdiplus::REAL>((center).y - (radius)), \
                                (radius)*2,                                        \
                                (radius)*2

HWND hWnd;
int need_repaint = 0; // 0:なし 1:画像エリア再描画 2:ウィンドウ全体を再描画

math::Point circleCenter{100, 100};
double circleRadius = 90;
std::vector<math::Point> points{{10, 10}, {190, 190}};

const int OBJECT_NONE = -1;
const int OBJECT_CIRCLE = -2;
int activeObject = OBJECT_NONE;

bool editing = false;
std::unique_ptr<Gdiplus::Image> image = nullptr;
math::Point imageSize;
math::Transform imageTransform; // 画像座標からウィンドウ座標への変換
math::Point windowAreaSize;
constexpr int imageAreaY = 24 * 3 + 4 * 4;
CrnData crnData;
String crnPath;

// 左右キーで別のファイルに移動するための情報
String pngDirectory;
std::vector<String> pngFileNames;
int pngFileIndex = 0;

HWND hWnd_characterName, hWnd_imageName, hWnd_lineType, hWnd_direction;
std::queue<String> errorMessageQueue;

void showError(const std::wstring &message)
{
    if (!message.empty())
    {
        // 画面再描画後にウィンドウを出す
        errorMessageQueue.push(message);
        need_repaint = 2;
    }
}

void recalcImageTransform()
{
    if (image != nullptr)
    {
        imageSize = {static_cast<double>(image->GetWidth()), static_cast<double>(image->GetHeight())};
        imageTransform.scale = std::min(windowAreaSize.x / imageSize.x, windowAreaSize.y / imageSize.y);
        imageTransform.shift = (windowAreaSize - imageSize * imageTransform.scale) / 2;
        imageTransform.shift.y += imageAreaY;
        need_repaint = 2;
    }
}

CrnData defaultCrnData(const int imageWidth, const int imageHeight)
{
    crnData = CrnData{};
    const double x = imageWidth * 0.5;
    const double h = imageHeight;
    crnData.headPosition = {x, h * 0.25};
    crnData.headRadius = h * 0.15;
    crnData.centerLine = {{x, h * 0.05}, {x, h * 0.95}};
    return crnData;
}

VOID OnPaint(HDC hdc)
{
    Gdiplus::Graphics windowGraphics(hdc);

    // 画面のチラつき防止のためにバッファに描画
    Gdiplus::Bitmap bufferBitmap(XY(imageSize * imageTransform.scale));
    Gdiplus::Graphics *graphics = Gdiplus::Graphics::FromImage(&bufferBitmap);

    math::Transform displayTransform;
    displayTransform.shift = -imageTransform.shift;

    // 色設定
    const Gdiplus::Color backgroundColor{255, 240, 240, 240};
    const Gdiplus::Color imageBackgroundColor{255, 255, 255, 255};
    const Gdiplus::Color activeColor{255, 255, 0, 0};
    const Gdiplus::Color lineColor{255, 0, 0, 255};
    const Gdiplus::Color pointEdgeColor{255, 0, 0, 0};
    const Gdiplus::Color helpLineColor{128, 255, 0, 255};
    const double lineWidth = 3.0;
    const double pointRadius = 8.0;
    const double pointEdgeWidth = 1.0;
    const double helpLineWidth = 1.0;

    // 背景
    graphics->Clear(image != nullptr ? imageBackgroundColor : backgroundColor);

    if (image != nullptr)
    {
        // 画像
        graphics->DrawImage(image.get(),
                            XY(displayTransform * imageTransform.shift),
                            XY(imageSize * imageTransform.scale * displayTransform.scale));

        // 中心座標インジケータ（補助線）
        {
            const auto c = displayTransform * math::lineCentroid(points);
            const Gdiplus::Pen pen(helpLineColor, helpLineWidth);
            graphics->DrawLine(&pen, c.x, 0.0, c.x, static_cast<Gdiplus::REAL>(bufferBitmap.GetHeight()));
            graphics->DrawLine(&pen, 0.0, c.y, static_cast<Gdiplus::REAL>(bufferBitmap.GetWidth()), c.y);
        }

        // キャラクター頭部円
        {
            const Gdiplus::Pen pen(activeObject == OBJECT_CIRCLE ? activeColor : lineColor, lineWidth);
            const math::Point c = displayTransform * circleCenter;
            const double r = displayTransform * circleRadius;
            graphics->DrawEllipse(&pen, CR2RECT(c, r));
        }

        // キャラクター中心線

        for (int i = 0; i < points.size() - 1; i++)
        {
            const Gdiplus::Pen pen(lineColor, lineWidth);
            const math::Point p = displayTransform * points[i], q = displayTransform * points[i + 1];
            graphics->DrawLine(&pen, XY(p), XY(q));
        }

        for (int i = 0; i < points.size(); i++)
        {
            const Gdiplus::SolidBrush brush(activeObject == i ? activeColor : lineColor);
            const Gdiplus::Pen pen(pointEdgeColor, pointEdgeWidth);
            const math::Point p = displayTransform * points[i];
            graphics->FillEllipse(&brush, CR2RECT(p, pointRadius));
            graphics->DrawEllipse(&pen, CR2RECT(p, pointRadius));
        }
    }

    if (need_repaint >= 2)
    {
        windowGraphics.Clear(backgroundColor);
    }
    windowGraphics.DrawImage(&bufferBitmap, XY(-displayTransform.shift));
    delete graphics;

    need_repaint = 0;

    while (!errorMessageQueue.empty())
    {
        MessageBox(hWnd, errorMessageQueue.front().c_str(), APP_NAME, MB_ICONERROR | MB_OK);
        errorMessageQueue.pop();
    }
}

bool inCrnDataToViewData = false;

void crnDataToViewData(const CrnData &crnData, bool onlyPoints = false)
{
    circleCenter = imageTransform * crnData.headPosition;
    circleRadius = imageTransform * crnData.headRadius;
    points.clear();
    for (const auto &p : crnData.centerLine)
    {
        points.push_back(imageTransform * p);
    }

    if (onlyPoints)
        return;

    inCrnDataToViewData = true;

    SetWindowTextW(hWnd_characterName, string::utf8_to_wstring(crnData.characterName).c_str());
    SetWindowTextW(hWnd_imageName, string::utf8_to_wstring(crnData.imageName).c_str());
    SendMessage(hWnd_lineType, CB_SETCURSEL, crnData.centerLine.size() - 2, 0);
    SendMessage(hWnd_direction, CB_SETCURSEL, static_cast<int>(crnData.direction), 0);

    inCrnDataToViewData = false;
}

void viewDataToCrnData(CrnData &crnData, bool onlyPoints = false)
{
    const auto transform = imageTransform.inverse();

    crnData.headPosition = transform * circleCenter;
    crnData.headRadius = transform * circleRadius;
    crnData.centerLine.clear();
    for (const auto &p : points)
    {
        crnData.centerLine.push_back(transform * p);
    }

    // 正規化
    auto reg = [](math::Point &p)
    {
        p.x = std::clamp(p.x, 0.0, imageSize.x);
        p.y = std::clamp(p.y, 0.0, imageSize.y);
    };
    reg(crnData.headPosition);
    crnData.headRadius = std::clamp(crnData.headRadius, 10.0, std::max(imageSize.x, imageSize.y) / 2);
    for (auto &p : crnData.centerLine)
    {
        reg(p);
    }

    if (onlyPoints)
        return;

    // コントロール類
    static TCHAR str[1024] = {};
    GetWindowText(hWnd_characterName, str, sizeof(str) / sizeof(TCHAR));
    crnData.characterName = string::wstring_to_utf8(str);
    GetWindowText(hWnd_imageName, str, sizeof(str) / sizeof(TCHAR));
    crnData.imageName = string::wstring_to_utf8(str);
    crnData.direction = static_cast<CrnData::Direction>(SendMessage(hWnd_direction, CB_GETCURSEL, 0, 0));
}

bool openFile(String filePath, const bool createPngIndex = false, const bool keepWindowTitleWhenFailed = false)
{
    activeObject = OBJECT_NONE;
    editing = false;
    need_repaint = 2;

    std::unique_ptr<Gdiplus::Image> newImage = nullptr;

    String errorMessage;

    TCHAR windowTitleBuffer[keepWindowTitleWhenFailed ? MAX_PATH + 100 : 0];
    if (keepWindowTitleWhenFailed)
        GetWindowText(hWnd, windowTitleBuffer, sizeof(windowTitleBuffer) / sizeof(TCHAR));

    try
    {
        // フォルダが指定された場合、フォルダ内の最初のpngファイルを開く
        const auto fileAttribute = GetFileAttributes(filePath.c_str());
        if (fileAttribute != INVALID_FILE_ATTRIBUTES && (fileAttribute & FILE_ATTRIBUTE_DIRECTORY))
        {
            pngDirectory = filePath;
            if (*pngDirectory.rbegin() != '\\')
            {
                pngDirectory += '\\';
            }
            WIN32_FIND_DATA ffd;
            HANDLE hFind = FindFirstFile((pngDirectory + TEXT("*.png")).c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                filePath = pngDirectory + ffd.cFileName;
            }
            else
            {
                errorMessage = TEXT("フォルダ ") + pngDirectory + TEXT(" 内にPNG画像はありません");
                throw std::exception();
            }
        }

        // CRNファイルが指定された場合、対応する画像ファイル名に変換
        const auto imagePath = [&]() -> String
        {
            const auto ext = string::lower(filePath.size() >= 4 ? filePath.substr(filePath.size() - 4) : TEXT(""));
            if (ext == TEXT(".crn"))
            {
                crnPath = filePath;
                return filePath.substr(0, filePath.size() - 4);
            }
            else
            {
                crnPath = filePath + TEXT(".crn");
                return filePath;
            }
        }();
        SetWindowText(hWnd, (imagePath + TEXT(" - ") APP_NAME).c_str());

        // 後に左右キーで別の画像ファイルに移動できるようにインデックスを作成する
        if (createPngIndex)
        {
            const auto sp = imagePath.find_last_of('\\');
            String targetLowerName;
            if (sp != String::npos)
            {
                pngDirectory = imagePath.substr(0, sp + 1);
                targetLowerName = string::lower(imagePath.substr(sp + 1));
            }
            else
            {
                pngDirectory = TEXT(".\\");
                targetLowerName = string::lower(imagePath);
            }

            pngFileNames = getFileList(pngDirectory + TEXT("*.png"));
            pngFileIndex = 0;
            for (int i = 0; i < pngFileNames.size(); i++)
            {
                if (string::lower(pngFileNames[i]) == targetLowerName)
                {
                    pngFileIndex = i;
                    break;
                }
            }
        }

        // 画像読み込み
        try
        {
            newImage = std::make_unique<Gdiplus::Bitmap>(string::toWideChar(imagePath).c_str());
            if (newImage->GetLastStatus() != Gdiplus::Status::Ok)
            {
                throw std::exception();
            }
        }
        catch (...)
        {
            errorMessage = TEXT("画像ファイル ") + imagePath + TEXT(" の読み込みに失敗しました");
            throw;
        }

        // crnデータ読み込み
        if (PathFileExists(crnPath.c_str()))
        {
            // crnファイルあり：読み込む
            try
            {
                crnData = CrnData::load(string::toMultiByte(crnPath));
            }
            catch (const CrnData::jsonVersionException &)
            {
                errorMessage = TEXT("CRNファイル") + crnPath + TEXT(" のバージョンが本アプリケーションのサポート対象外です。");
                throw;
            }
            catch (...)
            {
                errorMessage = TEXT("CRNファイル") + crnPath + TEXT(" の読み込みに失敗しました");
                throw;
            }
        }
        else
        {
            // crnファイルなし：画像サイズに基づくデフォルト値を使用
            crnData = defaultCrnData(newImage->GetWidth(), newImage->GetHeight());
        }

        image = std::move(newImage);
        recalcImageTransform();
        crnDataToViewData(crnData);
    }
    catch (...)
    {
        if (errorMessage.empty())
            errorMessage = TEXT("ファイル") + filePath + TEXT("の読み込み時に予期せぬエラーが発生しました");
        showError(errorMessage);

        if (keepWindowTitleWhenFailed)
            SetWindowText(hWnd, windowTitleBuffer);

        return false;
    }

    showError(TEXT(""));
    return true;
}

int pickObject(const math::Point &p)
{
    // 与えられた点に近い、ドラッグで動かせるパーツ（キャラクター中心線の端点・頭部円）

    const double hitAreaSize = 12.0;

    std::vector<std::pair<double, int>> candidates; // (distance, objectIndex)
    candidates.emplace_back(abs(p.distanceTo(circleCenter) - circleRadius), OBJECT_CIRCLE);
    for (int i = 0; i < points.size(); i++)
    {
        candidates.emplace_back(p.distanceTo(points[i]), i);
    }

    std::pair<double, int> result{hitAreaSize, OBJECT_NONE};
    for (const auto &x : candidates)
    {
        result = min(result, x);
    }
    return result.second;
}

bool changeLineType(const int newLineType)
{
    if (image == nullptr || editing)
        return false;

    if (newLineType == 2)
    {
        if (points.size() == 3)
        {
            points = {points[0], points[2]};
            need_repaint = 1;
            viewDataToCrnData(crnData, true);
            crnDataToViewData(crnData);
            return true;
        }
    }
    else if (newLineType == 3)
    {
        if (points.size() == 2)
        {
            points = {points[0], (points[0] + points[1]) / 2, points[1]};
            need_repaint = 1;
            viewDataToCrnData(crnData, true);
            crnDataToViewData(crnData);
            return true;
        }
    }
    else
    {
        throw std::invalid_argument("Unknown line type");
    }

    return false;
}

VOID OnMouseMove(const math::Point &p)
{
    static math::Point previousMousePosition = p;

    if (editing)
    {
        // 編集
        if (activeObject == OBJECT_CIRCLE && GetKeyState(VK_SHIFT) < 0)
        {
            circleRadius -= p.y - previousMousePosition.y;
        }
        else
        {
            math::Point &target = activeObject == OBJECT_CIRCLE ? circleCenter : points[activeObject];
            target += p - previousMousePosition;
        }

        // この変換を挟むと正規化される
        viewDataToCrnData(crnData, true);
        crnDataToViewData(crnData, true);

        need_repaint = 1;
    }
    else
    {
        const auto oldActiveObject = activeObject;
        activeObject = pickObject(p);
        if (activeObject != oldActiveObject)
        {
            need_repaint = 1;
        }
    }

    previousMousePosition = p;
}

VOID OnMouseDown(const math::Point &p)
{
    SetFocus(hWnd); // 画面外のMouseMoveを拾えるように
    if (activeObject != OBJECT_NONE)
    {
        editing = true;
    }
}

void save()
{
    if (image != nullptr)
    {
        viewDataToCrnData(crnData);
        crnData.save(string::toMultiByte(crnPath));
    }
}

void unload()
{
    image = nullptr;
    imageSize = {0, 0};

    crnData = CrnData{};
    editing = false;
    activeObject = OBJECT_NONE;

    inCrnDataToViewData = true;

    SetWindowTextW(hWnd_characterName, TEXT(""));
    SetWindowTextW(hWnd_imageName, TEXT(""));
    SendMessage(hWnd_lineType, CB_SETCURSEL, -1, 0);
    SendMessage(hWnd_direction, CB_SETCURSEL, -1, 0);
    SetWindowText(hWnd, APP_NAME);

    inCrnDataToViewData = false;

    need_repaint = 2;
}

void batchRenameCrn(const std::string &newName)
{
    if (image == nullptr)
        return;
    save();

    {
        StringStream confirmationMsg;
        confirmationMsg << TEXT("同じフォルダに存在する ")
                        << pngFileNames.size()
                        << TEXT(" 件のPNG画像のうち、キャラクター名が未設定のものを全て \"")
                        << string::toString(string::utf8_to_wstring(newName))
                        << TEXT("\" に変更しますか？")
                        << TEXT("\n\nこの操作は元に戻せません。")
                        << TEXT("\nまた、処理には数分かかる可能性があります。");
        if (MessageBox(hWnd, confirmationMsg.str().c_str(), TEXT("キャラクター名一括変更 - ") APP_NAME, MB_YESNO) != IDYES)
            return;
    }

    int changed = 0, skipped = 0, failed = 0;
    for (const auto &pngName : pngFileNames)
    {
        const String pngPath = pngDirectory + pngName;
        const String crnPath = pngPath + TEXT(".crn");
        try
        {
            CrnData data;
            if (PathFileExists(crnPath.c_str()))
            {
                data = CrnData::load(string::toMultiByte(crnPath));
                if (!data.characterName.empty())
                {
                    skipped++;
                    continue;
                }
            }
            else
            {
                Gdiplus::Bitmap img(string::toWideChar(pngPath).c_str());
                if (img.GetLastStatus() != Gdiplus::Status::Ok)
                    throw std::exception();
                data = defaultCrnData(img.GetWidth(), img.GetHeight());
            }
            data.characterName = newName;
            data.save(string::toMultiByte(crnPath));
            changed++;
        }
        catch (...)
        {
            failed++;
        }
    }

    StringStream resultMsg;
    resultMsg << TEXT("処理が完了しました\n\n")
              << TEXT("変更済み: ") << changed << TEXT(" 件    ")
              << TEXT("スキップ: ") << skipped << TEXT(" 件    ")
              << TEXT("エラー: ") << failed << TEXT(" 件");
    MessageBox(hWnd, resultMsg.str().c_str(), TEXT("キャラクター名一括変更 - ") APP_NAME, MB_OK);
}

VOID OnMouseUp(const math::Point &p)
{
    if (editing)
    {
        editing = false;

        activeObject = pickObject(p);

        need_repaint = 1;
    }
}

VOID OnResize()
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    windowAreaSize.x = clientRect.right;
    windowAreaSize.y = std::max(clientRect.bottom - imageAreaY, (LONG)1);

    if (image != nullptr)
    {
        recalcImageTransform();
        crnDataToViewData(crnData, true);
    }

    need_repaint = 2;
}

VOID OnKeyDown(const WPARAM key)
{
    if (key == VK_LEFT)
    {
        // Shift + 左キー : 向きを「左向き」に設定
        if (GetKeyState(VK_SHIFT) < 0)
        {
            if (image != nullptr)
            {
                crnData.direction = CrnData::Direction::Left;
                crnDataToViewData(crnData);
                return;
            }
        }

        // 左キー : 前の画像へ移動
        if (pngFileIndex - 1 >= 0)
        {
            save();
            unload();
            pngFileIndex--;
            openFile(pngDirectory + pngFileNames[pngFileIndex]);
        }
    }
    else if (key == VK_RIGHT)
    {
        // Shift + 右キー : 向きを「右向き」に設定
        if (GetKeyState(VK_SHIFT) < 0)
        {
            if (image != nullptr)
            {

                crnData.direction = CrnData::Direction::Right;
                crnDataToViewData(crnData);
                return;
            }
        }

        // 右キー : 次の画像へ移動
        if (pngFileIndex + 1 < pngFileNames.size())
        {
            save();
            unload();
            pngFileIndex++;
            openFile(pngDirectory + pngFileNames[pngFileIndex]);
        }
    }
    else if (key == VK_DOWN)
    {
        // Shift + 下キー：向きを「正面」に設定
        if (GetKeyState(VK_SHIFT) < 0)
        {
            if (image != nullptr)
            {
                crnData.direction = CrnData::Direction::None;
                crnDataToViewData(crnData);
                return;
            }
        }
    }
    else if (key == '2')
    {
        // 2キー : 中心線を「直線」に設定
        changeLineType(2);
    }
    else if (key == '3')
    {
        // 3キー : 中心線を「折れ線」に設定
        changeLineType(3);
    }
    else if (key == 'D')
    {
        // Dキー：デバッグ用
        // DEBUG_LOG(imageTransform.shift.x << TEXT(",") << imageTransform.shift.y << TEXT("    ") << imageTransform.scale);
    }
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, INT iCmdShow)
{
    MSG msg;
    WNDCLASS wndClass;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    // Initialize GDI+.
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("crnEditor");

    RegisterClass(&wndClass);

    hWnd = CreateWindow(
        TEXT("crnEditor"),   // window class name
        APP_NAME,            // window caption
        WS_OVERLAPPEDWINDOW, // window style
        CW_USEDEFAULT,       // initial x position
        CW_USEDEFAULT,       // initial y position
        CW_USEDEFAULT,       // initial x size
        CW_USEDEFAULT,       // initial y size
        NULL,                // parent window handle
        NULL,                // window menu handle
        hInstance,           // program instance handle
        NULL);               // creation parameters

    DragAcceptFiles(hWnd, TRUE);

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    OnResize(); // get window size and initialize GUI

    // コマンドライン引数が与えられたら、そのファイルパスを開く
    LPWSTR *szArglist;
    int nArgs;
    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (szArglist != nullptr && nArgs >= 2)
    {
        openFile(szArglist[1], true);
    }

    // メインループ
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (need_repaint > 0)
        {
            InvalidateRect(hWnd, NULL, FALSE);
            UpdateWindow(hWnd);
            Sleep(10);
        }
    }

    image = nullptr; // ちゃんと捨てておかないと怒られる
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return msg.wParam;
} // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                         WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:
    {
        // GUI作成（左上のコントロール群）

        constexpr int margin = 4;
        constexpr int rowHeight = 24;

        constexpr int labelWidth = 140;
        constexpr int buttonWidth = 80;
        constexpr int dropBoxWidth = 140;
        constexpr int panelWidth = 400;

        int x = margin;
        int y = margin;

        CreateWindow(TEXT("static"), TEXT("キャラクター名"),
                     WS_VISIBLE | WS_CHILD,
                     x, y, labelWidth, rowHeight,
                     hWnd, NULL, NULL, NULL);
        x += labelWidth + margin;
        hWnd_characterName = CreateWindow(TEXT("edit"), TEXT(""),
                                          WS_VISIBLE | WS_CHILD | WS_BORDER,
                                          x, y, panelWidth - labelWidth - buttonWidth - margin * 4, rowHeight,
                                          hWnd, NULL, NULL, NULL);
        x = panelWidth - buttonWidth - margin;
        CreateWindow(TEXT("button"), TEXT("一括変更"),
                     WS_VISIBLE | WS_CHILD,
                     x, y, buttonWidth, rowHeight,
                     hWnd, (HMENU)1, NULL, NULL);

        y += rowHeight + margin;
        x = margin;

        CreateWindow(TEXT("static"), TEXT("素材名"),
                     WS_VISIBLE | WS_CHILD,
                     x, y, labelWidth, rowHeight,
                     hWnd, NULL, NULL, NULL);
        x += labelWidth + margin;
        hWnd_imageName = CreateWindow(TEXT("edit"), TEXT(""),
                                      WS_VISIBLE | WS_CHILD | WS_BORDER,
                                      x, y, panelWidth - labelWidth - margin * 3, rowHeight,
                                      hWnd, NULL, NULL, NULL);

        y += rowHeight + margin;
        x = margin;

        hWnd_lineType = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
                                     WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                     x, y, dropBoxWidth, rowHeight * 4,
                                     hWnd, NULL, NULL, NULL);
        SendMessage(hWnd_lineType, CB_ADDSTRING, 0, (LPARAM)TEXT("中心線：直線"));
        SendMessage(hWnd_lineType, CB_ADDSTRING, 0, (LPARAM)TEXT("中心線：折れ線"));

        x += dropBoxWidth + margin;
        hWnd_direction = CreateWindow(TEXT("combobox"), TEXT(""),
                                      WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                      x, y, panelWidth - labelWidth - margin * 4 - 50, rowHeight * 5,
                                      hWnd, NULL, NULL, NULL);
        SendMessage(hWnd_direction, CB_ADDSTRING, 0, (LPARAM)TEXT("反転不可 または 正面"));
        SendMessage(hWnd_direction, CB_ADDSTRING, 0, (LPARAM)TEXT("反転可【左向き】"));
        SendMessage(hWnd_direction, CB_ADDSTRING, 0, (LPARAM)TEXT("反転可【右向き】"));

        x = panelWidth - 50 - margin;
        CreateWindow(TEXT("button"), TEXT("出力"),
                     WS_VISIBLE | WS_CHILD,
                     x, y, 50, rowHeight,
                     hWnd, (HMENU)2, NULL, NULL);

        y += rowHeight + margin;
        assert(y == imageAreaY);
    }
        return 0;

    case WM_COMMAND:
    {
        if (image == nullptr)
            return 0;

        const auto wID = LOWORD(wParam);
        const auto wNotifyCode = HIWORD(wParam);
        const auto hwndControl = reinterpret_cast<HWND>(lParam);

        if (wNotifyCode == EN_CHANGE && !inCrnDataToViewData)
        {
            // テキストボックス編集時：crnDataに反映
            viewDataToCrnData(crnData);
        }
        else if (wNotifyCode == CBN_SELCHANGE)
        {
            if (hwndControl == hWnd_lineType)
            {
                // 中心線タイプ変更
                if (!inCrnDataToViewData) // プログラム側からの変更でも発火してしまうのでブロック
                    changeLineType(SendMessage(hWnd_lineType, CB_GETCURSEL, 0, 0) + 2);
            }
            else if (hwndControl == hWnd_direction)
            {
                // 向き変更
                if (!inCrnDataToViewData) // プログラム側からの変更でも発火してしまうのでブロック
                    viewDataToCrnData(crnData);
            }
        }
        else if (wNotifyCode == BN_CLICKED)
        {
            if (wID == 1 /* 名前の一括変更 */)
            {
                batchRenameCrn(crnData.characterName);
            }
            else if (wID == 2 /* 出力 */)
            {
                // TODO
                DEBUG_LOG(TEXT("出力"));
            }
        }
    }
        return 0;
    case WM_DROPFILES:
    {
        // ファイルをドロップして開く
        save();
        unload();
        const auto hDrop = reinterpret_cast<HDROP>(wParam);
        TCHAR fileName[MAX_PATH] = {};
        DragQueryFile(hDrop, 0, fileName, sizeof(fileName) / sizeof(TCHAR));
        openFile(fileName, true);
    }
        return 0;
    case WM_SIZE:
        OnResize();
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove({static_cast<double>(GET_X_LPARAM(lParam)), static_cast<double>(GET_Y_LPARAM(lParam))});
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(hWnd);
        OnMouseDown({static_cast<double>(GET_X_LPARAM(lParam)), static_cast<double>(GET_Y_LPARAM(lParam))});
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        OnMouseUp({static_cast<double>(GET_X_LPARAM(lParam)), static_cast<double>(GET_Y_LPARAM(lParam))});
        return 0;
    case WM_KEYDOWN:
        OnKeyDown(wParam);
        return 0;
    case WM_CLOSE:
        if (image != nullptr)
            save();
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
} // WndProc
