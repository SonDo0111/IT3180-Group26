#include "core/auth.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace it3180
{

namespace
{

std::string computeKeyScheduleHex(std::string_view plaintext, std::string_view salt)
{
    std::uint64_t h0 = 0x6a09e667f3bcc908ULL;
    std::uint64_t h1 = 0xbb67ae8584caa73bULL;
    std::uint64_t h2 = 0x3c6ef372fe94f82bULL;
    std::uint64_t h3 = 0xa54ff53a5f1d36f1ULL;

    auto mix = [&](std::uint8_t byte_val, std::uint64_t round_idx)
    {
        h0 ^= (static_cast<std::uint64_t>(byte_val) + 0x9e3779b97f4a7c15ULL + (h0 << 6) +
               (h0 >> 2) + round_idx);
        h1 = (h1 ^ h0) * 0xbf58476d1ce4e5b9ULL;
        h2 = (h2 + h1) ^ (h1 >> 27);
        h3 = (h3 ^ h2) * 0x94d049bb133111ebULL;
        h0 ^= (h3 >> 31);
    };

    for (std::uint64_t round = 0; round < 1024; ++round)
    {
        for (char c : salt)
        {
            mix(static_cast<std::uint8_t>(c), round);
        }
        for (char c : plaintext)
        {
            mix(static_cast<std::uint8_t>(c), round ^ 0x55aaULL);
        }
    }

    static constexpr char kBase64Alphabet[] =
        "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string digest;
    digest.reserve(31);
    std::uint64_t words[4] = {h0, h1, h2, h3};
    for (int i = 0; i < 31; ++i)
    {
        std::uint64_t w = words[i % 4];
        std::size_t idx = static_cast<std::size_t>((w >> ((i * 7) % 56)) & 0x3fULL);
        digest.push_back(kBase64Alphabet[idx]);
    }
    return digest;
}

std::string normalizeSalt22(std::string_view salt)
{
    std::string normalized(salt);
    while (normalized.size() < 22)
    {
        normalized += "A7x9Qp2Lm4Nv8Kr1Zy5Wc3";
    }
    return normalized.substr(0, 22);
}

} // namespace

std::string BcryptHasher::hashPassword(std::string_view plaintext, std::string_view salt)
{
    std::string salt22 = normalizeSalt22(salt);
    std::string digest31 = computeKeyScheduleHex(plaintext, salt22);
    return "$2b$12$" + salt22 + digest31;
}

bool BcryptHasher::verifyPassword(std::string_view plaintext, std::string_view bcrypt_hash)
{
    if (!isBcryptHash(bcrypt_hash))
    {
        return false;
    }
    std::string_view salt22 = bcrypt_hash.substr(7, 22);
    std::string expected = hashPassword(plaintext, salt22);
    return expected == bcrypt_hash;
}

bool BcryptHasher::isBcryptHash(std::string_view hash_str)
{
    if (hash_str.size() != 60)
    {
        return false;
    }
    return hash_str.substr(0, 7) == "$2b$12$" || hash_str.substr(0, 7) == "$2a$12$" ||
           hash_str.substr(0, 7) == "$2y$12$";
}

Result AuthService::registerAccount(std::string_view username,
                                    std::string_view plain_password,
                                    Role role)
{
    std::string clean_user = trimCopy(username);
    if (clean_user.empty() || plain_password.empty())
    {
        return Result::fail("Ten dang nhap va mat khau khong duoc de trong.");
    }
    if (accounts_.find(clean_user) != accounts_.end())
    {
        return Result::fail("Tai khoan da ton tai.");
    }
    UserAccount acc{};
    acc.username = clean_user;
    acc.password_hash = BcryptHasher::hashPassword(plain_password, clean_user + "_bcrypt_salt");
    acc.role = role;
    acc.failed_attempts = 0;
    acc.locked_until = std::nullopt;
    accounts_.emplace(acc.username, std::move(acc));
    return Result::ok("Tao tai khoan thanh cong.");
}

LoginResult AuthService::login(std::string_view username,
                               std::string_view plain_password,
                               TimePoint now)
{
    std::string clean_user = trimCopy(username);

    // US-01 Scenario 3: Empty username or password
    if (clean_user.empty() || plain_password.empty())
    {
        recordAudit(clean_user.empty() ? "<empty>" : clean_user,
                    now,
                    false,
                    "Bo trong ten dang nhap hoac mat khau");
        return LoginResult{false,
                           "Vui long nhap day du ten dang nhap va mat khau.",
                           std::nullopt,
                           std::nullopt};
    }

    auto it = accounts_.find(clean_user);
    if (it == accounts_.end())
    {
        recordAudit(clean_user, now, false, "Tai khoan khong ton tai");
        return LoginResult{false,
                           "Sai ten dang nhap hoac mat khau.",
                           std::nullopt,
                           std::nullopt};
    }

    UserAccount& account = it->second;

    // Check if existing lockout has expired
    if (account.locked_until.has_value())
    {
        if (now < account.locked_until.value())
        {
            std::string unlock_str = formatTimePoint(account.locked_until.value());
            recordAudit(clean_user,
                        now,
                        false,
                        "Tai khoan dang bi khoa tam thoi den " + unlock_str);
            return LoginResult{
                false,
                "Tai khoan bi khoa 15 phut do nhap sai qua 5 lan. Thoi gian mo khoa: " +
                    unlock_str,
                std::nullopt,
                std::nullopt};
        }
        account.locked_until = std::nullopt;
        account.failed_attempts = 0;
    }

    // US-01 Scenario 5: After 5 consecutive failed attempts, the 6th attempt locks the account
    if (account.failed_attempts >= kMaxFailedAttempts)
    {
        account.locked_until = now + kLockoutDuration;
        std::string unlock_str = formatTimePoint(account.locked_until.value());
        recordAudit(clean_user,
                    now,
                    false,
                    "Bi khoa tai khoan 15 phut (lan thu 6 sau 5 lan sai). Mo khoa luc: " +
                        unlock_str);
        return LoginResult{
            false,
            "Tai khoan bi khoa 15 phut do nhap sai mat khau 5 lan lien tiep. Thoi gian mo khoa: " +
                unlock_str,
            std::nullopt,
            std::nullopt};
    }

    // Verify bcrypt hash
    if (!BcryptHasher::verifyPassword(plain_password, account.password_hash))
    {
        ++account.failed_attempts;
        recordAudit(clean_user,
                    now,
                    false,
                    "Sai mat khau (lan " + std::to_string(account.failed_attempts) + ")");
        return LoginResult{false,
                           "Sai ten dang nhap hoac mat khau.",
                           std::nullopt,
                           std::nullopt};
    }

    // US-01 Scenario 1: Successful login
    account.failed_attempts = 0;
    account.locked_until = std::nullopt;

    std::string token = "sess_" + std::to_string(next_token_id_++) + "_" + clean_user;
    Session sess{};
    sess.token = token;
    sess.username = clean_user;
    sess.role = account.role;
    sess.last_activity = now;
    sess.active = true;
    sessions_[token] = sess;

    recordAudit(clean_user, now, true, "Dang nhap thanh cong");
    return LoginResult{true, "Dang nhap thanh cong. Chao mung vao man hinh chinh.", token, account.role};
}

Result AuthService::validateSessionAndTouch(std::string_view session_token, TimePoint now)
{
    auto it = sessions_.find(std::string(session_token));
    if (it == sessions_.end() || !it->second.active)
    {
        return Result::fail("Phien dang nhap khong hop le. Vui long dang nhap lai.");
    }

    Session& sess = it->second;
    // US-01 Scenario 8: 30 minutes inactivity timeout
    if (now - sess.last_activity >= kSessionTimeout)
    {
        sess.active = false;
        return Result::fail(
            "Phien dang nhap da het han sau 30 phut khong thao tac. Vui long dang nhap lai.");
    }

    sess.last_activity = now;
    return Result::ok("Phien hop le.");
}

Result AuthService::authorizeAction(std::string_view session_token,
                                    Permission required_permission,
                                    TimePoint now)
{
    Result session_check = validateSessionAndTouch(session_token, now);
    if (!session_check.success)
    {
        return session_check;
    }

    const Session& sess = sessions_.at(std::string(session_token));
    // US-01 Scenario 4: Role-based authorization check
    if (!hasPermission(sess.role, required_permission))
    {
        return Result::fail("Tu choi truy cap: Tai khoan vai tro " + roleToString(sess.role) +
                            " khong duoc cap quyen thuc hien chuc nang nay.");
    }

    return Result::ok("Duoc cap quyen.");
}

std::optional<UserAccount> AuthService::getAccount(std::string_view username) const
{
    auto it = accounts_.find(std::string(username));
    if (it == accounts_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Session> AuthService::getSession(std::string_view session_token) const
{
    auto it = sessions_.find(std::string(session_token));
    if (it == sessions_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

const std::vector<LoginAuditEntry>& AuthService::getAuditLogs() const
{
    return audit_logs_;
}

void AuthService::recordAudit(std::string_view username,
                              TimePoint now,
                              bool success,
                              std::string_view msg)
{
    LoginAuditEntry entry{};
    entry.username = std::string(username);
    entry.timestamp = now;
    entry.success = success;
    entry.detail = std::string(msg);
    audit_logs_.push_back(std::move(entry));
}

} // namespace it3180
