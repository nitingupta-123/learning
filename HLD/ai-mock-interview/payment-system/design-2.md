# Payment System Design — Ultimate Revision Cheat Sheet
### FAANG SDE2 | Paytm / PhonePe / UPI + Amazon / E-commerce Scale

---

## PRIORITY READING ORDER

**BEFORE INTERVIEW (must know):**
→ Section 1 — Mental Models
→ Section 2 — Requirements
→ Section 3 — Numbers
→ Section 4 — Architecture
→ Section 5 — Deep Dives
→ Section 9 — Failures
→ Section 11 — Interview Speak
→ Section 12 — Rubric + Red Flags

**GOOD TO KNOW (read if time permits):**
→ Section 6 — DB + Schema
→ Section 7 — APIs
→ Section 8 — Tradeoffs

**REFERENCE ONLY (open only when confused):**
→ Section 10 — Q&A (use only for self-quiz)
→ Section 13 — BBG mapping (academic only, zero direct interview value)

---

# Section 1 — Mental Models

## Mental Model 1: Functional Requirements
### "Paisa kahan se aaya, kahan gaya, kya hua agar nahi gaya?"

| Question | Answer | FR |
|----------|--------|----|
| Kahan se aaya? | Bank → Wallet | **Add money (Pay-in)** |
| Kahan gaya? (friend) | Wallet → Friend | **P2P transfer** |
| Kahan gaya? (shop) | Wallet → Merchant | **Merchant payment** |
| Kahan gaya? (seller) | Platform → Seller bank | **Pay-out (batch)** |
| Nahi gaya — user ne manga | User request | **Refund** (intentional) |
| Nahi gaya — system ne detect kiya | Auto-undo | **Reverse** (automatic) |
| Trace karo | History | **Transaction history** |

---

## Mental Model 2: Non-Functional Requirements
### "Pehle, Dauran, Baad" Triangle

**PEHLE (Before):**
- Idempotency — duplicate hai kya?
- Balance check — SELECT FOR UPDATE
- AML/CFT risk check — compliance pass?
- PCI DSS — card data secure?

**DAURAN (During):**
- Async + PENDING — user ko < 500ms mein jawab
- 99.99% availability
- Ledger optimistically write karo

**BAAD (After):**
- Saga compensating transactions — crash pe rollback
- Real-time reconciliation — har 5 min PENDING resolve
- End-of-day reconciliation — bank settlement file match
- Audit — har rupee traceable

---

## Mental Model 3: Numbers
### "Kitne log, kitni baar, kitna bhaari"

| | Value |
|--|-------|
| Kitne log? | 10M DAU |
| Kitni baar? | 1M txns/day |
| Avg speed? | ~12 TPS |
| Peak speed? | ~120 TPS |
| Kitna bhaari? | ~500 MB/day |

**[KEY INSIGHT]** 120 TPS = correctness problem, not throughput problem. PostgreSQL. Never Cassandra.

---

# Section 2 — Requirements

## Functional Requirements

| FR | One-line reasoning |
|----|-------------------|
| **Add money (Pay-in)** | Buyer → PSP → platform bank account |
| **P2P transfer** | User → user, core wallet use case |
| **Merchant payment** | Real-time debit, batch settlement end-of-day |
| **Pay-out** | Platform bank → seller bank via third-party (Tipalti) |
| **Refund** | User-initiated ONLY — POST /refund API |
| **Transaction history** | Audit trail, user-facing |

**[COMMON MISTAKE]** Refund ≠ Reverse. Refund = user ne manga. Reverse = system ne detect kiya. Never use them interchangeably.

---

## Non-Functional Requirements

| NFR | Value | If wrong → |
|-----|-------|-----------|
| **No double charge** | Idempotency key (DB, not Redis) | Double payment |
| **No partial state** | Saga + compensating transactions | Money lost in system |
| **Latency** | p99 < 500ms for initiation only | Users tap twice |
| **Availability** | 99.99% (not 99.9%) | 8.7 hrs downtime/year |
| **Consistency > Availability** | CP not AP | Negative balances |
| **Audit** | Every rupee traceable | Regulatory violation |
| **Bank timeout handling** | PENDING + PSP query | Blind retry = double payment |
| **AML/CFT compliance** | Third-party risk check before processing | Fraud, legal liability |
| **PCI DSS** | Never store raw card numbers | Massive fine + breach |

---

# Section 3 — Numbers

```
DAU:                  10M users
Transactions/day:     1M txns

Avg TPS:
  1M ÷ 86,400 = ~12 TPS

Peak TPS (10x — festivals, sales):
  12 × 10 = ~120 TPS

Storage/day:
  1M × 500 bytes = ~500 MB/day
  × 365 = ~180 GB/year

Read QPS:
  Read:Write = 10:1
  120 × 10 = ~1,200 reads/sec
  → Read replicas + Redis for txn history (NOT balances)
```

**PostgreSQL at 120 TPS:** Handles easily. ACID + SELECT FOR UPDATE = correctness guaranteed.
**Switch to CockroachDB:** When you need distributed ACID across regions at 10,000+ TPS. Not before.
**Never Cassandra:** Eventual consistency + concurrent debits = negative balances. Wrong tool entirely.

---

# Section 4 — Architecture

## Path 1: Real-Time Pay-In (User-Facing)

```
User hits Pay
  ↓
API Gateway
  └── Auth, rate limiting, idempotency key check
  ↓
Payment Service
  ├── AML/CFT risk check (third-party) → fail = reject
  ├── Check idempotency key → PostgreSQL (NOT Redis)
  ├── Validate balance → SELECT FOR UPDATE (row lock)
  └── Return PENDING to user immediately (<500ms) ← user gets response HERE
  ↓
Kafka (async from here — user already has PENDING response)
  ↓
Payment Executor (separate from Payment Service — BBG pattern)
  └── Two integration options:
      Option A (API): Call PSP directly
      Option B (Hosted page — recommended):
        1. Register payment with PSP → receive REGISTRATION TOKEN
        2. Client displays PSP hosted page (Stripe/PayPal)
        3. User enters card on PSP page (PCI DSS — we never see card)
        4. PSP processes → returns PAYMENT TOKEN
        5. PSP calls our webhook with result
  ↓
Ledger Service
  └── Write 2 entries OPTIMISTICALLY (before bank confirms)
      DEBIT  sender_account   500  ← paper world updated
      CREDIT receiver_account 500
  ↓
Bank/UPI Adapter (PSP — Razorpay/PayU/Stripe)
  ├── SUCCESS → mark txn SUCCESS, store payment_token as bank_ref_id
  ├── TIMEOUT → stay PENDING, reconciliation takes over
  └── FAILED  → Saga: compensating transactions, REVERSE ledger entries
  ↓
Notification Service
  ├── Push notification → User's phone (FCM/APNs) — "₹500 sent"
  ├── Webhook → Merchant's backend (server-to-server, HMAC signed)
  └── Polling → GET /payment/status (fallback)
```

**[KEY INSIGHT]** Payment Service = orchestrates + risk check. Payment Executor = calls PSP. Separation allows independent scaling and cleaner failure isolation.

---

## Path 2: Pay-Out / Batch Path (Merchant Settlement)

```
Cron Job (11:59 PM daily)
  ↓
Payout Aggregator
  └── SELECT SUM(amount) FROM transactions
      WHERE receiver_type='MERCHANT'
      AND status='SUCCESS'
      AND settled=FALSE
      AND DATE(created_at) = TODAY
  ↓
Third-Party Payout Provider (Tipalti / Razorpay Payout)
  └── NEFT/RTGS/SWIFT → Merchant's bank account
  ↓
Mark transactions SETTLED=TRUE
  ↓
Wallet Service updated (merchant balance zeroed for day)
```

---

## Path 3A: Real-Time Reconciliation (Ours — Every 5 Min)

```
Our cron job (every 5 min)
  ↓
SELECT * FROM transactions
WHERE status = 'PENDING'
AND created_at < NOW() - INTERVAL '5 minutes'
  ↓
For each: Query PSP → GET /txn-status?ref={bank_ref_id}
  ↓
PSP SUCCESS  → mark SUCCESS, complete ledger, notify
PSP FAILED   → Saga compensating transactions → REVERSE ledger
PSP UNKNOWN  → retry next cycle (max 12 over 1 hour)
  ↓
After 12 retries → Dead Letter Queue → finance team manual review
```

---

## Path 3B: End-of-Day Reconciliation (Bank Runs It)

```
Bank sends settlement file (midnight)
  ↓
Our system downloads + parses
  ↓
Transaction-by-transaction comparison vs our ledger
  ↓
Three resolution tiers:
  Tier 1 (classifiable + automatable) → auto-fix program runs
  Tier 2 (classifiable, not automatable) → job queue → finance team
  Tier 3 (unclassifiable) → special queue → manual investigation

Mismatch scenarios:
  Our DB=SUCCESS, Bank=FAILED  → REVERSE ledger, credit sender back
  Our DB=FAILED,  Bank=SUCCESS → credit receiver, investigate
  Our DB=PENDING, Bank=SUCCESS → mark SUCCESS, complete ledger
```

---

# Section 5 — Deep Dives

---

## DD1: Double-Entry Bookkeeping

**Analogy:** Ledger = diary (truth). Wallet = calculator (derived). Diary is always right.
**If wrong:** Crash mid-transaction → balance column stale → money appears/disappears.

```
User A pays User B Rs.500:

DEBIT  ACC_001  500  ← sender
CREDIT ACC_002  500  ← receiver
Sum = -500 + 500 = 0 ✓ (zero sum game)

NEVER delete. NEVER update.
Wrong entry? Write CONTRA ENTRY (new nullifying operation).

entry_id | txn_id        | type   | amount | note
1        | TXN_1         | DEBIT  | 500    | original
2        | TXN_1         | CREDIT | 500    | original
3        | TXN_1_REVERSE | CREDIT | 500    | compensating
4        | TXN_1_REVERSE | DEBIT  | 500    | compensating
Net = 0 ✓ Audit trail intact.
```

**Two different checks:**
| Check | What it verifies |
|-------|-----------------|
| Net sum = zero | Ledger internally consistent (accounting audit) |
| Transaction-by-transaction | Our status matches bank's status (reconciliation) |

**Paper world vs real world:**
- Real world (bank): money never moved → clean
- Paper world (ledger): entries written → dirty
- Reverse = clean the paper world

---

## DD2: Idempotency

**Analogy:** Hotel receptionist — "Already checked you in, here's your existing key."
**If wrong:** Network drop → client retries → double charge → user pays twice.

```
Client generates UUID BEFORE hitting Pay.
Sends SAME UUID on every retry.

Request 1: key NOT in DB → process → store response → return
Request 2 (retry): key FOUND in DB → return stored response → NO processing

Why DB not Redis:
  Redis LRU eviction → key lost → retry treated as new → double charge ❌
  PostgreSQL row never evicted ✓

Also called: nonce (BBG), deduplication ID (PSP docs), idempotency key (Stripe/PayPal)

Key rules:
  Scope: user_id + key (not global)
  Expiry: 24-48 hours
  Storage: PostgreSQL ONLY

Two different problems:
  Same key retry → idempotency handles it
  Different key (client bug) → server cooldown: same user+merchant+amount within 60s → reject
```

---

## DD3: Saga + 2PC

**Analogy:** Trip booking — flight + hotel + cab. Cab fails → cancel hotel → cancel flight. Controlled reverse cascade.
**If wrong (2PC):** Coordinator crashes → all services locked forever → system dead.

```
Payment Saga — 4 steps:

Step 1: Payment Service → debit sender
        COMPENSATING = credit sender back

Step 2: Ledger Service → write 2 entries optimistically
        COMPENSATING = write 2 contra entries (never delete)

Step 3: Bank Adapter → call PSP
        COMPENSATING = N/A (Steps 1+2 compensate)

Step 4: Notification Service → push + webhook
        COMPENSATING = N/A

Step 3 fails:
  Run Step 2 compensating → Run Step 1 compensating
  Net = zero. As if never happened.
```

**Choreography:** No coordinator. Each service → local txn → publish event → next service picks up.

**2PC failure:**
```
Phase 1: All services lock resources, say YES
Phase 2: Coordinator sends COMMIT to A ✓, B ✓, crashes before C
         C waits → timeout → C rollbacks
Result: A committed, B committed, C rolled back = INCONSISTENT STATE ❌
Timeout doesn't fix 2PC — it creates inconsistency instead of deadlock.
```

**2PC valid only:** Inside single DB (PostgreSQL uses it internally). Never across microservices.
**2PC era:** 1970s monolithic. Pre-internet, pre-distributed systems.

| | Saga | 2PC |
|--|------|-----|
| Coordinator | None | Required (SPOF) |
| Locking | None | All services Phase 1 |
| Crash recovery | Compensate | Stuck forever |
| Timeout | Compensate cleanly | Inconsistent state |
| Microservices | ✅ | ❌ |
| Single DB | Overkill | ✅ |

---

## DD4: Gateway Timeout + PSP + Retry

**Analogy:** You sent a letter — post office says "we don't know if it arrived." Resending may deliver twice.
**If wrong:** Blind retry → double payment → user charged twice.

```
Dual-write problem:
  We call PSP → 30s, our timeout = 10s → we timeout
  Possibility A: PSP processed → money moved
  Possibility B: PSP didn't process → money didn't move
  WE DON'T KNOW. Never retry blindly.

Our role: send once → mark PENDING → store bank_ref_id → wait
PSP role: retry safely with same reference ID → bank deduplicates

Two tokens from PSP:
  Registration token: PSP registers payment → short-lived → used by hosted page
  Payment token: PSP executes payment → permanent → stored as bank_ref_id → used for refunds/reconciliation

Retry strategy (exponential backoff):
  Immediate → 1s → 2s → 4s → 8s → cancel
  Include Retry-After header
  Retryable: network errors, timeouts
  Non-retryable: invalid card, insufficient funds

Retry queue → max retries exceeded → Dead Letter Queue (DLQ) → finance team
```

---

## DD5: Reconciliation

**Analogy:** Real-time = checking your pocket every 5 min. End-of-day = comparing full passbook with bank statement at night.
**If wrong:** Silent money discrepancies accumulate → audit failure → regulatory action.

**Real-time (Ours — every 5 min):**
```sql
SELECT * FROM transactions
WHERE status = 'PENDING'
AND created_at < NOW() - INTERVAL '5 minutes';
-- Only PENDING — SUCCESS and FAILED already resolved
```
Then query PSP for each → resolve.

**End-of-day (Bank runs it — "settlement file" in BBG):**
Transaction-by-transaction comparison. All transactions. Exhaustive.

**Three mismatch resolution tiers:**
| Tier | Condition | Action |
|------|-----------|--------|
| 1 | Classifiable + automatable | Auto-fix program |
| 2 | Classifiable, not automatable | Job queue → finance team |
| 3 | Unclassifiable | Special queue → manual investigation |

**Mismatch actions (REVERSE not refund):**
| Our DB | Bank | Action |
|--------|------|--------|
| SUCCESS | FAILED | REVERSE ledger, credit sender |
| FAILED | SUCCESS | Credit receiver, investigate |
| PENDING | SUCCESS | Mark SUCCESS, complete ledger |
| PENDING | FAILED | REVERSE ledger, credit sender |

---

## DD6: Hosted Payment Page + PSP Integration

**Analogy:** You don't build your own ATM — you use the bank's.
**If wrong:** Store card numbers yourself → PCI DSS violation → massive fine + breach liability.

```
Why hosted page:
  PCI DSS (Payment Card Industry Data Security Standard)
  Storing card numbers requires expensive annual certification
  Most companies use PSP hosted page instead — PSP handles compliance

Full token flow:
  1. Our server: POST /register-payment → PSP
  2. PSP returns REGISTRATION TOKEN (nonce) — short-lived
  3. Client displays PSP hosted page (Stripe/PayPal) with token
  4. User enters card directly on PSP page — we NEVER see card number
  5. PSP processes → returns PAYMENT TOKEN — permanent
  6. PSP calls our webhook with result + payment token
  7. We store payment token as bank_ref_id

Two PSP integration models:
  Model 1 (API): We collect card info, send to PSP via API
    → Requires PCI DSS certification ❌ expensive
  Model 2 (Hosted page): PSP collects card info on their page
    → No PCI DSS for us ✅ recommended

AML/CFT check placement:
  Before payment reaches PSP
  Payment Service → AML/CFT third-party check → pass → Payment Executor → PSP
```

---

## DD7: Payment Security

| Threat | Solution |
|--------|---------|
| Request/response eavesdropping | HTTPS everywhere |
| Data tampering | Encryption + integrity monitoring |
| Man-in-the-middle | SSL with certificate pinning |
| Data loss | DB replication across multiple regions |
| DDoS | Rate limiting + WAF (Web Application Firewall) |
| Card theft | Tokenization — store token not card number |
| PCI compliance | PSP hosted page — never store raw card data |
| Fraud | AVS (address verification), CVV check, behavior analysis |
| Money laundering | AML/CFT third-party check before processing |
| Webhook spoofing | HMAC signature verification on every webhook |

---

# Section 6 — DB + Schema

## Storage Decisions

| Data | Where | Why |
|------|-------|-----|
| Accounts + balances | PostgreSQL | ACID, SELECT FOR UPDATE |
| Ledger entries | PostgreSQL append-only | Atomic 2-entry writes with accounts |
| Idempotency keys | PostgreSQL | Redis evicts → double charge |
| Transactions (metadata) | PostgreSQL | Status, bank_ref_id, PENDING tracking |
| Txn history (read) | Redis cache | Safe — past data doesn't change |
| Saga events | Kafka | Durable, replayable on crash |
| Audit log | S3 cold storage | 7+ year regulatory retention |

**[RED FLAG]** Balance in Redis = free money bug. Always DB + SELECT FOR UPDATE.

---

## Amount Storage

| Layer | Format | Why |
|-------|--------|-----|
| DB storage | BIGINT (paise) | Integer arithmetic exact — no rounding |
| API transmission | String | Serialization precision across systems (BBG recommendation) |

Best practice: store BIGINT in DB, transmit as string in API payloads.
**If wrong:** 0.1 + 0.2 = 0.30000000000000004 (IEEE 754) × 1M transactions = serious discrepancy.

---

## Schemas

```sql
-- accounts
CREATE TABLE accounts (
  account_id  UUID     PRIMARY KEY,
  user_id     UUID     NOT NULL,
  balance     BIGINT   NOT NULL DEFAULT 0,  -- PAISE, not rupees
  currency    VARCHAR(3) DEFAULT 'INR',
  created_at  TIMESTAMP DEFAULT NOW()
);

-- ledger_entries (APPEND-ONLY — NEVER UPDATE OR DELETE)
CREATE TABLE ledger_entries (
  entry_id    UUID     PRIMARY KEY,
  txn_id      UUID     NOT NULL,
  account_id  UUID     NOT NULL,
  type        VARCHAR(6) CHECK (type IN ('DEBIT','CREDIT')),
  amount      BIGINT   CHECK (amount > 0),  -- always positive
  created_at  TIMESTAMP DEFAULT NOW()
  -- NO updated_at — append only
);
CREATE INDEX idx_ledger_txn  ON ledger_entries(txn_id);
CREATE INDEX idx_ledger_acct ON ledger_entries(account_id, created_at);

-- transactions
CREATE TABLE transactions (
  txn_id           UUID        PRIMARY KEY,
  sender_id        UUID        NOT NULL,
  receiver_id      UUID        NOT NULL,
  amount           BIGINT      NOT NULL,
  status           VARCHAR(10) CHECK (status IN ('PENDING','SUCCESS','FAILED','REVERSED')),
  idempotency_key  VARCHAR(255) UNIQUE NOT NULL,
  bank_ref_id      VARCHAR(255),  -- payment token from PSP
  settled          BOOLEAN     DEFAULT FALSE,
  created_at       TIMESTAMP   DEFAULT NOW(),
  resolved_at      TIMESTAMP
);
CREATE INDEX idx_txn_pending  ON transactions(status, created_at);
CREATE INDEX idx_txn_bank_ref ON transactions(bank_ref_id);

-- idempotency_keys (NEVER Redis)
CREATE TABLE idempotency_keys (
  key               VARCHAR(255) PRIMARY KEY,
  user_id           UUID        NOT NULL,
  status            VARCHAR(15) CHECK (status IN ('PROCESSING','SUCCESS','FAILED')),
  response_payload  JSONB,
  expires_at        TIMESTAMP   NOT NULL,  -- 24-48 hours
  created_at        TIMESTAMP   DEFAULT NOW()
);
```

## Wallet: Table vs Service

| Scale | Approach |
|-------|---------|
| Simple (< 1M users) | balance column in accounts table |
| Complex (BBG pattern) | Separate Wallet Service with own DB — tracks merchant running balance |

## Replication Options

| Option | When |
|--------|------|
| All reads + writes on primary | Safe, less scalable, good up to ~10K TPS |
| CockroachDB / YugabyteDB | Distributed ACID across regions, 10K+ TPS, global scale |

---

# Section 7 — APIs

## Payment Initiation

```
POST /v1/payment/initiate     (our style)
POST /v1/payments             (BBG style — same concept)

Headers:
  Authorization: Bearer {token}
  Idempotency-Key: {client-uuid}

Body:
{
  "sender_account_id": "ACC_001",
  "receiver_account_id": "ACC_002",
  "amount": "50000",          ← STRING in API (BIGINT in DB)
  "currency": "INR",
  "note": "dinner"
}

Response (always PENDING):
{
  "txn_id": "TXN_abc",
  "status": "PENDING",
  "created_at": "2024-01-15T14:30:00Z"
}
```

## Status + Refund

```
GET /v1/payment/{txn_id}/status
→ { txn_id, status, amount, sender, receiver, resolved_at }

POST /v1/payment/{txn_id}/refund   ← USER-INITIATED ONLY
Body: { reason, amount }
→ { refund_txn_id, status: "PENDING" }
```

## Status Delivery — Three Methods

| Method | Direction | "Client" here | Use case |
|--------|-----------|---------------|----------|
| **Push notification** | Server → phone | End user's device | "₹500 sent" to user |
| **Webhook** | Server → backend | Merchant's server | Order confirmation to merchant |
| **Polling** | Client → server | App or merchant | Fallback when above missed |

Best practice: all three. Push + webhook primary. Polling fallback.

## Webhook

```json
POST https://merchant.com/webhooks/payment
Headers: X-Signature: hmac-sha256={payload-hash}

{
  "event": "payment.success",
  "txn_id": "TXN_abc",
  "amount": "50000",
  "payment_token": "psp_token_xyz",
  "timestamp": "2024-01-15T14:30:05Z"
}
```
Merchant MUST verify HMAC before processing.

---

# Section 8 — Tradeoffs

## PostgreSQL vs Cassandra vs CockroachDB

| | PostgreSQL | Cassandra | CockroachDB |
|--|------------|-----------|-------------|
| Consistency | ✅ Strong ACID | ❌ Eventual | ✅ Distributed ACID |
| SELECT FOR UPDATE | ✅ | ❌ | ✅ |
| At 120 TPS | ✅ Perfect | ❌ Wrong tool | Overkill |
| At 10K+ TPS global | Needs sharding | ❌ Wrong | ✅ |
| Negative balance risk | None | ❌ High | None |
| **Verdict** | **Default choice** | **Never for payments** | **At massive scale** |

---

## Saga vs 2PC

| | Saga | 2PC |
|--|------|-----|
| Coordinator | None | Required (SPOF) |
| Locking | None | All services Phase 1 |
| Crash | Compensate | Stuck forever |
| Timeout rollback | Clean | Inconsistent state |
| Era | Modern | 1970s monolithic |
| Microservices | ✅ | ❌ |
| Single DB | Overkill | ✅ |

---

## Sync vs Async Communication

| | Sync (HTTP) | Async (Kafka) |
|--|-------------|---------------|
| Coupling | Tight | Loose |
| Failure isolation | Poor — cascades | Good |
| Scalability | Hard | Easy |
| Multiple consumers | No | ✅ (payment + analytics + billing) |
| Complexity | Simple | Higher |
| **Verdict** | Small scale only | ✅ Production payments |

---

## Push Notification vs Webhook vs Polling

| | Push | Webhook | Polling |
|--|------|---------|---------|
| Direction | Server → phone | Server → merchant | Client → server |
| Audience | End user | Merchant backend | Anyone (fallback) |
| Latency | Instant | Instant | Poll interval |
| If receiver down | FCM retries | Message lost (need retry) | Always works |

---

## API Integration vs Hosted Payment Page

| | API Integration | Hosted Page (recommended) |
|--|-----------------|--------------------------|
| Card data | We collect + send | PSP collects — we never see |
| PCI DSS | Full certification required | PSP handles it |
| Flexibility | High | Medium |
| Security risk | High | Low |
| **Verdict** | Only if PCI certified | ✅ Default |

---

## BIGINT vs String for Amount

| | BIGINT (paise) | String |
|--|----------------|--------|
| DB storage | ✅ Exact integer math | ❌ Needs parsing |
| API transmission | ❌ Precision loss risk | ✅ Safe across systems |
| **Verdict** | ✅ Use in DB | ✅ Use in API |

---

## Wallet Table vs Wallet Service

| | Wallet as table | Wallet Service |
|--|-----------------|----------------|
| Scale | Simple systems | Complex/large scale |
| Separation | account.balance column | Separate service + DB |
| BBG recommendation | — | ✅ Separate service |
| Our recommendation | ✅ Start here | Migrate when needed |

---

# Section 9 — Failures

## Failure 1: Bank API Timeout

| | |
|--|--|
| What happens | PSP takes 30s, our timeout = 10s. Did PSP process it? Unknown. |
| Detection | Timeout exception in Payment Executor |
| Wrong approach | Retry immediately → double payment if PSP processed |
| Correct approach | Mark PENDING, store bank_ref_id, reconciliation queries PSP every 5 min. PSP retries safely — we don't. |

---

## Failure 2: Crash Mid-Saga

| | |
|--|--|
| What happens | Debit event in Kafka, Ledger Service crashes before writing entries |
| Detection | Kafka consumer lag spike, transaction stuck PENDING |
| Wrong approach | Without Kafka — event lost = silent data corruption |
| Correct approach | Kafka retains message. Service restarts, reads from last offset, continues Saga. Idempotency prevents double-processing. |

---

## Failure 3: Duplicate Payment

**Case A — Same idempotency key (correct client retry):**
Key found in DB → return stored response → no second payment.

**Case B — Different key (client bug):**
Server cooldown: same user + merchant + amount within 60s → reject with 409.

---

## Failure 4: Reconciliation Mismatch

| | |
|--|--|
| What happens | Our DB=SUCCESS, Bank=FAILED |
| Meaning | Ledger written optimistically, bank never processed. Paper world dirty, real world clean. |
| Wrong action | "Refund karo" — WRONG. User didn't request anything. |
| Correct action | REVERSE ledger entries (contra entries), credit sender back, mark REVERSED, raise alert. |

---

## Failure 5: Retry Exhausted → DLQ

| | |
|--|--|
| What happens | 12 retries over 1 hour, PSP still UNKNOWN |
| Detection | Retry count threshold exceeded |
| Wrong approach | Keep retrying indefinitely |
| Correct approach | Move to Dead Letter Queue. Finance team investigates manually. Three resolution tiers. |

---

## Failure 6: AML/CFT Rejection

| | |
|--|--|
| What happens | Third-party compliance check flags transaction (money laundering pattern) |
| Detection | AML/CFT service returns REJECT |
| Wrong approach | Process anyway |
| Correct approach | Reject payment immediately. Log for compliance audit. User sees "Payment declined." No ledger entries written. |

---

# Section 10 — Q&A

**Use this section for self-quiz only. Cover answer, read question, speak answer aloud.**

---

**Q1: Balance Redis mein cache kyun nahi karte?**
Redis LRU eviction → stale balance → two concurrent payments both see "sufficient balance" → both succeed → negative balance. SELECT FOR UPDATE on PostgreSQL serializes reads. Correctness > 0.9ms latency saving.

---

**Q2: Idempotency key DB mein kyun, Redis mein kyun nahi?**
Redis evicts under memory pressure → key gone → retry treated as new payment → double charge. PostgreSQL row never evicted. Also called nonce (BBG) or deduplication ID (PSP). Same concept, different name.

---

**Q3: Compensating transaction kaise kaam karta hai?**
New operation that nullifies the previous one. DEBIT hua → CREDIT karo same amount. Ledger append-only — never delete. Both original + compensating entries exist. Net = zero. Audit trail intact. "Galti mita nahi sakte, nullify kar sakte ho."

---

**Q4: 2PC timeout rollback kyun fail hota hai?**
Coordinator sent COMMIT to A ✓ and B ✓, crashes before C. C rollbacks after timeout. A committed, B committed, C rolled back = inconsistent state. Timeout prevents deadlock but creates inconsistency. Both equally dangerous.

---

**Q5: Bank timeout pe retry kyun nahi karte?**
Dual-write problem — we don't know if PSP processed. Retry may double-charge. PSP retries safely with same reference ID — bank deduplicates. Our role = PENDING + query status. Never blind retry.

---

**Q6: Refund vs reverse kya fark hai?**
Refund = user requested via POST /refund. Intentional. Reverse = system-detected (reconciliation mismatch, Saga failure). Automatic. Bank never processed — real world clean, paper world dirty. Reverse cleans paper world.

---

**Q7: Push vs webhook vs polling — client kaun hai each mein?**
Push notification → user's phone (FCM/APNs). Webhook → merchant's backend server (server-to-server). Polling → whoever polls (app or merchant, fallback). Three different audiences, three different mechanisms.

---

**Q8: Money float mein kyun nahi store karte?**
0.1 + 0.2 = 0.30000000000000004 (IEEE 754). Multiplied over 1M transactions = serious discrepancy. BIGINT paise in DB = exact integer arithmetic. String in API = safe serialization. Stripe = cents, Razorpay = paise.

---

**Q9: PSP safely retry kyun kar sakta hai, hum kyun nahi?**
PSP sends same reference ID on retry. Bank deduplicates — same ID seen twice = ignore second. We can't retry directly — we'd send new request with new context = bank treats as new payment.

---

**Q10: Do reconciliation mein kya fark hai?**
Real-time (ours, every 5 min, PENDING only, selective). End-of-day (bank runs it, settlement file, all transactions, exhaustive). Real-time = speed. End-of-day = completeness. Both needed.

---

**Q11: Hosted payment page kyun — khud form kyun nahi banate?**
PCI DSS — storing card numbers requires expensive annual certification. PSP hosted page means PSP stores card data, we never see it. No certification needed. Flow: register → get registration token → display PSP page → PSP processes → sends us payment token via webhook.

---

**Q12: Retry queue aur DLQ mein kya fark hai?**
Retry queue = retryable failures (network error, timeout) get retried with exponential backoff. DLQ = max retries exceeded OR non-retryable failures → parked for manual investigation by finance team. DLQ prevents infinite retry loop.

---

# Section 11 — Interview Speak

---

```
CONCEPT: Why PostgreSQL not Cassandra
KEYWORDS: ACID, SELECT FOR UPDATE, row lock, correctness problem not throughput,
          concurrent reads, negative balance
AVOID: "Cassandra doesn't scale", "PostgreSQL is better", "eventual consistency is fine"

ANSWER: "At 120 peak TPS, this is a correctness problem, not a throughput problem.
PostgreSQL gives me ACID and SELECT FOR UPDATE — the row lock prevents two concurrent
payments from both reading 'sufficient balance' before either write completes.
With Cassandra's eventual consistency, both would succeed and the balance goes negative.
Cassandra would be relevant at millions of TPS, but even then I'd need application-level
consistency on top — which defeats the purpose."
```

---

```
CONCEPT: Why idempotency key in DB not Redis
KEYWORDS: LRU eviction, key loss, double charge, DB-stored response, correctness over performance
AVOID: "cached result", "Redis is slower", "Redis doesn't work"

ANSWER: "Idempotency keys live in PostgreSQL — never Redis. Redis uses LRU eviction;
under memory pressure it drops keys. If a payment's key is evicted and the client retries,
the server treats it as a new payment — double charge. A DB row is never evicted.
Yes, DB lookup adds ~1ms vs Redis ~0.1ms. That's irrelevant when the alternative is
double-charging a user. This is DB-stored response retrieval, not caching."
```

---

```
CONCEPT: How Saga works and why not 2PC
KEYWORDS: compensating transactions, choreography, no coordinator, no distributed lock,
          blocking protocol, point of no return
AVOID: "2PC is slow", "Saga is better", "2PC doesn't work"

ANSWER: "I'd use Saga over 2PC. 2PC requires a coordinator — if it crashes after sending
COMMIT to some services but not all, those that didn't receive it rollback after timeout,
creating inconsistent state. A+B committed, C rolled back — payments are in limbo.
Saga has no coordinator. Each service does its local transaction, publishes an event,
next service picks up. On failure, compensating transactions run in reverse — each writes
a new nullifying operation, never deletes. Tradeoff: I must write compensating logic
explicitly. But no distributed lock and no single point of failure is worth it."
```

---

```
CONCEPT: How you handle bank API timeout
KEYWORDS: dual-write problem, PENDING, bank_ref_id, PSP query, no blind retry,
          exponential backoff, reconciliation loop
AVOID: "retry immediately", "mark as failed", "ignore it"

ANSWER: "Bank timeout creates the dual-write problem — we don't know if PSP processed it.
Retrying immediately risks double payment. Correct approach: mark PENDING, store bank_ref_id,
let reconciliation query PSP every 5 min using that reference ID. PSP can safely retry using
the same ID because banks deduplicate — we cannot retry directly.
PENDING is an honest state: 'I don't know yet, and I have a process to find out.'
Resolves within 1 hour maximum. If PSP is still unknown after 12 retries, it goes to DLQ."
```

---

```
CONCEPT: What happens if service crashes mid-transaction
KEYWORDS: Kafka offset, at-least-once, idempotency prevents double-processing,
          PENDING, Saga recovery
AVOID: "data is lost", "transaction fails", "we retry"

ANSWER: "If a service crashes mid-Saga, Kafka saves us. Events are persisted in Kafka —
when the service restarts, it reads from its last committed offset and picks up where
it stopped. Idempotency keys prevent double-processing if it had partially completed.
Transaction stays PENDING until Saga completes. Without Kafka, crash means lost event —
silent data corruption. With Kafka, crash means temporary delay and clean recovery."
```

---

```
CONCEPT: How reconciliation works — both types
KEYWORDS: PENDING only, bank_ref_id, settlement file, transaction-by-transaction,
          selective vs exhaustive, three resolution tiers
AVOID: "we do all reconciliation", "only end-of-day", "refund on mismatch"

ANSWER: "Two separate reconciliation processes. Real-time: ours, every 5 min, queries
only PENDING transactions via bank_ref_id, resolves them — selective not exhaustive.
End-of-day: bank generates settlement file, we compare transaction-by-transaction
against our ledger — exhaustive, catches everything real-time missed.
Mismatches have three resolution tiers: auto-fix, job queue for finance team,
and unclassifiable cases for manual investigation. Mismatch where we say SUCCESS
but bank says FAILED — we reverse the ledger, not refund. System-detected, not user-requested."
```

---

```
CONCEPT: Why async response pattern
KEYWORDS: synchronous initiation, asynchronous resolution, server thread freed,
          PENDING < 500ms, Swiggy analogy
AVOID: "user doesn't wait", "async means faster", "we don't respond"

ANSWER: "Synchronous initiation, asynchronous resolution. We return PENDING in under 500ms —
that's the synchronous part. Bank call, ledger write, notification happen in background —
async part. User sees 'Processing...' and waits, but our server thread is freed for other
requests. Like Swiggy: 'Order placed' is instant, food delivery is async.
If we waited synchronously for the bank, a 30s timeout causes users to tap again —
exactly the double-payment scenario we're preventing."
```

---

```
CONCEPT: Why ledger written optimistically before bank call
KEYWORDS: optimistic write, audit trail from start, compensating transaction on failure,
          PENDING safety net
AVOID: "ledger after bank confirmation only", "one correct approach"

ANSWER: "I write ledger entries optimistically before calling the bank.
The alternative — waiting for bank confirmation — means either the user waits 30 seconds,
or there's a crash window where bank processed but ledger wasn't written.
Optimistic write gives us an audit trail from the very start.
If bank fails, Saga runs compensating transactions — new contra entries nullify the originals.
PENDING plus reconciliation is the safety net for both approaches."
```

---

```
CONCEPT: Refund vs reverse
KEYWORDS: user-initiated, system-detected, intentional, automatic, POST /refund API,
          Saga compensating, paper world vs real world
AVOID: using "refund" for system mismatches, treating them as synonyms

ANSWER: "Refund and reverse are distinct — I'd never use them interchangeably.
Refund: user explicitly requests money back via POST /refund. Intentional.
Reverse: system detects mismatch — reconciliation finds our DB says SUCCESS but bank says FAILED.
Bank never processed the money. Real world is clean, paper world (ledger) is dirty.
Reverse cleans the paper world by writing compensating contra entries.
No user request involved. Using 'refund' for a system mismatch shows imprecise thinking."
```

---

```
CONCEPT: How status delivered to user vs merchant
KEYWORDS: push notification, FCM/APNs, webhook, HMAC, polling fallback,
          server-to-server, three separate mechanisms
AVOID: "we just send a notification", "webhook to user", "only one method"

ANSWER: "Three separate mechanisms for different audiences.
End user: push notification via FCM/APNs — server to their phone.
Merchant: webhook — server-to-server POST to their registered URL with HMAC signature.
Anyone who misses either: polling via GET /payment/status.
These are distinct because audience is different — user has a phone, merchant has a backend.
I implement all three: push and webhook as primary, polling as universal fallback."
```

---

```
CONCEPT: Pay-in vs Pay-out separation
KEYWORDS: pay-in flow, pay-out flow, PSP, third-party payout provider,
          platform bank account, settlement
AVOID: treating them as same flow

ANSWER: "Pay-in and pay-out are two completely separate flows.
Pay-in: buyer's credit card → PSP → platform's bank account. Real-time.
Pay-out: platform's bank account → seller's bank account, via third-party payout provider
like Tipalti. Batch, end-of-day. The platform holds money in its account as custodian —
sellers get paid when payout condition is met. Keeping them separate simplifies
failure handling — pay-in failure doesn't affect pay-out batch."
```

---

```
CONCEPT: Why hosted payment page / PCI DSS
KEYWORDS: PCI DSS, card number, tokenization, registration token, payment token,
          PSP hosted page, compliance
AVOID: "we build our own form", "PCI is optional", "we handle cards ourselves"

ANSWER: "PCI DSS — Payment Card Industry Data Security Standard — requires expensive
annual certification if you store raw card numbers. Most companies avoid this by using
PSP hosted pages: our server registers the payment, gets a registration token,
client displays PSP's page (Stripe Checkout, PayPal), user enters card directly on PSP's page —
we never see the card number. PSP processes and returns a payment token via webhook.
We store the payment token as bank_ref_id for reconciliation and refunds.
Building your own card form without PCI certification is a compliance violation."
```

---

# Section 12 — Rubric + Red Flags

## SDE2 Rubric — 11 Dimensions

| Dimension | SDE1 (Fail) | SDE2 (Pass) |
|-----------|-------------|-------------|
| Requirements | No clarifications | P2P? merchants? pay-out? AML? PCI? scale? bank timeout explicit. 99.99% not 99.9%. |
| Numbers | None | 12 avg TPS, 120 peak. PostgreSQL reasoning. CockroachDB threshold. |
| Consistency model | "Eventual is fine" | ACID, SELECT FOR UPDATE, balance never Redis |
| Idempotency | Not mentioned | UUID/nonce in DB not Redis, scope, expiry, two-problem distinction |
| Distributed txn | 2PC or nothing | Saga + compensating. Knows 2PC coordinator crash AND timeout inconsistency. |
| Gateway timeout | "Retry immediately" | PENDING, PSP query via bank_ref_id, reconciliation loop, PSP vs us distinction |
| Status delivery | "Send notification" | All three: push (user), webhook (merchant), polling (fallback). Knows who "client" is. |
| Failures | 0-1 mentioned | 6 failures with detection + correct approach each |
| PCI DSS | Not mentioned | Hosted payment page, why we don't store cards, registration vs payment token |
| Retry strategy | "Retry every X seconds" | Exponential backoff, retry queue, DLQ, finance team |
| DLQ | Not mentioned | Retry exhausted → DLQ → manual investigation |

---

## Red Flags — 15 Instant SDE1

| # | Red Flag | Why It Kills |
|---|----------|-------------|
| 1 | Cache wallet balance in Redis | Eviction = stale = free money |
| 2 | Retry immediately on timeout | Double payment |
| 3 | Cassandra for payments | Eventual consistency = negative balances |
| 4 | No idempotency | Haven't thought about retries |
| 5 | 2PC without knowing coordinator crash | 2PC = stuck forever |
| 6 | 2PC timeout rollback solves it | Creates inconsistent state instead |
| 7 | Store money as float | Rounding errors at scale |
| 8 | No reconciliation | Trusting real-time is perfect |
| 9 | "Refund" for system mismatches | Imprecise — shows you don't know the distinction |
| 10 | "Webhook to notify the user" | Webhook = merchant backend, not user |
| 11 | "Ledger after bank confirms" | User waits 30s, async pattern missed |
| 12 | Only one status delivery method | Missing push/webhook/polling distinction |
| 13 | "Build our own card form" | PCI DSS violation — use hosted page |
| 14 | Fixed interval retry | Exponential backoff is correct strategy |
| 15 | No DLQ mention | Retry loop never ends, finance team has no visibility |

---

## Interview Time Guide — 35 Minutes

| Phase | Time | Cover |
|-------|------|-------|
| Requirements | 0–3 min | P2P, merchants, pay-out, refund vs reverse, AML, PCI, bank timeout, 99.99% |
| Numbers | 3–6 min | 12/120 TPS, storage, PostgreSQL reasoning one-liner |
| Architecture | 6–14 min | All three paths: real-time pay-in, pay-out batch, reconciliation (both types) |
| Deep dive | 14–22 min | Double-entry, idempotency, Saga, timeout+PSP, reconciliation. Let interviewer guide. |
| Failures | 22–28 min | All 6 failures: timeout, crash, duplicate, mismatch, DLQ, AML |
| Tradeoffs | 28–35 min | PostgreSQL vs Cassandra, Saga vs 2PC, sync vs async, push vs webhook vs polling |

**If asked "what at 10x scale?":** Shard PostgreSQL by user_id + CockroachDB for distributed ACID + separate Wallet Service + regional PSP integrations.

---

# Section 13 — BBG vs Our Doc (Reference Only)

## Part A: Concept Name Mapping

| Our Term | BBG Term | Use in Interview | Notes |
|----------|----------|-----------------|-------|
| Idempotency key | Nonce / deduplication ID | Either acceptable | Same concept — Stripe = idempotency key, BBG = nonce |
| Payment Adapter / Bank Adapter | Payment Executor | BBG term more standard | BBG separates from Payment Service; simpler systems merge |
| End-of-day reconciliation | Settlement file | Either | BBG only covers this type — our doc adds real-time |
| Compensating transaction | Adjustment | Either | Same concept |
| Real-time reconciliation | (Not in BBG) | Our term | BBG doesn't cover this at all |
| Wallet balance column | Wallet Service | Depends on scale | BBG = separate service; start with column, migrate |
| PENDING transaction | In-flight payment | Either | Same concept |
| Payment token | bank_ref_id | Our term more precise | Permanent PSP reference, for reconciliation/refunds |
| Registration token | Nonce (PSP side) | BBG term | Short-lived, used by hosted page to identify payment |
| Amount as BIGINT | Amount as string | BIGINT in DB, string in API | Not contradictory — different layers |

---

## Part B: Where BBG is Wrong or Incomplete

**[BBG INCORRECT] Wallet updated before ledger:**
BBG page 2: "payment service updates wallet THEN calls ledger."
Why wrong: ledger = source of truth (append-only, crash-safe). Wallet = derived state. If wallet updates but ledger doesn't (crash) → wallet and ledger inconsistent → audit fails. Ledger must always be written first.

**[BBG INCOMPLETE] Only end-of-day reconciliation:**
BBG only covers "every night bank sends settlement file."
Missing entirely: real-time reconciliation every 5 min for PENDING transactions. This is critical for fast resolution of timeouts and crashes. Our doc adds this.

**[BBG INCOMPLETE] Only registration token mentioned:**
BBG covers nonce/registration token for hosted page flow.
Missing: payment execution token returned after PSP processes payment — this is the permanent reference stored as bank_ref_id, used for all future reconciliation, refunds, and dispute resolution.

**[BBG PARTIALLY CORRECT] Amount as string:**
BBG says store amount as string. Correct for API transmission (serialization safety).
Wrong for DB storage — BIGINT in paise is correct for exact arithmetic. Best practice: BIGINT in DB + string in API.

**[BBG INCOMPLETE] Reconciliation mismatch handling:**
BBG shows basic mismatch detection.
Missing: three resolution tiers (auto-fix, job queue → finance, unclassifiable → manual). Our doc adds complete picture.

---

*Document complete. All 13 sections. Readable months later with zero prior context.*
*Generated from: in-depth Paytm/PhonePe/UPI learning conversation + ByteByteGo Chapter 27.*
