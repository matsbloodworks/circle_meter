#include "can_rx_task.h"

#include <Arduino.h>

#include "link_decoder.h"
#include "rusefi_decoder.h"

namespace cm {

namespace {

struct RxContext {
    CanDriver* can;
    SignalStore* store;
    const Config* cfg;
};
RxContext g_rx;
link::LinkDecoder g_link;  // SWR-103: 車速のために車輪速度を持つ。このタスクだけが使う
CanDriver* g_healthCan = nullptr;

void canRxTask(void*) {
    twai_message_t msg;
    for (;;) {
        if (!g_rx.can->receive(msg, 100)) {
            continue;
        }
        const uint32_t now = millis();
        rusefi::DecodeResult r;
        if (g_rx.cfg->ecuType == static_cast<uint8_t>(EcuType::Link)) {
            // SWR-104: Link のときは rusEFI のデコーダを呼ばない（ID が重なっても誤読しない）
            if (!msg.extd) {
                r = g_link.decode(msg.identifier, msg.data, msg.data_length_code, g_rx.cfg->linkCanId);
            }
        } else {
            r = rusefi::decodeFrame(msg.identifier, msg.data, msg.data_length_code, g_rx.cfg->canBaseId);
        }
        if (!r.accepted && !msg.extd && g_rx.cfg->ecuType != static_cast<uint8_t>(EcuType::Link)) {
            // SWR-11: rusEFI WBO が自分で送る状態・温度（標準 ID 0x190 / 0x191）
            r = rusefi::decodeWboFrame(msg.identifier, msg.data, msg.data_length_code);
        }

        if (!r.accepted) {
            if (msg.data_length_code < rusefi::kFrameDlc) {
                g_rx.can->mutableStats().badDlc++;
            } else {
                g_rx.can->mutableStats().unknownId++;
            }
            continue;
        }
        g_rx.can->mutableStats().rxFrames++;

        for (uint8_t i = 0; i < r.count; ++i) {
            const SignalId id = r.signals[i].id;
            const float v     = r.signals[i].value;

            // SYS-42 / RSK-09: rusEFI が未構成センサに送る 0 をストアへ入れない。
            // 入れなければ鮮度が Fresh にならず、UI は自動的に "--" を出す。
            if ((id == SignalId::Lambda1 || id == SignalId::Lambda2) && !rusefi::isLambdaValid(v)) {
                continue;
            }
            if ((id == SignalId::Egt1 || id == SignalId::Egt2) && !rusefi::isEgtValid(v)) {
                continue;
            }
            g_rx.store->update(id, v, now);
        }
    }
}

void canHealthTask(void*) {
    for (;;) {
        g_healthCan->poll(millis());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

}  // namespace

void startCanRxTask(CanDriver& can, SignalStore& store, const Config& cfg) {
    g_rx = RxContext{&can, &store, &cfg};
    xTaskCreatePinnedToCore(canRxTask, "can_rx", 4096, nullptr, 10, nullptr, 0);
}

void startCanHealthTask(CanDriver& can) {
    g_healthCan = &can;
    xTaskCreatePinnedToCore(canHealthTask, "can_health", 3072, nullptr, 5, nullptr, 0);
}

}  // namespace cm
