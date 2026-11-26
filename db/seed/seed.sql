-- IT3180 Group 26 - Seed Data for US-01, US-03, US-04

INSERT INTO users (username, password_hash, role) VALUES
    ('bqt_admin', '$2b$12$bqt_admin_bcrypt_saltA5oD7vN2hK9mW1xP4qR8zL0cE3yT6uF1', 'BQT'),
    ('thuquy01', '$2b$12$thuquy01_bcrypt_saltA7x9Q2pL4mN8vK1rZ5yW3cB6dH0jG9sM2', 'ThuQuy')
ON CONFLICT (username) DO NOTHING;

INSERT INTO households (apartment_code, floor, area_m2, head_name, phone_number, move_in_date) VALUES
    ('1205', 12, 75.50, 'Nguyen Van An', '0901234567', '2025-01-15'),
    ('1502', 15, 92.00, 'Tran Thi Binh', '0912345678', '2025-03-01')
ON CONFLICT (apartment_code) DO NOTHING;

INSERT INTO residents (apartment_code, full_name, birth_date, gender, relation_to_head, cccd) VALUES
    ('1205', 'Nguyen Van An', '1985-04-10', 'Nam', 'Chu ho', '001085012345'),
    ('1205', 'Le Thi Hoa', '1988-09-20', 'Nu', 'Vo', '001188054321'),
    ('1205', 'Nguyen Minh Khang', '2016-06-01', 'Nam', 'Con', NULL)
ON CONFLICT (cccd) DO NOTHING;
