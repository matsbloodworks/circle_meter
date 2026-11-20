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
    bool onEvent(InputEvent) override { return false; }
    void setDiagText(const char* text) override;

private:
    void setLabel(lv_obj_t* label, char* last, size_t size, const char* text);

    lv_obj_t* m_root = nullptr;

    BigNumber m_egtNum;
    BigNumber m_afrNum;

    lv_obj_t* m_speedLabel = nullptr;
    lv_obj_t* m_cltLabel = nullptr;
    lv_obj_t* m_iatLabel = nullptr;
    lv_obj_t* m_matLabel = nullptr;
    lv_obj_t* m_mapLabel = nullptr;
    lv_obj_t* m_oilPressureLabel = nullptr;
    lv_obj_t* m_oilTempLabel = nullptr;
    lv_obj_t* m_iscvDutyLabel = nullptr;
    lv_obj_t* m_vBattLabel = nullptr;

    lv_obj_t* m_speedTitle = nullptr;
    lv_obj_t* m_cltTitle = nullptr;
    lv_obj_t* m_iatTitle = nullptr;

    char m_speedBuf[16]{};
    char m_cltBuf[16]{};
    char m_iatBuf[16]{};
    char m_matBuf[16]{};
    char m_mapBuf[16]{};
    char m_oilPressureBuf[16]{};
    char m_oilTempBuf[16]{};
    char m_iscvDutyBuf[16]{};
    char m_vBattBuf[16]{};

    lv_obj_t* m_diag = nullptr;
    DisplayFilter m_filter;

    char m_lastDiag[24]{};
};

}  // namespace cm::ui
