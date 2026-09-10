"""Split the Sketchfab laptop into a body and a hinged lid.

Run once per part, in its own Blender process:

    blender -b --factory-startup --python split2.py -- body
    blender -b --factory-startup --python split2.py -- lid

One part per process on purpose. Doing both in one session meant the second
export inherited the first one's leftovers -- an orphaned mesh datablock still
holding the name LOD0, a material list containing five materials the lid does
not use, an object count the Definitions block had already been written for.
The first of those was findable (the geometry came out as LOD0.001); the rest
produced an FBX that Enfusion accepted, parsed as empty, and refused to build
with one line of log and no reason. A fresh process cannot have any of them.

Enfusion needs, per part: one visual mesh called LOD0, one collider called
UBX_<something>, and -- for the body -- a null called socket_LaptopLid whose
own local Y is the hinge axis, because DoorComponent turns an entity about its
local Y and the socket is what decides where that points.
"""

import bpy, math, os, sys
from mathutils import Matrix, Vector

PART  = sys.argv[-1]
assert PART in ('body', 'lid', 'open'), 'pass body, lid or open after --'

SRC   = r'C:\Users\Administrator\AppData\Local\Temp\mcf_laptop\src\laptop_leather.fbx'
OUT   = r'G:\MCF\addons\MCF_Ops\Assets\Props\Intel\Laptop'
SCALE = 10.0          # source is 36mm across; a 15" laptop is 360mm
CLOSED_FRAME = 1      # measured: hinge rotY -187 puts the lid flat on the base
OPEN_FRAME   = 121    # measured: hinge rotY -70, the end of the source animation

RENAME = {
    'blinn1':   'Laptop_Screen',    # the 640x480 screen image, lid only
    'phong1':   'Laptop_Leather',   # the 2560 tiled leather, the outer shell
    'lambert1': 'Laptop_Plastic',
    'phong2':   'Laptop_Keys',
    'phong3':   'Laptop_Trim',
    'phong4':   'Laptop_Rubber',
}

def P(*a): print('[B]', *a)

os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)

# The open preview is a still picture, so it needs no hinge and no composed
# rotation -- posing the source animation at its open frame and baking the
# whole thing into one mesh sidesteps the euler convention entirely.
bpy.context.scene.frame_set(OPEN_FRAME if PART == 'open' else CLOSED_FRAME)
bpy.context.view_layer.update()

hinge = bpy.data.objects['pCylinder3']
hinge_loc = hinge.matrix_world.translation.copy()

def subtree(o, acc):
    acc.append(o)
    for c in o.children:
        subtree(c, acc)
    return acc

lid_names = set(o.name for o in subtree(hinge, []))

for o in list(bpy.data.objects):
    if o.type == 'LIGHT' or o.name == 'aiSkyDomeLight1':
        bpy.data.objects.remove(o, do_unlink=True)

for old, new in RENAME.items():
    m = bpy.data.materials.get(old)
    if m:
        m.name = new

# Cut the parenting BEFORE baking. Setting matrix_world on a child only writes
# its local matrix against whatever the parent holds at that moment; flatten
# the parent afterwards and the child silently moves. That is how the lid once
# left here carrying a scale of 66000.
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
keep = {o: o.matrix_world.copy() for o in meshes}
for o in meshes:
    o.parent = None
for o in meshes:
    o.matrix_world = keep[o]

for o in meshes:
    o.data = o.data.copy()
    o.data.transform(o.matrix_world)
    o.matrix_world = Matrix.Identity(4)
    o.animation_data_clear()

lid_objs  = [o for o in meshes if o.name in lid_names]
body_objs = [o for o in meshes if o.name not in lid_names]

def bounds(objects):
    # From vertices, not bound_box: bound_box is cached and still holds the
    # pre-bake extents here, which silently offset the model by 12 units.
    lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
    for o in objects:
        for v in o.data.vertices:
            for i in range(3):
                lo[i] = min(lo[i], v.co[i]); hi[i] = max(hi[i], v.co[i])
    return lo, hi

lo, hi = bounds(body_objs)
centre = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z)) * SCALE
M = Matrix.Translation(-centre) @ Matrix.Scale(SCALE, 4)
for o in meshes:
    o.data.transform(M)

hinge_final = M @ hinge_loc
# The hinge frame. Blender's FBX exporter does NOT bake the Z-up to Y-up
# change into the vertex data -- it leaves the vertices in Blender axes and
# puts Lcl Rotation -90 0 0 on the node. So the engine applies
# (x,y,z) -> (x, z, -y) at import, and a local axis is NOT the same axis on
# both sides: Blender local Z is what arrives as engine Y.
#
# DoorComponent turns an entity about its local Y -- a plain door's hinge is
# vertical and its entity is unrotated, which is the proof. So the hinge axis
# has to be Blender local Z here, and the measured hinge runs along world X.
H = Matrix.Translation(hinge_final) @ Matrix.Rotation(math.radians(90), 4, 'Y')
P('hinge at', tuple(round(v, 4) for v in hinge_final))

# Throw away the half we are not exporting, so this process only ever holds
# one part's meshes and materials.
if PART == 'open':
    drop = []
    wanted = meshes
else:
    drop = lid_objs if PART == 'body' else body_objs
    wanted = body_objs if PART == 'body' else lid_objs
for o in drop:
    bpy.data.objects.remove(o, do_unlink=True)
for m in list(bpy.data.meshes):
    if m.users == 0:
        bpy.data.meshes.remove(m)
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)

if PART == 'lid':
    for o in wanted:
        o.data.transform(H.inverted())

if PART in ('lid', 'open'):
    # Two meshes must never share a material name with an already-imported
    # one: the workbench then parses the second as containing nothing at all,
    # with one line of log and no reason given.
    prefix = 'LaptopLid_' if PART == 'lid' else 'LaptopOpen_'
    for m in list(bpy.data.materials):
        if not m.name.startswith(prefix):
            m.name = prefix + m.name.replace('Laptop_', '')



for o in bpy.data.objects:
    o.select_set(False)
for o in wanted:
    o.select_set(True)
bpy.context.view_layer.objects.active = wanted[0]
if len(wanted) > 1:
    bpy.ops.object.join()
visual = bpy.context.view_layer.objects.active
visual.name = 'LOD0'
visual.data.name = 'LOD0'

def local_bounds(o):
    lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
    for v in o.data.vertices:
        for i in range(3):
            lo[i] = min(lo[i], v.co[i]); hi[i] = max(hi[i], v.co[i])
    return lo, hi

lo, hi = local_bounds(visual)
P(PART, 'bounds', tuple(round(x, 4) for x in lo), tuple(round(x, 4) for x in hi))

box_name = {'body': 'UBX_Laptop_Body', 'lid': 'UBX_Laptop_Lid', 'open': 'UBX_LaptopOpen'}[PART]
mesh = bpy.data.meshes.new(box_name)
verts = [(x, y, z) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]
mesh.from_pydata(verts, [], [(0,1,3,2), (4,6,7,5), (0,4,5,1), (2,3,7,6), (0,2,6,4), (1,5,7,3)])
mesh.update()
box = bpy.data.objects.new(box_name, mesh)
bpy.context.scene.collection.objects.link(box)

export = [visual, box]
if PART == 'body':
    socket = bpy.data.objects.new('socket_LaptopLid', None)
    socket.empty_display_type = 'PLAIN_AXES'
    socket.empty_display_size = 0.05
    socket.matrix_world = H
    bpy.context.scene.collection.objects.link(socket)
    export.append(socket)

for o in bpy.data.objects:
    o.select_set(False)
for o in export:
    o.select_set(True)
bpy.context.view_layer.objects.active = export[0]

path = os.path.join(OUT, {'body': 'Laptop_Body.fbx', 'lid': 'LaptopLid.fbx', 'open': 'LaptopOpen.fbx'}[PART])
bpy.ops.export_scene.fbx(
    filepath=path,
    use_selection=True,
    object_types={'MESH', 'EMPTY'},
    apply_unit_scale=True,
    global_scale=1.0,
    axis_forward='-Z',
    axis_up='Y',
    bake_space_transform=False,
    path_mode='AUTO',
    use_mesh_modifiers=False,
    add_leaf_bones=False,
)
P('wrote', path)
P('materials in file:', sorted(m.name for m in bpy.data.materials))
P('DONE')
