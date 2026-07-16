# Geometry Stamp for Unreal Engine

Geometry Stamp is an editor-only Unreal Engine tool for stamping real,
heightmap-displaced geometry onto landscapes and mesh surfaces. The preview is
generated as a Dynamic Mesh and is intended to be baked to a Nanite Static Mesh
before shipping a project.

The project is an independent clean-room implementation based on the general
workflow of terrain geometry stamping. It does not contain source code or assets
from third-party marketplace products.

## Current milestone: 0.3.0 stabilization

- Placeable `GeometryStampActor`.
- Circle and rectangle footprints.
- Adjustable grid resolution and physical size.
- Projection along world down or the actor's local down axis.
- Landscape and Static Mesh support through collision traces.
- G8 and BGRA8 heightmap displacement.
- Height channel, center, magnitude and additive offset controls.
- Smooth geometric edge falloff.
- Automatic editor preview rebuild.
- Lightweight, throttled preview while moving the actor.
- Full-resolution rebuild after the actor is released.
- Optional preview material and world-sized UVs.
- Dedicated **Place Actors → Geometry Stamp** drag-and-drop item.

Stamp presets are the next milestone. Static Mesh/Nanite baking, batch operations
and PCG integration are planned for later milestones.

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

`Displacement Strength` is intentionally independent from actor height. Moving
the actor changes the stamp location and projection direction without changing
the sculpted result. During movement, `Move Preview Resolution` is used for a
responsive preview; releasing the actor restores the full `Grid Resolution`.

High resolutions perform one collision trace per vertex. Use low resolution for
iteration and reserve high resolution for the future bake workflow.

## Status

The editor module builds against Unreal Engine 5.6.1 on Windows. Editor startup,
Place Actors registration, native actor creation and Dynamic Mesh projection onto
a collision-enabled Static Mesh have been smoke-tested.
