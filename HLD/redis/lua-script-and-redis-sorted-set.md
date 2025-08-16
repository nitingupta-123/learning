# 🛡️ Rate Limiting: Race Condition Issue & Solution with Redis + Lua

## ⚠️ Problem: Race Condition in Rate Limiting

### What is a Race Condition?
A race condition occurs when **multiple concurrent requests** access and modify shared data (like a rate limit counter) **without synchronization**, potentially causing:

- Inconsistent behavior (e.g., more requests allowed than the limit)
- Double-counting or under-counting requests
- Breaches in rate limiting under high load

---

## 🎯 Goal

Implement a rate limiter that allows **N requests per user within T seconds** — without letting simultaneous requests break the limit.

---

## ✅ Solution: Redis Sorted Sets + Lua Scripting

### Why Redis Sorted Sets?
- Efficiently store **request timestamps** using scores.
- Automatically **sorted by time**.
- Supports fast range operations (e.g., remove expired entries, count current ones).

### Why Lua Scripting in Redis?
- **Atomic execution**: All commands inside a Lua script are executed together.
- Prevents race conditions by ensuring no other operations interfere mid-process.

---

## 🧪 Example: Limit 3 Requests per 10 Seconds

### Step-by-Step Flow:
1. **Key**: `ratelimit:{user_id}`
2. **Score**: Current timestamp (e.g., epoch time in seconds)
3. **Member**: Unique string (e.g., `timestamp-random`)
4. **On Each Request**:
   - Remove old entries: `ZREMRANGEBYSCORE key -inf (now - window)`
   - Count current entries: `ZCARD key`
   - If count < limit:
     - Add new entry: `ZADD key now "now-unique"`
     - Allow the request
   - Else:
     - Reject the request (rate limit exceeded)

---

## 🧾 Sample Lua Script

```lua
local key = KEYS[1]
local now = tonumber(ARGV[1])
local window = tonumber(ARGV[2])
local limit = tonumber(ARGV[3])
local expire_time = tonumber(ARGV[4])

-- Remove outdated requests
redis.call("ZREMRANGEBYSCORE", key, "-inf", now - window)

-- Count current requests
local count = redis.call("ZCARD", key)

if count >= limit then
  return 0 -- Block
else
  redis.call("ZADD", key, now, tostring(now) .. "-" .. math.random())
  redis.call("EXPIRE", key, expire_time)
  return 1 -- Allow
end



## 🥜 In a Nutshell: Redis Sorted Set Rate Limiting

- You maintain a **Redis Sorted Set** for each user or API key.

### Each request is stored in the set with:
- **Score**: Current timestamp (in seconds or milliseconds)
- **Value**: Unique ID (e.g., `timestamp + random string`)

---

### On each request:
1. **Remove old entries** that fall outside the time window.
2. **Count the remaining entries** in the set.
3. If the count is **less than the allowed limit**:
   - Add the current request to the set.
   - ✅ **Allow** the request.
4. If the count is **equal to or more than the limit**:
   - ❌ **Reject/drop** the request (rate limit hit).
