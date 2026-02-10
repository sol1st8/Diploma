#include "call_generator.h"
#include "../config/dynamic_config.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

namespace call_simulator {

// ============================================================================
// CallCostCalculator Implementation
// ============================================================================

double CallCostCalculator::CalculateCost(
    int duration_seconds,
    double rate_per_minute,
    double markup_percent,
    double cost_per_channel
) {
    // Формула из DB.md:
    // 1. Базовая стоимость = (duration_seconds / 60) × rate_per_minute
    double base_cost = (duration_seconds / 60.0) * rate_per_minute;
    
    // 2. Наценка = базовая_стоимость × (markup_percent / 100)
    double markup = base_cost * (markup_percent / 100.0);
    
    // 3. Стоимость транка = cost_per_channel
    // 4. Итого = базовая_стоимость + наценка + стоимость_транка
    double total_cost = base_cost + markup + cost_per_channel;
    
    return total_cost;
}

double CallCostCalculator::CalculateCostWithFreeMinutes(
    int duration_seconds,
    double rate_per_minute,
    double markup_percent,
    double cost_per_channel,
    int free_minutes
) {
    // Вычитаем бесплатные минуты
    int billable_seconds = duration_seconds - (free_minutes * 60);
    
    if (billable_seconds <= 0) {
        // Звонок полностью покрывается бесплатными минутами
        // Но стоимость транка все равно взимается
        return cost_per_channel;
    }
    
    return CalculateCost(billable_seconds, rate_per_minute, markup_percent, cost_per_channel);
}

// ============================================================================
// CallGenerator Implementation
// ============================================================================

CallGenerator::CallGenerator() 
    : gen_(rd_()) {
}

std::string CallGenerator::GenerateCallId() {
    std::uniform_int_distribution<> dis(100000, 999999);
    std::stringstream ss;
    ss << "CALL-" << std::setfill('0') << std::setw(6) << dis(gen_);
    return ss.str();
}

int CallGenerator::GenerateDuration() {
    // Получаем текущую конфигурацию (потокобезопасно через atomic shared_ptr)
    auto cfg = config::g_config.Get();
    
    // Генерируем длительность используя параметры из конфигурации
    std::uniform_int_distribution<> dis(cfg->min_call_duration, cfg->max_call_duration);
    return dis(gen_);
}

std::string CallGenerator::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

const ui::detail::PricelistInfo* CallGenerator::FindPricelist(
    int pricelist_id,
    const std::vector<ui::detail::PricelistInfo>& pricelists
) const {
    auto it = std::find_if(pricelists.begin(), pricelists.end(),
        [pricelist_id](const ui::detail::PricelistInfo& pl) {
            return pl.id == pricelist_id;
        });
    
    return (it != pricelists.end()) ? &(*it) : nullptr;
}

CallRoute CallGenerator::BuildRoute(
    const std::vector<ui::detail::HubInfo>& hubs,
    const std::vector<ui::detail::ServerInfo>& servers,
    const std::vector<ui::detail::TrunkInfo>& trunks
) {
    CallRoute route;
    
    if (hubs.empty() || servers.empty() || trunks.empty()) {
        return route;
    }
    
    // Выбираем случайный хаб
    std::uniform_int_distribution<size_t> hub_dis(0, hubs.size() - 1);
    const auto& selected_hub = hubs[hub_dis(gen_)];
    route.hub_id = selected_hub.id;
    route.hub_name = selected_hub.name;
    
    // Находим серверы, принадлежащие выбранному хабу
    std::vector<ui::detail::ServerInfo> hub_servers;
    std::copy_if(servers.begin(), servers.end(), std::back_inserter(hub_servers),
        [&selected_hub](const ui::detail::ServerInfo& s) {
            return s.hub_id == selected_hub.id && s.is_active;
        });
    
    if (hub_servers.empty()) {
        // Если нет активных серверов в хабе, берем любой активный сервер
        std::vector<ui::detail::ServerInfo> active_servers;
        std::copy_if(servers.begin(), servers.end(), std::back_inserter(active_servers),
            [](const ui::detail::ServerInfo& s) { return s.is_active; });
        
        if (!active_servers.empty()) {
            std::uniform_int_distribution<size_t> srv_dis(0, active_servers.size() - 1);
            const auto& selected_server = active_servers[srv_dis(gen_)];
            route.server_id = selected_server.id;
            route.server_name = selected_server.name;
        } else {
            return route;
        }
    } else {
        // Выбираем случайный сервер из хаба
        std::uniform_int_distribution<size_t> srv_dis(0, hub_servers.size() - 1);
        const auto& selected_server = hub_servers[srv_dis(gen_)];
        route.server_id = selected_server.id;
        route.server_name = selected_server.name;
    }
    
    // Находим транки, принадлежащие выбранному серверу
    std::vector<ui::detail::TrunkInfo> server_trunks;
    std::copy_if(trunks.begin(), trunks.end(), std::back_inserter(server_trunks),
        [&route](const ui::detail::TrunkInfo& t) {
            return t.server_id == route.server_id;
        });
    
    if (server_trunks.empty()) {
        // Если нет транков на сервере, берем любой транк
        if (!trunks.empty()) {
            std::uniform_int_distribution<size_t> trunk_dis(0, trunks.size() - 1);
            const auto& selected_trunk = trunks[trunk_dis(gen_)];
            route.trunk_id = selected_trunk.id;
            route.trunk_name = selected_trunk.name;
        }
    } else {
        // Выбираем случайный транк с сервера
        std::uniform_int_distribution<size_t> trunk_dis(0, server_trunks.size() - 1);
        const auto& selected_trunk = server_trunks[trunk_dis(gen_)];
        route.trunk_id = selected_trunk.id;
        route.trunk_name = selected_trunk.name;
    }
    
    return route;
}

GeneratedCall CallGenerator::GenerateCall(
    const std::vector<ui::detail::HubInfo>& hubs,
    const std::vector<ui::detail::ServerInfo>& servers,
    const std::vector<ui::detail::TrunkInfo>& trunks,
    const std::vector<ui::detail::TarifInfo>& tarifs,
    const std::vector<ui::detail::PricelistInfo>& pricelists
) {
    GeneratedCall call;
    
    // Генерируем ID и время
    call.call_id = GenerateCallId();
    call.call_time = GetCurrentTimestamp();
    call.duration_seconds = GenerateDuration();
    
    // Строим маршрут (hub -> server -> trunk)
    call.route = BuildRoute(hubs, servers, trunks);
    call.trunk_id = call.route.trunk_id;
    
    // Выбираем случайный тариф
    if (!tarifs.empty()) {
        std::uniform_int_distribution<size_t> tarif_dis(0, tarifs.size() - 1);
        const auto& selected_tarif = tarifs[tarif_dis(gen_)];
        call.tarif_id = selected_tarif.id;
        
        // Находим прайс-лист для тарифа
        const auto* pricelist = FindPricelist(selected_tarif.pricelist_id, pricelists);
        
        if (pricelist && pricelist->is_active) {
            // Находим транк для получения cost_per_channel
            auto trunk_it = std::find_if(trunks.begin(), trunks.end(),
                [&call](const ui::detail::TrunkInfo& t) {
                    return t.id == call.trunk_id;
                });
            
            double cost_per_channel = (trunk_it != trunks.end()) 
                ? trunk_it->cost_per_channel 
                : 0.0;
            
            // Рассчитываем стоимость с учетом бесплатных минут
            call.cost = CallCostCalculator::CalculateCostWithFreeMinutes(
                call.duration_seconds,
                pricelist->rate_per_minute,
                selected_tarif.markup_percent,
                cost_per_channel,
                selected_tarif.free_minutes
            );
        } else {
            call.cost = 0.0;
        }
    } else {
        call.tarif_id = 0;
        call.cost = 0.0;
    }
    
    return call;
}

std::vector<GeneratedCall> CallGenerator::GenerateBatch(
    int count,
    const std::vector<ui::detail::HubInfo>& hubs,
    const std::vector<ui::detail::ServerInfo>& servers,
    const std::vector<ui::detail::TrunkInfo>& trunks,
    const std::vector<ui::detail::TarifInfo>& tarifs,
    const std::vector<ui::detail::PricelistInfo>& pricelists
) {
    std::vector<GeneratedCall> calls;
    calls.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        calls.push_back(GenerateCall(hubs, servers, trunks, tarifs, pricelists));
    }
    
    return calls;
}

} // namespace call_simulator
