// ============================================================
// Network worker — implementasi.
// ============================================================
#include "net_worker.h"
#include <chrono>

namespace rompmod {

void NetWorker::Start() {
    if (running_) return;
    stop_ = false;
    running_ = true;
    thread_ = std::thread(&NetWorker::ThreadMain, this);
}

void NetWorker::Stop() {
    if (!running_) return;
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    running_ = false;
}

void NetWorker::RequestConnect(const std::string& host, uint16_t port,
                               const std::string& nick) {
    std::lock_guard<std::mutex> l(req_mutex_);
    req_connect_ = true;
    req_disconnect_ = false;
    req_host_ = host; req_port_ = port; req_nick_ = nick;
}

void NetWorker::RequestDisconnect() {
    std::lock_guard<std::mutex> l(req_mutex_);
    req_disconnect_ = true;
    req_connect_ = false;
}

void NetWorker::SendChat(const std::string& text) {
    std::lock_guard<std::mutex> l(out_mutex_);
    if (chat_out_.size() < 16) chat_out_.push_back(text);
}

void NetWorker::SendPos(float x, float y, float z) {
    std::lock_guard<std::mutex> l(out_mutex_);
    if (pos_out_.size() < 4) pos_out_.push_back({x, y, z});
}

void NetWorker::ThreadMain() {
    Mod& mod = Mod::Get();
    auto& players = client_.Players();  // untuk nama (read-only di sini)
    (void)players;

    while (!stop_) {
        // --- proses request ---
        {
            std::string h; uint16_t p = 0; std::string n;
            bool do_connect = false, do_disconnect = false;
            {
                std::lock_guard<std::mutex> l(req_mutex_);
                do_connect = req_connect_; do_disconnect = req_disconnect_;
                h = req_host_; p = req_port_; n = req_nick_;
                req_connect_ = req_disconnect_ = false;
            }
            if (do_disconnect) client_.Disconnect("Keluar.");
            if (do_connect && !client_.Connect(h, p, n)) {
                GameEvent e;
                e.type = GameEvent::Disconnected;
                e.text = "Gagal connect ke " + h + ":" + std::to_string(p);
                mod.PushEvent(std::move(e));
            }
        }

        // --- kirim chat/pos antri ---
        {
            std::lock_guard<std::mutex> l(out_mutex_);
            while (!chat_out_.empty()) {
                client_.SendChat(chat_out_.front());
                chat_out_.pop_front();
            }
            // kirim posisi terbaru saja
            if (!pos_out_.empty()) {
                Pos pp = pos_out_.back();
                pos_out_.clear();
                client_.SendPos(pp.x, pp.y, pp.z);
            }
        }

        // --- pump network ---
        client_.Poll();
        while (client_.HasEvent()) {
            romppc::Event ev = client_.NextEvent();
            GameEvent ge;
            switch (ev.type) {
                case romppc::EventType::Connected:
                    ge.type = GameEvent::Connected;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::Rejected:
                    ge.type = GameEvent::Rejected;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::Disconnected:
                    ge.type = GameEvent::Disconnected;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::Chat: {
                    ge.type = GameEvent::Chat;
                    ge.player_id = ev.from;
                    ge.kind = ev.kind;
                    // prefix nama di game thread (punya daftar pemain)
                    ge.text = ev.text;
                    break;
                }
                case romppc::EventType::PlayerJoin:
                    ge.type = GameEvent::PlayerJoin;
                    ge.player_id = ev.from;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::PlayerLeave:
                    ge.type = GameEvent::PlayerLeave;
                    ge.player_id = ev.from;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::PlayerList: {
                    ge.type = GameEvent::PlayerList;
                    // kirim sebagai beberapa event join (sederhana)
                    for (auto& pi : ev.players) {
                        GameEvent j;
                        j.type = GameEvent::PlayerJoin;
                        j.player_id = pi.id;
                        j.text = pi.name;
                        mod.PushEvent(std::move(j));
                    }
                    continue;
                }
                case romppc::EventType::NuiMessage:
                    ge.type = GameEvent::NuiMessage;
                    ge.text = ev.text;
                    break;
                case romppc::EventType::NuiFocus:
                    continue;
            }
            mod.PushEvent(std::move(ge));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    client_.Disconnect();
}

}  // namespace rompmod
