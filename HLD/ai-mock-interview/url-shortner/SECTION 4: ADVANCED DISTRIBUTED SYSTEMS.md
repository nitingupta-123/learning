# **SECTION 4: ADVANCED DISTRIBUTED SYSTEMS** 🌐

---

## **4.1 Bloom Filter Deep Dive**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned using a bloom filter to check if URLs are malicious. Can you explain how bloom filters work? How would you size it for 1 billion malicious URLs?"

### ❌ Your Initial Answer

You said: **"All the characters that a URL can have will be the size of bloom filter"**

**The problem:** You confused bloom filter with character sets or hash tables. Bloom filters are probabilistic data structures with specific sizing formulas.

**What was missing:**
- How bloom filters actually work
- Mathematical formulas for sizing
- False positive rate calculations
- When to use vs not use bloom filters

---

### ✅ Complete Answer

#### **What is a Bloom Filter?**

**Definition:**
```
A bloom filter is a space-efficient probabilistic data structure
that tells you:
  - "DEFINITELY NOT in set" (100% accurate)
  - "PROBABLY in set" (might be false positive)

Key properties:
  ✓ Very fast: O(1) lookups
  ✓ Very compact: Much smaller than storing actual items
  ✓ No false negatives: If it says NO, it's definitely NO
  ✗ False positives: If it says YES, might be wrong
  ✗ Can't delete items (standard bloom filter)
```

---

#### **How Bloom Filters Work:**

**Analogy:**

```
Think of a bloom filter like a bouncer at a club:

Bouncer has a checklist with 1000 checkboxes (not names!)

When someone tries to enter:
  1. Bouncer applies 3 rules to person's name
  2. Each rule points to a checkbox number
  3. If ALL checkboxes are checked → "You're on the list" (probably)
  4. If ANY checkbox is unchecked → "You're NOT on the list" (definitely)

False positive: Someone whose name happens to match the same
checkboxes as actual guests, even though they're not invited.
```

---

### 🎯 URL Shortener Context

#### **Structure of a Bloom Filter:**

```
Components:

1. Bit array of size m
   Example: m = 1000 bits
   [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ...]
   
2. k hash functions
   Example: k = 3
   hash1(), hash2(), hash3()

3. Add operation:
   To add "evil.com":
     i1 = hash1("evil.com") % 1000 = 234
     i2 = hash2("evil.com") % 1000 = 567
     i3 = hash3("evil.com") % 1000 = 891
     
     Set bits 234, 567, 891 to 1:
     [0, 0, ..., 1, ..., 1, ..., 1, ...]
           ↑234    ↑567    ↑891

4. Check operation:
   To check "evil.com":
     i1 = hash1("evil.com") % 1000 = 234
     i2 = hash2("evil.com") % 1000 = 567
     i3 = hash3("evil.com") % 1000 = 891
     
     Check bits 234, 567, 891:
       bit[234] = 1 ✓
       bit[567] = 1 ✓
       bit[891] = 1 ✓
       
     Result: "PROBABLY in set" ✓
   
   To check "good.com" (not added):
     i1 = hash1("good.com") % 1000 = 123
     i2 = hash2("good.com") % 1000 = 456
     i3 = hash3("good.com") % 1000 = 789
     
     Check bits 123, 456, 789:
       bit[123] = 0 ✗
       
     Result: "DEFINITELY NOT in set" ✓
```

---

#### **Why False Positives Happen:**

```
Scenario:

Step 1: Add "evil.com"
  hash1("evil.com") = 100 → bit[100] = 1
  hash2("evil.com") = 200 → bit[200] = 1
  hash3("evil.com") = 300 → bit[300] = 1

Step 2: Add "bad.com"
  hash1("bad.com") = 150 → bit[150] = 1
  hash2("bad.com") = 250 → bit[250] = 1
  hash3("bad.com") = 300 → bit[300] = 1 (already set!)

Step 3: Add "malware.com"
  hash1("malware.com") = 100 → bit[100] = 1 (already set!)
  hash2("malware.com") = 250 → bit[250] = 1 (already set!)
  hash3("malware.com") = 350 → bit[350] = 1

Step 4: Check "innocent.com" (never added!)
  hash1("innocent.com") = 100 → bit[100] = 1 ✓
  hash2("innocent.com") = 250 → bit[250] = 1 ✓
  hash3("innocent.com") = 300 → bit[300] = 1 ✓
  
  Result: "PROBABLY in set" ← FALSE POSITIVE! ❌
  
Reason: bits 100, 250, 300 were set by OTHER items,
but happen to match "innocent.com"'s hash values
```

---

#### **Bloom Filter Math:**

**Key formulas:**

```
Given:
  n = number of items to store (e.g., 1 billion malicious URLs)
  p = desired false positive rate (e.g., 1% = 0.01)

Calculate:
  m = optimal number of bits
  k = optimal number of hash functions

Formulas:
  m = -(n × ln(p)) / (ln(2))²
  k = (m/n) × ln(2)

Where:
  ln = natural logarithm
  ln(2) ≈ 0.693
```

---

#### **Sizing for 1 Billion Malicious URLs:**

**Calculation:**

```
Given:
  n = 1,000,000,000 URLs (1 billion)
  p = 0.01 (1% false positive rate)

Step 1: Calculate m (number of bits)
  m = -(n × ln(p)) / (ln(2))²
  m = -(10⁹ × ln(0.01)) / (0.693)²
  m = -(10⁹ × -4.605) / 0.480
  m = 4,605,000,000 / 0.480
  m ≈ 9,600,000,000 bits
  m ≈ 1.2 GB

Step 2: Calculate k (number of hash functions)
  k = (m/n) × ln(2)
  k = (9.6 × 10⁹ / 10⁹) × 0.693
  k = 9.6 × 0.693
  k ≈ 6.65
  k = 7 hash functions (round up)

Answer:
  Bloom filter size: 1.2 GB
  Hash functions needed: 7
  False positive rate: 1%
```

---

**Different false positive rates:**

```
For n = 1 billion URLs:

p = 0.1 (10% false positive):
  m = 4.8 billion bits = 0.6 GB
  k = 3 hash functions

p = 0.01 (1% false positive):
  m = 9.6 billion bits = 1.2 GB
  k = 7 hash functions

p = 0.001 (0.1% false positive):
  m = 14.4 billion bits = 1.8 GB
  k = 10 hash functions

Trade-off:
  Lower false positive rate → More memory needed
  10× better accuracy → ~50% more memory
```

---

#### **Implementation for URL Shortener:**

**Use case: Pre-check malicious URLs before calling Google Safe Browsing API**

**Architecture:**

```
Request flow:

User creates URL: "https://suspicious.com"
     ↓
Step 1: Check bloom filter (1ms)
  bloom_filter.might_contain("suspicious.com")
     ↓
     ├─── FALSE → URL definitely safe ✓
     │             Return immediately, no API call
     │             (95% of URLs take this path)
     │
     └─── TRUE → URL MIGHT be malicious ⚠
                  Proceed to Step 2

Step 2: Call Google Safe Browsing API (50ms)
  google_api.check("suspicious.com")
     ↓
     ├─── Safe → Allow URL creation
     │
     └─── Malicious → Reject URL creation
```

**Benefit:**

```
Without bloom filter:
  Every URL creation → Google API call (50ms)
  Cost: 100 million API calls/month × $0.0001 = $10,000/month
  Latency: +50ms per create

With bloom filter:
  95% of URLs → Bloom filter only (1ms)
  5% of URLs → Bloom filter + API call (51ms)
  Cost: 5 million API calls/month × $0.0001 = $500/month
  Latency: +1ms average (mostly bloom filter)
  
Savings: $9,500/month + better UX ✓
```

---

**Redis implementation:**

```
Redis supports bloom filters via RedisBloom module

Initialize:
  BF.RESERVE malicious_urls 0.01 1000000000
  
  Arguments:
    - Key: "malicious_urls"
    - Error rate: 0.01 (1%)
    - Capacity: 1 billion items

Add malicious URL:
  BF.ADD malicious_urls "evil.com"
  
Check URL:
  result = BF.EXISTS malicious_urls "suspicious.com"
  
  Returns:
    0 → Definitely NOT malicious
    1 → Probably malicious (need to verify)

Bulk operations:
  BF.MADD malicious_urls "evil1.com" "evil2.com" "evil3.com"
  BF.MEXISTS malicious_urls "url1.com" "url2.com" "url3.com"
```

---

**Updating the bloom filter:**

```
Problem: Bloom filters can't delete items!

If "previously-bad.com" is now safe, you can't remove it.

Solutions:

Option 1: Rebuild periodically
  - Once per week, create new bloom filter
  - Load latest malicious URL list from source
  - Swap old filter for new filter
  - Old filter discarded

  Schedule:
    Sunday 2 AM:
      1. Download latest malicious URL database
      2. Build new bloom filter (10 minutes)
      3. Upload to Redis
      4. Atomic swap: BF.RESERVE malicious_urls_new ...
         Then: RENAME malicious_urls_new malicious_urls

Option 2: Counting bloom filter
  - Each bit is actually a counter (not 0/1)
  - Can increment on add, decrement on delete
  - More memory (4 bytes per counter vs 1 bit)
  
  Memory: 1.2 GB × 32 = 38.4 GB (too large!)
  Verdict: Not practical for 1 billion items

Option 3: Time-based expiry
  - Multiple bloom filters, one per week
  - malicious_urls_week_48
  - malicious_urls_week_49
  - Check all recent weeks
  - Drop old weeks automatically
  
  Memory: 1.2 GB × 4 weeks = 4.8 GB
  Verdict: Reasonable for rolling window
```

---

#### **When to Use Bloom Filters:**

**✓ Good use cases:**

```
1. Cache miss prevention (URL shortener)
   - Check bloom filter before expensive cache lookup
   - "Is this item definitely NOT in cache?"

2. Malicious URL detection (our use case)
   - Pre-filter before expensive API call
   - 95% of lookups avoided

3. Duplicate detection
   - "Have we seen this URL before?"
   - Fast negative confirmation

4. Database query optimization
   - "Does this key exist in table?"
   - Skip query if bloom filter says NO

5. Spell checking
   - "Is this word definitely not in dictionary?"
   - Fast rejection of typos
```

---

**✗ Bad use cases:**

```
1. Exact membership testing
   - Need to know for certain if item is in set
   - False positives not acceptable
   - Use: Hash table instead

2. Need to delete items frequently
   - Bloom filters don't support deletion
   - Use: Hash table with expiry

3. Very small datasets
   - Bloom filter overhead not worth it
   - Just use hash table (fast enough)

4. Need to retrieve stored data
   - Bloom filter only says YES/NO
   - Doesn't return the actual data
   - Use: Hash table or database
```

---

#### **Bloom Filter vs Alternatives:**

```
Comparison for "Check if URL is malicious":

Option A: Hash Table (in Redis)
  Memory: 1 billion URLs × 100 bytes = 100 GB
  Lookup: O(1), 1ms
  False positives: 0%
  Can delete: Yes
  Cost: $300/month (Redis memory)

Option B: Bloom Filter
  Memory: 1.2 GB
  Lookup: O(1), 0.5ms
  False positives: 1%
  Can delete: No (rebuild weekly)
  Cost: $15/month (Redis memory)
  
Option C: Database
  Memory: 100 GB (on disk)
  Lookup: O(log n), 10-50ms
  False positives: 0%
  Can delete: Yes
  Cost: $50/month (storage)
  
Option D: External API only
  Memory: 0 GB
  Lookup: 50-100ms
  False positives: 0%
  Can delete: N/A
  Cost: $10,000/month (API calls)

Best choice: Bloom Filter (Option B)
  - 83× less memory than hash table
  - 20× cheaper than hash table
  - 50× faster than database
  - 200× cheaper than API only
  - 1% false positive rate acceptable (just means extra API calls)
```

---

### 📊 Key Takeaways

1. **Bloom filters are probabilistic:** Can say "definitely NO" but only "probably YES"

2. **Size formula:** m = -(n × ln(p)) / (ln(2))² 
   - For 1B items at 1% FP: 1.2 GB

3. **Hash functions:** k = (m/n) × ln(2)
   - For above: 7 hash functions

4. **Trade-off:** Lower false positive rate = more memory (but not linear)

5. **Can't delete:** Standard bloom filters are append-only

6. **Perfect for pre-filtering:** Avoid expensive operations (API calls, DB queries)

7. **Redis support:** RedisBloom module provides production-ready implementation

---

### 🔗 Further Reading

- **"Bloom Filters by Example":** Interactive visualization
- **Redis documentation:** RedisBloom module
- **Paper:** "Network Applications of Bloom Filters: A Survey"

---

### ✏️ Practice Exercise

**Scenario:** Your URL shortener has these requirements:

```
- 100 million URLs created per month
- Need to detect duplicates BEFORE inserting to DB
- Current approach: SELECT COUNT(*) WHERE long_url = ? (50ms query)
- 40 URLs/sec × 50ms = blocking 2 seconds worth of requests

Questions:
1. Should you use a bloom filter for duplicate detection?
2. What size bloom filter do you need?
3. What's the downside of false positives in this case?
4. What's a better alternative?
```

<details>
<summary>Click to see answer</summary>

**1. Should you use bloom filter for duplicate detection?**

**Analysis:**

```
Use case: Check if long_url exists before creating short URL

Bloom filter behavior:
  - FALSE (definitely NOT duplicate) → Safe to create new URL ✓
  - TRUE (probably duplicate) → Need to check database to confirm
  
If false positive (1% rate):
  - Bloom filter says "probably duplicate"
  - Query database to confirm
  - Find: Actually NOT a duplicate
  - Result: Wasted one DB query (50ms)
  
Impact:
  - 99% of URLs: Bloom filter only (0.5ms) ✓
  - 1% of URLs: Bloom filter + DB query (50.5ms)
  - Average: 0.99 × 0.5ms + 0.01 × 50.5ms = 1ms
  
Current without bloom filter:
  - 100% of URLs: DB query (50ms)
  
Improvement: 50ms → 1ms (50× faster!) ✓✓✓
```

**Verdict: YES, use bloom filter!**

---

**2. What size bloom filter?**

```
Given:
  n = 100 million URLs/month
  Keep last 12 months = 1.2 billion URLs
  p = 0.01 (1% false positive)

Calculate:
  m = -(1.2 × 10⁹ × ln(0.01)) / (ln(2))²
  m = -(1.2 × 10⁹ × -4.605) / 0.480
  m ≈ 11.5 billion bits
  m ≈ 1.44 GB

  k = (11.5 × 10⁹ / 1.2 × 10⁹) × 0.693
  k ≈ 6.6
  k = 7 hash functions

Answer:
  Bloom filter size: 1.44 GB
  Hash functions: 7
  Monthly cost: ~$20 (Redis memory)
```

---

**3. What's the downside of false positives?**

```
False positive scenario:

User creates URL: "https://example.com/new-page-12345"
     ↓
Bloom filter check → TRUE (probably exists)
     ↓
Query database: SELECT id FROM urls WHERE long_url = ?
     ↓
Result: NOT FOUND (false positive!)
     ↓
Create new URL as normal

Downside:
  - Wasted 1 DB query (50ms)
  - User still gets their short URL
  - No user-facing impact! ✓

At 1% false positive rate:
  - 100M URLs/month × 1% = 1M wasted queries
  - 1M × 50ms = 50,000 seconds = 14 hours of DB time
  - DB can handle this (not a problem)

Verdict: Acceptable! ✓
```

---

**4. What's a better alternative?**

**Even better solution: Cache + Bloom Filter**

```
Architecture:

Layer 1: Bloom Filter (1.44 GB in Redis)
  Check: BF.EXISTS urls "https://example.com/page"
  
  If FALSE → Definitely doesn't exist, create new URL
  If TRUE → Might exist, check Layer 2

Layer 2: Cache of recent URLs (10 GB in Redis)
  Check: GET "url:hash:abc123def456"
  
  If HIT → URL exists, return existing short code
  If MISS → Probably doesn't exist, check Layer 3

Layer 3: Database
  Query: SELECT id FROM urls WHERE long_url = ?
  
  If FOUND → URL exists, cache it, return short code
  If NOT FOUND → Create new URL

Performance:
  95% of URLs: Layer 1 only (0.5ms) ← New URLs
  4% of URLs: Layer 1 + Layer 2 (1.5ms) ← Recent duplicates
  1% of URLs: Layer 1 + Layer 2 + Layer 3 (52ms) ← Old duplicates / FP
  
Average latency: 0.95 × 0.5 + 0.04 × 1.5 + 0.01 × 52 = 1.1ms

vs Current (no bloom filter): 50ms
Improvement: 45× faster! ✓✓✓

Memory:
  Bloom filter: 1.44 GB
  Cache (10M recent URLs): 10 GB
  Total: 11.44 GB = $150/month

Cost-benefit:
  - $150/month for 45× performance improvement
  - DB load reduced by 99%
  - ROI: Excellent ✓
```

---

**Implementation:**

```
Pseudocode:

FUNCTION create_short_url(long_url):
    url_hash = md5(long_url)
    
    // Layer 1: Bloom filter
    might_exist = bloom_filter.check(url_hash)
    
    IF NOT might_exist:
        // Definitely new URL
        id = generate_snowflake_id()
        db.insert(id, long_url)
        cache.set("url:hash:" + url_hash, id)
        bloom_filter.add(url_hash)
        RETURN encode_base62(id)
    
    // Layer 2: Cache
    cached_id = cache.get("url:hash:" + url_hash)
    
    IF cached_id:
        // Recent duplicate
        RETURN encode_base62(cached_id)
    
    // Layer 3: Database
    existing_id = db.query("SELECT id WHERE long_url = ?", long_url)
    
    IF existing_id:
        // Old duplicate
        cache.set("url:hash:" + url_hash, existing_id)
        RETURN encode_base62(existing_id)
    
    // False positive from bloom filter
    id = generate_snowflake_id()
    db.insert(id, long_url)
    cache.set("url:hash:" + url_hash, id)
    bloom_filter.add(url_hash)  // Already added, but idempotent
    RETURN encode_base62(id)
```

---

**Summary:**

| Approach | Avg Latency | DB Load | Memory | Cost/Month |
|----------|-------------|---------|--------|------------|
| Current (DB only) | 50ms | 100% | 0 | $0 |
| Bloom filter only | 1ms | 1% | 1.44 GB | $20 |
| Bloom + Cache | 1.1ms | 1% | 11.44 GB | $150 |

**Recommendation: Bloom + Cache** ✓
- Best performance (45× improvement)
- Lowest DB load (99% reduction)
- Reasonable cost ($150/month)
- Handles both new URLs (bloom) and duplicate URLs (cache)

</details>

---

**Great progress! That was the most mathematical topic. The rest of Section 4 will be more architectural.**

**Ready for 4.2 Redis High Availability?** 🚀

# **Continuing Section 4...** 📝

---

## **4.2 Redis High Availability (Sentinel vs Cluster)**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned using Redis for caching and rate limiting. What happens if Redis goes down? How do you ensure Redis is highly available?"

### ❌ Your Initial Answer

You said: **"Keep multiple instances of Redis (cluster) and use ZooKeeper to manage them"**

**The problem:** 
- Redis does NOT use ZooKeeper
- Redis has its own built-in solutions: **Sentinel** and **Cluster**
- You confused Redis with other systems (like Kafka, which does use ZooKeeper)

**What was missing:**
- Redis Sentinel architecture
- Redis Cluster architecture
- When to use Sentinel vs Cluster
- Failover mechanisms

---

### ✅ Complete Answer

#### **Redis HA Solutions Overview:**

```
Redis has TWO built-in high availability solutions:

1. Redis Sentinel
   Purpose: Automatic failover for master-slave replication
   Use case: Single logical Redis instance with HA
   Best for: <100 GB data, simple setup

2. Redis Cluster  
   Purpose: Horizontal scaling + HA
   Use case: Multiple Redis instances, data partitioned
   Best for: >100 GB data, need to scale writes

Neither uses ZooKeeper! ✓
Both have their own coordination mechanisms.
```

---

### 🎯 URL Shortener Context

#### **Redis Sentinel (Recommended for URL Shortener):**

**What is Sentinel?**

```
Sentinel = Monitoring + Automatic Failover for Redis

Architecture:

              ┌──────────────┐
              │  Sentinel 1  │
              └──────┬───────┘
                     │ monitors
       ┌─────────────┼─────────────┐
       ↓             ↓             ↓
┌──────────┐  ┌──────────┐  ┌──────────┐
│  Master  │→→│ Replica 1│  │ Replica 2│
│  Redis   │  │  Redis   │  │  Redis   │
└──────────┘  └──────────┘  └──────────┘
       ↑             ↑             ↑
       └─────────────┼─────────────┘
                     │ monitors
              ┌──────┴───────┐
              │  Sentinel 2  │
              └──────────────┘
              ┌──────────────┐
              │  Sentinel 3  │
              └──────────────┘

Components:
- 1 Master Redis (accepts reads + writes)
- 2 Replica Redis (accepts reads only, sync from master)
- 3 Sentinel processes (monitor and coordinate failover)

Note: Always use ODD number of Sentinels (3, 5, 7) for quorum
```

---

**How Sentinel Works:**

**Normal Operation:**

```
Application → Sentinel → "Who is the master?"
Sentinel → "Master is at 192.168.1.10:6379"
Application → Master Redis (read/write)
Application → Replica Redis (read only, optional)

All Sentinels continuously:
  - Ping Master every 1 second
  - Ping Replicas every 1 second
  - Exchange info with other Sentinels
  - "Is master still alive?" "Yes" "Yes" "Yes"
```

---

**Failure Detection & Failover:**

```
Timeline of Master Failure:

14:00:00 - Master Redis crashes (server dies)

14:00:01 - Sentinel 1 fails to ping master
           Status: "Master might be down"

14:00:02 - Sentinel 1 asks Sentinel 2: "Is master down?"
           Sentinel 2: "Yes, I can't reach it either"

14:00:03 - Sentinel 1 asks Sentinel 3: "Is master down?"
           Sentinel 3: "Yes, confirmed"

14:00:04 - QUORUM REACHED (3 of 3 Sentinels agree)
           Status: "Master is definitely down"

14:00:05 - Sentinel 1 (leader) initiates failover
           Decision: Promote Replica 1 to new master

14:00:06 - Sentinel 1 sends command to Replica 1:
           SLAVEOF NO ONE (become master)

14:00:07 - Replica 1 becomes new master
           Starts accepting writes

14:00:08 - Sentinel 1 reconfigures Replica 2:
           SLAVEOF 192.168.1.11:6379 (new master)

14:00:09 - Sentinel 1 notifies applications:
           "New master is at 192.168.1.11:6379"

14:00:10 - Applications reconnect to new master
           Service restored! ✓

Total downtime: 10 seconds
```

---

**Configuration Files:**

**Redis Master (redis-master.conf):**

```conf
# Basic config
port 6379
bind 0.0.0.0

# Replication
replica-announce-ip 192.168.1.10
replica-announce-port 6379

# Persistence (optional, for cache usually disabled)
save ""
appendonly no

# Memory
maxmemory 4gb
maxmemory-policy allkeys-lru

# Performance
tcp-backlog 511
timeout 0
```

---

**Redis Replica (redis-replica.conf):**

```conf
# Basic config
port 6379
bind 0.0.0.0

# Replication - point to current master
replicaof 192.168.1.10 6379

# Read-only mode (replicas don't accept writes)
replica-read-only yes

# Same memory/performance settings as master
maxmemory 4gb
maxmemory-policy allkeys-lru
```

---

**Sentinel Configuration (sentinel.conf):**

```conf
# Sentinel port
port 26379

# Monitor master
# Format: sentinel monitor <name> <ip> <port> <quorum>
sentinel monitor mymaster 192.168.1.10 6379 2

# Quorum = 2 means:
#   "Need 2 Sentinels to agree master is down before failover"
#   With 3 Sentinels, need 2/3 majority

# How long to wait before declaring master down (milliseconds)
sentinel down-after-milliseconds mymaster 5000
# (5 seconds without response = considered down)

# How long failover can take before giving up
sentinel failover-timeout mymaster 180000
# (3 minutes)

# How many replicas can sync from new master simultaneously
sentinel parallel-syncs mymaster 1
# (1 = cautious, sync one at a time to avoid overwhelming new master)

# Authentication (if Redis has password)
# sentinel auth-pass mymaster YourRedisPassword
```

---

**Application Configuration (Python example):**

```python
from redis.sentinel import Sentinel

# Connect to Sentinels (not directly to Redis!)
sentinel = Sentinel([
    ('sentinel1.internal', 26379),
    ('sentinel2.internal', 26379),
    ('sentinel3.internal', 26379)
], socket_timeout=0.1)

# Get master connection (for writes)
master = sentinel.master_for(
    'mymaster',
    socket_timeout=0.1,
    password='YourRedisPassword'
)

# Get slave connection (for reads)
slave = sentinel.slave_for(
    'mymaster',
    socket_timeout=0.1,
    password='YourRedisPassword'
)

# Usage
# Writes go to master
master.set('url:123', 'https://google.com')

# Reads can go to slave (reduce master load)
url = slave.get('url:123')

# If master fails:
#   - Sentinel automatically promotes replica
#   - master.set() automatically reconnects to new master
#   - No code changes needed! ✓
```

---

**Key Benefits of Sentinel:**

```
✓ Automatic failover (10-30 seconds downtime)
✓ Monitoring built-in (health checks, notifications)
✓ Client libraries handle reconnection automatically
✓ Simple setup (just add Sentinel processes)
✓ No data partitioning (all data on master)
✓ Good for <100 GB datasets

✗ Single master bottleneck (can't scale writes horizontally)
✗ All data must fit on single master node
✗ Failover causes brief downtime (10-30 sec)
```

---

#### **Redis Cluster (For Massive Scale):**

**What is Redis Cluster?**

```
Cluster = Horizontal Scaling + HA

Architecture:

Data partitioned across 3 masters:

Master 1 (slots 0-5460)      Master 2 (slots 5461-10922)   Master 3 (slots 10923-16383)
    ↓                              ↓                              ↓
Replica 1                      Replica 2                      Replica 3

Hash slots: 16,384 total
  - Each key hashed to a slot number (0-16383)
  - Each master owns a range of slots
  - Data distributed across masters

Example:
  Key "url:123" → hash → slot 5000 → Master 1
  Key "url:456" → hash → slot 8000 → Master 2
  Key "url:789" → hash → slot 15000 → Master 3
```

---

**How Cluster Works:**

**Write Operation:**

```
Application → SET url:123 "https://google.com"
     ↓
Master 1: Hash("url:123") = slot 5000
Master 1: "I own slots 0-5460, so I handle this"
Master 1: Writes to local storage
Master 1: Replicates to Replica 1
Master 1: Returns OK

If client had connected to wrong master:
Application → SET url:456 "value"
     ↓
Master 1: Hash("url:456") = slot 8000
Master 1: "I DON'T own this slot"
Master 1: Returns: MOVED 8000 192.168.1.11:6379
     ↓
Application → Redirects to Master 2
Master 2: Writes data
Master 2: Returns OK
```

---

**Failover in Cluster:**

```
Scenario: Master 2 dies

14:00:00 - Master 2 crashes

14:00:05 - Other masters detect Master 2 down
           (no PING response for 5 seconds)

14:00:06 - Cluster initiates automatic failover
           Promotes Replica 2 to new Master 2

14:00:07 - Replica 2 becomes master for slots 5461-10922
           Starts accepting writes

14:00:08 - Cluster updates routing table
           All nodes know: slots 5461-10922 now at new Master 2

14:00:10 - Service restored
           Applications redirect to new Master 2

Total downtime: 10 seconds (for that shard only!)
Other shards (Master 1, Master 3) never went down ✓
```

---

**Cluster Configuration:**

```bash
# Create 6 Redis instances (3 masters + 3 replicas)

# Master 1
redis-server --port 7000 --cluster-enabled yes --cluster-config-file nodes-7000.conf

# Master 2
redis-server --port 7001 --cluster-enabled yes --cluster-config-file nodes-7001.conf

# Master 3
redis-server --port 7002 --cluster-enabled yes --cluster-config-file nodes-7002.conf

# Replica 1
redis-server --port 7003 --cluster-enabled yes --cluster-config-file nodes-7003.conf

# Replica 2
redis-server --port 7004 --cluster-enabled yes --cluster-config-file nodes-7004.conf

# Replica 3
redis-server --port 7005 --cluster-enabled yes --cluster-config-file nodes-7005.conf

# Create cluster
redis-cli --cluster create \
  192.168.1.10:7000 \
  192.168.1.11:7001 \
  192.168.1.12:7002 \
  192.168.1.13:7003 \
  192.168.1.14:7004 \
  192.168.1.15:7005 \
  --cluster-replicas 1
```

---

**Key Benefits of Cluster:**

```
✓ Horizontal scaling (add more masters = more capacity)
✓ Can handle >100 GB (distribute across nodes)
✓ Can scale writes (each master handles portion of writes)
✓ Partial availability (if 1 master dies, others still work)

✗ More complex setup and operations
✗ Multi-key operations limited (MGET, transactions)
✗ Can't use all Redis features (Lua scripts with multiple keys)
✗ Resharding is complex (moving slots between masters)
```

---

#### **Sentinel vs Cluster Comparison:**

```
┌─────────────────────┬─────────────────────┬─────────────────────┐
│      Feature        │   Redis Sentinel    │   Redis Cluster     │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Purpose             │ HA for single       │ Horizontal scaling  │
│                     │ master-replica      │ + HA                │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Data Distribution   │ All data on master  │ Partitioned across  │
│                     │                     │ masters (sharding)  │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Write Scaling       │ ✗ Single master     │ ✓ Multiple masters  │
│                     │ bottleneck          │ (distributed)       │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Max Data Size       │ ~64 GB (single      │ Hundreds of TB      │
│                     │ machine memory)     │ (distributed)       │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Failover Time       │ 10-30 seconds       │ 5-10 seconds        │
│                     │ (full downtime)     │ (partial downtime)  │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Setup Complexity    │ ✓ Simple            │ ✗ Complex           │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Client Complexity   │ ✓ Simple            │ ⚠ Moderate          │
│                     │ (Sentinel-aware)    │ (Cluster-aware)     │
├─────────────────────┼─────────────────────┼─────────────────────┤
│ Multi-key Ops       │ ✓ Full support.     |                     |





# **SECTION 4: ADVANCED DISTRIBUTED SYSTEMS (DETAILED)** 🌐

---

## **4.3 Database Sharding Strategy**

### 🤔 Interview Question/Context

**Interviewer:** "Your database will grow to billions of URLs. How do you shard it? Why did you choose consistent hashing?"

### ❌ Your Initial Answer

You said: **"Use consistent hashing"**

**The problem:** Consistent hashing is ONE option, but not optimal for time-ordered IDs like Snowflake.

---

### ✅ Complete Answer

#### **Why Sharding is Needed:**

```
Single Database Limits:

PostgreSQL single instance:
  - Max connections: ~500-1000
  - Max storage: ~1-2 TB (practical limit)
  - Max throughput: ~10,000 writes/sec

Our growth:
  - Year 1: 1.2 billion URLs (500 GB)
  - Year 3: 3.6 billion URLs (1.5 TB) ← hitting limits
  - Year 5: 6 billion URLs (2.5 TB) ← MUST shard

Solution: Split data across multiple database instances (shards)
```

---

### 🎯 URL Shortener Context

#### **Three Sharding Strategies:**

### **Strategy 1: Range-Based Sharding (RECOMMENDED)** ✅

**How it works:**

```
Partition by ID range:

Shard 1: IDs 0 - 1,000,000,000
Shard 2: IDs 1,000,000,001 - 2,000,000,000
Shard 3: IDs 2,000,000,001 - 3,000,000,000
Shard 4: IDs 3,000,000,001 - 4,000,000,000

Routing logic:
  if id <= 1_000_000_000:
      route to Shard 1
  elif id <= 2_000_000_000:
      route to Shard 2
  elif id <= 3_000_000_000:
      route to Shard 3
  else:
      route to Shard 4
```

**Example queries:**

```
Query: Get URL with ID 500,000,000
  500,000,000 <= 1B → Route to Shard 1
  
Query: Get URL with ID 1,500,000,000
  1,500,000,000 > 1B AND <= 2B → Route to Shard 2
  
Query: Get URL with ID 3,200,000,000
  3,200,000,000 > 3B → Route to Shard 4
```

---

**Pros:**

```
✓ Simple routing logic (just compare ID ranges)

✓ Time locality preserved
  - Recent URLs are together (same shard)
  - Useful for analytics: "URLs created this month"
  - Query: "SELECT * FROM urls WHERE created_at > '2025-11-01'"
    → Only hits newest shard(s)

✓ Easy to add new shards
  - Just add new range: Shard 5 for IDs 4B-5B
  - No data migration needed
  - Old shards untouched

✓ Range queries work
  - "Get URLs created between Jan-March 2025"
  - Since IDs are time-ordered (Snowflake), can query specific shards
```

**Cons:**

```
✗ Hotspot problem (write skew)
  
  All new URLs go to newest shard:
  
  Time: Nov 2025
  Shard 1: 1B URLs (full, no new writes)
  Shard 2: 1B URLs (full, no new writes)
  Shard 3: 1B URLs (full, no new writes)
  Shard 4: Currently at 200M URLs ← ALL writes here!
  
  Shard 4 is a hotspot - receives 100% of writes ❌
```

---

**Solution to Hotspot:**

```
Use Snowflake ID structure to distribute writes:

Snowflake ID: [timestamp][datacenter_id][worker_id][sequence]

Routing formula:
  shard_id = (datacenter_id × 32 + worker_id) % num_shards

Example with 4 shards:
  
  Datacenter 1, Worker 1:
    shard_id = (1 × 32 + 1) % 4 = 33 % 4 = 1 → Shard 1
  
  Datacenter 1, Worker 2:
    shard_id = (1 × 32 + 2) % 4 = 34 % 4 = 2 → Shard 2
  
  Datacenter 1, Worker 3:
    shard_id = (1 × 32 + 3) % 4 = 35 % 4 = 3 → Shard 3
  
  Datacenter 1, Worker 4:
    shard_id = (1 × 32 + 4) % 4 = 36 % 4 = 0 → Shard 4
  
  Datacenter 2, Worker 1:
    shard_id = (2 × 32 + 1) % 4 = 65 % 4 = 1 → Shard 1

Result: Writes distributed evenly across all shards! ✓
```

**Distribution example:**

```
Setup: 10 service instances, 4 shards

Instance 1 (dc=1, worker=1) → Shard 1 (25% of writes)
Instance 2 (dc=1, worker=2) → Shard 2 (25% of writes)
Instance 3 (dc=1, worker=3) → Shard 3 (25% of writes)
Instance 4 (dc=1, worker=4) → Shard 4 (25% of writes)
Instance 5 (dc=1, worker=5) → Shard 1 (25% of writes)
Instance 6 (dc=1, worker=6) → Shard 2 (25% of writes)
Instance 7 (dc=1, worker=7) → Shard 3 (25% of writes)
Instance 8 (dc=1, worker=8) → Shard 4 (25% of writes)
Instance 9 (dc=2, worker=1) → Shard 1 (25% of writes)
Instance 10 (dc=2, worker=2) → Shard 2 (25% of writes)

All shards receive roughly equal writes! ✓
Time locality still preserved within each shard! ✓
```

---

**Read routing:**

```
User clicks short URL: tiny.url/aBc123

Step 1: Decode base62 → ID: 1,234,567,890

Step 2: Calculate shard
  Extract from ID: datacenter_id = 1, worker_id = 5
  shard_id = (1 × 32 + 5) % 4 = 37 % 4 = 1

Step 3: Query Shard 1
  SELECT long_url FROM url_mappings WHERE id = 1234567890
  
Step 4: Return long_url, redirect user

This ensures reads go to correct shard ✓
```

---

### **Strategy 2: Hash-Based Sharding**

**How it works:**

```
Partition by hash of ID:

shard_id = hash(id) % num_shards

Example with 4 shards:
  hash(123) % 4 = 3 → Shard 3
  hash(456) % 4 = 0 → Shard 0
  hash(789) % 4 = 1 → Shard 1
  hash(999) % 4 = 3 → Shard 3

Data distributed randomly across shards based on hash function
```

---

**Pros:**

```
✓ Even distribution
  - Hash function ensures uniform distribution
  - No hotspots
  - Each shard receives ~25% of data (with 4 shards)

✓ Simple implementation
  - Just hash and modulo
  - No special routing logic
```

**Cons:**

```
✗ No range queries
  - "Get URLs created in January" requires querying ALL shards
  - No time locality
  - Every query must fan out to all shards

✗ Adding shards is expensive (resharding)
  
  Problem:
    Change from 4 shards to 5 shards
    
    Old: hash(id) % 4
    New: hash(id) % 5
    
    Example:
      ID 123: hash(123) % 4 = 3 (Shard 3)
              hash(123) % 5 = 3 (Shard 3) ✓ same
      
      ID 456: hash(456) % 4 = 0 (Shard 0)
              hash(456) % 5 = 1 (Shard 1) ✗ different!
    
    Result: Most data needs to move to different shards!
    
  Resharding process:
    1. Provision Shard 5
    2. For each ID in all shards:
       - Calculate new shard: hash(id) % 5
       - If different from current shard, move data
    3. ~80% of data moves (expensive!)
    
  Downtime: Hours or days for billions of records
```

---

**When to use:**

```
✓ Random IDs (not time-ordered)
✓ Don't need range queries
✓ Stable number of shards (rarely add new ones)

Example: User database sharded by user_id hash
  - Users created randomly over time
  - No time locality needed
  - Query by user_id only (no range queries)
```

---

### **Strategy 3: Consistent Hashing**

**How it works:**

```
Hash ring: 0 to 2^32-1 (circular)

Step 1: Place servers on ring
  hash("Server A") = 100 → position 100 on ring
  hash("Server B") = 500 → position 500 on ring
  hash("Server C") = 900 → position 900 on ring

Step 2: Place keys on ring
  hash(key) → position → go clockwise to next server

Example:
  hash("url:123") = 250 → Clockwise → Server B (position 500)
  hash("url:456") = 700 → Clockwise → Server C (position 900)
  hash("url:789") = 50  → Clockwise → Server A (position 100)

Visual:
          0/2^32
           ↑
   900 ←   C   → 100
   C       |       A
           |
   500 ←   B

Keys 0-99: → Server A
Keys 100-499: → Server B
Keys 500-899: → Server C
Keys 900-2^32: → Server A
```

---

**Adding a server:**

```
Add Server D at position 300:

Before:
  Keys 100-499 → Server B (400 keys)

After:
  Keys 100-299 → Server B (200 keys)
  Keys 300-499 → Server D (200 keys)

Data movement:
  Only 200 keys move (from B to D)
  Other servers (A, C) unaffected ✓

Compare to hash-based:
  Hash-based: 80% of data moves
  Consistent hashing: Only ~25% of data moves (1/n where n = new num servers)
```

---

**Virtual nodes (for better balance):**

```
Problem: Servers not evenly distributed on ring

Physical placement:
  Server A: position 100   (owns 0-99)
  Server B: position 500   (owns 100-499) ← 400 keys
  Server C: position 900   (owns 500-899) ← 400 keys
  Server A wraps: 900-2^32 (owns 900-2^32) ← Many keys
  
  Imbalanced! ❌

Solution: Virtual nodes
  Each physical server gets 100 positions on ring
  
  Server A: positions 100, 250, 380, 500, ... (100 positions)
  Server B: positions 150, 300, 420, 600, ... (100 positions)
  Server C: positions 200, 350, 480, 700, ... (100 positions)
  
  Keys distributed evenly across 300 virtual nodes
  Each physical server owns ~33% of keys ✓
```

---

**Pros:**

```
✓ Adding/removing servers is cheap
  - Only K/n keys move (K = total keys, n = num servers)
  - With hash-based: ~80% of keys move
  - With consistent hashing: ~25% move

✓ Gradual rebalancing
  - Move data in background
  - Service stays available

✓ Good for dynamic environments
  - Cloud auto-scaling
  - Nodes frequently join/leave
```

**Cons:**

```
✗ More complex to implement
  - Hash ring management
  - Virtual nodes
  - Replication logic

✗ Not optimal for time-ordered data
  - No time locality
  - Range queries don't work

✗ More complex routing
  - Need to maintain ring state
  - All nodes must agree on ring topology
```

---

**When to use:**

```
✓ Distributed caches (Memcached, Redis Cluster)
  - Nodes frequently join/leave
  - No time locality needed
  - Key-value lookups only

✓ Distributed databases with dynamic scaling
  - Cassandra, DynamoDB
  - Auto-scaling based on load

✗ NOT for URL shortener
  - We have time-ordered Snowflake IDs
  - Range-based is simpler and better
```

---

#### **Comparison Table:**

```
┌────────────────────┬──────────────┬──────────────┬────────────────┐
│     Feature        │ Range-Based  │ Hash-Based   │ Consistent     │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Write             │ ⚠ Hotspot    │ ✓ Even       │ ✓ Even         │
│ distribution      │ (need fix)   │              │                │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Fix for hotspot   │ ✓ Yes        │ N/A          │ N/A            │
│                   │ (use dc+     │              │                │
│                   │ worker_id)   │              │                │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Range queries     │ ✓ Efficient  │ ✗ Must       │ ✗ Must         │
│                   │              │ fan out      │ fan out        │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Time locality     │ ✓ Preserved  │ ✗ Lost       │ ✗ Lost         │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Add shard cost    │ ✓ Low        │ ✗ High       │ ✓ Medium       │
│                   │ (no data     │ (~80% data   │ (~25% data     │
│                   │ movement)    │ moves)       │ moves)         │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Routing           │ ✓ Simple     │ ✓ Simple     │ ✗ Complex      │
│ complexity        │              │              │                │
├────────────────────┼──────────────┼──────────────┼────────────────┤
│ Best for          │ Time-ordered │ Random IDs   │ Dynamic        │
│                   │ IDs          │              │ caches         │
└────────────────────┴──────────────┴──────────────┴────────────────┘

For URL shortener with Snowflake IDs: Range-Based ✓
```

---

#### **Implementation Details:**

**Sharding proxy/router:**

```
Architecture:

[Application]
      ↓
[Sharding Router] ← Knows which shard has which IDs
      ↓
   ┌──┴────┬────────┬────────┐
   ↓       ↓        ↓        ↓
[Shard 1][Shard 2][Shard 3][Shard 4]

Router logic:
  def get_shard(id):
      # Extract datacenter and worker from Snowflake ID
      datacenter_id = (id >> 17) & 0x1F  # Bits 17-21
      worker_id = (id >> 12) & 0x1F      # Bits 12-16
      
      # Calculate shard
      shard_id = (datacenter_id * 32 + worker_id) % num_shards
      
      return shards[shard_id]
  
  def query_url(id):
      shard = get_shard(id)
      return shard.query("SELECT * FROM url_mappings WHERE id = ?", id)
```

---

**Handling shard unavailability:**

```
Scenario: Shard 2 is down

Option 1: Fail fast
  - Return error to user
  - "Service temporarily unavailable"
  - Simple but bad UX

Option 2: Read from replica
  - Each shard has replicas: Shard 2A, Shard 2B
  - If 2A down, route to 2B
  - High availability ✓

Option 3: Retry with circuit breaker
  - Try Shard 2
  - If fails, mark as down for 60 seconds
  - Route to replica
  - After 60 seconds, try Shard 2 again

Recommended: Option 2 + Option 3 combined
```

---

**Sharding configuration:**

```yaml
# sharding_config.yaml

shards:
  - id: 1
    master:
      host: shard1-master.db.internal
      port: 5432
    replicas:
      - host: shard1-replica1.db.internal
        port: 5432
      - host: shard1-replica2.db.internal
        port: 5432
  
  - id: 2
    master:
      host: shard2-master.db.internal
      port: 5432
    replicas:
      - host: shard2-replica1.db.internal
        port: 5432
      - host: shard2-replica2.db.internal
        port: 5432
  
  - id: 3
    master:
      host: shard3-master.db.internal
      port: 5432
    replicas:
      - host: shard3-replica1.db.internal
        port: 5432
      - host: shard3-replica2.db.internal
        port: 5432
  
  - id: 4
    master:
      host: shard4-master.db.internal
      port: 5432
    replicas:
      - host: shard4-replica1.db.internal
        port: 5432
      - host: shard4-replica2.db.internal
        port: 5432

routing:
  strategy: snowflake_modulo
  num_shards: 4
```

---

### 📊 Key Takeaways

1. **Range-based sharding is best** for Snowflake IDs (time-ordered data)

2. **Hotspot fix:** Use `(datacenter_id × 32 + worker_id) % num_shards` to distribute writes evenly

3. **Time locality preserved:** Recent URLs on same shard, good for analytics

4. **Easy to add shards:** Just add new range, no data migration

5. **Hash-based:** Good for random IDs, but resharding is expensive

6. **Consistent hashing:** Good for distributed caches (Memcached), not for time-ordered data

7. **For URL shortener:** Range-based with hotspot fix is optimal

---

### 🔗 Further Reading

- **"Designing Data-Intensive Applications":** Chapter 6 (Partitioning)
- **Instagram Engineering Blog:** How they shard billions of photos
- **Discord Engineering Blog:** How they shard messages

---

### ✏️ Practice Exercise

**Scenario:** You have 4 shards using range-based sharding:
- Shard 1: IDs 0-1B
- Shard 2: IDs 1B-2B
- Shard 3: IDs 2B-3B
- Shard 4: IDs 3B-4B

Your system is growing and you need to add Shard 5.

**Questions:**
1. How do you add Shard 5 without downtime?
2. Do you need to move any existing data?
3. What's the routing logic after adding Shard 5?
4. How does this compare to adding a shard with hash-based sharding?

<details>
<summary>Click to see answer</summary>

**1. How to add Shard 5 without downtime:**

```
Step-by-step process:

Step 1: Provision Shard 5
  - Create new database instance: shard5-master
  - Create replicas: shard5-replica1, shard5-replica2
  - Set up replication between master and replicas
  - Duration: 30 minutes

Step 2: Update routing configuration
  - Add Shard 5 to config: IDs 4B-5B
  - Deploy updated config to all application instances
  - Use rolling deployment (no downtime)
  - Duration: 10 minutes

Step 3: Verify
  - Create test URL (should go to Shard 5 if ID > 4B)
  - Monitor Shard 5 for incoming writes
  - Check that Shards 1-4 unchanged

Step 4: Monitor
  - Track write distribution across all 5 shards
  - Verify Shard 5 receiving expected traffic
  
Total time: 40 minutes
Downtime: 0 seconds ✓
```

---

**2. Do you need to move existing data?**

```
NO! ✓

Current state:
  Shard 1: IDs 0-1B (1B URLs, full)
  Shard 2: IDs 1B-2B (1B URLs, full)
  Shard 3: IDs 2B-3B (1B URLs, full)
  Shard 4: IDs 3B-4B (currently 500M URLs, growing)

After adding Shard 5:
  Shard 1: IDs 0-1B (unchanged)
  Shard 2: IDs 1B-2B (unchanged)
  Shard 3: IDs 2B-3B (unchanged)
  Shard 4: IDs 3B-4B (unchanged)
  Shard 5: IDs 4B-5B (new URLs only)

All existing data stays in place!
Only NEW URLs (with IDs > 4B) go to Shard 5.

This is the huge advantage of range-based sharding! ✓
```

---

**3. Routing logic after adding Shard 5:**

```
Updated routing function:

def get_shard(id):
    # Extract datacenter and worker from Snowflake ID
    datacenter_id = (id >> 17) & 0x1F
    worker_id = (id >> 12) & 0x1F
    
    # Calculate base shard
    shard_id = (datacenter_id * 32 + worker_id) % 5  # Changed from % 4 to % 5
    
    return shards[shard_id]

Distribution with 5 shards:
  DC 1, Worker 1: (1*32 + 1) % 5 = 33 % 5 = 3 → Shard 3
  DC 1, Worker 2: (1*32 + 2) % 5 = 34 % 5 = 4 → Shard 4
  DC 1, Worker 3: (1*32 + 3) % 5 = 35 % 5 = 0 → Shard 0
  DC 1, Worker 4: (1*32 + 4) % 5 = 36 % 5 = 1 → Shard 1
  DC 1, Worker 5: (1*32 + 5) % 5 = 37 % 5 = 2 → Shard 2

All 5 shards receive writes evenly! ✓
```

---

**4. Comparison with hash-based sharding:**

```
Hash-Based Sharding:

Step 1: Provision Shard 5 (30 min)
  Same as range-based

Step 2: Calculate new hash distribution
  Old: hash(id) % 4
  New: hash(id) % 5
  
  Impact analysis:
    ID 123: hash(123) % 4 = 3, hash(123) % 5 = 3 (same shard) ✓
    ID 456: hash(456) % 4 = 0, hash(456) % 5 = 1 (different!) ✗
    ID 789: hash(789) % 4 = 1, hash(789) % 5 = 4 (different!) ✗
    
  ~80% of IDs map to different shards!

Step 3: Migrate data (CRITICAL DIFFERENCE)
  For each of 4 billion URLs:
    1. Read from old shard
    2. Calculate new shard: hash(id) % 5
    3. If different, write to new shard
    4. Delete from old shard (after verification)
  
  Data to move: ~3.2 billion URLs (80%)
  At 10,000 URLs/sec: 3,200,000,000 / 10,000 = 320,000 seconds = 89 hours!
  
Step 4: Update routing (10 min)
  Can only update after ALL data migrated

Step 5: Cleanup old data (10 hours)

Total time: ~100 hours (4+ days)
Downtime options:
  Option A: Full downtime (4 days) ❌
  Option B: Dual-write mode (complex, risky)
  Option C: Read-only mode during migration (poor UX)

Comparison:

┌─────────────────┬──────────────────┬──────────────────┐
│                 │  Range-Based     │  Hash-Based      │
├─────────────────┼──────────────────┼──────────────────┤
│ Data movement   │  0 URLs          │  3.2B URLs       │
│                 │  (0%)            │  (80%)           │
├─────────────────┼──────────────────┼──────────────────┤
│ Time to add     │  40 minutes      │  100 hours       │
│ shard           │                  │  (4+ days)       │
├─────────────────┼──────────────────┼──────────────────┤
│ Downtime        │  0 seconds       │  Hours to days   │
├─────────────────┼──────────────────┼──────────────────┤
│ Risk            │  Very low        │  High (data      │
│                 │                  │  corruption      │
│                 │                  │  possible)       │
├─────────────────┼──────────────────┼──────────────────┤
│ Complexity      │  Simple config   │  Complex data    │
│                 │  update          │  migration       │
└─────────────────┴──────────────────┴──────────────────┘

Verdict: Range-based sharding is FAR superior for adding shards! ✓
```

---

**Key insight:**

Range-based sharding with time-ordered IDs is perfect for append-heavy workloads like URL shortener. New data naturally goes to new shards without touching old data.

Hash-based sharding forces expensive reshuffling of existing data, making it impractical for large datasets.

</details>

---

**Continuing to 4.4...**

