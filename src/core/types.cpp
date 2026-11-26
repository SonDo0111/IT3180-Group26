#include "core/types.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace it3180
{

std::string trimCopy(std::string_view input)
{
    std::size_t first = 0;
    while (first < input.size() && std::isspace(static_cast<unsigned char>(input[first])))
    {
        ++first;
    }
    std::size_t last = input.size();
    while (last > first && std::isspace(static_cast<unsigned char>(input[last - 1])))
    {
        --last;
    }
    return std::string(input.substr(first, last - first));
}

bool isAllDigits(std::string_view input)
{
    if (input.empty())
    {
        return false;
    }
    for (char ch : input)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
        {
            return false;
        }
    }
    return true;
}

std::optional<double> parseDoubleStrict(std::string_view input)
{
    std::string trimmed = trimCopy(input);
    if (trimmed.empty())
    {
        return std::nullopt;
    }
    std::size_t idx = 0;
    try
    {
        double val = std::stod(trimmed, &idx);
        if (idx != trimmed.size())
        {
            return std::nullopt;
        }
        return val;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

namespace
{

bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month)
{
    switch (month)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        case 2:
            return isLeapYear(year) ? 29 : 28;
        default:
            return 0;
    }
}

} // namespace

std::optional<YmdDate> parseIsoDate(std::string_view iso_date)
{
    std::string s = trimCopy(iso_date);
    if (s.size() != 10 || s[4] != '-' || s[7] != '-')
    {
        return std::nullopt;
    }
    std::string_view y_str(s.data(), 4);
    std::string_view m_str(s.data() + 5, 2);
    std::string_view d_str(s.data() + 8, 2);
    if (!isAllDigits(y_str) || !isAllDigits(m_str) || !isAllDigits(d_str))
    {
        return std::nullopt;
    }
    int year = std::stoi(std::string(y_str));
    int month = std::stoi(std::string(m_str));
    int day = std::stoi(std::string(d_str));
    if (year < 1900 || year > 2200 || month < 1 || month > 12)
    {
        return std::nullopt;
    }
    int max_day = daysInMonth(year, month);
    if (day < 1 || day > max_day)
    {
        return std::nullopt;
    }
    return YmdDate{year, month, day};
}

int calculateAgeYears(const YmdDate& birth_date, const YmdDate& current_date)
{
    int age = current_date.year - birth_date.year;
    if (current_date.month < birth_date.month ||
        (current_date.month == birth_date.month && current_date.day < birth_date.day))
    {
        --age;
    }
    return age;
}

std::string formatTimePoint(TimePoint tp)
{
    std::time_t raw_time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val{};
    gmtime_r(&raw_time, &tm_val);
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

bool hasPermission(Role role, Permission permission)
{
    if (role == Role::BQT)
    {
        return true;
    }
    switch (permission)
    {
        case Permission::ViewHousehold:
        case Permission::ManageFee:
        case Permission::ManagePayment:
        case Permission::ViewReport:
            return true;
        case Permission::ManageHousehold:
        case Permission::ManageResident:
            return false;
    }
    return false;
}

std::string roleToString(Role role)
{
    switch (role)
    {
        case Role::BQT:
            return "BQT";
        case Role::ThuQuy:
            return "ThuQuy";
    }
    return "Unknown";
}

} // namespace it3180
