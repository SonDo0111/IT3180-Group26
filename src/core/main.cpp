#include "core/auth.hpp"
#include "core/types.hpp"
#include "modules/household/household.hpp"

#include <iostream>

int main()
{
    using namespace it3180;

    AuthService auth;
    auth.registerAccount("bqt_admin", "Admin@123", Role::BQT);
    auth.registerAccount("thu_quy", "Cashier@123", Role::ThuQuy);

    HouseholdService household_service(auth);

    std::cout << "=== IT3180 Group 26 - Apartment Management System ===\n";

    LoginResult login_res = auth.login("bqt_admin", "Admin@123");
    std::cout << "[US-01] Login BQT: " << login_res.message << "\n";
    if (!login_res.success || !login_res.session_token.has_value())
    {
        return 1;
    }
    std::string token = login_res.session_token.value();

    Result hh_res = household_service.createHousehold(
        token,
        HouseholdInput{"1205", "12", "75.5", "Nguyen Van An", "0901234567", "2025-01-15"});
    std::cout << "[US-03] Create Household 1205: " << hh_res.message << "\n";

    Result res1 = household_service.addResident(
        token,
        "1205",
        ResidentInput{"Nguyen Van An", "1985-04-10", "Nam", "Chu ho", "001085012345"});
    std::cout << "[US-04] Add Head Resident: " << res1.message << "\n";

    Result res2 = household_service.addResident(
        token,
        "1205",
        ResidentInput{"Nguyen Minh Khang", "2016-06-01", "Nam", "Con", ""});
    std::cout << "[US-04] Add Under-14 Resident (no CCCD): " << res2.message << "\n";

    auto details = household_service.getHouseholdDetails("1205");
    if (details.has_value())
    {
        std::cout << "Household " << details->apartment_code << " (" << details->area_m2
                  << " m2) has " << details->residents.size() << " residents.\n";
    }

    return 0;
}
