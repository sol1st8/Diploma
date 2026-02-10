#pragma once

#include "api/api_handler.h"
#include "http_server/http_server.h"
#include "resp_maker.h"

#include <boost/beast.hpp>
#include <filesystem>
#include <utility>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
using HttpResponse = http::response<http::string_body>;
using namespace std::literals;

class RequestHandler {
  public:
    explicit RequestHandler(std::filesystem::path static_data_path) : static_data_path_(std::move(static_data_path)) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

  private:
    std::filesystem::path static_data_path_;

    template <typename Body, typename Allocator, typename Send>
    void HandleStaticDataResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                               std::string_view target, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    void TryToSendFile(http::request<Body, http::basic_fields<Allocator>>& req,
                       const std::filesystem::path& file, Send&& send);
};


template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
                                std::string_view target = req.target();

    if (target.starts_with("/api"s)) {
        api_handler::HandleApiRequest(std::forward<decltype(req)>(req), std::forward<Send>(send));
    }
    else {
        HandleStaticDataResponse(std::forward<decltype(req)>(req), target, std::forward<Send>(send));
    }
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::HandleStaticDataResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                                              std::string_view target, Send&& send) {
    using namespace resp_maker::txt_resp;
    namespace fs = std::filesystem;

    fs::path file = fs::weakly_canonical(static_data_path_ / target.substr(1));

    if (!file.string().starts_with(static_data_path_.string())) {
        return send(MakeBadRequestResponse(req, "Access denied"s));
    }
    if (file.filename().string() == "login"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "pricelists"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "tarifs"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "tables"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "call-stats"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "simulator"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "config"s) {
        file = file = file.parent_path();
    }
    if (file.filename().string() == "logs"s) {
        file = file = file.parent_path();
    }

    if (fs::exists(file)) {
        if (fs::is_directory(file)) {
            if (fs::exists(file / "index.html"s)) {
                file /= "index.html"s;
                TryToSendFile(req, file, send);
            }
            else if (fs::exists(file / "index.htm"s)){
                file /= "index.htm"s;
                TryToSendFile(req, file, send);
            }
            else {
                return send(MakeNotFoundResponse(req, "No index.html in directory"s));
            }
        }
        else {
            TryToSendFile(req, file, send);
        }
    }
    else {
        return send(MakeNotFoundResponse(req, "No such file or directory"s));
    }
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::TryToSendFile(http::request<Body, http::basic_fields<Allocator>>& req,
                                   const std::filesystem::path& file, Send&& send) {
    using namespace resp_maker;

    try {
        return send(file_resp::MakeFileResponse(req, file));
    }
    catch (const std::runtime_error& e) {
        return send(txt_resp::MakeServerErrorResponse(req, e.what()));
    }
    catch (...) {
        return send(txt_resp::MakeServerErrorResponse(req, "Unknown error"s));
    }
}

}  // namespace http_handler
