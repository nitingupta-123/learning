# **SECTION 2: MONITORING & OBSERVABILITY** 📊

---

## **2.1 Key Metrics to Track (RED/USE Methods)**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned using DataDog for monitoring. That's good, but what specific metrics would you track for your URL shortener? How would you know if the system is healthy?"

### ❌ Your Initial Answer

You said: **"Use DataDog for metrics, query performance, and alerting"**

**The problem:** You named a **tool** but didn't specify **WHAT to monitor**.

This is like saying "I'll use a thermometer" without saying "I'll measure body temperature to detect fever."

**What was missing:**
- Specific metric names
- What values are normal vs abnormal
- Which metrics matter most
- How metrics relate to user experience

---

### ✅ Complete Answer

#### **Two Monitoring Frameworks:**

### **RED Method** (For Request-Driven Services) ✅

**R**ate - How many requests per second  
**E**rrors - How many requests are failing  
**D**uration - How long requests take

**Perfect for:** URL shortener (request/response service)

---

### **USE Method** (For Resources)

**U**tilization - % of resource being used  
**S**aturation - How much work is queued  
**E**rrors - Error count

**Perfect for:** Databases, caches, queues

---

### 🎯 URL Shortener Context

#### **RED Metrics - Application Level:**

### **1. RATE Metrics**

**What to track:**

```
Metric: http_requests_total
Labels: 
  - endpoint: "/api/v1/urls", "/{shortCode}"
  - method: "GET", "POST"
  - status_code: "200", "201", "302", "404", "429", "500"

Query examples:
  - Total requests/sec: rate(http_requests_total[1m])
  - Create URL rate: rate(http_requests_total{endpoint="/api/v1/urls"}[1m])
  - Redirect rate: rate(http_requests_total{endpoint="/{shortCode}"}[1m])
```

**Normal values for your system:**
```
Create URL: 40 requests/sec (average), 200 requests/sec (peak)
Redirects: 4,000 requests/sec (average), 20,000 requests/sec (peak)
```

**Why this matters:**
- Sudden drop → Service down or traffic loss
- Sudden spike → Viral link or attack (need to scale)

---

**Dashboard visualization:**

```
Graph 1: Request Rate by Endpoint
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                               ╱╲
4000 req/s ─────────────────╱──╲─────  Redirects
                          ╱      ╲
                        ╱          ╲
   0 ─────────────────────────────────
       10:00    11:00    12:00    13:00

  40 req/s ─────────────────────────  Create URL
   0 ─────────────────────────────────
       10:00    11:00    12:00    13:00

Insight: Redirect spike at 12:00 (lunch hour traffic)
         Create URL steady (business users)
```

---

### **2. ERRORS Metrics**

**What to track:**

```
Metric: http_errors_total
Labels:
  - endpoint
  - error_type: "rate_limit", "invalid_url", "db_error", "cache_timeout"
  - status_code: "400", "404", "429", "500", "503"

Query examples:
  - Error rate: rate(http_errors_total[1m])
  - Error percentage: 
    rate(http_errors_total[1m]) / rate(http_requests_total[1m]) * 100
  
  - 4xx errors: rate(http_errors_total{status_code=~"4.."}[1m])
  - 5xx errors: rate(http_errors_total{status_code=~"5.."}[1m])
```

**Normal values:**
```
4xx errors: <1% (client mistakes, expected)
  - 404: URL not found (0.1% - rare)
  - 429: Rate limited (0.5% - abuse attempts)
  - 400: Invalid URL (0.3% - user error)

5xx errors: <0.1% (server problems, NOT expected!)
  - 500: Internal error (0.05% - bugs)
  - 503: Service unavailable (0.01% - overload)
```

**Why this matters:**
- 4xx spike → Possible attack or client bug
- 5xx spike → **YOUR PROBLEM** - service degraded

---

**Alert rules:**

```
Alert: HighErrorRate
Condition: 
  (rate(http_errors_total{status_code=~"5.."}[5m]) 
   / rate(http_requests_total[5m])) > 0.01

Meaning: More than 1% of requests failing
Action: Page on-call engineer immediately
Severity: CRITICAL

────────────────────────────────────────

Alert: ClientErrorSpike
Condition:
  rate(http_errors_total{status_code="400"}[5m]) > 100

Meaning: 100+ invalid URLs per second (possible attack)
Action: Slack notification to team
Severity: WARNING
```

---

### **3. DURATION Metrics (Latency)**

**What to track:**

```
Metric: http_request_duration_seconds
Type: Histogram (buckets: 0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0)
Labels:
  - endpoint
  - method

Query examples:
  - P50 latency: histogram_quantile(0.5, http_request_duration_seconds)
  - P95 latency: histogram_quantile(0.95, http_request_duration_seconds)
  - P99 latency: histogram_quantile(0.99, http_request_duration_seconds)
```

**Normal values:**

```
Endpoint: POST /api/v1/urls (Create URL)
  P50: 50ms  (median - half of requests faster than this)
  P95: 150ms (95% of requests faster than this)
  P99: 300ms (99% of requests faster than this)

Endpoint: GET /{shortCode} (Redirect)
  P50: 10ms  (cache hit - very fast!)
  P95: 50ms  (some cache misses)
  P99: 100ms (DB queries)
```

**Why track percentiles, not averages?**

```
Example: 100 redirect requests

Scenario A - All fast:
  99 requests: 10ms each
  1 request: 50ms (cache miss)
  
  Average: 10.4ms  ✓
  P95: 10ms ✓
  P99: 50ms ✓

Scenario B - Some slow:
  95 requests: 10ms each
  5 requests: 2000ms (DB timeout!)
  
  Average: 109ms  ← Hides the problem!
  P95: 10ms ✓
  P99: 2000ms ← Shows the problem! ✓✓✓
```

**Insight:** P99 catches outliers that hurt user experience!

---

**SLA Definition using RED:**

```
Service Level Agreement (SLA):
  "99.9% of redirect requests complete in under 100ms"

Translation to metrics:
  - Rate: Can handle 10,000 req/sec
  - Errors: <0.1% error rate
  - Duration: P99 latency <100ms

If any violated for >5 minutes → SLA breach
```

---

#### **USE Metrics - Infrastructure Level:**

### **1. Database (Master & Slaves)**

```
Utilization:
  - db_connections_active / db_connections_max
  - db_cpu_usage_percent
  - db_memory_usage_percent

Saturation:
  - db_query_queue_length (queries waiting)
  - db_disk_io_utilization (disk busy %)

Errors:
  - db_connection_errors_total
  - db_query_errors_total
  - db_deadlocks_total
```

**Normal values:**

```
Master DB:
  Connection utilization: 40% (20/50 connections)
  CPU: 30-50%
  Memory: 70% (high is OK - caching!)
  Query queue: 0-5 queries
  
Slave DB:
  Connection utilization: 60% (30/50 connections) - higher due to reads
  CPU: 40-60%
  Replication lag: <1 second
```

**Critical alert:**

```
Alert: DatabaseReplicationLag
Condition: db_replication_lag_seconds > 10

Why bad: 
  - User creates URL → writes to master
  - User clicks URL → reads from slave
  - Slave 10 seconds behind → URL not found! ❌
  
Action: Route reads to master temporarily
```

---

### **2. Redis Cache**

```
Utilization:
  - redis_memory_used_bytes / redis_memory_max_bytes
  - redis_connected_clients / redis_max_clients
  - redis_cpu_usage_percent

Saturation:
  - redis_blocked_clients (waiting for operations)
  - redis_evicted_keys (keys kicked out due to memory pressure)

Errors:
  - redis_rejected_connections_total
  - redis_command_errors_total
```

**Normal values:**

```
Memory: 60-80% (leave room for growth)
Clients: 100-200 connections (from all service instances)
Evictions: <1000/sec (low eviction = good cache size)
Hit rate: 95%+ (calculated separately)
```

**Cache-specific metrics:**

```
Metric: cache_hit_rate
Calculation: cache_hits / (cache_hits + cache_misses)

Normal: >95%
Warning: 90-95% (cache too small or TTL too short)
Critical: <90% (cache not working well)
```

**Dashboard:**

```
Cache Performance
━━━━━━━━━━━━━━━━━━━━━━━━━━━
Hit Rate:    96.5% ✓
Memory:      2.1 GB / 4 GB (52%) ✓
Evictions:   234/sec (low) ✓
Latency P95: 1.2ms ✓

Status: HEALTHY
```

---

### **3. Kafka Message Queue**

```
Utilization:
  - kafka_disk_usage_percent
  - kafka_network_io_bytes

Saturation:
  - kafka_consumer_lag (events waiting to be processed)
  - kafka_fetch_queue_size

Errors:
  - kafka_failed_produce_requests_total
  - kafka_failed_fetch_requests_total
```

**Normal values:**

```
Consumer lag: <10,000 events (processed within seconds)
Produce rate: 4,000 events/sec (matching redirect rate)
Consume rate: 4,000 events/sec (keeping up!)
```

**Critical alert:**

```
Alert: KafkaConsumerLagHigh
Condition: kafka_consumer_lag > 100000

Why bad:
  - Events piling up
  - Analytics delayed by hours
  - Disk might fill up
  
Action: Scale up consumer workers
```

---

#### **Business Metrics:**

**Beyond technical metrics, track business health:**

```
1. urls_created_total
   - Track growth rate
   - Compare to yesterday/last week
   
2. urls_clicked_total
   - User engagement metric
   - Revenue proxy (ads/analytics)
   
3. unique_users_daily
   - Active user count
   - Growth indicator
   
4. top_10_urls_by_clicks
   - Which URLs are viral?
   - Detect trends
   
5. malicious_urls_blocked_total
   - Security effectiveness
   - Abuse rate
```

**Business dashboard:**

```
Daily Stats (Nov 29, 2025)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
URLs Created:     3,456  (↑ 12% vs yesterday)
Total Clicks:     345,678 (↑ 5% vs yesterday)
Unique Users:     12,345
Blocked Malicious: 23 URLs

Top URL: tiny.url/xyz789
  - 45,678 clicks
  - Source: Twitter
  - Geography: 60% US, 20% UK, 10% India
```

---

### 📊 Key Takeaways

1. **RED for services:** Rate, Errors, Duration - covers user experience

2. **USE for resources:** Utilization, Saturation, Errors - covers infrastructure health

3. **Track percentiles:** P95/P99 catch outliers that averages hide

4. **Error budget:** <0.1% errors (99.9% success) is typical SLA

5. **Latency targets:** P99 <100ms for redirects, <500ms for creates

6. **Consumer lag:** Critical metric for async processing

7. **Business metrics:** URLs created, clicks, growth rate

---

### 🔗 Further Reading

- **"Site Reliability Engineering" by Google:** Chapter on monitoring
- **Brendan Gregg's USE Method:** Detailed resource monitoring
- **Tom Wilkie's RED Method:** Service-level monitoring

---

### ✏️ Practice Exercise

**Scenario:** Your monitoring shows these metrics at 2 PM:

```
Redirects P99 latency: 2000ms (normally 50ms)
Redis hit rate: 45% (normally 95%)
Redis memory: 95% (normally 60%)
Redis evictions: 50,000/sec (normally 500/sec)
Error rate: Still 0.01% (normal)
```

**Questions:**
1. What's happening?
2. What's the root cause?
3. How do you fix it immediately?
4. How do you prevent it long-term?

<details>
<summary>Click to see answer</summary>

**1. What's happening:**

The cache is **thrashing** - running out of memory and aggressively evicting keys, causing most requests to miss cache and hit the slow database.

**Symptom cascade:**
```
Redis runs out of memory (95% full)
    ↓
Evicts 50,000 keys/sec (panic mode)
    ↓
Hit rate drops to 45% (most keys evicted)
    ↓
55% of requests hit DB instead of cache
    ↓
P99 latency explodes to 2000ms (DB queries slow)
    ↓
Users see slow redirects!
```

---

**2. Root cause possibilities:**

**A. Traffic spike:**
```
Viral link → 10x more URLs being cached
→ Cache too small for new traffic pattern
```

**B. TTL change:**
```
Developer changed TTL from "never expire" to "1 hour"
→ More churn in cache
→ More memory pressure
```

**C. Large URLs:**
```
Someone creating URLs with very long destinations
→ Each cache entry larger than expected
→ Cache fills up faster
```

---

**3. Immediate fix:**

**Option A: Increase Redis memory (FASTEST)**
```
Action: Scale up Redis instance
  Current: 4 GB
  New: 8 GB
  
Time: 5 minutes (if cloud provider supports hot resize)
Result: Evictions stop, hit rate recovers
```

**Option B: Clear low-value keys**
```
Action: Evict URLs with <10 clicks
Frees: ~30% of cache
Time: 1 minute
Result: Room for hot URLs
```

**Option C: Add cache tier**
```
Action: Add second Redis cluster (emergency capacity)
Route: New URLs → New cluster, Old URLs → Old cluster
Time: 15 minutes
Result: Distribute load
```

---

**4. Long-term prevention:**

**A. Capacity planning:**
```
Monitor trend: cache_memory_usage
Predict: Will hit 80% in 2 weeks
Action: Scale up before crisis
```

**B. Better eviction policy:**
```
Current: LRU (Least Recently Used)
Change to: LFU (Least Frequently Used)
Result: Keep viral URLs, evict one-time URLs
```

**C. Tiered caching:**
```
L1 Cache (Hot): 2 GB, never expire, top 1M URLs
L2 Cache (Warm): 4 GB, 1 hour TTL, next 5M URLs
L3 Cache (Cold): Database

Result: Better memory utilization
```

**D. Alerting:**
```
Alert: CacheMemoryHigh
Condition: redis_memory_usage > 75%
When: Triggered 30 minutes BEFORE crisis
Action: Auto-scale or notify team

Prevents: Last-minute firefighting
```

</details>

---

## **2.2 Detecting Performance Issues**

### 🤔 Interview Question/Context

**Interviewer:** "Users are complaining that redirects are slow. Your dashboards show average latency is 50ms, which looks normal. How would you investigate?"

### ❌ Your Initial Answer

You said: **"Use DataDog to check query and API performance"**

**The problem:** Too vague! You need a **systematic investigation approach**.

Also, you fell into the "average latency trap" - averages can hide problems!

---

### ✅ Complete Answer

#### **The Average Latency Trap:**

```
Scenario: 1000 redirect requests

995 requests: 10ms each
5 requests: 10,000ms each (10 seconds!)

Average: (995×10 + 5×10000) / 1000 = 59.95ms

Dashboard shows: "Average latency: 60ms ✓ Looks fine!"
Users experience: "5 out of 1000 requests took 10 seconds! ❌"

Problem: Average HIDES the slow requests!
Solution: Track P95, P99, P999 percentiles
```

---

### 🎯 URL Shortener Context

#### **Method 1: Synthetic Monitoring (Proactive)**

**What it is:** Automated tests that continuously check your service from multiple locations.

**Setup:**

```
Monitoring Service (runs every 60 seconds from 5 regions):

Region: US-East, US-West, EU, Asia, South America

Test flow:
  1. Create a URL: POST /api/v1/urls
     - Measure: create_latency
     - Expected: <500ms
  
  2. Click the URL: GET /{shortCode}
     - Measure: redirect_latency  
     - Expected: <100ms
  
  3. Check again (cache hit): GET /{shortCode}
     - Measure: cached_redirect_latency
     - Expected: <20ms
  
  4. Report metrics to monitoring system
  
  5. If any step fails or exceeds threshold:
     - Alert: "Synthetic check failed in EU region"
```

**Benefits:**
- ✅ Detect issues **before users complain**
- ✅ Catch regional problems (EU slow, US fine)
- ✅ Baseline performance over time

---

**Example alert:**

```
Alert: SyntheticCheckSlow
Time: 2025-11-29 14:23:00
Region: EU-West
Issue: redirect_latency = 1200ms (expected <100ms)

Timeline:
  14:00 - 14:20: 50ms (normal)
  14:21: 200ms (starting to slow)
  14:22: 500ms (degraded)
  14:23: 1200ms (critical) ← ALERT FIRED

Action: Investigate EU-West infrastructure
```

---

#### **Method 2: Real User Monitoring (RUM)**

**What it is:** Track actual user experience from their browsers/apps.

**How it works:**

**Browser-side instrumentation:**

```
When user clicks short URL:
  
  1. Browser JavaScript records:
     - DNS lookup time
     - TCP connection time
     - TLS handshake time
     - Time to first byte (TTFB)
     - Total page load time
     - User's location (IP geolocation)
     - User's device (mobile/desktop)
     - User's browser (Chrome/Safari/Firefox)
  
  2. Browser sends beacon to analytics endpoint:
     POST /api/metrics/rum
     {
       "url": "tiny.url/abc123",
       "ttfb": 150,
       "total_time": 250,
       "location": "US-CA",
       "device": "mobile"
     }
  
  3. Analytics service aggregates:
     - P95 TTFB by region
     - P99 total time by device
     - Slowest pages for users
```

**Dashboard showing RUM data:**

```
Real User Performance (Last Hour)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

By Region:
  US:     P95 = 50ms  ✓
  EU:     P95 = 1200ms ❌ ← Problem in EU!
  Asia:   P95 = 300ms ⚠️  (Expected - far from servers)

By Device:
  Desktop: P95 = 60ms ✓
  Mobile:  P95 = 150ms ✓ (Mobile networks slower, expected)

By Browser:
  Chrome:  P95 = 70ms ✓
  Safari:  P95 = 65ms ✓
  Firefox: P95 = 80ms ✓
```

**Insight:** EU users experiencing 10x slower redirects!

---

#### **Method 3: Distributed Tracing**

**What it is:** Track a single request through all system components to find bottlenecks.

**Tools:** Jaeger, Zipkin, AWS X-Ray

**How it works:**

```
Each request gets a unique Trace ID

Request: GET /abc123
Trace ID: trace_xyz789

Flow through system:
┌─────────────────────────────────────────────────────────┐
│ [Load Balancer]         Span ID: span_1                 │
│ Duration: 2ms                                           │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ [Service Instance 3]    Span ID: span_2                 │
│ Duration: 48ms (TOTAL)                                  │
│   ├─ Decode short code: 0.1ms (span_2.1)               │
│   ├─ Check rate limit: 1ms (span_2.2)                  │
│   ├─ Query Redis cache: 45ms (span_2.3) ❌ SLOW!       │
│   └─ Return redirect: 0.1ms (span_2.4)                 │
└──────────────────────────────────────────────────────────┘

Total request time: 50ms
Bottleneck: Redis query took 45ms (90% of time!)
```

**Trace visualization (Jaeger UI):**

```
Trace: trace_xyz789 (Total: 50ms)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Load Balancer     [██] 2ms
                   ↓
Service Instance  [████████████████████████████████] 48ms
  ├─ Decode       [░] 0.1ms
  ├─ Rate Limit   [█] 1ms
  ├─ Redis Cache  [████████████████████████] 45ms ← BOTTLENECK!
  └─ Return       [░] 0.1ms

Root Cause: Redis slow
Next Step: Investigate Redis cluster
```

---

**Drill down into Redis span:**

```
Span: Redis GET operation (span_2.3)
Duration: 45ms

Breakdown:
  Network latency to Redis: 1ms
  Redis processing time: 2ms
  Network latency back: 1ms
  Waiting in connection pool: 41ms ❌ ← REAL PROBLEM!

Root cause: Connection pool exhausted!
  Max connections: 50
  Active connections: 50
  Requests waiting for connection: 200

Solution: Increase connection pool size to 100
```

---

#### **Method 4: Database Query Analysis**

**Slow Query Log:**

```
PostgreSQL slow query log (queries >50ms):

[2025-11-29 14:23:15] Duration: 2500ms
Query: SELECT long_url FROM url_mappings WHERE id = 123456789

Execution plan:
  Seq Scan on url_mappings (cost=0.00..500000.00)
    Filter: (id = 123456789)
  
Problem: Sequential scan (no index!) ❌
Expected: Index scan (fast)

Investigation:
  1. Check indexes: \d url_mappings
     Result: PRIMARY KEY index exists on id
  
  2. Check table stats: ANALYZE url_mappings
     Result: Stats outdated (last updated 30 days ago)
  
  3. Run: VACUUM ANALYZE url_mappings
     Result: Stats updated, index now used
  
  4. Retest query: Duration: 5ms ✓

Root cause: Outdated statistics caused query planner to ignore index
```

---

#### **Systematic Investigation Process:**

**Step 1: Identify the symptom**

```
User report: "Redirects slow"

Check dashboards:
  ✓ Average latency: 50ms (looks normal)
  ❌ P99 latency: 2000ms (PROBLEM!)
  
Conclusion: Small percentage of requests very slow
```

---

**Step 2: Narrow down when/where**

```
Questions to answer:
  - When did it start? (timeline)
  - Which region? (geography)
  - Which endpoint? (create vs redirect)
  - Which users? (all vs specific IPs)

Check metrics:
  Timeline: Started at 14:00 today
  Region: Only EU users affected
  Endpoint: Only redirects (GET /{shortCode})
  Users: All EU users

Hypothesis: EU infrastructure issue
```

---

**Step 3: Check dependencies**

```
External dependencies health:
  ✓ Redis: Latency 1ms, hit rate 95% (normal)
  ✓ Database: Latency 10ms, no slow queries
  ✓ Kafka: Consumer lag 1000 (normal)
  ❌ CDN: EU edge nodes reporting errors

Found: Cloudflare EU edges having connectivity issues

Action: 
  1. Temporarily route EU traffic to US edges
  2. Contact Cloudflare support
  3. Monitor for improvement
```

---

**Step 4: Look for recent changes**

```
Check deployment history:
  - Last deploy: 2 days ago (no issues since then)
  - Config changes: None
  - Infrastructure changes: Added EU Redis replica 6 hours ago

Check new EU Redis replica:
  - Status: Running
  - Latency: 500ms ❌ (should be <5ms)
  - Network: 50% packet loss to replica

Root cause: New Redis replica has network misconfiguration

Action:
  1. Remove unhealthy replica from rotation
  2. Route EU traffic to working replica
  3. Fix network config on problematic replica
  4. Re-add when healthy
```

---

**Step 5: Verify fix**

```
After applying fix:
  Wait 5 minutes for metrics to update
  
Check dashboards:
  ✓ P99 latency: 80ms (back to normal!)
  ✓ RUM shows EU users: 50ms average
  ✓ No alerts firing
  
Synthetic checks:
  ✓ EU-West: PASS (redirect in 45ms)
  ✓ EU-East: PASS (redirect in 50ms)

User reports:
  ✓ No new complaints
  ✓ Resolved tickets from earlier

Conclusion: Issue resolved!
```

---

#### **Performance Regression Detection:**

**Automated performance testing:**

```
On every deployment:

Pre-deployment baseline:
  - Run load test: 5000 req/sec for 10 minutes
  - Measure: P95 = 50ms, P99 = 80ms
  
Deploy new version

Post-deployment test:
  - Run same load test
  - Measure: P95 = 50ms, P99 = 200ms ❌
  
Performance regression detected!
  P99 latency increased by 150% (80ms → 200ms)

Action:
  1. Auto-rollback deployment
  2. Alert team: "Deployment abc123 caused performance regression"
  3. Investigate code changes in deployment
```

**Finding the bad code:**

```
Changed files in deployment:
  - cache_client.py (modified)
  - url_validator.py (new)
  - database.py (no changes)

Suspect: cache_client.py

Diff review:
  - Line 45: Added timeout=5 (was timeout=0.1)
  
Root cause: Increased Redis timeout from 100ms to 5 seconds!
  Impact: Slow Redis queries now block for 5 seconds instead of failing fast

Fix: Revert timeout to 0.1 seconds
Test: P99 latency back to 80ms ✓
Deploy: Fixed version
```

---

### 📊 Key Takeaways

1. **Don't trust averages:** Always check P95, P99, max latency

2. **Synthetic monitoring:** Proactive checks from multiple regions

3. **RUM:** Track real user experience (device, browser, location)

4. **Distributed tracing:** Find bottlenecks in multi-component flows

5. **Slow query logs:** Catch database performance issues

6. **Recent changes:** Check deployments, config, infrastructure

7. **Verify fixes:** Always confirm metrics improved after fix

---

### 🔗 Further Reading

- **"Observability Engineering" by Charity Majors:** Modern monitoring practices
- **Jaeger documentation:** Distributed tracing deep dive
- **DataDog RUM guide:** Real user monitoring best practices

---

### ✏️ Practice Exercise

**Scenario:** After a deployment, you see these metrics:

```
Before deployment:
  P50: 10ms, P95: 50ms, P99: 80ms
  
After deployment:
  P50: 10ms, P95: 50ms, P99: 5000ms

Error rate: No change (still 0.01%)
Traffic: No change (4000 req/sec)
```

**Questions:**
1. Would you roll back? Why/why not?
2. How do you find the root cause?
3. What code change could cause this pattern?

<details>
<summary>Click to see answer</summary>

**1. Should you roll back?**

**YES, immediately!** Here's why:

```
Impact analysis:
  - P99 = 5000ms means 1% of requests taking 5 seconds
  - 1% of 4000 req/sec = 40 requests/sec affected
  - 40 requests/sec × 3600 sec/hour = 144,000 unhappy users/hour!
  
Even though 99% of requests are fine, 1% is significant at this scale.
```

**Rollback decision tree:**
```
Is P99 > 10x normal? (5000ms vs 80ms = 62x) → YES
Are users affected? (40/sec) → YES
Is error rate normal? (yes, but latency matters too) → Doesn't matter

Decision: ROLLBACK
```

---

**2. Finding root cause:**

**Step A: Check distributed traces for slow requests**

```
Filter traces: duration > 1000ms

Sample slow trace (5200ms total):
  Load Balancer: 2ms
  Service: 5198ms
    ├─ Decode: 0.1ms
    ├─ Rate limit: 1ms
    ├─ Cache check: 2ms (cache miss)
    ├─ DB query: 15ms
    ├─ External API call: 5180ms ❌ ← NEW!
    └─ Return: 0.1ms

Found: New external API call taking 5 seconds!
```
**Step B: Code diff review**

```
New code in deployment:
  
  function get_long_url(id):
      long_url = cache.get(id) or db.query(id)
      
      // NEW CODE:
      if is_malicious_url(long_url):  ← Added malicious check
          return error("URL blocked")
      
      return long_url
  
  function is_malicious_url(url):
      response = google_safe_browsing_api.check(url)  ← External API!
      return response.is_malicious
```

**Root cause identified:**
```
Added Google Safe Browsing API call in synchronous path!
- API timeout: 5 seconds
- Cache miss rate: 5%
- 5% of requests call API → 5% hit 5-second timeout
- This explains P99 regression (1% timeout, 4% fast API response)
```

---

**3. What code changes cause this pattern?**

**Pattern: P50/P95 normal, P99 terrible**

**Common causes:**

**A. New external API call (this case):**
```
99% of time: API responds in 100ms
1% of time: API times out at 5 seconds

Result: P99 = 5000ms
```

**B. Database connection pool exhausted:**
```
99% of time: Get connection immediately
1% of time: Wait for connection (up to 5 seconds)

Result: P99 = 5000ms
```

**C. Rare code path with expensive operation:**
```
99% of requests: Normal flow (10ms)
1% of requests: Trigger expensive regex (5000ms)

Result: P99 = 5000ms
```

**D. Cache stampede:**
```
Popular URL expires from cache
Next 100 requests hit cache miss simultaneously
All query DB at once
DB overloaded → slow queries

Result: P99 spike during cache refresh
```

---

**Fix for this specific case:**

**Option 1: Make API call async**
```
function get_long_url(id):
    long_url = cache.get(id) or db.query(id)
    
    // Check malicious in background
    async_check_malicious(long_url)
    
    return long_url  // Don't block user!

// If found malicious later, mark URL as blocked
```

**Option 2: Cache API results**
```
function is_malicious_url(url):
    cached = cache.get("malicious:" + url)
    if cached:
        return cached
    
    response = google_safe_browsing_api.check(url, timeout=100ms)
    cache.set("malicious:" + url, response, ttl=1hour)
    return response
```

**Option 3: Use bloom filter pre-check**
```
function is_malicious_url(url):
    // Fast bloom filter check
    if not bloom_filter.might_be_malicious(url):
        return false  // Definitely safe, no API call!
    
    // Only call API for suspicious URLs
    return google_safe_browsing_api.check(url)
```

**Best solution: Combine all three!**
- Pre-check with bloom filter (fast)
- Cache API results (avoid repeated calls)
- Make checks async when possible (don't block user)

</details>

---

## **2.3 Alerting Strategy & SLOs**

### 🤔 Interview Question/Context

**Interviewer:** "Your monitoring system detected an issue at 2 AM. How do you decide if it's worth waking up an engineer?"

### ❌ Your Initial Answer

You didn't provide an alerting strategy initially - just mentioned "alterting" as a feature of monitoring tools.

**What was missing:**
- Which issues warrant immediate page vs email
- How to avoid alert fatigue
- Service Level Objectives (SLOs) to define "healthy"

---

### ✅ Complete Answer

#### **Alert Severity Levels:**

```
P0 - CRITICAL (Page immediately, 24/7)
  Impact: Service down or severely degraded
  User impact: Most/all users affected
  Response: Page on-call engineer NOW
  Example: "Service returning 500 errors for 90% of requests"

P1 - HIGH (Page during business hours, escalate after hours)
  Impact: Partial service degradation
  User impact: Some users affected
  Response: Page if not resolved in 15 minutes
  Example: "P99 latency > 1 second for 10 minutes"

P2 - MEDIUM (Slack notification)
  Impact: Warning signs, might become critical
  User impact: Minimal current impact
  Response: Team reviews during business hours
  Example: "Cache hit rate dropped to 85%"

P3 - LOW (Email digest)
  Impact: FYI, no immediate action needed
  User impact: None
  Response: Weekly review
  Example: "Disk usage reached 70%"
```

---

#### **Service Level Objectives (SLOs):**

**What is an SLO?**

```
SLO = Service Level Objective
  "Target for how well our service should perform"

Example SLO:
  "99.9% of redirect requests complete in under 100ms"

Components:
  - SLI (Service Level Indicator): What we measure (latency)
  - Target: 99.9% (3 nines)
  - Threshold: 100ms
  - Time window: Rolling 30 days
```

---

### 🎯 URL Shortener Context

#### **Defining SLOs for URL Shortener:**

**SLO 1: Availability**

```
Objective: 99.9% of requests succeed

Translation:
  - Success = HTTP 200, 201, 302 (not 5xx errors)
  - 99.9% = 99.9% uptime
  - Downtime budget: 0.1% = 43 minutes/month

Measurement:
  successful_requests / total_requests ≥ 0.999

Alert:
  If success rate < 99.9% for 5 minutes → Page on-call
```

**Monthly error budget:**

```
Total requests/month: 10 billion
Error budget (0.1%): 10 million requests

If we've "spent" error budget:
  Week 1: 2 million errors (20% spent)
  Week 2: 3 million errors (50% spent)
  Week 3: 4 million errors (90% spent)
  Week 4: 1 million left ← Freeze deploys! Too risky!

Rule: If >80% error budget spent, stop non-critical changes
```

---

**SLO 2: Latency (Redirects)**

```
Objective: 99% of redirect requests complete in under 100ms

Why 99%, not 99.9%?
  - Tighter than availability (100ms is aggressive)
  - Some slow requests acceptable (DB query, network blip)
  
Measurement:
  P99 latency ≤ 100ms (rolling 30-day window)

Alert:
  If P99 > 100ms for 10 minutes → Slack notification (P2)
  If P99 > 500ms for 5 minutes → Page on-call (P1)
```

---

**SLO 3: Latency (Create URL)**

```
Objective: 95% of create requests complete in under 500ms

Why looser than redirects?
  - Creating URL less time-sensitive
  - Involves more operations (validation, DB write, etc.)
  - Lower volume (40/sec vs 4000/sec)

Measurement:
  P95 latency ≤ 500ms

Alert:
  If P95 > 500ms for 15 minutes → Slack notification (P2)
  If P95 > 2000ms for 5 minutes → Page on-call (P1)
```

---

**SLO 4: Data Durability**

```
Objective: Zero data loss for created URLs

Translation:
  - Every created URL must be persisted
  - Database backups must be restorable
  - No silent data corruption

Measurement:
  - Daily backup verification: Restore and spot-check 1000 URLs
  - Database consistency check: SELECT COUNT(*) matches expected

Alert:
  If backup fails → Page immediately (P0)
  If data corruption detected → Page immediately (P0)
```

---

#### **Alert Rules with SLOs:**

**Rule 1: High error rate (SLO breach)**

```
Alert: ServiceUnavailable
Severity: P0 (CRITICAL)

Condition:
  (
    rate(http_requests_total{status_code=~"5.."}[5m]) 
    / rate(http_requests_total[5m])
  ) > 0.001

Meaning:
  Error rate > 0.1% for 5 minutes
  → Violating 99.9% availability SLO

Action:
  1. Page on-call engineer
  2. Auto-create incident in PagerDuty
  3. Post to #incidents Slack channel
  4. Run automated diagnostics script

Runbook:
  1. Check service logs for stack traces
  2. Check database connection pool
  3. Check Redis connectivity
  4. Check recent deployments (rollback if needed)
```

---

**Rule 2: Latency degradation (approaching SLO breach)**

```
Alert: LatencyHigh
Severity: P1 (HIGH)

Condition:
  histogram_quantile(0.99, 
    rate(http_request_duration_seconds_bucket{endpoint="/{shortCode}"}[5m])
  ) > 0.1

Meaning:
  P99 latency > 100ms for 5 minutes
  → Violating latency SLO

Action:
  1. Page during business hours (8 AM - 6 PM)
  2. Auto-escalate to P0 if not resolved in 15 minutes
  3. Slack notification to #reliability

Runbook:
  1. Check cache hit rate (should be >95%)
  2. Check database slow query log
  3. Check Redis latency
  4. Review distributed traces for slow requests
```

---

**Rule 3: Leading indicator (prevent SLO breach)**

```
Alert: CacheHitRateLow
Severity: P2 (MEDIUM)

Condition:
  cache_hit_rate < 0.90

Meaning:
  Cache hit rate < 90%
  → Will cause latency increase soon
  → Proactive warning before SLO breach

Action:
  1. Slack notification only
  2. No page (not urgent yet)
  3. Team investigates during business hours

Runbook:
  1. Check cache memory usage
  2. Check cache eviction rate
  3. Consider increasing cache size
  4. Review TTL settings
```

---

#### **Alert Fatigue Prevention:**

**Problem: Too many alerts = ignored alerts**

```
Bad alerting:
  100 alerts/day → Team ignores them → Miss critical alert

Good alerting:
  2-3 alerts/week → Team responds quickly → Issues resolved fast
```

**Strategies to reduce noise:**

**1. Alert on symptoms, not causes:**

```
❌ BAD: Alert on "Redis CPU > 80%"
   Why: High CPU might be normal during traffic spike
   Result: False alarms

✓ GOOD: Alert on "P99 latency > 100ms"
   Why: This is user-facing impact (SLO breach)
   Result: Only alerts when users affected
```

---

**2. Use multi-window alerting:**

```
❌ BAD: Alert immediately when latency spikes
   Result: Alert on every transient blip

✓ GOOD: Alert if latency high for 5 minutes
   Why: Filters out momentary spikes
   Result: Fewer false positives

Even better: Alert if latency high for 5 of last 10 minutes
   Why: Allows brief recovery without canceling alert
```

---

**3. Group related alerts:**

```
❌ BAD: 
   Alert 1: "Service A latency high"
   Alert 2: "Service B latency high"
   Alert 3: "Service C latency high"
   → 3 pages for same root cause (database slow)

✓ GOOD: 
   Alert: "Database latency high"
   Affected services: A, B, C
   → 1 page with full context
```

---

**4. Different channels for different severity:**

```
P0: PagerDuty → Phone call + SMS
P1: PagerDuty → SMS only
P2: Slack → #alerts channel
P3: Email → Daily digest

Result: Engineers only woken up for true emergencies
```

---

**5. Self-healing before alerting:**

```
Issue: Redis replica unhealthy

Option A (alert immediately):
  1. Detect unhealthy replica
  2. Alert team
  3. Engineer manually removes replica
  4. 15 minutes to resolution

Option B (auto-remediate):
  1. Detect unhealthy replica
  2. Auto-remove from rotation
  3. Log event
  4. Alert only if auto-remediation fails
  5. 30 seconds to resolution, no human needed

Result: 90% of issues self-heal, alerts only for complex problems
```

---

#### **On-Call Runbooks:**

**What is a runbook?**

```
Runbook = Step-by-step guide for responding to an alert

Good runbook structure:
  1. Alert description
  2. Impact assessment
  3. Investigation steps
  4. Common causes
  5. Resolution steps
  6. Escalation path
```

**Example runbook:**

```
═══════════════════════════════════════════════════════════
RUNBOOK: High Error Rate Alert
═══════════════════════════════════════════════════════════

ALERT: ServiceUnavailable
SEVERITY: P0 (Critical)
SLO IMPACT: Availability SLO breached

SYMPTOM:
  Error rate > 0.1% (SLO: 99.9% success)
  Users seeing 500/503 errors

IMPACT:
  - X% of requests failing (check dashboard)
  - Y users affected per minute
  - Revenue impact: $Z/hour (if applicable)

INVESTIGATION (5 minutes max):

  Step 1: Check error logs
    Command: kubectl logs -l app=urlshortener --tail=100
    Look for: Stack traces, repeated errors
  
  Step 2: Check dependencies
    - Redis: Check /health endpoint
    - Database: Check connection count
    - Kafka: Check consumer lag
  
  Step 3: Check recent changes
    - Last deployment: Check CI/CD system
    - Config changes: Check version control
    - Infrastructure: Check cloud provider status

COMMON CAUSES & FIXES:

  Cause 1: Database connection pool exhausted
    Symptom: Errors like "could not acquire connection"
    Fix: Increase pool size in config, restart service
    Command: kubectl set env deployment/urlshortener DB_POOL_SIZE=100
  
  Cause 2: Redis cluster down
    Symptom: Errors like "redis timeout"
    Fix: Service should failover to DB, check Redis health
    Command: redis-cli -h redis-master ping
  
  Cause 3: Bad deployment
    Symptom: Errors started after recent deploy
    Fix: Rollback to previous version
    Command: kubectl rollout undo deployment/urlshortener

RESOLUTION:

  If fixed:
    1. Monitor for 10 minutes to confirm
    2. Update incident ticket
    3. Schedule post-mortem
  
  If not fixed in 15 minutes:
    1. Escalate to senior engineer
    2. Consider rolling back to last known good state
    3. Notify stakeholders in #incidents

POST-INCIDENT:
  - Write post-mortem (within 48 hours)
  - Identify prevention measures
  - Update runbook with learnings
```

---

#### **Alert Review Process:**

**Weekly alert review meeting:**

```
Agenda:
  1. Review all alerts from past week
  2. Categorize each alert:
     - True positive (legitimate issue)
     - False positive (alert fired incorrectly)
     - Expected (known issue, planned maintenance)
  
  3. For false positives:
     - Why did it fire incorrectly?
     - Adjust threshold or condition
     - Goal: Reduce false positive rate to <5%
  
  4. For true positives:
     - Was runbook helpful?
     - Update runbook with actual resolution steps
     - Consider automation to prevent recurrence

Metrics:
  - Total alerts: 15
  - True positives: 12 (80%)
  - False positives: 3 (20%) ← Too high, fix these!
  - Avg time to resolution: 8 minutes
```

---

### 📊 Key Takeaways

1. **SLOs define "healthy":** Set clear targets (99.9% availability, P99 <100ms)

2. **Error budget:** Track how much "failure" you can afford

3. **Severity levels:** P0 = page now, P1 = page business hours, P2 = Slack, P3 = email

4. **Alert on symptoms:** User-facing impact, not internal metrics

5. **Prevent fatigue:** 2-3 alerts/week is healthy, 100/day is broken

6. **Runbooks:** Step-by-step guides for on-call engineers

7. **Self-healing:** Auto-remediate common issues, alert only if fails

---

### 🔗 Further Reading

- **"Site Reliability Engineering" (Google):** Chapter on SLOs and error budgets
- **"Implementing Service Level Objectives":** SLO practical guide
- **PagerDuty runbook templates:** Real-world examples

---

### ✏️ Practice Exercise

**Scenario:** Your URL shortener has these SLOs:
- Availability: 99.9%
- P99 latency: 100ms

**Monthly budget:**
- 10 billion requests
- Error budget: 10 million failures
- Latency budget: 100 million slow requests

**This month so far (Day 20 of 30):**
- 7 billion requests processed
- 8 million failures
- 85 million slow requests

**Questions:**
1. Are you on track to meet SLOs?
2. Should you freeze deployments?
3. A critical security patch needs to be deployed - do you deploy?

<details>
<summary>Click to see answer</summary>

**1. Are you on track?**

**Availability SLO analysis:**

```
Projected requests by end of month:
  Current rate: 7B / 20 days = 350M/day
  Days remaining: 10
  Projected total: 7B + (350M × 10) = 10.5B requests

Projected failures:
  Current rate: 8M / 20 days = 400K failures/day
  Projected by end of month: 8M + (400K × 10) = 12M failures

Error budget: 10M failures allowed
Projected failures: 12M
Status: ❌ WILL BREACH SLO by 2M failures (20% over budget)
```

**Latency SLO analysis:**

```
Projected slow requests by end of month:
  Current rate: 85M / 20 days = 4.25M slow/day
  Projected: 85M + (4.25M × 10) = 127.5M slow requests

Latency budget: 100M slow requests allowed
Projected: 127.5M
Status: ❌ WILL BREACH SLO by 27.5M (27.5% over budget)
```

---

**2. Should you freeze deployments?**

**YES, freeze non-critical deployments!**

```
Reasoning:
  - Already spent 80% of availability error budget
  - Already spent 85% of latency budget
  - 10 days left in month
  - Any deployment risk could breach SLO

Actions:
  1. Freeze all feature deployments
  2. Allow only:
     - Critical bug fixes
     - Security patches
     - Performance improvements
  3. Investigate why error/latency rates high
  4. Focus on improving reliability

Example communication:
  "We're in SLO freeze until end of month. Current error rate
   trending toward SLO breach. Only P0/P1 fixes allowed."
```

---

**3. Deploy critical security patch?**

**YES, but with extra caution!**

```
Decision framework:

Security risk:
  - Severity: Critical (RCE vulnerability)
  - Exploitation: Actively being exploited in the wild
  - Impact if not patched: Complete system compromise

SLO risk:
  - Deployment might cause errors
  - Already close to SLO breach
  - Could push us over budget

Trade-off:
  Security breach > SLO breach
  → Deploy the patch!

Risk mitigation:
  1. Deploy during low-traffic window (3 AM)
  2. Canary deployment:
     - Deploy to 1% of instances
     - Monitor for 30 minutes
     - If no errors, proceed to 10%, 50%, 100%
  3. Have rollback plan ready
  4. Extra monitoring during deployment
  5. On-call engineer standing by

If deployment causes errors:
  - Count against error budget
  - Document in post-mortem
  - Trade-off was worth it (security > SLO)
```

---

**4. How to get back on track for next month?**

```
Actions to improve reliability:

Short-term (this week):
  1. Investigate top error causes:
     - Run: Top 10 errors by frequency
     - Fix quick wins (input validation, timeout tuning)
  
  2. Optimize P99 latency:
     - Increase cache hit rate
     - Add database indexes
     - Increase connection pool sizes

Long-term (next month):
  1. Add more automated tests
  2. Improve deployment process (better canaries)
  3. Add chaos engineering (find issues proactively)
  4. Review and tighten SLOs if too aggressive

Goal for next month:
  - Stay under 50% error budget usage
  - Build buffer for unexpected issues
```

</details>

---

## **2.4 Monitoring Tool Stack**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned Prometheus, Grafana, Kibana, DataDog. How do these tools fit together? What does each one do?"

### ❌ Your Initial Answer

You listed tool names but weren't sure how they work together or which to choose.

**The confusion:** Each tool has a specific role in the observability stack.

---

### ✅ Complete Answer

#### **The Three Pillars of Observability:**

```
1. METRICS (numbers over time)
   - What: Counters, gauges, histograms
   - Example: "Requests per second", "P99 latency"
   - Tools: Prometheus, DataDog, CloudWatch

2. LOGS (text events)
   - What: Application logs, error messages
   - Example: "ERROR: Database connection failed at 14:23:05"
   - Tools: Elasticsearch, Splunk, CloudWatch Logs

3. TRACES (request flow)
   - What: Path of a request through system
   - Example: "Request took 50ms: 2ms load balancer + 45ms Redis + 3ms service"
   - Tools: Jaeger, Zipkin, DataDog APM
```

---

### 🎯 URL Shortener Context

#### **Recommended Stack (Open Source):**

```
┌──────────────────────────────────────────────────────────┐
│                    OBSERVABILITY STACK                    │
└──────────────────────────────────────────────────────────┘

1. METRICS PIPELINE:
   [Service] → exports metrics at /metrics endpoint
        ↓
   [Prometheus] → scrapes /metrics every 15 seconds
        ↓
   [Prometheus] → stores time-series data
        ↓
   [Grafana] → queries Prometheus, displays dashboards

2. LOGS PIPELINE:
   [Service] → writes logs to stdout
        ↓
   [Fluentd] → collects logs from all containers
        ↓
   [Elasticsearch] → stores & indexes logs
        ↓
   [Kibana] → searches/visualizes logs

3. TRACES PIPELINE:
   [Service] → sends trace spans to Jaeger
        ↓
   [Jaeger Collector] → receives spans
        ↓
   [Jaeger Storage] → stores traces
        ↓
   [Jaeger UI] → visualizes request flows
```

---

#### **Tool Details:**

**1. Prometheus (Metrics Collection & Storage)**

**What it does:**
- Scrapes metrics from services
- Stores time-series data
- Runs alert rules
- Provides query language (PromQL)

**How services expose metrics:**

```
Service exposes /metrics endpoint:

GET /metrics

Response:
# HELP http_requests_total Total HTTP requests
# TYPE http_requests_total counter
http_requests_total{endpoint="/api/v1/urls",method="POST",status="201"} 15234

# HELP http_request_duration_seconds HTTP request latency
# TYPE http_request_duration_seconds histogram
http_request_duration_seconds_bucket{endpoint="/{shortCode}",le="0.01"} 9500
http_request_duration_seconds_bucket{endpoint="/{shortCode}",le="0.05"} 9800
http_request_duration_seconds_bucket{endpoint="/{shortCode}",le="0.1"} 9950
http_request_duration_seconds_sum{endpoint="/{shortCode}"} 125.5
http_request_duration_seconds_count{endpoint="/{shortCode}"} 10000
```

**Prometheus configuration:**

```yaml
# prometheus.yml

scrape_configs:
  - job_name: 'urlshortener'
    scrape_interval: 15s  # Scrape every 15 seconds
    static_configs:
      - targets:
        - 'service-1:8080'
        - 'service-2:8080'
        - 'service-3:8080'

  - job_name: 'redis'
    scrape_interval: 30s
    static_configs:
      - targets:
        - 'redis-exporter:9121'

  - job_name: 'postgres'
    scrape_interval: 30s
    static_configs:
      - targets:
        - 'postgres-exporter:9187'
```

---

**2. Grafana (Visualization & Dashboards)**

**What it does:**
- Connects to Prometheus (and other data sources)
- Creates dashboards with graphs
- No data storage (just visualization)

**Sample dashboard panels:**

```
Dashboard: URL Shortener Overview

Panel 1: Request Rate
  Query: rate(http_requests_total[1m])
  Visualization: Line graph
  Y-axis: Requests/second
  
Panel 2: Error Rate
  Query: 
    rate(http_requests_total{status_code=~"5.."}[1m]) 
    / rate(http_requests_total[1m]) * 100
  Visualization: Line graph
  Y-axis: Error percentage
  Alert: Red if > 0.1%

Panel 3: P99 Latency
  Query: 
    histogram_quantile(0.99, 
      rate(http_request_duration_seconds_bucket[5m]))
  Visualization: Line graph with threshold line at 100ms
  
Panel 4: Cache Hit Rate
  Query: 
    rate(cache_hits[1m]) 
    / (rate(cache_hits[1m]) + rate(cache_misses[1m])) * 100
  Visualization: Gauge (showing current %)
  Green: >95%, Yellow: 90-95%, Red: <90%
```

---

**3. ELK Stack (Logs)**

**Elasticsearch:**
- Stores logs
- Indexes for fast search
- Distributed (can handle billions of logs)

**Logstash / Fluentd:**
- Collects logs from services
- Parses log format
- Enriches logs (add tags, geo-location)
- Sends to Elasticsearch

**Kibana:**
- Searches logs
- Creates log dashboards
- Visualizes patterns

---

**Log format (structured JSON):**

```json
{
  "timestamp": "2025-11-29T14:23:45.123Z",
  "level": "ERROR",
  "service": "urlshortener",
  "instance": "service-2",
  "trace_id": "abc123",
  "user_id": "user_456",
  "endpoint": "/api/v1/urls",
  "method": "POST",
  "status_code": 500,
  "error": "Database connection timeout",
  "stack_trace": "...",
  "duration_ms": 5000
}
```

**Kibana search queries:**

```
Find all errors in last hour:
  level:ERROR AND timestamp:[now-1h TO now]

Find slow requests:
  duration_ms:>1000

Find errors for specific user:
  user_id:"user_456" AND level:ERROR

Find database errors:
  error:*database* OR error:*connection*
```

---

**4. Jaeger (Distributed Tracing)**

**What it does:**
- Tracks request flow through system
- Shows which component is slow
- Helps debug complex microservice issues

**How it works:**

```
Service instruments code:

function handle_redirect(short_code):
    span = tracer.start_span("handle_redirect")
    
    child_span_1 = tracer.start_span("decode_short_code", parent=span)
    id = decode(short_code)
    child_span_1.finish()
    
    child_span_2 = tracer.start_span("query_cache", parent=span)
    url = redis.get(f"url:{id}")
    child_span_2.finish()
    
    if not url:
        child_span_3 = tracer.start_span("query_database", parent=span)
        url = db.query("SELECT long_url FROM urls WHERE id = ?", id)
        child_span_3.finish()
    
    span.finish()
    return url
```

**Jaeger UI shows:**

```
Trace ID: abc123xyz (Total: 48ms)

├─ handle_redirect (48ms)
   ├─ decode_short_code (0.1ms)
   ├─ query_cache (45ms) ← Slow!
   └─ (cache miss, so:)
      └─ query_database (2ms)

Conclusion: Redis query slow, investigate Redis cluster
```

---

#### **Alternative: All-in-One Solution (DataDog)**

**DataDog pros:**
- ✅ Single tool for metrics, logs, traces
- ✅ Fully managed (no ops burden)
- ✅ Great UI
- ✅ Easy setup
- ✅ Built-in alerting, dashboards

**DataDog cons:**
- ❌ Expensive ($15-50/host/month)
- ❌ Vendor lock-in
- ❌ Costs scale with data volume

---

**Cost comparison (for URL shortener with 10 service instances):**

```
Open Source Stack:
  - Prometheus: Free (self-hosted on 1 VM)
  - Grafana: Free (self-hosted)
  - ELK: Free (self-hosted on 3 VMs)
  - Jaeger: Free (self-hosted on 1 VM)
  
  Infrastructure cost: 5 VMs × $50/month = $250/month
  Engineering time: 20 hours/month maintenance = $2000/month
  Total: $2250/month

DataDog:
  - 10 hosts × $15/host/month = $150/month (base)
  - Logs: 100GB/month × $1.70/GB = $170/month
  - APM: 10 hosts × $31/host/month = $310/month
  
  Total: $630/month
  Engineering time: 2 hours/month = $200/month
  Total: $830/month

Winner: DataDog is cheaper AND less work!
```

**When to use open source vs DataDog:**

```
Use DataDog if:
  - Small team (<20 engineers)
  - Need to move fast
  - Budget allows ($500-5000/month)

Use Open Source if:
  - Large team (can dedicate engineers to ops)
  - Very high data volume (DataDog gets expensive)
  - Compliance requires data on-prem
  - Need customization
```

---

### 📊 Key Takeaways

1. **Three pillars:** Metrics (Prometheus), Logs (ELK), Traces (Jaeger)

2. **Prometheus + Grafana :** Standard for metrics & dashboards

3. **ELK stack:** Powerful log search & analysis

4. **Jaeger:** Distributed tracing for debugging

5. **DataDog:** All-in-one alternative (easier but costly)

6. **For URL shortener at your scale:** DataDog is probably best choice

7. **Structured logging:** Use JSON format for easy parsing

---

### 🔗 Further Reading

- **Prometheus documentation:** Best practices for instrumentation
- **Grafana tutorials:** Building effective dashboards
- **DataDog blog:** Monitoring patterns

---

# **End of Section 2** ✅

---

## **Section 2 Completion Checklist:**

- ☑ **2.1 Key Metrics (RED/USE)** - Know what to measure
- ☑ **2.2 Detecting Performance Issues** - Systematic debugging
- ☑ **2.3 Alerting & SLOs** - When to page engineers
- ☑ **2.4 Monitoring Tool Stack** - How tools fit together

**Progress:** Section 2 complete! (8/32 topics = 25%)

---

**Excellent progress! You've completed 25% of the study guide.** 🎉

**Would you like to:**
1. Continue to **Section 3** (Disaster Recovery & High Availability)
2. Take a break and review Section 2
3. Do practice exercises from Section 2
4. Jump to a different section

Let me know! 🚀