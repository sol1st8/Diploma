#include "api_handler.h"
#include "../call_simulator/call_generator.h"
#include "../sync/thread_loader.h"
#include "../analytics/analytics.h"
#include "../config/dynamic_config.h"
#include "../logger/logger.h"

#include <pqxx/pqxx>

#include <algorithm>
#include <string>
#include <chrono>

namespace {

std::string CleanErrorMessage(const std::string& message) {
    std::string cleaned_message = message;

    size_t detail_pos = message.find("DETAIL:");
    if (detail_pos != std::string::npos) {
        cleaned_message = message.substr(detail_pos + 9);
    }

    cleaned_message.erase(std::remove(cleaned_message.begin(), cleaned_message.end(), '\"'), cleaned_message.end());
    cleaned_message.erase(std::remove(cleaned_message.begin(), cleaned_message.end(), '\n'), cleaned_message.end());

    return cleaned_message;
}

} // namespace

namespace api_handler {

bool ApiHandler::CheckEndPath() {
    return req_info_.target == "/"sv || req_info_.target.empty();
}

std::string ApiHandler::FindAndCutTarget(RequestInfo& req) {
    std::string res;

    size_t q_pos = req.target.find_last_of('?');

    size_t pos = req.target.find_first_of('/', 1);

    if (pos != req.target.npos) {
        res = req.target.substr(0, pos);
        req.target = req.target.substr(res.size());
        return res;
    }

    res = req.target;
    req.target = "";
    return res;
}

std::string ApiHandler::GetIdFromTarget(const std::string& target) {
    size_t last_slash_pos = target.find_last_of('/');
    if (last_slash_pos == std::string::npos) {
        return ""s;
    }
    return target.substr(last_slash_pos + 1);
}

void ApiHandler::HandleApiResponse() {
    // Обрабатываем OPTIONS запросы для всех путей (CORS preflight)
    if (req_info_.method == http::verb::options) {
        return HandleOptions();
    }

    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/add"s) {
        HandleAdd();
    }
    else if (path_part == "/get"s) {
        HandleGet();
    }
    else if (path_part == "/update"s) {
        HandleUpdate();
    }
    else if (path_part == "/simulate"s) {
        HandleSimulate();
    }
    else if (path_part == "/sync"s) {
        HandleSync();
    }
    else if (path_part == "/system"s) {
        HandleSystem();
    }
    else if (path_part == "/analytics"s) {
        HandleAnalytics();
    }
    else if (path_part == "/config"s) {
        HandleConfig();
    }
    else if (path_part == "/logs"s) {
        HandleLogs();
    }
    //else if (path_part == "/register"s) {
    //    HandleRegister();
    //}
    else if (path_part == "/login"s) {
        HandleLogin();
    }
    else if (path_part == "/logout"s) {
        HandleLogout();
    }
    //else if (path_part == "/token"s) {
    //    HandleToken();
    //}
    //else if (path_part == "/user"s) {
    //    HandleUser();
    //}
    else {
        SendBadRequestResponseDefault();
    }
}

void ApiHandler::HandleOptions() {
    ResponseInfo result = MakeResponse(http::status::ok, true);
    SendOkResponse({});
}

void ApiHandler::HandleAdd() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/pricelist"s) {
        HandleAddPricelist();
    }
    else if (path_part == "/tarif"s) {
        HandleAddTarif();
    }
    else if (path_part == "/trunk"s) {
        HandleAddTrunk();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleAddPricelist() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::PricelistInfo pricelist = json::value_to<ui::detail::PricelistInfo>(jv);

    try {
        if (CheckEndPath()) {
            application_.GetUseCases().AddPricelist(pricelist);
            LOG_INFO("Pricelist added: " + pricelist.name);
            return SendOkResponse({});
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to add pricelist: " + std::string(e.what()));
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleAddTarif() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::TarifInfo tarif = json::value_to<ui::detail::TarifInfo>(jv);

    try {
        if (CheckEndPath()) {
            application_.GetUseCases().AddTarif(tarif);
            return SendOkResponse({});
        }
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleAddTrunk() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::TrunkInfo trunk = json::value_to<ui::detail::TrunkInfo>(jv);

    try {
        if (CheckEndPath()) {
            application_.GetUseCases().AddTrunk(trunk);
            return SendOkResponse({});
        }
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleSimulate() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/calls"s) {
        HandleSimulateCalls();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleSimulateCalls() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    try {
        auto cfg = config::g_config.Get();
        
        // Парсим параметры из body (количество звонков)
        int call_count = cfg->default_call_count; // Из конфигурации
        
        if (!req_info_.body.empty()) {
            json::value jv = json::parse(req_info_.body);
            if (jv.is_object() && jv.as_object().contains("count"s)) {
                call_count = jv.at("count").as_int64();
                if (call_count <= 0 || call_count > cfg->max_calls_per_request) {
                    LOG_WARNING("Invalid call count requested: " + std::to_string(call_count));
                    return SendBadRequestResponse(
                        "Значение должно быть между 1 и "s + std::to_string(cfg->max_calls_per_request)
                    );
                }
            }
        }
        
        LOG_INFO("Starting call simulation: " + std::to_string(call_count) + " calls");

        auto hubs = application_.GetUseCases().GetHubs();
        auto servers = application_.GetUseCases().GetServers();
        auto trunks = application_.GetUseCases().GetTrunks();
        auto tarifs = application_.GetUseCases().GetTarifs();
        auto pricelists = application_.GetUseCases().GetPricelists();

        if (hubs.empty() || servers.empty() || trunks.empty() ||
            tarifs.empty() || pricelists.empty()) {
            return SendBadRequestResponse(
                "Cannot generate calls: missing required data (hubs, servers, trunks, tarifs, or pricelists)"s
            );
        }

        call_simulator::CallGenerator generator;
        auto generated_calls = generator.GenerateBatch(
            call_count, hubs, servers, trunks, tarifs, pricelists
        );

        int saved_count = 0;
        for (const auto& call : generated_calls) {
            try {
                ui::detail::CallStatisticsInfo call_info{
                    0, // id будет автоматически сгенерирован БД
                    call.call_id,
                    call.trunk_id,
                    call.tarif_id,
                    call.duration_seconds,
                    call.cost,
                    call.call_time
                };
                application_.GetUseCases().AddCallStatistics(call_info);
                saved_count++;
            }
            catch (const std::exception& e) {
                // Логируем ошибку, но продолжаем
                LOG_ERROR("Failed to save call " + call.call_id + ": " + std::string(e.what()));
            }
        }

        json::value response = {
            {"success"s, true},
            {"message"s, "Calls generated successfully"s},
            {"requested"s, call_count},
            {"generated"s, static_cast<int>(generated_calls.size())},
            {"saved"s, saved_count}
        };

        LOG_INFO("Call simulation completed: " + std::to_string(saved_count) + " calls saved");
        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Call simulation failed: " + std::string(e.what()));
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleSync() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/trigger"s) {
        HandleSyncTrigger();
    }
    else if (path_part == "/status"s) {
        HandleSyncStatus();
    }
    else if (path_part == "/full"s) {
        HandleSyncFull();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleSyncTrigger() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    try {
        // Синхронизация запускается автоматически в фоновом потоке
        // Здесь мы просто возвращаем успешный ответ
        json::value response = {
            {"success"s, true},
            {"message"s, "Synchronization is running in background"s},
            {"note"s, "Background sync is managed by ThreadLoader"s}
        };

        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleSyncStatus() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        // Возвращаем информацию о том, что синхронизация управляется ThreadLoader
        json::value response = {
            {"enabled"s, true},
            {"message"s, "Synchronization is managed by ThreadLoader in background"s},
            {"note"s, "Check application.conf for sync configuration"s}
        };

        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleSyncFull() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    try {
        // Парсим server_id из body
        int server_id = 0;
        
        if (!req_info_.body.empty()) {
            json::value jv = json::parse(req_info_.body);
            if (jv.is_object() && jv.as_object().contains("server_id"s)) {
                server_id = jv.at("server_id").as_int64();
            } else {
                return SendBadRequestResponse("server_id is required"s);
            }
        } else {
            return SendBadRequestResponse("Request body is required"s);
        }

        if (server_id <= 0) {
            return SendBadRequestResponse("server_id must be positive"s);
        }

        // Получаем конфигурацию синхронизации из динамической конфигурации
        auto sync_config = config::g_config.GetSyncConfig();
        
        if (sync_config.central_db_url.empty()) {
            return SendBadRequestResponse("Central database is not configured"s, "syncNotConfigured"s);
        }

        // Подключаемся к центральной БД
        pqxx::connection central_conn(sync_config.central_db_url);
        
        if (!central_conn.is_open()) {
            return SendBadRequestResponse("Failed to connect to central database"s, "dbConnectionFailed"s);
        }

        // Вызываем функцию event.notify_all(server_id)
        pqxx::work txn(central_conn);
        
        auto result = txn.exec_params(
            "SELECT event.notify_all($1)",
            server_id
        );
        
        txn.commit();
        
        bool success = false;
        if (!result.empty() && !result[0][0].is_null()) {
            success = result[0][0].as<bool>();
        }

        if (success) {
            json::value response = {
                {"success"s, true},
                {"message"s, "Full synchronization initiated successfully"s},
                {"server_id"s, server_id},
                {"note"s, "All tables will be synchronized to the target server"s}
            };
            return SendOkResponse(json::serialize(response));
        } else {
            return SendBadRequestResponse("event.notify_all() returned false"s, "syncFailed"s);
        }
    }
    catch (const pqxx::sql_error& e) {
        std::string error_msg = "Database error: "s + e.what();
        return SendBadRequestResponse(CleanErrorMessage(error_msg), "sqlError"s);
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleSystem() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/health"s) {
        HandleSystemHealth();
    }
    else if (path_part == "/stats"s) {
        HandleSystemStats();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleSystemHealth() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        // Проверяем доступность БД
        auto hubs = application_.GetUseCases().GetHubs();
        
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%d %H:%M:%S");

        json::value response = {
            {"status"s, "healthy"s},
            {"database"s, "connected"s},
            {"timestamp"s, ss.str()}
        };

        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        json::value response = {
            {"status"s, "unhealthy"s},
            {"error"s, CleanErrorMessage(e.what())}
        };
        return SendOkResponse(json::serialize(response));
    }
}

void ApiHandler::HandleSystemStats() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        // Получаем статистику из БД
        auto hubs = application_.GetUseCases().GetHubs();
        auto servers = application_.GetUseCases().GetServers();
        auto trunks = application_.GetUseCases().GetTrunks();
        auto tarifs = application_.GetUseCases().GetTarifs();
        auto pricelists = application_.GetUseCases().GetPricelists();
        auto call_stats = application_.GetUseCases().GetCallStatistics();
        auto nas_ips = application_.GetUseCases().GetNasIps();

        // Подсчитываем активные элементы
        int active_hubs = std::count_if(hubs.begin(), hubs.end(),
            [](const ui::detail::HubInfo& h) { return h.is_active; });
        int active_servers = std::count_if(servers.begin(), servers.end(),
            [](const ui::detail::ServerInfo& s) { return s.is_active; });
        int active_pricelists = std::count_if(pricelists.begin(), pricelists.end(),
            [](const ui::detail::PricelistInfo& p) { return p.is_active; });

        // Подсчитываем общую стоимость звонков
        double total_revenue = 0.0;
        int total_duration = 0;
        for (const auto& call : call_stats) {
            total_revenue += call.cost;
            total_duration += call.duration_seconds;
        }

        json::value response = {
            {"database"s, {
                {"hubs"s, {
                    {"total"s, static_cast<int>(hubs.size())},
                    {"active"s, active_hubs}
                }},
                {"servers"s, {
                    {"total"s, static_cast<int>(servers.size())},
                    {"active"s, active_servers}
                }},
                {"trunks"s, static_cast<int>(trunks.size())},
                {"nas_ips"s, static_cast<int>(nas_ips.size())},
                {"tarifs"s, static_cast<int>(tarifs.size())},
                {"pricelists"s, {
                    {"total"s, static_cast<int>(pricelists.size())},
                    {"active"s, active_pricelists}
                }}
            }},
            {"calls"s, {
                {"total"s, static_cast<int>(call_stats.size())},
                {"total_revenue"s, total_revenue},
                {"total_duration_seconds"s, total_duration},
                {"total_duration_minutes"s, total_duration / 60}
            }}
        };

        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleAnalytics() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/calls-by-trunk"s) {
        HandleAnalyticsCallsByTrunk();
    }
    else if (path_part == "/calls-by-tarif"s) {
        HandleAnalyticsCallsByTarif();
    }
    else if (path_part == "/calls-by-hub"s) {
        HandleAnalyticsCallsByHub();
    }
    else if (path_part == "/revenue"s) {
        HandleAnalyticsRevenue();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleAnalyticsCallsByTrunk() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        auto calls = application_.GetUseCases().GetCallStatistics();
        auto trunks = application_.GetUseCases().GetTrunks();

        auto analytics = analytics::AnalyticsCalculator::CalculateByTrunk(calls, trunks);
        json::value jv = analytics::AnalyticsCalculator::ToJson(analytics);

        return SendOkResponse(json::serialize(jv));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleAnalyticsCallsByTarif() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        auto calls = application_.GetUseCases().GetCallStatistics();
        auto tarifs = application_.GetUseCases().GetTarifs();

        auto analytics = analytics::AnalyticsCalculator::CalculateByTarif(calls, tarifs);
        json::value jv = analytics::AnalyticsCalculator::ToJson(analytics);

        return SendOkResponse(json::serialize(jv));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleAnalyticsCallsByHub() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        auto calls = application_.GetUseCases().GetCallStatistics();
        auto hubs = application_.GetUseCases().GetHubs();
        auto servers = application_.GetUseCases().GetServers();
        auto trunks = application_.GetUseCases().GetTrunks();

        auto analytics = analytics::AnalyticsCalculator::CalculateByHub(calls, hubs, servers, trunks);
        json::value jv = analytics::AnalyticsCalculator::ToJson(analytics);

        return SendOkResponse(json::serialize(jv));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleAnalyticsRevenue() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        auto calls = application_.GetUseCases().GetCallStatistics();

        // Получаем параметр period из query string (по умолчанию hour)
        std::string period = "hour";
        size_t query_pos = req_info_.target.find('?');
        if (query_pos != std::string::npos) {
            std::string query = req_info_.target.substr(query_pos + 1);
            if (query.find("period=day") != std::string::npos) {
                period = "day";
            }
        }

        json::value jv;
        if (period == "day") {
            auto analytics = analytics::AnalyticsCalculator::CalculateRevenueByDay(calls);
            jv = analytics::AnalyticsCalculator::ToJson(analytics);
        } else {
            auto analytics = analytics::AnalyticsCalculator::CalculateRevenueByHour(calls);
            jv = analytics::AnalyticsCalculator::ToJson(analytics);
        }

        return SendOkResponse(json::serialize(jv));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleConfig() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (CheckEndPath()) {
        // GET /api/config - получить конфигурацию
        if (req_info_.method == http::verb::get || req_info_.method == http::verb::head) {
            HandleConfigGet();
        }
        // PUT /api/config - обновить конфигурацию
        else if (req_info_.method == http::verb::put) {
            HandleConfigUpdate();
        }
        else {
            SendBadRequestResponseDefault();
        }
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleConfigGet() {
    try {
        std::string config_json = config::g_config.ToJson();
        return SendOkResponse(config_json);
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleConfigUpdate() {
    try {
        config::g_config.UpdateFromJson(req_info_.body);
        
        json::value response = {
            {"success"s, true},
            {"message"s, "Configuration updated successfully"s},
            {"version"s, config::g_config.GetVersion()}
        };
        
        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleLogs() {
    // Обрабатываем OPTIONS запрос для CORS preflight
    if (req_info_.method == http::verb::options) {
        return HandleOptions();
    }

    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        LOG_WARNING("HandleLogs: wrong method");
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }

    try {
        int lines_count = 1000;
        size_t query_pos = req_info_.target.find('?');
        if (query_pos != std::string::npos) {
            std::string query = req_info_.target.substr(query_pos + 1);
            size_t lines_pos = query.find("lines=");
            if (lines_pos != std::string::npos) {
                std::string lines_str = query.substr(lines_pos + 6);
                size_t amp_pos = lines_str.find('&');
                if (amp_pos != std::string::npos) {
                    lines_str = lines_str.substr(0, amp_pos);
                }
                try {
                    lines_count = std::stoi(lines_str);
                    if (lines_count <= 0) lines_count = 100;
                    if (lines_count > 1000) lines_count = 1000; // Максимум 1000 строк
                }
                catch (...) {
                    lines_count = 100;
                }
            }
        }

        // Получаем логи из памяти (быстрее чем чтение файла)
        auto logs = logger::Logger::Instance().GetRecentLogs(lines_count);
        size_t total = logger::Logger::Instance().GetTotalLogsCount();

        // Формируем JSON массив
        json::array logs_array;
        for (const auto& log : logs) {
            logs_array.push_back(json::value(log));
        }

        json::value response = {
            {"logs"s, logs_array},
            {"count"s, static_cast<int>(logs.size())},
            {"total"s, static_cast<int>(total)}
        };

        return SendOkResponse(json::serialize(response));
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }
}

void ApiHandler::HandleGet() {
    std::string path_part = FindAndCutTarget(req_info_);

    if (path_part == "/pricelist"s) {
        HandleGetPricelists();
    }
    else if (path_part == "/tarif"s) {
        HandleGetTarifs();
    }
    else if (path_part == "/server"s) {
        HandleGetServers();
    }
    else if (path_part == "/trunk"s) {
        HandleGetTrunks();
    }
    else if (path_part == "/hub"s) {
        HandleGetHubs();
    }
    else if (path_part == "/call-statistics"s) {
        HandleGetCallStatistics();
    }
    else if (path_part == "/nas-ip"s) {
        HandleGetNasIps();
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleGetPricelists() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetPricelists());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetTarifs() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetTarifs());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetHubs() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetHubs());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetServers() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetServers());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetTrunks() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetTrunks());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetCallStatistics() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetCallStatistics());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleGetNasIps() {
    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
    }
    if (CheckEndPath()) {
        json::value jv = json::value_from(application_.GetUseCases().GetNasIps());
        return SendOkResponse(json::serialize(jv));
    }
    SendBadRequestResponseDefault();
}

void ApiHandler::HandleUpdate() {
    std::string path_part = FindAndCutTarget(req_info_);

    std::string target = req_info_.target;
    if (GetIdFromTarget(target).empty()) {
        return SendBadRequestResponseDefault();
    }

    int id = std::stoi(GetIdFromTarget(target).c_str());

    if (path_part == "/pricelist"s) {
        HandleUpdatePricelist(id);
    }
    else if (path_part == "/tarif"s) {
        HandleUpdateTarif(id);
    }
    else if (path_part == "/trunk"s) {
        HandleUpdateTrunk(id);
    }
    else {
        SendNotFoundResponse();
    }
}

void ApiHandler::HandleUpdatePricelist(int id) {
    if (req_info_.method != http::verb::put && req_info_.method != http::verb::options) {
        return SendWrongMethodResponseAllowedPut("Wrong method"s, true);
    }

    if (req_info_.method == http::verb::options) {
        return HandleOptions();
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::PricelistInfo pricelist = json::value_to<ui::detail::PricelistInfo>(jv);

    try {
        application_.GetUseCases().UpdatePricelist(pricelist, id);
        return SendOkResponse({});
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleUpdateTarif(int id) {
    if (req_info_.method != http::verb::put && req_info_.method != http::verb::options) {
        return SendWrongMethodResponseAllowedPut("Wrong method"s, true);
    }

    if (req_info_.method == http::verb::options) {
        return HandleOptions();
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::TarifInfo tarif = json::value_to<ui::detail::TarifInfo>(jv);

    try {
        application_.GetUseCases().UpdateTarif(tarif, id);
        return SendOkResponse({});
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

void ApiHandler::HandleUpdateTrunk(int id) {
    if (req_info_.method != http::verb::put && req_info_.method != http::verb::options) {
        return SendWrongMethodResponseAllowedPut("Wrong method"s, true);
    }

    if (req_info_.method == http::verb::options) {
        return HandleOptions();
    }

    json::value jv = json::parse(req_info_.body);
    ui::detail::TrunkInfo trunk = json::value_to<ui::detail::TrunkInfo>(jv);

    try {
        application_.GetUseCases().UpdateTrunk(trunk, id);
        return SendOkResponse({});
    }
    catch (const std::exception& e) {
        return SendBadRequestResponse(CleanErrorMessage(e.what()));
    }

    SendBadRequestResponseDefault();
}

PersonInfo ApiHandler::CreateAdmin() {
    Person admin{"admin@work.com"s, "Admin1Admin"s, "admin"s};
    persons_.insert({admin, "Admin"s});

    PersonInfo p_info{admin.email, admin.password, persons_[admin], admin.role};

    std::string access_token = GetUniqueToken();
    std::string refresh_token = GetUniqueToken();

    tokens_[p_info] = {access_token, refresh_token, TimeTracker{}};
    auth_to_person_[access_token] = p_info;
    refresh_tokens_.push_back(refresh_token);
    refresh_token_to_person_[refresh_token] = p_info;
    last_role_ = admin.role;

    return p_info;
}

void ApiHandler::HandleLogin() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    json::value person = json::parse(req_info_.body);

    if (!person.as_object().contains("email"s) || !person.as_object().contains("password"s)) {
        LOG_WARNING("Login attempt with invalid format");
        return SendNoAuthResponse("Invalid login format"s, "invalidLogin"s);
    }

    if (person.at("email").as_string() == "admin@work.com"s && person.at("password").as_string() == "Admin1Admin"s) {
        PersonInfo p_info = CreateAdmin();
        LOG_INFO("User logged in: " + p_info.email);
        json::value jv {
            {"success"s, true},
            {"accessToken"s, "Bearer "s + tokens_[p_info].access_token},
            {"refreshToken"s, tokens_[p_info].refresh_token},
            {"user"s, {
                    {"email"s, p_info.email},
                    {"name"s, p_info.name},
                    {"role"s, p_info.role}
                }}
        };
        return SendOkResponse(json::serialize(jv));
    }

    //std::string email;
    //std::string password;
    //std::string role;

    //email = person.at("email").as_string();
    //password = person.at("password").as_string();

    //try {
    //    role = login_to_role_.at({email, password});
    //}
    //catch (...) {
    //    return SendNoAuthResponse("Invalid login format"s, "invalidLogin"s);
    //}

    //personnel_number_ = application_.GetUseCases().GetPersonnelNumberForEmail(email);

    //Person p{email, password, role};
    //last_role_ = role;

    //if (persons_.contains(p)) {
    //    PersonInfo p_info{email, password, persons_[p], role};
    //    if (tokens_.contains(p_info)) {
    //        refresh_tokens_.push_back(tokens_[p_info].refresh_token);
    //        json::value jv {
    //            {"success"s, true},
    //            {"accessToken"s, "Bearer "s + tokens_[p_info].access_token},
    //            {"refreshToken"s, tokens_[p_info].refresh_token},
    //            {"user"s, {
    //                    {"email"s, p_info.email},
    //                    {"name"s, p_info.name},
    //                    {"role"s, p_info.role}
    //                }}
    //        };
    //        return SendOkResponse(json::serialize(jv));
    //    }
    //}

    SendNoAuthResponse("Invalid login format"s, "invalidLogin"s);
}

void ApiHandler::HandleLogout() {
    if (req_info_.method != http::verb::post) {
        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
    }

    json::value token = json::parse(req_info_.body);

    if (!token.as_object().contains("token")) {
        return SendBadRequestResponse("Invalid token"s, "invalidToken"s);
    }

    std::string tok;
    tok = token.at("token").as_string();

    if (refresh_tokens_.empty()) {
        return SendBadRequestResponse("Invalid token"s, "invalidToken"s);
    }

    if (refresh_tokens_.back() == tok) {
        json::value jv {
            {"success"s, true},
            {"message"s, "Successful logout"s}
        };
        return SendOkResponse(json::serialize(jv));
    }
    else {
        return SendBadRequestResponse("Invalid logout"s, "invalidLogout"s);
    }

    SendBadRequestResponseDefault();
}

//void ApiHandler::HandleToken() {
//    if (req_info_.method != http::verb::post && req_info_.method != http::verb::options) {
//        return SendWrongMethodResponseAllowedPost("Wrong method"s, true);
//    }
//
//    if (req_info_.method == http::verb::options) {
//        return HandleOptions();
//    }
//
//    json::value token = json::parse(req_info_.body);
//
//    if (!token.as_object().contains("token")) {
//        return SendBadRequestResponse("Invalid token"s, "invalidToken"s);
//    }
//
//    std::string tok;
//    tok = token.at("token").as_string();
//
//    try {
//        PersonInfo p_info = refresh_token_to_person_.at(tok);
//        std::string access_token = GetUniqueToken();
//        std::string refresh_token = GetUniqueToken();
//        json::value jv {
//            {"success"s, true},
//            {"accessToken"s, "Bearer "s + access_token},
//            {"refreshToken"s, refresh_token}
//        };
//
//        tokens_[p_info].access_token = access_token;
//        tokens_[p_info].refresh_token = refresh_token;
//        tokens_[p_info].tracker.reset_timer();
//        auth_to_person_[access_token] = p_info;
//        refresh_token_to_person_[refresh_token] = p_info;
//        refresh_tokens_.push_back(refresh_token);
//
//        return SendOkResponse(json::serialize(jv));
//    }
//    catch (...) {
//        return SendBadRequestResponse("Invalid token"s, "invalidToken"s);
//    }
//
//    SendBadRequestResponseDefault();
//}

//void ApiHandler::HandleUser() {
//    static size_t token_size = 32;
//
//    if (req_info_.method != http::verb::get && req_info_.method != http::verb::head && req_info_.method != http::verb::options) {
//        return SendWrongMethodResponseAllowedGetHead("Wrong method"s, true);
//    }
//
//    if (req_info_.method == http::verb::options) {
//        return HandleOptions();
//    }
//
//    if (req_info_.auth.empty()) {
//        return SendNoAuthResponse("Invalid token"s, "invalidToken"s);
//    }
//
//    std::string token_str{req_info_.auth};
//
//    try {
//        std::string email = auth_to_person_.at(token_str.substr(7)).email;
//        std::string password = auth_to_person_.at(token_str.substr(7)).password;
//        std::string name = auth_to_person_.at(token_str.substr(7)).name;
//        std::string role = auth_to_person_.at(token_str.substr(7)).role;
//        Person p{email, password, role};
//        PersonInfo p_info{email, password, persons_[p], role};
//        if (!tokens_[p_info].tracker.Has20MinutesPassed()) {
//            json::value jv = {
//                {"success"s, true},
//                {"user"s, {
//                        {"email"s, email},
//                        {"name"s, name},
//                        {"role"s, role}
//                    }}
//            };
//            return SendOkResponse(json::serialize(jv));
//        }
//        else {
//            return SendBadRequestResponse("Token is expired"s, "tokenIsExpired"s);
//        }
//    }
//    catch (...) {
//        return SendBadRequestResponse("Invalid token"s, "invalidToken"s);
//    }
//
//    SendBadRequestResponseDefault();
//}

ApiHandler::ResponseInfo ApiHandler::MakeResponse(http::status status, bool no_cache) {
    ResponseInfo result;

    result.status = status;
    result.version = req_info_.version;
    result.content_type = body_type::json;
    result.keep_alive = req_info_.keep_alive;
    result.no_cache = no_cache;
    result.additional_fields.emplace_back(http::field::access_control_allow_origin, "*"s);
    result.additional_fields.emplace_back(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS"s);
    result.additional_fields.emplace_back(http::field::access_control_allow_headers, "Content-Type, Authorization"s);

    return result;
}

void ApiHandler::SendOkResponse(const std::string& body, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::ok, no_cache);

    result.body = body;

    send_(result);
}

void ApiHandler::SendBadRequestResponse(std::string message, std::string code, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::bad_request, no_cache);

    json::value body = {
        {"code"s, code},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendNotFoundResponse(const std::string& message, const std::string& key, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::not_found, no_cache);

    json::value body {
        {"code"s, key},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendNoAuthResponse(const std::string& message, const std::string& key, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::unauthorized, no_cache);

    json::value body = {
        {"code"s, key},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedDelete(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {"code"s, "invalidMethod"s},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "DELETE"s);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedGetHead(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {"code"s, "invalidMethod"s},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "GET, HEAD"s);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedPost(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {"code"s, "invalidMethod"s},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "POST"s);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedPut(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {"code"s, "invalidMethod"s},
        {"message"s, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "PUT"s);

    send_(result);
}

} // namespace api_handler
