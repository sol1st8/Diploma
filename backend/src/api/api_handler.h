#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "../application.h"
#include "../resp_maker.h"

#include <deque>

namespace api_handler {
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using namespace std::literals;

namespace {

constexpr const char DB_URL_ENV_NAME[]{"DB_URL"};

db::AppConfig GetConfigFromEnv() {
    db::AppConfig config;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        config.db_url = url;
    }
    else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return config;
}

}  // namespace

class TimeTracker {
  public:
    TimeTracker() : start_time_(std::chrono::high_resolution_clock::now()) {}

    bool Has20MinutesPassed() {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::minutes>(current_time - start_time_);
        return elapsed_time >= std::chrono::minutes(20);
    }

    void reset_timer() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};

struct Login {
    std::string email;
    std::string password;

    bool operator==(const Login& other) const {
        return email == other.email &&
               password == other.password;
    }
};

struct LoginHasher {
    std::size_t operator()(const Login& login) const {
        std::size_t h1 = std::hash<std::string>()(login.email);
        std::size_t h2 = std::hash<std::string>()(login.password);

        return h1 + h2 * 23;
    }
};

struct Person {
    std::string email;
    std::string password;
    std::string role;

    bool operator==(const Person& other) const {
        return email == other.email &&
               password == other.password &&
               role == other.role;
    }
};

struct PersonHasher {
    std::size_t operator()(const Person& person) const {
        std::size_t h1 = std::hash<std::string>()(person.email);
        std::size_t h2 = std::hash<std::string>()(person.password);
        std::size_t h3 = std::hash<std::string>()(person.role);

        return h1 + h2 * 19 + h3 * 19 * 19;
    }
};

struct PersonInfo {
    std::string email;
    std::string password;
    std::string name;
    std::string role;

    bool operator==(const PersonInfo& other) const {
        return email == other.email &&
               password == other.password &&
               name == other.name &&
               role == other.role;
    }
};

struct PersonInfoHasher {
    std::size_t operator()(const PersonInfo& person) const {
        std::size_t h1 = std::hash<std::string>()(person.email);
        std::size_t h2 = std::hash<std::string>()(person.password);
        std::size_t h3 = std::hash<std::string>()(person.name);
        std::size_t h4 = std::hash<std::string>()(person.role);

        return h1 + h2 * 37 + h3 * 37 * 37 + h4 * 37 * 37 * 37;
    }
};

struct Tokens {
    std::string access_token;
    std::string refresh_token;
    TimeTracker tracker;
};

struct RequestInfo {
    std::string target;
    std::string body;
    http::verb method;
    std::string content_type;
    int version;
    bool keep_alive;
    std::string auth;
};

inline std::unordered_map<Person, std::string, PersonHasher> persons_;
inline std::unordered_map<PersonInfo, Tokens, PersonInfoHasher> tokens_;
inline std::unordered_map<std::string, PersonInfo> auth_to_person_;
inline std::unordered_map<std::string, PersonInfo> refresh_token_to_person_;
inline std::deque<std::string> refresh_tokens_;
//inline int personnel_number_{};
inline std::string last_role_;
//inline std::unordered_map<Login, std::string, LoginHasher> login_to_role_;
//inline std::unordered_set<std::string> emails_;

class ApiHandler : public std::enable_shared_from_this<ApiHandler> {
  public:
    ApiHandler() = default;

    using ResponseInfo = resp_maker::detail::ResponseInfo;

    template <typename Body, typename Allocator, typename Send>
    void Handle(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send);

  private:
    std::function<void(ResponseInfo)> send_;
    RequestInfo req_info_{};
    db::Application application_{GetConfigFromEnv()};

    bool CheckEndPath();
    std::string FindAndCutTarget(RequestInfo& req);
    std::string GetIdFromTarget(const std::string& target);

    void HandleApiResponse();

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    std::string GetUniqueToken() {
        std::stringstream stream;
        stream.fill('0');
        stream << std::hex << std::setw(16) << generator1_();
        stream << std::hex << std::setw(16) << generator2_();
        return stream.str();
    }

    void HandleOptions();

    void HandleAdd();
    void HandleAddPricelist();
    void HandleAddTarif();
    void HandleAddTrunk();

    void HandleSimulate();
    void HandleSimulateCalls();

    void HandleSync();
    void HandleSyncTrigger();
    void HandleSyncStatus();
    void HandleSyncFull();

    void HandleSystem();
    void HandleSystemHealth();
    void HandleSystemStats();

    void HandleConfig();
    void HandleConfigGet();
    void HandleConfigUpdate();

    void HandleLogs();

    void HandleGet();
    void HandleGetHubs();
    void HandleGetServers();
    void HandleGetTrunks();
    void HandleGetPricelists();
    void HandleGetTarifs();
    void HandleGetNasIps();
    void HandleGetCallStatistics();

    void HandleAnalytics();
    void HandleAnalyticsCallsByTrunk();
    void HandleAnalyticsCallsByTarif();
    void HandleAnalyticsCallsByHub();
    void HandleAnalyticsRevenue();

    void HandleUpdate();
    void HandleUpdatePricelist(int id);
    void HandleUpdateTarif(int id);
    void HandleUpdateTrunk(int id);

    //void HandleRegister();
    PersonInfo CreateAdmin();
    void HandleLogin();
    void HandleLogout();
    //void HandleToken();
    //void HandleUser();

    template <typename Body, typename Allocator>
    RequestInfo ParseRequest(const http::request<Body, http::basic_fields<Allocator>>& req);

    ResponseInfo MakeResponse(http::status status, bool no_cache);

    void SendOkResponse(const std::string& body, bool no_cache = true);
    void SendBadRequestResponse(std::string message, std::string code =
                                "badRequest"s, bool no_cache = true);
    void SendBadRequestResponseDefault(bool no_cache = true) {
        SendBadRequestResponse("Bad request"s, "badRequest"s, no_cache);
    }
    void SendNotFoundResponse(const std::string& message =  "Table not found"s,
                              const std::string& key = "tableNotFound"s, bool no_cache = true);

    void SendNoAuthResponse(const std::string& message = "invalidToken"s,
                            const std::string& key = "Something wrong with token"s, bool no_cache = true);

    void SendWrongMethodResponseAllowedDelete(const std::string& message = "Only DELETE method is expected"s, bool no_cache = true);
    void SendWrongMethodResponseAllowedGetHead(const std::string& message = "Only GET/HEAD method is expected"s,
                                               bool no_cache = true);
    void SendWrongMethodResponseAllowedPost(const std::string& message = "Only POST method is expected"s, bool no_cache = true);
    void SendWrongMethodResponseAllowedPut(const std::string& message = "Only PUT method is expected"s, bool no_cache = true);
};

template <typename Body, typename Allocator, typename Send>
void ApiHandler::Handle(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
    send_ = std::forward<Send>(send);
    req_info_ = ParseRequest(req);
    req_info_.target = req_info_.target.substr("/api"s.size());

    HandleApiResponse();
}

template <typename Body, typename Allocator>
RequestInfo ApiHandler::ParseRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
    RequestInfo result;
    result.target = req.target();
    result.body = req.body();
    result.method = req.method();
    result.version = req.version();
    result.keep_alive = req.keep_alive();

    if (req.find(http::field::content_type) != req.end()) {
        result.content_type = req.at(http::field::content_type);
    }
    if (req.find(http::field::authorization) != req.end()) {
        result.auth = req.at(http::field::authorization);
    }

    return result;
}

template <typename Body, typename Allocator, typename Send>
void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req,
                      Send&& send) {
    auto handler = std::make_shared<ApiHandler>();

    handler->Handle(req, [send = std::forward<Send>(send)](const resp_maker::detail::ResponseInfo& info){
        send(resp_maker::detail::MakeTextResponse<Body, Allocator>(info));
    });
}

} // namespace api_handler
