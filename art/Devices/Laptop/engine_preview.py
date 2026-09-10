"""Rebuild the laptop the way the ENGINE builds it, and render it.

Everything so far was reasoned about in Blender space and kept coming out
wrong, so this reads the two exported .fbx files back, converts the vertices
the way Enfusion does ((x,y,z) -> (x, z, -y)), applies the prefab's child
transform by hand, and renders. What comes out is what the game shows.
"""
import bpy, struct, zlib, math, os, sys
from mathutils import Vector, Matrix

ROLL = float(sys.argv[-1])
B    = r'G:\MCF\addons\MCF_Ops\Assets\Props\Intel\Laptop'
TMP  = r'G:\MCF\_check'
COORDS = (0.165, 0.021, 0.107)

def read_geom(path):
    d = open(path,'rb').read()
    ver = struct.unpack_from('<I', d, 23)[0]; is64 = ver >= 7500
    def node(pos):
        if is64: end, np_, pl = struct.unpack_from('<QQQ', d, pos); pos += 24
        else:    end, np_, pl = struct.unpack_from('<III', d, pos); pos += 12
        nl = d[pos]; pos += 1
        name = d[pos:pos+nl].decode('utf-8','replace'); pos += nl
        if end == 0: return None, pos
        props = []
        for _ in range(np_):
            t = chr(d[pos]); pos += 1
            if t in 'CBYIFDL':
                sz = {'C':1,'B':1,'Y':2,'I':4,'F':4,'D':8,'L':8}[t]
                fmt = {'C':'<b','B':'<b','Y':'<h','I':'<i','F':'<f','D':'<d','L':'<q'}[t]
                props.append(struct.unpack_from(fmt, d, pos)[0]); pos += sz
            elif t in 'fdlbi':
                cnt, enc, cl = struct.unpack_from('<III', d, pos); pos += 12
                raw = d[pos:pos+cl]; pos += cl
                if enc == 1: raw = zlib.decompress(raw)
                fmt = {'f':'<f','d':'<d','l':'<q','i':'<i','b':'<b'}[t]
                sz  = {'f':4,'d':8,'l':8,'i':4,'b':1}[t]
                props.append([struct.unpack_from(fmt, raw, k*sz)[0] for k in range(len(raw)//sz)])
            elif t in 'SR':
                ln = struct.unpack_from('<I', d, pos)[0]; pos += 4
                props.append(d[pos:pos+ln].decode('utf-8','replace') if t=='S' else '<raw>'); pos += ln
        kids = []
        while pos < end:
            k, pos = node(pos)
            if k is None: break
            kids.append(k)
        return (name, props, kids), end
    off = 27; roots = []
    while off < len(d)-20:
        n, off = node(off)
        if n is None: break
        roots.append(n)
    out = {}
    for name, props, kids in roots:
        if name != 'Objects': continue
        for kn, kp, kk in kids:
            if kn != 'Geometry': continue
            label = kp[1].split('\x00')[0]
            vs = idx = None
            for a,b,c in kk:
                if a == 'Vertices': vs = b[0]
                if a == 'PolygonVertexIndex': idx = b[0]
            if vs is None: continue
            verts = list(zip(vs[0::3], vs[1::3], vs[2::3]))
            faces = []; cur = []
            for i in idx:
                if i < 0: cur.append(-i - 1); faces.append(tuple(cur)); cur = []
                else: cur.append(i)
            out[label] = (verts, faces)
    return out

def add(name, verts, faces, xform):
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(xform(v)) for v in verts], [], faces)
    me.update()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob

bpy.ops.wm.read_factory_settings(use_empty=True)

# The engine's reading of blender-authored vertex data.
eng = lambda v: (v[0], v[2], -v[1])

body = read_geom(os.path.join(B, 'Laptop_Body.fbx'))
lid  = read_geom(os.path.join(B, 'LaptopLid.fbx'))

add('body', *body['LOD0'], xform=eng)

DOOR = float(sys.argv[-2])
a = math.radians(ROLL)
c, s = math.cos(a), math.sin(a)
da = math.radians(DOOR)
dc, ds = math.cos(da), math.sin(da)
def child(v):
    e = eng(v)
    # DoorComponent turns the entity about its own local Y first
    e = (e[0]*dc + e[2]*ds, e[1], -e[0]*ds + e[2]*dc)
    return (e[0]*c - e[1]*s + COORDS[0], e[0]*s + e[1]*c + COORDS[1], e[2] + COORDS[2])
add('lid', *lid['LOD0'], xform=child)

# Blender renders Z-up, so tip the whole engine-space scene onto its side once.
for ob in bpy.data.objects:
    ob.matrix_world = Matrix.Rotation(math.radians(90), 4, 'X') @ ob.matrix_world

cam_d = bpy.data.cameras.new('c'); cam = bpy.data.objects.new('c', cam_d)
bpy.context.scene.collection.objects.link(cam); bpy.context.scene.camera = cam
cam.location = (0.8, -0.8, 0.55); cam.rotation_euler = (math.radians(62), 0, math.radians(45))
sun_d = bpy.data.lights.new('s','SUN'); sun = bpy.data.objects.new('s', sun_d)
bpy.context.scene.collection.objects.link(sun); sun.rotation_euler = (math.radians(50),0,math.radians(30)); sun_d.energy = 5

sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.render.resolution_x = 700; sc.render.resolution_y = 500
sc.display.shading.color_type = 'OBJECT'
bpy.data.objects['body'].color = (0.45, 0.45, 0.5, 1)
bpy.data.objects['lid'].color  = (0.85, 0.6, 0.25, 1)

os.makedirs(TMP, exist_ok=True)
sc.render.filepath = os.path.join(TMP, 'engine_r%+d_d%+d.png' % (int(ROLL), int(DOOR)))
bpy.ops.render.render(write_still=True)
print('[E] wrote', sc.render.filepath)
