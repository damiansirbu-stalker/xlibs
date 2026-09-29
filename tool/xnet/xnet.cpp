// xnet: xlibs's native companion, launched from the game over a Lua FFI CreateProcess. It does the
// out-of-sandbox work the LuaJIT sandbox forbids. WinHTTP + static CRT, so `cl /MT` yields a single
// exe that depends only on system DLLs and never trips the Go-runtime antivirus heuristic.
//
// Two modes:
//   xnet -url U [-method M] [-header-file H] [-body-file B] [-out O]   generic HTTP(S) request
//   xnet github-put -repo R -path P -file F -token-file T [-branch] [-message]   commit a file to a repo
//
// The auth token always arrives from a file, never on the command line, so it cannot leak through the
// process arguments.
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

static const wchar_t *API_HOST = L"api.github.com";

// widen: UTF-8/ANSI argv to the wide strings WinHTTP takes.
static std::wstring widen(const std::string &s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], n);
    return out;
}

// readFile: whole file as bytes, false when it cannot be opened.
static bool readFile(const std::string &path, std::string &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}

// writeFile: response body to disk, false on a write failure.
static bool writeFile(const std::string &path, const std::string &data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(data.data(), (std::streamsize)data.size());
    return f.good();
}

// base64: standard encoding of the content bytes for the GitHub Contents payload.
static std::string base64(const std::string &in) {
    static const char *T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8) | (unsigned char)in[i + 2];
        out += T[(v >> 18) & 63]; out += T[(v >> 12) & 63]; out += T[(v >> 6) & 63]; out += T[v & 63];
    }
    if (i < in.size()) {
        unsigned v = (unsigned char)in[i] << 16;
        bool two = (i + 1 < in.size());
        if (two) v |= (unsigned char)in[i + 1] << 8;
        out += T[(v >> 18) & 63]; out += T[(v >> 12) & 63];
        out += two ? T[(v >> 6) & 63] : '='; out += '=';
    }
    return out;
}

// jsonEscape: a JSON string body (message and the base64 content are the only fields that carry text).
static std::string jsonEscape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c & 0xff); out += b; }
                else out += c;
        }
    }
    return out;
}

// firstStringField: the value of the first "key":"value" pair, or "" when the key is absent. Enough to
// lift the blob sha off the Contents GET response, whose top-level sha comes before any nested one.
static std::string firstStringField(const std::string &body, const std::string &key) {
    std::string needle = "\"" + key + "\"";
    size_t k = body.find(needle);
    if (k == std::string::npos) return "";
    size_t colon = body.find(':', k + needle.size());
    if (colon == std::string::npos) return "";
    size_t open = body.find('"', colon + 1);
    if (open == std::string::npos) return "";
    size_t close = body.find('"', open + 1);
    if (close == std::string::npos) return "";
    return body.substr(open + 1, close - open - 1);
}

// httpRequest: one WinHTTP call the caller fully composes. TLS runs through the OS store. Returns the
// HTTP status in status and the body in respBody; false only when the transport itself fails.
static bool httpRequest(const std::wstring &method, const std::wstring &url,
                        const std::wstring &headers, const std::string &body,
                        DWORD &status, std::string &respBody) {
    status = 0;
    respBody.clear();

    URL_COMPONENTS uc = {0};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {0}, path[4096] = {0};
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 4096;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    bool secure = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    HINTERNET session = WinHttpOpen(L"xnet", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;

    HINTERNET connect = WinHttpConnect(session, host, uc.nPort, 0);
    if (!connect) { WinHttpCloseHandle(session); return false; }

    DWORD reqFlags = secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, method.c_str(), path, nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, reqFlags);
    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return false; }

    bool ok = true;
    if (!headers.empty())
        WinHttpAddRequestHeaders(request, headers.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    void *bodyPtr = body.empty() ? WINHTTP_NO_REQUEST_DATA : (void *)body.data();
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            bodyPtr, (DWORD)body.size(), (DWORD)body.size(), 0)) ok = false;
    if (ok && !WinHttpReceiveResponse(request, nullptr)) ok = false;

    if (ok) {
        DWORD code = 0, len = sizeof(code);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &code, &len, WINHTTP_NO_HEADER_INDEX);
        status = code;
        DWORD avail = 0;
        do {
            avail = 0;
            if (!WinHttpQueryDataAvailable(request, &avail)) break;
            if (avail == 0) break;
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request, &chunk[0], avail, &read)) break;
            respBody.append(chunk.data(), read);
        } while (avail > 0);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

// flag: the value following -name in argv, or "" when absent. Single-dash, space-separated, matching
// what xnet.script composes.
static std::string flag(int argc, char **argv, int from, const std::string &name) {
    for (int i = from; i + 1 < argc; i++)
        if (name == argv[i]) return argv[i + 1];
    return "";
}

// headerFileToBlock: a file of 'Key: Value' lines to the CRLF-joined block WinHttpAddRequestHeaders wants.
static std::wstring headerFileToBlock(const std::string &path) {
    std::string data;
    if (!readFile(path, data)) return L"";
    std::wstring block;
    size_t start = 0;
    while (start < data.size()) {
        size_t nl = data.find('\n', start);
        std::string line = data.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.find(':') != std::string::npos) { block += widen(line); block += L"\r\n"; }
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return block;
}

// genericRequest: one HTTP(S) call the caller fully composes.
static int genericRequest(int argc, char **argv) {
    std::string method = flag(argc, argv, 1, "-method");
    std::string url = flag(argc, argv, 1, "-url");
    std::string headerFile = flag(argc, argv, 1, "-header-file");
    std::string bodyFile = flag(argc, argv, 1, "-body-file");
    std::string out = flag(argc, argv, 1, "-out");
    if (method.empty()) method = "POST";
    if (url.empty()) { std::cerr << "xnet: -url is required\n"; return 2; }

    std::string body;
    if (!bodyFile.empty() && !readFile(bodyFile, body)) { std::cerr << "xnet: body-file\n"; return 2; }
    std::wstring headers = headerFile.empty() ? L"" : headerFileToBlock(headerFile);

    DWORD status = 0;
    std::string resp;
    if (!httpRequest(widen(method), widen(url), headers, body, status, resp)) {
        std::cerr << "xnet: send\n";
        return 1;
    }
    if (!out.empty() && !writeFile(out, resp)) { std::cerr << "xnet: out\n"; return 1; }
    std::cout << status << "\n";
    return (status >= 200 && status < 300) ? 0 : 1;
}

// githubPut: commit one file to a repo through the GitHub Contents API, creating or updating it. The
// caller supplies the content; xnet base64-encodes it, reads the current sha for an update, and PUTs.
static int githubPut(int argc, char **argv) {
    std::string repo = flag(argc, argv, 2, "-repo");
    std::string path = flag(argc, argv, 2, "-path");
    std::string file = flag(argc, argv, 2, "-file");
    std::string tokenFile = flag(argc, argv, 2, "-token-file");
    std::string branch = flag(argc, argv, 2, "-branch");
    std::string message = flag(argc, argv, 2, "-message");
    if (message.empty()) message = "update";
    if (repo.empty() || path.empty() || file.empty() || tokenFile.empty()) {
        std::cerr << "github-put: -repo, -path, -file, -token-file are required\n";
        return 2;
    }

    std::string token, content;
    if (!readFile(tokenFile, token)) { std::cerr << "github-put: token-file\n"; return 2; }
    while (!token.empty() && (token.back() == '\n' || token.back() == '\r' || token.back() == ' ')) token.pop_back();
    if (!readFile(file, content)) { std::cerr << "github-put: file\n"; return 2; }

    std::wstring headers = widen("Authorization: Bearer " + token) + L"\r\n" +
                           L"Accept: application/vnd.github+json\r\n" +
                           L"User-Agent: xnet\r\n";
    std::wstring baseUrl = std::wstring(L"https://") + API_HOST + L"/repos/" + widen(repo) + L"/contents/" + widen(path);

    std::wstring getUrl = baseUrl;
    if (!branch.empty()) getUrl += L"?ref=" + widen(branch);
    DWORD status = 0;
    std::string getBody;
    std::string sha;
    if (httpRequest(L"GET", getUrl, headers, "", status, getBody) && status == 200)
        sha = firstStringField(getBody, "sha");

    std::string payload = "{\"message\":\"" + jsonEscape(message) + "\",\"content\":\"" + base64(content) + "\"";
    if (!sha.empty()) payload += ",\"sha\":\"" + jsonEscape(sha) + "\"";
    if (!branch.empty()) payload += ",\"branch\":\"" + jsonEscape(branch) + "\"";
    payload += "}";

    std::wstring putHeaders = headers + L"Content-Type: application/json\r\n";
    std::string putBody;
    if (!httpRequest(L"PUT", baseUrl, putHeaders, payload, status, putBody)) {
        std::cerr << "github-put: send\n";
        return 1;
    }
    std::cout << status << "\n";
    if (status < 200 || status >= 300) {
        std::cerr << "github-put: " << putBody << "\n";
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "github-put") return githubPut(argc, argv);
    return genericRequest(argc, argv);
}
