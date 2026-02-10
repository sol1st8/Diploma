#pragma once

#include "../ui/view.h"
#include <boost/json.hpp>
#include <vector>
#include <map>
#include <string>

namespace analytics {

namespace json = boost::json;
using namespace std::literals;

// Структура для аналитики по транкам
struct TrunkAnalytics {
    int trunk_id;
    std::string trunk_name;
    int total_calls;
    double total_revenue;
    int total_duration_seconds;
    double avg_duration_seconds;
    double avg_cost;
};

// Структура для аналитики по тарифам
struct TarifAnalytics {
    int tarif_id;
    std::string tarif_name;
    int total_calls;
    double total_revenue;
    int total_duration_seconds;
    double avg_cost;
};

// Структура для аналитики по хабам
struct HubAnalytics {
    int hub_id;
    std::string hub_name;
    int total_calls;
    double total_revenue;
    int server_count;
    int trunk_count;
};

// Структура для аналитики выручки по периодам
struct RevenueAnalytics {
    std::string period; // hour, day, month
    double revenue;
    int call_count;
};

class AnalyticsCalculator {
public:
    // Аналитика по транкам
    static std::vector<TrunkAnalytics> CalculateByTrunk(
        const std::vector<ui::detail::CallStatisticsInfo>& calls,
        const std::vector<ui::detail::TrunkInfo>& trunks
    );

    // Аналитика по тарифам
    static std::vector<TarifAnalytics> CalculateByTarif(
        const std::vector<ui::detail::CallStatisticsInfo>& calls,
        const std::vector<ui::detail::TarifInfo>& tarifs
    );

    // Аналитика по хабам
    static std::vector<HubAnalytics> CalculateByHub(
        const std::vector<ui::detail::CallStatisticsInfo>& calls,
        const std::vector<ui::detail::HubInfo>& hubs,
        const std::vector<ui::detail::ServerInfo>& servers,
        const std::vector<ui::detail::TrunkInfo>& trunks
    );

    // Аналитика выручки по часам
    static std::vector<RevenueAnalytics> CalculateRevenueByHour(
        const std::vector<ui::detail::CallStatisticsInfo>& calls
    );

    // Аналитика выручки по дням
    static std::vector<RevenueAnalytics> CalculateRevenueByDay(
        const std::vector<ui::detail::CallStatisticsInfo>& calls
    );

    // Конвертация в JSON
    static json::value ToJson(const std::vector<TrunkAnalytics>& data);
    static json::value ToJson(const std::vector<TarifAnalytics>& data);
    static json::value ToJson(const std::vector<HubAnalytics>& data);
    static json::value ToJson(const std::vector<RevenueAnalytics>& data);
};

} // namespace analytics
