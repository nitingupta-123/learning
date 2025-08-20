

## 1. Handling Failures in Distributed Systems

- **Temporary Failures → Hinted Handoff**
- **Permanent / Long-term Failures → Anti-Entropy (with Merkle Trees)**

---

## 2. Hinted Handoff

### 🔹 Definition
A mechanism to handle **temporary replica failures**.  
If a replica is unavailable when a write happens, another replica temporarily stores a *hint* of the missed write.

### 🔹 How it works
1. Write request comes in.
2. One replica is down.
3. A healthy replica stores a *hint* for the downed replica.
4. When the failed replica recovers, hints are replayed.

### 🔹 Analogy
> "I’ll keep your homework safe until you come back tomorrow."

### 🔹 Pros & Cons
- ✅ Fast recovery from short outages.  
- ❌ Not suitable for long-term failures (hints may expire).

---

## 3. Anti-Entropy

### 🔹 Definition
A background process that ensures **eventual consistency** across replicas by comparing data and fixing mismatches.

### 🔹 How it works
- Uses **Merkle trees** to compare large datasets efficiently.
- If differences exist, only mismatched parts are transferred.

### 🔹 Analogy
> "Let’s sit down once in a while and compare all our notes to make sure we’re in sync."

### 🔹 Pros & Cons
- ✅ Ensures full consistency over time.  
- ❌ More expensive (CPU + I/O).  
- ✅ Runs in background (low-priority).

---

## 4. Merkle Trees

### 🔹 Definition
A **hash tree** where:
- Leaf nodes contain hashes of data buckets.
- Non-leaf nodes contain hashes of their children.
- The **root hash** summarizes the entire dataset.

### 🔹 Why Merkle Trees?
- Avoid comparing every key individually.
- Allow synchronization proportional to **differences**, not total data size.
- Reduce network usage.

---

## 5. Example: Building a Merkle Tree

### Step 1: Divide Key Space into Buckets
Keys: `1..12`  
Buckets (4 total):  
- Bucket 1 → Keys 1–3  
- Bucket 2 → Keys 4–6  
- Bucket 3 → Keys 7–9  
- Bucket 4 → Keys 10–12  

```
Buckets:
[1,2,3] | [4,5,6] | [7,8,9] | [10,11,12]
```

---

### Step 2: Hash Each Key
Example (simplified hash):
```
1 -> h(1) = 2343
2 -> h(2) = 6789
...
```

---

### Step 3: Compute Bucket Hash
```
Bucket1 = h(h(1), h(2), h(3))
Bucket2 = h(h(4), h(5), h(6))
...
```

---

### Step 4: Build Tree Upwards
```
             Root
           /      \
     Hash(1,2)   Hash(3,4)
     /     \       /     \
 Bucket1 Bucket2 Bucket3 Bucket4
```

---

### Step 5: Compare Two Servers
1. Compare **root hashes**.
   - If equal → data is same.
   - If not → go down the tree.
2. Compare child hashes → find mismatched buckets.
3. Sync only those buckets.

---

## 6. Overlapping Differences

Sometimes **differences span across buckets**.  
Example:
- Key 6 (in bucket 2) is different.
- Key 7 (in bucket 3) is also different.

```
Bucket2 [4,5,6] → mismatch
Bucket3 [7,8,9] → mismatch
```

👉 Both buckets are flagged. No problem.  
Each bucket is repaired independently.

---

## 7. Cost Considerations

- Hashing every **key-value pair** can be expensive.  
- To optimize:
  - Use **large buckets** (e.g., 1M buckets for 1B keys → 1000 keys per bucket).  
  - Only recalc when data changes.  
  - Incremental hashing strategies.

---

## 8. Example Python Simulation

```python
import hashlib

def h(val):
    return hashlib.md5(str(val).encode()).hexdigest()

# Build a simple Merkle tree for 12 keys, 4 buckets
def build_merkle(data, bucket_size=3):
    buckets = []
    for i in range(0, len(data), bucket_size):
        bucket_data = "".join(data[i:i+bucket_size])
        buckets.append(h(bucket_data))

    # Build tree upwards
    while len(buckets) > 1:
        new_level = []
        for i in range(0, len(buckets), 2):
            chunk = "".join(buckets[i:i+2])
            new_level.append(h(chunk))
        buckets = new_level
    return buckets[0]  # root hash

data = [f"val{i}" for i in range(1, 13)]
root_hash = build_merkle(data)
print("Merkle Root:", root_hash)
```

---

## 9. ASCII Diagram of Repair Flow

```
                ┌──────────────┐
                │ Client Write │
                └──────┬───────┘
                       │
          ┌────────────┴────────────┐
          │                         │
   Replica available         Replica down
          │                         │
          │                  Store HINTED HANDOFF
          │                         │
          │            Replica recovers → Replay hint
          │
   ──────────────────────────────────────────────
          │
   Periodic Background Check (ANTI-ENTROPY)
          │
   Compare Merkle Trees between replicas
          │
   If mismatch → Sync only mismatched buckets
```

---

## 10. Key Takeaways

- **Hinted Handoff** handles **short outages** quickly.  
- **Anti-Entropy (Merkle Trees)** ensures **long-term consistency**.  
- **Merkle Trees** make comparisons efficient by hashing buckets.  
- **Overlaps across buckets** are handled naturally.  
- Trade-off: **CPU hashing cost vs. reduced network cost**.

---
