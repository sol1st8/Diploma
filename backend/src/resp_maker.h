#pragma once

#include "body_types/body_types.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/system.hpp>
#include <filesystem>

namespace resp_maker {
namespace beast = boost::beast;
namespace http  = beast::http;
namespace sys   = boost::system;

namespace detail {

struct ResponseInfo {
    http::status status;
    std::string body;
    std::string content_type;
    bool no_cache = false;
    int version;
    bool keep_alive = false;
    std::vector<std::pair<http::field, std::string>> additional_fields;
};

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 ResponseInfo& resp_info);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const ResponseInfo& resp_info);

} // namespace detail

namespace file_resp {

template <typename Body, typename Allocator>
http::response<http::file_body, http::basic_fields<Allocator>>
MakeFileResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 const std::filesystem::path& file);

} // namespace file_resp

namespace txt_resp {

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

} // namespace txt_resp

} // namespace resp_maker


namespace resp_maker {

namespace detail {

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 ResponseInfo& resp_info) {
    http::response<Body, http::basic_fields<Allocator>> result(resp_info.status, req.version());
    result.body() = resp_info.body;
    result.content_length(result.body().size());
    result.keep_alive(req.keep_alive());
    result.set(http::field::content_type, resp_info.content_type);

    if (resp_info.no_cache) {
        result.set(http::field::cache_control, "no-cache");
    }

    for (auto& [key, value] : resp_info.additional_fields) {
        result.set(key, value);
    }

    return result;
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const ResponseInfo& resp_info) {
    http::response<Body, http::basic_fields<Allocator>> result(resp_info.status, resp_info.version);
    result.body() = resp_info.body;
    result.content_length(result.body().size());
    result.keep_alive(resp_info.keep_alive);
    result.set(http::field::content_type, resp_info.content_type);

    if (resp_info.no_cache) {
        result.set(http::field::cache_control, "no-cache");
    }

    for (auto& [key, value] : resp_info.additional_fields) {
        result.set(key, value);
    }

    return result;
}

} // namespace detail

namespace file_resp {
using namespace std::literals;

template <typename Body, typename Allocator>
http::response<http::file_body, http::basic_fields<Allocator>>
MakeFileResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 const std::filesystem::path& file) {
    using FileResponseType = http::response<http::file_body, http::basic_fields<Allocator>>;

    http::file_body::value_type data;

    if (sys::error_code ec; data.open(file.string().data(), beast::file_mode::read, ec), ec) {
        throw std::runtime_error("Failed to open file"s);
    }

    std::string_view content_type = body_type::GetTypeByExtention(file.extension().string());
    FileResponseType result(http::status::ok, req.version());
    result.keep_alive(req.keep_alive());
    result.set(http::field::content_type, content_type);

    result.body() = std::move(data);
    result.prepare_payload();

    return result;
}

} // namespace file_resp

namespace txt_resp {

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache) {
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::bad_request;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache) {
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::not_found;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache) {
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::internal_server_error;

    return detail::MakeTextResponse(req, info);
}

} // namespace txt_resp

} // namespace resp_maker
