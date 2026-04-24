// YarrTriggerIDSyncDataCollector.cc
//
// A data collector that synchronises sub-events from multiple YARR producers
// (and a TLU producer) on the TLU hardware trigger ID, which every YARR event
// now carries as its EUDAQ TriggerN number via SetTriggerN(tlu_id).
//
// Derived from the upstream TriggerIDSyncDataCollector with one key addition:
//
//   TIMEOUT-BASED FILLER INJECTION
//   ───────────────────────────────
//   If a producer has not delivered an event for a given trigger_n within
//   SYNC_TIMEOUT_MS milliseconds, the collector assumes the event is
//   permanently missing (chip-side data loss) and injects a synthetic filler
//   sub-event so the remaining producers are not blocked forever.
//
//   Filler sub-events carry:
//     IS_FILLER = 1            (EUDAQ tag)
//     FILLER_TLU_ID = <n>      (EUDAQ tag, the missing trigger number)
//     TriggerN = <n>           (so downstream tools can count the gap)
//
// Configuration keys (set in the run-control config file):
//   DISABLE_PRINT   (int, default 0)  — suppress per-event console output
//   SYNC_TIMEOUT_MS (int, default 500) — ms before a missing event is filled

#include "eudaq/DataCollector.hh"
#include "eudaq/Event.hh"

#include <chrono>
#include <map>
#include <set>
#include <deque>
#include <mutex>
#include <string>

namespace eudaq {

class YarrTriggerIDSyncDataCollector : public DataCollector {
public:
    YarrTriggerIDSyncDataCollector(const std::string &name,
                                   const std::string &rc)
        : DataCollector(name, rc) {}

    void DoConnect(ConnectionSPC id)    override;
    void DoDisconnect(ConnectionSPC id) override;
    void DoConfigure()                  override;
    void DoReset()                      override;
    void DoReceive(ConnectionSPC id, EventSP ev) override;

    static const uint32_t m_id_factory =
        cstr2hash("YarrTriggerIDSyncDataCollector");

private:
    // ── Types ────────────────────────────────────────────────────────────────

    struct TimestampedEvent {
        EventSPC ev;
        std::chrono::steady_clock::time_point arrived;
    };

    // ── State ────────────────────────────────────────────────────────────────

    mutable std::mutex m_mtx;
    // Per-connection queue of pending events, ordered by TriggerN.
    std::map<ConnectionSPC, std::deque<TimestampedEvent>> m_conn_evque;
    std::set<ConnectionSPC> m_conn_inactive;

    uint32_t m_noprint{0};
    std::chrono::milliseconds m_timeout{500};

    // ── Helpers ──────────────────────────────────────────────────────────────

    /// Build a synthetic filler sub-event for @p conn at trigger number @p tn.
    static EventSP makeFiller(ConnectionSPC conn, uint32_t tn) {
        auto filler = Event::MakeUnique("YarrFiller");
        filler->SetTag("IS_FILLER",     1);
        filler->SetTag("FILLER_TLU_ID", tn);
        filler->SetTag("FILLER_PRODUCER", conn->GetName());
        filler->SetTriggerN(tn);
        filler->SetFlagTrigger();
        return filler;
    }

    /// Try to assemble and write one synchronised packet.
    /// Must be called with m_mtx held.
    void tryFlush() {
        auto now = std::chrono::steady_clock::now();

        // Find the minimum TriggerN available across all active connections.
        uint32_t target_tn = UINT32_MAX;
        for (auto &[conn, q] : m_conn_evque) {
            if (m_conn_inactive.count(conn)) continue;
            if (q.empty()) {
                // This producer has nothing yet — check timeout.
                // We can only act if we know what trigger to fill.
                // target_tn will remain UINT32_MAX → handled below.
                continue;
            }
            uint32_t tn = q.front().ev->GetTriggerN();
            if (tn < target_tn) target_tn = tn;
        }

        if (target_tn == UINT32_MAX) return; // no data anywhere yet

        // For each connection, check whether it has the target trigger or
        // whether the timeout has expired (→ inject filler).
        bool ready = true;
        for (auto &[conn, q] : m_conn_evque) {
            if (m_conn_inactive.count(conn)) continue;
            if (q.empty()) {
                // No event at all for this connection yet.
                // Check if any other queue's oldest event is old enough.
                // Use the oldest arrival across non-empty queues as reference.
                std::chrono::steady_clock::time_point oldest = now;
                for (auto &[c2, q2] : m_conn_evque) {
                    if (!q2.empty() && q2.front().arrived < oldest)
                        oldest = q2.front().arrived;
                }
                if (now - oldest >= m_timeout) {
                    EUDAQ_WARN("YarrTriggerIDSyncDataCollector: timeout waiting for "
                               + conn->GetName() + " at trigger " + std::to_string(target_tn)
                               + " — injecting filler");
                    q.push_front({makeFiller(conn, target_tn), now});
                } else {
                    ready = false;
                }
            } else if (q.front().ev->GetTriggerN() != target_tn) {
                // This queue's front is a *higher* trigger number.
                // The target must have been lost for this producer.
                auto &front_ts = q.front().arrived;
                if (now - front_ts >= m_timeout) {
                    EUDAQ_WARN("YarrTriggerIDSyncDataCollector: timeout for "
                               + conn->GetName() + " at trigger " + std::to_string(target_tn)
                               + " (front=" + std::to_string(q.front().ev->GetTriggerN())
                               + ") — injecting filler");
                    q.push_front({makeFiller(conn, target_tn), now});
                } else {
                    ready = false;
                }
            }
            // else: q.front().GetTriggerN() == target_tn → present, good.
        }

        if (!ready) return;

        // All connections have the target trigger (real or filler) → build packet.
        auto ev_sync = Event::MakeUnique("YarrTriggerIDSyncOnline");
        ev_sync->SetFlagPacket();
        ev_sync->SetTriggerN(target_tn);

        bool any_filler = false;
        for (auto &[conn, q] : m_conn_evque) {
            if (m_conn_inactive.count(conn)) continue;
            auto &front = q.front().ev;
            if (front->GetTag("IS_FILLER", 0) == 1) any_filler = true;
            ev_sync->AddSubEvent(front);
            q.pop_front();
        }
        if (any_filler)
            ev_sync->SetTag("HAS_FILLER", 1);

        // Clean up fully-drained inactive connections.
        if (!m_conn_inactive.empty()) {
            std::set<ConnectionSPC> done;
            for (auto &conn : m_conn_inactive) {
                if (m_conn_evque.count(conn) && m_conn_evque[conn].empty()) {
                    m_conn_evque.erase(conn);
                    done.insert(conn);
                }
            }
            for (auto &conn : done) m_conn_inactive.erase(conn);
        }

        if (!m_noprint) ev_sync->Print(std::cout);
        WriteEvent(std::move(ev_sync));

        // Recurse: there may be more complete triggers waiting.
        tryFlush();
    }
};

// ── Registration ─────────────────────────────────────────────────────────────

namespace {
    auto dummy0 = Factory<DataCollector>::Register<
        YarrTriggerIDSyncDataCollector,
        const std::string &, const std::string &>(
        YarrTriggerIDSyncDataCollector::m_id_factory);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void YarrTriggerIDSyncDataCollector::DoConnect(ConnectionSPC idx) {
    std::unique_lock<std::mutex> lk(m_mtx);
    m_conn_evque[idx].clear();
    m_conn_inactive.erase(idx);
    EUDAQ_INFO("YarrTriggerIDSyncDataCollector: " + idx->GetName() + " connected");
}

void YarrTriggerIDSyncDataCollector::DoDisconnect(ConnectionSPC idx) {
    std::unique_lock<std::mutex> lk(m_mtx);
    m_conn_inactive.insert(idx);
    if (m_conn_inactive.size() == m_conn_evque.size()) {
        m_conn_inactive.clear();
        m_conn_evque.clear();
    }
    EUDAQ_INFO("YarrTriggerIDSyncDataCollector: " + idx->GetName() + " disconnected");
}

void YarrTriggerIDSyncDataCollector::DoConfigure() {
    m_noprint = 0;
    m_timeout = std::chrono::milliseconds(500);
    auto conf = GetConfiguration();
    if (conf) {
        conf->Print();
        m_noprint  = conf->Get("DISABLE_PRINT",   0);
        m_timeout  = std::chrono::milliseconds(
                         conf->Get("SYNC_TIMEOUT_MS", 500));
    }
    EUDAQ_INFO("YarrTriggerIDSyncDataCollector configured: timeout="
               + std::to_string(m_timeout.count()) + " ms");
}

void YarrTriggerIDSyncDataCollector::DoReset() {
    std::unique_lock<std::mutex> lk(m_mtx);
    m_noprint = 0;
    m_timeout = std::chrono::milliseconds(500);
    m_conn_evque.clear();
    m_conn_inactive.clear();
}

// ── Main receive path ─────────────────────────────────────────────────────────

void YarrTriggerIDSyncDataCollector::DoReceive(ConnectionSPC idx, EventSP evsp) {
    if (!evsp->IsFlagTrigger()) {
        EUDAQ_THROW("YarrTriggerIDSyncDataCollector: received event without trigger flag");
    }

    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_conn_evque[idx].push_back({evsp, std::chrono::steady_clock::now()});
        tryFlush();
    }
}

} // namespace eudaq
