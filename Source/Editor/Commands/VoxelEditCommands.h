#pragma once
#include "Editor/Commands/EditorCommand.h"
#include "Game/Voxel/VoxelEditApi.h"
#include "Game/Voxel/VoxelType.h"
#include <cstdint>

namespace NF::Editor {

/// @brief Undoable command that places a voxel at a world position.
///
/// Execute() sets the voxel; Undo() restores the previous voxel type.
class VoxelPlaceCommand : public EditorCommand {
public:
    /// @param api       Reference to the voxel edit API.
    /// @param wx,wy,wz  World-space coordinates of the target voxel.
    /// @param newType   Voxel type to place.
    VoxelPlaceCommand(NF::Game::VoxelEditApi& api,
                      int32_t wx, int32_t wy, int32_t wz,
                      NF::Game::VoxelId newType)
        : m_Api(api), m_X(wx), m_Y(wy), m_Z(wz), m_NewType(newType)
    {
        // Capture the previous voxel so we can restore it on Undo().
        m_OldType = m_Api.GetVoxel(m_X, m_Y, m_Z);
    }

    void Execute() override {
        m_Api.SetVoxel(m_X, m_Y, m_Z, m_NewType);
    }

    void Undo() override {
        m_Api.SetVoxel(m_X, m_Y, m_Z, m_OldType);
    }

private:
    NF::Game::VoxelEditApi& m_Api;
    int32_t                 m_X, m_Y, m_Z;
    NF::Game::VoxelId       m_NewType;
    NF::Game::VoxelId       m_OldType;
};

/// @brief Undoable command that removes (mines) a voxel at a world position.
///
/// Execute() mines the voxel to Air; Undo() restores the original type.
class VoxelRemoveCommand : public EditorCommand {
public:
    /// @param api       Reference to the voxel edit API.
    /// @param wx,wy,wz  World-space coordinates of the target voxel.
    VoxelRemoveCommand(NF::Game::VoxelEditApi& api,
                       int32_t wx, int32_t wy, int32_t wz)
        : m_Api(api), m_X(wx), m_Y(wy), m_Z(wz)
    {
        m_OldType = m_Api.GetVoxel(m_X, m_Y, m_Z);
    }

    void Execute() override {
        // durability=0 → instant removal (see VoxelEditApi::Mine).
        m_Api.Mine(m_X, m_Y, m_Z, 0);
    }

    void Undo() override {
        m_Api.SetVoxel(m_X, m_Y, m_Z, m_OldType);
    }

private:
    NF::Game::VoxelEditApi& m_Api;
    int32_t                 m_X, m_Y, m_Z;
    NF::Game::VoxelId       m_OldType;
};

/// @brief Undoable command that changes a voxel's type via inspector editing.
///
/// Execute() sets the new type; Undo() restores the old type.
class VoxelTypeEditCommand : public EditorCommand {
public:
    /// @param api       Reference to the voxel edit API.
    /// @param wx,wy,wz  World-space coordinates of the target voxel.
    /// @param oldType   The previous voxel type (for undo).
    /// @param newType   The new voxel type to apply.
    VoxelTypeEditCommand(NF::Game::VoxelEditApi& api,
                         int32_t wx, int32_t wy, int32_t wz,
                         NF::Game::VoxelId oldType,
                         NF::Game::VoxelId newType)
        : m_Api(api), m_X(wx), m_Y(wy), m_Z(wz)
        , m_OldType(oldType), m_NewType(newType)
    {}

    void Execute() override {
        m_Api.SetVoxel(m_X, m_Y, m_Z, m_NewType);
    }

    void Undo() override {
        m_Api.SetVoxel(m_X, m_Y, m_Z, m_OldType);
    }

private:
    NF::Game::VoxelEditApi& m_Api;
    int32_t                 m_X, m_Y, m_Z;
    NF::Game::VoxelId       m_OldType;
    NF::Game::VoxelId       m_NewType;
};

} // namespace NF::Editor
