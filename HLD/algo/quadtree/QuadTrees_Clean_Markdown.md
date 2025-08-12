# QuadTrees and Location-Based Databases - HLD Interview Notes

> **📺 Video Reference:** [Location Based Databases](https://www.youtube.com/watch?v=OcUKFIjhKu0)  
> **📊 Visual Examples:** [QuadTrees_Visual_Examples.md](./QuadTrees_Visual_Examples.md)

---

## Table of Contents
- [Problems with Traditional Systems](#problems-with-traditional-systems)
- [System Requirements](#system-requirements)
- [Coordinate Systems Reality Check](#coordinate-systems-reality-check)
- [QuadTree Data Structure](#quadtree-data-structure)
- [Space-Filling Curves](#space-filling-curves)
- [Real-World Systems](#real-world-systems)
- [Interview Questions](#interview-questions)

---

## Problems with Traditional Systems
**Timestamps:** `[00:00:11 - 00:01:26]`

### Pin Code Problem Example

**Real case [00:00:50]:** Restaurant (PIN: 400051) vs Customer (PIN: 400050)

```
📍 Pin Code Problem:
┌─────────────────┬─────────────────┐
│   PIN: 400050   │   PIN: 400051   │
│   🏠 Customer   │   🍕 Restaurant │
└─────────────────┴─────────────────┘
         │                 │
         └─────────────────┘
              🚧 BRIDGE
         (2km detour required)
```

- **Issue:** Similar codes ≠ geographical proximity
- **Result:** 2km extra delivery distance due to bridge

### Industry Examples
- **DoorDash:** Cross-river delivery issues with zip codes
- **Amazon:** Now uses drive-time calculations + zip codes
- **Zomato:** Mumbai delivery problems due to geographical barriers

---

## System Requirements
**Timestamps:** `[00:01:26 - 00:02:39]`

### Core Requirements

1. **Distance Measurement** `[00:01:26]`
   - Accurate distance between any two points
   - ⚠️ **Must use spherical math, not Euclidean**

2. **Scalable Granularity** `[00:01:55]`
   - Country → State → City → Block level
   - Arbitrary precision scaling

3. **Proximity Search** `[00:02:02]`
   - "Find all X within Y kilometers"
   - Efficient range queries

```
🎯 Proximity Search:
      🍕     🍕
   🍕    🏠     🍕
      🍕  ↑  🍕
         10km
    (Find restaurants
     within radius)
```

---

## Coordinate Systems Reality Check
**Timestamps:** `[00:02:39 - 00:03:44]`

### ❌ Video's Incorrect Claim

> **❌ WRONG:** "find the Euclidean distance with the simple Euclidean formula" `[00:02:55]`

### ✅ Correct Approach: Haversine Formula

```javascript
function haversineDistance(lat1, lon1, lat2, lon2) {
    const R = 6371; // Earth radius in km
    const dLat = toRadians(lat2 - lat1);
    const dLon = toRadians(lon2 - lon1);
    const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
              Math.cos(toRadians(lat1)) * Math.cos(toRadians(lat2)) *
              Math.sin(dLon/2) * Math.sin(dLon/2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    return R * c;
}
```

### Error Comparison Table

| Distance | Euclidean Error | Impact |
|----------|----------------|---------|
| 1km      | ~0.01%         | ✅ OK |
| 100km    | ~1%            | ⚠️ Noticeable |
| 1000km   | ~10%           | ❌ Bad |
| 10000km  | ~20%+          | ❌ Terrible |

### Real Usage
- **Google Maps API:** Haversine formula
- **MongoDB:** 2dsphere indexes with spherical geometry
- **Redis:** GEODIST command uses Haversine
- **PostGIS:** ST_Distance_Sphere() function

---

## QuadTree Data Structure
**Timestamps:** `[00:08:20 - 00:10:33]`

### Basic Concept `[00:08:20]`
- Binary Search Tree → 2D space
- 4 children per node (NW, NE, SW, SE)
- Recursive spatial partitioning

### Visual Progression

**Level 0: Whole World**
```
┌─────────────────┐
│                 │
│     World       │
│                 │
└─────────────────┘
```

**Level 1: 4 Quadrants**
```
┌─────────┬─────────┐
│   NW    │   NE    │
│         │         │
├─────────┼─────────┤
│   SW    │   SE    │
│         │         │
└─────────┴─────────┘
```

**Level 2: 16 Sub-quadrants**
```
┌───┬───┬───┬───┐
│NW │NE │NW │NE │
├───┼───┼───┼───┤
│SW │SE │SW │SE │
├───┼───┼───┼───┤
│NW │NE │NW │NE │
├───┼───┼───┼───┤
│SW │SE │SW │SE │
└───┴───┴───┴───┘
```

### Splitting Strategies `[00:09:25]`

1. **Fixed Depth:** Split to target granularity (e.g., 1km squares)
2. **Threshold-Based:** Split when points > threshold `[00:09:40]`

**Threshold Example:**
```
Before Split (8 points > threshold=3):
┌─────────────────────┐
│  •     •      •     │  8 points total
│                     │  exceeds threshold
│    •   •    •       │  → split required
│           •    •    │
└─────────────────────┘

After Split:
┌─────────┬───────────┐
│  •      │  •        │  NW: 1 ✓
│         │           │  NE: 1 ✓  
├─────────┼───────────┤
│   •  •  │  •        │  SW: 2 ✓
│         │     •  •  │  SE: 3 ✓
└─────────┴───────────┘
```

### Real-World Examples

**Pokemon GO Implementation:**
```
Level 0: Game Region (12 spawns)
┌─────────────────────────┐
│🟢  🔴  🟡  🔵  🟠  🟣  │  Too many spawns
│🟢  🔴     🟡  🔵       │  → split needed
│        🟠  🟣         │
└─────────────────────────┘

Level 1: After Split
┌───────────┬─────────────┐
│🟢  🔴  🟡 │   🟠       │  NW: 6 (high)
│           │             │  NE: 1 (low)
├───────────┼─────────────┤  
│🟢  🔴     │  🟣        │  SW: 3 (med)
│           │             │  SE: 2 (low)
└───────────┴─────────────┘
```

### Performance & Limitations

**Time Complexities:**
- Point Query: O(log n) average, O(h) worst case
- Range Query: O(√n + k) balanced, O(n) worst case
- Space: O(n)

**Skew Problem:**
```
Mumbai (Dense):          Rural (Sparse):
┌─┬─┬─┬─┬─┬─┬─┬─┐       ┌─────────────────┐
├─┼─┼─┼─┼─┼─┼─┼─┤  vs   │        •        │
├─┼─┼─┼─┼─┼─┼─┼─┤       │                 │
└─┴─┴─┴─┴─┴─┴─┴─┘       └─────────────────┘
   Deep tree (slow)         Shallow (fast)
```

---

## Space-Filling Curves
**Timestamps:** `[00:11:12 - 00:15:41]`

### Problem: 2D → 1D Conversion `[00:10:40]`

**Why?** 1D algorithms are much more efficient:
- Segment trees: O(log n) range queries
- Interval trees: Well-understood, optimized

### Three Curve Types `[00:14:10 - 00:15:04]`

#### 1. Z-Curve (Morton Order) `[00:14:13]`
```
┌─────┬─────┐     Traversal:
│  0  │  1  │     ┌─→───┬─────┐
├─────┼─────┤  =  │  1  │  2  │
│  2  │  3  │     ├─────┼─↗───┤
└─────┴─────┘     │  0  │  3  │
                  └─↑───┴─────┘
Path: 0→1→2→3 (Z-shape)
```

**Used by:** Google Bigtable, HBase, DynamoDB

#### 2. Hilbert Curve ⭐ `[00:14:26]`
```
┌─────┬─────┐     Traversal:
│  0  │  1  │     ┌─→─┬─→─┐
├─────┼─────┤  =  │ 0 │ 1 │
│  3  │  2  │     ├─↑─┼─↓─┤
└─────┴─────┘     │ 3 │ 2 │
                  └───┴─←─┘
Path: 0→1→2→3 (U-shape)
✅ Better proximity preservation!
```

**Used by:** Google S2, Uber H3, Bing Maps

#### 3. Poor Example (Don't Use)
```
┌─────┬─────┐     
│  0  │  3  │     ❌ Long jumps between  
├─────┼─────┤        nearby points
│  1  │  2  │     
└─────┴─────┘     
```

---

## Real-World Systems

### Google S2 (What Google Actually Uses)
- **Spherical geometry** on unit sphere
- **Hilbert curve** on sphere surface  
- **30 resolution levels:** ~85km² to ~1cm²
- **64-bit cell IDs**

```python
import s2sphere
region = s2sphere.LatLngRect(
    lat_lo=s2sphere.LatLng.from_degrees(37.4, -122.2),
    lat_hi=s2sphere.LatLng.from_degrees(37.5, -122.1)
)
covering = s2sphere.RegionCoverer().get_covering(region)
```

### Uber's H3 System
- **Hexagonal grids** (better than squares)
- **6 neighbors** vs 4 in QuadTree
- **15 resolution levels**
- **Open source**

```python
import h3
h3_index = h3.geo_to_h3(37.775, -122.419, 9)
neighbors = h3.k_ring(h3_index, 1)
```

**H3 vs QuadTree:**
```
Square (QuadTree):    Hexagonal (H3):
┌─┬─┬─┬─┐              ⬢─⬢─⬢─⬢
├─┼─┼─┼─┤             ⬢─⬢─⬢─⬢─⬢
└─┴─┴─┴─┘              ⬢─⬢─⬢─⬢

4 neighbors           6 neighbors
❌ Uneven distances   ✅ Even distances
```

### Redis Geospatial
```redis
GEOADD cities -122.27652 37.80510 "San Francisco"
GEORADIUS cities -122.27652 37.80510 100 km WITHDIST
```

### MongoDB 2dsphere
```javascript
db.places.createIndex({location: "2dsphere"})
db.restaurants.find({
    location: {
        $near: {
            $geometry: {type: "Point", coordinates: [-73.97, 40.75]},
            $maxDistance: 1000
        }
    }
})
```

---

## Proximity Search Implementation
**Timestamps:** `[00:19:38 - 00:21:30]`

### Basic Algorithm `[00:19:43]`
1. Map query point to 1D position
2. Apply threshold (±k) around position
3. Return all points in range

### Example `[00:19:38]`
```
Query point: 29, threshold: ±6
Range: [23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35]

2D Result:
┌───┬───┬───┬───┐
│23 │24 │33 │34 │
├───┼───┼───┼───┤
│25 │26 │32 │35 │
├───┼───┼───┼───┤
│27 │28 │31 │36 │
├───┼───┼───┼───┤
│📍│30 │   │   │  📍 = Query point (29)
└───┴───┴───┴───┘
```

### Edge Cases `[00:20:17]`

**False Negative:** Points 18 & 29 very close in 2D, far in 1D
**False Positive:** Points 29 & 40 far in 2D, close in 1D

### Production Solutions

**Multi-Resolution Search:**
```python
def proximity_search(point, radius):
    results = set()
    for resolution in [8, 9, 10]:  # Multiple levels
        cells = get_covering_cells(point, radius, resolution)
        for cell in cells:
            results.update(query_cell(cell))
    return filter_by_actual_distance(results, point, radius)
```

**Boundary Buffer:** Search neighboring cells at boundaries

---

## Performance Comparison

| System | Point Query | Range Query | Space | Best For |
|--------|-------------|-------------|-------|----------|
| QuadTree | O(log n) | O(√n + k) | O(n) | Sparse data |
| Geohashing | O(1) | O(cells+k) | O(n) | General purpose |
| R-Tree | O(log n) | O(log n+k) | O(n) | Polygons |
| H3 | O(1) | O(rings+k) | O(n) | Hexagonal grids |

---

## Interview Questions

### System Design
1. **"Design Uber's driver matching"**
   - H3 vs QuadTree trade-offs
   - Real-time updates (every 4 seconds)
   - Geographic load balancing

2. **"Design food delivery proximity search"**
   - Multi-layer: city → neighborhood → precise
   - Caching popular areas
   - Peak hour handling

3. **"Scale to 1 billion users"**
   - Partitioning strategies
   - Update frequency optimization
   - Read vs write optimization

### Technical Deep-Dive
1. **"Compare spatial indexing methods"**
   - When to use each approach
   - Performance characteristics
   - Real-world examples

2. **"Handle boundary problems"**
   - Multi-cell search
   - Buffer zones
   - Edge case strategies

3. **"Optimize for real-time vs batch"**
   - Caching strategies
   - Consistency trade-offs
   - Load balancing

---

## Key Interview Points

### ✅ What to Emphasize
1. **Trade-offs:** Accuracy vs Performance vs Complexity
2. **Real constraints:** Data skew, update frequency
3. **Modern alternatives:** H3, S2 often better than pure QuadTrees
4. **Production reality:** Hybrid approaches, caching, edge cases

### ❌ Common Mistakes
1. Using Euclidean distance for lat/long
2. Ignoring data distribution skew
3. Forgetting boundary problems
4. Oversimplifying - real systems are complex

---

## Quick Reference

**Best Fonts for ASCII Diagrams:**
- Consolas (Windows)
- Monaco (Mac)  
- Courier New (Cross-platform)
- JetBrains Mono (Modern)

**Recommended Tools:**
- VS Code with Consolas font
- Notion (great for technical docs)
- Obsidian (markdown focused)

---

## References
- **Video:** [Location Based Databases](https://www.youtube.com/watch?v=OcUKFIjhKu0)
- **Google S2:** http://s2geometry.io/
- **Uber H3:** https://h3geo.org/
- **Redis Geo:** https://redis.io/commands/#geo 