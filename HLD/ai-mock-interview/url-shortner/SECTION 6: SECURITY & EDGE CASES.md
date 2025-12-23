# **SECTION 6: SECURITY & EDGE CASES** 🔒

---

## **6.1 SSRF Attack Prevention**

### 🤔 Interview Question/Context

**Interviewer:** "Users can submit any URL. What if someone submits `http://localhost:6379/config/set`? What security risks exist?"

### ❌ Your Initial Answer

You didn't answer this question when asked.

---

### ✅ Complete Answer

#### **What is SSRF (Server-Side Request Forgery)?**

**Definition:**
```
SSRF = Attacker tricks YOUR server into making requests
to internal services that shouldn't be accessible externally.

The attack:
1. Attacker submits malicious URL
2. Your server tries to validate/fetch it
3. Your server accidentally accesses internal systems
4. Attacker gains access to internal data
```

---

### 🎯 URL Shortener Attack Scenarios

#### **Attack 1: Access Internal Services**

```
Attacker submits:
  POST /api/v1/urls
  {
    "longUrl": "http://localhost:6379/CONFIG/SET"
  }

Your server (if vulnerable):
  1. Validates URL by fetching it
  2. Makes request to localhost:6379
  3. Connects to Redis (internal service!)
  4. Executes Redis CONFIG command
  5. Attacker can now reconfigure Redis ❌

Impact: Full compromise of internal Redis
```

---

#### **Attack 2: AWS Metadata Endpoint**

```
Attacker submits:
  "http://169.254.169.254/latest/meta-data/iam/security-credentials/"

Your server:
  1. Fetches this URL
  2. Gets AWS credentials for your EC2 instance
  3. Returns credentials in response (or logs them)
  
Attacker now has:
  - AWS Access Key
  - AWS Secret Key
  - Can access your entire AWS account ❌

Impact: Total AWS compromise, data breach, huge AWS bill
```

---

#### **Attack 3: Port Scanning Internal Network**

```
Attacker submits multiple URLs:
  "http://10.0.0.1:22"
  "http://10.0.0.1:80"
  "http://10.0.0.1:3306"
  "http://10.0.0.2:22"
  ...

Your server tries to fetch each one.

Responses reveal which ports are open:
  - Timeout = port closed or host down
  - Connection refused = host up, port closed
  - HTTP response = port open with HTTP service
  
Attacker maps your internal network ❌

Impact: Reconnaissance for further attacks
```

---

### ✅ Prevention Strategies

#### **Defense 1: Blacklist Internal IP Ranges (CRITICAL)**

```python
BLOCKED_IP_RANGES = [
    '127.0.0.0/8',       # Localhost
    '10.0.0.0/8',        # Private network
    '172.16.0.0/12',     # Private network
    '192.168.0.0/16',    # Private network
    '169.254.0.0/16',    # AWS metadata, link-local
    '::1/128',           # IPv6 localhost
    'fc00::/7',          # IPv6 private
]

def is_url_safe(url):
    # Parse URL
    parsed = urlparse(url)
    hostname = parsed.hostname
    
    # Resolve DNS to IP
    try:
        ip = socket.gethostbyname(hostname)
    except:
        return False, "Cannot resolve hostname"
    
    # Check if IP is in blocked ranges
    for blocked_range in BLOCKED_IP_RANGES:
        if ip_in_range(ip, blocked_range):
            return False, "Internal IP addresses not allowed"
    
    return True, "OK"
```

---

#### **Defense 2: DNS Rebinding Protection**

**The Attack:**
```
Attacker controls DNS for evil.com

First DNS lookup:
  evil.com → 1.2.3.4 (safe public IP)
  Your server: "OK, looks safe"
  
Attacker changes DNS:
  evil.com → 127.0.0.1 (localhost!)
  
Second lookup (when fetching):
  evil.com → 127.0.0.1
  Your server connects to localhost ❌
```

**Prevention:**
```python
def fetch_url_safely(url):
    # Resolve DNS
    ip1 = resolve_dns(url)
    
    # Check if safe
    if is_internal_ip(ip1):
        raise SecurityError("Internal IP blocked")
    
    # Wait a moment
    time.sleep(0.1)
    
    # Resolve again (detect DNS rebinding)
    ip2 = resolve_dns(url)
    
    if ip1 != ip2:
        raise SecurityError("DNS rebinding detected")
    
    # Now safe to fetch
    response = requests.get(url)
    return response
```

---

#### **Defense 3: Don't Fetch URLs for Validation**

**Best practice:**
```
DON'T do this:
  User submits URL → Fetch it → Validate response → Create short URL

DO this instead:
  User submits URL → Validate format only → Create short URL
  
Validation without fetching:
  - Check URL format (regex)
  - Check against malicious URL database (Google Safe Browsing)
  - Check domain reputation
  - DON'T actually fetch the URL
```

---

#### **Defense 4: Use Allow-List (If Fetching Required)**

```python
# If you MUST fetch URLs (e.g., for preview generation)

ALLOWED_PROTOCOLS = ['http', 'https']  # No file://, ftp://, etc.

def fetch_with_restrictions(url):
    parsed = urlparse(url)
    
    # Check protocol
    if parsed.scheme not in ALLOWED_PROTOCOLS:
        raise SecurityError("Only HTTP/HTTPS allowed")
    
    # Check IP not internal
    ip = resolve_dns(parsed.hostname)
    if is_internal_ip(ip):
        raise SecurityError("Internal IPs blocked")
    
    # Fetch with timeout and size limit
    response = requests.get(
        url,
        timeout=5,              # 5 second timeout
        max_redirects=0,        # Don't follow redirects
        stream=True             # Don't load entire response
    )
    
    # Check response size
    content_length = response.headers.get('content-length', 0)
    if int(content_length) > 10_000_000:  # 10 MB limit
        raise SecurityError("Response too large")
    
    return response
```

---

### **Interview Answer Template:**

**Q: "How do you prevent SSRF attacks?"**

```
"SSRF is when attackers trick our server into accessing internal services.

Prevention layers:

1. Blacklist internal IPs
   - Block 127.0.0.0/8 (localhost)
   - Block 10.0.0.0/8, 192.168.0.0/16 (private networks)
   - Block 169.254.169.254 (AWS metadata endpoint)

2. Don't fetch user-submitted URLs
   - For URL shortener, we don't need to fetch
   - Just validate format and check against malicious URL database
   
3. If fetching required (for other features):
   - Resolve DNS, check IP before fetching
   - Use timeouts and size limits
   - Don't follow redirects
   
4. DNS rebinding protection
   - Resolve DNS twice, ensure IP didn't change

This prevents attackers from accessing our Redis, databases,
or AWS credentials through our URL shortener."
```

---

## **6.2 Malicious URL Detection**

### 🤔 Interview Question/Context

**Interviewer:** "How do you prevent users from creating short URLs for phishing sites or malware?"

---

### ✅ Complete Answer

#### **The Problem:**

```
Attacker creates short URLs for:
- Phishing sites (fake login pages)
- Malware downloads
- Scam websites
- Illegal content

Your short URL becomes the distribution method.

Impact:
- Your service used for attacks
- Reputation damage
- Legal liability
- Users lose trust
```

---

### ✅ Detection Strategy (Layered Approach)

#### **Layer 1: Bloom Filter (Fast Pre-Check)**

```python
# 1.2 GB bloom filter with 1B known malicious URLs
malicious_bloom = load_bloom_filter()

def quick_check(url):
    if not malicious_bloom.might_contain(url):
        return "DEFINITELY_SAFE"  # 95% of URLs
    
    return "NEEDS_VERIFICATION"  # 5% of URLs
```

**Benefit:** Eliminate 95% of URLs instantly (0.5ms)

---

#### **Layer 2: Google Safe Browsing API**

```python
def check_malicious(url):
    # Layer 1: Bloom filter
    bloom_result = quick_check(url)
    
    if bloom_result == "DEFINITELY_SAFE":
        return False  # Safe, no API call needed
    
    # Layer 2: Google Safe Browsing (for 5% of URLs)
    try:
        result = google_safe_browsing_api.check(url, timeout=100ms)
        return result.is_malicious
    except APIError:
        # API down, check Layer 3
        pass
    
    # Layer 3: Local database (fallback)
    return local_malicious_db.contains(url)
```

---

#### **Layer 3: User Reporting**

```python
# Allow users to report malicious URLs

def report_url(short_code, user_id, reason):
    report = {
        'short_code': short_code,
        'reported_by': user_id,
        'reason': reason,
        'timestamp': now()
    }
    
    # Store report
    db.insert('reports', report)
    
    # If 5+ reports, auto-disable URL
    report_count = db.count_reports(short_code)
    
    if report_count >= 5:
        db.update('urls', short_code, status='DISABLED')
        alert_security_team(short_code)
```

---

#### **Layer 4: Pattern Detection**

```python
# Detect suspicious patterns

def detect_suspicious_patterns(url, user_id):
    # Same user creating many URLs quickly
    recent_urls = db.count_urls_by_user(user_id, last_hour=True)
    if recent_urls > 20:
        flag_for_review(user_id, "Bulk creation")
    
    # URL contains known phishing keywords
    phishing_keywords = ['login', 'verify', 'account', 'suspended']
    if any(keyword in url.lower() for keyword in phishing_keywords):
        flag_for_review(url, "Phishing keywords")
    
    # Newly registered domain (high risk)
    domain_age = get_domain_age(url)
    if domain_age < 30_days:
        flag_for_review(url, "New domain")
```

---

### **Interview Answer:**

**Q: "How do you detect malicious URLs?"**

```
"We use a layered approach:

1. Bloom filter (pre-screening)
   - 1.2 GB filter with 1B known malicious URLs
   - Eliminates 95% of checks instantly
   
2. Google Safe Browsing API
   - For URLs that pass bloom filter
   - Industry-standard malicious URL database
   
3. User reporting
   - Users can report suspicious URLs
   - Auto-disable after 5 reports
   
4. Pattern detection
   - Flag bulk creation
   - Detect phishing keywords
   - Check domain age

If URL flagged as malicious:
- Reject creation (with error message)
- Or disable existing short URL
- Alert security team for review

Cost: $500/month (vs $10K without bloom filter optimization)"
```

---

## **6.3 API Failure Handling (Fail Open vs Fail Closed)**

### 🤔 Interview Question/Context

**Interviewer:** "Google Safe Browsing API is down. Do you block all URL creation (fail closed) or allow it (fail open)?"

### ❌ Your Initial Answer

You initially said: **"Fail closed (reject all) - user safety is top priority"**

**The problem:** This causes service outage when external API fails.

---

### ✅ Complete Answer

#### **The Trade-off:**

```
Fail Closed:
  API down → Block all URL creation
  
  ✓ Maximum security
  ✗ Service unavailable (bad UX)
  ✗ Dependent on external service
  ✗ Revenue loss during outage

Fail Open:
  API down → Allow URL creation (with warnings)
  
  ✓ Service stays available
  ✓ Not dependent on external service
  ✗ Some malicious URLs might get through
  
Which is worse?
  - Service down for 100% of users?
  - Or 0.1% malicious URLs get through?
```

---

### ✅ Recommended Strategy: Fail Open with Safeguards

```python
def create_short_url(long_url):
    # Try primary check
    try:
        is_malicious = google_safe_browsing_api.check(
            long_url, 
            timeout=100ms
        )
        
        if is_malicious:
            return 403, "URL flagged as malicious"
        
        # Safe, proceed
        
    except APITimeout:
        # API slow, use fallback
        log.warning("Safe Browsing API timeout")
        
    except APIError:
        # API down, fail open with safeguards
        log.error("Safe Browsing API unavailable")
        
        # Fallback checks
        if bloom_filter.probably_malicious(long_url):
            return 403, "URL potentially malicious (service degraded)"
        
        if local_db.is_malicious(long_url):
            return 403, "URL known to be malicious"
        
        # Passed fallback checks, allow but flag
        url_id = create_url(long_url, flagged=True)
        
        # Async: Review when API recovers
        queue_for_review(url_id)
        
        return 201, {
            "shortUrl": f"tiny.url/{url_id}",
            "warning": "Security check unavailable, URL will be reviewed"
        }
```

---

### **Interview Answer:**

**Q: "Fail open or fail closed when security API is down?"**

```
"I'd fail open with safeguards:

Why fail open:
- Service availability is critical (URLs embedded in tweets, emails)
- External API outages shouldn't take down our service
- Malicious URLs are rare (<0.1% of submissions)

Safeguards when API down:
1. Check local bloom filter (1B known malicious URLs)
2. Check local database (recently flagged URLs)
3. If both pass, allow creation but FLAG for review
4. When API recovers, review flagged URLs retroactively
5. Disable any found to be malicious

Trade-off:
- Fail closed: 100% of users blocked, 0% malicious URLs
- Fail open: 0% of users blocked, maybe 0.1% malicious URLs slip through

For URL shortener, service availability > perfect security.
We can catch malicious URLs after creation and disable them."
```

---

## **6.4 Rate Limiting Abuse Prevention**

### 🤔 Interview Question/Context

**Interviewer:** "Users are bypassing rate limits using VPNs and multiple API keys. How do you detect and prevent this?"

---

### ✅ Detection Strategies

#### **1. Behavioral Analysis**

```python
def detect_abuse_patterns(user_id, url):
    # Same long URL created by many "different" users
    similar_urls = db.query("""
        SELECT COUNT(DISTINCT user_id) 
        FROM urls 
        WHERE long_url = ? 
        AND created_at > NOW() - INTERVAL 1 HOUR
    """, url)
    
    if similar_urls > 5:
        flag_abuse("Same URL created by 5+ users in 1 hour")
    
    # Rapid sequential creation
    last_10_urls = db.get_recent_urls(user_id, limit=10)
    time_diff = last_10_urls[0].time - last_10_urls[9].time
    
    if time_diff < 60_seconds:
        flag_abuse("10 URLs in 60 seconds")
    
    # Identical URL patterns
    if all_urls_match_pattern(last_10_urls):
        flag_abuse("Automated pattern detected")
```

---

#### **2. Device Fingerprinting**

```javascript
// Browser-side fingerprinting
const fingerprint = {
    canvas: canvas_hash(),        // Canvas rendering signature
    webgl: webgl_hash(),          // WebGL rendering signature
    fonts: installed_fonts(),     // Font list
    plugins: browser_plugins(),   // Plugin list
    screen: screen_resolution(),
    timezone: timezone_offset()
};

// Combine with IP for more accurate ID
const device_id = hash(fingerprint + ip_subnet);
```

---

#### **3. CAPTCHA After Threshold**

```python
def should_show_captcha(user_id, ip):
    # Count recent requests
    recent_count = redis.get(f"count:{ip}:last_hour") or 0
    
    if recent_count > 10:
        return True  # Show CAPTCHA after 10 URLs
    
    return False

# After CAPTCHA solved, allow more URLs
def handle_captcha_success(ip):
    redis.set(f"captcha_solved:{ip}", True, ex=3600)  # 1 hour
    redis.delete(f"count:{ip}:last_hour")  # Reset counter
```

---

### **Interview Answer:**

**Q: "How do you prevent rate limit bypass?"**

```
"Multiple detection layers:

1. Behavioral analysis
   - Same URL created by many 'different' users → Coordinated attack
   - Rapid sequential creation → Automation
   - Pattern matching → Bot behavior

2. Device fingerprinting (browser)
   - Combine canvas/WebGL/fonts to identify device
   - Harder to spoof than IP alone
   
3. Progressive challenges
   - First 3 URLs: No friction
   - After 5 URLs: Show CAPTCHA
   - After 10 URLs: Require email verification
   - After 20 URLs: Require phone verification

4. Account-based limits
   - Unauthenticated: 10 URLs/day
   - Email verified: 100 URLs/day
   - Phone verified: 1000 URLs/day
   - Paid: 10,000 URLs/day

For persistent abuse:
- Ban device fingerprint + IP subnet
- Require credit card verification (even for free tier)
- Manual review for high-volume users

Balance: Stop abuse without hurting legitimate users"
```

---

# **SECTION 6 COMPLETE!** ✅

## **Section 6 Quick Revision:**

```
6.1 SSRF Prevention
    - Block internal IPs (127.0.0.0/8, 10.0.0.0/8, 169.254.169.254)
    - Don't fetch user URLs
    - DNS rebinding protection
    
6.2 Malicious URLs
    - Bloom filter (95% fast reject)
    - Google Safe Browsing API
    - User reporting (5+ reports → disable)
    
6.3 API Failure
    - Fail open (service availability > perfect security)
    - Use fallback checks (bloom filter, local DB)
    - Flag for review when API recovers
    
6.4 Rate Limit Abuse
    - Behavioral analysis (patterns, same URL from many users)
    - Device fingerprinting (canvas, WebGL)
    - Progressive challenges (CAPTCHA, email, phone)
```

**Progress: 20/32 topics (62.5%)** 🎉

---

**You've covered the MOST CRITICAL sections for SDE-2 interviews!**

**Sections 1-4, 6 = 80% of interview questions** ✅

**Remaining sections (lower priority):**
- Section 5: CDN (skip or 5-min summary)
- Section 7: Advanced features (custom URLs, etc.) - mostly covered
- Section 8: Production readiness (deployment, cost) - less critical for design interviews

**Would you like to:**
1. **STOP HERE** - You have 80% coverage (recommended) ⭐
2. Quick 10-min summary of Sections 5, 7, 8 (polish remaining 20%)
3. Deep dive into any specific topic you want to review

**What do you prefer?** 🚀