// ============================================================
// Network worker thread — menjalankan PcClient (RO-MP protocol).
// SEMUA akses PcClient hanya dari thread ini. Hasilnya dipush
// sebagai GameEvent ke Mod::events untuk diproses game thread.
// ============================================================
#pragma once
#include <thread>
#include <atomic>
#include "romp_mod.h"

namespace rompmod {

class NetWorker {
public:
    static NetWorker& Get() {
        static NetWorker w;
        return w;
    }

    void Start();
    void Stop();

    // Dipanggil dari game thread / overlay.
    void RequestConnect(const std::string& host, uint16_t port,
                        const std::string& nick);
    void RequestDisconnect();
    void SendChat(const std::string& text);
    void SendPos(float x, float y, float z);

    bool IsRunning() const { return running_; }

private:
    NetWorker() = default;
    void ThreadMain();

    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};

    // Request (ditulis game thread, dibaca worker).
    std::mutex req_mutex_;
    bool req_connect_ = false;
    bool req_disconnect_ = false;
    std::string req_host_;
    uint16_t req_port_ = 0;
    std::string req_nick_;

    // Chat/pos queue (ditulis game thread).
    std::mutex out_mutex_;
    std::deque<std::string> chat_out_;
    struct Pos { float x, y, z; };
    std::deque<Pos> pos_out_;

    romppc::Client client_;
};

}  // namespace rompmod
