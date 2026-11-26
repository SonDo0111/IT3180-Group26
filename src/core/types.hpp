#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace it3180
{

using TimePoint = std::chrono::system_clock::time_point;

enum class Role
{
    BQT,
    ThuQuy
};

enum class Permission
{
    ManageHousehold,
    ManageResident,
    ViewHousehold,
    ManageFee,
    ManagePayment,
    ViewReport
};

struct Result
{
    bool success{false};
    std::string message{};

    static Result ok(std::string msg = "Thanh cong")
    {
        return Result{true, std::move(msg)};
    }

    static Result fail(std::string msg)
    {
        return Result{false, std::move(msg)};
    }
};

struct YmdDate
{
    int year{0};
    int month{0};
    int day{0};

    bool operator==(const YmdDate& other) const
    {
        return year == other.year && month == other.month && day == other.day;
    }

    bool operator<(const YmdDate& other) const
    {
        if (year != other.year)
        {
            return year < other.year;
        }
        if (month != other.month)
        {
            return month < other.month;
        }
        return day < other.day;
    }

    bool operator>(const YmdDate& other) const
    {
        return other < *this;
    }
};

std::string trimCopy(std::string_view input);
bool isAllDigits(std::string_view input);
std::optional<double> parseDoubleStrict(std::string_view input);
std::optional<YmdDate> parseIsoDate(std::string_view iso_date);
int calculateAgeYears(const YmdDate& birth_date, const YmdDate& current_date);
std::string formatTimePoint(TimePoint tp);
bool hasPermission(Role role, Permission permission);
std::string roleToString(Role role);

} // namespace it3180
