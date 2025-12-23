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







