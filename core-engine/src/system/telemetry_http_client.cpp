#include "system/telemetry_http_client.hpp"
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace netsim::system {

namespace {

TelemetryEndpoint parse_endpoint(const std::string& url) {
    constexpr const char* prefix = "http://";
    if (url.rfind(prefix, 0) != 0) {
        throw std::invalid_argument("Only http:// telemetry endpoints are supported");
    }

    const std::string without_scheme = url.substr(std::strlen(prefix));
    const std::size_t path_pos = without_scheme.find('/');
    const std::string host_port = without_scheme.substr(0, path_pos);
    const std::string path = path_pos == std::string::npos ? "/" : without_scheme.substr(path_pos);

    const std::size_t port_pos = host_port.rfind(':');
    if (port_pos == std::string::npos) {
        return {host_port, "80", path};
    }

    return {host_port.substr(0, port_pos), host_port.substr(port_pos + 1), path};
}

std::string escape_json(const std::string& input) {
    std::ostringstream out;
    for (char ch : input) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << ch; break;
        }
    }
    return out.str();
}

std::string to_json(const TelemetrySample& sample) {
    std::ostringstream body;
    body << "{"
         << "\"timestamp_us\":" << sample.timestamp_us << ","
         << "\"source_node\":\"" << escape_json(sample.source_node) << "\","
         << "\"dest_node\":\"" << escape_json(sample.dest_node) << "\","
         << "\"packet_size\":" << sample.packet_size << ","
         << "\"latency_ns\":" << sample.latency_ns << ","
         << "\"dropped_packets\":" << sample.dropped_packets << ","
         << "\"throughput_kbps\":" << sample.throughput_kbps
         << "}";
    return body.str();
}

bool send_all(int fd, const std::string& request) {
    std::size_t total_sent = 0;
    while (total_sent < request.size()) {
        const ssize_t sent = ::send(fd, request.data() + total_sent, request.size() - total_sent, 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += static_cast<std::size_t>(sent);
    }
    return true;
}

bool status_is_success(const std::string& response) {
    const std::size_t first_space = response.find(' ');
    if (first_space == std::string::npos || first_space + 3 >= response.size()) {
        return false;
    }
    const int status_code = std::stoi(response.substr(first_space + 1, 3));
    return status_code >= 200 && status_code < 300;
}

} // namespace

TelemetryHttpClient::TelemetryHttpClient(std::string endpoint_url)
    : endpoint_url_(std::move(endpoint_url)), endpoint_(parse_endpoint(endpoint_url_)) {}

bool TelemetryHttpClient::post_sample(const TelemetrySample& sample) const noexcept {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (::getaddrinfo(endpoint_.host.c_str(), endpoint_.port.c_str(), &hints, &result) != 0) {
        return false;
    }

    int sock = -1;
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) {
            continue;
        }

        if (::connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        ::close(sock);
        sock = -1;
    }

    ::freeaddrinfo(result);
    if (sock < 0) {
        return false;
    }

    const std::string body = to_json(sample);
    std::ostringstream request;
    request << "POST " << endpoint_.path << " HTTP/1.1\r\n"
            << "Host: " << endpoint_.host << ":" << endpoint_.port << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n"
            << "\r\n"
            << body;

    if (!send_all(sock, request.str())) {
        ::close(sock);
        return false;
    }

    char buffer[512]{};
    const ssize_t received = ::recv(sock, buffer, sizeof(buffer) - 1, 0);
    ::close(sock);

    if (received <= 0) {
        return false;
    }

    try {
        return status_is_success(std::string(buffer, static_cast<std::size_t>(received)));
    } catch (...) {
        return false;
    }
}

bool TelemetryHttpClient::post_metric_event(const MetricEvent& event, double throughput_kbps) const noexcept {
    TelemetrySample sample{
        event.timestamp_us,
        event.source_node,
        event.dest_node,
        event.packet_size,
        static_cast<double>(event.latency_ns),
        event.dropped ? 1U : 0U,
        throughput_kbps
    };
    return post_sample(sample);
}

} // namespace netsim::system
