// 新デザイン：EGT/AFR/Speed/CLT/IAT表示ページ
#pragma once

#include "big_number.h"
#include "display_filter.h"
#include "ipage.h"
#include "lambda_ring.h"

namespace cm::ui {

class PageData : public IPage {
public:
    const char* id() const override { return "PAGE_DATA"; }
    void onCreate(lv_obj_t* parent, const Config& cfg) override;
    void onShow() override;
    void onHide() override;
    void onUpdate(const Snapshot& snap, const Config& cfg) override;
    bool onEvent(InputEvent ev) override;
    void setDiagText(const char* text) override;
    /// SWR-106: 走行距離（m）。変わったときだけ文字を描き直す（DEC-08）
    void setDistance(uint32_t odoM, uint32_t tripM);

private:
    void setLabel(lv_obj_t* label, char* last, size_t size, const char* text);

    lv_obj_t* m_root = nullptr;
    lv_obj_t* m_odoLabel  = nullptr;  ///< SWR-106: "ODO 123456"
    lv_obj_t* m_tripLabel = nullptr;  ///< SWR-106: "TRIP 123.4"
    char m_lastOdo[20]{};
    char m_lastTrip[20]{};
    int32_t m_distGapPx = 0;  ///< ODO と TRIP の間（空白 2 文字分 - 5 px）

    BigNumber m_egtNum;
    BigNumber m_afrNum;
    BigNumber m_speedNum;
    BigNumber m_tpsNum;
    BigNumber m_cltNum;
    BigNumber m_oilPNum;
    BigNumber m_oilTNum;
    BigNumber m_mapNum;
    BigNumber m_matNum;
    BigNumber m_iscvNum;
    BigNumber m_vBattNum;
    BigNumber m_pwNum;
    BigNumber m_advNum;
    BigNumber m_baroNum;

    lv_obj_t* m_speedTitle = nullptr;
    lv_obj_t* m_cltTitle = nullptr;
    lv_obj_t* m_iatTitle = nullptr;

    // 右側の2列5行ラベル参照（色変更用）
    lv_obj_t* m_tpsTitleLabel = nullptr;
    lv_obj_t* m_iscvTitleLabel = nullptr;
    lv_obj_t* m_mapTitleLabel = nullptr;
    lv_obj_t* m_matTitleLabel = nullptr;
    lv_obj_t* m_oilPTitleLabel = nullptr;
    lv_obj_t* m_oilTTitleLabel = nullptr;
    lv_obj_t* m_pwTitleLabel = nullptr;
    lv_obj_t* m_advTitleLabel = nullptr;
    lv_obj_t* m_vBattTitleLabel = nullptr;
    lv_obj_t* m_spdTitleLabel = nullptr;
    lv_obj_t* m_cltTitleLabel = nullptr;

    char m_speedBuf[16]{};
    char m_cltBuf[16]{};
    char m_iatBuf[16]{};
    char m_matBuf[16]{};
    char m_mapBuf[16]{};
    char m_oilPressureBuf[16]{};
    char m_oilTempBuf[16]{};
    char m_iscvDutyBuf[16]{};
    char m_vBattBuf[16]{};
    char m_tpsBuf[16]{};
    char m_pwBuf[16]{};
    char m_advBuf[16]{};
    char m_baroBuf[16]{};

    DisplayFilter m_filter;

    lv_obj_t* m_rusEfiLabel = nullptr;
    lv_obj_t* m_mbwLabel = nullptr;
    lv_obj_t* m_topBar = nullptr;
    uint32_t m_blinkTimer = 0;
    bool m_blinkState = true;
    uint32_t m_updateCounter = 0;  ///< onUpdate の回数（25 ms 周期）。温度系 200 ms / その他 500 ms ごとに描く
};

}  // namespace cm::ui
