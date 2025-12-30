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

```



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

# **Continuing Section 4.3 → 4.4** 📝

---

## **4.4 Distributed Locks for Custom URLs**

### 🤔 Interview Question/Context

**Interviewer:** "Two users simultaneously try to create the custom URL 'google'. How do you prevent both from succeeding?"

### ❌ Your Initial Answer

You didn't consider the race condition in distributed systems with multiple service instances.

---

### ✅ Complete Answer

#### **The Problem: Race Condition**

**Scenario:**

```
Time: 14:30:00.000

User A (in California) → Server 1 (US-EAST)
User B (in New York)   → Server 2 (US-EAST)

Both want custom slug: "google"
```

**What happens without proper synchronization:**

```
Timeline:

14:30:00.000 - User A clicks "Create" with slug "google"
14:30:00.000 - User B clicks "Create" with slug "google"

14:30:00.100 - Server 1 receives request
               Checks DB: "Is 'google' taken?"
               Query: SELECT id FROM url_mappings WHERE custom_slug = 'google'
               Result: No rows (available!)

14:30:00.105 - Server 2 receives request
               Checks DB: "Is 'google' taken?"
               Query: SELECT id FROM url_mappings WHERE custom_slug = 'google'
               Result: No rows (available!)
               
               ← RACE CONDITION! Both servers think "google" is available

14:30:00.200 - Server 1 inserts:
               INSERT INTO url_mappings (id, custom_slug, long_url)
               VALUES (100, 'google', 'https://userA.com')
               Result: Success! ✓

14:30:00.210 - Server 2 inserts:
               INSERT INTO url_mappings (id, custom_slug, long_url)
               VALUES (101, 'google', 'https://userB.com')
               Result: ???

Outcome depends on database:
  - With UNIQUE constraint: Server 2's insert FAILS (constraint violation)
  - Without UNIQUE constraint: BOTH inserts succeed ❌
    → Database now has TWO rows with slug "google"
    → Undefined behavior when someone visits tiny.url/google
```

---

### 🎯 URL Shortener Context

#### **Solution 1: Database UNIQUE Constraint (SIMPLE)** ✅

**The easiest solution:**

```sql
CREATE TABLE url_mappings (
    id BIGINT PRIMARY KEY,
    long_url VARCHAR(2048) NOT NULL,
    custom_slug VARCHAR(50) UNIQUE,  -- Database enforces uniqueness
    user_id BIGINT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- The UNIQUE constraint guarantees only ONE row can have a given slug
```

**How it works:**

```
Timeline with UNIQUE constraint:

14:30:00.000 - User A and User B both click "Create" with "google"

14:30:00.100 - Server 1 checks: "google" available ✓

14:30:00.105 - Server 2 checks: "google" available ✓

14:30:00.200 - Server 1 inserts:
               INSERT INTO url_mappings (id, custom_slug, long_url, user_id)
               VALUES (100, 'google', 'https://userA.com', 1)
               
               Database accepts insert ✓
               Internally, database creates index entry: 'google' → row 100

14:30:00.210 - Server 2 inserts:
               INSERT INTO url_mappings (id, custom_slug, long_url, user_id)
               VALUES (101, 'google', 'https://userB.com', 2)
               
               Database checks unique constraint:
                 - Index already has 'google' → row 100
                 - UNIQUE constraint violation!
               
               Database REJECTS insert with error:
               ERROR: duplicate key value violates unique constraint "url_mappings_custom_slug_key"
               DETAIL: Key (custom_slug)=(google) already exists.

14:30:00.211 - Server 2 catches error, returns to User B:
               HTTP 409 Conflict
               {
                 "error": "SLUG_TAKEN",
                 "message": "Custom slug 'google' is already taken",
                 "suggestion": "Try: google-1, google-2, google-ny"
               }

Result: Only User A got "google" ✓
        User B notified it's taken ✓
        No duplicate slugs in database ✓
```

---

**Implementation:**

```python
def create_custom_url(long_url, custom_slug, user_id):
    # Validate slug format
    if not is_valid_slug(custom_slug):
        return 400, {"error": "Invalid slug format"}
    
    # Generate Snowflake ID
    url_id = generate_snowflake_id()
    
    # Try to insert
    try:
        db.execute("""
            INSERT INTO url_mappings (id, long_url, custom_slug, user_id, created_at)
            VALUES (?, ?, ?, ?, NOW())
        """, url_id, long_url, custom_slug, user_id)
        
        # Success!
        return 201, {
            "shortUrl": f"https://tiny.url/{custom_slug}",
            "longUrl": long_url
        }
        
    except UniqueConstraintViolation as e:
        # Slug already taken
        # Suggest alternatives
        suggestions = generate_suggestions(custom_slug)
        
        return 409, {
            "error": "SLUG_TAKEN",
            "message": f"Custom slug '{custom_slug}' is already taken",
            "suggestions": suggestions
        }
    
    except Exception as e:
        # Other database error
        log.error(f"Database error: {e}")
        return 500, {"error": "Internal server error"}
```

**Generating suggestions:**

```python
def generate_suggestions(slug):
    """Generate alternative slug suggestions"""
    suggestions = []
    
    # Try with numbers
    for i in range(1, 6):
        candidate = f"{slug}-{i}"
        if is_slug_available(candidate):
            suggestions.append(candidate)
            if len(suggestions) >= 3:
                break
    
    # Try with year
    year = datetime.now().year
    candidate = f"{slug}-{year}"
    if is_slug_available(candidate):
        suggestions.append(candidate)
    
    # Try with random suffix
    import random
    random_suffix = ''.join(random.choices('abcdefghijklmnopqrstuvwxyz', k=3))
    candidate = f"{slug}-{random_suffix}"
    if is_slug_available(candidate):
        suggestions.append(candidate)
    
    return suggestions[:3]  # Return top 3 suggestions

# Example output:
# generate_suggestions("google")
# → ["google-1", "google-2025", "google-xyz"]
```

---

**Pros of UNIQUE Constraint:**

```
✓ Simple - just add UNIQUE to schema
✓ Database guarantees - can't be bypassed by application bugs
✓ No additional infrastructure needed (no Redis locks)
✓ Works perfectly for our use case
✓ Atomic - database handles race condition automatically
✓ Low latency - no network round-trip to lock service
```

**Cons:**

```
✗ Can't do complex validation before insert
  Example: If you want to check profanity, reserve premium slugs,
  or apply custom business logic BEFORE inserting
  
  With UNIQUE constraint alone:
    1. Insert attempt
    2. Fails if taken
    3. Can't run custom logic between check and insert
```

---

#### **Solution 2: Distributed Lock with Redis (COMPLEX)** ⚠️

**When you need it:**

```
Use distributed locks if you need to:
  1. Run complex validation BEFORE inserting
  2. Check multiple conditions atomically
  3. Reserve resources across multiple tables
  4. Implement "soft" reservations (hold slug for 5 minutes while user fills form)

Example scenario:
  User starts creating URL with "google" slug
  → Show "google is available (reserved for you for 5 minutes)"
  → User fills in long URL, description, etc.
  → After 5 minutes, release reservation if not completed
  
This requires distributed locks because:
  - Need to "hold" slug across multiple requests
  - UNIQUE constraint alone can't do temporary reservations
```

---

**How distributed locks work:**

```
Concept: Only ONE server can hold the lock for a given slug at a time

Lock properties:
  1. Exclusive - only one holder at a time
  2. Timeout - automatically released after N seconds (prevents deadlock)
  3. Atomic acquire - guaranteed no race condition
  4. Token-based release - only lock owner can release
```

---

**Redis-based lock implementation:**

```python
import redis
import uuid

redis_client = redis.Redis(host='redis.internal', port=6379)

def create_custom_url_with_lock(long_url, custom_slug, user_id):
    lock_key = f"lock:slug:{custom_slug}"
    lock_value = str(uuid.uuid4())  # Unique token for this lock
    lock_timeout = 5  # seconds
    
    # Step 1: Try to acquire lock
    acquired = redis_client.set(
        lock_key,
        lock_value,
        nx=True,     # Only set if key doesn't exist (atomic)
        ex=lock_timeout  # Expire after 5 seconds (auto-release)
    )
    
    if not acquired:
        # Someone else is creating this slug right now
        return 429, {
            "error": "SLUG_LOCKED",
            "message": "Someone is currently creating this slug, try again in a moment"
        }
    
    try:
        # Step 2: Inside lock - do validation
        
        # Check profanity
        if contains_profanity(custom_slug):
            return 400, {"error": "Slug contains inappropriate content"}
        
        # Check if it's a premium slug (requires payment)
        if is_premium_slug(custom_slug) and not user_is_premium(user_id):
            return 403, {"error": "This is a premium slug. Upgrade to use it."}
        
        # Check database (even though we have lock, verify it's available)
        existing = db.query(
            "SELECT id FROM url_mappings WHERE custom_slug = ?",
            custom_slug
        )
        
        if existing:
            # Taken (shouldn't happen if lock is working, but defensive check)
            return 409, {"error": "Slug already taken"}
        
        # Step 3: All checks passed, insert
        url_id = generate_snowflake_id()
        
        db.execute("""
            INSERT INTO url_mappings (id, long_url, custom_slug, user_id, created_at)
            VALUES (?, ?, ?, ?, NOW())
        """, url_id, long_url, custom_slug, user_id)
        
        # Step 4: Success!
        return 201, {
            "shortUrl": f"https://tiny.url/{custom_slug}",
            "longUrl": long_url
        }
        
    finally:
        # Step 5: Release lock (IMPORTANT!)
        # Only release if we still own it (check token matches)
        release_lock(lock_key, lock_value)


def release_lock(lock_key, lock_value):
    """Safely release lock only if we own it"""
    
    # Lua script for atomic check-and-delete
    lua_script = """
    if redis.call('get', KEYS[1]) == ARGV[1] then
        return redis.call('del', KEYS[1])
    else
        return 0
    end
    """
    
    # Execute Lua script atomically
    redis_client.eval(lua_script, 1, lock_key, lock_value)
    
    # Why Lua script?
    # Without Lua:
    #   1. GET lock_key
    #   2. Compare with lock_value
    #   3. If match, DELETE lock_key
    # Problem: Steps 1-3 not atomic! Another process could acquire lock between step 2 and 3
    #
    # With Lua:
    #   All steps execute atomically on Redis server
    #   No race condition possible ✓
```

---

**Timeline with distributed lock:**

```
14:30:00.000 - User A and User B both request "google"

14:30:00.100 - Server 1: Try acquire lock("google")
               Redis: SET lock:slug:google "uuid-1" NX EX 5
               Redis: OK (lock acquired) ✓
               
               Server 1 now holds lock for 5 seconds

14:30:00.105 - Server 2: Try acquire lock("google")
               Redis: SET lock:slug:google "uuid-2" NX EX 5
               Redis: nil (key already exists, lock NOT acquired) ✗
               
               Server 2 returns: 429 "Someone is creating this slug"

14:30:00.200 - Server 1: Inside lock
               - Check profanity ✓
               - Check premium status ✓
               - Check database ✓
               - Insert to database ✓

14:30:00.250 - Server 1: Release lock
               Redis: DEL lock:slug:google (if owner matches)
               Lock released ✓

Result: Only Server 1 succeeded ✓
        Server 2 was blocked by lock ✓
        User B told to try again ✓
```

---

**Handling lock timeout (edge case):**

```
Scenario: Server crashes while holding lock

14:30:00.000 - Server 1 acquires lock("google") with 5 second timeout

14:30:00.500 - Server 1 crashes! (power failure, network issue, etc.)
               Lock NOT released (finally block didn't execute)

Without timeout:
  Lock held forever ❌
  "google" slug permanently locked
  No one can ever create it

With timeout (5 seconds):
  14:30:05.000 - Redis automatically deletes lock key (expired)
  14:30:05.001 - Server 2 can now acquire lock ✓
  
  Deadlock prevented! ✓

Lesson: Always set expiration on locks to handle crashes
```

---

**Pros of Distributed Locks:**

```
✓ Enables complex validation logic before insert
✓ Can implement "soft" reservations (hold for N minutes)
✓ Multiple checks can be atomic
✓ Prevents wasted database operations
```

**Cons:**

```
✗ More complex code (acquire, try-finally, release)
✗ Additional infrastructure (Redis for locks)
✗ Higher latency (+2-5ms for lock acquire/release)
✗ Potential deadlocks if not careful with timeouts
✗ Harder to debug (distributed systems issues)
```

---

#### **Solution 3: Optimistic Locking (Alternative)**

**Concept:**

```
Don't lock upfront. Instead:
  1. Read current state (with version number)
  2. Do processing
  3. Try to update only if version hasn't changed

Similar to how version control (Git) handles conflicts
```

**Implementation:**

```sql
-- Add version column
ALTER TABLE url_mappings ADD COLUMN version INT DEFAULT 1;

-- Reserve slug (optimistic)
UPDATE url_mappings
SET version = version + 1, reserved_by = ?, reserved_at = NOW()
WHERE custom_slug = ? AND reserved_by IS NULL AND version = ?

-- If 0 rows updated → someone else reserved it first (conflict)
-- If 1 row updated → we successfully reserved it
```

**This is less common for URL shortener but useful in other scenarios.**

---

### 📊 Key Takeaways

1. **Race conditions are real** in distributed systems with multiple service instances

2. **UNIQUE constraint is simplest** solution for custom URLs - use this! ✓

3. **Distributed locks needed** only for complex multi-step validation

4. **Always use lock timeouts** to prevent deadlocks (5-10 seconds typical)

5. **Lua scripts in Redis** ensure atomic check-and-release of locks

6. **For URL shortener:** UNIQUE constraint sufficient for 95% of cases

7. **Add locks later** only if business requirements demand complex validation

---

### 🔗 Further Reading

- **Redis documentation:** Distributed locks with Redlock algorithm
- **"Designing Data-Intensive Applications":** Chapter on distributed systems guarantees
- **Martin Kleppmann:** "How to do distributed locking" (blog post critique of Redlock)

---

### ✏️ Practice Exercise

**Scenario:** Your product team wants a new feature:

"Premium users can reserve a custom slug for 10 minutes while they decide on the long URL. After 10 minutes, if they haven't completed, release the slug for others."

**Questions:**
1. Can you implement this with just a UNIQUE constraint?
2. If not, what additional mechanism do you need?
3. How do you clean up expired reservations?
4. What happens if a user reserves "google", doesn't complete, and someone else tries "google" after 5 minutes?

<details>
<summary>Click to see answer</summary>

**1. Can you implement with just UNIQUE constraint?**

```
NO ✗

UNIQUE constraint only prevents duplicate final slugs.
It can't handle temporary "soft" reservations.

Problem with UNIQUE alone:
  - Can't INSERT incomplete URLs (missing long_url)
  - Can't partially fill a row then complete later
  - Can't automatically release after timeout

Need additional mechanism for reservations.
```

---

**2. What mechanism do you need?**

**Option A: Separate reservations table**

```sql
CREATE TABLE slug_reservations (
    custom_slug VARCHAR(50) PRIMARY KEY,
    user_id BIGINT NOT NULL,
    reserved_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    INDEX idx_expires_at (expires_at)
);

Flow:
  1. User clicks "Reserve 'google'"
  2. INSERT INTO slug_reservations:
       slug='google', user_id=123, expires_at=NOW() + 10 minutes
  3. User has 10 minutes to complete
  4. When completing:
     - Check reservation still valid (expires_at > NOW AND user_id matches)
     - If valid: INSERT into url_mappings, DELETE from slug_reservations
     - If expired: Return error "Reservation expired"

Cleanup job (runs every minute):
  DELETE FROM slug_reservations WHERE expires_at < NOW()
```

---

**Option B: Redis-based reservations**

```python
def reserve_slug(custom_slug, user_id):
    reservation_key = f"reservation:slug:{custom_slug}"
    reservation_data = {
        "user_id": user_id,
        "reserved_at": time.time()
    }
    
    # Try to reserve for 10 minutes (600 seconds)
    success = redis_client.set(
        reservation_key,
        json.dumps(reservation_data),
        nx=True,    # Only if not exists
        ex=600      # Expire after 10 minutes
    )
    
    if success:
        return 200, {
            "message": "Slug reserved for 10 minutes",
            "expires_at": time.time() + 600
        }
    else:
        # Check if someone else reserved it
        existing = redis_client.get(reservation_key)
        if existing:
            data = json.loads(existing)
            return 409, {
                "error": "Slug reserved by another user",
                "expires_in_seconds": redis_client.ttl(reservation_key)
            }

def complete_reservation(custom_slug, long_url, user_id):
    reservation_key = f"reservation:slug:{custom_slug}"
    
    # Check reservation
    reservation = redis_client.get(reservation_key)
    if not reservation:
        return 400, {"error": "No reservation found or reservation expired"}
    
    data = json.loads(reservation)
    if data["user_id"] != user_id:
        return 403, {"error": "This slug is reserved by another user"}
    
    # Reservation valid! Create URL
    url_id = generate_snowflake_id()
    
    try:
        db.execute("""
            INSERT INTO url_mappings (id, long_url, custom_slug, user_id)
            VALUES (?, ?, ?, ?)
        """, url_id, long_url, custom_slug, user_id)
        
        # Delete reservation
        redis_client.delete(reservation_key)
        
        return 201, {"shortUrl": f"https://tiny.url/{custom_slug}"}
        
    except UniqueConstraintViolation:
        # Shouldn't happen (we had reservation), but defensive
        return 409, {"error": "Slug taken"}
```

**Pros of Redis approach:**
- ✓ Automatic expiration (Redis TTL handles cleanup)
- ✓ Fast (in-memory)
- ✓ No cleanup job needed

**Pros of database approach:**
- ✓ Persistent (survives Redis crash)
- ✓ Can query reservations (analytics, debugging)
- ✓ ACID guarantees

**Recommended: Redis for simplicity** ✓

---

**3. Cleanup of expired reservations:**

**Redis approach:**
```
No cleanup needed! ✓
Redis automatically deletes keys after TTL expires (600 seconds)

Monitoring:
  - Track: reservation:* key count
  - Alert if > 10,000 (possible abuse)
```

**Database approach:**
```
Cron job (runs every minute):

#!/bin/bash
# cleanup-expired-reservations.sh

psql -h db-master -U app -d urlshortener -c "
  DELETE FROM slug_reservations 
  WHERE expires_at < NOW()
"

Metrics to track:
  - Reservations deleted per run
  - Average reservation age
  - Completion rate (reservations that become actual URLs)

Dashboard:
  Total reservations: 523
  Completed: 312 (60%)
  Expired: 211 (40%)
  → Indicates 40% of users abandon after reserving
  → Consider shorter timeout (5 min instead of 10 min)
```

---

**4. Scenario: User A reserves, User B tries after 5 minutes**

```
Timeline:

14:00:00 - User A reserves "google"
           Redis: SET reservation:slug:google {"user_id": 123, ...} EX 600
           Response: "Reserved for 10 minutes"

14:05:00 - User B tries to reserve "google" (5 minutes later)
           Redis: SET reservation:slug:google {"user_id": 456, ...} NX EX 600
           Redis: nil (key exists, reservation active)
           
           Check remaining time:
           Redis: TTL reservation:slug:google
           Redis: 300 (5 minutes = 300 seconds remaining)
           
           Response to User B:
           HTTP 409 Conflict
           {
             "error": "SLUG_RESERVED",
             "message": "This slug is reserved by another user",
             "expires_in_seconds": 300,
             "suggestion": "Try again in 5 minutes, or try: google-1, google-ny"
           }

14:10:00 - Redis automatically deletes reservation key (TTL expired)

14:10:01 - User B tries again
           Redis: SET reservation:slug:google {"user_id": 456, ...} NX EX 600
           Redis: OK (key doesn't exist, reservation succeeds) ✓
           
           Response: "Reserved for 10 minutes"

Result: User A had priority for 10 minutes
        After expiration, User B could reserve
        No conflicts, fair ordering ✓
```

---

**Edge case: What if User A completes AFTER expiration?**

```
14:00:00 - User A reserves "google" (expires at 14:10:00)
14:10:00 - Reservation expires automatically
14:10:01 - User B reserves "google"
14:11:00 - User A finally completes (11 minutes later, SLOW!)

User A's completion attempt:

def complete_reservation(custom_slug, long_url, user_id):
    reservation_key = f"reservation:slug:{custom_slug}"
    
    reservation = redis_client.get(reservation_key)
    if not reservation:
        # Reservation not found (expired!)
        return 400, {
            "error": "RESERVATION_EXPIRED",
            "message": "Your reservation expired after 10 minutes",
            "suggestion": "Try reserving again if the slug is still available"
        }
    
    # Continue only if reservation valid...

Response to User A:
  HTTP 400 Bad Request
  "Your reservation expired. Slug may have been taken by someone else."

User A must reserve again (if still available)
This is fair - they had 10 minutes ✓
```

---

**Summary:**

Feature requires:
1. ✓ Redis-based reservations (simple, automatic expiration)
2. ✓ TTL of 600 seconds (10 minutes)
3. ✓ Check reservation ownership before completing
4. ✓ No cleanup needed (Redis handles it)
5. ✓ Handle edge case of expired reservations gracefully

Cost: Minimal (few thousand reservations = <1 MB Redis memory)
Complexity: Medium (more complex than simple UNIQUE constraint, but manageable)

</details>

---

# **SECTION 4.5: SNOWFLAKE ID GENERATION** ❄️

---

## **4.5 Snowflake ID Generation Details**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned using Snowflake IDs. Can you explain how they work and why they're better than database auto-increment?"

---

### ✅ Complete Answer

#### **What is Snowflake ID?**

```
A 64-bit unique identifier that can be generated independently 
by multiple servers without coordination.

Invented by Twitter for their distributed systems.

Key property: Time-ordered (newer IDs numerically greater than older IDs)
```

---

#### **64-Bit Structure Breakdown:**

```
┌───────────────────────────────────────────────────────────┐
│                    64 bits total                          │
├─────────────────┬──────────┬──────────┬──────────────────┤
│  41 bits        │ 10 bits  │ 10 bits  │    12 bits       │
│  Timestamp      │Datacenter│ Worker   │   Sequence       │
└─────────────────┴──────────┴──────────┴──────────────────┘

Example ID: 1234567890123456789

Breakdown:
  Timestamp (41 bits): 
    - Milliseconds since custom epoch (e.g., Jan 1, 2020)
    - Supports 69 years of IDs (2^41 milliseconds)
  
  Datacenter ID (10 bits):
    - Identifies which datacenter generated the ID
    - Supports 1024 datacenters (2^10)
  
  Worker ID (10 bits):
    - Identifies which server/process in datacenter
    - Supports 1024 workers per datacenter (2^10)
  
  Sequence (12 bits):
    - Counter within same millisecond
    - Supports 4096 IDs per millisecond per worker (2^12)
```

---

#### **How It Works:**

```
Each server independently generates IDs:

Server configuration:
  datacenter_id = 1
  worker_id = 5
  custom_epoch = 1609459200000  // Jan 1, 2020 in milliseconds

ID generation algorithm:
  1. Get current timestamp: now() - custom_epoch
     Example: 1638360000 milliseconds since epoch
  
  2. If same millisecond as last ID:
     - Increment sequence: 0 → 1 → 2 → 3...
     - If sequence reaches 4096 → wait for next millisecond
  
  3. If new millisecond:
     - Reset sequence to 0
  
  4. Build 64-bit ID:
     ID = (timestamp << 22) | (datacenter_id << 12) | (worker_id << 2) | sequence
  
  5. Return ID

Example:
  Timestamp: 1638360000 (41 bits)
  Datacenter: 1 (10 bits)
  Worker: 5 (10 bits)
  Sequence: 123 (12 bits)
  
  Final ID: 6891234567890123
```

---

#### **Why Time-Ordered IDs Matter:**

```
Benefit 1: Database Index Efficiency
  Sequential IDs → efficient B-tree inserts
  Random IDs → B-tree rebalancing → slower

Benefit 2: Sharding by Time
  Shard 1: IDs 0-1B (Jan-Jun 2024)
  Shard 2: IDs 1B-2B (Jul-Dec 2024)
  Shard 3: IDs 2B-3B (Jan-Jun 2025)
  
  Recent URLs naturally group together

Benefit 3: Sorting
  ORDER BY id = ORDER BY created_at
  No separate timestamp column needed for sorting

Benefit 4: Analytics
  "URLs created last week" → simple ID range query
  SELECT * FROM urls WHERE id BETWEEN X AND Y
```

---

### 🎯 URL Shortener Context

#### **Worker ID Assignment Strategies:**

**Option 1: Environment Variable (Simple)**

```bash
# Docker/Kubernetes configuration
apiVersion: v1
kind: Deployment
metadata:
  name: urlshortener
spec:
  replicas: 10
  template:
    spec:
      containers:
      - name: service
        env:
        - name: DATACENTER_ID
          value: "1"
        - name: WORKER_ID
          value: "5"  # Manually assigned per instance
```

**Pros:**
- ✅ Simple to understand
- ✅ Full control over IDs

**Cons:**
- ❌ Manual assignment required
- ❌ Risk of duplicate IDs if misconfigured

---

**Option 2: Service Discovery (Advanced)**

```
Use ZooKeeper/etcd to auto-assign worker IDs:

Startup sequence:
  1. Service starts
  2. Connects to ZooKeeper
  3. Requests: "Give me unique worker ID for datacenter 1"
  4. ZooKeeper assigns: worker_id = 7 (next available)
  5. Service registers: "I am datacenter=1, worker=7"
  6. On shutdown: Release worker_id=7 for reuse

Benefits:
  ✅ Automatic assignment (no manual config)
  ✅ No duplicate IDs
  ✅ Worker IDs recycled when servers shut down
```

---

**Option 3: Use IP Address (Hacky but works)**

```python
# Extract last octet of IP as worker ID
import socket

def get_worker_id():
    hostname = socket.gethostname()
    ip = socket.gethostbyname(hostname)
    # IP: 192.168.1.42
    last_octet = int(ip.split('.')[-1])  # 42
    worker_id = last_octet % 1024  # Ensure within 10 bits
    return worker_id

# Works if you have <1024 servers and IPs don't repeat
```

**Pros:**
- ✅ Zero configuration

**Cons:**
- ❌ Doesn't work with DHCP
- ❌ Doesn't work with >1024 servers

---

#### **Handling Edge Cases:**

**Case 1: Clock Skew (Clock Goes Backward)**

```
Problem:
  Server clock: 10:00:00
  Generate ID with timestamp: 10:00:00
  
  Clock adjusted backward (NTP sync):
  Server clock: 09:59:50
  
  Next ID would have OLDER timestamp than previous ID!
  Violates time-ordering guarantee ❌

Solution:
  Track last_timestamp
  
  if current_timestamp < last_timestamp:
      // Clock went backward!
      log.error("Clock moved backwards. Refusing to generate IDs.")
      throw ClockBackwardsException
      // Wait for clock to catch up
  
  Alternative (less strict):
      // Wait until clock catches up to last_timestamp
      while current_timestamp <= last_timestamp:
          sleep(1 millisecond)
      
      // Now safe to generate with new timestamp
```

**Prevention:**
- Use NTP with gradual clock adjustment (not jumps)
- Monitor clock drift and alert
- Use monotonic clocks if available

---

**Case 2: Sequence Overflow**

```
Problem:
  In same millisecond, generated 4096 IDs (sequence maxed out)
  Need to generate 4097th ID in same millisecond
  
Solution:
  if sequence >= 4096:
      // Sequence overflow
      // Wait for next millisecond
      while current_timestamp == last_timestamp:
          current_timestamp = get_current_time()
      
      // New millisecond, reset sequence
      sequence = 0
      last_timestamp = current_timestamp
  
  generate_id(current_timestamp, datacenter, worker, sequence)
```

**In practice:**
- 4096 IDs/millisecond = 4 million IDs/second per worker
- We generate 40 IDs/second across all workers
- Sequence overflow NEVER happens at our scale ✓

---

**Case 3: Duplicate Worker IDs**

```
Problem:
  Server 1: datacenter=1, worker=5
  Server 2: datacenter=1, worker=5  (misconfigured!)
  
  Both generate IDs with same datacenter+worker
  → Collision possible! ❌

Detection:
  Monitor: Count unique IDs generated by each server
  If two servers produce same ID → Alert immediately

Prevention:
  - Use service discovery (ZooKeeper auto-assigns)
  - OR: Startup health check queries other servers for their worker IDs
  - Refuse to start if duplicate detected
```

---

#### **Capacity Calculation:**

```
Per worker capacity:
  4096 IDs per millisecond
  × 1000 milliseconds per second
  = 4,096,000 IDs per second per worker

Total system capacity:
  1024 datacenters × 1024 workers × 4M IDs/sec
  = 4.3 trillion IDs per second

Our usage:
  40 IDs per second across entire system
  
Headroom: 4.3 trillion / 40 = 107 billion times our current load ✓

Runway with Snowflake:
  69 years (2^41 milliseconds)
  At 100M URLs/month: 69 years of capacity ✓
```

---

#### **Implementation Pseudocode:**

```python
class SnowflakeIDGenerator:
    def __init__(self, datacenter_id, worker_id, custom_epoch):
        self.datacenter_id = datacenter_id  # 0-1023
        self.worker_id = worker_id          # 0-1023
        self.custom_epoch = custom_epoch    # Milliseconds
        self.sequence = 0
        self.last_timestamp = -1
    
    def generate_id(self):
        timestamp = self._current_timestamp()
        
        # Handle clock going backwards
        if timestamp < self.last_timestamp:
            raise Exception(f"Clock moved backwards. Refusing to generate ID.")
        
        # Same millisecond as last ID
        if timestamp == self.last_timestamp:
            # Increment sequence
            self.sequence = (self.sequence + 1) & 4095  # 12 bits max
            
            # Sequence overflow - wait for next millisecond
            if self.sequence == 0:
                timestamp = self._wait_next_millisecond(self.last_timestamp)
        else:
            # New millisecond - reset sequence
            self.sequence = 0
        
        self.last_timestamp = timestamp
        
        # Build 64-bit ID
        id = ((timestamp - self.custom_epoch) << 22) | \
             (self.datacenter_id << 12) | \
             (self.worker_id << 2) | \
             self.sequence
        
        return id
    
    def _current_timestamp(self):
        return int(time.time() * 1000)  # Milliseconds since epoch
    
    def _wait_next_millisecond(self, last_timestamp):
        timestamp = self._current_timestamp()
        while timestamp <= last_timestamp:
            timestamp = self._current_timestamp()
        return timestamp

# Usage
generator = SnowflakeIDGenerator(
    datacenter_id=1,
    worker_id=5,
    custom_epoch=1609459200000  # Jan 1, 2020
)

url_id = generator.generate_id()
# Example: 6891234567890123
```

---

#### **Decoding Snowflake IDs:**

```python
def decode_snowflake(id, custom_epoch):
    """
    Extract components from Snowflake ID
    """
    # Sequence (last 12 bits)
    sequence = id & 4095
    
    # Worker ID (next 10 bits)
    worker_id = (id >> 12) & 1023
    
    # Datacenter ID (next 10 bits)
    datacenter_id = (id >> 22) & 1023
    
    # Timestamp (first 41 bits)
    timestamp = (id >> 32) + custom_epoch
    created_at = datetime.fromtimestamp(timestamp / 1000)
    
    return {
        'id': id,
        'timestamp': timestamp,
        'created_at': created_at,
        'datacenter': datacenter_id,
        'worker': worker_id,
        'sequence': sequence
    }

# Example
result = decode_snowflake(6891234567890123, 1609459200000)
print(result)
# {
#   'id': 6891234567890123,
#   'timestamp': 1638360000,
#   'created_at': '2021-12-01 10:00:00',
#   'datacenter': 1,
#   'worker': 5,
#   'sequence': 123
# }
```

**Use cases for decoding:**
- Debugging (which server generated this ID?)
- Analytics (when was this URL created?)
- Sharding (route to shard based on datacenter)

---

### **Snowflake vs Auto-Increment Comparison:**

```
┌──────────────────────┬─────────────────┬─────────────────┐
│      Feature         │ Auto-increment  │   Snowflake     │
├──────────────────────┼─────────────────┼─────────────────┤
│ Coordination needed  │ Yes (DB hit)    │ No              │
├──────────────────────┼─────────────────┼─────────────────┤
│ Single point of      │ Yes (DB master) │ No              │
│ failure              │                 │                 │
├──────────────────────┼─────────────────┼─────────────────┤
│ Performance          │ Limited by DB   │ 4M IDs/sec per  │
│                      │ (~10K IDs/sec)  │ worker          │
├──────────────────────┼─────────────────┼─────────────────┤
│ Time-ordered         │ Yes (implicit)  │ Yes (explicit)  │
├──────────────────────┼─────────────────┼─────────────────┤
│ Multi-region         │ Difficult       │ Easy (different │
│                      │ (conflicts)     │ datacenter IDs) │
├──────────────────────┼─────────────────┼─────────────────┤
│ Can generate offline │ No              │ Yes             │
├──────────────────────┼─────────────────┼─────────────────┤
│ Sortable by time     │ No (need        │ Yes (ID itself) │
│                      │ timestamp col)  │                 │
├──────────────────────┼─────────────────┼─────────────────┤
│ Best for             │ Single-region,  │ Distributed,    │
│                      │ low scale       │ high scale      │
└──────────────────────┴─────────────────┴─────────────────┘
```

---

### **Why Snowflake for URL Shortener:**

```
Benefit 1: No DB hit for ID generation
  Auto-increment: Must query DB for next ID
  Snowflake: Generate locally (instant)
  
  Performance: 0.01ms (Snowflake) vs 10ms (DB query)

Benefit 2: Multi-region support
  US-EAST: datacenter_id = 1
  EU-WEST: datacenter_id = 2
  
  No coordination needed between regions
  No ID conflicts possible

Benefit 3: Sharding-friendly
  Shard by ID range (time-ordered)
  Recent URLs naturally group together
  Analytics queries efficient

Benefit 4: High throughput
  Can generate 4M IDs/sec per server
  Our load: 40 IDs/sec
  Headroom: 100,000× ✓

Benefit 5: Debugging
  Can decode ID to see which server generated it and when
  Useful for troubleshooting
```

---

### 📊 Key Takeaways

1. **64-bit structure:** [timestamp][datacenter][worker][sequence]

2. **No coordination:** Each server generates independently

3. **Time-ordered:** Newer IDs > older IDs (useful for sharding)

4. **High capacity:** 4M IDs/sec per worker (way more than needed)

5. **Multi-region safe:** Different datacenter IDs prevent collisions

6. **Handle clock skew:** Refuse to generate if clock goes backward

7. **Worker ID assignment:** Environment variable OR service discovery

---

### 🔗 Further Reading

- **Twitter Engineering Blog:** "Announcing Snowflake" (original post)
- **Snowflake ID visualization:** Understanding the bit layout
- **Instagram's approach:** Similar but different bit allocation

---

### ✏️ Practice Exercise

**Scenario:** You have 3 datacenters (US, EU, Asia), each with 100 servers.

**Questions:**
1. How would you assign datacenter IDs and worker IDs?
2. What's your total ID generation capacity?
3. One datacenter's clocks are 5 seconds behind. What happens?

<details>
<summary>Click to see answer</summary>

**1. Datacenter and Worker ID assignment:**

```
Datacenter IDs:
  US:   datacenter_id = 0
  EU:   datacenter_id = 1
  Asia: datacenter_id = 2

Worker IDs within each datacenter:
  Server 1: worker_id = 0
  Server 2: worker_id = 1
  ...
  Server 100: worker_id = 99

Total unique combinations:
  3 datacenters × 100 workers = 300 unique worker identities

Configuration example (US, Server 5):
  DATACENTER_ID=0
  WORKER_ID=4
```

---

**2. Total ID generation capacity:**

```
Per worker capacity:
  4096 IDs/millisecond × 1000 milliseconds/second
  = 4,096,000 IDs per second

Total workers:
  3 datacenters × 100 workers = 300 workers

Total system capacity:
  300 workers × 4,096,000 IDs/sec
  = 1,228,800,000 IDs per second
  = ~1.2 billion IDs per second

With our load (40 IDs/sec):
  Headroom: 1.2 billion / 40 = 30 million times our current load
  
Conclusion: Massively over-provisioned ✓
```

---

**3. Clock 5 seconds behind in one datacenter:**

```
Scenario:
  US datacenter:   Clock shows 10:00:05
  EU datacenter:   Clock shows 10:00:00 (5 seconds behind)
  Asia datacenter: Clock shows 10:00:05

What happens:

For EU datacenter servers:
  - Generate IDs with timestamp for 10:00:00
  - Meanwhile, US/Asia generate IDs with timestamp 10:00:05
  
  EU generates:  6891234567890000 (timestamp: 10:00:00)
  US generates:  6891234572890000 (timestamp: 10:00:05)
  
  US IDs are numerically LARGER than EU IDs

Impact:

1. Uniqueness: STILL GUARANTEED ✓
   - Different datacenter IDs prevent collision
   - EU (datacenter=1), US (datacenter=0) → different IDs

2. Time-ordering: GLOBALLY BROKEN ❌
   - EU ID looks "older" than it actually is
   - But within same datacenter, ordering still works
   
3. Sharding: SLIGHTLY AFFECTED
   - If sharding by ID range, EU IDs go to "older" shard
   - Not a major problem, just slightly inefficient

4. Analytics: SLIGHTLY INCORRECT
   - EU URLs appear 5 seconds older in analytics
   - Usually not a big deal (5 sec error in hours/days of data)

Detection & Fix:

1. Monitor clock drift:
   Alert if any server's clock differs by >1 second from NTP

2. Automatic correction:
   NTP daemon gradually adjusts clock (doesn't jump)
   After a few minutes, clocks sync again

3. If drift is large (>1 minute):
   Refuse to generate IDs until clock syncs
   Better to be unavailable than generate wrong timestamps

Prevention:
   - Use NTP on all servers
   - Monitor: clock_offset_seconds
   - Alert: if offset >1 second
   - Automatic: Restart service if offset >10 seconds
```

**Conclusion:**
Clock drift causes minor time-ordering issues but doesn't break uniqueness.
In practice, with good NTP monitoring, this rarely happens.

</details>

---

### **Interview Answer Template:**

**Q: "Explain Snowflake IDs and why you'd use them"**

```
"Snowflake IDs are 64-bit distributed unique identifiers.

Structure: [41-bit timestamp][10-bit datacenter][10-bit worker][12-bit sequence]

Key benefits:
1. No coordination - each server generates independently
2. Time-ordered - newer IDs are numerically greater
3. High capacity - 4 million IDs/sec per worker
4. Multi-region safe - different datacenter IDs prevent collisions

For URL shortener:
  - Each service instance has unique datacenter+worker ID
  - Generate IDs locally (no DB hit)
  - 4M IDs/sec capacity vs our 40 IDs/sec load = 100,000× headroom
  - Time-ordered enables efficient range-based sharding

Better than auto-increment because:
  - No single point of failure (DB master)
  - No performance bottleneck
  - Works in multi-region setup
  - Can generate even when DB is down"
```

---

# **SECTION 4 NOW COMPLETE!** ✅

**You now have full coverage of:**
- 4.1 Bloom Filters (sizing, use cases)
- 4.2 Redis HA (Sentinel vs Cluster)
- 4.3 Database Sharding (range vs hash vs consistent)
- 4.4 Distributed Locks (Redis locks vs DB constraints)
- 4.5 Snowflake IDs (structure, capacity, edge cases) ← COMPLETE

**All sections ready for interview! 🚀**
