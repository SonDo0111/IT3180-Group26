-- ==============================================================================
-- Seed Data for IT3180 Apartment Management System
-- Module: UC01 - Authentication & Account Management
-- Description:
--   Dữ liệu khởi tạo kiểm thử cho UC01 bao gồm các kịch bản:
--     1. admin         (BQT, ACTIVE)        - Password: Admin@123
--     2. thuquy01      (THU_QUY, ACTIVE)   - Password: Thuquy@123
--     3. user_disabled (THU_QUY, DISABLED) - Password: Password@123
--     4. user_locked   (THU_QUY, LOCKED)   - Password: Password@123 (Khóa còn hiệu lực 10m)
--     5. user_expired  (THU_QUY, LOCKED)   - Password: Password@123 (Khóa hết hạn 15m trước -> Lazy Unlock)
--     6. user_attempts (THU_QUY, ACTIVE)   - Password: Password@123 (failed_attempt_count = 4)
--
-- Password hashes được sinh theo chuẩn Bcrypt (cost = 10):
--   - Admin@123    : $2b$10$C0D7gk4H0eaNZXm8vBb35OKx7pNV/9NbtDcBjsus7fEcGyaeIu0Aq
--   - Thuquy@123   : $2b$10$GznMC36qB7tiO1lfTq.Xr.Ehm3Nvbq277JEprgindLn5AANCgb6Qu
--   - Password@123 : $2b$10$2baUveKB1NMeCiTDbvrByegG2AAayLwQ3XCL84rzjcZwK9iEIujei
-- ==============================================================================

BEGIN;

-- 1. Xóa dữ liệu cũ (theo thứ tự quan hệ khóa ngoại) để đảm bảo tính Idempotent khi chạy lại
DELETE FROM login_history;
DELETE FROM sessions;
DELETE FROM accounts;

-- 2. Thêm dữ liệu tài khoản mẫu
INSERT INTO accounts (
    id ,
    username,
    password_hash,
    role,
    status,
    failed_attempt_count,
    first_failed_attempt_at,
    locked_until,
    version,
    created_at,
    updated_at
)OVERRIDING SYSTEM VALUE VALUES
    -- (1) Tài khoản Quản trị viên (BQT) hoạt động bình thường
    (
        1,
        'admin',
        '$2b$10$C0D7gk4H0eaNZXm8vBb35OKx7pNV/9NbtDcBjsus7fEcGyaeIu0Aq',
        'BQT',
        'ACTIVE',
        0,
        NULL,
        NULL,
        1,
        NOW() - INTERVAL '30 days',
        NOW() - INTERVAL '30 days'
    ),

    -- (2) Tài khoản Thủ quỹ hoạt động bình thường
    (
        2,
        'thuquy01',
        '$2b$10$GznMC36qB7tiO1lfTq.Xr.Ehm3Nvbq277JEprgindLn5AANCgb6Qu',
        'THU_QUY',
        'ACTIVE',
        0,
        NULL,
        NULL,
        1,
        NOW() - INTERVAL '15 days',
        NOW() - INTERVAL '15 days'
    ),

    -- (3) Tài khoản bị vô hiệu hóa (DISABLED) bởi Ban Quản Trị
    (
        3,
        'user_disabled',
        '$2b$10$2baUveKB1NMeCiTDbvrByegG2AAayLwQ3XCL84rzjcZwK9iEIujei',
        'THU_QUY',
        'DISABLED',
        0,
        NULL,
        NULL,
        1,
        NOW() - INTERVAL '10 days',
        NOW() - INTERVAL '2 days'
    ),

    -- (4) Tài khoản đang bị khóa (LOCKED) và còn thời hạn khóa 10 phút
    (
        4,
        'user_locked',
        '$2b$10$2baUveKB1NMeCiTDbvrByegG2AAayLwQ3XCL84rzjcZwK9iEIujei',
        'THU_QUY',
        'LOCKED',
        5,
        NOW() - INTERVAL '5 minutes',
        NOW() + INTERVAL '10 minutes',
        1,
        NOW() - INTERVAL '7 days',
        NOW() - INTERVAL '5 minutes'
    ),

    -- (5) Tài khoản bị khóa (LOCKED) nhưng đã HẾT HẠN (quá 15 phút) -> Kiểm thử Lazy Unlock
    (
        5,
        'user_expired',
        '$2b$10$2baUveKB1NMeCiTDbvrByegG2AAayLwQ3XCL84rzjcZwK9iEIujei',
        'THU_QUY',
        'LOCKED',
        5,
        NOW() - INTERVAL '35 minutes',
        NOW() - INTERVAL '20 minutes',
        1,
        NOW() - INTERVAL '5 days',
        NOW() - INTERVAL '35 minutes'
    ),

    -- (6) Tài khoản đã nhập sai 4 lần trong vòng 2 phút qua -> Lần sai tiếp theo sẽ kích hoạt khóa
    (
        6,
        'user_attempts',
        '$2b$10$2baUveKB1NMeCiTDbvrByegG2AAayLwQ3XCL84rzjcZwK9iEIujei',
        'THU_QUY',
        'ACTIVE',
        4,
        NOW() - INTERVAL '2 minutes',
        NULL,
        1,
        NOW() - INTERVAL '3 days',
        NOW() - INTERVAL '2 minutes'
    );

-- Đồng bộ giá trị sequence của Identity column với ID lớn nhất vừa chèn
SELECT SETVAL(pg_get_serial_sequence('accounts', 'id'), COALESCE(MAX(id), 1)) FROM accounts;

-- 3. Thêm một số bản ghi mẫu cho lịch sử đăng nhập (Audit Log)
INSERT INTO login_history (
    account_id,
    attempted_username,
    occurred_at,
    result,
    source_info
) OVERRIDING SYSTEM VALUE VALUES
    -- Đăng nhập thành công của admin
    (
        1,
        'admin',
        NOW() - INTERVAL '1 day',
        'SUCCESS',
        'CLI / Localhost (127.0.0.1)'
    ),
    -- Đăng nhập sai mật khẩu của user_locked dẫn đến bị khóa
    (
        4,
        'user_locked',
        NOW() - INTERVAL '5 minutes',
        'FAILED_INVALID_CREDENTIALS',
        'CLI / Localhost (127.0.0.1)'
    ),
    -- Đăng nhập khi đã bị khóa
    (
        4,
        'user_locked',
        NOW() - INTERVAL '3 minutes',
        'FAILED_ACCOUNT_LOCKED',
        'CLI / Localhost (127.0.0.1)'
    ),
    -- Đăng nhập thất bại do username không tồn tại (account_id = NULL)
    (
        NULL,
        'hacker_anonymous',
        NOW() - INTERVAL '1 hour',
        'FAILED_INVALID_CREDENTIALS',
        'CLI / Remote (192.168.1.100)'
    );

COMMIT;
