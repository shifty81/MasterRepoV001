#pragma once

#include <cstdint>
#include <string>

namespace nf
{
    enum class SelectionKind
    {
        None = 0,
        Asset,
        WorldObject,
        Chunk,
        Voxel
    };

    struct SelectionHandle
    {
        SelectionKind kind = SelectionKind::None;
        std::uint64_t id = 0;
        std::string label;
    };

    // ---- Voxel coordinate packing utilities ----
    // Voxel world coordinates are packed into a 64-bit id using three
    // 20-bit signed fields: bits [0..19] = X, [20..39] = Y, [40..59] = Z.
    // This allows coordinates in the range [-524288, 524287].

    /// @brief Number of bits per packed voxel axis.
    static constexpr int kVoxelCoordBits = 20;
    /// @brief Bitmask for one packed axis (0xFFFFF = 20 set bits).
    static constexpr std::uint64_t kVoxelCoordMask = (1ULL << kVoxelCoordBits) - 1;
    /// @brief Sign bit within a packed 20-bit axis (bit 19).
    static constexpr std::uint64_t kVoxelCoordSignBit = 1ULL << (kVoxelCoordBits - 1);

    /// @brief Pack three signed world-space voxel coordinates into a uint64_t.
    inline std::uint64_t PackVoxelCoord(std::int32_t x, std::int32_t y, std::int32_t z) noexcept
    {
        return (static_cast<std::uint64_t>(x) & kVoxelCoordMask)
             | ((static_cast<std::uint64_t>(y) & kVoxelCoordMask) << kVoxelCoordBits)
             | ((static_cast<std::uint64_t>(z) & kVoxelCoordMask) << (kVoxelCoordBits * 2));
    }

    /// @brief Unpack one signed coordinate from a packed uint64_t.
    /// @param packed The packed 64-bit value.
    /// @param shift  Bit offset for the axis (0, 20, or 40).
    inline std::int32_t UnpackVoxelCoord(std::uint64_t packed, int shift) noexcept
    {
        auto val = static_cast<std::int32_t>((packed >> shift) & kVoxelCoordMask);
        if (val & static_cast<std::int32_t>(kVoxelCoordSignBit))
            val |= ~static_cast<std::int32_t>(kVoxelCoordMask); // sign-extend
        return val;
    }

    class SelectionService
    {
    public:
        void Clear();
        void Select(const SelectionHandle& handle);

        bool HasSelection() const noexcept;
        const SelectionHandle& GetSelection() const noexcept;
        SelectionKind GetSelectionKind() const noexcept;
        std::string GetSelectionLabel() const;

        bool IsSelectionChanged() const noexcept;
        void ClearSelectionChanged() noexcept;

    private:
        SelectionHandle m_current;
        bool m_selectionChanged = false;
    };
}
