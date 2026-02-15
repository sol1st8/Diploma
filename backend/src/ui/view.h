#pragma once

#include <boost/json.hpp>
#include <chrono>
#include <iosfwd>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace app {

class UseCases;

} // namespace app

namespace ui {

namespace detail {
namespace json = boost::json;
using namespace std::literals;

struct HubInfo {
    int id;
    std::string name;
    std::string location;
    bool is_active;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::HubInfo& hub) {
        jv = {
            {"id"s, hub.id},
            {"name"s, hub.name},
            {"location"s, hub.location},
            {"is_active"s, hub.is_active}
        };
    }

    friend ui::detail::HubInfo
    tag_invoke(json::value_to_tag<ui::detail::HubInfo>&, const json::value& hub) {
        ui::detail::HubInfo hub_info;

        hub_info.id = hub.at("id").as_int64();
        hub_info.name = hub.at("name").as_string();
        hub_info.location = hub.at("location").as_string();
        hub_info.is_active = hub.at("is_active").as_bool();

        return hub_info;
    }
};

struct ServerInfo {
    int id;
    int hub_id;
    std::string name;
    bool is_active;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::ServerInfo& server) {
        jv = {
            {"id"s, server.id},
            {"hub_id"s, server.hub_id},
            {"name"s, server.name},
            {"is_active"s, server.is_active}
        };
    }

    friend ui::detail::ServerInfo
    tag_invoke(json::value_to_tag<ui::detail::ServerInfo>&, const json::value& server) {
        ui::detail::ServerInfo serv_info;

        serv_info.id = server.at("id").as_int64();
        serv_info.hub_id = server.at("hub_id").as_int64();
        serv_info.name = server.at("name").as_string();
        serv_info.is_active = server.at("is_active").as_bool();

        return serv_info;
    }
};

struct NasIpInfo {
    int id;
    int server_id;
    std::string ip_address;
    std::string description;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::NasIpInfo& nas_ip) {
        jv = {
            {"id"s, nas_ip.id},
            {"server_id"s, nas_ip.server_id},
            {"ip_address"s, nas_ip.ip_address},
            {"description"s, nas_ip.description}
        };
    }

    friend ui::detail::NasIpInfo
    tag_invoke(json::value_to_tag<ui::detail::NasIpInfo>&, const json::value& nas_ip) {
        ui::detail::NasIpInfo nas_ip_info;

        nas_ip_info.id = nas_ip.at("id").as_int64();
        nas_ip_info.server_id = nas_ip.at("server_id").as_int64();
        nas_ip_info.ip_address = nas_ip.at("ip_address").as_string();
        nas_ip_info.description = nas_ip.at("description").as_string();

        return nas_ip_info;
    }
};

struct TrunkInfo {
    int id;
    int server_id;
    std::string name;
    int capacity;
    double cost_per_channel;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::TrunkInfo& trunk) {
        jv = {
            {"id"s, trunk.id},
            {"server_id"s, trunk.server_id},
            {"name"s, trunk.name},
            {"capacity"s, trunk.capacity},
            {"cost_per_channel"s, trunk.cost_per_channel}
        };
    }

    friend ui::detail::TrunkInfo
    tag_invoke(json::value_to_tag<ui::detail::TrunkInfo>&, const json::value& trunk) {
        ui::detail::TrunkInfo trunk_info;

        trunk_info.id = trunk.at("id").as_int64();
        trunk_info.server_id = trunk.at("server_id").as_int64();
        trunk_info.name = trunk.at("name").as_string();
        trunk_info.capacity = trunk.at("capacity").as_int64();
        trunk_info.cost_per_channel = trunk.at("cost_per_channel").as_double();

        return trunk_info;
    }
};

struct PricelistInfo {
    int id;
    std::string name;
    std::string currency;
    double rate_per_minute;
    bool is_active;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::PricelistInfo& pricelist) {
        jv = {
            {"id"s, pricelist.id},
            {"name"s, pricelist.name},
            {"currency"s, pricelist.currency},
            {"rate_per_minute"s, pricelist.rate_per_minute},
            {"is_active"s, pricelist.is_active}
        };
    }

    friend ui::detail::PricelistInfo
    tag_invoke(json::value_to_tag<ui::detail::PricelistInfo>&, const json::value& pricelist) {
        ui::detail::PricelistInfo pricelist_info;

        pricelist_info.id = pricelist.at("id").as_int64();
        pricelist_info.name = pricelist.at("name").as_string();
        pricelist_info.currency = pricelist.at("currency").as_string();
        pricelist_info.rate_per_minute = pricelist.at("rate_per_minute").as_double();
        pricelist_info.is_active = pricelist.at("is_active").as_bool();

        return pricelist_info;
    }
};

struct TarifInfo {
    int id;
    std::string name;
    int pricelist_id;
    int markup_percent;
    int free_minutes;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::TarifInfo& tarif) {
        jv = {
            {"id"s, tarif.id},
            {"name"s, tarif.name},
            {"pricelist_id"s, tarif.pricelist_id},
            {"markup_percent"s, tarif.markup_percent},
            {"free_minutes"s, tarif.free_minutes}
        };
    }

    friend ui::detail::TarifInfo
    tag_invoke(json::value_to_tag<ui::detail::TarifInfo>&, const json::value& tarif) {
        ui::detail::TarifInfo tarif_info;

        tarif_info.id = tarif.at("id").as_int64();
        tarif_info.name = tarif.at("name").as_string();
        tarif_info.pricelist_id = tarif.at("pricelist_id").as_int64();
        tarif_info.markup_percent = tarif.at("markup_percent").as_int64();
        tarif_info.free_minutes = tarif.at("free_minutes").as_int64();

        return tarif_info;
    }
};

struct CallStatisticsInfo {
    int64_t id;
    std::string call_id;
    int trunk_id;
    int tarif_id;
    int duration_seconds;
    double cost;
    std::string call_time;

    friend void tag_invoke(json::value_from_tag, json::value& jv,
                           const ui::detail::CallStatisticsInfo& call_stat) {
        jv = {
            {"id"s, call_stat.id},
            {"call_id"s, call_stat.call_id},
            {"trunk_id"s, call_stat.trunk_id},
            {"tarif_id"s, call_stat.tarif_id},
            {"duration_seconds"s, call_stat.duration_seconds},
            {"cost"s, call_stat.cost},
            {"call_time"s, call_stat.call_time}
        };
    }

    friend ui::detail::CallStatisticsInfo
    tag_invoke(json::value_to_tag<ui::detail::CallStatisticsInfo>&, const json::value& call_stat) {
        ui::detail::CallStatisticsInfo call_statistics_info;

        call_statistics_info.id = call_stat.at("id").as_int64();
        call_statistics_info.call_id = call_stat.at("call_id").as_string();
        call_statistics_info.trunk_id = call_stat.at("trunk_id").as_int64();
        call_statistics_info.tarif_id = call_stat.at("tarif_id").as_int64();
        call_statistics_info.duration_seconds = call_stat.at("duration_seconds").as_int64();
        call_statistics_info.cost = call_stat.at("cost").as_double();
        call_statistics_info.call_time = call_stat.at("call_time").as_string();

        return call_statistics_info;
    }
};

} // namespace detail

} // namespace ui
