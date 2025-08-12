# Geohash - High Level Design Interview Notes

## Table of Contents
1. [What is Geohash](#what-is-geohash)
2. [Geohash vs Quadtree](#geohash-vs-quadtree)
3. [Real-World Applications](#real-world-applications)
4. [Implementation with PostgreSQL](#implementation-with-postgresql)
5. [Boundary Problems & Solutions](#boundary-problems--solutions)
6. [Performance Considerations](#performance-considerations)
7. [Geohash Cell Size Variation](#geohash-cell-size-variation)
8. [Industrial Implementation Strategies](#industrial-implementation-strategies)
9. [Key Interview Points](#key-interview-points)

---

## What is Geohash

### Definition
Geohash is a geocoding system that encodes geographic coordinates (latitude/longitude) into a short string of letters and numbers.

### How It Works
- **Base-32 encoding**: Uses 32 characters (0-9, a-z excluding confusing ones like i, l, o)
- **Hierarchical**: Each additional character increases precision
- **Fixed subdivision**: Always follows the same 32-way splitting pattern

### Example
```
Location: Times Square, NYC (40.7589° N, 73.9851° W)
Geohash Levels:
- h        → Northeastern United States (~1000km)
- hv       → New York State (~300km)
- hv4      → NYC area (~100km)
- hv4w     → Manhattan (~30km)
- hv4w2    → Midtown Manhattan (~5km)
- hv4w2p   → Few city blocks (~1km)
- hv4w2p0  → Your front yard (~200m)
```

### Real-Life Analogy
Think of Geohash like a **postal code system for the entire world**, but much more precise. Each character you add makes the area smaller and more specific.

---

## Geohash vs Quadtree

| Aspect | Geohash | Quadtree |
|--------|---------|----------|
| **Division Pattern** | 32-way splits | 4-way splits (quadrants) |
| **Adaptiveness** | Fixed grid (NOT adaptive) | Truly adaptive to data density |
| **Structure** | String-based encoding | Tree data structure |
| **Precision** | Same everywhere (geographically variable) | Adapts to data distribution |
| **Memory** | Predictable, uniform | Variable, depends on data |
| **Query Speed** | Very fast prefix matching | Fast range queries |
| **Implementation** | Simple string operations | More complex tree operations |

### Simple Analogies
- **Geohash**: Like a postal code system - every area gets a code regardless of density
- **Quadtree**: Like organizing a closet - divide sections that get crowded, leave empty areas large

### When to Use Which
**Geohash:**
- ✅ Simple location encoding
- ✅ Fast nearby searches with known limitations
- ✅ String-based operations preferred
- ✅ Predictable performance needed

**Quadtree:**
- ✅ Uneven data distribution
- ✅ Memory efficiency important
- ✅ True spatial queries needed
- ✅ Adaptive detail required

---

## Real-World Applications

### Uber
```python
# Simplified Uber driver matching
user_location_geohash = "hv4w2p"
nearby_drivers = query("SELECT * FROM drivers WHERE geohash LIKE 'hv4w2%'")
```

### Yelp/Google Places Database Design
```sql
-- Business storage with Geohash
CREATE TABLE businesses (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    business_type VARCHAR(100),
    geohash VARCHAR(12),    -- Key for proximity search
    latitude DECIMAL(10,8),
    longitude DECIMAL(11,8)
);

-- Index for fast proximity searches
CREATE INDEX idx_businesses_geohash ON businesses (geohash);
CREATE INDEX idx_businesses_geohash_type ON businesses (geohash, business_type);
```

### Search Query Pattern
```sql
-- Find nearby restaurants
SELECT * FROM businesses 
WHERE geohash LIKE 'hv4w2%' 
AND business_type = 'restaurant'
ORDER BY ST_Distance(
    ST_MakePoint(longitude, latitude),
    ST_MakePoint(-73.9851, 40.7589)
);
```

---

## Boundary Problems & Solutions

### Problem 1: No Common Prefix
Two neighboring cells might have completely different prefixes due to Geohash boundary issues:
```
Example at Geohash boundary:
Location A: 7zzzzz (eastern edge of cell)
Location B: b00000 (western edge of adjacent cell)
These are physically next to each other but share NO common prefix!

Real-world example:
Location A: gcpvj0 (San Francisco)
Location B: 9q8yy0 (Just across a major boundary)
These locations might be very close but have completely different prefixes.
```

### Problem 2: Nearby Points in Different Cells
Physically close points end up in different Geohash cells due to grid boundaries.

### Industrial Solutions

#### Solution 1: Calculate Neighboring Cells
```python
import geohash2

def get_all_nearby_cells(lat, lng, precision=6):
    current = geohash2.encode(lat, lng, precision)
    neighbors = geohash2.neighbors(current)
    return [current] + list(neighbors.values())

# Query all 9 cells (current + 8 neighbors)
cells = get_all_nearby_cells(40.7589, -73.9851, 6)
# Result: ['hv4w2p', 'hv4w2n', 'hv4w2q', 'hv4w2m', 'hv4w2r', ...]
```

#### Solution 2: Hybrid Approach (Most Common)
```sql
-- Step 1: Use Geohash for fast filtering
-- Step 2: Use actual distance for precision
WITH geohash_filtered AS (
    SELECT * FROM businesses 
    WHERE geohash IN ('hv4w2p', 'hv4w2n', 'hv4w2q', 'hv4w2m', 'hv4w2r')
)
SELECT *, 
       ST_Distance(
         ST_MakePoint(longitude, latitude)::geography,
         ST_MakePoint(-73.9851, 40.7589)::geography
       ) as distance_meters
FROM geohash_filtered
WHERE ST_Distance(
    ST_MakePoint(longitude, latitude)::geography,
    ST_MakePoint(-73.9851, 40.7589)::geography
) < 1000
ORDER BY distance_meters;
```

---

## Performance Considerations

### ❌ SLOW Query (Don't Do This)
```sql
-- Scans ALL businesses, calculates distance for each
SELECT * FROM businesses 
WHERE latitude BETWEEN 40.7580 AND 40.7598
  AND longitude BETWEEN -73.9858 AND -73.9842
  AND ST_Distance(
    ST_MakePoint(longitude, latitude)::geography,
    ST_MakePoint(-73.9851, 40.7589)::geography
  ) < 1000;
-- Performance: 5-10 seconds on 1M+ businesses
```

### ✅ FAST Query (Do This)
```sql
-- Uses Geohash index, only calculates distance on filtered results
SELECT * FROM businesses 
WHERE geohash LIKE 'hv4w2%'
  AND ST_Distance(
    ST_MakePoint(longitude, latitude)::geography,
    ST_MakePoint(-73.9851, 40.7589)::geography
  ) < 1000;
-- Performance: 50-100ms on 1M+ businesses
```

### Performance Comparison
| Method | Records Scanned | Query Time | Use Case |
|--------|----------------|------------|----------|
| Bounding Box | 100K+ rows | 5-10 seconds | ❌ Never use |
| Geohash Prefix | ~100 rows | 50-100ms | ✅ Standard approach |
| Neighboring Cells | ~500 rows | 100-200ms | ✅ Best accuracy |

---

## Geohash Cell Size Variation

### Key Point: Geohash Cells Are NOT Uniform in Size

**Important**: Cell sizes vary by latitude due to Earth's spherical shape.

### Size Examples (6-character Geohash)
| Location | Latitude | Width | Height | Area |
|----------|----------|-------|--------|------|
| Equator | 0° | ~1.2 km | ~0.6 km | ~0.7 km² |
| New York | 40° | ~0.9 km | ~0.6 km | ~0.5 km² |
| Alaska | 70° | ~0.4 km | ~0.6 km | ~0.2 km² |
| North Pole | 90° | ~0 km | ~0.6 km | ~0 km² |

### Why This Happens
- **Longitude lines converge** at the poles
- **Latitude lines stay constant** (always ~111km apart)
- **Result**: Cells become narrower near poles

### Impact on Applications
```javascript
// Same precision, different coverage areas
const equatorCell = "s00000";  // ~0.7 km² area
const alaskaCell = "bpz000";   // ~0.2 km² area
// Alaska search might miss nearby businesses!
```

---

## Industrial Implementation Strategies

### Key Insight: Companies Make Geohash Adaptive

**Native Geohash**: Fixed, not adaptive  
**Industrial Usage**: Made adaptive through smart implementation strategies

### Strategy 1: Adaptive Precision by Region
```python
class LocationService:
    def get_optimal_precision(self, lat, lng):
        region_density = self.analyze_business_density(lat, lng)
        
        if region_density == "rural":
            return 4        # Large cells, few businesses
        elif region_density == "suburban":
            return 5        # Medium cells
        elif region_density == "urban":
            return 6        # Small cells, many businesses
        elif region_density == "dense_urban":
            return 7        # Very small cells, very dense
```

### Strategy 2: Dynamic Precision Based on Results
```python
def search_with_adaptive_precision(lat, lng, category):
    precision = 5  # Start medium
    
    while precision <= 8:
        geohash = encode_geohash(lat, lng, precision)
        results = query_database(f"geohash LIKE '{geohash}%'")
        
        if len(results) < 100:  # Good number
            return results
        else:
            precision += 1  # Increase precision for smaller area
    
    return results[:100]
```

### Strategy 3: Multi-Level Indexing
```python
# Different strategies for different densities
def yelp_search_strategy(lat, lng, radius):
    area_type = classify_area_density(lat, lng)
    
    if area_type == "rural":
        # Use broader search, fewer precision levels
        return search_with_precision(lat, lng, 4, radius * 2)
    
    elif area_type == "dense_urban":
        # Use multiple precision levels, exact distance filtering
        return search_with_neighboring_cells(lat, lng, 7, radius)
```

### Real Examples

#### Rural Montana
```
Geohash: 'c2b2' (4 characters)
Coverage: ~40km x 20km  
Businesses: 5 restaurants in entire area
Strategy: Large cells, simple search
```

#### Manhattan NYC
```
Geohash: 'hv4w2p0' (7 characters)
Coverage: ~150m x 75m
Businesses: 50+ restaurants in single cell  
Strategy: Small cells, neighboring search, distance filtering
```

---

## Key Interview Points

### What Interviewers Want to Hear

#### 1. Understanding of Core Concept
- "Geohash encodes lat/lng into hierarchical strings"
- "Each character increases precision"
- "Uses base-32 encoding for efficiency"

#### 2. Real-World Application Knowledge
- "Perfect for services like Uber, Yelp, Google Places"
- "Enables fast proximity searches using string prefix matching"
- "Stores well in PostgreSQL with proper indexing"

#### 3. Boundary Problem Awareness
- "Two neighboring locations might not share common prefix"
- "Solution: Calculate and search neighboring cells"
- "Use libraries to find the 8 neighboring Geohash cells"

#### 4. Performance Understanding
- "Geohash enables fast filtering, then use actual distance for precision"
- "Index the geohash column for lightning-fast prefix searches"
- "Avoid calculating distance on entire dataset"

#### 5. Limitations and Workarounds
- "Cell sizes vary by latitude due to Earth's curvature"
- "Native Geohash is not adaptive to data density"
- "Companies use adaptive precision strategies in implementation"

#### 6. When NOT to Use Geohash
- "Fixed grid doesn't work well for highly uneven data distribution"
- "Consider Quadtree for truly adaptive spatial indexing"
- "Use actual spatial databases (PostGIS) for complex spatial queries"

### Sample Interview Responses

**Q: How would you implement nearby search for Yelp?**

**A:** "I'd use Geohash for efficient proximity search:

1. **Storage**: Store each business with its geohash in PostgreSQL
2. **Indexing**: Create index on geohash column for fast prefix searches  
3. **Search Strategy**: 
   - Calculate user's geohash and neighboring cells (9 total)
   - Query businesses in those cells using indexed prefix search
   - Filter results by actual distance for precision
   - Sort by distance and apply business logic (ratings, hours, etc.)

4. **Boundary Handling**: Always search neighboring cells to avoid missing nearby businesses
5. **Performance**: This gives us ~100x faster searches compared to calculating distance on entire dataset"

**Q: What are the limitations of Geohash?**

**A:** "Main limitations are:

1. **Boundary Issues**: Neighboring locations might not share prefixes
2. **Fixed Grid**: Not adaptive to data density - rural and urban areas get same detail
3. **Geographic Variation**: Cell sizes vary by latitude, smaller near poles
4. **Solution**: Use neighboring cell searches, adaptive precision strategies, and hybrid approaches with actual distance calculations"

---

## Quick Reference

### Popular Libraries
- **Python**: `geohash2`, `pygeohash`
- **JavaScript**: `ngeohash`, `geohash.js`  
- **Java**: `ch.hsr.geohash`
- **Go**: `github.com/mmcloughlin/geohash`

### PostgreSQL Functions
```sql
-- PostGIS distance functions
ST_Distance(point1::geography, point2::geography)  -- Returns meters
ST_DWithin(point1::geography, point2::geography, distance)  -- Efficient range query
ST_MakePoint(longitude, latitude)  -- Create point from coordinates
```

### Precision Guide
| Characters | Lat Error | Lng Error | Coverage |
|------------|-----------|-----------|----------|
| 1 | ±23 km | ±23 km | Countries |
| 2 | ±2.8 km | ±5.6 km | Large cities |
| 3 | ±0.70 km | ±0.70 km | City districts |
| 4 | ±0.087 km | ±0.18 km | Neighborhoods |
| 5 | ±0.022 km | ±0.022 km | City blocks |
| 6 | ±0.0027 km | ±0.0055 km | Buildings |
| 7 | ±0.00068 km | ±0.00068 km | Rooms |

---

*This document covers all key Geohash concepts for High Level Design interviews. Focus on understanding the trade-offs, real-world applications, and implementation strategies.* 