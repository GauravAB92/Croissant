import numpy as np

def normalize(v):
    norm = np.linalg.norm(v)
    return v / norm if norm != 0 else v

def detect_silhouette_edge(vtxID, normalWS, posWS, world_view):
    """
    Detect silhouette edges by transforming world-space positions and normals 
    into view space using the provided world-view matrix (4x4 homogeneous).
    """
    # Transform normals as vec4 (w=0)
    normalVS = []
    for n in normalWS:
        n4 = np.dot(world_view, np.array([n[0], n[1], n[2], 0.0]))
        normalVS.append(normalize(n4[:3]))
    
    # Transform positions as vec4 (w=1)
    posVS = []
    for p in posWS:
        p4 = np.dot(world_view, np.array([p[0], p[1], p[2], 1.0]))
        posVS.append(p4[:3] / p4[3] if p4[3] != 0 else p4[:3])
    
    # Compute view vector for this vertex (camera at origin)
    viewVec = normalize(-posVS[vtxID % 3])
    
    # Apply silhouette logic
    n0 = normalVS[vtxID % 3]
    n1 = normalVS[(vtxID + 1) % 3]
    n2 = normalVS[(vtxID + 2) % 3]
    n3 = normalize(n0 + n1 - n2)
    n4 = normalize(n0 + n1 + n2)

    back_face = np.dot(n3, viewVec) > 0.0
    is_silhouette = (np.dot(n4, viewVec) * np.dot(n3, viewVec)) < 0.0
    
    return bool(is_silhouette), bool(back_face)

# Placeholder world-view matrix elements (row-major)
world_view_elements = [
   -1.0000, 0.0000, 0.0000, 0.0000,
   0.0000, 1.0000, 0.0000, 0.0000,
   0.0000, 0.0000, -1.0000, 0.0000,
   0.0000, 0.0166, 2.9766, 1.0000
]

world_view = np.array(world_view_elements, dtype=float).reshape((4, 4))

# Example triangle data
sample_normals = [
    np.array([-0.427, 0.485, 0.763, 0.0]),
    np.array([-0.040, 0.954, 0.296, 0.0]),
    np.array([-0.630, 0.764, 0.140, 0.0])
]

sample_positions = [
    np.array([-0.091, -0.164, 0.806, 1.0]),
    np.array([-0.049, -0.139, 0.781, 1.0]),
    np.array([-0.101, -0.148, 0.782, 1.0])
]


print("Edge  | Silhouette | Back-Face")
print("------------------------------")
for vtxID in range(3):
    edge = f"{vtxID}-{(vtxID + 1) % 3}"
    sil, back = detect_silhouette_edge(vtxID, sample_normals, sample_positions, world_view)
    print(f"{edge:5} | {sil!s:10} | {back!s:9}")
