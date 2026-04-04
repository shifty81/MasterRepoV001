#include "Editor/Panels/EditorToolbar.h"
#include "Editor/Panels/EditorTheme.h"
#include "Core/Logging/Log.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX    // prevent min/max macros from conflicting with std::min/std::max
#define NODRAWTEXT  // prevent DrawText being macro-expanded to DrawTextA/DrawTextW
#include <windows.h>
#endif

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Colour aliases — resolved from the active EditorTheme at draw time.
// These macros keep the rest of this file readable while eliminating
// hard-coded colour constants.
// ---------------------------------------------------------------------------
#define kToolbarBg     ActiveTheme().toolbarBg
#define kMenuBarBg     ActiveTheme().menuBarBg
#define kMenuOpenBg    ActiveTheme().menuOpenBg
#define kDropdownBg    ActiveTheme().dropdownBg
#define kBtnBg         ActiveTheme().buttonBg
#define kBtnBgHover    ActiveTheme().buttonHover
#define kBtnBgActive   ActiveTheme().buttonActive
#define kBtnBgPlay     ActiveTheme().buttonPlay
#define kBtnBgStop     ActiveTheme().buttonStop
#define kBtnBgLaunch   ActiveTheme().buttonLaunch
#define kTextColor     ActiveTheme().textHeader
#define kTitleColor    ActiveTheme().textTitle
#define kSepColor      ActiveTheme().separator
#define kDisabledColor ActiveTheme().textDisabled

// ---------------------------------------------------------------------------
// Static menu definitions
// ---------------------------------------------------------------------------

struct MenuItemDef {
    const char* label;      ///< nullptr means a separator line.
    const char* commandId;  ///< Command to execute; nullptr = no-op / placeholder.
};

struct MenuDef {
    const char*        name;
    const MenuItemDef* items;
    int                count;
};

static const MenuItemDef kFileItems[] = {
    { "New World",    "File.NewWorld"        },
    { "Save World",   "World.SaveDevWorld"   },
    { "Reload World", "World.ReloadDevWorld" },
    { nullptr,        nullptr                },  // separator
    { "Exit",         "File.Exit"            },
};

static const MenuItemDef kEditItems[] = {
    { "Undo", "Edit.Undo" },
    { "Redo", "Edit.Redo" },
};

static const MenuItemDef kViewItems[] = {
    { "Voxel Overlay (see VoxelInspector panel)", nullptr },
};

static const MenuDef kMenus[] = {
    { "File", kFileItems, 5 },
    { "Edit", kEditItems, 2 },
    { "View", kViewItems, 1 },
};

static constexpr int   kMenuCount = 3;
static constexpr float kMenuBtnW  = 52.f;  // logical width of each menu header button

// ---------------------------------------------------------------------------
// DrawButton
// ---------------------------------------------------------------------------

bool EditorToolbar::DrawButton(float x, float y, float bw, float bh,
                                const char* label,
                                uint32_t bgColor, uint32_t textColor,
                                bool enabled)
{
    if (!m_Renderer) return false;

    bool hovered = false;
    bool clicked = false;

    if (m_Input && enabled) {
        const float mx = m_Input->mouseX;
        const float my = m_Input->mouseY;
        hovered = (mx >= x && mx < x + bw && my >= y && my < y + bh);
        clicked = hovered && m_Input->leftJustPressed;
    }

    const float dpi = m_Renderer->GetDpiScale();
    const uint32_t fill = hovered ? kBtnBgHover : bgColor;
    m_Renderer->DrawRect({x, y, bw, bh}, fill);
    m_Renderer->DrawOutlineRect({x, y, bw, bh}, kSepColor);

    if (label) {
        const uint32_t tc = enabled ? textColor : kDisabledColor;
        m_Renderer->DrawText(label, x + 6.f * dpi, y + 4.f * dpi, tc, 2.f);
    }

    return clicked;
}

// ---------------------------------------------------------------------------
// Draw — menu bar row + tool button row
// ---------------------------------------------------------------------------

void EditorToolbar::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    m_DrawX = x;
    m_DrawY = y;

    const float dpi   = m_Renderer->GetDpiScale();
    const float menuH = kMenuHeight * dpi;
    const float btnY  = y + menuH;       // top of the button row
    const float btnH  = h - menuH;       // height remaining for buttons

    // ---- Menu bar background ----
    m_Renderer->DrawRect({x, y, w, menuH}, kMenuBarBg);
    m_Renderer->DrawRect({x, y + menuH - 1.f, w, 1.f}, kSepColor);

    // ---- Title (left of File/Edit/View) ----
    m_Renderer->DrawText("NovaForge", x + 8.f * dpi, y + 4.f * dpi, kTitleColor, 2.f);

    // ---- Menu header buttons: File | Edit | View ----
    float hx = x + 100.f * dpi;
    const float mBtnW = kMenuBtnW * dpi;

    for (int i = 0; i < kMenuCount; ++i) {
        const bool isOpen = (m_OpenMenuIdx == i);
        const bool hovered = m_Input &&
            m_Input->mouseX >= hx   && m_Input->mouseX < hx + mBtnW &&
            m_Input->mouseY >= y    && m_Input->mouseY < y  + menuH;

        const uint32_t bg = isOpen ? kMenuOpenBg : (hovered ? kBtnBgHover : kMenuBarBg);
        m_Renderer->DrawRect({hx, y, mBtnW, menuH}, bg);
        m_Renderer->DrawText(kMenus[i].name, hx + 6.f * dpi, y + 4.f * dpi, kTextColor, 2.f);

        if (hovered && m_Input->leftJustPressed) {
            m_OpenMenuIdx = isOpen ? -1 : i;
        }
        hx += mBtnW;
    }

    // ---- Button row background ----
    m_Renderer->DrawRect({x, btnY, w, btnH}, kToolbarBg);
    m_Renderer->DrawRect({x, btnY + btnH - 1.f, w, 1.f}, kSepColor);

    const float bRowH = btnH - 6.f * dpi;
    const float bRowY = btnY + 3.f * dpi;
    const float btnW  = 80.f * dpi;
    const float gap   = 4.f * dpi;
    float bx = x + 8.f * dpi;

    // ---- Tool mode buttons ----
    if (m_ToolContext) {
        auto toolBtn = [&](const char* label, nf::EditorToolMode mode) {
            const bool isActive = (m_ToolContext->activeMode == mode);
            const uint32_t bg = isActive ? kBtnBgActive : kBtnBg;
            if (DrawButton(bx, bRowY, btnW, bRowH, label, bg, kTextColor)) {
                m_ToolContext->activeMode = mode;
                Logger::Log(LogLevel::Info, "EditorToolbar",
                            std::string("Tool mode: ") + label);
            }
            bx += btnW + gap;
        };

        toolBtn("Select",  nf::EditorToolMode::Select);
        toolBtn("Inspect", nf::EditorToolMode::VoxelInspect);
        toolBtn("+ Voxel", nf::EditorToolMode::VoxelAdd);
        toolBtn("- Voxel", nf::EditorToolMode::VoxelRemove);

        m_Renderer->DrawRect({bx, bRowY, 1.f, bRowH}, kSepColor);
        bx += gap * 2.f;
    }

    // ---- Undo / Redo (enabled only when history allows) ----
    {
        const bool canUndo = m_CommandRegistry && m_CommandRegistry->CanExecute("Edit.Undo");
        const bool canRedo = m_CommandRegistry && m_CommandRegistry->CanExecute("Edit.Redo");
        const float udW = 56.f * dpi;

        if (DrawButton(bx, bRowY, udW, bRowH, "Undo", kBtnBg, kTextColor, canUndo)) {
            if (m_CommandRegistry) m_CommandRegistry->Execute("Edit.Undo");
        }
        bx += udW + gap;

        if (DrawButton(bx, bRowY, udW, bRowH, "Redo", kBtnBg, kTextColor, canRedo)) {
            if (m_CommandRegistry) m_CommandRegistry->Execute("Edit.Redo");
        }
        bx += udW + gap;

        m_Renderer->DrawRect({bx, bRowY, 1.f, bRowH}, kSepColor);
        bx += gap * 2.f;
    }

    // ---- Play ----
    {
        const uint32_t playBg = m_PieActive ? kBtnBgActive : kBtnBgPlay;
        if (DrawButton(bx, bRowY, btnW, bRowH, "> Play", playBg, kTextColor)) {
            if (!m_PieActive) {
                m_PieActive = true;
                if (m_Loop) m_Loop->Reset();
                Logger::Log(LogLevel::Info, "EditorToolbar", "PIE started");
            }
        }
    }
    bx += btnW + gap;

    // ---- Stop ----
    if (DrawButton(bx, bRowY, btnW, bRowH, "[] Stop", kBtnBgStop, kTextColor)) {
        if (m_PieActive) {
            m_PieActive = false;
            if (m_Loop) m_Loop->Reset();
            Logger::Log(LogLevel::Info, "EditorToolbar", "PIE stopped");
        }
    }
    bx += btnW + gap;

#ifdef _WIN32
    // ---- Launch standalone game ----
    const float launchW = 110.f * dpi;
    if (DrawButton(bx, bRowY, launchW, bRowH, "Launch Game", kBtnBgLaunch, kTextColor)) {
        Logger::Log(LogLevel::Info, "EditorToolbar", "Launching NovaForgeGame...");

        wchar_t selfPath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        wchar_t* lastSlash = wcsrchr(selfPath, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wchar_t gamePath[MAX_PATH]{};
            if (swprintf_s(gamePath, MAX_PATH, L"%s%s", selfPath, L"NovaForgeGame.exe") > 0) {
                STARTUPINFOW si{};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi{};
                const BOOL launched = CreateProcessW(gamePath, nullptr, nullptr, nullptr,
                                                     FALSE, 0, nullptr, nullptr, &si, &pi);
                const DWORD lastErr = launched ? 0 : GetLastError();
                if (launched) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                } else {
                    Logger::Log(LogLevel::Warning, "EditorToolbar",
                                "CreateProcess failed for NovaForgeGame.exe (error "
                                + std::to_string(lastErr) + ")");
                }
            }
        }
    }
#endif
}

// ---------------------------------------------------------------------------
// DrawDropdown — renders the open drop-down on top of all docking panels
// ---------------------------------------------------------------------------

void EditorToolbar::DrawDropdown()
{
    if (!m_Renderer || m_OpenMenuIdx < 0) return;

    const float dpi   = m_Renderer->GetDpiScale();
    const float menuH = kMenuHeight * dpi;

    // X position of the open header button
    const float hx  = m_DrawX + 100.f * dpi + static_cast<float>(m_OpenMenuIdx) * (kMenuBtnW * dpi);
    const float ddX = hx;
    const float ddY = m_DrawY + menuH;
    const float ddW = 200.f * dpi;

    const float itemH = 20.f * dpi;
    const float sepH  =  6.f * dpi;

    // Compute total drop-down height
    const MenuDef& menu = kMenus[m_OpenMenuIdx];
    float ddH = 6.f * dpi;  // top + bottom padding
    for (int i = 0; i < menu.count; ++i)
        ddH += menu.items[i].label ? itemH : sepH;

    // Background and border
    m_Renderer->DrawRect({ddX, ddY, ddW, ddH}, kDropdownBg);
    m_Renderer->DrawOutlineRect({ddX, ddY, ddW, ddH}, kSepColor);

    // Items
    float iy = ddY + 3.f * dpi;
    bool clickedItem = false;

    for (int i = 0; i < menu.count; ++i) {
        const MenuItemDef& item = menu.items[i];

        if (!item.label) {
            // Separator line
            m_Renderer->DrawRect({ddX + 6.f * dpi, iy + 2.f * dpi,
                                   ddW - 12.f * dpi, 1.f}, kSepColor);
            iy += sepH;
            continue;
        }

        // An item with commandId=nullptr is a visual placeholder (not executable)
        const bool canExec = item.commandId &&
                             (!m_CommandRegistry ||
                              m_CommandRegistry->CanExecute(item.commandId));

        const bool itemHov = m_Input &&
            m_Input->mouseX >= ddX && m_Input->mouseX < ddX + ddW &&
            m_Input->mouseY >= iy  && m_Input->mouseY < iy  + itemH;

        if (itemHov && canExec)
            m_Renderer->DrawRect({ddX, iy, ddW, itemH}, kBtnBgHover);

        m_Renderer->DrawText(item.label,
                              ddX + 14.f * dpi, iy + 2.f * dpi,
                              canExec ? kTextColor : kDisabledColor, 2.f);

        if (itemHov && canExec && m_Input && m_Input->leftJustPressed) {
            m_CommandRegistry->Execute(item.commandId);
            m_OpenMenuIdx = -1;
            clickedItem   = true;
        }

        iy += itemH;
    }

    // Close menu on click outside (but not on the menu bar row itself, which
    // already handles the toggle in Draw())
    if (!clickedItem && m_Input && m_Input->leftJustPressed) {
        const bool insideDrop = m_Input->mouseX >= ddX && m_Input->mouseX < ddX + ddW &&
                                m_Input->mouseY >= ddY && m_Input->mouseY < ddY + ddH;
        const bool onMenuBar  = m_Input->mouseY >= m_DrawY &&
                                m_Input->mouseY <  m_DrawY + menuH;
        if (!insideDrop && !onMenuBar)
            m_OpenMenuIdx = -1;
    }
}

} // namespace NF::Editor
