#pragma once
#include "TestPlatform.h"
// Explicit deterministic network fake: no sockets or TLS claims are made by this harness.
class HttpDownloader {
  public:  using ProgressCallback = std::function<void(size_t, size_t)>;
  using CancelCallback = std::function<bool()>;
  using DataCallback = std::function<bool(const uint8_t*, size_t)>;
  enum DownloadError {
    OK, HTTP_ERROR, FILE_ERROR, ABORTED
  };
  enum class Transport {
    ESP_HTTP, WOLFSSL
  };
  struct DownloadOptions {
    explicit DownloadOptions():preservePartial(false), resumePartial(false), bufferSize(0), transport(Transport::ESP_HTTP) {}
    bool preservePartial, resumePartial;
    CancelCallback shouldCancel;
    size_t bufferSize;
    Transport transport;
  };
  static inline std::vector<std::string> requests;
  static inline std::vector<Transport> transports;
  static inline std::vector<int> lockDepths;
  static inline std::vector<uint8_t> body = std::vector<uint8_t>(128, 0xa5);
  static inline DownloadError result = OK;
  static inline size_t chunkSize = 31;
  static inline std::function<void()> duringRequest;
  static DownloadError streamUrl(const std::string&url, const DataCallback&cb, ProgressCallback = nullptr, const std::string& = "", const std::string& = "", DownloadOptions opts = DownloadOptions()) {
    requests.push_back(url);
    transports.push_back(opts.transport);
    lockDepths.push_back(RenderLock::depth);
    if (duringRequest)duringRequest();
    if (result != OK)return result;
    for (size_t p = 0; p<body.size(); p += chunkSize) {
      if (opts.shouldCancel && opts.shouldCancel())return ABORTED;
      if (!cb(body.data()+p, std::min(chunkSize, body.size()-p)))return ABORTED;
    }
    return OK;
  }
  static DownloadError downloadToFile(const std::string&url, const std::string&path, ProgressCallback, bool*, const std::string&, const std::string&, DownloadOptions) {
    requests.push_back(url);
    if (result != OK)return result;
    HalFile f;
    if (!Storage.openFileForWrite("FAKE", path.c_str(), f))return FILE_ERROR;
    bool ok = f.write(body.data(), body.size()) == body.size();
    f.close();
    return ok?OK:FILE_ERROR;
  }
};
