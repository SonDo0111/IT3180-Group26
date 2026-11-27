BEGIN;

CREATE TABLE accounts (
    id                      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    username                VARCHAR(64)  NOT NULL,
    password_hash           TEXT         NOT NULL,
    role                    VARCHAR(32)  NOT NULL,
    status                  VARCHAR(32)  NOT NULL DEFAULT 'ACTIVE',
    failed_attempt_count    SMALLINT     NOT NULL DEFAULT 0,
    first_failed_attempt_at TIMESTAMPTZ  NULL,
    locked_until            TIMESTAMPTZ  NULL,
    version                 INTEGER      NOT NULL DEFAULT 1,
    created_at              TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at              TIMESTAMPTZ  NOT NULL DEFAULT NOW(),

    -- Ràng buộc nghiệp vụ ngay tại tầng Database (Defense in Depth)
    CONSTRAINT chk_accounts_username_trimmed
        CHECK (BTRIM(username) <> '' AND username = BTRIM(username)),
    CONSTRAINT chk_accounts_role
        CHECK (role IN ('BQT', 'THU_QUY')),
    CONSTRAINT chk_accounts_status
        CHECK (status IN ('ACTIVE', 'LOCKED', 'DISABLED')),
    CONSTRAINT chk_accounts_failed_count
        CHECK (failed_attempt_count >= 0),
    CONSTRAINT chk_accounts_locked_invariant
        CHECK (status <> 'LOCKED' OR locked_until IS NOT NULL)
);

-- Đảm bảo username duy nhất KHÔNG phân biệt chữ hoa/thường 
CREATE UNIQUE INDEX uq_accounts_username_lower
    ON accounts (LOWER(username));

COMMIT;