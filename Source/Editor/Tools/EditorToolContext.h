#pragma once

namespace nf
{
    enum class EditorToolMode
    {
        Select = 0,
        VoxelInspect,
        VoxelAdd,
        VoxelRemove
    };

    struct EditorToolContext
    {
        EditorToolMode activeMode = EditorToolMode::Select;
        bool cameraNavigationActive = false;
        bool worldDirty = false;
        int selectedVoxelType = 1;
        bool showChunkBorders = false; ///< Draw 6DOF chunk-border wireframes in the viewport.
    };
}
