BEGIN;

CREATE TABLE sessions (
    id               UUID         PRIMARY KEY DEFAULT gen_random_uuid(), -- Chống đoán ID phiên
    account_id       BIGINT       NOT NULL REFERENCES accounts(id) ON DELETE RESTRICT,
    status           VARCHAR(20)  NOT NULL DEFAULT 'ACTIVE',
    started_at       TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    last_activity_at TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    ended_at         TIMESTAMPTZ  NULL,

    CONSTRAINT chk_sessions_status
        CHECK (status IN ('ACTIVE', 'EXPIRED', 'LOGGED_OUT', 'REVOKED')),
    -- Bất biến State Machine: Phiên ACTIVE thì ended_at = NULL, phiên đã kết thúc thì bắt buộc có ended_at
    CONSTRAINT chk_sessions_ended_invariant
        CHECK ((status = 'ACTIVE' AND ended_at IS NULL) OR (status <> 'ACTIVE' AND ended_at IS NOT NULL))
);

-- Partial Index: tìm & thu hồi toàn bộ phiên đang ACTIVE của 1 account
CREATE INDEX idx_sessions_account_active
    ON sessions (account_id)
    WHERE status = 'ACTIVE';

COMMIT;
