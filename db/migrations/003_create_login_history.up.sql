BEGIN;

CREATE TABLE login_history (
    id                 BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    account_id         BIGINT       NULL REFERENCES accounts(id) ON DELETE SET NULL, -- 0..1 (NULL nếu username không tồn tại, hoặc khi account bị xóa)
    attempted_username VARCHAR(64)  NOT NULL,
    occurred_at        TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    result             VARCHAR(40)  NOT NULL,
    source_info        VARCHAR(255) NULL,

    CONSTRAINT chk_login_history_result
        CHECK (result IN (
            'SUCCESS',
            'FAILED_INVALID_CREDENTIALS',
            'FAILED_ACCOUNT_LOCKED',
            'FAILED_ACCOUNT_DISABLED'
        )),
    -- Bất biến nghiệp vụ: Đăng nhập thành công thì bắt buộc phải gắn với một Account
    CONSTRAINT chk_login_history_success_account
        CHECK (result <> 'SUCCESS' OR account_id IS NOT NULL)
);

CREATE INDEX idx_login_history_occurred_at
    ON login_history (occurred_at DESC);

CREATE INDEX idx_login_history_account_id
    ON login_history (account_id)
    WHERE account_id IS NOT NULL;

COMMIT;
