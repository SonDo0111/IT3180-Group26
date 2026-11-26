-- IT3180 Group 26 - Initial PostgreSQL Schema for US-01, US-03, US-04

CREATE TABLE IF NOT EXISTS users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(64) NOT NULL UNIQUE,
    password_hash CHAR(60) NOT NULL CHECK (password_hash LIKE '$2_$12$%'),
    role VARCHAR(16) NOT NULL CHECK (role IN ('BQT', 'ThuQuy')),
    failed_attempts INT NOT NULL DEFAULT 0,
    locked_until TIMESTAMPTZ NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS login_audit_logs (
    log_id BIGSERIAL PRIMARY KEY,
    username VARCHAR(64) NOT NULL,
    login_time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    success BOOLEAN NOT NULL,
    detail TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS households (
    apartment_code CHAR(4) PRIMARY KEY CHECK (apartment_code ~ '^(0[6-9]|[12][0-9]|30)(0[1-9]|[1-9][0-9])$'),
    floor INT NOT NULL CHECK (floor BETWEEN 6 AND 30),
    area_m2 NUMERIC(8, 2) NOT NULL CHECK (area_m2 > 0),
    head_name VARCHAR(128) NOT NULL,
    phone_number VARCHAR(20) NOT NULL,
    move_in_date DATE NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS household_change_logs (
    change_id BIGSERIAL PRIMARY KEY,
    apartment_code CHAR(4) NOT NULL REFERENCES households(apartment_code) ON DELETE CASCADE,
    changed_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    changed_by VARCHAR(64) NOT NULL,
    old_area_m2 NUMERIC(8, 2) NOT NULL,
    new_area_m2 NUMERIC(8, 2) NOT NULL,
    summary TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS fee_notices (
    notice_id VARCHAR(32) PRIMARY KEY,
    apartment_code CHAR(4) NOT NULL REFERENCES households(apartment_code) ON DELETE RESTRICT,
    billing_period VARCHAR(16) NOT NULL,
    billed_area_m2 NUMERIC(8, 2) NOT NULL CHECK (billed_area_m2 > 0),
    rate_per_m2 NUMERIC(12, 2) NOT NULL CHECK (rate_per_m2 >= 0),
    total_fee NUMERIC(14, 2) NOT NULL CHECK (total_fee >= 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS residents (
    resident_id BIGSERIAL PRIMARY KEY,
    apartment_code CHAR(4) NOT NULL REFERENCES households(apartment_code) ON DELETE CASCADE,
    full_name VARCHAR(128) NOT NULL,
    birth_date DATE NOT NULL CHECK (birth_date <= CURRENT_DATE),
    gender VARCHAR(16) NOT NULL,
    relation_to_head VARCHAR(32) NOT NULL,
    cccd CHAR(12) UNIQUE NULL CHECK (cccd IS NULL OR cccd ~ '^[0-9]{12}$')
);
