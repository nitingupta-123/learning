# **SECTION 3: DISASTER RECOVERY & HIGH AVAILABILITY** 🚨

---

## **3.1 Multi-Region Architecture**

### 🤔 Interview Question/Context

**Interviewer:** "What happens if your entire datacenter goes down? How do you ensure the URL shortener stays available?"

### ❌ Your Initial Answer

You said: **"I am not sure."**

**What was missing:**
- Understanding of multi-region deployment
- Failover strategies
- DNS-based routing
- Data replication across regions

---

### ✅ Complete Answer

#### **The Datacenter Failure Scenario:**

**What can cause entire datacenter to fail:**

```
Natural disasters:
  - Earthquake damages datacenter
  - Flood destroys equipment
  - Power grid failure
  - Fire in facility

Cloud provider issues:
  - AWS US-EAST-1 outage (happens every few years)
  - Network partition
  - Planned maintenance gone wrong

Human error:
  - Accidental deletion of critical resources
  - Bad configuration pushed to all instances
```

**Impact without multi-region:**

```
Single region architecture:
  All services in US-EAST
      ↓
  US-EAST goes down
      ↓
  100% of users affected ❌
  Service completely unavailable
  Revenue loss: $X per hour
```

---

### 🎯 URL Shortener Context

#### **Multi-Region Architecture Patterns:**

### **Pattern 1: Active-Passive (Recommended for URL Shortener)** ✅

**Setup:**

```
PRIMARY REGION (US-EAST):
  ├─ 10 Service Instances
  ├─ Master Database (writes + reads)
  ├─ 2 Slave Databases (reads)
  ├─ Redis Cluster (3 nodes)
  ├─ Kafka Cluster
  └─ Handles 100% of traffic

SECONDARY REGION (US-WEST): 
  ├─ 5 Service Instances (standby, minimal load)
  ├─ Slave Database (replicating from US-EAST master)
  ├─ Redis Cluster (separate, empty)
  ├─ Kafka Cluster
  └─ Handles 0% of traffic (standby only)

DNS: tiny.url → US-EAST (primary)
```

**Normal operation:**

```
User request:
  DNS resolves tiny.url → US-EAST IP
      ↓
  Request goes to US-EAST
      ↓
  US-EAST handles request
      ↓
  Success!

US-WEST: Idle, ready to take over
```

---

**Failover process (when US-EAST fails):**

```
Step 1: Health check detects US-EAST down (30 seconds)
  ├─ Load balancer health checks fail
  ├─ Synthetic monitoring alerts fire
  └─ DNS health check fails

Step 2: DNS failover (60 seconds)
  ├─ Route 53 updates: tiny.url → US-WEST IP
  ├─ DNS propagation: 60 seconds (with low TTL)
  └─ New requests go to US-WEST

Step 3: Database promotion (2 minutes)
  ├─ Promote US-WEST slave to master
  ├─ Command: pg_ctl promote
  ├─ US-WEST now accepts writes
  └─ Update application config to point to new master

Step 4: Scale up US-WEST (5 minutes)
  ├─ Auto-scaling adds 5 more service instances
  ├─ US-WEST now has 10 instances (same as US-EAST had)
  └─ Can handle full production traffic

Total failover time: 8 minutes
Downtime: ~2 minutes (DNS + DB promotion)
```

---

**Architecture diagram:**

```
                    [Route 53 DNS]
                    tiny.url
                         ↓
          ┌──────────────┴──────────────┐
          ↓ (primary)         ↓ (failover)
    [US-EAST Region]      [US-WEST Region]
          
    ┌─────────────────┐   ┌─────────────────┐
    │ Load Balancer   │   │ Load Balancer   │
    │ Health: OK ✓    │   │ Health: OK ✓    │
    └────────┬────────┘   └────────┬────────┘
             ↓                      ↓
    ┌─────────────────┐   ┌─────────────────┐
    │ 10 Services     │   │ 5 Services      │
    │ (active)        │   │ (standby)       │
    └────────┬────────┘   └────────┬────────┘
             ↓                      ↓
    ┌─────────────────┐   ┌─────────────────┐
    │ Master DB       │──→│ Slave DB        │
    │ (writes)        │   │ (replicating)   │
    │                 │   │                 │
    │ 2× Slave DB     │   │                 │
    │ (reads)         │   │                 │
    └─────────────────┘   └─────────────────┘
             ↓                      ↓
    ┌─────────────────┐   ┌─────────────────┐
    │ Redis Cluster   │   │ Redis Cluster   │
    └─────────────────┘   └─────────────────┘

Normal: 100% traffic → US-EAST
Failover: 100% traffic → US-WEST
```

---

**Pros & Cons:**

```
Pros:
  ✅ Simple architecture
  ✅ Lower cost (secondary region smaller)
  ✅ No cross-region latency during normal operation
  ✅ No data conflicts (single write master)

Cons:
  ❌ Secondary region wastes resources (underutilized)
  ❌ Failover requires manual/automated intervention
  ❌ 2-8 minute downtime during failover
  ❌ Secondary region might not be "warm" enough
```

---

### **Pattern 2: Active-Active (For Massive Scale)** 

**Setup:**

```
BOTH REGIONS ACTIVE:

US-EAST Region:
  ├─ Handles 50% of traffic (US users)
  ├─ Database: Can write AND read
  ├─ Redis: Independent cache
  └─ Kafka: Independent cluster

EU-WEST Region:
  ├─ Handles 50% of traffic (EU users)
  ├─ Database: Can write AND read
  ├─ Redis: Independent cache
  └─ Kafka: Independent cluster

DNS: GeoDNS routing
  - US users → US-EAST
  - EU users → EU-WEST
```

---

**Challenges with Active-Active:**

**Challenge 1: Data replication conflicts**

```
Scenario:
  Time 0: User in US creates URL for google.com
          → US-EAST DB: ID 100, long_url = "google.com"
  
  Time 1: User in EU creates URL for google.com
          → EU-WEST DB: ID 101, long_url = "google.com"
  
  Time 2: Databases replicate to each other
          → Conflict! Same URL has two different IDs!

Problem: Without coordination, different regions generate different short URLs for same long URL
```

**Solution: Global ID coordination**

```
Option A: Use Snowflake IDs with datacenter ID
  US-EAST: datacenter_id = 1 → IDs: 1000, 1032, 1064...
  EU-WEST: datacenter_id = 2 → IDs: 2000, 2032, 2064...
  
  No collision possible! ✓

Option B: Partition ID space
  US-EAST: Generates IDs 0 - 500,000,000,000
  EU-WEST: Generates IDs 500,000,000,001 - 1,000,000,000,000
  
  No collision possible! ✓
```

---

**Challenge 2: Cache inconsistency**

```
Scenario:
  User in US creates URL → Cached in US-EAST Redis
  User in EU clicks same URL → Cache miss in EU-WEST Redis
  
Result: EU user experiences slow redirect (cache miss)

Solution: Accept eventual consistency
  - Each region builds own cache based on local traffic
  - Popular URLs get cached in both regions naturally
  - 95% hit rate still achievable per region
```

---

**Challenge 3: Database replication conflicts**

```
Problem: Multi-master replication is complex!

US-EAST DB ←──────→ EU-WEST DB
     ↕                   ↕
(both accept writes)

Conflict scenarios:
  - Same URL created in both regions (handled by Snowflake)
  - Same URL updated in both regions (last-write-wins)
  - Replication lag causes stale reads

Solution: Use database that supports multi-master
  - CockroachDB (distributed SQL)
  - Cassandra (NoSQL, eventual consistency)
  - DynamoDB Global Tables (AWS)

OR: Stick with active-passive! (much simpler)
```

---

**When to use Active-Active:**

```
✓ Use Active-Active when:
  - Global user base (US + EU + Asia users)
  - Need lowest latency for all users
  - Can afford complexity
  - Scale > 100,000 requests/sec

✗ Don't use Active-Active when:
  - Regional user base (mostly US)
  - Simplicity is priority
  - Scale < 10,000 requests/sec
  - Small team

For URL shortener at 4,000 RPS: Active-Passive is sufficient! ✓
```

---

#### **DNS Failover Configuration:**

**Route 53 health check:**

```yaml
Health Check: urlshortener-us-east
  Type: HTTPS
  Domain: us-east.urlshortener.internal
  Path: /health
  Port: 443
  Interval: 30 seconds
  Failure threshold: 2 consecutive failures
  
  Health check logic:
    - Sends request every 30 seconds
    - If response not 200 OK → failure count++
    - If failure count >= 2 → Mark unhealthy
    - If unhealthy → Trigger failover
```

**DNS record with failover:**

```yaml
DNS Record: tiny.url
  Type: A (IPv4)
  Routing Policy: Failover
  
  Primary:
    Value: 54.23.45.67 (US-EAST Load Balancer IP)
    Health Check: urlshortener-us-east
    TTL: 60 seconds (low TTL for fast failover)
  
  Secondary:
    Value: 35.12.78.90 (US-WEST Load Balancer IP)
    Health Check: urlshortener-us-west
    TTL: 60 seconds

Behavior:
  - Normal: DNS returns US-EAST IP
  - US-EAST unhealthy: DNS returns US-WEST IP
  - Both unhealthy: DNS returns last known good (US-EAST)
```

---

**Low TTL importance:**

```
Scenario: High TTL (1 hour)

  US-EAST fails at 14:00
      ↓
  DNS updated to US-WEST at 14:02
      ↓
  But users still have cached US-EAST IP (TTL not expired)
      ↓
  Users can't access service until 15:00 (1 hour later)
      ↓
  Effective downtime: 1 hour! ❌

Scenario: Low TTL (60 seconds)

  US-EAST fails at 14:00
      ↓
  DNS updated to US-WEST at 14:02
      ↓
  Users' cached DNS expires at 14:01 (60 seconds after last lookup)
      ↓
  Users get new IP at 14:02
      ↓
  Effective downtime: 2 minutes ✓

Trade-off: Low TTL = more DNS queries = slightly higher cost
Cost: $0.40 per million queries (negligible)
```

---

#### **Database Cross-Region Replication:**

**PostgreSQL streaming replication:**

```
US-EAST (Primary):
  postgresql.conf:
    wal_level = replica
    max_wal_senders = 3
    
  pg_hba.conf:
    host replication replicator us-west-ip/32 md5

US-WEST (Secondary):
  recovery.conf:
    primary_conninfo = 'host=us-east-ip port=5432 user=replicator'
    primary_slot_name = 'us_west_slot'

Replication lag: 
  Normal: <1 second
  Cross-region: 50-200ms (network latency)
  Alert if: >5 seconds
```

**Monitoring replication:**

```sql
-- On primary (US-EAST):
SELECT 
  client_addr,
  state,
  sent_lsn,
  write_lsn,
  flush_lsn,
  replay_lsn,
  sync_state,
  EXTRACT(EPOCH FROM (NOW() - pg_last_committed_xact())) AS lag_seconds
FROM pg_stat_replication;

Result:
client_addr     | state     | lag_seconds
----------------+-----------+------------
35.12.78.90     | streaming | 0.15

Alert if lag_seconds > 5
```

---

**Promoting secondary to primary (failover):**

```bash
# Automated failover script

#!/bin/bash

echo "Starting failover to US-WEST..."

# Step 1: Stop replication on US-WEST
ssh us-west-db "pg_ctl promote -D /var/lib/postgresql/data"

# Step 2: Wait for promotion to complete
sleep 10

# Step 3: Update application config
kubectl set env deployment/urlshortener \
  DATABASE_HOST=us-west-db-master.internal

# Step 4: Restart application instances
kubectl rollout restart deployment/urlshortener

# Step 5: Update DNS
aws route53 change-resource-record-sets \
  --hosted-zone-id Z123456 \
  --change-batch file://failover-dns.json

echo "Failover complete! US-WEST is now primary."
```

---

#### **Testing Disaster Recovery (Chaos Engineering):**

**Scheduled DR drills:**

```
Quarterly DR Test (Last Sunday of quarter, 2 AM):

Pre-drill checklist:
  ✓ Notify team 1 week in advance
  ✓ Stakeholders aware (no customer impact expected)
  ✓ Monitoring dashboards ready
  ✓ Runbooks printed/accessible
  ✓ Roll back plan prepared

Drill steps:
  1. Start: 2:00 AM
     - Announce: "DR drill starting"
  
  2. Simulate failure: 2:01 AM
     - Shut down US-EAST region
     - Method: Terminate all instances via script
  
  3. Detection: 2:02 AM (target: <2 minutes)
     - Health checks fail
     - Alerts fire
     - On-call acknowledges
  
  4. Failover: 2:05 AM (target: <5 minutes)
     - Run automated failover script
     - Promote US-WEST database
     - Update DNS
  
  5. Verification: 2:10 AM (target: <10 minutes)
     - Synthetic checks pass
     - Manual smoke tests
     - Check error rates
  
  6. Restore: 2:30 AM
     - Bring US-EAST back online
     - Fail back to US-EAST
  
  7. Post-drill: 2:45 AM
     - Review metrics
     - Document issues
     - Update runbooks

Success criteria:
  - Failover completed in <10 minutes ✓
  - No data loss ✓
  - Error rate <1% during failover ✓
  - All team members responded ✓
```

---

**Metrics from DR drill:**

```
DR Drill Results (2025-11-29)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Detection Time: 1m 45s ✓ (target: <2m)
Failover Time: 4m 32s ✓ (target: <5m)
Total Downtime: 6m 17s ✓ (target: <10m)

Traffic during failover:
  Total requests: 15,000
  Successful: 14,200 (94.7%)
  Failed: 800 (5.3%)

Data loss: 0 URLs ✓

Issues found:
  1. Runbook step 3 outdated (fixed)
  2. US-WEST needed manual scaling (automated now)
  3. Redis not pre-warmed (added to script)

Overall: PASS ✓
Next drill: 2026-02-28
```

---

### 📊 Key Takeaways

1. **Active-Passive is simpler:** Recommended for URL shortener scale

2. **Active-Active is complex:** Only needed for global, massive scale

3. **Low DNS TTL:** 60 seconds for fast failover

4. **Database replication:** Monitor lag, automate promotion

5. **DR drills:** Test quarterly, measure RTO, update runbooks

6. **Target RTO:** <10 minutes (detection + failover + verification)

7. **Snowflake IDs:** Prevent collisions in multi-region setup

---

### 🔗 Further Reading

- **AWS Well-Architected Framework:** Reliability pillar
- **Google SRE Book:** Chapter on managing multi-region services
- **"Chaos Engineering" by Netflix:** Testing failure scenarios

---

### ✏️ Practice Exercise

**Scenario:** You're running Active-Passive with US-EAST primary, US-WEST secondary.

**Event timeline:**
- 14:00:00 - US-EAST region loses power (total failure)
- 14:00:30 - Health checks detect failure
- 14:01:00 - Automated failover begins
- 14:02:30 - US-WEST promoted to primary
- 14:03:00 - DNS updated
- 14:05:00 - US-WEST handling 100% traffic

**At 14:10:00**, US-EAST power restored. All instances boot up.

**Questions:**
1. What happens when US-EAST comes back online?
2. Should you fail back to US-EAST immediately?
3. How do you prevent data conflicts?
4. What if a user created a URL during the 5-minute failover?

<details>
<summary>Click to see answer</summary>

**1. What happens when US-EAST comes back online?**

```
US-EAST boots up:
  ├─ Services start
  ├─ Database starts (as replica)
  ├─ Redis starts (empty cache)
  └─ Load balancer starts

Current state:
  - US-WEST: Primary database (accepting writes)
  - US-EAST: Booting up
  - DNS: Still pointing to US-WEST

What US-EAST does NOT know:
  - URLs created during failover (14:00 - 14:10)
  - Those are only in US-WEST database

Danger: If US-EAST starts accepting traffic, it's missing 10 minutes of data!
```

---

**2. Should you fail back immediately?**

**NO! Wait and verify.** Here's the safe process:

```
Step 1: US-EAST comes online (14:10)
  - DO NOT route traffic yet!
  - Let it boot completely

Step 2: Reconnect replication (14:12)
  - Configure US-EAST as replica of US-WEST
  - Command: Set primary_conninfo to US-WEST
  - Replication starts: US-WEST → US-EAST

Step 3: Wait for catch-up (14:15)
  - Monitor replication lag
  - Wait until lag < 1 second
  - Query: SELECT * FROM pg_stat_replication;

Step 4: Verify data consistency (14:20)
  - Check row counts match
  - US-WEST: SELECT COUNT(*) FROM url_mappings; → 1,000,523
  - US-EAST: SELECT COUNT(*) FROM url_mappings; → 1,000,523 ✓
  
Step 5: Smoke tests (14:25)
  - Create test URL in US-WEST
  - Verify it appears in US-EAST within 1 second
  - Click test URL from US-EAST region
  - Works! ✓

Step 6: Gradual fail back (14:30)
  - Option A: Stay on US-WEST (don't fail back)
  - Option B: Fail back during low-traffic window
  
  If failing back:
    1. Promote US-EAST back to primary
    2. Update DNS: tiny.url → US-EAST
    3. Demote US-WEST to replica
    4. Monitor for issues

Recommendation: Wait until next planned maintenance window
  - Don't rush fail back
  - US-WEST is stable and working
  - Fail back during 2 AM window when traffic low
```

---

**3. How do you prevent data conflicts?**

**Critical: Only ONE database accepts writes at a time!**

```
Safe state during failover:
  14:00 - 14:02: US-EAST down, US-WEST not yet primary
    → NO writes happening (brief downtime) ✓
  
  14:02 - 14:10: US-WEST promoted to primary
    → ALL writes go to US-WEST ✓
  
  14:10+: US-EAST back, configured as replica
    → Reads from US-WEST, writes go to US-WEST ✓

Dangerous state (avoid!):
  Both US-EAST and US-WEST accepting writes
    → Data conflicts! ❌
    → Two different IDs for same URL
    → Split-brain scenario

Prevention mechanism:
  - Database promotion requires explicit command
  - Only automation or on-call engineer can promote
  - Cannot have two primaries simultaneously
  - Fencing: If US-EAST tries to accept writes, reject them
```

**Fencing implementation:**

```sql
-- On US-EAST when it boots up:

-- Check if I'm the current primary
SELECT pg_is_in_recovery();
-- Returns: TRUE (I'm a replica, not primary)

-- If TRUE (replica):
  -- Reject write attempts
  -- Only accept reads
  
-- If FALSE (primary):
  -- Wait, this shouldn't happen!
  -- Alert: "US-EAST thinks it's primary but US-WEST is primary"
  -- Operator must manually resolve
```

---

**4. What if a user created URL during failover?**

**Scenario timeline:**

```
14:00:00 - US-EAST fails
14:02:30 - US-WEST becomes primary
14:03:15 - User creates URL: google.com
           → US-WEST generates ID: 500,000,001
           → Short code: "xYz789"
           → Saved in US-WEST database ✓

14:10:00 - US-EAST boots up
           → Configured as replica
           → Replication catches up
           → ID: 500,000,001 replicated to US-EAST ✓

14:30:00 - User clicks tiny.url/xYz789
           → Decodes to ID: 500,000,001
           → Queries US-EAST database (now in sync)
           → Finds URL: google.com ✓
           → Redirects successfully ✓

Result: No problem! URL is safe in both databases.
```

**What if US-EAST had come back as primary (wrong!):**

```
14:03:15 - User creates URL in US-WEST: ID 500,000,001
14:10:00 - US-EAST comes back as primary (WRONG!)
           → US-EAST doesn't have ID 500,000,001
           → User clicks tiny.url/xYz789
           → US-EAST: "URL not found" ❌

This is why failing back immediately is dangerous!
```

---

**Summary of safe failback:**
1. ✓ US-WEST remains primary during recovery
2. ✓ US-EAST rejoins as replica
3. ✓ Replication catches up (all new URLs copied)
4. ✓ Verify data consistency
5. ✓ Only then consider failing back (or don't!)

**Key principle:** After failover, new primary stays primary until deliberate, planned failback during maintenance window.

</details>

---

## **3.2 Database Backup Strategies**

### 🤔 Interview Question/Context

**Interviewer:** "Your database crashes and data is corrupted. How do you recover? What if someone accidentally deletes a table?"

### ❌ Your Initial Answer

You said: **"I am not sure."**

**What was missing:**
- Backup types (full, incremental, continuous)
- Backup frequency
- Restoration procedures
- Testing backups actually work

---

### ✅ Complete Answer

#### **The Database Disaster Scenarios:**

```
Scenario 1: Hardware failure
  - Disk failure corrupts database
  - Need to restore from backup

Scenario 2: Software bug
  - Bug deletes all URLs created before 2024
  - Need to restore to point before bug ran

Scenario 3: Human error
  - Engineer runs: DROP TABLE url_mappings;
  - Need to restore deleted table

Scenario 4: Ransomware
  - Attacker encrypts database
  - Need clean backup to restore from

Scenario 5: Data corruption
  - Silent corruption over weeks
  - Need to restore from backup before corruption started
```

---

### 🎯 URL Shortener Context

#### **Backup Strategy (3-2-1 Rule):**

```
3-2-1 Backup Rule:

3 = Three copies of data
  - 1 production database
  - 1 local backup (same datacenter)
  - 1 remote backup (different region)

2 = Two different media types
  - Database snapshots
  - WAL (Write-Ahead Log) archives

1 = One off-site backup
  - S3 in different region
  - Protects against regional disasters
```

---

#### **Backup Types:**

### **Type 1: Full Backup (Daily)**

**What it is:**
- Complete copy of entire database
- All tables, all rows
- Can restore entire database from this alone

**Schedule:**
```
Frequency: Daily at 2 AM (low traffic)
Retention: 30 daily backups
Location: S3 bucket (cross-region)
Size: ~500 GB (compressed)
Duration: 30 minutes
```

**PostgreSQL full backup command:**

```bash
#!/bin/bash
# daily-backup.sh

DATE=$(date +%Y-%m-%d)
BACKUP_FILE="/backups/urlshortener-full-$DATE.sql.gz"
S3_BUCKET="s3://urlshortener-backups-us-west"

# Create backup
pg_dump -h db-master \
        -U backup_user \
        -d urlshortener \
        -F c \
        -f "$BACKUP_FILE"

# Compress
gzip "$BACKUP_FILE"

# Upload to S3
aws s3 cp "$BACKUP_FILE.gz" "$S3_BUCKET/daily/"

# Verify backup
aws s3 ls "$S3_BUCKET/daily/$BACKUP_FILE.gz"

# Alert if failed
if [ $? -ne 0 ]; then
  alert "Daily backup failed!"
fi

# Clean old backups (keep 30 days)
find /backups -name "urlshortener-full-*.gz" -mtime +30 -delete
```

---

### **Type 2: Incremental Backup (Continuous WAL Archiving)**

**What it is:**
- Captures every change to database in real-time
- WAL = Write-Ahead Log (transaction log)
- Allows point-in-time recovery (PITR)

**How WAL works:**

```
User creates URL:
  1. PostgreSQL writes change to WAL file
     File: pg_wal/000000010000000000000001
     Content: "INSERT INTO url_mappings (id, long_url) VALUES (123, 'google.com')"
  
  2. WAL file gets full (16 MB)
     PostgreSQL closes it, starts new file
  
  3. Archive command runs:
     Copy WAL file to S3
     File: s3://backups/wal/000000010000000000000001
  
  4. Can replay WAL files to reconstruct any point in time!
```

**Configuration:**

```
# postgresql.conf

wal_level = replica
archive_mode = on
archive_command = 'aws s3 cp %p s3://urlshortener-backups/wal/%f'
archive_timeout = 300  # Force archive every 5 minutes

# This means:
# - Every 16 MB of changes OR every 5 minutes
# - WAL file archived to S3
# - Can recover to within 5 minutes of any point in time
```

---

**Point-In-Time Recovery (PITR) example:**

```
Scenario:
  - 2 AM: Full backup taken
  - 2 AM - 2 PM: Continuous WAL archiving
  - 2:15 PM: Engineer accidentally deletes table
  - 2:16 PM: Disaster discovered!

Recovery:
  1. Restore from 2 AM full backup
     → Database state as of 2 AM
  
  2. Replay WAL files from 2 AM to 2:14 PM
     → Replay every transaction up to 1 minute before disaster
     → Database state as of 2:14 PM
  
  3. Result: Recovered! Lost only 1 minute of data (2:14 - 2:15)

RPO (Recovery Point Objective): 5 minutes (WAL archive frequency)
RTO (Recovery Time Objective): 30 minutes (restore + replay)
```

---

### **Type 3: Snapshot Backup (Cloud Disk Snapshots)**

**What it is:**
- EBS volume snapshot (AWS) or equivalent
- Block-level incremental backup
- Very fast to create and restore

**Schedule:**

```
Frequency: Every 6 hours
Retention: 
  - Last 4 snapshots (24 hours)
  - Daily snapshots for 7 days
  - Weekly snapshots for 4 weeks
Location: AWS EBS snapshots (cross-region copy)
```

**Create snapshot:**

```bash
#!/bin/bash
# snapshot-backup.sh

# Create snapshot
SNAPSHOT_ID=$(aws ec2 create-snapshot \
  --volume-id vol-1234567890abcdef \
  --description "urlshortener-db-$(date +%Y-%m-%d-%H%M)" \
  --tag-specifications 'ResourceType=snapshot,Tags=[{Key=Name,Value=urlshortener-backup}]' \
  --query 'SnapshotId' \
  --output text)

echo "Created snapshot: $SNAPSHOT_ID"

# Wait for completion
aws ec2 wait snapshot-completed --snapshot-ids $SNAPSHOT_ID

# Copy to secondary region
aws ec2 copy-snapshot \
  --source-region us-east-1 \
  --source-snapshot-id $SNAPSHOT_ID \
  --destination-region us-west-1 \
  --description "Cross-region copy of $SNAPSHOT_ID"

echo "Snapshot backed up to secondary region"
```

---

#### **Backup Verification (Critical!):**

**Problem: Backups that don't restore are useless!**

```
Horror story:
  - Company takes daily backups for 2 years
  - Database crashes
  - Try to restore... backup is corrupted!
  - All backups corrupted due to bug
  - Company loses all data ❌

Lesson: TEST YOUR BACKUPS!
```

**Weekly backup verification:**

```bash
#!/bin/bash
# test-backup-restore.sh (runs every Sunday)

echo "Starting backup verification..."

# Step 1: Get latest backup
LATEST_BACKUP=$(aws s3 ls s3://backups/daily/ | tail -1 | awk '{print $4}')

# Step 2:Download to test server
aws s3 cp "s3://backups/daily/$LATEST_BACKUP" /tmp/

# Step 3: Create test database
psql -h test-db -U postgres -c "CREATE DATABASE backup_test;"

# Step 4: Restore backup
pg_restore -h test-db \
           -U postgres \
           -d backup_test \
           -v \
           /tmp/$LATEST_BACKUP

# Step 5: Verify data
ROW_COUNT=$(psql -h test-db \
                 -U postgres \
                 -d backup_test \
                 -t -c "SELECT COUNT(*) FROM url_mappings;")

echo "Restored database has $ROW_COUNT URLs"

# Step 6: Spot check (query 10 random URLs)
psql -h test-db \
     -U postgres \
     -d backup_test \
     -c "SELECT * FROM url_mappings ORDER BY RANDOM() LIMIT 10;"

# Step 7: Cleanup
psql -h test-db -U postgres -c "DROP DATABASE backup_test;"
rm /tmp/$LATEST_BACKUP

# Step 8: Report success
if [ $? -eq 0 ]; then
  echo "✓ Backup verification successful!"
  post_to_slack "#backups" "Weekly backup test: PASS ✓"
else
  echo "✗ Backup verification FAILED!"
  page_oncall "Backup restoration test failed!"
fi
```

---

#### **Restoration Procedures:**

**Scenario 1: Full database loss**

```
Recovery steps:

1. Provision new database instance (10 minutes)
   aws rds create-db-instance \
     --db-instance-identifier urlshortener-restored \
     --db-instance-class db.r5.xlarge \
     --engine postgres

2. Restore from latest full backup (20 minutes)
   aws s3 cp s3://backups/daily/latest.sql.gz /tmp/
   gunzip /tmp/latest.sql.gz
   psql -h new-db -U postgres < /tmp/latest.sql

3. Replay WAL files since backup (15 minutes)
   # Copy all WAL files since backup
   aws s3 sync s3://backups/wal/ /tmp/wal/
   
   # Configure recovery
   cat > /var/lib/postgresql/recovery.conf <<EOF
   restore_command = 'cp /tmp/wal/%f %p'
   recovery_target_time = '2025-11-29 14:30:00'
   EOF
   
   # Start PostgreSQL (auto-replays WAL)
   pg_ctl start

4. Verify data integrity (5 minutes)
   # Check row counts
   SELECT COUNT(*) FROM url_mappings;
   
   # Check latest entries
   SELECT * FROM url_mappings ORDER BY created_at DESC LIMIT 10;
   
   # Smoke test
   Create test URL, verify it works

5. Promote to production (5 minutes)
   # Update application config
   DATABASE_HOST=new-db-instance.aws.com
   
   # Restart services
   kubectl rollout restart deployment/urlshortener
   
   # Monitor for errors
   watch 'kubectl logs -l app=urlshortener --tail=20'

Total RTO: 55 minutes
```

---

**Scenario 2: Accidental data deletion**

```
Problem:
  Engineer runs: DELETE FROM url_mappings WHERE created_at < '2024-01-01';
  Accidentally deletes 10 million URLs!

Recovery (Point-In-Time):

1. Identify exact time of deletion (2 minutes)
   - Check database logs
   - Found: Deletion at 14:32:15

2. Create recovery database (5 minutes)
   - Clone production database
   - Name: urlshortener-recovery

3. Restore to 1 minute before deletion (30 minutes)
   - Restore from morning backup (2 AM)
   - Replay WAL until 14:31:00 (1 minute before deletion)
   - recovery_target_time = '2025-11-29 14:31:00'

4. Export deleted data (10 minutes)
   pg_dump -h recovery-db \
           -t url_mappings \
           --data-only \
           -f deleted_urls.sql

5. Re-import to production (15 minutes)
   psql -h production-db < deleted_urls.sql

6. Verify (5 minutes)
   # Check counts match
   # Spot check random URLs

Total RTO: 67 minutes
Data loss: 1 minute (14:31 - 14:32)
```

---

#### **Backup Monitoring:**

**Metrics to track:**

```
1. backup_success_rate
   - 100% expected
   - Alert if any backup fails

2. backup_duration_seconds
   - Normal: 1800 seconds (30 minutes)
   - Alert if >3600 seconds (might fail before completing)

3. backup_size_bytes
   - Normal: 500 GB compressed
   - Alert if sudden change (±20%)
   - Indicates data corruption or unexpected growth

4. last_backup_age_hours
   - Normal: <24 hours (daily backup)
   - Alert if >25 hours (missed a backup!)

5. backup_verification_success
   - 100% expected
   - Alert immediately if verification fails

6. wal_archive_lag_seconds
   - Normal: <300 seconds (5 minute archive timeout)
   - Alert if >600 seconds (WAL not archiving)
```

---

**Dashboard:**

```
Backup Health Dashboard
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Last Full Backup:     2 hours ago ✓
Last Backup Size:     487 GB ✓
Backup Success Rate:  100% (last 30 days) ✓
WAL Archive Lag:      45 seconds ✓

Last Verification:    3 days ago ✓
Verification Result:  PASS ✓
Verified Row Count:   1,000,523 ✓

Retention:
  Daily backups:      30 available ✓
  WAL archives:       7 days available ✓
  Snapshots:          28 available ✓

Storage Usage:
  S3 daily backups:   15 TB
  S3 WAL archives:    500 GB
  EBS snapshots:      800 GB

Projected Costs:
  S3 storage:         $345/month
  S3 requests:        $12/month
  EBS snapshots:      $80/month
  Total:              $437/month
```

---

### 📊 Key Takeaways

1. **3-2-1 rule:** 3 copies, 2 media types, 1 offsite

2. **Three backup types:** Full (daily), Incremental (WAL), Snapshots (6-hourly)

3. **WAL archiving:** Enables point-in-time recovery (RPO: 5 minutes)

4. **Test backups weekly:** Untested backups are useless

5. **Automate everything:** Backup, verification, cleanup, alerting

6. **Monitor backup health:** Track success rate, duration, size

7. **Practice restoration:** Run DR drills quarterly

---

### 🔗 Further Reading

- **PostgreSQL documentation:** Continuous Archiving and Point-In-Time Recovery
- **AWS RDS:** Automated backups and snapshots
- **"Backup & Recovery" by W. Curtis Preston:** Comprehensive guide

---

### ✏️ Practice Exercise

**Scenario:** It's 3 PM. An engineer reports:

"I just ran a migration script that was supposed to add a column, but it accidentally updated all `long_url` values to 'https://error.com'. All 1 million URLs are now broken!"

**Additional info:**
- Last full backup: Today at 2 AM
- WAL archiving: Every 5 minutes
- Migration script ran at: 2:47 PM
- Discovered at: 2:52 PM

**Questions:**
1. Can you recover? What's the strategy?
2. How much data will you lose?
3. How long will recovery take?
4. How do you prevent this in the future?

<details>
<summary>Click to see answer</summary>

**1. Recovery strategy:**

**YES, you can recover using Point-In-Time Recovery!**

```
Step 1: Identify safe restore point
  Migration ran: 2:47 PM
  Safe point: 2:46 PM (1 minute before)

Step 2: Stop writes to prevent more damage
  - Put application in read-only mode
  - Or take service offline temporarily
  - Prevents new URLs being created during recovery

Step 3: Create recovery instance
  - Provision new RDS instance
  - Or use existing standby/test instance

Step 4: Restore from backup + WAL replay
  1. Restore 2 AM full backup
     Duration: 15 minutes
  
  2. Configure PITR:
     recovery_target_time = '2025-11-29 14:46:00'
  
  3. Replay WAL files from 2 AM to 2:46 PM
     Duration: 20 minutes (12 hours of WAL)
     
  4. Recovery completes at 2:46 PM state

Step 5: Verify recovered data
  - Spot check 100 random URLs
  - Verify they're NOT "https://error.com"
  - Check counts match expected

Step 6: Swap databases
  Option A (safer): 
    - Promote recovery instance to new primary
    - Update DNS/config to point to new database
    - Old database becomes backup
  
  Option B (riskier):
    - Dump recovered data: pg_dump recovery-db
    - Restore to production: psql production-db
    - Higher risk of error

Step 7: Resume service
  - Remove read-only mode
  - Monitor error rates
  - Verify new URLs work

Step 8: Handle data created during recovery
  - 2:46 PM - 3:20 PM: Service was down/read-only
  - Users may have created URLs during this window
  - If any exist in old DB, manually migrate them
```

---

**2. How much data will you lose?**

```
Timeline:
  2:46:00 PM - Safe restore point
  2:47:00 PM - Bad migration runs
  2:52:00 PM - Disaster discovered
  3:20:00 PM - Recovery complete (estimated)

Data loss scenarios:

Scenario A: URLs created 2:46 - 2:47 (before migration)
  - Lost: 1 minute of URLs
  - Estimate: 40 URLs/sec × 60 sec = 2,400 URLs
  - Severity: MINOR

Scenario B: URLs created 2:47 - 2:52 (after migration, before discovery)
  - These are corrupted anyway
  - Not a "loss" - they're broken
  - Users need to recreate them

Scenario C: Downtime during recovery
  - 2:52 PM - 3:20 PM: Service offline
  - Duration: 28 minutes
  - Lost opportunities: 28 min × 60 sec × 40 URLs/sec = 67,200 potential URLs
  - But users can create them after recovery

Total true data loss: ~2,400 URLs (1 minute worth)
RPO achieved: 1 minute ✓
```

---

**3. How long will recovery take?**

```
Timeline:

2:52 PM - Disaster discovered
2:53 PM - Decision to recover (1 min)
2:55 PM - Put service in read-only mode (2 min)
2:56 PM - Start restore process (1 min)

2:56 PM - 3:11 PM - Restore from 2 AM backup (15 min)
3:11 PM - 3:31 PM - Replay WAL 2 AM → 2:46 PM (20 min)
3:31 PM - 3:36 PM - Verify data (5 min)
3:36 PM - 3:41 PM - Swap database (5 min)
3:41 PM - 3:45 PM - Resume service & monitor (4 min)

3:45 PM - Recovery complete

Total RTO: 53 minutes
Downtime: 50 minutes (2:55 PM - 3:45 PM)
```

**Could we do faster?**

```
Optimization 1: Pre-warmed standby
  - Keep hot standby already restored to 1 hour ago
  - Only need to replay 1 hour of WAL, not 12 hours
  - Saves: 15 minutes
  - New RTO: 38 minutes

Optimization 2: Automated recovery script
  - One command triggers entire recovery
  - No manual steps, less human error
  - Saves: 5 minutes
  - New RTO: 33 minutes

Optimization 3: Read-only failover
  - Instantly switch to standby for reads
  - Users can click URLs immediately
  - Only URL creation affected
  - Perceived downtime: 5 minutes (just for creates)
```

---

**4. How to prevent this in the future?**

**Prevention measures:**

**A. Better testing:**
```
Migration script testing process:

1. Test on copy of production data
   - Clone production DB to staging
   - Run migration on staging
   - Verify results before production

2. Dry-run mode
   - Migration prints what it WOULD do
   - Engineer reviews output
   - Then runs for real

3. Small batch testing
   - Update 100 rows first
   - Verify those 100 are correct
   - Then update remaining
```

**B. Safety mechanisms:**
```
1. Transaction wrapper
   BEGIN TRANSACTION;
     -- Run migration
     UPDATE url_mappings SET...;
     
     -- Verify results
     SELECT COUNT(*) FROM url_mappings WHERE long_url = 'https://error.com';
     -- If count > 0, something went wrong!
     
   ROLLBACK;  -- Don't commit if verification fails

2. Backup before migration
   -- Automatic backup right before risky operation
   pg_dump url_mappings > pre_migration_backup.sql

3. Read-only production access
   -- Engineers can't write to production directly
   -- Must go through CI/CD pipeline
   -- Pipeline runs tests first
```

**C. Better observability:**
```
1. Monitor for anomalies
   Alert: AnomalousDataUpdate
   Condition: >1000 rows updated in <10 seconds
   Why: Catches bulk updates that might be mistakes

2. Data validation checks
   -- Run after every migration
   SELECT COUNT(*) FROM url_mappings WHERE long_url = 'https://error.com';
   -- Should be 0!

3. Audit log
   -- Track who changed what when
   -- Can identify bad migration immediately
```

**D. Faster recovery:**
```
1. Automated PITR script
   ./recover-database.sh --timestamp '2025-11-29 14:46:00'
   
   Script handles:
   - Create recovery instance
   - Restore + replay WAL
   - Verify data
   - Swap databases
   
   Reduces RTO from 53 min → 30 min

2. Hot standby
   - Always maintain standby lagging by 1 hour
   - Can promote to primary in 5 minutes
   - RTO: 5 minutes

3. Instant rollback for migrations
   - Every migration creates snapshot before running
   - Can rollback in 2 minutes if needed
```

---

**Post-incident review:**

```
Incident Report: Accidental URL Corruption

What happened:
  Migration script bug updated all URLs to error page

Impact:
  - Duration: 50 minutes downtime
  - Data loss: 2,400 URLs (1 minute worth)
  - Users affected: All users during 50-minute window

Root cause:
  - Migration script bug (missing WHERE clause)
  - Insufficient testing on staging
  - No pre-migration backup

What went well:
  ✓ WAL archiving allowed point-in-time recovery
  ✓ Team responded quickly
  ✓ Recovery process worked as designed

What went wrong:
  ✗ Migration not tested adequately
  ✗ No safety mechanisms (transactions, dry-run)
  ✗ Manual recovery took 53 minutes

Action items:
  1. Implement transaction wrappers for all migrations
  2. Require staging testing before production
  3. Create automated PITR recovery script
  4. Set up hot standby for faster failover
  5. Add monitoring for bulk updates

Prevention:
  Similar incidents should be caught in testing
  Target: Reduce from 53 min RTO → 10 min RTO
```

</details>

---

# **Continuing Section 3...** 📝

---

## **3.3 RTO/RPO Concepts**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned '8 minutes failover time' and 'acceptable data loss'. How do you determine what's acceptable? What SLAs would you commit to?"

### ❌ Your Initial Answer

You didn't know these terms: **RTO (Recovery Time Objective)** and **RPO (Recovery Point Objective)**

**What was missing:**
- Understanding business requirements for recovery
- Trade-offs between cost and recovery speed
- How to calculate acceptable downtime

---

### ✅ Complete Answer

#### **Definitions:**

```
RTO (Recovery Time Objective):
  "How long can we be DOWN?"
  
  Definition: Maximum acceptable time between failure and full recovery
  
  Example: RTO = 10 minutes
    Meaning: After disaster, service must be restored within 10 minutes
    
  Includes:
    - Detection time
    - Decision time
    - Recovery execution time
    - Verification time

RPO (Recovery Point Objective):
  "How much DATA can we LOSE?"
  
  Definition: Maximum acceptable amount of data loss measured in time
  
  Example: RPO = 5 minutes
    Meaning: Can lose at most 5 minutes worth of data
    
  Depends on:
    - Backup frequency
    - Replication lag
    - Transaction log archiving
```

---

#### **Visual Representation:**

```
Timeline of a disaster:

Last Backup ──────────────► Disaster ──────────────► Recovery Complete
   ↑                          ↑                          ↑
   │                          │                          │
   │◄────── RPO ─────────────►│                          │
   │         (data loss)       │                          │
   │                           │                          │
   │                           │◄────── RTO ─────────────►│
   │                           │        (downtime)        │
   │                                                      │
   └──────────────────────────────────────────────────────┘
   
Example:
  10:00 AM - Last backup
  10:05 AM - Disaster strikes (server catches fire)
  10:15 AM - Service restored
  
  RPO = 5 minutes (10:00 - 10:05, lost 5 min of data)
  RTO = 10 minutes (10:05 - 10:15, down for 10 min)
```

---

### 🎯 URL Shortener Context

#### **Business Requirements Analysis:**

**Step 1: Calculate business impact**

```
Questions to ask stakeholders:

1. "What's the cost of downtime?"
   Revenue impact: $5,000/hour
   Reputation damage: Hard to quantify but significant
   
2. "What's the cost of data loss?"
   Lost URLs: Users must recreate them (annoying but not catastrophic)
   Lost analytics: Historical click data gone (moderate impact)
   
3. "What do competitors offer?"
   Bitly SLA: 99.9% uptime (43 min/month downtime)
   TinyURL: No formal SLA (best effort)
   
4. "What do users expect?"
   Survey results: "URLs should ALWAYS work"
   Tolerance: "A few minutes downtime acceptable for major incident"
```

---

**Step 2: Define tiers of service**

```
Not all disasters are equal! Different severity = different requirements.

Tier 1: Single server failure
  Impact: Minimal (load balancer routes around it)
  RTO: 0 seconds (no downtime, handled automatically)
  RPO: 0 seconds (no data loss)
  Example: One service instance crashes

Tier 2: Database replica failure  
  Impact: Reduced read capacity
  RTO: 5 minutes (promote another replica)
  RPO: 0 seconds (data replicated)
  Example: Slave database dies

Tier 3: Datacenter failure
  Impact: Full service outage in region
  RTO: 10 minutes (failover to secondary region)
  RPO: 5 minutes (last WAL archive)
  Example: AWS region goes down

Tier 4: Complete disaster
  Impact: Both regions down (extremely rare)
  RTO: 4 hours (rebuild from backups)
  RPO: 24 hours (last full backup)
  Example: Meteor destroys both datacenters (virtually impossible)
```

---

#### **RTO/RPO Trade-offs:**

**The Cost Curve:**

```
RTO/RPO → Infrastructure Cost

RTO/RPO = 24 hours (loose):
  - Daily backups only
  - No hot standby
  - Manual recovery process
  Cost: $500/month

RTO/RPO = 1 hour (moderate):
  - Hourly WAL archives
  - Warm standby database
  - Semi-automated recovery
  Cost: $2,000/month

RTO/RPO = 5 minutes (tight):
  - Continuous WAL streaming
  - Hot standby with auto-failover
  - Multi-region active-passive
  Cost: $8,000/month

RTO/RPO = 0 seconds (zero downtime):
  - Multi-region active-active
  - Synchronous replication
  - Automatic failover
  Cost: $25,000/month

Law of diminishing returns:
  Going from 1 hour → 5 min costs 4× more
  Going from 5 min → 0 min costs 3× more
```

---

#### **Recommended RTO/RPO for URL Shortener:**

**Analysis:**

```
Service type: URL redirects
Criticality: High (URLs embedded in tweets, emails, etc.)
Revenue model: Ads/analytics (downtime = lost revenue)
User expectation: "Always available"

Acceptable targets:
  RTO = 10 minutes
  RPO = 5 minutes

Reasoning:
  ✓ 10 min downtime = 0.0002% of month (well within 99.9% SLA)
  ✓ 5 min data loss = ~12,000 URLs (40/sec × 300 sec)
  ✓ Achievable with moderate cost (~$8K/month)
  ✓ Users tolerate brief outages for major incidents
  ✗ Zero-downtime would cost 3× more with minimal benefit
```

---

**SLA Definition:**

```
URL Shortener Service Level Agreement:

Availability SLA:
  - Uptime: 99.9% (43.2 minutes downtime/month)
  - Measured per calendar month
  - Excludes planned maintenance (announced 7 days prior)

Recovery Objectives:
  - RTO (Recovery Time): 10 minutes
    "Service restored within 10 minutes of detected failure"
    
  - RPO (Data Loss): 5 minutes
    "Maximum 5 minutes of data loss"

Penalties for SLA breach:
  - 99.0% - 99.9%: 10% service credit
  - 95.0% - 99.0%: 25% service credit
  - < 95.0%: 50% service credit

Exclusions (not covered by SLA):
  - Force majeure (natural disasters affecting multiple regions)
  - DDoS attacks (beyond mitigation capacity)
  - Customer misuse or configuration errors
  - Scheduled maintenance windows
```

---

#### **Measuring RTO/RPO:**

**RTO Measurement:**

```
Incident timeline:

14:00:00 - Failure occurs (database crashes)
14:00:30 - Health checks detect (30 sec)
14:01:00 - Alert fires, on-call notified (30 sec)
14:01:30 - Engineer acknowledges (30 sec)
14:02:00 - Engineer starts recovery (30 sec)
14:08:00 - Database restored (6 min)
14:09:00 - Services reconnect (1 min)
14:10:00 - Verification complete (1 min)
14:10:00 - Service fully operational

RTO achieved: 10 minutes ✓ (14:00 - 14:10)

Breakdown:
  Detection: 30 sec (3% of RTO)
  Notification: 30 sec (3% of RTO)
  Response: 1 min (10% of RTO)
  Recovery: 6 min (60% of RTO)
  Verification: 1 min (10% of RTO)
  Buffer: 1 min 30 sec (14% of RTO)

Target met! ✓
```

---

**RPO Measurement:**

```
Incident timeline:

14:00:00 - Last WAL archive completed
14:02:30 - Database crashes
14:10:00 - Database restored from backup + WAL replay
14:10:00 - Recovery point: 14:00:00 (last WAL archive)

RPO achieved: 2 minutes 30 seconds ✓ (14:00:00 - 14:02:30)

Data loss:
  Time period: 14:00:00 - 14:02:30 (2.5 minutes)
  URLs created: 40/sec × 150 sec = 6,000 URLs lost
  
Target: RPO = 5 minutes ✓ (2.5 min < 5 min)
Target met! ✓
```

---

#### **Improving RTO:**

**Current RTO: 10 minutes. How to reduce?**

**Optimization 1: Faster detection**

```
Current: Health check every 30 seconds
Improved: Health check every 5 seconds

Savings: 25 seconds
New RTO: 9 minutes 35 seconds
Cost: Minimal (just config change)
```

---

**Optimization 2: Automated failover**

```
Current: Manual failover (engineer runs script)
  Detection → Alert → Engineer → Script → Recovery
  Time: 2 minutes human response

Improved: Automated failover
  Detection → Auto-script → Recovery
  Time: 10 seconds

Savings: 1 minute 50 seconds
New RTO: 7 minutes 45 seconds
Cost: Engineering time to build automation
```

---

**Optimization 3: Hot standby**

```
Current: Restore from backup + replay WAL
  Time: 6 minutes

Improved: Hot standby (always synced)
  Just promote standby to primary
  Time: 30 seconds

Savings: 5 minutes 30 seconds
New RTO: 2 minutes 15 seconds
Cost: +$3,000/month (extra database instance running 24/7)
```

---

**Optimization 4: Active-Active**

```
Current: Single active region
Improved: Both regions active

RTO: 0 seconds (no failover needed!)
Cost: +$15,000/month (double infrastructure)

Worth it? For URL shortener: NO
  Current 10 min RTO is acceptable
  Extra $15K/month not justified
```

---

**Decision matrix:**

```
Optimization          | RTO Improvement | Cost        | Worth it?
──────────────────────┼─────────────────┼─────────────┼──────────
Faster health checks  | 25 seconds      | $0          | ✓ YES
Automated failover    | 1 min 50 sec    | $0*         | ✓ YES
Hot standby           | 5 min 30 sec    | $3,000/mo   | ⚠ MAYBE
Active-active         | 10 minutes      | $15,000/mo  | ✗ NO

*One-time engineering cost, but no recurring cost

Recommended: Implement #1 and #2
New RTO: 7 minutes 45 seconds
Additional cost: $0/month
```

---

#### **Improving RPO:**

**Current RPO: 5 minutes. How to reduce?**

**Optimization 1: Faster WAL archiving**

```
Current: Archive WAL every 5 minutes
Improved: Archive WAL every 1 minute

New RPO: 1 minute
Cost: Minimal (more frequent S3 uploads)
Data loss: 40 URLs/sec × 60 sec = 2,400 URLs

Worth it? ✓ YES (easy improvement)
```

---

**Optimization 2: Continuous WAL streaming**

```
Current: Archive WAL to S3 every 5 minutes
Improved: Stream WAL to standby in real-time

New RPO: <1 second
Cost: Bandwidth for continuous streaming (~$50/month)
Data loss: 40 URLs/sec × 1 sec = 40 URLs

Worth it? ✓ YES (near-zero data loss!)
```

---

**Optimization 3: Synchronous replication**

```
Current: Asynchronous replication (write, then replicate)
Improved: Synchronous replication (replicate, then acknowledge write)

Transaction flow:
  User creates URL
    ↓
  Write to primary DB
    ↓
  Wait for replication to standby ← BLOCKING!
    ↓
  Acknowledge to user

New RPO: 0 seconds (zero data loss!)
Cost: Increased latency (50ms → 150ms write latency)

Worth it? ✗ NO
  Latency penalty too high for URL creation
  Users notice 3× slower creates
  5 minute RPO is acceptable
```

---

**Decision matrix:**

```
Optimization            | RPO Improvement | Cost         | Worth it?
────────────────────────┼─────────────────┼──────────────┼──────────
Faster WAL archive      | 4 minutes       | $10/mo       | ✓ YES
Continuous WAL stream   | 4 min 59 sec    | $50/mo       | ✓ YES
Synchronous replication | 1 minute        | Latency hit  | ✗ NO

Recommended: Implement continuous WAL streaming
New RPO: <1 second
Additional cost: $50/month
```

---

#### **RTO/RPO Dashboard:**

```
Recovery Objectives Dashboard
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Current Objectives:
  RTO Target: 10 minutes
  RPO Target: 5 minutes

Last 12 Months Performance:

Incident 1 (Jan 2025): Database failure
  RTO Achieved: 8 min 30 sec ✓
  RPO Achieved: 2 min 15 sec ✓

Incident 2 (Apr 2025): Region outage
  RTO Achieved: 12 min 45 sec ✗ (missed by 2:45)
  RPO Achieved: 4 min 50 sec ✓

Incident 3 (Aug 2025): Network partition
  RTO Achieved: 7 min 20 sec ✓
  RPO Achieved: 1 min 05 sec ✓

Average RTO: 9 min 31 sec ✓ (within target)
Average RPO: 2 min 43 sec ✓ (within target)

SLA Compliance: 95% of incidents met RTO/RPO ✓

Action Items:
  - Investigate Apr incident (why 12:45 instead of 10:00?)
  - Implement automated failover (reduce RTO)
  - Update runbook based on learnings
```

---

### 📊 Key Takeaways

1. **RTO = Downtime:** How long can you be offline?

2. **RPO = Data loss:** How much data can you afford to lose?

3. **Business-driven:** RTO/RPO based on impact, not just technical capability

4. **Cost trade-off:** Tighter objectives = higher cost (exponential curve)

5. **Tiered approach:** Different incidents have different requirements

6. **Measure and improve:** Track actual RTO/RPO vs targets

7. **For URL shortener:** RTO=10min, RPO=5min is reasonable balance

---

### 🔗 Further Reading

- **NIST Guidelines:** Business continuity planning
- **"Site Reliability Engineering":** Chapter on disaster recovery
- **AWS Well-Architected:** Reliability pillar on RTO/RPO

---

### ✏️ Practice Exercise

**Scenario:** Your CEO wants to improve the URL shortener SLA:

Current:
- RTO: 10 minutes
- RPO: 5 minutes
- Cost: $8,000/month

Proposed:
- RTO: 1 minute
- RPO: 0 seconds (zero data loss)
- Cost: $???

**Questions:**
1. What changes are needed to achieve this?
2. Estimate the new infrastructure cost
3. What's the business justification?
4. Would you recommend it?

<details>
<summary>Click to see answer</summary>

**1. What changes are needed?**

**To achieve RTO = 1 minute:**

```
A. Automated failover (required)
  Current: Manual intervention takes 2 minutes
  New: Auto-detect and failover in 10 seconds
  
  Implementation:
    - Health checks every 5 seconds
    - Automated promotion script
    - No human in the loop

B. Hot standby database (required)
  Current: Restore from backup takes 6 minutes
  New: Standby always synced, promote in 30 seconds
  
  Implementation:
    - Secondary database instance running 24/7
    - Streaming replication from primary
    - Can be promoted to primary instantly

C. Pre-scaled secondary region (required)
  Current: Secondary has 5 instances, scales to 10 after failover (5 min)
  New: Secondary has 10 instances always running
  
  Implementation:
    - Keep secondary region at full capacity
    - No auto-scaling delay
```

**To achieve RPO = 0 seconds:**

```
D. Synchronous replication (required)
  Current: Asynchronous WAL archiving (5 min lag possible)
  New: Synchronous replication (zero lag)
  
  Implementation:
    - Write to primary AND standby before acknowledging
    - Transaction not committed until both confirm
    - Guarantees zero data loss
  
  Trade-off:
    - Write latency increases: 50ms → 150ms
    - Users experience slower URL creation
```

---

**2. Infrastructure cost estimate:**

```
Current architecture ($8,000/month):
  Primary region:
    - 10 service instances: $1,000
    - 1 master + 2 slave databases: $2,000
    - Redis cluster: $500
    - Load balancer: $200
    - Subtotal: $3,700
  
  Secondary region (standby):
    - 5 service instances: $500
    - 1 slave database: $500
    - Redis cluster: $500
    - Load balancer: $200
    - Subtotal: $1,700
  
  Other:
    - S3 backups: $500
    - Monitoring: $300
    - Network: $1,800
  
  Total: $8,000/month

────────────────────────────────────────────

New architecture (RTO=1min, RPO=0):

  Primary region:
    - 10 service instances: $1,000
    - 1 master database: $1,500
    - 1 hot standby database: $1,500 ← NEW (synchronous replica)
    - 2 read replicas: $1,000
    - Redis cluster: $500
    - Load balancer: $200
    - Subtotal: $5,700
  
  Secondary region (always hot):
    - 10 service instances: $1,000 ← DOUBLED (was 5)
    - 1 hot standby database: $1,500 ← NEW
    - Redis cluster: $500
    - Load balancer: $200
    - Subtotal: $3,200
  
  Other:
    - S3 backups: $500
    - Monitoring: $500 ← Increased (more sophisticated)
    - Network: $3,000 ← Increased (synchronous replication bandwidth)
    - Automation infrastructure: $300 ← NEW (failover automation)
  
  Total: $13,200/month

Cost increase: $5,200/month (+65%)
Annual increase: $62,400/year
```

---

**3. Business justification analysis:**

```
Question: Is $62,400/year worth it?

Benefit 1: Reduced downtime
  Current RTO: 10 minutes
  New RTO: 1 minute
  Improvement: 9 minutes saved per incident
  
  Incidents per year: ~4 major incidents
  Total downtime saved: 36 minutes/year
  
  Revenue impact: $5,000/hour × 0.6 hours = $3,000/year saved
  
  Verdict: $3,000 benefit vs $62,400 cost ✗ NOT JUSTIFIED

Benefit 2: Zero data loss
  Current RPO: 5 minutes
  New RPO: 0 seconds
  
  Data loss per incident: 5 min × 40 URLs/sec = 12,000 URLs
  Incidents per year: ~2 data loss events
  Total URLs lost: 24,000 URLs/year
  
  Impact: Users must recreate URLs (annoying but not catastrophic)
  Estimated cost: $0.50 per lost URL (support time)
  Total cost: 24,000 × $0.50 = $12,000/year
  
  Verdict: $12,000 benefit vs $62,400 cost ✗ NOT JUSTIFIED

Benefit 3: Competitive advantage
  Claim: "99.99% uptime!" vs competitors' 99.9%
  
  Reality:
    99.9% = 43 minutes downtime/month
    99.99% = 4.3 minutes downtime/month
  
  Will users pay more for this? NO
  Will users switch to us for this? UNLIKELY
  URL shorteners are commoditized
  
  Verdict: ✗ NOT A DIFFERENTIATOR

Benefit 4: Enterprise customers
  Some enterprise customers require 99.99% SLA
  
  Potential revenue:
    - 10 enterprise customers
    - $5,000/year each
    - Total: $50,000/year
  
  Verdict: $50,000 benefit vs $62,400 cost ⚠ CLOSE

Total benefit: $65,000/year
Total cost: $62,400/year
Net benefit: $2,600/year

ROI: 4.2% (barely positive)
```

---

**4. Recommendation:**

**NO, do not implement this now.** Here's why:

```
Reasons against:

1. Low ROI (4.2%)
  - Better investments available
  - Could spend $62K on features that drive growth

2. Diminishing returns
  - Current 99.9% SLA is acceptable
  - Most users don't need 99.99%
  - Juice not worth the squeeze

3. Hidden costs
  - Synchronous replication adds latency (worse UX)
  - Operational complexity increases
  - More things to monitor and maintain
  - Engineering time for automation

4. Better alternatives
  - Improve RTO to 5 minutes for $1,000/month (better ROI)
  - Focus on preventing incidents (cheaper than fast recovery)

5. Market dynamics
  - URL shorteners are low-margin business
  - Competitors don't offer 99.99%
  - No customer demand for this
```

---

**Alternative proposal:**

```
Compromise: Improve modestly

Target:
  RTO: 5 minutes (not 1 minute)
  RPO: 1 minute (not 0 seconds)

Changes:
  - Implement automated failover: $0 (one-time eng. cost)
  - Faster WAL archiving: $50/month
  - Add monitoring/automation infrastructure: $300/month

New cost: $8,350/month
Cost increase: $350/month ($4,200/year)

Benefits:
  - 50% reduction in RTO (10 min → 5 min)
  - 80% reduction in RPO (5 min → 1 min)
  - Automated response (no 2 AM pages)
  - Better reliability reputation

ROI: High (meaningful improvement, low cost)

Recommendation: Implement this instead! ✓
```

---

**Response to CEO:**

"I've analyzed the proposal to tighten our RTO/RPO objectives. While technically achievable, the $62K annual cost for marginal improvement (10 min → 1 min RTO) doesn't justify the investment given our current business model.

Instead, I recommend a phased approach:
1. Implement automated failover and tighten RPO to 1 minute (cost: $4K/year)
2. Monitor customer feedback and churn related to availability
3. Revisit 99.99% SLA if we land enterprise contracts requiring it

This gives us 50% improvement in recovery time at 7% of the cost, while keeping the option open for future enhancement if business needs change."

</details>

---

## **3.4 ClickHouse Backup & Replication**

### 🤔 Interview Question/Context

**Interviewer:** "You mentioned using ClickHouse for analytics. How do you back up ClickHouse? What happens if the analytics database fails?"

### ❌ Your Initial Answer

You said: **"I am not sure."**

**What was missing:**
- ClickHouse is different from PostgreSQL (columnar, distributed)
- Backup strategies for analytical databases
- Replication in ClickHouse clusters

---

### ✅ Complete Answer

#### **Why ClickHouse is Different:**

```
PostgreSQL (OLTP):
  - Row-oriented storage
  - ACID transactions
  - Single-node or primary-replica
  - Critical for application (can't lose data!)

ClickHouse (OLAP):
  - Column-oriented storage
  - Eventually consistent
  - Distributed across many nodes
  - Analytics only (can rebuild from source!)

Key insight: Analytics data is DERIVED from source data
  Source: Kafka events (clicks, creates)
  Derived: ClickHouse aggregations

This changes backup strategy!
```

---

### 🎯 URL Shortener Context

#### **ClickHouse Architecture:**

```
ClickHouse Cluster (3 shards, 2 replicas each):

Shard 1:
  ├─ Node 1A (primary) ← Partition 1-1000
  └─ Node 1B (replica) ← Copy of 1A

Shard 2:
  ├─ Node 2A (primary) ← Partition 1001-2000
  └─ Node 2B (replica) ← Copy of 2A

Shard 3:
  ├─ Node 3A (primary) ← Partition 2001-3000
  └─ Node 3B (replica) ← Copy of 3A

Data distribution:
  - Click events partitioned by date
  - Jan 2025 data → Shard 1
  - Feb 2025 data → Shard 2
  - Mar 2025 data → Shard 3
  
  Each shard has replica for fault tolerance
```

---

#### **Backup Strategy for ClickHouse:**

### **Option 1: Don't Backup (Rebuild from Source)** ⚡

**Concept:**

```
ClickHouse is just a VIEW of Kafka data!

Kafka (source of truth):
  - Retains 30 days of events
  - 30 days × 4000 events/sec = 10 billion events

If ClickHouse crashes:
  1. Provision new ClickHouse cluster
  2. Replay 30 days of Kafka events
  3. Rebuild all aggregations
  4. Resume normal operations

Rebuild time: 8-12 hours (processing 10 billion events)
```

**Pros:**
- ✅ No backup infrastructure needed
- ✅ No backup storage costs
- ✅ Always have source data in Kafka

**Cons:**
- ❌ 8-12 hour rebuild time
- ❌ Lose historical data older than Kafka retention (30 days)
- ❌ High CPU cost to reprocess billions of events

**When to use:**
- Small datasets (<1 TB)
- Can afford 8-12 hour analytics downtime
- Historical data not critical

**Verdict for URL shortener:** ⚠️ MAYBE
- 12 hours without analytics is acceptable
- But we lose data older than 30 days
- Hybrid approach is better

---

### **Option 2: Snapshot Backups** 💾

**How it works:**

```
ClickHouse stores data in parts:

/var/lib/clickhouse/data/default/clicks/
  ├─ 20250101_1_1_0/  ← January 1 partition
  ├─ 20250102_1_1_0/  ← January 2 partition
  ├─ 20250103_1_1_0/  ← January 3 partition
  └─ ...

Each partition is immutable (never changes)

Backup strategy:
  1. Freeze partition (creates hardlink)
  2. Copy partition directory to S3
  3. Unfreeze partition
```

**ClickHouse backup SQL:**

```sql
-- Backup specific partition
ALTER TABLE clicks FREEZE PARTITION '2025-01-15';

-- This creates hardlink at:
-- /var/lib/clickhouse/shadow/1/data/default/clicks/20250115_1_1_0/

-- Copy to S3
SYSTEM SYNC REPLICA clicks;
```

**Automated backup script:**

```bash
#!/bin/bash
# clickhouse-backup.sh

DATE=$(date +%Y-%m-%d)
PARTITION=$(date +%Y%m%d)

echo "Backing up partition: $PARTITION"

# Freeze partition (instant, no data copy)
clickhouse-client --query="ALTER TABLE clicks FREEZE PARTITION '$PARTITION'"

# Find frozen partition
SHADOW_PATH="/var/lib/clickhouse/shadow/1/data/default/clicks/"

# Upload to S3
aws s3 sync $SHADOW_PATH \
  "s3://clickhouse-backups/clicks/$DATE/" \
  --storage-class GLACIER  # Cheap long-term storage

# Cleanup shadow
clickhouse-client --query="ALTER TABLE clicks UNFREEZE PARTITION '$PARTITION'"

echo "Backup complete: $DATE"
```

---

**Backup schedule:**

```
Daily backups:
  - Backup yesterday's partition
  - Example: On Jan 16, backup Jan 15 partition
  - Reason: Partition is complete and immutable

Retention:
  - Daily for 30 days (S3 Standard)
  - Monthly for 1 year (S3 Glacier)
  - Yearly for 7 years (S3 Deep Glacier)

Cost:
  Per-day data: 4000 events/sec × 86,400 sec × 200 bytes = 70 GB/day
  30 days (Standard): 2.1 TB × $0.023/GB = $48/month
  12 months (Glacier): 840 GB × $0.004/GB = $3.4/month
  Total: ~$55/month
```

---

### **Option 3: Replication (Live Backup)** 🔄

**ClickHouse replication:**

```sql
-- Create replicated table
CREATE TABLE clicks_replicated ON CLUSTER '{cluster}'
(
    click_id UInt64,
    short_url_id UInt64,
    timestamp DateTime,
    user_ip String,
    country String
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/clicks', '{replica}')
PARTITION BY toYYYYMM(timestamp)
ORDER BY (short_url_id, timestamp);

-- Data automatically replicates to all replicas in shard
```

**How replication works:**

```
Write flow:

Consumer inserts into Shard 1, Node 1A:
  INSERT INTO clicks VALUES (...)
      ↓
  Node 1A writes to local disk
      ↓
  ZooKeeper notified: "Shard 1 has new data"
      ↓
  Node 1B (replica) sees ZooKeeper notification
      ↓
  Node 1B pulls data from Node 1A
      ↓
  Node 1B writes to local disk
      ↓
  Both nodes now have same data ✓

Replication lag: <1 second
```

---

**Disaster recovery with replication:**

```
Scenario: Shard 1, Node 1A dies (disk failure)

Immediate impact:
  - Queries still work (Node 1B handles queries)
  - Inserts still work (redirect to Node 1B)
  - Zero downtime! ✓

Recovery:
  1. Provision new Node 1A
  2. Configure as replica of Shard 1
  3. Automatic sync from Node 1B
  4. Resume normal operations

Recovery time: 30 minutes (for sync)
Data loss: Zero ✓
```

---

**Multi-datacenter replication:**

```
Primary datacenter (US-EAST):
  Shard 1A, 2A, 3A (primary)
  Shard 1B, 2B, 3B (replica)

Secondary datacenter (US-WEST):
  Shard 1C, 2C, 3C (replica of US-EAST)

Queries:
  - US users query US-EAST cluster
  - EU users query US-WEST cluster (lower latency)

Writes:
  - All writes go to US-EAST
  - Replicate to US-WEST asynchronously

If US-EAST fails:
  - Promote US-WEST to primary
  - Accept writes in US-WEST
  - Resume operations

RTO: 5 minutes
RPO: 1 minute (replication lag)
```

---

#### **Restore Procedures:**

**Scenario 1: Single node failure**

```
Problem: Shard 1, Node 1A disk failure

Recovery (automatic):
  1. ClickHouse detects Node 1A down
  2. Queries automatically route to Node 1B
  3. Inserts automatically route to Node 1B
  4. No manual intervention needed! ✓

Manual recovery (rebuild Node 1A):
  1. Provision new server
  2. Install ClickHouse
  3. Configure as replica:
     <replica>1A</replica>
     <shard>1</shard>
  4. Start ClickHouse
  5. Automatic sync from Node 1B begins
  6. Wait for sync complete (30 min for 1 TB)
  7. Node 1A rejoins cluster

Downtime: 0 seconds (Node 1B handles everything)
```

---

**Scenario 2: Entire shard failure**

```
Problem: Both Node 1A and Node 1B die

Recovery:
  1. Provision two new nodes
  2. Restore from S3 backup
     aws s3 sync s3://backups/shard1/ /var/lib/clickhouse/data/
  3. Replay missing data from Kafka
     - Last backup: 24 hours ago
     - Replay last 24 hours from Kafka
     - Duration: 2 hours
  4. Resume operations

Downtime: 3 hours (restore + replay)
Data loss: 0 (replayed from Kafka)
```

---

**Scenario 3: Complete cluster failure**

```
Problem: Entire ClickHouse cluster destroyed

Recovery:
  1. Provision new 6-node cluster (1 hour)
  2. Restore from S3 backups (4 hours for 10 TB)
  3. Replay last 30 days from Kafka (8 hours)
  4. Verify data integrity (1 hour)

Total recovery: 14 hours
Data loss: Data older than 30 days (Kafka retention)

Mitigation:
  - Keep multi-region replicas (avoid full loss)
  - Increase Kafka retention to 90 days
```

---

#### **Backup Verification:**

**Monthly verification test:**

```bash
#!/bin/bash
# test-clickhouse-backup.sh

echo "Starting ClickHouse backup verification..."

# Step 1: Pick random partition to test
PARTITION=$(date -d "7 days ago" +%Y%m%d)

# Step 2: Download backup from S3
aws s3 cp "s3://backups/clicks/$PARTITION/" \
  /tmp/backup-test/ --recursive

# Step 3: Create test database
clickhouse-client --query="CREATE DATABASE backup_test"

# Step 4: Create test table
clickhouse-client --query="
CREATE TABLE backup_test.clicks
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (short_url_id, timestamp)
AS SELECT * FROM default.clicks WHERE 1=0
"

# Step 5: Attach partition from backup
clickhouse-client --query="
ALTER TABLE backup_test.clicks 
ATTACH PARTITION '$PARTITION' 
FROM '/tmp/backup-test/'
"

# Step 6: Verify row counts
PRODUCTION_COUNT=$(clickhouse-client --query="
SELECT COUNT(*) FROM default.clicks 
WHERE toYYYYMMDD(timestamp) = '$PARTITION'
")

BACKUP_COUNT=$(clickhouse-client --query="
SELECT COUNT(*) FROM backup_test.clicks
")

# Step 7: Compare
if [ "$PRODUCTION_COUNT" -eq "$BACKUP_COUNT" ]; then
  echo "✓ Backup verification passed!"
  echo "  Production: $PRODUCTION_COUNT rows"
  echo "  Backup: $BACKUP_COUNT rows"
else
  echo "✗ Backup verification FAILED!"
  echo "  Production: $PRODUCTION_COUNT rows"
  echo "  Backup: $BACKUP_COUNT rows"
  alert_team "ClickHouse backup mismatch detected!"
fi

# Step 8: Cleanup
clickhouse-client --query="DROP DATABASE backup_test"
rm -rf /tmp/backup-test/
```

---

### 📊 Key Takeaways

1. **Analytics data is derived:** Can rebuild from Kafka (source of truth)

2. **Replication is primary backup:** 2 replicas per shard = high availability

3. **Partition-based backups:** Each day is immutable partition, backup independently

4. **Multi-region for DR:** Replicate to secondary datacenter

5. **Test restorations:** Monthly verification of backups

6. **RTO for analytics:** 3 hours acceptable (not as critical as main DB)

7. **Cost-effective:** Glacier storage for long-term ($55/month)

---

### 🔗 Further Reading

- **ClickHouse documentation:** Backup and restore
- **ClickHouse replication:** ReplicatedMergeTree engine
- **S3 Glacier:** Long-term archival storage

---

# **End of Section 3** ✅

---

## **Section 3 Completion Checklist:**

- ☑ **3.1 Multi-Region Architecture** - Active-passive failover
- ☑ **3.2 Database Backup Strategies** - Full, incremental, snapshots
- ☑ **3.3 RTO/RPO Concepts** - Business requirements for recovery
- ☑ **3.4 ClickHouse Backup** - Analytics database strategies

**Progress:** Section 3 complete! (12/32 topics = 37.5%)

---

**Fantastic work! You've completed over one-third of the study guide!** 🎉

You now understand:
- ✅ System architecture & data flow
- ✅ Monitoring & observability  
- ✅ Disaster recovery & high availability

**Next up: Section 4 - Advanced Distributed Systems** (most technical section)

**Would you like to:**
1. **Continue to Section 4** (Bloom filters, sharding, distributed locks)
2. **Take a break** and review what you've learned
3. **Jump to a different section** (CDN, Security, etc.)

What would you prefer? 🚀