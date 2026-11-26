#include "core/auth.hpp"
#include "core/types.hpp"
#include "modules/household/household.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

namespace
{

using namespace it3180;
using namespace std::chrono_literals;

void testUs01Scenarios()
{
    AuthService auth;
    HouseholdService hh_service(auth);

    Result reg_bqt = auth.registerAccount("bqt_user", "CorrectPass@123", Role::BQT);
    assert(reg_bqt.success);
    Result reg_cashier = auth.registerAccount("cashier_user", "CashierPass@123", Role::ThuQuy);
    assert(reg_cashier.success);

    TimePoint t0 = std::chrono::system_clock::now();

    // US-01 Scenario 6: Password is stored as bcrypt hash ($2b$12$...), never plaintext
    auto acc_opt = auth.getAccount("bqt_user");
    assert(acc_opt.has_value());
    assert(acc_opt->password_hash != "CorrectPass@123");
    assert(BcryptHasher::isBcryptHash(acc_opt->password_hash));

    // US-01 Scenario 3: Empty username or password
    LoginResult empty_user = auth.login("", "CorrectPass@123", t0);
    assert(!empty_user.success);
    LoginResult empty_pass = auth.login("bqt_user", "", t0);
    assert(!empty_pass.success);

    // US-01 Scenario 2: Wrong username or password
    LoginResult wrong_user = auth.login("non_existent", "CorrectPass@123", t0);
    assert(!wrong_user.success);
    LoginResult wrong_pass = auth.login("bqt_user", "WrongPass", t0);
    assert(!wrong_pass.success);

    // US-01 Scenario 1: Valid login enters main screen
    LoginResult valid_login = auth.login("bqt_user", "CorrectPass@123", t0);
    assert(valid_login.success);
    assert(valid_login.session_token.has_value());
    std::string bqt_token = valid_login.session_token.value();

    // US-01 Scenario 4: Role-based access control (ThuQuy cannot add/edit/delete residents)
    LoginResult cashier_login = auth.login("cashier_user", "CashierPass@123", t0);
    assert(cashier_login.success);
    std::string cashier_token = cashier_login.session_token.value();

    Result bqt_create_hh = hh_service.createHousehold(
        bqt_token,
        HouseholdInput{"1001", "10", "80.0", "Pham Van A", "0909000111", "2025-01-01"},
        t0);
    assert(bqt_create_hh.success);

    Result cashier_add_res = hh_service.addResident(
        cashier_token,
        "1001",
        ResidentInput{"Pham Van A", "1980-01-01", "Nam", "Chu ho", "001080000001"},
        YmdDate{2026, 11, 26},
        t0);
    assert(!cashier_add_res.success);

    // US-01 Scenario 5: 5 consecutive failed attempts, 6th attempt locks account for 15 minutes
    auth.registerAccount("lock_test", "Secret@123", Role::BQT);
    for (int i = 1; i <= 5; ++i)
    {
        LoginResult fail_attempt = auth.login("lock_test", "BadPass", t0 + std::chrono::seconds(i));
        assert(!fail_attempt.success);
    }
    TimePoint lock_time = t0 + 10s;
    LoginResult sixth_attempt = auth.login("lock_test", "Secret@123", lock_time);
    assert(!sixth_attempt.success);
    assert(sixth_attempt.message.find("15 phut") != std::string::npos);

    // Still locked after 14 minutes
    LoginResult still_locked = auth.login("lock_test", "Secret@123", lock_time + 14min);
    assert(!still_locked.success);

    // Unlocked after 15 minutes
    LoginResult unlocked = auth.login("lock_test", "Secret@123", lock_time + 15min);
    assert(unlocked.success);

    // US-01 Scenario 7: Login audit logs record username, timestamp, and result
    const auto& logs = auth.getAuditLogs();
    assert(logs.size() >= 10);
    assert(logs.back().username == "lock_test");
    assert(logs.back().success);

    // US-01 Scenario 8: Session inactive for 30 minutes requires re-login
    Result active_at_20m = auth.validateSessionAndTouch(bqt_token, t0 + 20min);
    assert(active_at_20m.success);
    // 30 minutes of inactivity after the 20min touch -> expired at t0 + 50min
    Result expired_at_50m = auth.validateSessionAndTouch(bqt_token, t0 + 50min);
    assert(!expired_at_50m.success);

    std::cout << "[PASS] US-01: All 8 Gherkin scenarios passed.\n";
}

void testUs03Scenarios()
{
    AuthService auth;
    auth.registerAccount("bqt_admin", "Admin@123", Role::BQT);
    TimePoint t0 = std::chrono::system_clock::now();
    std::string token = auth.login("bqt_admin", "Admin@123", t0).session_token.value();

    HouseholdService hh_service(auth);

    // US-03 Scenario 1: Valid household creation appears in list
    Result s1 = hh_service.createHousehold(
        token,
        HouseholdInput{"1205", "12", "75.0", "Nguyen Van An", "0901234567", "2025-02-01"},
        t0);
    assert(s1.success);
    assert(hh_service.listHouseholds().size() == 1);

    // US-03 Scenario 2: Missing required field fails and does not save
    Result s2 = hh_service.createHousehold(
        token,
        HouseholdInput{"1206", "12", "75.0", "", "0901234567", "2025-02-01"},
        t0);
    assert(!s2.success);
    assert(hh_service.listHouseholds().size() == 1);

    // US-03 Scenario 3: Duplicate apartment code warns and fails
    Result s3 = hh_service.createHousehold(
        token,
        HouseholdInput{"1205", "12", "80.0", "Tran Van B", "0911222333", "2025-03-01"},
        t0);
    assert(!s3.success);

    // US-03 Scenario 4: Area <= 0 or non-numeric fails
    Result s4_zero = hh_service.createHousehold(
        token,
        HouseholdInput{"1207", "12", "0", "Tran Van B", "0911222333", "2025-03-01"},
        t0);
    assert(!s4_zero.success);
    Result s4_neg = hh_service.createHousehold(
        token,
        HouseholdInput{"1207", "12", "-45.5", "Tran Van B", "0911222333", "2025-03-01"},
        t0);
    assert(!s4_neg.success);
    Result s4_nan = hh_service.createHousehold(
        token,
        HouseholdInput{"1207", "12", "75m2", "Tran Van B", "0911222333", "2025-03-01"},
        t0);
    assert(!s4_nan.success);

    // US-03 Scenario 5: Floor outside 06-30 or bad 4-digit apartment code fails
    Result s5_low_floor = hh_service.createHousehold(
        token,
        HouseholdInput{"0501", "05", "70.0", "Le Van C", "0922333444", "2025-03-01"},
        t0);
    assert(!s5_low_floor.success);
    Result s5_high_floor = hh_service.createHousehold(
        token,
        HouseholdInput{"3101", "31", "70.0", "Le Van C", "0922333444", "2025-03-01"},
        t0);
    assert(!s5_high_floor.success);
    Result s5_bad_fmt = hh_service.createHousehold(
        token,
        HouseholdInput{"A1205", "12", "70.0", "Le Van C", "0922333444", "2025-03-01"},
        t0);
    assert(!s5_bad_fmt.success);

    // US-03 Scenario 6 & 7: Issue fee notice before updating area; update logs history and only
    // affects subsequent fee notices
    Result inv1 = hh_service.issueFeeNotice(token, "1205", "2026-10", 6000.0, t0);
    assert(inv1.success);

    Result s6 = hh_service.updateHousehold(
        token,
        "1205",
        HouseholdInput{"1205", "12", "85.0", "Nguyen Van An", "0988777666", "2025-02-01"},
        t0 + 1min);
    assert(s6.success);

    auto history = hh_service.getChangeHistory("1205");
    assert(history.size() == 1);
    assert(std::abs(history[0].old_area_m2 - 75.0) < 1e-6);
    assert(std::abs(history[0].new_area_m2 - 85.0) < 1e-6);

    Result inv2 = hh_service.issueFeeNotice(token, "1205", "2026-11", 6000.0, t0 + 2min);
    assert(inv2.success);

    auto notices = hh_service.getFeeNotices("1205");
    assert(notices.size() == 2);
    // Old fee notice keeps 75.0 m2 (450,000 VND)
    assert(std::abs(notices[0].billed_area_m2 - 75.0) < 1e-6);
    assert(std::abs(notices[0].total_fee - 450000.0) < 1e-6);
    // New fee notice uses 85.0 m2 (510,000 VND)
    assert(std::abs(notices[1].billed_area_m2 - 85.0) < 1e-6);
    assert(std::abs(notices[1].total_fee - 510000.0) < 1e-6);

    std::cout << "[PASS] US-03: All 7 Gherkin scenarios passed.\n";
}

void testUs04Scenarios()
{
    AuthService auth;
    auth.registerAccount("bqt_admin", "Admin@123", Role::BQT);
    TimePoint t0 = std::chrono::system_clock::now();
    std::string token = auth.login("bqt_admin", "Admin@123", t0).session_token.value();

    HouseholdService hh_service(auth);
    hh_service.createHousehold(
        token,
        HouseholdInput{"1502", "15", "90.0", "Tran Thi Binh", "0912345678", "2025-01-10"},
        t0);

    YmdDate today{2026, 11, 26};

    // US-04 Scenario 1: Add valid adult resident with 12-digit CCCD
    Result s1 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Tran Thi Binh", "1985-05-20", "Nu", "Chu ho", "001185012345"},
        today,
        t0);
    assert(s1.success);

    // US-04 Scenario 2: Missing required field fails
    Result s2 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"", "1990-01-01", "Nam", "Em trai", "001090012345"},
        today,
        t0);
    assert(!s2.success);

    // US-04 Scenario 3: Duplicate CCCD fails
    Result s3 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chong", "001185012345"},
        today,
        t0);
    assert(!s3.success);

    // US-04 Scenario 4: CCCD not 12 digits or contains non-digit characters fails
    Result s4_short = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chong", "00118501"},
        today,
        t0);
    assert(!s4_short.success);
    Result s4_alpha = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chong", "00118501234A"},
        today,
        t0);
    assert(!s4_alpha.success);

    // US-04 Scenario 5: Resident under 14 years old with empty CCCD succeeds
    Result s5 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Bao Nam", "2018-08-12", "Nam", "Con", ""},
        today,
        t0);
    assert(s5.success);

    // Adult (>= 14) with empty CCCD fails
    Result s5_adult_empty = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chong", ""},
        today,
        t0);
    assert(!s5_adult_empty.success);

    // US-04 Scenario 6: Adding a second head of household ("Chu ho") fails
    Result s6 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chu ho", "001084098765"},
        today,
        t0);
    assert(!s6.success);

    // US-04 Scenario 7: Birth date in the future fails
    Result s7 = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Tuong Lai", "2027-01-01", "Nam", "Con", ""},
        today,
        t0);
    assert(!s7.success);

    // Add valid spouse so household has 3 residents
    Result add_spouse = hh_service.addResident(
        token,
        "1502",
        ResidentInput{"Nguyen Van X", "1984-03-15", "Nam", "Chong", "001084098765"},
        today,
        t0);
    assert(add_spouse.success);

    // US-04 Scenario 8: Viewing household details displays all residents of the household
    auto details = hh_service.getHouseholdDetails("1502");
    assert(details.has_value());
    assert(details->residents.size() == 3);
    assert(details->residents[0].full_name == "Tran Thi Binh");
    assert(details->residents[1].full_name == "Nguyen Bao Nam");
    assert(details->residents[2].full_name == "Nguyen Van X");

    std::cout << "[PASS] US-04: All 8 Gherkin scenarios passed.\n";
}

} // namespace

int main()
{
    testUs01Scenarios();
    testUs03Scenarios();
    testUs04Scenarios();
    std::cout << "=== ALL 23 GHERKIN SCENARIOS (US-01, US-03, US-04) PASSED ===\n";
    return 0;
}
