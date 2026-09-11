"""Builds the map board's mesh: a flat panel in a thin frame.

WHY THIS EXISTS AT ALL. The first board borrowed the laptop lid, which was a
shortcut to prove the render target worked and was never the thing itself --
a map board that is visibly a laptop lid is a map board nobody believes. This
makes the real one, and it is thirty lines because the shape is a rectangle.

TWO MATERIAL SLOTS, NAMED, and the names are the contract. The .xob.meta
assigns materials by SOURCE MATERIAL NAME, so these two strings are what tie
the geometry to MapBoard_Surface.emat (which samples $rendertarget) and
MapBoard_Frame.emat. Rename one here and the board loses its map.

ONE METRE WIDE AND 10:7, which is roughly a wall map's proportion and, more to
the point, a number a Game Master can reason about when scaling: at scale 3
the board is three metres across. The render texture is square, so the map is
drawn square and the panel shows it at the board's own aspect -- the component
sizes the widget to match, rather than the map being stretched to fit.

Run it with the Blender the other MCF models were made in:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --python tools/make_map_board.py
"""

import os
import sys

import bpy

OUT = r"G:\MCF\addons\MCF_Ops\Assets\Props\Intel\MapBoard\MapBoard.fbx"

WIDTH = 1.0          # metres across
HEIGHT = 0.7         # metres tall
FRAME = 0.025        # how far the frame stands proud of the map, each side
DEPTH = 0.03         # how thick the board is

SURFACE_MATERIAL = "MapBoard_Surface"
FRAME_MATERIAL = "MapBoard_Frame"


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(name):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    return mat


def build():
    half_w = WIDTH * 0.5
    half_h = HEIGHT * 0.5

    # ---- the frame: a slab, the full size of the board
    bpy.ops.mesh.primitive_cube_add(size=1)
    frame = bpy.context.active_object
    frame.name = "MapBoard"
    frame.scale = (WIDTH, DEPTH, HEIGHT)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # Standing on the floor rather than sunk halfway into it, so a Game
    # Master dropping one gets a board resting where they put it.
    frame.location = (0, 0, half_h)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    # ---- the face: a plane just proud of the front, inset by the frame width
    bpy.ops.mesh.primitive_plane_add(size=1)
    face = bpy.context.active_object
    face.name = "MapBoardFace"
    face.rotation_euler = (1.5707963, 0, 0)      # stand it up, facing -Y
    face.scale = (WIDTH - FRAME * 2, HEIGHT - FRAME * 2, 1)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    face.location = (0, -(DEPTH * 0.5) - 0.001, half_h)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    # ---- materials, one per object, so the slots survive the join in order
    frame.data.materials.append(material(FRAME_MATERIAL))
    face.data.materials.append(material(SURFACE_MATERIAL))

    # ---- one object, two slots
    bpy.ops.object.select_all(action="DESELECT")
    face.select_set(True)
    frame.select_set(True)
    bpy.context.view_layer.objects.active = frame
    bpy.ops.object.join()

    board = bpy.context.active_object
    board.name = "MapBoard"

    # ---- the collision box, named the way the importer expects
    #
    # UBX_ IS NOT DECORATION. Enfusion reads the prefix: UBX is a box
    # collider, and the GeometryParam in the .xob.meta is keyed on this exact
    # name. Without it the board has no physics and cannot be stood in front
    # of, leaned on, or shot.
    bpy.ops.mesh.primitive_cube_add(size=1)
    collider = bpy.context.active_object
    collider.name = "UBX_MapBoard"
    collider.scale = (WIDTH, DEPTH, HEIGHT)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    collider.location = (0, 0, half_h)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    return board, collider


def export():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)

    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.fbx(
        filepath=OUT,
        use_selection=True,
        apply_unit_scale=True,
        global_scale=1.0,
        # THE AXES ARE NOT OPTIONAL. Enfusion is Y-up, Blender is Z-up, and
        # HANDOVER records what happens when the exporter is left to guess:
        # the change is not baked into the vertices and the model arrives on
        # its side.
        axis_forward="-Z",
        axis_up="Y",
        bake_space_transform=True,
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        path_mode="COPY",
    )


clear()
board, collider = build()
export()

print("MAP BOARD WRITTEN: %s" % OUT)
print("  panel %.2f x %.2f m, frame %.3f m, depth %.3f m" % (WIDTH, HEIGHT, FRAME, DEPTH))
print("  material slots: %s" % ", ".join(slot.name for slot in board.data.materials))
