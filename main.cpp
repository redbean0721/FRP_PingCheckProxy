#include "crow.h"
#include <cpr/cpr.h>
#include <string>
#include <map>
#include <mutex>

std::map<std::string, int> visit_counts;
std::mutex counts_mutex;

struct VisitCounterMiddleware {
    struct context {};

    template <typename AllContext>
    void before_handle(crow::request& req, crow::response& res, context& ctx, AllContext& /*pc*/) {
        std::lock_guard<std::mutex> lock(counts_mutex);
        visit_counts[req.url]++;
    }

    template <typename AllContext>
    void after_handle(crow::request& req, crow::response& res, context& ctx, AllContext& /*pc*/) {}
};

struct PingCheckResponse {
    bool reject = false;
    bool unchange = true;
    std::string message;

    crow::json::wvalue to_json() const {
        crow::json::wvalue response;
        response["reject"] = reject;
        response["unchange"] = unchange;
        if (!message.empty()) { response["message"] = message; }
        return response;
    }
};

int main(int argc, char* argv[]) {
    const char* VERSION = "1.0.0";
    int port = 8000;
    std::string addr = "0.0.0.0";
    // --addr, -a
    // --port, -p
    // --help, -h
    // --version, -v
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--addr" || arg == "-a") {
                if (i + 1 < argc) {
                    addr = argv[++i];
                }
            } else if (arg == "--port" || arg == "-p") {
                if (i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                }
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --addr, -a <address>    Set the bind address (default: " << addr << ")\n";
                std::cout << "  --port, -p <port>       Set the bind port (default: " << port << ")\n";
                std::cout << "  --help, -h               Show this help message\n";
                std::cout << "  --version, -v            Show version information\n";
                return 0;
            } else if (arg == "--version" || arg == "-v") {
                std::cout << argv[0] << " version " << VERSION << "\n";
                return 0;
            }
        }
    }
    crow::App<VisitCounterMiddleware> app;

    CROW_ROUTE(app, "/")([VERSION](){
        crow::json::wvalue response;
        response["title"] = "Ping Check Service for TaiwanFRP Auth System";
        response["version"] = VERSION;
        response["status"] = "OK";
        return response;
    });

    CROW_ROUTE(app, "/status")([](){
        std::lock_guard<std::mutex> lock(counts_mutex);

        crow::json::wvalue response;
        crow::json::wvalue counters;

        // 將統計資料放入 counters 子物件
        for (auto const& [route, count] : visit_counts) {
            counters[route] = count;
        }

        // 包在 "counters" 鍵下
        response["counters"] = std::move(counters);
        response["status"] = "OK";

        return response;
    });

    CROW_ROUTE(app, "/ping_check").methods("GET"_method)([]() {
        return PingCheckResponse().to_json();
    });

    CROW_ROUTE(app, "/ping_check").methods("POST"_method)([](const crow::request& req) {
        auto resp = cpr::Post(
            cpr::Url { "https://taiwanfrp.ddns.net/ping_check" },
            cpr::Body { req.body },
            cpr::Header { { "Content-Type", "application/json" } },
            cpr::Timeout { 500 }   // 500 milliseconds timeout
        );

        if (resp.status_code == 200) {
            crow::response res(resp.text);
            res.set_header("Content-Type", "application/json");
            return res;
        } else {
            CROW_LOG_WARNING << "Upstream failed with status code: " << resp.status_code << " " << resp.error.message;
            return crow::response(PingCheckResponse().to_json());
        }
    });

    app.bindaddr(addr).port(port).multithreaded().run();
}