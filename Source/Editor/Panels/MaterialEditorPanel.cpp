#include "Editor/Panels/MaterialEditorPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<MaterialPin> MakeOutputPin(uint32_t& nextId,
                                               const std::string& name,
                                               PinType type)
{
    MaterialPin p;
    p.id       = nextId++;
    p.name     = name;
    p.type     = type;
    p.isOutput = true;
    return {p};
}

static MaterialPin MakeInputPin(uint32_t& nextId,
                                const std::string& name,
                                PinType type)
{
    MaterialPin p;
    p.id       = nextId++;
    p.name     = name;
    p.type     = type;
    p.isOutput = false;
    return p;
}

// ---------------------------------------------------------------------------
// Pin type colour
// ---------------------------------------------------------------------------

static uint32_t PinTypeColor(PinType type)
{
    const auto& t = ActiveTheme();
    switch (type) {
    case PinType::Float:   return t.pinFloat;
    case PinType::Vec3:    return t.pinVec3;
    case PinType::Color:   return t.pinColor;
    case PinType::Texture: return t.pinTexture;
    case PinType::Bool:    return t.pinBool;
    }
    return 0xAAAAAAAA;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MaterialEditorPanel::MaterialEditorPanel() = default;

// ---------------------------------------------------------------------------
// Node management
// ---------------------------------------------------------------------------

uint32_t MaterialEditorPanel::AddNode(const std::string& kind, float x, float y)
{
    MaterialNode node;
    node.id    = AllocId();
    node.kind  = kind;
    node.label = kind;
    node.posX  = x;
    node.posY  = y;
    node.pins  = MakePins(kind);
    m_Nodes.push_back(std::move(node));
    Logger::Log(LogLevel::Debug, "MaterialEditor",
                "Added node: " + kind);
    return m_Nodes.back().id;
}

void MaterialEditorPanel::RemoveNode(uint32_t nodeId)
{
    // Remove links that reference any pin on this node
    auto* n = FindNode(nodeId);
    if (!n) return;

    std::vector<uint32_t> pinIds;
    pinIds.reserve(n->pins.size());
    for (const auto& p : n->pins) pinIds.push_back(p.id);

    m_Links.erase(
        std::remove_if(m_Links.begin(), m_Links.end(),
            [&](const MaterialLink& lk) {
                for (uint32_t pid : pinIds)
                    if (lk.fromPin == pid || lk.toPin == pid)
                        return true;
                return false;
            }),
        m_Links.end());

    m_Nodes.erase(
        std::remove_if(m_Nodes.begin(), m_Nodes.end(),
            [nodeId](const MaterialNode& nd) { return nd.id == nodeId; }),
        m_Nodes.end());
}

// ---------------------------------------------------------------------------
// Link management
// ---------------------------------------------------------------------------

uint32_t MaterialEditorPanel::AddLink(uint32_t fromPinId, uint32_t toPinId)
{
    if (fromPinId == toPinId) return 0;

    MaterialLink lk;
    lk.id      = AllocId();
    lk.fromPin = fromPinId;
    lk.toPin   = toPinId;
    m_Links.push_back(lk);
    return lk.id;
}

void MaterialEditorPanel::RemoveLink(uint32_t linkId)
{
    m_Links.erase(
        std::remove_if(m_Links.begin(), m_Links.end(),
            [linkId](const MaterialLink& lk) { return lk.id == linkId; }),
        m_Links.end());
}

// ---------------------------------------------------------------------------
// Compile
// ---------------------------------------------------------------------------

std::shared_ptr<Material> MaterialEditorPanel::Compile() const
{
    // Locate the Output node — required for a valid graph.
    const MaterialNode* outputNode = nullptr;
    for (const auto& n : m_Nodes)
        if (n.kind == "Output") { outputNode = &n; break; }

    if (!outputNode) {
        Logger::Log(LogLevel::Warning, "MaterialEditor",
                    "Compile: no Output node found");
        return nullptr;
    }

    auto mat = std::make_shared<Material>();
    Logger::Log(LogLevel::Info, "MaterialEditor", "Compiled material graph");
    return mat;
}

// ---------------------------------------------------------------------------
// Draw — render the node graph as boxes with pin labels and link lines
// ---------------------------------------------------------------------------

void MaterialEditorPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Open) return;
    if (!m_Renderer) return;

    const auto& t = ActiveTheme();
    const uint32_t kBgColor      = t.graphBg;
    const uint32_t kGridColor    = t.graphGrid;
    const uint32_t kNodeBg       = t.nodeBg;
    const uint32_t kNodeHeader   = t.nodeHeader;
    const uint32_t kNodeBorder   = t.nodeBorder;
    const uint32_t kTextColor    = t.textHeader;
    const uint32_t kHeaderText   = 0xFFFFFFFF;
    const uint32_t kLinkColor    = t.nodeLink;
    const uint32_t kTitleColor   = t.textHeader;

    const float dpi   = m_Renderer->GetDpiScale();
    const float scale = 1.f;

    // Background
    m_Renderer->DrawRect({x, y, w, h}, kBgColor);

    // Grid lines (vertical + horizontal every 40px)
    const float gridStep = 40.f * dpi;
    for (float gx = x; gx < x + w; gx += gridStep)
        m_Renderer->DrawRect({gx, y, 1.f, h}, kGridColor);
    for (float gy = y; gy < y + h; gy += gridStep)
        m_Renderer->DrawRect({x, gy, w, 1.f}, kGridColor);

    // Title
    m_Renderer->DrawText("Material Editor", x + 6.f * dpi, y + 4.f * dpi, kTitleColor, scale);

    if (m_Nodes.empty()) {
        m_Renderer->DrawText("(empty graph — add nodes via code)",
                             x + 6.f * dpi, y + 24.f * dpi, 0x808080FF, scale);
        return;
    }

    // Node dimensions
    const float nodeW      = 140.f * dpi;
    const float headerH    = 20.f  * dpi;
    const float pinRowH    = 16.f  * dpi;
    const float pinPad     = 4.f   * dpi;
    const float pinDotR    = 4.f   * dpi;

    // Helper: find a pin's screen position (centre of its dot).
    // Returns (px, py) in panel-local coords.
    struct PinPos {
        float px, py;
        bool  found;
    };

    auto findPinPos = [&](uint32_t pinId) -> PinPos {
        for (const auto& node : m_Nodes) {
            int inputIdx  = 0;
            int outputIdx = 0;
            for (const auto& pin : node.pins) {
                if (pin.id == pinId) {
                    const float nx = x + node.posX * dpi;
                    const float ny = y + 24.f * dpi + node.posY * dpi;
                    if (pin.isOutput) {
                        return {nx + nodeW, ny + headerH + pinPad + pinRowH * static_cast<float>(outputIdx) + pinRowH * 0.5f, true};
                    } else {
                        return {nx, ny + headerH + pinPad + pinRowH * static_cast<float>(inputIdx) + pinRowH * 0.5f, true};
                    }
                }
                if (pin.isOutput) ++outputIdx;
                else ++inputIdx;
            }
        }
        return {0.f, 0.f, false};
    };

    // Draw links first (behind nodes)
    for (const auto& lk : m_Links) {
        auto from = findPinPos(lk.fromPin);
        auto to   = findPinPos(lk.toPin);
        if (!from.found || !to.found) continue;

        // Draw a horizontal-then-vertical stepped line
        const float midX = (from.px + to.px) * 0.5f;
        m_Renderer->DrawRect({from.px, from.py, midX - from.px, 1.f}, kLinkColor);
        const float lineTop = std::min(from.py, to.py);
        const float lineBot = std::max(from.py, to.py);
        m_Renderer->DrawRect({midX, lineTop, 1.f, lineBot - lineTop}, kLinkColor);
        m_Renderer->DrawRect({midX, to.py, to.px - midX, 1.f}, kLinkColor);
    }

    // Draw nodes
    for (const auto& node : m_Nodes) {
        const float nx = x + node.posX * dpi;
        const float ny = y + 24.f * dpi + node.posY * dpi;

        // Count pins
        int nInputs = 0, nOutputs = 0;
        for (const auto& p : node.pins)
            p.isOutput ? ++nOutputs : ++nInputs;
        const int maxPins = std::max(nInputs, nOutputs);
        const float nodeH = headerH + pinPad * 2.f + pinRowH * static_cast<float>(std::max(1, maxPins));

        // Node background + border
        m_Renderer->DrawRect({nx, ny, nodeW, nodeH}, kNodeBg);
        m_Renderer->DrawOutlineRect({nx, ny, nodeW, nodeH}, kNodeBorder);

        // Header
        m_Renderer->DrawRect({nx, ny, nodeW, headerH}, kNodeHeader);
        m_Renderer->DrawText(node.label, nx + 4.f * dpi, ny + 2.f * dpi, kHeaderText, scale);

        // Input pins (left side)
        int inputIdx = 0;
        for (const auto& pin : node.pins) {
            if (pin.isOutput) continue;
            const float py = ny + headerH + pinPad + pinRowH * static_cast<float>(inputIdx);
            // Pin dot
            m_Renderer->DrawRect({nx - pinDotR, py + pinRowH * 0.5f - pinDotR,
                                   pinDotR * 2.f, pinDotR * 2.f}, PinTypeColor(pin.type));
            // Pin label
            m_Renderer->DrawText(pin.name, nx + 4.f * dpi, py + 1.f * dpi, kTextColor, 1.f);
            ++inputIdx;
        }

        // Output pins (right side)
        int outputIdx = 0;
        for (const auto& pin : node.pins) {
            if (!pin.isOutput) continue;
            const float py = ny + headerH + pinPad + pinRowH * static_cast<float>(outputIdx);
            // Pin dot
            m_Renderer->DrawRect({nx + nodeW - pinDotR, py + pinRowH * 0.5f - pinDotR,
                                   pinDotR * 2.f, pinDotR * 2.f}, PinTypeColor(pin.type));
            // Pin label (right-justified heuristic)
            m_Renderer->DrawText(pin.name, nx + nodeW - 40.f * dpi, py + 1.f * dpi, kTextColor, 1.f);
            ++outputIdx;
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

MaterialNode* MaterialEditorPanel::FindNode(uint32_t id)
{
    for (auto& n : m_Nodes)
        if (n.id == id) return &n;
    return nullptr;
}

std::vector<MaterialPin> MaterialEditorPanel::MakePins(const std::string& kind)
{
    std::vector<MaterialPin> pins;

    auto addIn  = [&](const std::string& name, PinType type) {
        pins.push_back(MakeInputPin(m_NextId, name, type));
    };
    auto addOut = [&](const std::string& name, PinType type) {
        for (auto& p : MakeOutputPin(m_NextId, name, type))
            pins.push_back(p);
    };

    if (kind == "Output") {
        addIn("BaseColor", PinType::Color);
        addIn("Metallic",  PinType::Float);
        addIn("Roughness", PinType::Float);
        addIn("Normal",    PinType::Vec3);
    } else if (kind == "Texture2D") {
        addIn("UV",     PinType::Vec3);
        addOut("RGBA",  PinType::Color);
        addOut("R",     PinType::Float);
    } else if (kind == "Multiply") {
        addIn("A",      PinType::Float);
        addIn("B",      PinType::Float);
        addOut("Out",   PinType::Float);
    } else if (kind == "Add") {
        addIn("A",      PinType::Float);
        addIn("B",      PinType::Float);
        addOut("Out",   PinType::Float);
    } else if (kind == "Constant") {
        addOut("Value", PinType::Float);
    } else if (kind == "ConstantVec3") {
        addOut("Value", PinType::Vec3);
    } else {
        // Unknown kind: one generic input + one generic output
        addIn("In",    PinType::Float);
        addOut("Out",  PinType::Float);
    }

    return pins;
}

} // namespace NF::Editor
