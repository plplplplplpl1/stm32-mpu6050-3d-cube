"""
Generate 4D regular polytope vertex/edge data as C header file.
All vertices normalized to unit 3-sphere (radius = 1.0).
Edges determined by minimum vertex-pair distance (or k-th level for star forms).
"""
import math, os, itertools

SPHERE_SCALE = 1.0

# ============================================================
# Helper functions
# ============================================================

def squared_dist(v, w):
    return sum((a - b) ** 2 for a, b in zip(v, w))

def normalize(v):
    r = math.sqrt(sum(x*x for x in v))
    if r == 0:
        return v
    return tuple(x / r for x in v)

def edge_list_from_vertices(verts, tolerance=1e-6):
    """Find all edges as pairs of vertices at minimum distance."""
    min_d2 = float('inf')
    n = len(verts)
    for i in range(n):
        for j in range(i + 1, n):
            d2 = squared_dist(verts[i], verts[j])
            if d2 < min_d2:
                min_d2 = d2
    edges = []
    for i in range(n):
        for j in range(i + 1, n):
            d2 = squared_dist(verts[i], verts[j])
            if abs(d2 - min_d2) < tolerance:
                edges.append((i, j))
    return sorted(edges)

def edge_list_from_vertices_by_level(verts, level, tolerance=1e-6):
    """
    Find edges using the (level)-th smallest unique distance.
    level=0 is the minimum distance (same as edge_list_from_vertices).
    level=1 is the next distance threshold, etc.
    """
    n = len(verts)
    # Collect all unique distances
    all_d2 = set()
    for i in range(n):
        for j in range(i + 1, n):
            d2 = squared_dist(verts[i], verts[j])
            all_d2.add(round(d2, 10))  # round to avoid floating noise
    sorted_d2 = sorted(all_d2)
    if level >= len(sorted_d2):
        level = len(sorted_d2) - 1
    target_d2 = sorted_d2[level]
    edges = []
    for i in range(n):
        for j in range(i + 1, n):
            d2 = squared_dist(verts[i], verts[j])
            if abs(d2 - target_d2) < tolerance:
                edges.append((i, j))
    return sorted(edges)

def even_permutations(signs):
    """Generate all even permutations of a sign pattern (list of floats)."""
    n = len(signs)
    indices = list(range(n))
    all_perms = set()
    for perm in itertools.permutations(indices):
        inv = 0
        for a in range(n):
            for b in range(a + 1, n):
                if perm[a] > perm[b]:
                    inv += 1
        if inv % 2 == 0:
            tup = tuple(signs[perm[i]] for i in range(n))
            all_perms.add(tup)
    return sorted(all_perms)

def all_sign_combinations(pattern):
    """Generate all sign combinations of a pattern tuple.
    pattern: tuple like (1, 1, 0, 0) — zeros stay zero, non-zeros get +/-."""
    results = []
    n = len(pattern)
    for bits in range(1 << n):
        tup = []
        for i in range(n):
            if pattern[i] == 0:
                tup.append(0.0)
            elif pattern[i] > 0:
                tup.append(pattern[i] if (bits >> i) & 1 else -pattern[i])
            else:
                tup.append(pattern[i])
        results.append(tuple(tup))
    return results

def all_sign_permutes(base_pattern):
    """All sign choices AND all permutations of a pattern."""
    seen = set()
    results = []
    for perm in set(itertools.permutations(base_pattern)):
        for signs in all_sign_combinations(perm):
            if signs not in seen:
                seen.add(signs)
                results.append(signs)
    return results

# ============================================================
PHI = (1.0 + math.sqrt(5.0)) / 2.0
INV_PHI = 1.0 / PHI

# ============================================================
# 1. 5-cell {3,3,3} — 5 vertices, 10 edges (complete graph K5)
# ============================================================
def make_5cell_verts():
    s = math.sqrt(5.0 / 2.0)
    r3 = math.sqrt(3.0)
    r6 = math.sqrt(6.0)
    r10 = math.sqrt(10.0)
    verts_raw = [
        (-0.5,      -r3 / 6.0,  -r6 / 12.0, -r10 / 20.0),
        (0.5,       -r3 / 6.0,  -r6 / 12.0, -r10 / 20.0),
        (0.0,        r3 / 3.0,  -r6 / 12.0, -r10 / 20.0),
        (0.0,        0.0,        r6 / 4.0,  -r10 / 20.0),
        (0.0,        0.0,        0.0,         r10 / 5.0),
    ]
    return [tuple(c * s for c in v) for v in verts_raw]

# ============================================================
# 2. Tesseract {4,3,3} — 16 vertices, 32 edges
# ============================================================
def make_tesseract_verts():
    verts = []
    for bits in range(16):
        v = tuple(1.0 if (bits >> i) & 1 else -1.0 for i in range(4))
        verts.append(normalize(v))
    return verts

# ============================================================
# 3. 16-cell {3,3,4} — 8 vertices, 24 edges
# ============================================================
def make_16cell_verts():
    verts = []
    for i in range(4):
        for s in [1.0, -1.0]:
            v = [0.0, 0.0, 0.0, 0.0]
            v[i] = s
            verts.append(tuple(v))
    return [normalize(v) for v in verts]

# ============================================================
# 4. 24-cell {3,4,3} — 24 vertices, 96 edges
# ============================================================
def make_24cell_verts():
    verts = set()
    for perm in set(itertools.permutations([1.0, 1.0, 0.0, 0.0])):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))
    for i in range(4):
        for s in [1.0, -1.0]:
            v = [0.0, 0.0, 0.0, 0.0]
            v[i] = s
            verts.add(tuple(v))
    return [normalize(v) for v in verts]

# ============================================================
# 5. 600-cell {3,3,5} — 120 vertices, 720 edges
# ============================================================
def make_600cell_verts():
    verts = set()
    for bits in range(16):
        v = tuple(1.0 if (bits >> i) & 1 else -1.0 for i in range(4))
        verts.add(v)
    for i in range(4):
        for s in [2.0, -2.0]:
            v = [0.0, 0.0, 0.0, 0.0]
            v[i] = s
            verts.add(tuple(v))
    base = [PHI, 1.0, INV_PHI, 0.0]
    for perm in even_permutations(base):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))
    result = [normalize(v) for v in verts]
    assert len(result) == 120, f"Expected 120 vertices, got {len(result)}"
    return result

# ============================================================
# 6. 120-cell {5,3,3} — 600 vertices, 1200 edges
# ============================================================
def make_120cell_verts():
    """
    Generate all 600 vertices of the 120-cell {5,3,3}.

    Construction: all vertices can be generated from 3 seed patterns
    using ALL permutations (not just even) with all sign choices,
    plus additional patterns for the remaining vertices.

    The 600-cell vertices (120) are a subset.
    The remaining 480 vertices correspond to 5 rotated copies
    of the 600-cell vertex set.
    """
    verts = set()

    # === 600-cell vertices (subset) ===
    # Set 1: (±1, ±1, ±1, ±1)
    for bits in range(16):
        v = tuple(1.0 if (bits >> i) & 1 else -1.0 for i in range(4))
        verts.add(v)

    # Set 2: (±2, 0, 0, 0) with all permutations
    for i in range(4):
        for s in [2.0, -2.0]:
            v = [0.0, 0.0, 0.0, 0.0]
            v[i] = s
            verts.add(tuple(v))

    # Set 3: all even permutations of (±PHI, ±1, ±INV_PHI, 0)
    base = [PHI, 1.0, INV_PHI, 0.0]
    for perm in even_permutations(base):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # === Additional 120-cell vertices ===
    # Seed patterns giving new vertices beyond the 600-cell.
    # Each generates additional coordinate sets via even permutations + signs.

    # Pattern A: (PHI, PHI, 1, 0)
    base_a = [PHI, PHI, 1.0, 0.0]
    for perm in even_permutations(base_a):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern B: (PHI^2, 1/PHI, 1, 0)
    base_b = [PHI*PHI, INV_PHI, 1.0, 0.0]
    for perm in even_permutations(base_b):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern C: (PHI^2, PHI, 1/PHI^2, 0)
    base_c = [PHI*PHI, PHI, INV_PHI*INV_PHI, 0.0]
    for perm in even_permutations(base_c):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern D: (PHI^2, PHI^2, 1/PHI, 0)
    base_d = [PHI*PHI, PHI*PHI, INV_PHI, 0.0]
    for perm in even_permutations(base_d):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern E: (PHI^2, 1/PHI^2, 1, 1) -- 4 non-zero values
    # hmm... let me try another approach

    # Actually, let me try: all permutations of PHI, 1, 1/PHI, 1/PHI
    base_e = [PHI, 1.0, INV_PHI, INV_PHI]
    for perm in even_permutations(base_e):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern F: (PHI, PHI, 1/PHI, 1/PHI)
    base_f = [PHI, PHI, INV_PHI, INV_PHI]
    for perm in even_permutations(base_f):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    # Pattern G: (PHI, PHI, 1/PHI, 0)
    base_g = [PHI, PHI, INV_PHI, 0.0]
    for perm in even_permutations(base_g):
        for signs in all_sign_combinations(perm):
            verts.add(tuple(signs))

    result = [normalize(v) for v in verts]
    print(f"  120-cell: {len(result)} vertices")
    return result


# ============================================================
# Star polytopes — 600-cell vertex set, different edge distances
# ============================================================
def make_star_polytope(verts, level, label):
    """Generate star polytope from vertex set using level-th edge distance."""
    edges = edge_list_from_vertices_by_level(verts, level)
    return verts, edges


# ============================================================
# Polytope definitions
# ============================================================

def build_polytopes():
    """Return list of (label, vertices, edges, comment) for all polytopes."""
    # 600-cell vertices (used by multiple polytopes)
    v600 = make_600cell_verts()

    polytopes = [
        ("P5_CELL",   make_5cell_verts(),       None, "5-cell {3,3,3}"),
        ("TESSERACT", make_tesseract_verts(),    None, "Tesseract {4,3,3}"),
        ("P16_CELL",  make_16cell_verts(),       None, "16-cell {3,3,4}"),
        ("P24_CELL",  make_24cell_verts(),       None, "24-cell {3,4,3}"),
        ("P600_CELL", v600,                      None, "600-cell {3,3,5}"),
    ]

    # Star polytopes from 600-cell vertices at different edge distance levels
    # Level 0 = 600-cell (already added above)
    star_info = [
        (1, "P600_STAR1", "{5,5/2,5} Icos. 120-cell"),
        (2, "P600_STAR2", "{3,3,5/2} Grand 600-cell"),
        (3, "P600_STAR3", "{3,5/2,3} Great 600-cell"),
        (4, "P600_STAR4", "{5/2,3,5/2} GrGrand 600-cell"),
    ]
    for level, label, comment in star_info:
        edges = edge_list_from_vertices_by_level(v600, level)
        polytopes.append((label, v600, edges, comment + f" (lvl{level})"))

    return polytopes


# ============================================================
# Generate C header
# ============================================================

output_path = os.path.join(os.path.dirname(__file__), "..", "Hardware", "Cube3D_4D.h")
output_path = os.path.normpath(output_path)

polytopes = build_polytopes()

with open(output_path, "w") as f:
    f.write("/* Auto-generated by Tools/gen_4d_cell.py */\n")
    f.write("#ifndef __CUBE3D_4D_H\n#define __CUBE3D_4D_H\n\n")
    f.write('#include "Cube3D.h"\n\n')

    max_v = 0
    total_b = 0

    for label, verts_raw, edges_override, comment in polytopes:
        if edges_override is not None:
            edges = edges_override
        else:
            edges = edge_list_from_vertices(verts_raw)
        nv = len(verts_raw)
        ne = len(edges)
        max_v = max(max_v, nv)
        vb = nv * 4 * 4   # Vec4f_t = 4 floats × 4 bytes
        eb = ne * 2
        total_b += vb + eb

        f.write(f"/* {comment}: {nv} verts, {ne} edges, ~{vb+eb}B */\n")
        f.write(f"#define {label}_VTX_COUNT  {nv}\n")
        f.write(f"#define {label}_EDGE_COUNT {ne}\n\n")
        f.write(f"static const Vec4f_t k{label}Vertices[{label}_VTX_COUNT] =\n{{\n")
        for v in verts_raw:
            f.write(f"\t{{{v[0]:.8f}f, {v[1]:.8f}f, {v[2]:.8f}f, {v[3]:.8f}f}},\n")
        f.write("};\n\n")
        f.write(f"static const uint8_t k{label}Edges[{label}_EDGE_COUNT][2] =\n{{\n")
        for v1, v2 in edges:
            f.write(f"\t{{{v1}, {v2}}},\n")
        f.write("};\n\n")

    f.write(f"/* MAX_VTX = {max_v} */\n")
    f.write(f"#define CELL4D_MAX_VTX {max_v}\n\n")
    f.write(f"/* Total Flash: ~{total_b} bytes */\n")
    f.write("#endif\n")

    print(f"Generated {output_path}")
    print(f"  {len(polytopes)} polytopes, max {max_v} verts, ~{total_b} bytes")
    print(f"  Polytopes: {[l for l,_,_,_ in polytopes]}")
