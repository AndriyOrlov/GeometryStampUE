# Geometry Stamp for Unreal Engine

Geometry Stamp is an editor-only Unreal Engine tool for stamping real,
heightmap-displaced geometry onto landscapes and mesh surfaces. The preview is
generated as a Dynamic Mesh and is intended to be baked to a Nanite Static Mesh
before shipping a project.

The project is an independent clean-room implementation based on the general
workflow of terrain geometry stamping. It does not contain source code or assets
from third-party marketplace products.

## Current milestone: 0.5.2 bakeable MVP

- Placeable `GeometryStampActor`.
- Circle and square footprints; the circle uses a full rounded-square quad grid.
- Adjustable grid resolution and physical size.
- Projection along world down or the actor's local down axis.
- Configurable surface-angle clipping (85° by default) prevents projection onto walls.
- Landscape and Static Mesh support through collision traces.
- G8 and BGRA8 heightmap displacement.
- Height channel, center, magnitude and additive offset controls.
- Smooth geometric edge falloff.
- Adjustable edge inset for a clean transition into the projected surface.
- Automatic editor preview rebuild.
- Lightweight, throttled preview while moving the actor.
- Full-resolution rebuild after the actor is released.
- Optional preview material and world-sized UVs.
- Dedicated **Place Actors → Geometry Stamp** drag-and-drop item.
- Reusable `GeometryStampPreset` Data Assets with Apply, Save As and Reset actions.
- Draft (16), Preview (64), High (128) and Bake (256) geometry quality presets.
- Material texture auto-detection with Displacement → Height → generated luminance fallback.
- Plugin-owned example stamps, editable materials and heightmaps.
- Custom Geometry Stamp icon for Place Actors and Blueprint-derived classes.
- **Bake to Static Mesh** window with destination folder and asset name.
- Optional Nanite enablement and replacement with a Static Mesh Actor.
- Bake assets are always created as new assets and never silently overwritten.
- Bake quality can be selected independently: Draft 16, Preview 64, High 128 or Bake 256.
- Crater, Rock Shelf and Ground Patch presets are included in Plugin Content.
- Inline **Height Profile** curve remaps sampled height from normalized input `0..1` to output `0..1` before displacement.

Mesh reduction, generated LODs, batch operations and PCG integration are planned
for later milestones.

## Installation

1. Copy this repository to `<YourProject>/Plugins/GeometryStamp`.
2. Open the Unreal project.
3. Enable **Geometry Stamp** when prompted and rebuild the C++ modules.
4. Ensure the target Landscape or Static Mesh has Visibility collision enabled.
5. Open **Place Actors → Geometry Stamp** and drag **Geometry Stamp** into the level.

The placed native actor is Blueprint-compatible. You can use it directly or
create a Blueprint child when project-specific defaults and presets are needed.

Target: Unreal Engine 5.6.1 on Windows.

## Basic use

1. Place the actor above the target surface.
2. Set `Size` and a moderate `Grid Resolution`, such as 32 or 64.
3. Assign a source height texture. G8 or BGRA8 source formats are currently supported.
4. Adjust `Displacement Strength`, `Height Center`, `Edge Falloff` and `Mask Hardness`.
5. Use `Rebuild Stamp` if automatic rebuild is disabled.

Assigning `Material` automatically looks for a texture parameter or texture asset
named like Displacement, Height, Bump or Parallax. If none exists, the best
available material texture is sampled as luminance height data. The source texture
asset is never modified. Use `Auto from Material` to run the same detection again.

Use `Save Current as Preset` to create a new preset asset. It always opens a Save
As dialog and never overwrites the selected preset. `Apply Preset` copies only
stamp settings; actor transform and scene actor references are not stored.

To bake, select one stamp and click `Bake to Static Mesh...` at the top of its
Details panel. Choose a `/Game` folder, a new asset name and bake quality, then
enable Nanite and actor replacement as needed. The baked asset keeps the projected geometry,
material and UVs. Save the new asset when prompted by Unreal Editor. Undo restores
the preview actor when replacement was enabled; it does not delete the new asset.

## Ready-to-drag examples

Open **Place Actors → Geometry Stamp** and drag one of these onto a Landscape or
collision-enabled Static Mesh:

- **Example — Crater** — circular impact with a raised rim and recessed center.
- **Example — Rock Shelf** — rectangular layered rock breakup.
- **Example — Ground Patch** — broad, soft organic ground variation.

Their source assets are under **Plugin Content → Geometry Stamp → Examples**.
`M_GS_Example` is an editable master material; its instances and clean-room G8
heightmaps are included with the plugin.

`Displacement Strength` is intentionally independent from actor height. Moving
the actor changes the stamp location and projection direction without changing
the sculpted result. During movement, `Move Preview Resolution` is used for a
responsive preview; releasing the actor restores the full `Grid Resolution`.

High resolutions perform one collision trace per vertex. Use low resolution for
iteration and reserve high resolution for the future bake workflow.

## Status

The editor module builds against Unreal Engine 5.6.1 on Windows. Editor startup,
Place Actors registration, preset Save/Apply, quality switching, Undo/Redo and
Dynamic Mesh projection onto collision-enabled geometry have been smoke-tested.
