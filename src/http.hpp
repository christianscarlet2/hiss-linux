// Tiny libcurl wrapper — JSON in / JSON out, Bearer auth. Header-only.
#pragma once
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <mutex>

namespace hiss {

using json = nlohmann::json;

class Http {
public:
    Http(std::string base, std::string token) : base_(std::move(base)), token_(std::move(token)) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~Http() { curl_global_cleanup(); }

    // GET <base><path> -> parsed JSON (throws on transport error; returns null on non-JSON).
    json get(const std::string& path) { return request("GET", path, ""); }

    // POST JSON body to <base><path>.
    json post(const std::string& path, const json& body) {
        return request("POST", path, body.dump());
    }

    // A GraphQL query against an absolute-or-relative endpoint.
    json graphql(const std::string& endpoint, const std::string& query, const json& variables = json::object()) {
        json body = {{"query", query}, {"variables", variables}};
        return request("POST", endpoint, body.dump());
    }

    long lastStatus() const { return status_; }

private:
    static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* s = static_cast<std::string*>(userdata);
        s->append(ptr, size * nmemb);
        return size * nmemb;
    }

    json request(const std::string& method, const std::string& path, const std::string& body) {
        std::lock_guard<std::mutex> lock(mu_);
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl init failed");

        std::string url = path.rfind("http", 0) == 0 ? path : base_ + path;
        std::string buf;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("User-Agent: Hiss-Linux/1.0"));
        std::string auth;
        if (!token_.empty()) { auth = "Authorization: Bearer " + token_; headers = curl_slist_append(headers, auth.c_str()); }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        CURLcode rc = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (rc != CURLE_OK) throw std::runtime_error(std::string("curl: ") + curl_easy_strerror(rc));
        if (buf.empty()) return json(nullptr);
        try { return json::parse(buf); } catch (...) { return json(nullptr); }
    }

    std::string base_, token_;
    long status_ = 0;
    std::mutex mu_;
};

} // namespace hiss
