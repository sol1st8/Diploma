#pragma once

#include "../ui/view.h"
#include <random>
#include <string>
#include <vector>
#include <chrono>

namespace call_simulator {

// Структура для представления маршрута звонка
struct CallRoute {
    int hub_id;
    int server_id;
    int trunk_id;
    std::string hub_name;
    std::string server_name;
    std::string trunk_name;
};

// Структура для генерируемого звонка
struct GeneratedCall {
    std::string call_id;
    int trunk_id;
    int tarif_id;
    int duration_seconds;
    double cost;
    std::string call_time;
    CallRoute route;
};

// Класс для расчета стоимости звонка
class CallCostCalculator {
public:
    // Расчет стоимости звонка по формуле из DB.md
    static double CalculateCost(
        int duration_seconds,
        double rate_per_minute,
        double markup_percent,
        double cost_per_channel
    );
    
    // Расчет с учетом бесплатных минут
    static double CalculateCostWithFreeMinutes(
        int duration_seconds,
        double rate_per_minute,
        double markup_percent,
        double cost_per_channel,
        int free_minutes
    );
};

// Генератор звонков
class CallGenerator {
public:
    CallGenerator();
    
    // Генерация одного звонка
    GeneratedCall GenerateCall(
        const std::vector<ui::detail::HubInfo>& hubs,
        const std::vector<ui::detail::ServerInfo>& servers,
        const std::vector<ui::detail::TrunkInfo>& trunks,
        const std::vector<ui::detail::TarifInfo>& tarifs,
        const std::vector<ui::detail::PricelistInfo>& pricelists
    );
    
    // Генерация пакета звонков
    std::vector<GeneratedCall> GenerateBatch(
        int count,
        const std::vector<ui::detail::HubInfo>& hubs,
        const std::vector<ui::detail::ServerInfo>& servers,
        const std::vector<ui::detail::TrunkInfo>& trunks,
        const std::vector<ui::detail::TarifInfo>& tarifs,
        const std::vector<ui::detail::PricelistInfo>& pricelists
    );
    
    // Построение маршрута звонка (hub -> server -> trunk)
    CallRoute BuildRoute(
        const std::vector<ui::detail::HubInfo>& hubs,
        const std::vector<ui::detail::ServerInfo>& servers,
        const std::vector<ui::detail::TrunkInfo>& trunks
    );

private:
    std::random_device rd_;
    std::mt19937 gen_;
    
    // Генерация уникального ID звонка
    std::string GenerateCallId();
    
    // Генерация случайной длительности звонка (30-600 секунд)
    int GenerateDuration();
    
    // Получение текущего времени в формате ISO 8601
    std::string GetCurrentTimestamp();
    
    // Поиск прайс-листа по ID
    const ui::detail::PricelistInfo* FindPricelist(
        int pricelist_id,
        const std::vector<ui::detail::PricelistInfo>& pricelists
    ) const;
};

} // namespace call_simulator
