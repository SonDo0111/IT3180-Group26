#pragma once

#include "core/auth.hpp"
#include "core/types.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace it3180
{

struct HouseholdInput
{
    std::string apartment_code{};
    std::string floor_input{};
    std::string area_input{};
    std::string head_name{};
    std::string phone_number{};
    std::string move_in_date{};
};

struct HouseholdChangeLog
{
    std::string apartment_code{};
    TimePoint changed_at{};
    std::string changed_by{};
    std::string summary{};
    double old_area_m2{0.0};
    double new_area_m2{0.0};
};

struct FeeNoticeSnapshot
{
    std::string notice_id{};
    std::string apartment_code{};
    std::string billing_period{};
    double billed_area_m2{0.0};
    double rate_per_m2{0.0};
    double total_fee{0.0};
};

struct ResidentInput
{
    std::string full_name{};
    std::string birth_date{};
    std::string gender{};
    std::string relation_to_head{};
    std::string cccd{};
};

struct Resident
{
    std::uint64_t resident_id{0};
    std::string apartment_code{};
    std::string full_name{};
    std::string birth_date{};
    std::string gender{};
    std::string relation_to_head{};
    std::string cccd{};
};

struct Household
{
    std::string apartment_code{};
    int floor{0};
    double area_m2{0.0};
    std::string head_name{};
    std::string phone_number{};
    std::string move_in_date{};
    std::vector<Resident> residents{};
};

class HouseholdService
{
public:
    explicit HouseholdService(AuthService& auth_service);

    // US-03: Create and update household profile
    Result createHousehold(std::string_view session_token,
                           const HouseholdInput& input,
                           TimePoint now = std::chrono::system_clock::now());

    Result updateHousehold(std::string_view session_token,
                           std::string_view apartment_code,
                           const HouseholdInput& updated_input,
                           TimePoint now = std::chrono::system_clock::now());

    // US-03 Scenario 7 helper: Issue a fee notice that snapshots current household area
    Result issueFeeNotice(std::string_view session_token,
                          std::string_view apartment_code,
                          std::string_view billing_period,
                          double rate_per_m2,
                          TimePoint now = std::chrono::system_clock::now());

    // US-04: Add and update residents within a household
    Result addResident(std::string_view session_token,
                       std::string_view apartment_code,
                       const ResidentInput& input,
                       const YmdDate& current_date = YmdDate{2026, 11, 26},
                       TimePoint now = std::chrono::system_clock::now());

    Result updateResident(std::string_view session_token,
                          std::uint64_t resident_id,
                          const ResidentInput& updated_input,
                          const YmdDate& current_date = YmdDate{2026, 11, 26},
                          TimePoint now = std::chrono::system_clock::now());

    Result removeResident(std::string_view session_token,
                          std::uint64_t resident_id,
                          TimePoint now = std::chrono::system_clock::now());

    std::optional<Household> getHouseholdDetails(std::string_view apartment_code) const;
    std::vector<Household> listHouseholds() const;
    std::vector<HouseholdChangeLog> getChangeHistory(std::string_view apartment_code) const;
    std::vector<FeeNoticeSnapshot> getFeeNotices(std::string_view apartment_code) const;

private:
    static Result validateHouseholdInput(const HouseholdInput& input,
                                         int& out_floor,
                                         double& out_area);
    static bool isHeadRelation(std::string_view relation);

    AuthService& auth_service_;
    std::unordered_map<std::string, Household> households_{};
    std::vector<std::string> insertion_order_{};
    std::vector<HouseholdChangeLog> change_logs_{};
    std::vector<FeeNoticeSnapshot> fee_notices_{};
    std::unordered_map<std::string, std::uint64_t> cccd_index_{};
    std::uint64_t next_resident_id_{1};
    std::uint64_t next_notice_id_{1};
};

} // namespace it3180
