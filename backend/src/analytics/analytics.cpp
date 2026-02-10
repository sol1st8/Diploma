#include "analytics.h"
#include <algorithm>
#include <numeric>
#include <map>

namespace analytics {

std::vector<TrunkAnalytics> AnalyticsCalculator::CalculateByTrunk(
    const std::vector<ui::detail::CallStatisticsInfo>& calls,
    const std::vector<ui::detail::TrunkInfo>& trunks
) {
    std::map<int, TrunkAnalytics> trunk_map;

    // Инициализируем данные для каждого транка
    for (const auto& trunk : trunks) {
        trunk_map[trunk.id] = TrunkAnalytics{
            trunk.id,
            trunk.name,
            0, 0.0, 0, 0.0, 0.0
        };
    }

    // Агрегируем данные по звонкам
    for (const auto& call : calls) {
        if (trunk_map.find(call.trunk_id) != trunk_map.end()) {
            auto& analytics = trunk_map[call.trunk_id];
            analytics.total_calls++;
            analytics.total_revenue += call.cost;
            analytics.total_duration_seconds += call.duration_seconds;
        }
    }

    // Рассчитываем средние значения
    std::vector<TrunkAnalytics> result;
    for (auto& [id, analytics] : trunk_map) {
        if (analytics.total_calls > 0) {
            analytics.avg_duration_seconds = 
                static_cast<double>(analytics.total_duration_seconds) / analytics.total_calls;
            analytics.avg_cost = analytics.total_revenue / analytics.total_calls;
        }
        result.push_back(analytics);
    }

    // Сортируем по выручке (убывание)
    std::sort(result.begin(), result.end(),
        [](const TrunkAnalytics& a, const TrunkAnalytics& b) {
            return a.total_revenue > b.total_revenue;
        });

    return result;
}

std::vector<TarifAnalytics> AnalyticsCalculator::CalculateByTarif(
    const std::vector<ui::detail::CallStatisticsInfo>& calls,
    const std::vector<ui::detail::TarifInfo>& tarifs
) {
    std::map<int, TarifAnalytics> tarif_map;

    // Инициализируем данные для каждого тарифа
    for (const auto& tarif : tarifs) {
        tarif_map[tarif.id] = TarifAnalytics{
            tarif.id,
            tarif.name,
            0, 0.0, 0, 0.0
        };
    }

    // Агрегируем данные по звонкам
    for (const auto& call : calls) {
        if (tarif_map.find(call.tarif_id) != tarif_map.end()) {
            auto& analytics = tarif_map[call.tarif_id];
            analytics.total_calls++;
            analytics.total_revenue += call.cost;
            analytics.total_duration_seconds += call.duration_seconds;
        }
    }

    // Рассчитываем средние значения
    std::vector<TarifAnalytics> result;
    for (auto& [id, analytics] : tarif_map) {
        if (analytics.total_calls > 0) {
            analytics.avg_cost = analytics.total_revenue / analytics.total_calls;
        }
        result.push_back(analytics);
    }

    // Сортируем по выручке (убывание)
    std::sort(result.begin(), result.end(),
        [](const TarifAnalytics& a, const TarifAnalytics& b) {
            return a.total_revenue > b.total_revenue;
        });

    return result;
}

std::vector<HubAnalytics> AnalyticsCalculator::CalculateByHub(
    const std::vector<ui::detail::CallStatisticsInfo>& calls,
    const std::vector<ui::detail::HubInfo>& hubs,
    const std::vector<ui::detail::ServerInfo>& servers,
    const std::vector<ui::detail::TrunkInfo>& trunks
) {
    std::map<int, HubAnalytics> hub_map;

    // Инициализируем данные для каждого хаба
    for (const auto& hub : hubs) {
        hub_map[hub.id] = HubAnalytics{
            hub.id,
            hub.name,
            0, 0.0, 0, 0
        };
    }

    // Подсчитываем серверы и транки для каждого хаба
    for (const auto& server : servers) {
        if (hub_map.find(server.hub_id) != hub_map.end()) {
            hub_map[server.hub_id].server_count++;
            
            // Подсчитываем транки для этого сервера
            for (const auto& trunk : trunks) {
                if (trunk.server_id == server.id) {
                    hub_map[server.hub_id].trunk_count++;
                }
            }
        }
    }

    // Создаем маппинг trunk_id -> hub_id
    std::map<int, int> trunk_to_hub;
    for (const auto& trunk : trunks) {
        for (const auto& server : servers) {
            if (trunk.server_id == server.id) {
                trunk_to_hub[trunk.id] = server.hub_id;
                break;
            }
        }
    }

    // Агрегируем данные по звонкам
    for (const auto& call : calls) {
        auto it = trunk_to_hub.find(call.trunk_id);
        if (it != trunk_to_hub.end()) {
            int hub_id = it->second;
            if (hub_map.find(hub_id) != hub_map.end()) {
                auto& analytics = hub_map[hub_id];
                analytics.total_calls++;
                analytics.total_revenue += call.cost;
            }
        }
    }

    // Собираем результат
    std::vector<HubAnalytics> result;
    for (const auto& [id, analytics] : hub_map) {
        result.push_back(analytics);
    }

    // Сортируем по выручке (убывание)
    std::sort(result.begin(), result.end(),
        [](const HubAnalytics& a, const HubAnalytics& b) {
            return a.total_revenue > b.total_revenue;
        });

    return result;
}

std::vector<RevenueAnalytics> AnalyticsCalculator::CalculateRevenueByHour(
    const std::vector<ui::detail::CallStatisticsInfo>& calls
) {
    std::map<std::string, RevenueAnalytics> hour_map;

    for (const auto& call : calls) {
        // Извлекаем час из call_time (формат: "YYYY-MM-DD HH:MM:SS")
        if (call.call_time.length() >= 13) {
            std::string hour = call.call_time.substr(11, 2);
            
            if (hour_map.find(hour) == hour_map.end()) {
                hour_map[hour] = RevenueAnalytics{hour + ":00", 0.0, 0};
            }
            
            hour_map[hour].revenue += call.cost;
            hour_map[hour].call_count++;
        }
    }

    std::vector<RevenueAnalytics> result;
    for (const auto& [hour, analytics] : hour_map) {
        result.push_back(analytics);
    }

    // Сортируем по часу
    std::sort(result.begin(), result.end(),
        [](const RevenueAnalytics& a, const RevenueAnalytics& b) {
            return a.period < b.period;
        });

    return result;
}

std::vector<RevenueAnalytics> AnalyticsCalculator::CalculateRevenueByDay(
    const std::vector<ui::detail::CallStatisticsInfo>& calls
) {
    std::map<std::string, RevenueAnalytics> day_map;

    for (const auto& call : calls) {
        // Извлекаем дату из call_time (формат: "YYYY-MM-DD HH:MM:SS")
        if (call.call_time.length() >= 10) {
            std::string day = call.call_time.substr(0, 10);
            
            if (day_map.find(day) == day_map.end()) {
                day_map[day] = RevenueAnalytics{day, 0.0, 0};
            }
            
            day_map[day].revenue += call.cost;
            day_map[day].call_count++;
        }
    }

    std::vector<RevenueAnalytics> result;
    for (const auto& [day, analytics] : day_map) {
        result.push_back(analytics);
    }

    // Сортируем по дате
    std::sort(result.begin(), result.end(),
        [](const RevenueAnalytics& a, const RevenueAnalytics& b) {
            return a.period < b.period;
        });

    return result;
}

// Конвертация в JSON
json::value AnalyticsCalculator::ToJson(const std::vector<TrunkAnalytics>& data) {
    json::array arr;
    for (const auto& item : data) {
        arr.push_back(json::object{
            {"trunk_id"s, item.trunk_id},
            {"trunk_name"s, item.trunk_name},
            {"total_calls"s, item.total_calls},
            {"total_revenue"s, item.total_revenue},
            {"total_duration_seconds"s, item.total_duration_seconds},
            {"avg_duration_seconds"s, item.avg_duration_seconds},
            {"avg_cost"s, item.avg_cost}
        });
    }
    return arr;
}

json::value AnalyticsCalculator::ToJson(const std::vector<TarifAnalytics>& data) {
    json::array arr;
    for (const auto& item : data) {
        arr.push_back(json::object{
            {"tarif_id"s, item.tarif_id},
            {"tarif_name"s, item.tarif_name},
            {"total_calls"s, item.total_calls},
            {"total_revenue"s, item.total_revenue},
            {"total_duration_seconds"s, item.total_duration_seconds},
            {"avg_cost"s, item.avg_cost}
        });
    }
    return arr;
}

json::value AnalyticsCalculator::ToJson(const std::vector<HubAnalytics>& data) {
    json::array arr;
    for (const auto& item : data) {
        arr.push_back(json::object{
            {"hub_id"s, item.hub_id},
            {"hub_name"s, item.hub_name},
            {"total_calls"s, item.total_calls},
            {"total_revenue"s, item.total_revenue},
            {"server_count"s, item.server_count},
            {"trunk_count"s, item.trunk_count}
        });
    }
    return arr;
}

json::value AnalyticsCalculator::ToJson(const std::vector<RevenueAnalytics>& data) {
    json::array arr;
    for (const auto& item : data) {
        arr.push_back(json::object{
            {"period"s, item.period},
            {"revenue"s, item.revenue},
            {"call_count"s, item.call_count}
        });
    }
    return arr;
}

} // namespace analytics
