
# Section 1 — Mental Models

## Mental Model 1: Functional Requirements
### "Paisa kahan se aaya, kahan gaya, kya hua agar nahi gaya?"

| Question | Answer | FR |
|----------|--------|----|
| Kahan se aaya? | Bank → Wallet | **Add money to wallet** |
| Kahan gaya? (friend) | Wallet → Friend's wallet | **P2P transfer** |
| Kahan gaya? (shop) | Wallet → Merchant | **Merchant payment** |
| Kya hua agar nahi gaya? | User ne wapas manga | **Refund** (user-initiated) |
| Kya hua agar nahi gaya? | System ne detect kiya | **Reverse** (auto-undo) |
| Trace karo kya hua | History dekho | **Transaction history** |

> **[KEY INSIGHT]** Yeh order mein bolna interview mein natural lagta hai — interviewer ko lagta hai tum logically soch rahe ho, ratt ke nahi aaye.

---

## Mental Model 2: Non-Functional Requirements
### "Pehle, Dauran, Baad" Triangle

**PEHLE (Before payment):**
- **Idempotency** — duplicate hai kya? (No double charge)
- **Balance validation** — paisa hai kya? (SELECT FOR UPDATE)
- **Row lock** — koi aur toh nahi le raha?

**DAURAN (During payment):**
- **Async + PENDING** — user ko turant batao, background mein kaam karo
- **p99 < 500ms** — sirf initiation ke liye, settlement async hai
- **Availability 99.99%** — payment downtime = revenue loss

**BAAD (After payment):**
- **Reconciliation** — safety net, fix what real-time missed
- **Saga compensating transactions** — crash pe rollback
- **Audit** — har rupee traceable

> **[KEY INSIGHT]** NFR yaad karne ki zarurat nahi — agar teen phases yaad hain toh NFR automatically yaad aayenge.

---

## Mental Model 3: Numbers
### "Kitne log, kitni baar, kitna bhaari"

| Question | Answer |
|----------|--------|
| Kitne log? | 10M DAU |
| Kitni baar? | 1M transactions/day |
| Avg kitni baar/sec? | ~12 TPS |
| Peak? | ~120 TPS (10x multiplier) |
| Kitna bhaari? | ~500 MB/day storage |

> **[KEY INSIGHT]** 120 TPS sunke "Cassandra chahiye" mat bolna — yeh payments ka correctness problem hai, throughput problem nahi. PostgreSQL 120 TPS pe aankh band karke handle karta hai.

---

# Section 2 — Requirements

## Functional Requirements

| FR | One-line reasoning |
|----|-------------------|
| **Add money to wallet** | Bank → wallet, entry point of money into system |
| **P2P transfer** | User → user, core use case |
| **Merchant payment** | User → merchant, real-time debit, batch settlement |
| **Refund** | *User-initiated* intentional reversal only — user ne request ki |
| **Transaction history** | Audit trail, user-facing ledger view |

> **[COMMON MISTAKE]** "Refund" aur "Reverse" mix mat karo. Refund = user ne manga. Reverse = system ne detect kiya aur automatically undo kiya. Interviewer notice karta hai.

---

## Non-Functional Requirements

| NFR | Value | Reasoning |
|-----|-------|-----------|
| **No double charge** | Idempotency on every operation | Client retries pe same payment dobara nahi honi chahiye |
| **No partial state** | Saga + compensating transactions | Debit hua, credit nahi — rollback complete hona chahiye |
| **Latency** | p99 < 500ms for *initiation only* | Settlement async hai — latency sirf PENDING return karne tak |
| **Availability** | 99.99% | 99.9% = 8.7 hrs downtime/year — payment system ke liye unacceptable |
| **Consistency over availability** | CP not AP | Hum CAP mein C choose karte hain — balance kabhi wrong nahi hona chahiye |
| **Audit** | Every rupee traceable | Regulatory requirement, double-entry ledger |
| **Bank timeout handling** | PENDING + PSP query | Explicitly state karo — yeh sabse tricky failure hai |

> **[KEY INSIGHT]** "Bank API timeouts gracefully handled" explicitly NFR mein bolna — interviewer ko pata chalta hai tum trickiest failure jaante ho.

---

# Section 3 — Numbers

## Scale Assumptions

```
DAU:              10M users
Transactions/day: 1M txns
```

## TPS Calculation

```
Avg TPS:
  1M txns/day ÷ 86,400 sec/day = ~12 TPS

Peak TPS (10x multiplier for festivals/sales):
  12 × 10 = ~120 TPS peak
```

## Storage Calculation

```
Per transaction: ~500 bytes
Per day:         1M × 500B = 500 MB/day
Per year:        500MB × 365 = ~180 GB/year
```

## Read QPS

```
Read:Write ratio = 10:1 (users check history frequently)
Read QPS = 120 × 10 = ~1,200 reads/sec
→ Handle with: Read replicas + Redis cache for transaction history
```

> **[KEY INSIGHT] The one line that separates SDE2 from SDE1:**
> "120 TPS pe PostgreSQL perfectly kaam karta hai. Cassandra ki zarurat nahi — Cassandra eventually consistent hai, payments mein yeh deadly hai. Do concurrent reads same balance dekh sakte hain, dono debit kar sakte hain, balance negative ho jaata hai."

> **[RED FLAG]** "I'll use Cassandra for scale" at 120 TPS = instant SDE1. Payments correctness problem hai, throughput problem nahi.

---

# Section 4 — Block Diagram

## Path 1: Real-Time Path (User-Facing)

```
User hits Pay
  ↓
API Gateway
  └── Auth, rate limiting, idempotency key check
  ↓
Payment Service
  ├── Check idempotency key → PostgreSQL (DB-stored, not Redis)
  ├── Validate balance → SELECT FOR UPDATE (row lock)
  └── Return PENDING to user immediately (<500ms) ← user gets response here
  ↓
Kafka
  └── Async from here — user already has response, server thread freed
  ↓
Ledger Service
  └── Write 2 ledger entries OPTIMISTICALLY (before bank call)
      DEBIT  sender    500
      CREDIT receiver  500
  ↓
Bank/UPI Adapter (PSP — Razorpay/PayU)
  ├── SUCCESS → mark txn SUCCESS, trigger notifications
  ├── TIMEOUT → stay PENDING, reconciliation job takes over
  └── FAILED  → Saga: compensating transactions, reverse ledger entries
  ↓
Notification Service
  ├── Push notification → User's phone (FCM/APNs) — "₹500 sent"
  ├── Webhook → Merchant's backend server (server-to-server)
  └── Polling → Available as fallback (GET /payment/status)
```

> **[KEY INSIGHT]** Teen cheezein alag hain: Push notification = server → user's phone. Webhook = server → merchant's backend. Polling = client pulls status. "Client" har context mein alag hota hai.

---

## Path 2: Batch Path (Merchant Payout — End of Day)

```
Cron Job triggers at 11:59 PM
  ↓
Payout Aggregator
  └── SELECT SUM(amount) FROM transactions
      WHERE receiver_type='MERCHANT'
      AND status='SUCCESS'
      AND settled=FALSE
      AND DATE(created_at) = TODAY
  ↓
Batch Transfer Service
  └── NEFT/RTGS → Merchant's bank account
  ↓
Mark all included transactions as SETTLED=TRUE
```

> **[KEY INSIGHT]** Merchant payout real-time nahi hota — end-of-day batch hai. Yeh explicitly bolna ki "merchant settlement alag path hai" shows architectural thinking.

---

## Path 3A: Real-Time Reconciliation (Every 5 Minutes — Ours)

```
Reconciliation Job (runs every 5 min — our system)
  ↓
SELECT * FROM transactions
WHERE status = 'PENDING'
AND created_at < now() - INTERVAL '5 minutes'
  ↓
For each PENDING transaction:
  Query PSP/Bank: GET /txn-status?ref={bank_ref_id}
  ↓
  Bank says SUCCESS → mark SUCCESS, complete ledger
  Bank says FAILED  → mark FAILED, run compensating transactions (REVERSE)
  Bank says UNKNOWN → retry in next cycle (max 12 retries over 1 hour)
```

> **[KEY INSIGHT]** Real-time reconciliation sirf PENDING transactions check karta hai — SUCCESS aur FAILED already resolved hain. Selective, not exhaustive.

---

## Path 3B: End-of-Day Reconciliation (Bank Runs It)

```
Bank generates daily settlement statement (midnight)
  ↓
Our system downloads statement CSV/API
  ↓
Transaction-by-transaction comparison:
  Our ledger TXN_001 ↔ Bank statement TXN_001
  ↓
  Match    → OK
  Mismatch → flag for investigation + auto-action
  ↓
Our DB=SUCCESS, Bank=FAILED  → REVERSE ledger entries, credit sender back
Our DB=FAILED,  Bank=SUCCESS → credit receiver, raise alert, investigate
Our DB=PENDING, Bank=SUCCESS → mark SUCCESS, complete ledger
```

> **[COMMON MISTAKE]** Do reconciliation hain — mix mat karo. Real-time = ours, selective (PENDING only). End-of-day = bank runs it, exhaustive (all transactions). Dono zaroori hain.

---

# Section 5 — Deep Dives

---

## Deep Dive 1: Double-Entry Bookkeeping

### Core Concept
**Every rupee movement = exactly 2 ledger entries. Sum of all entries for any transaction = zero.**

This is how every real bank in the world works. Non-negotiable foundation.

### Example

```
User A pays User B Rs.500:

ledger_entries (append-only — NEVER update or delete):

entry_id | txn_id | account_id | type   | amount | created_at
---------|--------|------------|--------|--------|------------
1        | TXN_1  | ACC_001    | DEBIT  | 500    | 2024-01-15
2        | TXN_1  | ACC_002    | CREDIT | 500    | 2024-01-15

Check: -500 + 500 = 0 ✓ (Zero sum game)
```

### Balance is Derived, Not Stored

```sql
-- Balance is NEVER stored directly
-- Always calculated from ledger entries:

SELECT SUM(CASE WHEN type='CREDIT' THEN amount ELSE -amount END)
FROM ledger_entries
WHERE account_id = 'ACC_001'

-- Why? If system crashes, balance column can go stale.
-- Ledger entries are append-only truth — always accurate.
```

### Why Delete is Never the Answer

```
Bank failed. Ledger has wrong entries.

WRONG approach:
  DELETE FROM ledger_entries WHERE txn_id = 'TXN_1' ❌
  → Audit trail destroyed
  → Regulatory violation
  → Can't reconstruct what happened

CORRECT approach — Contra Entry:
  Write NEW entries that nullify the old ones:
  entry_id | txn_id        | type   | amount | note
  3        | TXN_1_REVERSE | CREDIT | 500    | compensating — bank failed
  4        | TXN_1_REVERSE | DEBIT  | 500    | compensating — bank failed

  Net = -500 + 500 + 500 - 500 = 0 ✓
  Both original + reversal entries exist — full audit trail intact
```

### Paper World vs Real World

```
Real world (bank):    Money never actually moved → clean
Paper world (ledger): Entries were written → dirty

Reverse = clean the paper world to match the real world.
```

### Two Different Checks

| Check | What it verifies | When used |
|-------|-----------------|-----------|
| **Net sum = zero** | Ledger internally consistent — double entry correct | Accounting audit |
| **Transaction-by-transaction match** | Our status matches bank's status for each txn | End-of-day reconciliation |

> **[KEY INSIGHT]** Net sum zero = our ledger is internally consistent. Transaction match = our ledger matches the bank. These are two separate correctness guarantees.

---

## Deep Dive 2: Idempotency

### The Problem

```
User clicks Pay → request reaches server → server processes payment
→ response lost in network → client retries
→ server processes AGAIN → double charge ❌
```

### The Solution: Idempotency Key

```
Client generates UUID BEFORE hitting Pay:
  idempotency_key = "uuid-abc-123-def-456"

Client sends this key with EVERY retry of the same payment.

Server logic:
  1. Check DB: has this idempotency_key been seen before?
     YES → return stored response immediately (no processing)
     NO  → process payment → store result against key → return response
```

### Full UUID Flow

```
Request 1 (original):
  POST /payment/initiate
  Header: Idempotency-Key: uuid-abc-123
  → DB: key NOT found
  → Process payment → TXN_1 created
  → Store: {key: uuid-abc-123, status: SUCCESS, response: {txn_id: TXN_1}}
  → Return: {txn_id: TXN_1, status: PENDING}

Network drops. Client retries.

Request 2 (retry):
  POST /payment/initiate
  Header: Idempotency-Key: uuid-abc-123  ← SAME KEY
  → DB: key FOUND, status=SUCCESS
  → Return stored response immediately
  → No second payment. No processing.
```

### Why DB, Not Redis

```
Redis can evict keys under memory pressure (LRU eviction policy).

Timeline:
  Payment processed → key stored in Redis ✓
  Memory pressure → Redis evicts key ✗
  Client retries → key not found → treated as NEW payment → double charge ❌

PostgreSQL row is NEVER evicted.

Tradeoff: DB lookup ~1ms vs Redis ~0.1ms.
That 0.9ms is irrelevant compared to risk of double-charging Rs.50,000.
Correctness > performance for idempotency keys.
```

> **[COMMON MISTAKE]** "Cached result" is a misleading phrase. It is NOT Redis cache. It is a DB-stored response in the PostgreSQL idempotency_keys table.

### Key Design Rules

```
Scope:   user_id + idempotency_key (user-scoped, not globally unique)
Expiry:  24-48 hours (prevents infinite storage, enough for retry window)
Storage: PostgreSQL idempotency_keys table (never Redis)
```

### Two Different Problems

| Problem | Cause | Solution |
|---------|-------|----------|
| Same key retry | Network failure, client correctly retries | Idempotency key → return stored DB response |
| Different key (client bug) | Client generates new UUID per retry | Server-side cooldown: same user + merchant + amount within 1 min → reject |

---

## Deep Dive 3: Saga Pattern + 2PC

### Saga — Trip Booking Analogy

```
You book a trip:
  Step 1: Book flight ✓
  Step 2: Book hotel ✓
  Step 3: Book cab ✗ — unavailable!

You don't leave user with flight + hotel booked but no cab.
You undo in REVERSE order:
  Cancel hotel (Step 2 undo)
  Cancel flight (Step 1 undo)

This is Saga. Controlled cascade upstream.
```

### Full Payment Saga (4 Steps)

```
Step 1: Payment Service → Debit sender
        Publish "debited" event → Kafka
        COMPENSATING ACTION = Credit sender back

Step 2: Ledger Service → Write 2 ledger entries optimistically
        Publish "recorded" event → Kafka
        COMPENSATING ACTION = Write 2 reverse entries (contra entry)

Step 3: Bank Adapter → Call PSP/bank API
        Publish "bank_success" event → Kafka
        COMPENSATING ACTION = N/A (if bank failed, Step 2 + Step 1 compensate)

Step 4: Notification Service → Push notification + Webhook
        COMPENSATING ACTION = N/A

If Step 3 FAILS:
  Run Step 2 compensating: reverse ledger entries
  Run Step 1 compensating: credit sender back
  Transaction = as if it never happened
  Ledger has 4 entries total (2 original + 2 compensating) — net = zero
```

### Why Compensating = New Operation, Never Delete

```
"Galti mita nahi sakte, undo kar sakte ho naye operation se."

Like accounting:
  Wrong entry made → don't erase → write contra entry
  Wrong bank transfer → don't reverse → write reverse transfer
  Wrong ledger debit → don't delete → write credit entry

Same logic in Saga. Ledger append-only. History never destroyed.
```

### Choreography (No Coordinator)

```
Event-driven. No central coordinator.
Each service:
  1. Does its local transaction
  2. Publishes event to Kafka
  3. Next service picks up event independently

No service waits for another to confirm.
No central lock. No SPOF.
```

### 2PC — The Old Way

```
Two-Phase Commit (1970s monolithic era):

Phase 1 — Prepare:
  Coordinator asks ALL services: "Can you commit?"
  All services LOCK their resources
  All respond YES or NO

Phase 2 — Decision:
  All said YES → Coordinator sends COMMIT to all
  Any said NO  → Coordinator sends ROLLBACK to all
```

### 2PC Core Problem — Coordinator Crash

```
Phase 1 complete. All services said YES. All resources LOCKED.
Coordinator sends COMMIT to Service A ✓
Coordinator sends COMMIT to Service B ✓
Coordinator about to send COMMIT to Service C...
  ↓
COORDINATOR CRASHES
  ↓
Service C is locked, waiting.
Service C has no way to know: should I commit or rollback?
Only coordinator knew. Coordinator is dead.
Result: Service C locked FOREVER.
```

### Why Timeout-Based Rollback Also Fails

```
"Let's add timeout — if no response in 30s, rollback."

Timeline:
  Coordinator sent COMMIT to A ✓ (A committed)
  Coordinator sent COMMIT to B ✓ (B committed)
  Coordinator crashed before sending to C
  C waits 30s → timeout → C rollbacks

Result:
  Service A → COMMITTED
  Service B → COMMITTED
  Service C → ROLLED BACK
  → INCONSISTENT STATE ❌

Timeout saved us from being stuck forever.
But created inconsistency instead.
Both outcomes (stuck + inconsistent) are equally dangerous in payments.
```

### 2PC is a Blocking Protocol

```
Definition: A protocol where progress requires an external coordinator.
Without coordinator → no decision possible → system blocked.

This is fundamentally broken for distributed microservices across a network.
```

### Where 2PC is Still Valid

| Context | Use |
|---------|-----|
| Single DB, multiple tables | ✅ PostgreSQL uses 2PC internally — coordinator = DB itself, never crashes mid-transaction |
| Same process, same machine | ✅ Risk of coordinator crash near zero |
| Legacy banking mainframes | ✅ Monolithic, single system |
| Multiple microservices | ❌ Use Saga |

### Saga vs 2PC Comparison

| | Saga | 2PC |
|--|------|-----|
| Coordinator | None (event-driven) | Required (SPOF) |
| Locking | None | All services lock in Phase 1 |
| Coordinator crash | No impact | System stuck forever |
| Timeout rollback | Compensate cleanly | Inconsistent state |
| Protocol type | Non-blocking | Blocking |
| Latency | Async — fast | Sync — slow |
| Failure recovery | Compensating transactions | DB handles (single DB only) |
| Microservices | ✅ Perfect | ❌ Broken |
| Single DB | Overkill | ✅ Fine |
| Era | Modern | 1970s monolithic |

---

## Deep Dive 4: Gateway Timeout + PSP

### The Dual-Write Problem

```
Our system calls bank API.
Bank API takes 30s. Our timeout = 10s.
We timeout.

NOW WE DON'T KNOW:
  Possibility A: Bank processed it → money moved → we don't know
  Possibility B: Bank didn't process → money didn't move → we don't know

This is the dual-write problem.
Two systems. Two sources of truth. No coordinator.
```

### Why Blind Retry is Dangerous

```
If Possibility A (bank processed):
  We retry → bank processes AGAIN → double payment ❌

If Possibility B (bank didn't process):
  We retry → correct ✓

We don't know which possibility. 50% chance of double payment.
Payments mein yeh acceptable nahi.
```

### Our Role vs PSP Role

```
OUR ROLE:
  1. Send request to PSP (Razorpay/PayU) — one time
  2. Mark transaction PENDING
  3. Store bank_ref_id (our reference ID sent to bank)
  4. Wait for PSP's final answer
  5. DO NOT retry directly

PSP'S ROLE:
  1. Receive our request
  2. Call bank with our reference ID
  3. If bank times out → PSP retries with SAME reference ID
  4. Bank deduplicates using reference ID — same ID seen twice → ignore second
  5. PSP returns final SUCCESS/FAILED to us

PSP can safely retry. We cannot.
PSP bank ka trusted partner hai with deduplication agreement.
```

### Reconciliation Loop

```
Bank timeout → mark PENDING → store bank_ref_id
  ↓
Real-time reconciliation (every 5 min):
  SELECT * FROM transactions WHERE status='PENDING' AND created_at < now()-5min
  → Query PSP: GET /txn-status?ref={bank_ref_id}
  → SUCCESS  → mark SUCCESS, complete ledger
  → FAILED   → run Saga compensating transactions (REVERSE ledger)
  → UNKNOWN  → retry in next cycle (max 12 retries over 1 hour)
  ↓
After 12 retries still UNKNOWN → escalate to manual review
```

### PENDING is an Honest State

```
PENDING ≠ failure.
PENDING = "I don't know yet, and I have a process to find out."

Most developers treat PENDING as something to avoid.
In payments, PENDING is your best friend.
It means: something happened I can't confirm right now,
          reconciliation will resolve it.
Never lie about state. PENDING is honest.
```

---

## Deep Dive 5: Reconciliation (Two Types)

### Real-Time Reconciliation (Ours — Every 5 Minutes)

**Who runs it:** Our system (cron job)
**What it checks:** Only PENDING transactions

```sql
SELECT *
FROM transactions
WHERE status = 'PENDING'
AND created_at < NOW() - INTERVAL '5 minutes'
ORDER BY created_at ASC;
```

**What it does:**
```
For each PENDING transaction:
  Query PSP with bank_ref_id
  ↓
  PSP says SUCCESS →
    Mark transaction SUCCESS
    Complete ledger (if not already complete)
    Trigger notifications

  PSP says FAILED →
    Run Saga compensating transactions
    REVERSE ledger entries (contra entries)
    Credit sender back
    Mark transaction FAILED

  PSP says UNKNOWN →
    Leave as PENDING
    Retry in next 5-min cycle
    After 12 retries → escalate to manual
```

**Why only PENDING?** SUCCESS and FAILED are already resolved. Only PENDING = unknown state = needs resolution.

### End-of-Day Reconciliation (Bank Runs It)

**Who runs it:** Bank / PSP generates statement
**What it checks:** ALL transactions for the day — exhaustive

```
Bank generates daily settlement statement at midnight:
  TXN_001 | Rs.500  | SUCCESS
  TXN_002 | Rs.200  | SUCCESS
  TXN_003 | Rs.100  | FAILED
  ...all transactions for the day

Our system downloads and compares transaction-by-transaction:
  Our DB TXN_001 ↔ Bank TXN_001 → match ✓
  Our DB TXN_002 ↔ Bank TXN_002 → match ✓
  Our DB TXN_003 ↔ Bank TXN_003 → MISMATCH ✗
```

### All Mismatch Scenarios

| Our DB | Bank | Meaning | Correct Action |
|--------|------|---------|----------------|
| SUCCESS | FAILED | We thought it went through, bank says no | **REVERSE** ledger entries, credit sender back |
| FAILED | SUCCESS | Bank processed it, we didn't know | Credit receiver, update ledger, investigate |
| PENDING | SUCCESS | Timeout case not yet resolved | Mark SUCCESS, complete ledger |
| PENDING | FAILED | Timeout case, bank confirms failed | **REVERSE** ledger, credit sender, mark FAILED |

> **[COMMON MISTAKE]** "Our DB=SUCCESS, Bank=FAILED → refund karo." WRONG. Refund = user requested. This is system-detected mismatch → REVERSE. Bank never processed the money. Real world is clean. Ledger (paper world) needs to be cleaned.

### Two Different Checks Within End-of-Day

| Check | What it verifies |
|-------|-----------------|
| **Net sum = zero** | Our ledger is internally consistent — double-entry correct. Every debit has matching credit. |
| **Transaction-by-transaction match** | Our status matches bank's status for each individual transaction. |

These are two separate correctness checks. Both must pass. Net sum zero doesn't mean statuses match.

---

# Section 6 — DB + Schema

## Storage Decision Table

| Data | Storage | Why |
|------|---------|-----|
| **Accounts + balances** | PostgreSQL | ACID, SELECT FOR UPDATE, strong consistency |
| **Ledger entries** | PostgreSQL (append-only) | Same DB as accounts — 2 entries must be atomic (single transaction) |
| **Idempotency keys** | PostgreSQL | Redis evicts → key loss → double charge. Never Redis. |
| **Transactions (metadata)** | PostgreSQL | Status, timestamps, bank_ref_id, PENDING tracking |
| **Transaction history (read)** | Redis cache | Last 30 days, read-heavy, safe to cache (not balances) |
| **Event stream (Saga steps)** | Kafka | Decouples services, durable, replayable on crash |
| **Audit log** | S3 / cold storage | Immutable, cheap, regulatory retention 7+ years |

> **[RED FLAG]** "Cache wallet balance in Redis" = instant fail. Redis eviction = stale balance = free money bug. Balance is ALWAYS read from DB with SELECT FOR UPDATE during payment flow.

> **[KEY INSIGHT]** Read-heavy transaction history is safe to cache in Redis. Balance is not. The distinction: history is append-only (past data doesn't change), balance changes with every transaction.

---

## Schemas

### accounts table
```sql
CREATE TABLE accounts (
  account_id  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id     UUID        NOT NULL REFERENCES users(user_id),
  balance     BIGINT      NOT NULL DEFAULT 0,  -- stored in PAISE, not rupees
  currency    VARCHAR(3)  NOT NULL DEFAULT 'INR',
  created_at  TIMESTAMP   NOT NULL DEFAULT NOW(),
  updated_at  TIMESTAMP   NOT NULL DEFAULT NOW()
);
-- balance in PAISE: Rs.500 stored as 50000
-- NEVER float or decimal — floating point rounding errors compound at scale
-- 0.1 + 0.2 = 0.30000000000000004 in IEEE 754
```

### ledger_entries table
```sql
CREATE TABLE ledger_entries (
  entry_id    UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
  txn_id      UUID        NOT NULL REFERENCES transactions(txn_id),
  account_id  UUID        NOT NULL REFERENCES accounts(account_id),
  type        VARCHAR(6)  NOT NULL CHECK (type IN ('DEBIT', 'CREDIT')),
  amount      BIGINT      NOT NULL CHECK (amount > 0),  -- always positive
  created_at  TIMESTAMP   NOT NULL DEFAULT NOW()
  -- NO updated_at — this table is APPEND-ONLY
  -- NEVER UPDATE OR DELETE any row
);

CREATE INDEX idx_ledger_txn  ON ledger_entries(txn_id);
CREATE INDEX idx_ledger_acct ON ledger_entries(account_id, created_at);
```

### transactions table
```sql
CREATE TABLE transactions (
  txn_id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
  sender_id        UUID         NOT NULL REFERENCES accounts(account_id),
  receiver_id      UUID         NOT NULL REFERENCES accounts(account_id),
  amount           BIGINT       NOT NULL CHECK (amount > 0),
  status           VARCHAR(10)  NOT NULL CHECK (status IN (
                     'PENDING', 'SUCCESS', 'FAILED', 'REVERSED'
                   )),
  idempotency_key  VARCHAR(255) UNIQUE NOT NULL,
  bank_ref_id      VARCHAR(255),  -- our reference ID sent to PSP/bank
  settled          BOOLEAN      NOT NULL DEFAULT FALSE,  -- for merchant batch
  created_at       TIMESTAMP    NOT NULL DEFAULT NOW(),
  resolved_at      TIMESTAMP
);

CREATE INDEX idx_txn_status   ON transactions(status, created_at);
CREATE INDEX idx_txn_idem_key ON transactions(idempotency_key);
CREATE INDEX idx_txn_bank_ref ON transactions(bank_ref_id);
```

### idempotency_keys table
```sql
CREATE TABLE idempotency_keys (
  key               VARCHAR(255) PRIMARY KEY,  -- client-generated UUID
  user_id           UUID         NOT NULL,
  status            VARCHAR(15)  NOT NULL CHECK (status IN (
                      'PROCESSING', 'SUCCESS', 'FAILED'
                    )),
  response_payload  JSONB,       -- stored response returned to client on retry
  expires_at        TIMESTAMP    NOT NULL,  -- 24-48 hours from creation
  created_at        TIMESTAMP    NOT NULL DEFAULT NOW()
);
-- Stored in PostgreSQL — NEVER Redis
-- Redis eviction = key loss = double charge
```

---

## Why Paise, Not Rupees

```
WRONG:
  balance DECIMAL(10,2)  -- Rs.500.00

  0.1 + 0.2 in IEEE 754 floating point = 0.30000000000000004
  Over millions of transactions → rounding errors compound
  Rs.0.000001 error × 1M transactions = Rs.1 lost
  At scale: serious financial discrepancy

CORRECT:
  balance BIGINT  -- 50000 (paise)

  Integer arithmetic is exact. Always.
  Rs.500 = 50000 paise. No decimal. No rounding.
  Stripe stores in cents. Razorpay stores in paise. Every serious payment system does this.
```

## Why Balance is Derived, Not Stored

```
Option A — Store balance directly:
  UPDATE accounts SET balance = balance - 500 WHERE account_id = 'ACC_001'
  Problem: if this UPDATE succeeds but ledger INSERT fails → balance wrong
  Problem: balance column can go stale if crash mid-transaction

Option B — Derive balance from ledger (preferred):
  SELECT SUM(CASE WHEN type='CREDIT' THEN amount ELSE -amount END)
  FROM ledger_entries WHERE account_id = 'ACC_001'

  Ledger is the single source of truth.
  Balance can always be reconstructed by replaying ledger.
  Crash-safe — ledger entries are atomic writes.

Note: In practice, balance column IS stored for performance (index-friendly),
but it's always verified against ledger on payment — SELECT FOR UPDATE.
```

---

# Section 7 — APIs

## Core Payment APIs

### POST /v1/payment/initiate

```
POST /v1/payment/initiate
Headers:
  Authorization: Bearer {token}
  Idempotency-Key: {client-generated-uuid}

Body:
{
  "sender_account_id": "ACC_001",
  "receiver_account_id": "ACC_002",
  "amount": 50000,        ← in PAISE (Rs.500 = 50000)
  "currency": "INR",
  "note": "dinner split"
}

Response (always PENDING — never final status synchronously):
{
  "txn_id": "TXN_abc-123",
  "status": "PENDING",
  "message": "Payment initiated",
  "created_at": "2024-01-15T14:30:00Z"
}
```

**Why always PENDING?**
Bank API can take 5–30 seconds. We cannot block user for 30s — they'll tap again (double payment risk). We return PENDING immediately (<500ms), resolution happens async in background.

---

### GET /v1/payment/{txn_id}/status

```
GET /v1/payment/TXN_abc-123/status

Response:
{
  "txn_id": "TXN_abc-123",
  "status": "SUCCESS",       ← PENDING / SUCCESS / FAILED / REVERSED
  "amount": 50000,
  "sender": "User A",
  "receiver": "User B",
  "created_at": "2024-01-15T14:30:00Z",
  "resolved_at": "2024-01-15T14:30:05Z"
}
```

This is the polling endpoint — used as fallback when push notification or webhook is missed.

---

### POST /v1/payment/{txn_id}/refund

```
POST /v1/payment/TXN_abc-123/refund
← USER-INITIATED ONLY. System-detected mismatches use REVERSE (internal), not this API.

Body:
{
  "reason": "wrong_amount",
  "amount": 50000
}

Response:
{
  "refund_txn_id": "TXN_refund-xyz",
  "original_txn_id": "TXN_abc-123",
  "status": "PENDING"
}
```

> **[COMMON MISTAKE]** This endpoint = user-initiated refund only. System-detected reconciliation mismatch triggers internal REVERSE operation — no user API call involved.

---

## Status Delivery — Three Methods

| Method | Direction | "Client" in this context | Latency | Reliability | Use case |
|--------|-----------|--------------------------|---------|-------------|----------|
| **Push notification** | Server → User's phone | End user's mobile device | Near-instant (FCM/APNs) | FCM/APNs handle retry | User ko "₹500 sent" dikhao |
| **Webhook** | Server → Merchant's backend | Merchant's server (machine-to-machine) | Near-instant | Merchant server can be down — need retry | Merchant ko order confirm karo |
| **Polling** | Client → Server | Mobile app OR merchant backend | Up to poll interval | Always works | Fallback when above two missed |

> **[KEY INSIGHT]** Teeno alag hain. "Client" word har context mein different entity hai. Push = phone. Webhook = merchant server. Polling = whoever needs status as fallback.

Best practice: All three together. Push notification primary for users. Webhook primary for merchants. Polling as universal fallback.

---

## Webhook Payload

```json
POST https://merchant.com/webhooks/payment

Headers:
  X-Signature: hmac-sha256-of-payload
  Content-Type: application/json

Body:
{
  "event": "payment.success",
  "txn_id": "TXN_abc-123",
  "amount": 50000,
  "currency": "INR",
  "timestamp": "2024-01-15T14:30:05Z",
  "signature": "sha256=abc123def456..."
}
```

Merchant must verify HMAC signature before processing — prevents spoofed webhooks.

---

# Section 8 — Tradeoffs

## Tradeoff 1: PostgreSQL vs Cassandra

| | PostgreSQL | Cassandra |
|--|------------|-----------|
| Consistency | ✅ Strong ACID | ❌ Eventual (tunable) |
| Multi-row transactions | ✅ Native BEGIN/COMMIT | ❌ Single-row only |
| SELECT FOR UPDATE (row lock) | ✅ Built-in | ❌ Not supported |
| Two concurrent debits of same balance | ✅ Serialized by row lock | ❌ Both can read same balance → negative balance |
| At 120 TPS | ✅ Handles easily | ❌ Wrong choice — will cause negative balances |
| At 10M+ TPS | Could need sharding | ✅ Designed for this |
| **Verdict at payment scale** | **✅ Always pick this** | **❌ Never for payments at this scale** |

> **[KEY INSIGHT]** Cassandra is wrong not because of throughput — at 120 TPS Cassandra is overkill. It's wrong because of consistency. Two concurrent payments can both read the same "sufficient balance" and both succeed → negative balance. This is not fixable without application-level locking that defeats the purpose.

---

## Tradeoff 2: Saga vs 2PC

| | Saga (Choreography) | 2PC |
|--|---------------------|-----|
| Coordinator | None — event-driven | Required — SPOF |
| Locking | None | All services lock in Phase 1 |
| Coordinator crash | No impact | System stuck forever |
| Timeout-based rollback | Compensate cleanly | Creates inconsistent state (A+B committed, C rolled back) |
| Protocol type | Non-blocking | Blocking |
| Latency | Async — fast | Sync — slow |
| Failure recovery | Compensating transactions (explicit) | DB handles automatically (single DB only) |
| Complexity | Must write compensating logic | DB handles rollback |
| Era | Modern microservices | 1970s monolithic |
| Works for microservices | ✅ | ❌ |
| Works for single DB | Overkill | ✅ |
| **Verdict** | **✅ Use for payment system** | **❌ Only inside single DB** |

---

## Tradeoff 3: Sync vs Async Response

| | Synchronous | Async (Recommended) |
|--|-------------|---------------------|
| User waits | Up to 30s for bank API | Returns PENDING in <500ms |
| Bank timeout handling | Client times out → retries → double charge risk | No timeout — client already has response |
| Server thread | Blocked for 30s | Freed immediately — handles other requests |
| UX | ❌ 30s spinner | ✅ Instant "Processing..." feedback |
| Swiggy analogy | User waits at restaurant while food is being cooked | User gets "Order placed" instantly, food comes later |
| **Verdict** | **❌ Never in production** | **✅ Always async** |

> **[KEY INSIGHT]** Async ≠ user doesn't wait. User sees "Processing..." and waits. Async = server thread is not blocked. The thread is freed to handle other requests while bank processes in background.

---

## Tradeoff 4: Push Notification vs Webhook vs Polling

| | Push Notification | Webhook | Polling |
|--|-------------------|---------|---------|
| Direction | Server → User's phone | Server → Merchant backend | Client → Server |
| "Client" | End user's phone | Merchant's server | Mobile app or merchant |
| Latency | Near-instant | Near-instant | Up to poll interval |
| Reliability | FCM/APNs handle retry | Merchant server can be down | Always works |
| Who benefits | End user | Merchant | Anyone needing fallback |
| Load on our server | Low (push) | Low (push) | Higher (repeated GETs) |
| **Best for** | End user experience | Merchant order confirmation | Universal fallback |
| **Verdict** | Primary for users | Primary for merchants | Fallback for both |

Best architecture: all three together.

---

# Section 9 — Failures

## Failure 1: Bank API Timeout

**What happens:** Payment Service calls PSP. PSP/bank takes 30s. Our timeout = 10s. We timeout. We don't know if bank processed it.

**Detection:** Timeout exception in Bank Adapter service.

**Wrong approach:** Retry immediately → if bank processed, double payment. 50% chance of catastrophic error.

**Correct approach:**
```
1. Mark transaction PENDING in DB
2. Store bank_ref_id (our reference sent to PSP)
3. DO NOT retry directly
4. Return PENDING to any status query
5. Real-time reconciliation job (every 5 min) queries PSP:
   GET /txn-status?ref={bank_ref_id}
6. PSP responds:
   SUCCESS  → mark SUCCESS, complete ledger, notify
   FAILED   → run Saga compensating transactions, REVERSE ledger
   UNKNOWN  → retry in next cycle (max 12 times over 1 hour)
7. After 12 retries still unknown → escalate to manual review
```

> **[KEY INSIGHT]** PSP can retry safely because they send same reference ID and bank deduplicates. We cannot retry — we'd send a new request.

---

## Failure 2: Crash Mid-Saga

**What happens:** Debit event published to Kafka. Ledger Service crashes before writing entries. Two ledger entries never written. Balance debited from sender, but ledger incomplete.

**Detection:** Kafka consumer lag spike on ledger topic. Alert fires. Transaction stuck in PENDING.

**Wrong approach:** Without Kafka — event is lost on crash. Debit happened, ledger never written, money effectively disappeared from system records.

**Correct approach:**
```
Kafka retains message even after Ledger Service crash.
  ↓
Ledger Service restarts.
  ↓
Reads from last committed offset in Kafka.
  ↓
Processes the debit event.
  ↓
Idempotency key prevents double-processing if service
had partially processed before crash.
  ↓
Saga continues from where it stopped.
```

**Why Kafka is critical here:** Without Kafka, crash = lost event = silent data corruption. With Kafka, crash = temporary delay. System recovers to consistent state automatically.

---

## Failure 3: Duplicate Payment

**Case A — Same idempotency key (correct client behavior):**
```
User clicks Pay → request 1 sent
Network drops
User clicks Pay again → request 2 sent with SAME key

Server:
  Request 2: key found in DB → return stored response → NO second payment
```

**Case B — Different idempotency key (client bug):**
```
Client generates new UUID on each retry (bug).
Both requests treated as new payments → double charge ❌

Server-side defense:
  Cooldown window check:
  Same user + same merchant + same amount within 60 seconds
  → Reject second request with 409 Conflict
  → "Duplicate payment detected, please wait"
```

Two different problems, two different solutions. Idempotency key handles Case A. Server-side cooldown handles Case B.

---

## Failure 4: Reconciliation Mismatch

**What happens:** End-of-day reconciliation finds Our DB=SUCCESS but Bank=FAILED.

**What it means:** We wrote ledger entries optimistically. Bank never actually processed the payment. Real world clean, paper world (ledger) dirty.

**Wrong action:** "Refund karo" — WRONG. Refund = user requested. This is system-detected.

**Correct action:**
```
REVERSE the ledger entries (system-initiated, automatic):
  Write compensating entries:
  CREDIT ACC_001  500  ← sender ko wapas (contra entry)
  DEBIT  ACC_002  500  ← receiver se wapas (contra entry)

Mark transaction status = REVERSED
Raise internal alert for investigation
Log in audit trail with reason: "reconciliation_mismatch"
```

> **[COMMON MISTAKE]** Always say "REVERSE" for system-detected mismatches. "Refund" is only when user explicitly requests money back through the refund API.

---

# Section 10 — Q&A

### Q1: Balance Redis mein cache kyun nahi karte?

Redis LRU eviction policy ke under keys evict ho jaati hain memory pressure pe. Agar wallet balance Redis mein cached hai aur evict ho gayi, agle read pe stale data aayega. Do concurrent payments dono Rs.1000 balance dekhenge, dono Rs.800 debit karenge, user Rs.-600 pe aa jaayega. Balance hamesha DB se SELECT FOR UPDATE ke saath padhna chahiye — row lock acquire hota hai, concurrent reads serialized ho jaate hain.

### Q2: Idempotency key DB mein kyun, Redis mein kyun nahi?

Redis LRU eviction = key loss = double charge. Timeline: payment processed → key Redis mein store hua → memory pressure → Redis ne key evict kar di → client retry kiya → key nahi mili → server ne naya payment treat kiya → double charge. PostgreSQL row kabhi evict nahi hoti. 0.9ms extra latency (DB vs Redis) payments mein irrelevant hai — correctness pehle, performance baad mein. "Cached result" phrase misleading hai — yeh Redis cache nahi, DB-stored response hai.

### Q3: Compensating transaction kaise kaam karta hai?

Ek naya operation likhte hain jo pichle ko nullify kare — ledger se delete nahi karte. DEBIT tha toh CREDIT likho same amount ka. Ledger append-only hai — history kabhi destroy nahi hoti, sirf nullify hoti hai. Jaise accounting mein contra entry hoti hai, bank mein reverse transfer hota hai. Original entry + compensating entry dono exist karti hain — net = zero. Audit trail intact rehta hai. "Galti mita nahi sakte, undo kar sakte ho naye operation se."

### Q4: 2PC mein timeout-based rollback kyun kaam nahi karta?

Coordinator ne Service A aur B ko COMMIT bhej diya — dono ne commit kar liya. Coordinator Service C ko COMMIT bhejna tha, crash ho gaya. C ne 30s baad timeout dekha, rollback kar liya. Result: A committed, B committed, C rolled back — inconsistent state. Timeout ne "stuck forever" se bachaya lekin inconsistency create kar di. Dono equally dangerous hain payments mein. Yahi reason hai Saga use karte hain — koi coordinator nahi, koi point of no return nahi.

### Q5: Bank timeout pe retry kyun nahi karte?

Bank timeout pe do possibilities hain: bank ne process kiya ya nahi kiya. Hume pata nahi. Retry kiya toh agar bank ne process kiya tha — double payment. Sahi approach: PENDING mark karo, bank_ref_id store karo, PSP se status query karo. PSP safely retry karta hai same reference ID se — bank deduplicates. Hum direct retry nahi karte. "Blind retry in payments = gambling with user's money."

### Q6: Refund aur reverse mein kya fark hai?

Refund = user ne explicitly request ki — "mujhe paisa wapas chahiye." User-initiated, intentional. POST /payment/{id}/refund API se trigger hota hai. Reverse = system ne automatically detect kiya — reconciliation mismatch, Saga failure, bank ne process nahi kiya. System-initiated, automatic. Internal Saga compensating transaction se trigger hota hai — koi user API call nahi. Interview mein galat word use karna shows you haven't thought about the distinction.

### Q7: Push notification, webhook aur polling mein kya fark hai?

Push notification: Server → User ka phone (FCM/APNs). Client = end user's mobile device. Webhook: Server → Merchant ka backend (server-to-server, machine-to-machine). Client = merchant's server. Polling: Client → Server (client pulls status). Client = whoever needs status — mobile app ya merchant backend. Teeno alag purposes, teeno saath use karo.

### Q8: Money float mein kyun nahi store karte?

IEEE 754 floating point mein 0.1 + 0.2 = 0.30000000000000004. Ek transaction mein yeh negligible hai. Millions of transactions mein compound hota hai — serious financial discrepancy. Solution: paise (smallest unit) mein store karo as BIGINT. Rs.500 = 50000 paise. Integer arithmetic exact hai — no rounding ever. Stripe cents mein, Razorpay paise mein store karta hai. Every serious payment system does this.

### Q9: PSP safely retry kyun kar sakta hai, hum kyun nahi?

PSP (Razorpay/PayU) bank ka trusted partner hai with deduplication agreement. PSP har request ek unique reference ID ke saath bank ko bhejta hai. Agar PSP retry kare same reference ID ke saath, bank us ID ko pehchanta hai — duplicate ko ignore karta hai. Hum bank ko direct retry karte toh hum ek nayi request bhejte (new reference ID) — bank ko pata nahi yeh duplicate hai — double processing. Humara kaam: PSP ko ek baar request do, PENDING rakho, PSP ka final answer wait karo.

### Q10: Do reconciliation mein kya fark hai — real-time aur end-of-day?

Real-time reconciliation: ours, every 5 min, sirf PENDING transactions, SQL query se PENDING fetch karo, PSP se status poocho, resolve karo. Selective. End-of-day reconciliation: bank runs it, raat ko, poore din ki saari transactions, bank statement se transaction-by-transaction comparison, mismatch detect karo. Exhaustive. Real-time = PENDING resolve karta hai. End-of-day = everything double-check karta hai — even jo real-time mein resolve hua. Dono zaroori hain — ek speed ke liye, ek completeness ke liye.

---

# Section 11 — Interview Speak

### Concept 1: Why PostgreSQL, Not Cassandra

```
KEYWORDS: ACID, SELECT FOR UPDATE, row lock, concurrent reads,
          negative balance, correctness problem not throughput problem
AVOID: "Cassandra doesn't scale" (wrong), "PostgreSQL is better"
       (too vague), "eventual consistency is fine" (deadly in payments)

INTERVIEW ANSWER:
"At 120 peak TPS, this is a correctness problem, not a throughput
problem. I'd use PostgreSQL — it gives me ACID transactions and
SELECT FOR UPDATE for row-level locking. If I used Cassandra,
two concurrent payments could both read the same 'sufficient balance'
before either write completes — both succeed, balance goes negative.
PostgreSQL's row lock serializes those reads. Cassandra would be
relevant at millions of TPS, but even then I'd build application-level
consistency on top — which defeats the purpose."
```

### Concept 2: Why Idempotency Key in DB, Not Redis

```
KEYWORDS: LRU eviction, key loss, double charge, DB-stored response,
          correctness over performance
AVOID: "cached result" (misleading), "Redis is slower" (wrong reason),
       "Redis doesn't work" (too vague)

INTERVIEW ANSWER:
"Idempotency keys must live in PostgreSQL, not Redis. Redis uses LRU
eviction — under memory pressure, it evicts keys. If a payment's
idempotency key gets evicted and the client retries, the server treats
it as a new payment — double charge. A DB row is never evicted.
Yes, DB lookup adds ~1ms vs Redis ~0.1ms. That tradeoff is irrelevant
when the alternative is double-charging a user. This is DB-stored
response retrieval, not caching."
```

### Concept 3: How Saga Works and Why Not 2PC

```
KEYWORDS: compensating transactions, choreography, no coordinator,
          no distributed lock, blocking protocol, point of no return
AVOID: "2PC is slow" (incomplete), "Saga is better" (too vague),
       "2PC doesn't work" (not specific enough)

INTERVIEW ANSWER:
"I'd use Saga over 2PC for distributed transactions. 2PC requires a
coordinator — if it crashes after sending COMMIT to some services but
not all, those services stay locked forever. Even timeout-based rollback
fails — services that already committed can't undo, creating inconsistent
state. Saga has no coordinator. Each service does its local transaction,
publishes an event, and the next service picks up. On failure, compensating
transactions run in reverse order — each one writes a new nullifying
operation, never deletes. The tradeoff is I must write compensating logic
explicitly. But no distributed lock and no single point of failure is
worth it."
```

### Concept 4: How You Handle Bank API Timeout

```
KEYWORDS: dual-write problem, PENDING state, bank_ref_id, PSP query,
          no blind retry, reconciliation loop
AVOID: "retry immediately" (dangerous), "mark as failed" (premature),
       "ignore it" (money lost)

INTERVIEW ANSWER:
"Bank timeout creates the dual-write problem — we don't know if the bank
processed it or not. Retrying immediately risks double payment if the bank
did process it. The correct approach: mark transaction PENDING, store our
bank_ref_id, and let the reconciliation job query PSP every 5 minutes
using that reference ID. PSP can safely retry using the same reference ID
because banks deduplicate on it — we cannot retry directly. PENDING is an
honest state — it means 'I don't know yet, and I have a process to find out.'
This resolves within 1 hour maximum."
```

### Concept 5: What Happens if Service Crashes Mid-Transaction

```
KEYWORDS: Kafka offset, at-least-once delivery, idempotency prevents
          double-processing, PENDING state, Saga recovery
AVOID: "data is lost" (wrong with Kafka), "transaction fails" (incomplete),
       "we retry" (vague)

INTERVIEW ANSWER:
"If a service crashes mid-Saga, Kafka saves us. Events are persisted in
Kafka — when the service restarts, it reads from its last committed offset
and picks up where it left off. Idempotency keys prevent double-processing
if the service had partially completed before crashing. The transaction
stays PENDING until Saga completes. If the crash happens after bank call
but before ledger write — reconciliation detects the PENDING state and
completes the ledger. Without Kafka, a crash mid-event would mean the
event is lost permanently — silent data corruption."
```

### Concept 6: How Reconciliation Works — Both Types

```
KEYWORDS: PENDING transactions, bank_ref_id query, transaction-by-transaction,
          net sum zero, exhaustive vs selective, bank runs end-of-day
AVOID: "we do all reconciliation" (bank does end-of-day),
       "reconciliation is one thing" (two separate processes),
       "refund on mismatch" (it's reverse)

INTERVIEW ANSWER:
"There are two separate reconciliation processes. Real-time reconciliation
runs every 5 minutes — it's ours, it queries only PENDING transactions,
fetches status from PSP using bank_ref_id, and resolves them. Selective,
not exhaustive. End-of-day reconciliation is run by the bank — they generate
a full statement, we compare transaction-by-transaction against our ledger.
This catches everything real-time missed. On mismatch where our DB says
SUCCESS but bank says FAILED, we reverse the ledger entries — it's
system-detected, not user-requested, so it's a reverse not a refund."
```

### Concept 7: Why Async Response Pattern

```
KEYWORDS: synchronous initiation, asynchronous resolution, server thread freed,
          PENDING returned immediately, Swiggy analogy
AVOID: "user doesn't wait" (wrong — user sees Processing...),
       "async means faster" (incomplete), "we don't respond" (wrong)

INTERVIEW ANSWER:
"The async response pattern means synchronous initiation with asynchronous
resolution. We respond immediately with PENDING in under 500ms — that's the
synchronous part. Bank call, ledger write, and notification happen in the
background — that's the async part. The user sees 'Processing...' and waits,
but our server thread is freed to handle other requests. Think Swiggy: 'Order
placed' comes instantly, food delivery is async. If we waited synchronously
for the bank, a 30-second timeout would cause users to tap again — exactly
the double-payment scenario we're trying to prevent."
```

### Concept 8: Why Ledger Written Optimistically Before Bank Call

```
KEYWORDS: optimistic write, audit trail from start, PENDING state,
          compensating transaction on bank failure, two valid approaches
AVOID: "ledger after bank confirmation" (user would wait 30s),
       "only one correct approach" (two valid approaches exist)

INTERVIEW ANSWER:
"I write ledger entries optimistically before the bank call. The alternative —
waiting for bank confirmation first — means either the user waits 30 seconds
for the bank, or we have a window where bank processed but ledger wasn't
written due to a crash. Optimistic write gives us an audit trail from the
very start of the transaction. If the bank fails, Saga runs compensating
transactions — new contra entries in the ledger that nullify the originals.
PENDING + reconciliation is the safety net for both approaches. I prefer
optimistic write because the audit trail exists throughout."
```

### Concept 9: Refund vs Reverse — When to Use Which

```
KEYWORDS: user-initiated, system-detected, intentional, automatic,
          POST /refund API, Saga compensating transaction
AVOID: "refund" for system mismatches (wrong), "reverse" for user requests
       (wrong), treating them as synonyms (interviewer notices)

INTERVIEW ANSWER:
"Refund and reverse are two distinct operations and I'd never use them
interchangeably. Refund is user-initiated — the user explicitly requests
their money back through POST /payment/{id}/refund. It's intentional.
Reverse is system-detected and automatic — when reconciliation finds a
mismatch, or when Saga detects bank failure, the system writes compensating
ledger entries without any user request. Using 'refund' for a system-detected
mismatch is technically wrong — the bank never processed the money, so
there's nothing to 'refund'. We're just cleaning the paper world to match
the real world."
```

### Concept 10: How Status is Delivered to User vs Merchant

```
KEYWORDS: push notification, FCM/APNs, webhook, HMAC signature,
          polling fallback, server-to-server, machine-to-machine
AVOID: "we just send a notification" (too vague),
       "webhook to user" (wrong — webhook is for merchants),
       "only one method" (three methods exist)

INTERVIEW ANSWER:
"Status delivery uses three separate mechanisms for different audiences.
For the end user, we send a push notification via FCM or APNs — server
to their phone. For merchants, we use webhooks — a server-to-server POST
to their registered URL with HMAC signature for authenticity verification.
For anyone who misses either, polling is the fallback — GET /payment/status.
These are distinct because the audience is different: user has a phone,
merchant has a backend server. I'd implement all three — push notification
and webhook as primary, polling as universal fallback."
```

---

# Section 12 — Rubric + Red Flags

## SDE2 Rubric — 8 Dimensions

| Dimension | SDE1 Bar (Fail) | SDE2 Bar (Pass) |
|-----------|----------------|-----------------|
| **Requirements** | No clarifications, jumps to design | Asks: P2P? merchants? refunds? international? scale? Explicitly states bank timeout as NFR. 99.99% not 99.9%. |
| **Numbers** | None or wrong DB choice justification | 12 avg TPS, 120 peak. One clear line: why PostgreSQL not Cassandra at this scale. |
| **Consistency model** | "Eventual consistency is fine" | ACID required. SELECT FOR UPDATE explained. Balance never in Redis. |
| **Idempotency** | Not mentioned at all | UUID key, DB not Redis with eviction reasoning, 24-48hr expiry, correct scope |
| **Distributed transaction** | "Use 2PC" or nothing | Saga with compensating transactions. Knows 2PC coordinator SPOF AND timeout inconsistency problem. |
| **Gateway timeout** | "Retry immediately" | Mark PENDING, PSP query via bank_ref_id, reconciliation loop. Never blind retry. PSP vs us distinction. |
| **Status delivery** | "Send a notification" | All three: push notification (user phone), webhook (merchant backend), polling (fallback). Knows who "client" is in each. |
| **Failures + Tradeoffs** | 0-1 failure, no tradeoffs | 4 failures with detection + correct approach. PostgreSQL vs Cassandra, Saga vs 2PC, sync vs async, all 3 notification methods. |

---

## Red Flags — Instant SDE1

| # | Red Flag | Why It Kills Your Score |
|---|----------|------------------------|
| 1 | **"Cache wallet balance in Redis"** | Redis eviction = stale balance = free money bug. Shows you don't understand consistency requirements. |
| 2 | **"Retry immediately on bank timeout"** | 50% chance of double payment. Shows you haven't thought about the dual-write problem. |
| 3 | **"Use Cassandra for payments at 120 TPS"** | Eventual consistency + concurrent debits = negative balances. Wrong tool for the problem. |
| 4 | **Not mentioning idempotency** | Shows you haven't thought about what happens when clients retry. Most common real-world payment bug. |
| 5 | **"Use 2PC" without knowing coordinator crash problem** | 2PC in microservices = system stuck forever on coordinator crash. |
| 6 | **"2PC with timeout rollback solves it"** | Shows you don't know timeout creates inconsistent state (A+B committed, C rolled back). |
| 7 | **Store money as float/decimal** | Rounding errors compound at scale. Every production payment system uses integers. |
| 8 | **No reconciliation mentioned** | Means you trust real-time handling is perfect. It never is. Reconciliation is non-negotiable. |
| 9 | **Saying "refund" for system-detected mismatches** | Refund = user-initiated. Reverse = system-detected. Conflating shows imprecise thinking. |
| 10 | **"Webhook to notify the user"** | Webhook = server-to-server, for merchants. Push notification = for users. Wrong audience. |
| 11 | **"Ledger written after bank confirms"** | User would wait 30s. Shows you haven't thought about async pattern. |
| 12 | **Only one status delivery method** | Missing push notification OR webhook OR polling shows incomplete design. |

---

## Interview Time Guide — 35 Minutes

| Phase | Time | What to cover |
|-------|------|---------------|
| **Requirements** | 0–3 min | P2P? merchants? refunds? international? scale? Bank timeout explicitly. 99.99%. Refund vs reverse distinction. |
| **Numbers** | 3–6 min | 12 avg TPS, 120 peak. Why PostgreSQL not Cassandra — one clear sentence. Storage/day. |
| **Block diagram** | 6–14 min | Draw all three paths: real-time, batch (merchant), reconciliation (both types). Label every component. |
| **Deep dive** | 14–22 min | Double-entry, idempotency, Saga vs 2PC, gateway timeout + PSP, reconciliation. Let interviewer guide which to go deep on. |
| **Failures** | 22–28 min | Bank timeout, crash mid-Saga, duplicate payment (2 cases), reconciliation mismatch. Detection + correct approach each. |
| **Tradeoffs** | 28–35 min | PostgreSQL vs Cassandra, Saga vs 2PC, sync vs async, push vs webhook vs polling. |

> **[KEY INSIGHT]** If interviewer asks "what would you do differently at 10x scale?" — answer: shard PostgreSQL by user_id, add read replicas, consider CockroachDB for distributed ACID. This shows you know PostgreSQL has limits — you just know 120 TPS isn't one of them.

---