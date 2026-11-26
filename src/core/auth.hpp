#pragma once

#include "core/types.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace it3180
{

struct UserAccount
{
    std::string username{};
    std::string password_hash{};
    Role role{Role::BQT};
    int failed_attempts{0};
    std::optional<TimePoint> locked_until{std::nullopt};
};

struct LoginAuditEntry
{
    std::string username{};
    TimePoint timestamp{};
    bool success{false};
    std::string detail{};
};

struct Session
{
    std::string token{};
    std::string username{};
    Role role{Role::BQT};
    TimePoint last_activity{};
    bool active{false};
};

struct LoginResult
{
    bool success{false};
    std::string message{};
    std::optional<std::string> session_token{std::nullopt};
    std::optional<Role> role{std::nullopt};
};

class BcryptHasher
{
public:
    static std::string hashPassword(std::string_view plaintext,
                                    std::string_view salt = "IT3180Grp26Salt16");
    static bool verifyPassword(std::string_view plaintext, std::string_view bcrypt_hash);
    static bool isBcryptHash(std::string_view hash_str);
};

class AuthService
{
public:
    static constexpr int kMaxFailedAttempts = 5;
    static constexpr std::chrono::minutes kLockoutDuration{15};
    static constexpr std::chrono::minutes kSessionTimeout{30};

    Result registerAccount(std::string_view username, std::string_view plain_password, Role role);

    LoginResult login(std::string_view username,
                      std::string_view plain_password,
                      TimePoint now = std::chrono::system_clock::now());

    Result validateSessionAndTouch(std::string_view session_token,
                                   TimePoint now = std::chrono::system_clock::now());

    Result authorizeAction(std::string_view session_token,
                           Permission required_permission,
                           TimePoint now = std::chrono::system_clock::now());

    std::optional<UserAccount> getAccount(std::string_view username) const;
    std::optional<Session> getSession(std::string_view session_token) const;
    const std::vector<LoginAuditEntry>& getAuditLogs() const;

private:
    void recordAudit(std::string_view username, TimePoint now, bool success, std::string_view msg);

    std::unordered_map<std::string, UserAccount> accounts_{};
    std::unordered_map<std::string, Session> sessions_{};
    std::vector<LoginAuditEntry> audit_logs_{};
    std::uint64_t next_token_id_{1};
};

} // namespace it3180
