"""Split the Sketchfab laptop into a body and a lid that Enfusion can hinge.

The source model is animated: one node, pCylinder3, rotates 117 degrees and
carries the lid with it. Enfusion does not want that animation -- DoorComponent
rotates a whole entity bodily -- so what it wants instead is two meshes, the
lid's origin sitting exactly on the hinge, and a socket in the body saying
where the lid goes and which way the axis points.

Everything is baked into mesh data and every exported object is left at
identity, so nothing depends on how a reader interprets node transforms.
"""

import bpy, math, os
from mathutils import Matrix, Vector

SRC   = r'C:\Users\Administrator\AppData\Local\Temp\mcf_laptop\src\laptop_leather.fbx'
OUT   = r'G:\MCF\addons\MCF_Ops\Assets\Props\Intel\Laptop'
SCALE = 10.0          # source is 36mm across; a 15" laptop is 360mm
CLOSED_FRAME = 1      # measured: rotY -187 = lid flat on the body

# Maya's default material names say nothing. These do.
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

# ---------------------------------------------------------------- closed pose
# The lid must be exported shut, because "shut" is the pose DoorComponent
# treats as angle zero. Set the frame, then drop the animation so the pose
# stops being a function of time.
bpy.context.scene.frame_set(CLOSED_FRAME)
bpy.context.view_layer.update()

hinge = bpy.data.objects['pCylinder3']
hinge_loc = hinge.matrix_world.translation.copy()
P('hinge at', tuple(round(v, 5) for v in hinge_loc))

def subtree(o, acc):
    acc.append(o)
    for c in o.children:
        subtree(c, acc)
    return acc

lid_names = set(o.name for o in subtree(hinge, []))

# ------------------------------------------------------------------- cleaning
for o in list(bpy.data.objects):
    if o.type == 'LIGHT' or o.name == 'aiSkyDomeLight1':
        P('dropping', o.type, o.name)
        bpy.data.objects.remove(o, do_unlink=True)

for old, new in RENAME.items():
    m = bpy.data.materials.get(old)
    if m:
        m.name = new

# --------------------------------------------------- bake every pose into data
# Cut the parenting FIRST. Setting matrix_world on a child only writes its
# local matrix, computed against whatever the parent held at that moment; when
# the parent is then flattened too, the child silently ends up somewhere else.
# That is how the lid left here once carrying a scale of 66000.
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
keep = {o: o.matrix_world.copy() for o in meshes}
for o in meshes:
    o.parent = None
for o in meshes:
    o.matrix_world = keep[o]

# Single-user the mesh data: two objects sharing one mesh would otherwise get
# the first one's world matrix applied twice.
for o in meshes:
    o.data = o.data.copy()
    o.data.transform(o.matrix_world)
    o.matrix_world = Matrix.Identity(4)
    o.animation_data_clear()


# ------------------------------------------------------------ scale and centre
lid_objs  = [o for o in meshes if o.name in lid_names]
body_objs = [o for o in meshes if o.name not in lid_names]
P('body meshes', len(body_objs), 'lid meshes', len(lid_objs))

def bounds(objects):
    # From vertices, not bound_box: bound_box is cached and still holds the
    # pre-bake extents here, which silently offset the whole model by 12 units.
    lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
    for o in objects:
        for v in o.data.vertices:
            for i in range(3):
                lo[i] = min(lo[i], v.co[i]); hi[i] = max(hi[i], v.co[i])
    return lo, hi


lo, hi = bounds(body_objs)
S = Matrix.Scale(SCALE, 4)
# Origin under the middle of the closed laptop, on the table: that is where a
# prop's origin belongs, and it is what the entity's transform will mean.
centre = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z)) * SCALE
T = Matrix.Translation(-centre)
M = T @ S

for o in meshes:
    o.data.transform(M)

# The hinge frame, in final coordinates.
#
# DoorComponent turns an entity about its own local Y, so the frame's local Y
# has to BE the hinge axis -- and "local Y" survives the Z-up to Y-up export
# conversion, because that conversion changes which world direction an axis
# points in, never which axis of the node it is. Measured: the hinge runs
# along world X, so local Y must land on world X.
hinge_final = M @ hinge_loc
R = Matrix.Rotation(math.radians(-90), 4, 'Z')     # local Y -> world X

H = Matrix.Translation(hinge_final) @ R
P('hinge final', tuple(round(v, 4) for v in hinge_final))

# Lid geometry expressed relative to that frame, so the exported lid's origin
# IS the hinge and its up axis IS the pivot.
Hi = H.inverted()
for o in lid_objs:
    o.data.transform(Hi)

# ----------------------------------------------------------------- join groups
def join(objects, name):
    for o in bpy.data.objects:
        o.select_set(False)
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    if len(objects) > 1:
        bpy.ops.object.join()
    joined = bpy.context.view_layer.objects.active
    joined.name = name
    joined.data.name = name
    return joined

body = join(body_objs, 'LOD0')
lid  = join(lid_objs,  'LOD0_lid')     # renamed to LOD0 at export time

def local_bounds(o):
    lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
    for v in o.data.vertices:
        for i in range(3):
            lo[i] = min(lo[i], v.co[i]); hi[i] = max(hi[i], v.co[i])
    return lo, hi

P('body bounds', [tuple(round(x, 4) for x in b) for b in local_bounds(body)])
P('lid bounds',  [tuple(round(x, 4) for x in b) for b in local_bounds(lid)])

# ------------------------------------------------------------------- colliders
def make_box(name, lo, hi):
    mesh = bpy.data.meshes.new(name)
    xs = (lo.x, hi.x); ys = (lo.y, hi.y); zs = (lo.z, hi.z)
    verts = [(x, y, z) for x in xs for y in ys for z in zs]
    faces = [(0,1,3,2), (4,6,7,5), (0,4,5,1), (2,3,7,6), (0,2,6,4), (1,5,7,3)]
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    ob = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(ob)
    return ob

body_box = make_box('UBX_Laptop_Body', *local_bounds(body))
lid_box  = make_box('UBX_Laptop_Lid',  *local_bounds(lid))

# ---------------------------------------------------------------- the socket
socket = bpy.data.objects.new('socket_LaptopLid', None)
socket.empty_display_type = 'PLAIN_AXES'
socket.empty_display_size = 0.05
socket.matrix_world = H
bpy.context.scene.collection.objects.link(socket)

# ---------------------------------------------------------------------- export
def export(objects, path):
    for o in bpy.data.objects:
        o.select_set(False)
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
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

export([body, body_box, socket], os.path.join(OUT, 'Laptop_Body.fbx'))

# The lid's visual mesh has to be called LOD0 too, and Blender will not allow
# two objects of that name -- the second silently becomes LOD0.001, which the
# .meta then fails to find. So the body leaves the scene before the lid is
# renamed.
dead = [body.data, body_box.data]
for o in (body, body_box, socket):
    bpy.data.objects.remove(o, do_unlink=True)

# The mesh DATABLOCK has to go too, not just the object. An orphaned datablock
# still called LOD0 keeps the name taken, so the lid's mesh becomes LOD0.001 --
# and Enfusion matches MeshParam on the geometry name, so it built nothing at
# all and said nothing about why.
for m in dead:
    bpy.data.meshes.remove(m)

lid.name = 'LOD0'
lid.data.name = 'LOD0'

export([lid, lid_box], os.path.join(OUT, 'Laptop_Lid.fbx'))


P('materials now:', sorted(m.name for m in bpy.data.materials))
P('DONE')
