#include "http_server.h"

#include <iostream>

namespace http_server {

void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

SessionBase::SessionBase(tcp::socket&& socket) : stream_(std::move(socket)),
                                                 remote_endpoint_(socket.remote_endpoint()) {}

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(30s);
    http::async_read(stream_, buffer_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] size_t bytes_written) {
    if (close) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "write"sv);
    }
    Read();
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

} // namespace http_server

namespace url_decode {

int IntFromHex(char hex) {
    if ('0' <= hex && hex <= '9') {
        return hex - '0';
    }
    else if ('A' <= hex && hex <= 'F') {
        return hex - 'A' + 10;
    }
    else if ('a' <= hex && hex <= 'f') {
        return hex - 'a' + 10;
    }
    return -1;
}

int IntFrom2Hex(char first, char second) {
    return IntFromHex(first) * 16 + IntFromHex(second);
}

std::string DecodeURL(std::string_view url) {
    using namespace std::literals;

    size_t size = url.size();
    size_t i = 0;
    std::string result;
    result.reserve(url.size());

    while (i < size) {
        char cur = url.at(i);
        if (cur == '%') {
            if (i > size - 3) {
                throw std::runtime_error("Incorrect url"s);
            }
            int hex = IntFrom2Hex(url.at(i + 1), url.at(i + 2));
            if (hex < 0) {
                throw std::runtime_error("Incorrect hex number"s);
            }
            result.push_back(static_cast<char>(hex));
            i += 2;
        }
        else if (cur == '+') {
            result.push_back(' ');
        }
        else {
            result.push_back(cur);
        }
        ++i;
    }
    return result;
}

} // namespace url_decode
