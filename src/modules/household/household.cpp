#include "modules/household/household.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace it3180
{

namespace
{

std::string toLowerAscii(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for (char ch : s)
    {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

} // namespace

HouseholdService::HouseholdService(AuthService& auth_service) : auth_service_(auth_service)
{
}

bool HouseholdService::isHeadRelation(std::string_view relation)
{
    std::string norm = toLowerAscii(trimCopy(relation));
    return norm == "chu ho" || norm == "chủ hộ" || norm == "chuho";
}

Result HouseholdService::validateHouseholdInput(const HouseholdInput& input,
                                                int& out_floor,
                                                double& out_area)
{
    std::string apt = trimCopy(input.apartment_code);
    std::string flr_str = trimCopy(input.floor_input);
    std::string area_str = trimCopy(input.area_input);
    std::string head = trimCopy(input.head_name);
    std::string phone = trimCopy(input.phone_number);
    std::string move_in = trimCopy(input.move_in_date);

    // US-03 Scenario 2: Missing required fields
    if (apt.empty() || flr_str.empty() || area_str.empty() || head.empty() || phone.empty() ||
        move_in.empty())
    {
        return Result::fail("Vui long nhap day du cac truong bat buoc cua ho gia dinh.");
    }

    // US-03 Scenario 5: Apartment code must be 4 digits (2-digit floor 06-30 + 2-digit unit > 00)
    if (apt.size() != 4 || !isAllDigits(apt))
    {
        return Result::fail(
            "So can ho sai dinh dang (phai gom 4 chu so: 2 chu so tang va 2 chu so thu tu, vi du "
            "1205).");
    }

    int apt_floor = std::stoi(apt.substr(0, 2));
    int apt_unit = std::stoi(apt.substr(2, 2));
    if (apt_floor < 6 || apt_floor > 30 || apt_unit <= 0)
    {
        return Result::fail("Tang can ho phai nam trong khoang 06-30 va so thu tu can ho hop le.");
    }

    if (!isAllDigits(flr_str))
    {
        return Result::fail("Tang phai la so nguyen hop le.");
    }
    out_floor = std::stoi(flr_str);
    if (out_floor < 6 || out_floor > 30 || out_floor != apt_floor)
    {
        return Result::fail(
            "Tang nhap vao phai nam trong khoang 06-30 va khop voi 2 chu so dau cua so can ho.");
    }

    // US-03 Scenario 4: Area must be a valid number > 0
    auto parsed_area = parseDoubleStrict(area_str);
    if (!parsed_area.has_value() || parsed_area.value() <= 0.0)
    {
        return Result::fail("Dien tich can ho phai la so lon hon 0.");
    }
    out_area = parsed_area.value();

    if (!parseIsoDate(move_in).has_value())
    {
        return Result::fail("Ngay bat dau cu tru khong dung dinh dang YYYY-MM-DD.");
    }

    return Result::ok();
}

Result HouseholdService::createHousehold(std::string_view session_token,
                                         const HouseholdInput& input,
                                         TimePoint now)
{
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManageHousehold, now);
    if (!auth.success)
    {
        return auth;
    }

    int floor_val = 0;
    double area_val = 0.0;
    Result valid = validateHouseholdInput(input, floor_val, area_val);
    if (!valid.success)
    {
        return valid;
    }

    std::string apt = trimCopy(input.apartment_code);

    // US-03 Scenario 3: Duplicate apartment code warning
    if (households_.find(apt) != households_.end())
    {
        return Result::fail("Canh bao trung: So can ho " + apt + " da ton tai trong he thong.");
    }

    Household hh{};
    hh.apartment_code = apt;
    hh.floor = floor_val;
    hh.area_m2 = area_val;
    hh.head_name = trimCopy(input.head_name);
    hh.phone_number = trimCopy(input.phone_number);
    hh.move_in_date = trimCopy(input.move_in_date);

    households_.emplace(apt, std::move(hh));
    insertion_order_.push_back(apt);

    return Result::ok("Them ho so ho gia dinh thanh cong.");
}

Result HouseholdService::updateHousehold(std::string_view session_token,
                                         std::string_view apartment_code,
                                         const HouseholdInput& updated_input,
                                         TimePoint now)
{
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManageHousehold, now);
    if (!auth.success)
    {
        return auth;
    }

    std::string target_apt = trimCopy(apartment_code);
    auto it = households_.find(target_apt);
    if (it == households_.end())
    {
        return Result::fail("Khong tim thay ho gia dinh voi so can ho " + target_apt + ".");
    }

    int floor_val = 0;
    double area_val = 0.0;
    Result valid = validateHouseholdInput(updated_input, floor_val, area_val);
    if (!valid.success)
    {
        return valid;
    }

    std::string new_apt = trimCopy(updated_input.apartment_code);
    if (new_apt != target_apt && households_.find(new_apt) != households_.end())
    {
        return Result::fail("Canh bao trung: So can ho " + new_apt + " da ton tai trong he thong.");
    }

    Household& hh = it->second;
    double old_area = hh.area_m2;
    std::string old_head = hh.head_name;
    std::string old_phone = hh.phone_number;

    hh.floor = floor_val;
    hh.area_m2 = area_val;
    hh.head_name = trimCopy(updated_input.head_name);
    hh.phone_number = trimCopy(updated_input.phone_number);
    hh.move_in_date = trimCopy(updated_input.move_in_date);

    std::string actor = "system";
    auto sess = auth_service_.getSession(session_token);
    if (sess.has_value())
    {
        actor = sess->username;
    }

    // US-03 Scenario 6: Save change history log
    std::ostringstream summary;
    summary << "Cap nhat ho " << target_apt << ": chu ho '" << old_head << "' -> '" << hh.head_name
            << "', SDT '" << old_phone << "' -> '" << hh.phone_number << "', dien tich " << old_area
            << "m2 -> " << hh.area_m2 << "m2";

    HouseholdChangeLog log_entry{};
    log_entry.apartment_code = target_apt;
    log_entry.changed_at = now;
    log_entry.changed_by = actor;
    log_entry.summary = summary.str();
    log_entry.old_area_m2 = old_area;
    log_entry.new_area_m2 = hh.area_m2;
    change_logs_.push_back(std::move(log_entry));

    return Result::ok("Cap nhat thong tin ho gia dinh va luu lich su thay doi thanh cong.");
}

Result HouseholdService::issueFeeNotice(std::string_view session_token,
                                        std::string_view apartment_code,
                                        std::string_view billing_period,
                                        double rate_per_m2,
                                        TimePoint now)
{
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManagePayment, now);
    if (!auth.success)
    {
        return auth;
    }

    std::string apt = trimCopy(apartment_code);
    auto it = households_.find(apt);
    if (it == households_.end())
    {
        return Result::fail("Khong tim thay ho gia dinh.");
    }

    // US-03 Scenario 7: Snapshot the household's current area_m2 at notice creation time
    FeeNoticeSnapshot notice{};
    notice.notice_id = "INV-" + std::to_string(next_notice_id_++);
    notice.apartment_code = apt;
    notice.billing_period = trimCopy(billing_period);
    notice.billed_area_m2 = it->second.area_m2;
    notice.rate_per_m2 = rate_per_m2;
    notice.total_fee = notice.billed_area_m2 * rate_per_m2;
    fee_notices_.push_back(std::move(notice));

    return Result::ok("Lap thong bao phi thanh cong.");
}

Result HouseholdService::addResident(std::string_view session_token,
                                     std::string_view apartment_code,
                                     const ResidentInput& input,
                                     const YmdDate& current_date,
                                     TimePoint now)
{
    // US-01 Scenario 4: Only authorized roles (BQT) can manage residents
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManageResident, now);
    if (!auth.success)
    {
        return auth;
    }

    std::string apt = trimCopy(apartment_code);
    auto it = households_.find(apt);
    if (it == households_.end())
    {
        return Result::fail("Ho gia dinh khong ton tai.");
    }

    std::string name = trimCopy(input.full_name);
    std::string bdate_str = trimCopy(input.birth_date);
    std::string gender = trimCopy(input.gender);
    std::string relation = trimCopy(input.relation_to_head);
    std::string cccd = trimCopy(input.cccd);

    // US-04 Scenario 2: Required fields check
    if (name.empty() || bdate_str.empty() || gender.empty() || relation.empty())
    {
        return Result::fail(
            "Vui long nhap day du thong tin nhan khau bat buoc (ho ten, ngay sinh, gioi tinh, quan "
            "he voi chu ho).");
    }

    auto parsed_bdate = parseIsoDate(bdate_str);
    if (!parsed_bdate.has_value())
    {
        return Result::fail("Ngay sinh khong hop le (dinh dang YYYY-MM-DD).");
    }

    // US-04 Scenario 7: Birth date cannot be in the future
    if (parsed_bdate.value() > current_date)
    {
        return Result::fail("Ngay sinh khong duoc nam sau ngay hien tai.");
    }

    int age = calculateAgeYears(parsed_bdate.value(), current_date);

    // US-04 Scenario 1, 2, 4, 5: CCCD validation (under 14 may leave empty; >= 14 requires 12 digits)
    if (cccd.empty())
    {
        if (age >= 14)
        {
            return Result::fail("Nhan khau tu 14 tuoi tro len bat buoc phai nhap CCCD 12 chu so.");
        }
    }
    else
    {
        if (cccd.size() != 12 || !isAllDigits(cccd))
        {
            return Result::fail("CCCD khong hop le: Phai gom dung 12 chu so.");
        }
        // US-04 Scenario 3: Duplicate CCCD check across all residents
        if (cccd_index_.find(cccd) != cccd_index_.end())
        {
            return Result::fail("Canh bao trung: So dinh danh (CCCD) " + cccd + " da ton tai.");
        }
    }

    Household& hh = it->second;

    // US-04 Scenario 6: Only one head of household ("Chu ho") per household
    if (isHeadRelation(relation))
    {
        for (const Resident& existing : hh.residents)
        {
            if (isHeadRelation(existing.relation_to_head))
            {
                return Result::fail("Canh bao: Moi ho gia dinh chi co mot Chu ho.");
            }
        }
    }

    Resident res{};
    res.resident_id = next_resident_id_++;
    res.apartment_code = apt;
    res.full_name = name;
    res.birth_date = bdate_str;
    res.gender = gender;
    res.relation_to_head = relation;
    res.cccd = cccd;

    if (!cccd.empty())
    {
        cccd_index_[cccd] = res.resident_id;
    }
    hh.residents.push_back(std::move(res));

    return Result::ok("Them nhan khau vao ho thanh cong.");
}

Result HouseholdService::updateResident(std::string_view session_token,
                                        std::uint64_t resident_id,
                                        const ResidentInput& updated_input,
                                        const YmdDate& current_date,
                                        TimePoint now)
{
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManageResident, now);
    if (!auth.success)
    {
        return auth;
    }

    std::string name = trimCopy(updated_input.full_name);
    std::string bdate_str = trimCopy(updated_input.birth_date);
    std::string gender = trimCopy(updated_input.gender);
    std::string relation = trimCopy(updated_input.relation_to_head);
    std::string cccd = trimCopy(updated_input.cccd);

    if (name.empty() || bdate_str.empty() || gender.empty() || relation.empty())
    {
        return Result::fail("Vui long nhap day du thong tin nhan khau bat buoc.");
    }

    auto parsed_bdate = parseIsoDate(bdate_str);
    if (!parsed_bdate.has_value() || parsed_bdate.value() > current_date)
    {
        return Result::fail("Ngay sinh khong hop le hoac nam sau ngay hien tai.");
    }

    int age = calculateAgeYears(parsed_bdate.value(), current_date);
    if (cccd.empty())
    {
        if (age >= 14)
        {
            return Result::fail("Nhan khau tu 14 tuoi tro len bat buoc phai nhap CCCD 12 chu so.");
        }
    }
    else
    {
        if (cccd.size() != 12 || !isAllDigits(cccd))
        {
            return Result::fail("CCCD khong hop le: Phai gom dung 12 chu so.");
        }
        auto idx_it = cccd_index_.find(cccd);
        if (idx_it != cccd_index_.end() && idx_it->second != resident_id)
        {
            return Result::fail("Canh bao trung: So dinh danh (CCCD) " + cccd + " da ton tai.");
        }
    }

    for (auto& [apt_code, hh] : households_)
    {
        for (Resident& res : hh.residents)
        {
            if (res.resident_id == resident_id)
            {
                if (isHeadRelation(relation))
                {
                    for (const Resident& other : hh.residents)
                    {
                        if (other.resident_id != resident_id &&
                            isHeadRelation(other.relation_to_head))
                        {
                            return Result::fail("Canh bao: Moi ho gia dinh chi co mot Chu ho.");
                        }
                    }
                }

                if (!res.cccd.empty())
                {
                    cccd_index_.erase(res.cccd);
                }
                if (!cccd.empty())
                {
                    cccd_index_[cccd] = resident_id;
                }

                res.full_name = name;
                res.birth_date = bdate_str;
                res.gender = gender;
                res.relation_to_head = relation;
                res.cccd = cccd;
                return Result::ok("Cap nhat nhan khau thanh cong.");
            }
        }
    }

    return Result::fail("Khong tim thay nhan khau.");
}

Result HouseholdService::removeResident(std::string_view session_token,
                                        std::uint64_t resident_id,
                                        TimePoint now)
{
    Result auth = auth_service_.authorizeAction(session_token, Permission::ManageResident, now);
    if (!auth.success)
    {
        return auth;
    }

    for (auto& [apt_code, hh] : households_)
    {
        for (auto it = hh.residents.begin(); it != hh.residents.end(); ++it)
        {
            if (it->resident_id == resident_id)
            {
                if (!it->cccd.empty())
                {
                    cccd_index_.erase(it->cccd);
                }
                hh.residents.erase(it);
                return Result::ok("Xoa nhan khau thanh cong.");
            }
        }
    }

    return Result::fail("Khong tim thay nhan khau.");
}

std::optional<Household> HouseholdService::getHouseholdDetails(std::string_view apartment_code) const
{
    auto it = households_.find(trimCopy(apartment_code));
    if (it == households_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

std::vector<Household> HouseholdService::listHouseholds() const
{
    std::vector<Household> result;
    result.reserve(insertion_order_.size());
    for (const std::string& apt : insertion_order_)
    {
        auto it = households_.find(apt);
        if (it != households_.end())
        {
            result.push_back(it->second);
        }
    }
    return result;
}

std::vector<HouseholdChangeLog>
HouseholdService::getChangeHistory(std::string_view apartment_code) const
{
    std::string apt = trimCopy(apartment_code);
    std::vector<HouseholdChangeLog> result;
    for (const HouseholdChangeLog& entry : change_logs_)
    {
        if (entry.apartment_code == apt)
        {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<FeeNoticeSnapshot> HouseholdService::getFeeNotices(std::string_view apartment_code) const
{
    std::string apt = trimCopy(apartment_code);
    std::vector<FeeNoticeSnapshot> result;
    for (const FeeNoticeSnapshot& notice : fee_notices_)
    {
        if (notice.apartment_code == apt)
        {
            result.push_back(notice);
        }
    }
    return result;
}

} // namespace it3180
