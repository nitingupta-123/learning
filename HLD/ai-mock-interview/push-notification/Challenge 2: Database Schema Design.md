# Challenge 2: Database Schema Design - Complete Documentation

---

## 📊 Complete Database Schema for Notification System

This document provides a comprehensive database schema design for a scalable notification system handling 10M+ notifications per day across multiple channels (Push, SMS, Email, In-app).

---

## 1. USER TABLE (SQL - PostgreSQL/MySQL)

Stores core user information.

```sql
CREATE TABLE users (
    user_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    email VARCHAR(255) UNIQUE NOT NULL,
    phone_number VARCHAR(20),
    country_code VARCHAR(5),
    full_name VARCHAR(255),
    timezone VARCHAR(50) DEFAULT 'UTC',  -- For Do Not Disturb hours
    language_preference VARCHAR(10) DEFAULT 'en',  -- For template localization
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    INDEX idx_email (email),
    INDEX idx_phone (phone_number, country_code),
    INDEX idx_created_at (created_at)
);
```

**Design Notes:**
- `user_id` as BIGINT supports 9+ quintillion users
- `timezone` enables respecting user's Do Not Disturb hours
- `language_preference` for multi-language template support
- `is_active` for soft deletes

**Handling Multiple Emails/Phones:**
For users with multiple contact methods, create separate tables:

```sql
CREATE TABLE user_emails (
    email_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    email VARCHAR(255) NOT NULL,
    email_type ENUM('primary', 'work', 'personal') DEFAULT 'primary',
    is_verified BOOLEAN DEFAULT FALSE,
    verified_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    UNIQUE KEY unique_email (email),
    INDEX idx_user_email (user_id, email_type)
);

CREATE TABLE user_phones (
    phone_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    phone_number VARCHAR(20) NOT NULL,
    country_code VARCHAR(5) NOT NULL,
    phone_type ENUM('primary', 'work', 'personal') DEFAULT 'primary',
    is_verified BOOLEAN DEFAULT FALSE,
    verified_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    UNIQUE KEY unique_phone (phone_number, country_code),
    INDEX idx_user_phone (user_id, phone_type)
);
```

---

## 2. DEVICE TABLE (SQL)

Stores user devices for push notifications.

```sql
CREATE TABLE devices (
    device_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    device_token VARCHAR(500) NOT NULL,  -- FCM/APNs token
    device_type ENUM('ios', 'android', 'web') NOT NULL,
    device_name VARCHAR(255),  -- e.g., "iPhone 13 Pro"
    app_version VARCHAR(50),
    os_version VARCHAR(50),
    is_active BOOLEAN DEFAULT TRUE,  -- Track if token is still valid
    token_expiry_date TIMESTAMP NULL,  -- Some tokens expire
    last_login_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    UNIQUE KEY unique_token (device_token),
    INDEX idx_user_device (user_id, is_active),
    INDEX idx_token (device_token)
);
```

**Design Notes:**
- `is_active` tracks token validity (FCM/APNs tokens can become invalid)
- `token_expiry_date` for tokens with known expiration
- One user can have multiple devices (iPhone, iPad, Android phone)
- `device_token` is unique globally

**Token Refresh Handling:**
When a token becomes invalid (reported by FCM/APNs):
```sql
UPDATE devices 
SET is_active = FALSE, 
    updated_at = CURRENT_TIMESTAMP 
WHERE device_token = '<invalid_token>';
```

---

## 3. NOTIFICATION TYPES (SQL)

Define all types of notifications your system supports.

```sql
CREATE TABLE notification_types (
    type_id INT PRIMARY KEY AUTO_INCREMENT,
    type_name VARCHAR(100) UNIQUE NOT NULL,  -- e.g., 'order_shipped', 'payment_received'
    display_name VARCHAR(255) NOT NULL,  -- User-friendly name
    category ENUM('transactional', 'promotional', 'reminder', 'alert') NOT NULL,
    default_priority ENUM('critical', 'high', 'medium', 'low') DEFAULT 'medium',
    description TEXT,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_category (category),
    INDEX idx_type_name (type_name)
);
```

**Example Data:**
```sql
INSERT INTO notification_types (type_name, display_name, category, default_priority) VALUES
('otp_verification', 'OTP Verification', 'transactional', 'critical'),
('order_shipped', 'Order Shipped', 'transactional', 'high'),
('payment_received', 'Payment Confirmation', 'transactional', 'high'),
('cart_reminder', 'Cart Reminder', 'reminder', 'medium'),
('flash_sale', 'Flash Sale Alert', 'promotional', 'low'),
('price_drop', 'Price Drop Alert', 'promotional', 'low');
```

---

## 4. USER NOTIFICATION PREFERENCES (SQL)

**Critical Table**: Supports per-notification-type, per-channel preferences.

```sql
CREATE TABLE user_notification_preferences (
    preference_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    notification_type_id INT NOT NULL,
    channel ENUM('email', 'sms', 'push', 'in_app') NOT NULL,
    is_enabled BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (notification_type_id) REFERENCES notification_types(type_id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_type_channel (user_id, notification_type_id, channel),
    INDEX idx_user_prefs (user_id, channel, is_enabled)
);
```

**Example Data:**
```sql
-- User 123 wants order updates via SMS + Email, but promotional only via Email
INSERT INTO user_notification_preferences (user_id, notification_type_id, channel, is_enabled) VALUES
(123, 2, 'email', TRUE),   -- order_shipped via email
(123, 2, 'sms', TRUE),     -- order_shipped via SMS
(123, 2, 'push', TRUE),    -- order_shipped via push
(123, 5, 'email', TRUE),   -- flash_sale via email only
(123, 5, 'sms', FALSE),    -- NO SMS for flash sales
(123, 5, 'push', FALSE);   -- NO push for flash sales
```

**Preference Matrix Representation:**
```
User 123:       | Email | SMS | Push | In-App
----------------|-------|-----|------|-------
Order Shipped   |  ✓    |  ✓  |  ✓   |   ✓
Payment Received|  ✓    |  ✓  |  ✓   |   ✓
Flash Sale      |  ✓    |  ✗  |  ✗   |   ✓
Cart Reminder   |  ✗    |  ✗  |  ✓   |   ✓
```

**Global Channel Opt-Out:**
If user wants to disable ALL SMS:
```sql
CREATE TABLE user_channel_preferences (
    user_id BIGINT PRIMARY KEY,
    email_enabled BOOLEAN DEFAULT TRUE,
    sms_enabled BOOLEAN DEFAULT TRUE,
    push_enabled BOOLEAN DEFAULT TRUE,
    in_app_enabled BOOLEAN DEFAULT TRUE,
    
    -- Do Not Disturb settings
    dnd_start_hour INT DEFAULT NULL,  -- e.g., 22 (10 PM)
    dnd_end_hour INT DEFAULT NULL,    -- e.g., 8 (8 AM)
    
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);
```

**Preference Check Logic:**
```python
def should_send_notification(user_id, notification_type, channel):
    # 1. Check global channel preference
    global_pref = get_channel_preference(user_id, channel)
    if not global_pref.enabled:
        return False
    
    # 2. Check Do Not Disturb hours
    if is_in_dnd_hours(user_id):
        if notification_priority != 'critical':  # OTP still goes through
            return False
    
    # 3. Check specific notification type + channel preference
    specific_pref = get_notification_preference(user_id, notification_type, channel)
    return specific_pref.is_enabled
```

---

## 5. NOTIFICATION TEMPLATES (SQL)

Stores reusable templates with versioning support.

```sql
CREATE TABLE notification_templates (
    template_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    template_name VARCHAR(255) UNIQUE NOT NULL,  -- e.g., 'order_shipped_email_v2'
    notification_type_id INT NOT NULL,
    channel ENUM('email', 'sms', 'push', 'in_app') NOT NULL,
    language VARCHAR(10) DEFAULT 'en',
    
    -- Email specific
    subject_template TEXT,  -- "Your order {{order_id}} has shipped!"
    
    -- All channels
    body_template TEXT NOT NULL,  -- Template with placeholders
    
    -- Template metadata
    version INT DEFAULT 1,
    is_active BOOLEAN DEFAULT TRUE,
    
    -- JSON for additional config
    metadata JSON,  -- e.g., {"retry_count": 3, "ttl": 3600}
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_by VARCHAR(255),  -- Who created/updated this template
    
    FOREIGN KEY (notification_type_id) REFERENCES notification_types(type_id),
    INDEX idx_template_lookup (notification_type_id, channel, language, is_active),
    INDEX idx_template_name (template_name)
);
```

**Template Versioning:**
```sql
-- When updating a template, create new version
INSERT INTO notification_templates 
    (template_name, notification_type_id, channel, language, subject_template, body_template, version)
VALUES 
    ('order_shipped_email_v3', 2, 'email', 'en', 
     'Order {{order_id}} is on its way! 🚚', 
     'Hi {{customer_name}}, your order will arrive by {{delivery_date}}.', 
     3);

-- Deactivate old version
UPDATE notification_templates 
SET is_active = FALSE 
WHERE template_name LIKE 'order_shipped_email_v2';
```

**Example Templates:**
```sql
INSERT INTO notification_templates 
    (template_name, notification_type_id, channel, language, subject_template, body_template, version)
VALUES
    -- Email template
    ('order_shipped_email_en_v1', 2, 'email', 'en',
     'Your order {{order_id}} has shipped!',
     'Hi {{customer_name}},\n\nYour order #{{order_id}} has been shipped and will arrive by {{delivery_date}}.\n\nTracking: {{tracking_url}}',
     1),
    
    -- SMS template (short)
    ('order_shipped_sms_en_v1', 2, 'sms', 'en',
     NULL,
     'Your order {{order_id}} shipped! Track: {{short_url}}',
     1),
    
    -- Push notification
    ('order_shipped_push_en_v1', 2, 'push', 'en',
     'Order Shipped',
     'Your order #{{order_id}} is on the way!',
     1);
```

**Caching Strategy for Templates:**
- **Primary Storage**: SQL database (source of truth)
- **Cache Layer**: Redis with key pattern `template:{type_id}:{channel}:{language}`
- **TTL**: 1 hour (templates don't change frequently)
- **Cache Invalidation**: When template is updated, invalidate specific cache key

```python
def get_template(notification_type_id, channel, language):
    cache_key = f"template:{notification_type_id}:{channel}:{language}"
    
    # Try cache first
    template = redis.get(cache_key)
    if template:
        return json.loads(template)
    
    # Cache miss - fetch from database
    template = db.query("""
        SELECT * FROM notification_templates 
        WHERE notification_type_id = ? 
          AND channel = ? 
          AND language = ? 
          AND is_active = TRUE
        ORDER BY version DESC 
        LIMIT 1
    """, [notification_type_id, channel, language])
    
    # Cache for 1 hour
    redis.setex(cache_key, 3600, json.dumps(template))
    return template
```

---

## 6. NOTIFICATION LOG (NoSQL - MongoDB/DynamoDB)

High-volume table for tracking all notification attempts and delivery status.

**MongoDB Schema:**
```javascript
{
    _id: ObjectId("..."),
    event_id: "order_123_shipped_20260106",  // For deduplication
    user_id: 12345,
    notification_type: "order_shipped",
    channel: "email",
    priority: "high",
    
    // Recipient info (denormalized for fast queries)
    recipient: {
        email: "user@example.com",
        phone: "+1234567890",
        device_token: "fcm_token_xyz"
    },
    
    // Template and content
    template_id: 456,
    template_version: 2,
    rendered_content: {
        subject: "Your order #123 has shipped!",
        body: "Hi John, your order will arrive by Jan 10th."
    },
    
    // Status tracking
    status: "delivered",  // pending, queued, sent, delivered, failed, retrying, abandoned
    
    // Timestamps
    created_at: ISODate("2026-01-06T10:30:00Z"),
    queued_at: ISODate("2026-01-06T10:30:01Z"),
    sent_at: ISODate("2026-01-06T10:30:03Z"),
    delivered_at: ISODate("2026-01-06T10:30:05Z"),
    
    // Retry tracking
    retry_count: 0,
    max_retries: 3,
    last_retry_at: null,
    next_retry_at: null,
    
    // Third-party tracking
    third_party_message_id: "msg_sendgrid_abc123",
    third_party_response: {
        status_code: 200,
        message: "Accepted"
    },
    
    // Error tracking (if failed)
    error_code: null,
    error_message: null,
    
    // Metadata
    metadata: {
        order_id: "123",
        tracking_url: "https://track.example.com/123",
        customer_name: "John Doe"
    },
    
    // Indexes for queries
    indexes: [
        { user_id: 1, created_at: -1 },  // User's notification history
        { event_id: 1 },  // Deduplication
        { status: 1, next_retry_at: 1 },  // Find notifications to retry
        { created_at: -1 },  // Time-based queries
        { notification_type: 1, channel: 1, created_at: -1 }  // Analytics
    ]
}
```

**DynamoDB Schema (Alternative):**
```
Table: NotificationLogs

Primary Key: 
- Partition Key: user_id (BIGINT)
- Sort Key: created_at (TIMESTAMP)

Global Secondary Indexes:
- GSI1: event_id (for deduplication lookups)
- GSI2: status + next_retry_at (for retry queue)
- GSI3: notification_type + created_at (for analytics)

Attributes:
{
    "user_id": 12345,
    "created_at": "2026-01-06T10:30:00Z",
    "event_id": "order_123_shipped_20260106",
    "notification_type": "order_shipped",
    "channel": "email",
    "status": "delivered",
    "recipient": {
        "email": "user@example.com"
    },
    "template_id": 456,
    "rendered_content": {...},
    "retry_count": 0,
    "third_party_message_id": "msg_abc123",
    "metadata": {...}
}
```

**Status State Machine:**
```
PENDING → QUEUED → PROCESSING → SENT → DELIVERED ✓
                      ↓
                   FAILED → RETRYING → [back to QUEUED]
                      ↓
                   ABANDONED (after max retries) ✗
```

**Allowed State Transitions:**
```python
ALLOWED_TRANSITIONS = {
    'pending': ['queued', 'failed'],
    'queued': ['processing', 'failed'],
    'processing': ['sent', 'failed'],
    'sent': ['delivered', 'failed'],
    'failed': ['retrying', 'abandoned'],
    'retrying': ['queued'],
    'delivered': [],  # Terminal state
    'abandoned': []   # Terminal state
}
```

---

## 7. DEDUPLICATION STRATEGY

**Problem:** Prevent duplicate notifications from being sent.

**Solution: Redis-based Deduplication**

```python
def check_and_mark_duplicate(event_id, ttl=3600):
    """
    Atomically check if event_id exists and set it if not.
    Returns True if this is a new event, False if duplicate.
    """
    # Redis SET with NX (only set if not exists) and EX (expiry)
    result = redis.set(
        key=f"event:dedupe:{event_id}",
        value="1",
        nx=True,  # Only set if key doesn't exist
        ex=ttl    # Expire after 1 hour (adjust based on use case)
    )
    
    return result is not None  # True if set successfully, False if already exists

# Usage
event_id = f"order_{order_id}_shipped_{timestamp}"
if not check_and_mark_duplicate(event_id):
    logger.info(f"Duplicate notification detected: {event_id}")
    return  # Skip processing
```

**Why Redis?**
- ✅ Atomic operation (no race conditions)
- ✅ Fast (<1ms latency)
- ✅ Automatic expiry (TTL)
- ✅ Handles high throughput

**Alternative: Database Unique Constraint**
```sql
CREATE TABLE notification_deduplication (
    event_id VARCHAR(255) PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    
    INDEX idx_expires (expires_at)
);

-- Cleanup expired entries (run periodically)
DELETE FROM notification_deduplication WHERE expires_at < NOW();
```

**Trade-offs:**
- Redis: Faster but requires separate infrastructure
- Database: Simpler but slower (10-50ms vs <1ms)

**Recommendation:** Use Redis for deduplication at 10M+ notifications/day scale.

---

## 8. NOTIFICATION QUEUE METADATA (Message Queue Attributes)

While the actual queue is in Kafka/SQS/RabbitMQ, you need to track metadata.

**Option A: Store in Message Queue Attributes (Recommended)**

When publishing to Kafka/SQS:
```python
message = {
    "user_id": 12345,
    "notification_type": "order_shipped",
    "channel": "email",
    "priority": "high",  # critical, high, medium, low
    "event_id": "order_123_shipped_20260106",
    "scheduled_at": "2026-01-06T10:30:00Z",
    "retry_count": 0,
    "max_retries": 3,
    "metadata": {
        "order_id": "123",
        "customer_name": "John Doe",
        "tracking_url": "https://..."
    }
}

# SQS example with message attributes
sqs.send_message(
    QueueUrl=queue_url,
    MessageBody=json.dumps(message),
    MessageAttributes={
        'priority': {'DataType': 'String', 'StringValue': 'high'},
        'notification_type': {'DataType': 'String', 'StringValue': 'order_shipped'},
        'retry_count': {'DataType': 'Number', 'StringValue': '0'}
    }
)
```

**Option B: Separate Tracking Table (If needed)**
```sql
CREATE TABLE notification_queue_tracking (
    queue_message_id VARCHAR(255) PRIMARY KEY,  -- SQS message ID
    user_id BIGINT NOT NULL,
    notification_type VARCHAR(100) NOT NULL,
    channel ENUM('email', 'sms', 'push', 'in_app') NOT NULL,
    priority ENUM('critical', 'high', 'medium', 'low') NOT NULL,
    retry_count INT DEFAULT 0,
    scheduled_at TIMESTAMP NOT NULL,
    status ENUM('queued', 'processing', 'completed') DEFAULT 'queued',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_scheduled (scheduled_at, status),
    INDEX idx_user_queue (user_id, created_at)
);
```

---

## 9. DEAD LETTER QUEUE TRACKING (SQL)

Track notifications that failed after all retries.

```sql
CREATE TABLE dead_letter_queue (
    dlq_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    event_id VARCHAR(255) NOT NULL,
    user_id BIGINT NOT NULL,
    notification_type VARCHAR(100) NOT NULL,
    channel ENUM('email', 'sms', 'push', 'in_app') NOT NULL,
    
    -- Original message
    original_message JSON NOT NULL,
    
    -- Failure details
    failure_reason TEXT NOT NULL,
    error_code VARCHAR(50),
    retry_count INT NOT NULL,
    last_error_message TEXT,
    
    -- Third-party info
    third_party_message_id VARCHAR(255),
    third_party_response JSON,
    
    -- Timestamps
    first_attempt_at TIMESTAMP NOT NULL,
    last_attempt_at TIMESTAMP NOT NULL,
    moved_to_dlq_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- Resolution
    is_resolved BOOLEAN DEFAULT FALSE,
    resolved_at TIMESTAMP NULL,
    resolved_by VARCHAR(255),
    resolution_notes TEXT,
    
    INDEX idx_user_dlq (user_id, moved_to_dlq_at),
    INDEX idx_event_id (event_id),
    INDEX idx_unresolved (is_resolved, moved_to_dlq_at),
    INDEX idx_notification_type (notification_type, channel)
);
```

**DLQ Processing:**
```python
def move_to_dead_letter_queue(notification_data, failure_info):
    db.execute("""
        INSERT INTO dead_letter_queue 
        (event_id, user_id, notification_type, channel, original_message, 
         failure_reason, error_code, retry_count, last_error_message,
         third_party_message_id, first_attempt_at, last_attempt_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, [
        notification_data['event_id'],
        notification_data['user_id'],
        notification_data['notification_type'],
        notification_data['channel'],
        json.dumps(notification_data),
        failure_info['reason'],
        failure_info['error_code'],
        notification_data['retry_count'],
        failure_info['last_error'],
        notification_data.get('third_party_message_id'),
        notification_data['first_attempt_at'],
        notification_data['last_attempt_at']
    ])
    
    # Alert monitoring system
    send_alert_to_ops(f"Notification moved to DLQ: {notification_data['event_id']}")
```

---

## 10. ANALYTICS & MONITORING TABLES (Optional but Recommended)

For dashboards and monitoring.

```sql
-- Daily aggregated stats
CREATE TABLE notification_stats_daily (
    stat_date DATE NOT NULL,
    notification_type VARCHAR(100) NOT NULL,
    channel ENUM('email', 'sms', 'push', 'in_app') NOT NULL,
    
    total_sent INT DEFAULT 0,
    total_delivered INT DEFAULT 0,
    total_failed INT DEFAULT 0,
    total_retried INT DEFAULT 0,
    
    avg_delivery_time_seconds DECIMAL(10,2),  -- Time from sent to delivered
    p95_delivery_time_seconds DECIMAL(10,2),
    p99_delivery_time_seconds DECIMAL(10,2),
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    PRIMARY KEY (stat_date, notification_type, channel),
    INDEX idx_date (stat_date)
);

-- Real-time service health
CREATE TABLE service_health_log (
    log_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    service_name VARCHAR(100) NOT NULL,  -- 'apns', 'fcm', 'twilio', 'sendgrid'
    status ENUM('healthy', 'degraded', 'down') NOT NULL,
    response_time_ms INT,
    error_rate DECIMAL(5,2),  -- Percentage
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_service_time (service_name, timestamp)
);
```

---

## 📊 Complete ER Diagram Summary

```
users (1) ────────┐
  ↓              │
  ├─ user_emails (M)
  ├─ user_phones (M)
  ├─ devices (M)
  ├─ user_notification_preferences (M)
  └─ user_channel_preferences (1)

notification_types (1)
  ├─ notification_templates (M)
  └─ user_notification_preferences (M)

notification_logs (NoSQL) ← Main transaction log
dead_letter_queue ← Failed notifications
notification_stats_daily ← Analytics
service_health_log ← Monitoring
```

---

## 🚀 Key Design Decisions Summary

| Aspect | Decision | Reasoning |
|--------|----------|-----------|
| **User Preferences** | Separate table with notification_type + channel granularity | Supports complex "Email for orders, Push for reminders" scenarios |
| **Templates** | SQL + Redis cache | SQL as source of truth, Redis for speed |
| **Template Versioning** | Version column + is_active flag | Allows A/B testing and safe rollbacks |
| **Notification Logs** | NoSQL (MongoDB/DynamoDB) | High write volume (10M+/day), flexible schema |
| **Deduplication** | Redis with atomic SET NX | Fast, atomic, auto-expiry |
| **Status Tracking** | State machine with allowed transitions | Clear status flow, prevents invalid states |
| **DLQ** | Separate SQL table | Easy querying for ops team, resolution tracking |
| **Multiple Contacts** | Separate user_emails, user_phones tables | Supports users with multiple contact methods |

---

## 💾 Indexing Strategy

**Critical Indexes:**
1. `notification_logs`: 
   - `(user_id, created_at)` - User notification history
   - `(event_id)` - Deduplication lookups
   - `(status, next_retry_at)` - Retry processing
2. `user_notification_preferences`:
   - `(user_id, channel, is_enabled)` - Fast preference checks
3. `devices`:
   - `(user_id, is_active)` - Active device lookups
4. `notification_templates`:
   - `(notification_type_id, channel, language, is_active)` - Template lookups

---

## 🎯 Cache Strategy Summary

```
Redis Cache:
├─ Templates: TTL 1 hour, invalidate on update
├─ User Preferences: TTL 30 min, invalidate on user update
├─ Device Tokens: TTL 1 hour, invalidate on device update
└─ Deduplication: TTL 1 hour (event window)
```

---

This completes the **Challenge 2: Database Schema Design**. This schema supports:
- ✅ 10M+ notifications/day
- ✅ Multi-channel (Email, SMS, Push, In-app)
- ✅ Complex user preferences
- ✅ Template versioning
- ✅ Deduplication
- ✅ Retry logic with DLQ
- ✅ Full audit trail
- ✅ Analytics and monitoring

**Save this document for your interview prep!** 📚