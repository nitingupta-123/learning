# Database Schema Design - Complete Interview Guide (Q&A Format)

---

## 🎯 What This Document Covers
- ✅ 10M+ notifications/day scale
- ✅ Multi-channel (Email, SMS, Push, In-app)
- ✅ Complex user preferences
- ✅ Template versioning
- ✅ Deduplication
- ✅ Retry logic with DLQ
- ✅ Full audit trail
- ✅ Analytics and monitoring

---

## PART 1: CORE ENTITIES (The Foundation)

### Q1: What are the main entities we need to store?

**Answer:**
We need 7 core entities:

1. **Users** - Who receives notifications
2. **Devices** - Where push notifications go
3. **Notification Types** - What kinds of notifications exist
4. **User Preferences** - What each user wants
5. **Templates** - Reusable message formats
6. **Notification Logs** - History of all notifications
7. **Dead Letter Queue** - Failed notifications

**Visual Relationship:**
```
Users (1) ──→ Devices (Many)
Users (1) ──→ Preferences (Many)
Notification Types (1) ──→ Templates (Many)
Notification Types (1) ──→ Preferences (Many)
```

---

### Q2: How do we store user information? What's the challenge?

**Challenge:** 
Users can have:
- Multiple emails (personal, work)
- Multiple phone numbers
- Different timezones (for DND hours)
- Language preferences (for templates)

**Bad Approach:**
```
Users table with: user_id, email, phone
Problem: Can only store ONE email, ONE phone ❌
```

**Better Approach:**
Main Users table with basic info, then separate tables:
- `user_emails` table: user can have many emails
- `user_phones` table: user can have many phones

**Why separate tables?**
- ✅ Supports multiple contacts per user
- ✅ Can mark one as "primary"
- ✅ Can track verification status per contact
- ✅ Easy to add/remove without affecting user record

**Fields needed in Users:**
- user_id, name, timezone, language_preference, created_at

**Interviewer might ask:** "Why store timezone?"
**Answer:** To respect Do Not Disturb hours. User in PST with DND 10PM-8AM shouldn't get marketing notifications at 2AM their time.

---

### Q3: How do we store devices for push notifications?

**Challenge:**
- One user has multiple devices (iPhone, iPad, Android tablet)
- Device tokens can expire or become invalid
- Need to know which device is which platform (iOS vs Android)

**Solution:**
Devices table with:
- device_id, user_id, device_token, device_type, is_active, last_login_at

**Key Design Points:**

1. **is_active flag**: When APNs/FCM reports a token is invalid, mark it inactive
2. **device_type**: iOS uses APNs, Android uses FCM - different APIs
3. **One-to-Many relationship**: user_id as foreign key allows multiple devices

**Interviewer might ask:** "What happens when token expires?"
**Answer:** Third-party service (APNs/FCM) will return error. We mark device as inactive and don't send to it anymore. User must re-login to get new token.

---

### Q4: What notification types do we support? Why a separate table?

**Challenge:**
Our system supports different types: OTP, order updates, promotions, reminders. Each has different priority and category.

**Solution:**
Notification Types table defining all possible types:

**Example types:**
```
Type               | Category       | Priority  
-------------------|----------------|----------
otp_verification   | transactional  | critical
order_shipped      | transactional  | high
payment_received   | transactional  | high
cart_reminder      | reminder       | medium
flash_sale         | promotional    | low
price_drop         | promotional    | low
```

**Why separate table?**
- ✅ Central registry of all notification types
- ✅ Can add new types without code changes
- ✅ Defines default priority (OTP is always critical)
- ✅ Category helps with user preference grouping

**Interviewer might ask:** "Why not hardcode these in application?"
**Answer:** Flexibility. Product team can add new notification types without engineering. Also ensures consistency - "order_shipped" means the same thing everywhere.

---

### Q5: How do we handle complex user preferences? This is CRITICAL!

**Challenge - The Big One:**
User wants:
- ✅ Order updates via SMS + Email + Push
- ✅ Promotional offers via Email ONLY (no SMS spam!)
- ✅ Cart reminders via Push ONLY
- ❌ Never send me anything via In-App

**Bad Approach #1:**
```
Simple flags: email_enabled, sms_enabled, push_enabled
Problem: All-or-nothing. Can't say "SMS for orders but not promos" ❌
```

**Bad Approach #2:**
```
One row per user with JSON: {"order_shipped": ["email", "sms"]}
Problem: Hard to query, validate, and scale ❌
```

**Correct Approach:**
Table: `user_notification_preferences`
Each row represents: (user, notification_type, channel, enabled?)

**Preference Matrix Example:**
```
User 123's preferences:

Notification Type  | Email | SMS | Push | In-App
-------------------|-------|-----|------|-------
order_shipped      |  ✓    |  ✓  |  ✓   |   ✓
payment_received   |  ✓    |  ✓  |  ✓   |   ✓
flash_sale         |  ✓    |  ✗  |  ✗   |   ✓
cart_reminder      |  ✗    |  ✗  |  ✓   |   ✓
```

**How this works:**
- Each ✓ or ✗ is one row in the table
- Row: (user_id=123, type=flash_sale, channel=sms, enabled=false)

**Additional Layer - Global Channel Opt-Out:**
Separate table: `user_channel_preferences`
User can disable ALL SMS regardless of type.

**Decision Logic:**
```
To send notification:
1. Check global channel setting (is SMS enabled at all?)
2. Check Do Not Disturb hours (is it 2AM user's time?)
3. Check specific preference (does user want THIS type via SMS?)

If all pass → Send
If any fails → Don't send
```

**Why this design?**
- ✅ Maximum flexibility
- ✅ Easy to query: "Find all users who want order_shipped via SMS"
- ✅ Easy to update: User changes one preference = update one row
- ✅ Scales to 100M users (indexed properly)

**Interviewer might ask:** "This creates a LOT of rows. 100M users × 6 types × 4 channels = 2.4B rows!"
**Answer:** True, but:
1. We only store preferences that differ from defaults
2. Proper indexing makes queries fast
3. We cache frequently accessed preferences (next question)

---

### Q6: How do we make preference checks fast? (Caching Strategy)

**Challenge:**
Every notification (10M/day) must check preferences. That's 115 DB queries/second minimum.

**Solution: Two-Layer Strategy**

**Layer 1: Database (Source of Truth)**
- Stores all preferences
- Handles writes (user updates settings)

**Layer 2: Redis Cache (Speed Layer)**
- Cache key: `pref:{user_id}:{notification_type}:{channel}`
- Value: true/false
- TTL: 30 minutes

**How it works:**
```
1. Worker needs to check: Should we send user 123 order_shipped via SMS?
2. Check cache: pref:123:order_shipped:sms
3. Cache hit? → Use cached value (1ms)
4. Cache miss? → Query database (50ms), cache result
```

**Cache Invalidation:**
When user updates preferences:
1. Update database
2. Delete specific cache keys for that user
3. Next request will rebuild cache from DB

**What if cache goes down?**
- System still works, just slower
- Falls back to direct database queries
- Not catastrophic, just degraded performance

**Interviewer might ask:** "Why not cache entire user preferences as one object?"
**Answer:** Granular caching is better because:
- Smaller cache entries (less memory)
- Partial invalidation (change one pref, invalidate one key)
- Handles high cardinality better (100M users × 24 prefs = 2.4B keys is fine for Redis Cluster)

---

## PART 2: TEMPLATES & VERSIONING

### Q7: How do we store notification templates?

**Challenge:**
- Same notification type needs different templates per channel
- Templates exist in multiple languages
- Marketing team wants to update templates without engineering
- Need to test new templates (A/B testing)

**Solution: Templates Table**

**What defines a unique template?**
- notification_type + channel + language = one template

**Example:**
```
template_name               | type          | channel | language | version
----------------------------|---------------|---------|----------|--------
order_shipped_email_en_v2   | order_shipped | email   | en       | 2
order_shipped_email_es_v1   | order_shipped | email   | es       | 1
order_shipped_sms_en_v1     | order_shipped | sms     | en       | 1
order_shipped_push_en_v3    | order_shipped | push    | en       | 3
```

**Template Structure:**
- **Subject** (for email): "Your order {{order_id}} has shipped!"
- **Body**: "Hi {{customer_name}}, your order will arrive by {{delivery_date}}."
- **Placeholders**: {{variable_name}} replaced at runtime

**Why versioning?**
- Can update templates without breaking in-flight notifications
- Can A/B test: send 50% with v2, 50% with v3
- Can rollback if new template has issues

---

### Q8: How does template versioning actually work?

**Scenario: Marketing wants to update email template**

**Current:** 
```
Template v2: "Your order {{order_id}} has shipped!"
```

**New:**
```
Template v3: "Great news! Order {{order_id}} is on its way! 🚚"
```

**Process:**
1. Create new row: order_shipped_email_en_v3, is_active=TRUE
2. Mark old row: order_shipped_email_en_v2, is_active=FALSE
3. New notifications use v3
4. Old in-flight notifications still reference v2 (in their metadata)

**Critical Decision:** Should in-flight notifications use old or new template?

**Answer:** Old template (safer)
- When notification is queued, we store template_id and template_version
- Worker uses specific version, not "latest"
- Prevents confusion if template changes mid-delivery

**Interviewer might ask:** "How do you A/B test templates?"
**Answer:** 
- Keep both versions active (v2 and v3 both is_active=TRUE)
- Random selection: 50% get v2, 50% get v3
- Track which version in notification logs
- After test period, deactivate losing version

---

### Q9: Where do we cache templates? Why?

**Caching Strategy:**

**Why cache?**
- Templates rarely change (maybe once a week)
- Every notification needs template (10M/day)
- Template lookup is read-heavy

**Cache Design:**
- **Key:** `template:{type_id}:{channel}:{language}`
- **Value:** Entire template object (subject, body, metadata)
- **TTL:** 1 hour (low churn data)

**Cache Invalidation:**
When template updated:
1. Update database
2. Delete cache key: `template:{type_id}:{channel}:{language}`
3. Next request rebuilds from DB

**What if cache is empty (cold start)?**
- Query database
- Populate cache
- Slightly slower first request, then fast

**Interviewer might ask:** "What if you have 1000 templates? Cache all of them?"
**Answer:** No, use LRU (Least Recently Used) eviction. Cache only frequently used templates. Rare templates are fetched from DB as needed.

---

## PART 3: NOTIFICATION LOGS & STATUS TRACKING

### Q10: How do we track all notifications? Why NoSQL?

**Challenge:**
- 10M notifications/day = 3.6 billion/year
- Need to track: sent, delivered, failed, retried
- Need to query: "Show me all notifications for user X"
- Need to query: "Find failed notifications to retry"

**SQL vs NoSQL Decision:**

**Why NOT SQL?**
- ❌ 10M writes/day is heavy for relational DB
- ❌ Joins are slow at this scale
- ❌ Schema changes are risky (ALTER TABLE on billion rows)
- ❌ Sharding is complex

**Why NoSQL (MongoDB/DynamoDB)?**
- ✅ Handles high write throughput easily
- ✅ Flexible schema (can add fields without migration)
- ✅ Horizontal scaling (add more nodes)
- ✅ Good for time-series data (notifications are append-only)

**What goes in Notification Log?**
```
Key fields:
- event_id (for deduplication)
- user_id (who received it)
- notification_type (what type)
- channel (email/sms/push)
- status (pending/sent/delivered/failed)
- created_at, sent_at, delivered_at
- retry_count (how many retries)
- third_party_message_id (tracking in SendGrid/Twilio)
- error_message (if failed)
- metadata (order_id, tracking_url, etc.)
```

**Indexes needed:**
1. user_id + created_at (user's notification history)
2. event_id (deduplication lookups)
3. status + next_retry_at (find notifications to retry)
4. created_at (time-based queries for analytics)

---

### Q11: What are all possible notification statuses? (State Machine)

**Status Flow:**
```
PENDING → QUEUED → PROCESSING → SENT → DELIVERED ✓
              ↓
          FAILED → RETRYING → [back to QUEUED]
              ↓
          ABANDONED (after max retries) ✗
```

**What each status means:**

1. **PENDING**: Just created, not yet queued
2. **QUEUED**: In message queue, waiting for worker
3. **PROCESSING**: Worker picked it up, preparing to send
4. **SENT**: Sent to third-party (Twilio/SendGrid/FCM)
5. **DELIVERED**: Third-party confirmed delivery ✓
6. **FAILED**: Something went wrong
7. **RETRYING**: Will retry shortly
8. **ABANDONED**: Failed after max retries, moved to DLQ ✗

**Allowed Transitions:**
- PENDING → QUEUED, FAILED
- QUEUED → PROCESSING, FAILED
- PROCESSING → SENT, FAILED
- SENT → DELIVERED, FAILED
- FAILED → RETRYING, ABANDONED
- RETRYING → QUEUED
- DELIVERED → (terminal state)
- ABANDONED → (terminal state)

**Why strict state machine?**
- ✅ Clear expectations: we know exactly what each status means
- ✅ Debugging: can't get into invalid states
- ✅ Monitoring: can alert on unexpected transitions

**Interviewer might ask:** "What if notification goes SENT but we never hear back from third-party?"
**Answer:** Timeout handling. After 5 minutes in SENT, mark as FAILED and retry. Third-parties usually respond within seconds.

---

### Q12: How do we handle failed notifications? (Retry Logic)

**Scenario:** Twilio is down, SMS fails to send.

**Strategy: Exponential Backoff**

**Attempt 1:** Immediate failure → retry in 1 minute
**Attempt 2:** Still failing → retry in 2 minutes  
**Attempt 3:** Still failing → retry in 4 minutes
**Attempt 4:** Still failing → retry in 8 minutes
**Attempt 5:** Give up → move to Dead Letter Queue

**Why exponential backoff?**
- ✅ Don't hammer failing service (makes outage worse)
- ✅ Gives service time to recover
- ✅ Spreads retry load over time

**How it works:**
1. Worker sends notification, gets error
2. Worker marks status=FAILED, retry_count=1
3. Worker calculates: next_retry_at = now + (2^retry_count minutes)
4. Worker re-queues with delay
5. Later, another worker picks it up, tries again

**Retry Limits:**
- **Transactional** (OTP, order updates): 5 retries, 30 min max
- **Promotional**: 3 retries, 1 hour max
- **After max retries**: Move to Dead Letter Queue

**Interviewer might ask:** "What if Twilio is down for 2 hours?"
**Answer:** After max retries (~30 min), notifications go to DLQ. Ops team is alerted. When Twilio recovers, we can manually replay DLQ notifications or let them expire (marketing emails don't matter 2 hours later).

---

### Q13: What is Dead Letter Queue? How does it work?

**Purpose:** Track notifications that failed after ALL retries.

**What goes to DLQ?**
- Notifications that failed 5 times
- Notifications with permanent errors (invalid phone number)
- Notifications blocked by third-party (spam filters)

**DLQ Contents:**
```
Fields:
- event_id
- user_id
- notification_type, channel
- original_message (full payload)
- failure_reason ("Twilio error: Invalid phone number")
- retry_count (how many times we tried)
- first_attempt_at, last_attempt_at
- is_resolved (false until ops team fixes it)
```

**What happens next?**

**Option 1: Alert Ops Team**
- Slack alert: "50 SMS notifications in DLQ"
- On-call engineer investigates
- Manual resolution if needed

**Option 2: Auto-Resolution**
- For transient failures: when service recovers, replay DLQ
- For permanent failures (bad phone): mark as resolved, notify user

**Option 3: Ignore**
- Marketing emails from 2 days ago? Not worth replaying
- Mark as resolved, move on

**DLQ is SQL table** (not NoSQL)
- Why? Need transactions for resolution tracking
- Ops team needs to query complex conditions
- Volume is low (only failures, not all notifications)

**Interviewer might ask:** "How do you prevent DLQ from growing infinitely?"
**Answer:** Retention policy. Auto-delete resolved items after 30 days. Alert if unresolved items > 1000 (indicates systemic issue).

---

## PART 4: DEDUPLICATION

### Q14: How do we prevent duplicate notifications? This is CRITICAL!

**Problem Scenario:**
```
10:00:00 - Order Service sends "order_shipped" notification
10:00:02 - (network blip) Order Service doesn't get ACK, retries
10:00:05 - Order Service sends SAME notification again
Result: User gets 2 identical emails ❌
```

**Solution: Event-Based Deduplication**

**Concept:**
Each notification has unique `event_id` based on business event.
- Example: `order_123_shipped_2026-01-06`
- If we've already processed this event_id → skip

**Implementation Options:**

**Option A: Database Unique Constraint**
```
Deduplication table: (event_id PRIMARY KEY)
Before processing: INSERT event_id
If insert fails → duplicate detected, skip
If insert succeeds → proceed

Pros: Simple, durable
Cons: Slow (50ms per check), not scalable at high volume
```

**Option B: Redis Atomic Check (RECOMMENDED)**
```
Before processing: Redis SET event_id value=1 NX EX 3600
NX = only set if not exists
EX 3600 = expire after 1 hour

If SET succeeds (returns OK) → new event, proceed
If SET fails (returns null) → duplicate, skip

Pros: Fast (<1ms), atomic, auto-expiry
Cons: Requires Redis, lost if Redis crashes (but TTL limits risk)
```

**Option C: Message Queue Deduplication**
```
Use SQS FIFO queue with deduplication ID
AWS handles deduplication automatically

Pros: No extra infrastructure
Cons: Tied to AWS, less flexible
```

**Which to choose?**
- At 10M/day scale: **Redis (Option B)**
- For AWS-only: SQS FIFO
- Small scale (<100K/day): Database is fine

**Why Redis wins:**
- ✅ Sub-millisecond latency
- ✅ Atomic operations (no race conditions)
- ✅ Auto-expiry (no manual cleanup)
- ✅ Can handle millions of ops/sec

**TTL Choice:**
- Set event TTL = notification validity window
- OTP: 10 minutes (OTP expires anyway)
- Order updates: 1 hour (unlikely to reprocess after 1 hour)
- Promotions: 24 hours (marketing campaigns run daily)

**Interviewer might ask:** "What if Redis goes down?"
**Answer:** 
- **Option 1:** Fallback to database (slower but works)
- **Option 2:** Accept duplicates temporarily (better than no notifications)
- **Option 3:** Redis cluster with replication (high availability)

**Interviewer follow-up:** "What if two servers check Redis at EXACT same time?"
**Answer:** Redis is single-threaded, operations are serialized. Even if two servers send SET command simultaneously, Redis processes them sequentially. First one succeeds, second one fails. No race condition.

---

## PART 5: SCALE & PERFORMANCE

### Q15: Why NoSQL for logs but SQL for preferences?

**Comparison Table:**

| Aspect | Notification Logs | User Preferences |
|--------|-------------------|------------------|
| **Write Pattern** | Append-only, 10M/day | Update-heavy, rare |
| **Read Pattern** | Time-range queries | Point lookups by user |
| **Schema** | Flexible (metadata varies) | Fixed, well-defined |
| **Volume** | Billions of rows | Millions of rows |
| **Consistency** | Eventual OK | Strong required |
| **Queries** | Simple (by user, by time) | Complex (joins with types) |
| **Best Fit** | NoSQL (MongoDB/DynamoDB) | SQL (PostgreSQL/MySQL) |

**Why logs need NoSQL:**
- Write-heavy (10M/day writes, 100K/day reads)
- Schema flexibility (metadata varies per notification type)
- Time-series nature (always querying recent data)
- Horizontal scaling (easy to add nodes for more capacity)

**Why preferences need SQL:**
- Strong consistency (user changes pref, must apply immediately)
- Relational (preferences join with notification_types, users)
- ACID transactions (updating multiple prefs atomically)
- Complex queries ("find users who want X via Y")

**Interviewer might ask:** "Can we use SQL for logs too?"
**Answer:** Yes, but challenges at scale:
- Partitioning needed (by date, by user_id hash)
- Expensive to run analytics (billion-row JOINs)
- ALTER TABLE takes hours on large tables
- Sharding is complex (where does user X's data live?)

NoSQL handles these concerns naturally.

---

### Q16: How do we partition/shard data at 100M+ users?

**User-Related Tables (SQL):**
- Partition by `user_id % num_shards`
- Example: 10 database shards, user 12345 goes to shard 12345 % 10 = shard 5
- All user's data (devices, preferences) in same shard (avoids cross-shard queries)

**Notification Logs (NoSQL):**
- MongoDB: Shard by `user_id` (hashed sharding)
- DynamoDB: `user_id` as partition key, `created_at` as sort key
- Ensures even distribution across nodes

**Templates (SQL):**
- Small dataset (<10K templates), no sharding needed
- Replicate to all regions for low latency

**Interviewer might ask:** "What if user data doesn't fit on one shard?"
**Answer:** Shouldn't happen (one user's data is small). But if needed, use composite sharding: (user_id % num_shards, data_type). Preferences on shard A, devices on shard B.

---

### Q17: How do we handle timezone-aware scheduling?

**Challenge:** 
User in PST wants cart reminder at 10 AM their time. Current time is 3 AM PST (too early).

**Solution:**

**Store timezone in users table:**
```
user_id: 123
timezone: "America/Los_Angeles"
```

**When scheduling notification:**
1. Check user's timezone
2. Check if current time in their timezone is within DND hours
3. If yes, calculate: next_allowed_time = DND_end_hour in their timezone
4. Schedule notification for that time

**Implementation:**
```
User's DND: 10 PM - 8 AM PST
Current time: 3 AM PST (Jan 6, 2026)
Marketing notification (non-critical)

Check: Is 3 AM between 10 PM and 8 AM? YES
Action: Delay notification until 8 AM PST
Queue message with: scheduled_at = "2026-01-06 08:00:00 PST"
```

**Critical notifications (OTP):**
- Ignore DND hours
- Send immediately regardless of time

**Interviewer might ask:** "How do you handle Daylight Saving Time?"
**Answer:** Use proper timezone library (moment-timezone, pytz). Store timezone as location ("America/Los_Angeles"), not offset (UTC-8). Library handles DST transitions automatically.

---

## PART 6: ANALYTICS & MONITORING

### Q18: How do we build analytics on notification data?

**Goal:** Answer questions like:
- How many notifications sent today?
- What's the delivery rate per channel?
- Which notification types have highest failure rate?

**Challenge:** Can't run analytics on 3.6 billion row notification_logs table (too slow).

**Solution: Pre-Aggregated Stats Table (SQL)**

**Daily Stats Table:**
```
Table: notification_stats_daily
Aggregation: Daily per notification_type per channel

Fields:
- stat_date (2026-01-06)
- notification_type (order_shipped)
- channel (email)
- total_sent (1,000,000)
- total_delivered (950,000)
- total_failed (50,000)
- avg_delivery_time_seconds (3.2)
- p95_delivery_time_seconds (5.1)
```

**How it works:**
1. Background job runs every hour
2. Aggregates last hour's data from notification_logs
3. Inserts into stats table
4. Dashboard queries stats table (fast, small)

**Real-time monitoring:**
- Stream notification_logs to data pipeline (Kafka → Spark/Flink)
- Calculate metrics in real-time
- Store in time-series DB (InfluxDB, TimescaleDB)
- Grafana dashboard displays metrics

**Interviewer might ask:** "What if I want to query individual notification details?"
**Answer:** 
- Recent data (<7 days): Query notification_logs directly
- Old data (>7 days): Archive to cold storage (S3), query via Athena/BigQuery
- Pre-aggregated stats: Use daily stats table

---

### Q19: How do we monitor third-party service health?

**Problem:** 
- Twilio goes down → all SMS fail
- Need to detect quickly and route around

**Solution: Service Health Tracking**

**Health Check Table:**
```
Table: service_health_log
- service_name (twilio_sms)
- status (healthy/degraded/down)
- response_time_ms (150)
- error_rate (5.2%)
- timestamp
```

**How it works:**
1. Every worker tracks third-party API response times and errors
2. Every minute, worker reports stats to health service
3. Health service aggregates: if error_rate > 10% → mark as degraded
4. If error_rate > 50% → mark as down, trigger alert

**Circuit Breaker Pattern:**
```
If service is DOWN:
- Stop sending requests (fail fast)
- Check health every 30 seconds
- When health check succeeds 3 times → mark as recovered

If service is DEGRADED:
- Reduce request rate (throttle)
- Increase retry delays
```

**Alerting:**
- PagerDuty alert if service down > 5 minutes
- Slack alert if error rate > 20%

**Fallback Strategy:**
- SMS down → try alternative provider (Plivo as backup)
- Push down → queue for later retry
- Email down → critical: use backup provider (Mailgun), non-critical: wait

---

## PART 7: ADVANCED TOPICS

### Q20: How do we support multiple languages?

**Challenge:** 
- User in Spain wants notifications in Spanish
- User in France wants French

**Solution:**
1. Store `language_preference` in users table (en, es, fr, de)
2. Create templates for each language:
   - order_shipped_email_en_v1
   - order_shipped_email_es_v1
   - order_shipped_email_fr_v1
3. When sending notification:
   - Look up user's language
   - Fetch template for (notification_type, channel, user_language)
   - Render template with user's data

**Fallback:** If template doesn't exist in user's language, use English (default).

---

### Q21: How do we implement rate limiting per user?

**Challenge:** Don't spam users with 100 notifications/day.

**Solution: Redis-based Rate Limiter**

**Sliding Window Rate Limit:**
```
Rule: Max 20 promotional notifications per user per day

Before sending promotional notification:
1. Check: Redis key = rate_limit:promo:user_123:2026-01-06
2. If count >= 20 → reject, don't send
3. If count < 20 → increment counter, send

Key expires at end of day (TTL = seconds until midnight)
```

**Different limits per category:**
- Transactional: No limit (always send)
- Promotional: 20/day max
- Reminders: 5/day max

**Interviewer might ask:** "What if Redis has wrong count?"
**Answer:** Redis is source of truth for rate limits. If it resets (cache clear), user might get extra notifications that day. Trade-off: perfect accuracy vs. operational simplicity. For rate limiting, approximate is acceptable.

---

## PART 8: INTERVIEW RAPID-FIRE Q&A

### Q22: How do you handle notification priority?

**Answer:** Separate queues per priority:
- critical_queue (OTP, security alerts) - processed first
- high_queue (order updates)
- medium_queue (reminders)
- low_queue (marketing)

Workers pull from highest priority queue first. Ensures OTPs never wait behind marketing emails.

---

### Q23: What if database goes down during peak traffic?

**Answer:** 
- **Read queries:** Serve from cache (Redis), degraded but functional
- **Write queries:** Queue writes in message queue, process when DB recovers
- **Preference checks:** Use cached values (might be slightly stale, but better than nothing)

Notification system is eventually consistent, short DB outage is tolerable.

---

### Q24: How do you test this system?

**Answer:**
- **Unit tests:** Each component (preference checker, template renderer, deduplication)
- **Integration tests:** Full flow (API → queue → worker → third-party mock)
- **Load tests:** Simulate 10M notifications/day, measure latency and error rate
- **Chaos tests:** Kill random services (Redis, DB, third-party), ensure graceful degradation

---

### Q25: Security concerns?

**Answer:**
- **PII in logs:** Hash user_id, mask email/phone in logs
- **Template injection:** Sanitize variables (prevent XSS in emails)
- **Rate limiting:** Prevent API abuse (malicious actor spamming notifications)
- **Third-party credentials:** Store in secrets manager (AWS Secrets Manager), rotate regularly

---

## QUICK REFERENCE: KEY DESIGN DECISIONS

| Decision | Choice | Why |
|----------|--------|-----|
| **User preferences storage** | Separate row per (user, type, channel) | Supports complex preferences |
| **Notification logs storage** | NoSQL (MongoDB/DynamoDB) | High write volume, flexible schema |
| **Template caching** | Redis, TTL 1 hour | Fast lookups, low churn data |
| **Deduplication** | Redis atomic SET NX | Fast, atomic, auto-expiry |
| **Retry strategy** | Exponential backoff, max 5 retries | Don't hammer failing services |
| **Status tracking** | State machine with strict transitions | Clear expectations, easier debugging |
| **Priority handling** | Separate queues per priority | Critical notifications never blocked |
| **Timezone handling** | Store in user table, check DND hours | Respect user sleep schedules |
| **Rate limiting** | Redis sliding window | Prevent spam, simple implementation |
| **Analytics** | Pre-aggregated daily stats table | Fast dashboard queries |

---

## COMMON INTERVIEW MISTAKES TO AVOID

❌ **Mistake 1:** Single boolean per channel (can't do "SMS for orders, not promos")
✅ **Correct:** Row per (user, type, channel)

❌ **Mistake 2:** Storing everything in SQL at 10M/day scale
✅ **Correct:** SQL for preferences, NoSQL for logs

❌ **Mistake 3:** No deduplication (users get duplicate notifications)
✅ **Correct:** Redis-based event deduplication

❌ **Mistake 4:** Infinite retries (DDoS your own third-party)
✅ **Correct:** Exponential backoff, max retries, DLQ

❌ **Mistake 5:** No template versioning (breaks in-flight notifications)
✅ **Correct:** Store template_version with each notification

---

**END OF DOCUMENT** 🎯

This covers all 8 requirements in Q&A format with explanations, trade-offs, and interview tips. Save this for your revision!