# **SECTION 1: SYSTEM ARCHITECTURE & DATA FLOW** 🏗️

---

## **1.1 Cache Strategy & Placement**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned caching in your architecture diagram. Walk me through exactly where the cache sits and how it's used. What happens when a user clicks a short URL?"

### ❌ Your Initial Answer

You drew a cache component in your diagram connected to the service layer, but when asked about the **read flow**, you didn't clearly explain:
- Whether you check cache before or after decoding the short URL
- What exactly gets cached (short URL? ID? both?)
- Cache miss behavior
- When data gets written to cache

Your diagram showed: `Service → Cache → DB` but the **data flow wasn't crystal clear**.

---

### ✅ Complete Answer

#### **What is Caching?**

Caching is storing frequently accessed data in **fast storage (memory)** to avoid hitting **slow storage (database)** repeatedly.

**Key principle:** 
- Cache reads: ~1ms (Redis in-memory)
- Database reads: ~10-50ms (disk I/O)
- **50x speed improvement!**

---

#### **Two Types of Caching in URL Shortener:**

### **A) Read Cache (Hot URL Cache)** - PRIMARY USE CASE

**Purpose:** Speed up redirects (GET /{shortCode})

**What to cache:**
```
Key: "url:123456789" (ID as key)
Value: "https://google.com" (long URL as value)
TTL: 1 hour (or never expire for permanent URLs)
```

**Why ID as key, not short code?**
- You decode `aBc123` → ID `123456789` anyway
- Integer keys are slightly more efficient
- Consistent with database primary key

---

### **B) Write Cache (Duplicate Detection Cache)** - SECONDARY USE CASE

**Purpose:** Fast duplicate checking when creating URLs

**What to cache:**
```
Key: "longurl:https://google.com" (long URL as key)
Value: "123456789" (ID as value)
TTL: 24 hours
```

**Why?**
- Avoid DB query for "does this URL already exist?"
- 95% of duplicate checks served from cache

---

### 🎯 URL Shortener Context

#### **Architecture Placement:**

```
┌─────────────────────────────────────────────────────────────┐
│                     REDIRECT FLOW (Read)                     │
└─────────────────────────────────────────────────────────────┘

User clicks: https://tiny.url/aBc123
        ↓
[Load Balancer]
        ↓
[Service Instance]
        ↓
   Decode: aBc123 → ID: 123456789
        ↓
        ↓
   ┌────▼─────────────────────────────────┐
   │  CHECK REDIS CACHE                   │
   │  Key: "url:123456789"                │
   └────┬─────────────────────────────────┘
        │
        ├─── Cache HIT (95% of requests) ──→ Return "https://google.com"
        │                                     ↓
        │                                  [302 Redirect]
        │                                     ↓
        │                                  Success! (1-2ms latency)
        │
        │
        └─── Cache MISS (5% of requests)
                    ↓
            [Query Slave DB]
            SELECT long_url FROM url_mappings WHERE id = 123456789
                    ↓
            Got: "https://google.com"
                    ↓
            [Write to Redis Cache] ← IMPORTANT: Update cache!
            SET "url:123456789" = "https://google.com" EX 3600
                    ↓
            Return "https://google.com"
                    ↓
            [302 Redirect]
                    ↓
            Success! (10-20ms latency)
```

---

```
┌─────────────────────────────────────────────────────────────┐
│                   CREATE URL FLOW (Write)                    │
└─────────────────────────────────────────────────────────────┘

User: POST /api/v1/urls {"longUrl": "https://google.com"}
        ↓
[Load Balancer]
        ↓
[Service Instance]
        ↓
        ↓
   ┌────▼──────────────────────────────────────┐
   │  CHECK DUPLICATE IN CACHE (Optional)      │
   │  Key: "longurl:https://google.com"        │
   └────┬──────────────────────────────────────┘
        │
        ├─── Cache HIT ──→ Return existing short URL
        │                  (No DB write needed!)
        │
        └─── Cache MISS
                    ↓
            [Check Master DB]
            SELECT id FROM url_mappings 
            WHERE long_url = 'https://google.com'
                    ↓
                    ├─── Exists ──→ Return existing
                    │
                    └─── Not exists
                            ↓
                    Generate Snowflake ID: 123456789
                            ↓
                    [Write to Master DB]
                    INSERT INTO url_mappings (id, long_url) 
                    VALUES (123456789, 'https://google.com')
                            ↓
                    [Write to Redis Cache] ← IMPORTANT!
                    SET "url:123456789" = "https://google.com"
                    SET "longurl:https://google.com" = "123456789"
                            ↓
                    Return: {"shortUrl": "https://tiny.url/aBc123"}
```

---

#### **Cache Patterns: Cache-Aside vs Write-Through**

**Cache-Aside Pattern (Lazy Loading)** ✅ RECOMMENDED FOR URL SHORTENER

```
Read:
1. Check cache
2. If MISS → Query DB → Write to cache → Return
3. If HIT → Return directly

Write:
1. Write to DB
2. Invalidate/update cache (optional)
```

**Why this is best:**
- ✅ Only hot data ends up in cache (self-optimizing)
- ✅ Cache failures don't block reads (degrade to DB)
- ✅ Simple to implement

---

**Write-Through Pattern** ❌ NOT RECOMMENDED HERE

```
Write:
1. Write to cache
2. Write to DB synchronously

Read:
1. Always check cache first
```

**Why NOT for URL shortener:**
- ❌ Every created URL goes to cache (even if never clicked)
- ❌ Cache becomes bloated with cold data
- ❌ Cache failure blocks writes

---

#### **Cache Sizing Calculation:**

**Question:** How big should the Redis cache be?

**Analysis:**
- Total URLs: 24 billion (over 20 years)
- Hot URLs: Top 10 million most-clicked (0.04% of total)
- Average entry size:
  - Key: `url:123456789` = 15 bytes
  - Value: `https://example.com/path` = 100 bytes average
  - Total per entry: ~115 bytes

**Calculation:**
```
10 million entries × 115 bytes = 1.15 GB
Add 30% overhead (Redis metadata) = 1.5 GB
Add buffer for write cache = 2 GB total
```

**Cache configuration:**
```
Redis:
  Max memory: 4 GB (room for growth)
  Eviction policy: LRU (Least Recently Used)
  Persistence: RDB snapshots (for recovery)
```

---

#### **Cache Hit Rate Optimization:**

**Target: 95%+ hit rate**

**Strategies:**

**1. TTL Strategy:**
```
Hot URLs (>100 clicks/day): TTL = Never expire
Warm URLs (10-100 clicks/day): TTL = 1 hour
Cold URLs: Don't cache
```

**2. Prewarming:**
```
On service startup:
  - Load top 1000 URLs into cache
  - Prevents "thundering herd" at cold start
```

**3. Monitoring:**
```
Track metrics:
  - cache_hit_rate = hits / (hits + misses)
  - cache_memory_usage
  - cache_eviction_rate
  
Alert if:
  - Hit rate < 90% (cache too small or TTL too short)
  - Eviction rate > 1000/sec (cache too small)
```

---

#### **Handling Cache Failures:**

**Scenario:** Redis cluster goes down

**Without graceful degradation:**
```
User clicks URL → Service checks cache → TIMEOUT (Redis down)
→ Service crashes or hangs → ALL requests fail ❌
```

**With graceful degradation:**
```
try:
    long_url = redis.get(f"url:{id}", timeout=0.1)  # 100ms timeout
    if long_url:
        return long_url
except RedisTimeout:
    log.warning("Redis timeout, falling back to DB")
except RedisError:
    log.error("Redis error, falling back to DB")

# Fallback: Query database directly
long_url = db.query("SELECT long_url FROM url_mappings WHERE id = ?", id)
return long_url
```

**Result:**
- ✅ Service stays available (slower, but works)
- ✅ No cascading failure
- ✅ Alerts fire for ops team to fix Redis

---

### 📊 Key Takeaways

1. **Cache placement:** Between service and database, check cache BEFORE hitting DB

2. **Two caches:** 
   - Read cache (url:ID → long_url) - 95% hit rate target
   - Write cache (longurl:URL → ID) - Optional, for duplicate detection

3. **Cache-aside pattern:** Best for read-heavy workloads like URL shortener

4. **Size conservatively:** 2-4 GB Redis handles 10 million hot URLs

5. **Graceful degradation:** Always fallback to DB if cache fails

6. **Monitor religiously:** Track hit rate, evictions, memory usage

---

### 🔗 Further Reading

- **Redis documentation:** Eviction policies (LRU, LFU, TTL)
- **Caching strategies:** "Cache-Aside vs Write-Through vs Write-Behind"
- **Real-world example:** Twitter's "Manhattan" distributed cache

---

### ✏️ Practice Exercise

**Scenario:** Your cache hit rate drops from 95% to 60% suddenly.

**Question:** What could be the causes? How would you investigate?

<details>
<summary>Click to see answer</summary>

**Possible causes:**
1. **Traffic spike with new URLs** - Many new URLs being clicked (cache hasn't warmed up yet)
2. **Cache size too small** - LRU evicting hot URLs too quickly
3. **TTL too short** - Hot URLs expiring before next access
4. **Uneven traffic distribution** - Viral link causing one URL to dominate cache space

**Investigation steps:**
1. Check Grafana: Plot cache hit rate vs traffic volume
2. Check Redis: `INFO stats` - evicted_keys, expired_keys
3. Check application logs: Which URLs are missing from cache?
4. Query analytics DB: Top 100 most-clicked URLs in last hour

**Solutions:**
- Increase cache size (scale up Redis)
- Increase TTL for hot URLs
- Pre-warm cache with known viral URLs
- Add a second cache tier (L1 = hot, L2 = warm)

</details>

---

## **1.2 Rate Limiter Architecture**

### 🤔 Interview Question/Context

**Interviewer:** "I see you placed rate limiting after the load balancer. Walk me through what happens when the same user makes requests to different service instances. Does your rate limiting work correctly?"

### ❌ Your Initial Answer

In your architecture diagram, you showed:

```
Client → Load Balancer → Rate Limiter → Service Instances
```

The issue: You placed rate limiting as a **separate component** but didn't explain:
- How rate limiter state is shared across services
- What happens if requests go to different servers
- Where the rate limit counters are stored

**The critical mistake:** If each service has its own rate limiter, a user can bypass limits by hitting different servers!

---

### ✅ Complete Answer

#### **The Problem with Distributed Rate Limiting:**

**Scenario without centralized state:**

```
User makes 15 requests in 1 second (limit is 10/sec)

Load Balancer distributes:
  Request 1-5  → Service A (checks local counter: 5 requests ✅)
  Request 6-10 → Service B (checks local counter: 5 requests ✅)
  Request 11-15→ Service C (checks local counter: 5 requests ✅)

Result: User made 15 requests, all allowed! ❌
Rate limit bypassed!
```

**Root cause:** Each service instance has **independent state** - they don't know about requests handled by other instances.

---

#### **Solution: Centralized Rate Limiting with Redis**

**Architecture:**

```
                    [Client]
                       ↓
               [Load Balancer]
                       ↓
        ┌──────────────┼──────────────┐
        ↓              ↓              ↓
   [Service A]   [Service B]   [Service C]
        ↓              ↓              ↓
        └──────────────┼──────────────┘
                       ↓
                  [Redis Cluster]
              (Shared rate limit state)
```

**How it works:**

```
Every service instance:
1. Receives request
2. Asks Redis: "Has user X exceeded limit?"
3. Redis responds: Yes/No (atomic operation)
4. Service allows or rejects request
```

**Redis stores:**
```
Key: "ratelimit:user:12345:create_url"
Value: 7 (number of requests in current window)
TTL: 60 seconds (sliding window)
```

---

#### **Token Bucket Implementation in Distributed System:**

**Redis data structure:**

```
Key: "ratelimit:user:12345"
Value: JSON {
  "tokens": 3,              // Remaining tokens
  "capacity": 10,           // Max tokens
  "refill_rate": 1,         // Tokens per second
  "last_refill": 1638360000 // Unix timestamp
}
TTL: 3600 seconds (1 hour)
```

**Service logic (pseudocode for HLD):**

```
FUNCTION check_rate_limit(user_id, action):
    key = "ratelimit:" + user_id + ":" + action
    
    // Atomic operation in Redis (Lua script)
    result = REDIS.EVAL("""
        -- Get current bucket
        local bucket = redis.call('GET', KEYS[1])
        if not bucket then
            -- First request - initialize bucket
            bucket = {tokens: CAPACITY, last_refill: now()}
        end
        
        -- Calculate tokens to add based on time elapsed
        elapsed = now() - bucket.last_refill
        tokens_to_add = elapsed * REFILL_RATE
        bucket.tokens = min(CAPACITY, bucket.tokens + tokens_to_add)
        bucket.last_refill = now()
        
        -- Check if request allowed
        if bucket.tokens >= 1 then
            bucket.tokens = bucket.tokens - 1
            redis.call('SETEX', KEYS[1], TTL, bucket)
            return {allowed: true, remaining: bucket.tokens}
        else
            return {allowed: false, retry_after: 1/REFILL_RATE}
        end
    """, key)
    
    RETURN result
```

**Why Lua script?**
- ✅ **Atomic operation** - no race conditions
- ✅ **Single round-trip** to Redis (not multiple GET/SET calls)
- ✅ **Guaranteed consistency** across all service instances

---

### 🎯 URL Shortener Context

#### **Rate Limiting Strategy:**

**Different limits for different actions:**

```
Action: Create URL
  Unauthenticated user (by IP): 10 URLs/day
  Authenticated free user: 100 URLs/day
  Authenticated premium user: 10,000 URLs/day

Action: Access URL (redirect)
  Any user: 1,000 requests/minute (DDoS protection)
  
Action: Custom URL creation
  Any authenticated user: 5 custom URLs/day
```

**Identifier strategies:**

```
1. IP Address:
   Key: "ratelimit:ip:192.168.1.1:create_url"
   Problem: Shared IPs (NAT), VPNs bypass
   Use case: Unauthenticated users

2. User ID:
   Key: "ratelimit:user:12345:create_url"
   Problem: Requires authentication
   Use case: Authenticated users

3. API Key:
   Key: "ratelimit:apikey:abc123:create_url"
   Problem: API key can be stolen
   Use case: Programmatic access

BEST PRACTICE: Combine multiple identifiers
```

---

#### **Layered Rate Limiting:**

**Defense in depth approach:**

```
Layer 1: CDN/DDoS Protection (Cloudflare)
  └─ 10,000 requests/sec per IP (blocks DDoS attacks)
         ↓
Layer 2: Load Balancer (NGINX)
  └─ 1,000 requests/sec per IP (prevents overload)
         ↓
Layer 3: Application Rate Limiter (Redis)
  └─ 10 URLs/day per user (business logic)
         ↓
Layer 4: Database Rate Limiter
  └─ Max 500 connections (prevents DB overload)
```

**Each layer protects the next!**

---

#### **Where Rate Limiting Happens in Request Flow:**

```
┌─────────────────────────────────────────────────────────┐
│              CREATE URL WITH RATE LIMITING               │
└─────────────────────────────────────────────────────────┘

POST /api/v1/urls {"longUrl": "..."}
        ↓
[Load Balancer]
  - Layer 2 check: IP not exceeding 1000 req/sec? ✓
        ↓
[Service Instance A]
        ↓
   ┌────▼────────────────────────────────────────┐
   │  RATE LIMIT CHECK (FIRST THING!)            │
   │                                              │
   │  Identify user:                              │
   │    - Authenticated? Use user_id              │
   │    - No? Use IP address                      │
   │                                              │
   │  Query Redis:                                │
   │  "ratelimit:user:12345:create_url"           │
   └────┬────────────────────────────────────────┘
        │
        ├─── ALLOWED ──→ Continue processing
        │                    ↓
        │                Validate URL
        │                    ↓
        │                Check duplicate
        │                    ↓
        │                Generate ID
        │                    ↓
        │                Save to DB
        │                    ↓
        │                Return 201
        │
        │
        └─── REJECTED ──→ Return 429 Too Many Requests
                          {
                            "error": "RATE_LIMIT_EXCEEDED",
                            "message": "You can create 10 URLs per day",
                            "retry_after": 3600
                          }
                          
                          Headers:
                            X-RateLimit-Limit: 10
                            X-RateLimit-Remaining: 0
                            X-RateLimit-Reset: 1638363600
                            Retry-After: 3600
```

**Key principle:** Rate limit check happens **BEFORE** any expensive operations (DB queries, external API calls, etc.)

---

#### **Handling Redis Failures (Rate Limiter Down):**

**Scenario:** Redis cluster goes down - what happens to rate limiting?

**Option 1: Fail Closed (Deny all requests)** ❌
```
Redis down → Can't check rate limit → Reject all requests → Service unusable
```

**Option 2: Fail Open (Allow all requests)** ⚠️
```
Redis down → Can't check rate limit → Allow all requests → Vulnerable to abuse
```

**Option 3: Graceful Degradation (Smart fallback)** ✅
```
try:
    rate_limit_check(user_id)
except RedisError:
    // Fallback: In-memory rate limiting per service instance
    // Not perfect (can be bypassed with load balancer), but better than nothing
    
    if local_rate_limiter.check(user_id):
        allow request (with warning log)
    else:
        reject request
        
    // Alert ops team: "Redis down, rate limiting degraded"
```

**Best practice for production:**
- ✅ Fail open for read operations (redirects)
- ✅ Fail closed for write operations (create URLs) if abuse is critical concern
- ✅ Always log and alert when falling back

---

#### **Rate Limiting Response Format:**

**When request is allowed:**
```
HTTP/1.1 201 Created
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 87
X-RateLimit-Reset: 1638363600

{
  "shortUrl": "https://tiny.url/aBc123",
  "longUrl": "https://google.com"
}
```

**When request is rejected:**
```
HTTP/1.1 429 Too Many Requests
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 1638363600
Retry-After: 3600

{
  "error": "RATE_LIMIT_EXCEEDED",
  "message": "You have exceeded your daily limit of 100 URLs. Limit resets in 1 hour.",
  "retry_after_seconds": 3600
}
```

**Headers explanation:**
- `X-RateLimit-Limit`: Total allowed in window
- `X-RateLimit-Remaining`: How many left
- `X-RateLimit-Reset`: Unix timestamp when limit resets
- `Retry-After`: Seconds to wait (standard HTTP header)

---

#### **Advanced: Sliding Window vs Fixed Window**

**Fixed Window Problem:**

```
Time:   00:00:00 ────────────────► 00:01:00 ────────────────► 00:02:00
Limit:  10 requests/minute

User:   00:00:50: Makes 10 requests ✓ (Window 1)
        00:01:05: Makes 10 requests ✓ (Window 2)

Result: 20 requests in 15 seconds! Burst attack possible! ❌
```

**Sliding Window Solution:**

```
Track requests in last 60 seconds (sliding), not per-minute windows

Redis data:
Key: "ratelimit:user:12345:timestamps"
Value: [
  1638360050,  // Timestamp 1
  1638360051,  // Timestamp 2
  1638360052,  // Timestamp 3
  ...
]

Check:
  1. Remove timestamps older than 60 seconds
  2. Count remaining timestamps
  3. If < limit, allow and add new timestamp
```

**Trade-off:**
- ✅ More accurate (prevents burst attacks)
- ❌ More memory (store individual timestamps)
- ❌ More complex

**For URL shortener:** Token bucket is sufficient (simpler, less memory)

---

### 📊 Key Takeaways

1. **Centralized state is critical:** Use Redis to share rate limit counters across all service instances

2. **Rate limit EARLY:** Check before expensive operations (DB, validation, etc.)

3. **Fail gracefully:** When Redis is down, degrade to local rate limiting (imperfect but better than crash)

4. **Return helpful headers:** `X-RateLimit-*` and `Retry-After` help clients behave well

5. **Layer your defenses:** CDN → Load Balancer → Application → Database

6. **Use Lua scripts:** Atomic Redis operations prevent race conditions

---

### 🔗 Further Reading

- **Redis Lua scripting:** Atomic rate limiting patterns
- **IETF Draft:** "RateLimit Header Fields for HTTP"
- **Kong/NGINX:** API gateway rate limiting plugins

---

### ✏️ Practice Exercise

**Scenario:** You notice some users are bypassing rate limits by:
1. Using multiple API keys
2. Using VPNs to change IPs
3. Distributing requests across time zones

**Question:** How would you detect and prevent this abuse?

<details>
<summary>Click to see answer</summary>

**Detection strategies:**

1. **Behavioral analysis:**
   - Track patterns: Same long URLs from "different" users
   - Flag suspicious: 10 users creating identical URLs within 1 hour

2. **Device fingerprinting:**
   - Browser fingerprint (canvas, WebGL, fonts)
   - Combine with IP for more accurate identification

3. **CAPTCHA after threshold:**
   - After 3 URLs: No CAPTCHA
   - After 5 URLs: Show CAPTCHA
   - After 10 URLs: Require phone verification

4. **Honeypot URLs:**
   - Hidden form fields (bots fill them)
   - Track users who submit honeypot data

**Prevention:**

1. **Require email verification** for higher limits
2. **Phone verification** for custom URLs
3. **Credit card** (even $0 charge) for premium tier
4. **Manual review** for high-volume users

**Redis tracking:**
```
Key: "abuse:url_hash:abc123def456"
Value: List of user_ids who created this URL
Alert if: Same URL created by 5+ users in 1 hour
```

</details>

---

## **1.3 Async Processing with Message Queues**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned an 'Analytics DB' in your diagram. How does click data get from your main service to the analytics database? Does the service write to both the main DB and analytics DB synchronously?"

### ❌ Your Initial Answer

In your diagram, you showed:
```
Service → Master DB
Service → Analytics DB
```

But you didn't explain:
- **When/how** data flows to Analytics DB
- Whether it's **synchronous** (blocking) or **asynchronous** (non-blocking)
- What happens if Analytics DB is slow or down

**The problem:** If the service writes to Analytics DB synchronously, a slow analytics query blocks the user's redirect!

```
User clicks link → Service records analytics → Analytics DB slow (500ms) 
→ User waits 500ms → Bad UX! ❌
```

---

### ✅ Complete Answer

#### **Why Async Processing is Critical:**

**Synchronous approach (BAD):**

```
User Request → Service → Write to Main DB ✓ (50ms)
                      → Write to Analytics DB ✓ (500ms)
                      → Return response
                      
Total latency: 550ms ❌
```

**If Analytics DB is down:**
```
User Request → Service → Write to Main DB ✓
                      → Write to Analytics DB ✗ (timeout)
                      → Return 500 error to user ❌
```

**Async approach (GOOD):**

```
User Request → Service → Write to Main DB ✓ (50ms)
                      → Publish event to Queue ✓ (2ms)
                      → Return response immediately
                      
Total latency: 52ms ✓

Background Worker → Read from Queue
                 → Write to Analytics DB
                 → User doesn't wait!
```

**Key principle:** User-facing operations must be **fast** and **reliable**. Analytics is important but **not critical** for user experience.

---

#### **Message Queue Architecture:**

```
┌──────────────────────────────────────────────────────────────┐
│                    ASYNC EVENT FLOW                          │
└──────────────────────────────────────────────────────────────┘

[User Action]
     ↓
[Service] ──────────────────────────────┐
     ↓                                   │
[Main DB]                                │ Publish event
     ↓                                   │ (non-blocking)
[Return 302]                             ↓
     ↓                          ┌────────────────┐
User gets response fast!        │  Kafka Cluster │
(50ms)                          │  (Event Queue) │
                                └────────┬───────┘
                                         │
                        ┌────────────────┼────────────────┐
                        ↓                ↓                ↓
                [Consumer Group: Analytics Workers]
                [Worker 1]      [Worker 2]      [Worker 3]
                        ↓                ↓                ↓
                        └────────────────┼────────────────┘
                                         ↓
                                [Analytics DB]
                                (ClickHouse)
                                         ↓
                                [Dashboard/Reports]
```

---

#### **Kafka vs RabbitMQ vs Amazon SQS:**

| Feature | Kafka ✅ | RabbitMQ | Amazon SQS |
|---------|---------|-----------|------------|
| **Throughput** | Millions/sec | Thousands/sec | Thousands/sec |
| **Durability** | Disk persistence | Memory + disk | Fully managed |
| **Ordering** | Partition-level | Queue-level | FIFO queues |
| **Replay** | Yes (retain logs) | No | Limited |
| **Scalability** | Horizontal | Vertical | Auto-scaling |
| **Best for** | High-volume analytics | Task queues | Serverless/AWS |

**For URL shortener:** **Kafka is best choice**

**Why?**
- ✅ Handle 4,000 clicks/sec easily (can scale to millions)
- ✅ Persistent logs (can replay if analytics processing fails)
- ✅ Multiple consumers (analytics, fraud detection, ML training)
- ✅ Partition by short_url_id (parallel processing)

---

### 🎯 URL Shortener Context

#### **Event Types:**

**1. URL Created Event:**
```json
{
  "event_type": "url_created",
  "event_id": "evt_123456789",
  "timestamp": "2025-11-29T10:30:00Z",
  "data": {
    "short_url_id": 123456789,
    "short_code": "aBc123",
    "long_url": "https://google.com",
    "user_id": 456,
    "user_ip": "192.168.1.1",
    "created_via": "web_ui"
  }
}
```

**2. URL Clicked Event:**
```json
{
  "event_type": "url_clicked",
  "event_id": "evt_987654321",
  "timestamp": "2025-11-29T10:35:00Z",
  "data": {
    "short_url_id": 123456789,
    "short_code": "aBc123",
    "long_url": "https://google.com",
    "user_ip": "203.45.67.89",
    "user_agent": "Mozilla/5.0...",
    "referer": "https://twitter.com",
    "country": "US",
    "city": "San Francisco"
  }
}
```

---

#### **Kafka Topic Structure:**

```
Topic: url-events
  Partitions: 10 (for parallel processing)
  Replication: 3 (for durability)
  Retention: 7 days (can replay last week's events)

Partitioning strategy:
  Key: short_url_id
  Why: All events for same URL go to same partition
       → Maintains ordering per URL
       → Worker can process in sequence
```

**Example:**
```
URL aBc123 (ID: 100) → Partition 0
URL xYz789 (ID: 200) → Partition 0
URL mNo456 (ID: 300) → Partition 1

All events for aBc123 are ordered in Partition 0
```

---

#### **Service Publishing Logic:**

```
┌──────────────────────────────────────────────────────────┐
│          REDIRECT WITH ASYNC ANALYTICS                   │
└──────────────────────────────────────────────────────────┘

GET /