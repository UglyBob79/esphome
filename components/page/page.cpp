#include "page.h"

namespace esphome {
namespace page {

Page *Page::active = nullptr;

void ActionInput::activate() { this->page_->on_submit(); }

static constexpr int PAGE_WIDTH = 400;
static constexpr int DIVIDER_Y = 48;
static constexpr int ROW_START_Y = 62;
static constexpr int ROW_H = 38;
static constexpr int ROW_GAP = 20;
static constexpr int ROW_STEP = ROW_H + ROW_GAP;
static constexpr int TICKER_W = 72;
static constexpr int SWITCH_W = 60;
static constexpr int SWITCH_H = 30;
static constexpr int CONTENT_PAD = 20;

lv_obj_t *Page::make_ticker_box(lv_obj_t *parent, lv_obj_t **label_out) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_size(box, TICKER_W, ROW_H);
  lv_obj_set_style_bg_color(box, lv_color_make(255, 255, 255), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box, 2, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(box, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(box, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(box);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 3);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_label_set_text(lbl, "00");
  if (this->font_)
    lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

  *label_out = lbl;
  return box;
}

static lv_obj_t *make_row_container(lv_obj_t *col) {
  lv_obj_t *cont = lv_obj_create(col);
  lv_obj_set_size(cont, PAGE_WIDTH, ROW_H);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(cont, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(cont, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  // No layout — children positioned with lv_obj_align, not flex
  return cont;
}

void Page::build_ui() {
  if (!this->page_obj_)
    return;

  lv_obj_set_style_bg_color(this->page_obj_, lv_color_make(255, 255, 255), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(this->page_obj_, LV_OPA_COVER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

  // Title
  lv_obj_t *title_lbl = lv_label_create(this->page_obj_);
  lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 12);
  lv_label_set_text(title_lbl, this->title_ ? this->title_ : "");
  if (this->font_)
    lv_obj_set_style_text_font(title_lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title_lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

  // Divider
  lv_obj_t *divider = lv_obj_create(this->page_obj_);
  lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, DIVIDER_Y);
  lv_obj_set_size(divider, PAGE_WIDTH - 40, 2);
  lv_obj_set_style_bg_color(divider, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(divider, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(divider, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

  // Flex column — vertical stacking only; fixed height avoids LV_SIZE_CONTENT cascade
  int n = (int)this->rows_.size();
  int col_h = n * ROW_H + (n > 0 ? (n - 1) * ROW_GAP : 0);
  lv_obj_t *col = lv_obj_create(this->page_obj_);
  lv_obj_set_pos(col, 0, ROW_START_Y);
  lv_obj_set_size(col, PAGE_WIDTH, col_h);
  lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(col, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(col, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(col, ROW_GAP, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(col, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  for (auto &row : this->rows_) {
    if (row.type == RowType::TOGGLE) {
      lv_obj_t *cont = make_row_container(col);

      lv_obj_t *lbl = lv_label_create(cont);
      lv_obj_align(lbl, LV_ALIGN_LEFT_MID, CONTENT_PAD, 0);
      lv_label_set_text(lbl, row.toggle.label);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      lv_obj_t *sw = lv_switch_create(cont);
      lv_obj_set_size(sw, SWITCH_W, SWITCH_H);
      lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -CONTENT_PAD, 0);
      lv_group_remove_obj(sw);

      auto *inp = new ToggleInput(sw);
      inp->set_entity_id(row.toggle.entity_id);
      inp->set_page_active_ptr(&this->page_active_);
      this->inputs_.push_back(inp);

    } else if (row.type == RowType::TIME_INPUT) {
      lv_obj_t *cont = make_row_container(col);

      lv_obj_t *lbl = lv_label_create(cont);
      lv_obj_align(lbl, LV_ALIGN_LEFT_MID, CONTENT_PAD, 0);
      lv_label_set_text(lbl, row.time_input.label);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      // [HH] : [MM] right-aligned via lv_obj_align, no nested flex
      int m_x = PAGE_WIDTH - CONTENT_PAD - TICKER_W;  // 308
      int h_x = m_x - 16 - TICKER_W;                  // 220

      lv_obj_t *h_lbl = nullptr;
      lv_obj_t *h_box = this->make_ticker_box(cont, &h_lbl);
      lv_obj_set_pos(h_box, h_x, 0);
      lv_group_remove_obj(h_box);

      lv_obj_t *colon = lv_label_create(cont);
      lv_obj_set_pos(colon, h_x + TICKER_W + 3, 5);
      lv_label_set_text(colon, ":");
      if (this->font_)
        lv_obj_set_style_text_font(colon, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(colon, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      lv_obj_t *m_lbl = nullptr;
      lv_obj_t *m_box = this->make_ticker_box(cont, &m_lbl);
      lv_obj_set_pos(m_box, m_x, 0);
      lv_group_remove_obj(m_box);

      auto *h_inp = new NumberInput(h_box, h_lbl, 0, 23);
      h_inp->set_entity_id(row.time_input.entity_hour_id);
      h_inp->set_page_active_ptr(&this->page_active_);

      auto *m_inp = new NumberInput(m_box, m_lbl, 0, 59);
      m_inp->set_entity_id(row.time_input.entity_min_id);
      m_inp->set_page_active_ptr(&this->page_active_);

      this->inputs_.push_back(h_inp);
      this->inputs_.push_back(m_inp);

    } else if (row.type == RowType::SENSOR) {
      lv_obj_t *lbl = lv_label_create(col);
      lv_obj_set_size(lbl, PAGE_WIDTH, ROW_H);
      lv_label_set_text(lbl, "---");
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      auto *inp = new SensorLabel(lbl, row.sensor.format);
      inp->set_entity_id(row.sensor.entity_id);
      inp->set_page_active_ptr(&this->page_active_);
      this->inputs_.push_back(inp);

    } else if (row.type == RowType::LABEL) {
      lv_obj_t *lbl = lv_label_create(col);
      lv_obj_set_size(lbl, PAGE_WIDTH, ROW_H);
      lv_label_set_text(lbl, row.label_row.text ? row.label_row.text : "");
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

    } else if (row.type == RowType::CHECKBOX) {
      lv_obj_t *cont = make_row_container(col);

      // Label on the left — identical to toggle row
      lv_obj_t *lbl = lv_label_create(cont);
      lv_obj_align(lbl, LV_ALIGN_LEFT_MID, CONTENT_PAD, 0);
      lv_label_set_text(lbl, row.checkbox.label);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      // Plain square indicator on the right — avoids lv_checkbox text-space quirks
      lv_obj_t *cb = lv_obj_create(cont);
      lv_obj_set_size(cb, SWITCH_H, SWITCH_H);
      lv_obj_align(cb, LV_ALIGN_RIGHT_MID, -CONTENT_PAD, 0);
      lv_obj_clear_flag(cb, LV_OBJ_FLAG_SCROLLABLE);
      lv_group_remove_obj(cb);
      lv_obj_set_style_radius(cb, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(cb, 2, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(cb, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(cb, lv_color_make(255, 255, 255), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(cb, LV_OPA_COVER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(cb, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(cb, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_CHECKED);

      auto *inp = new CheckboxInput(cb);
      inp->set_entity_id(row.checkbox.entity_id);
      inp->set_page_active_ptr(&this->page_active_);
      this->inputs_.push_back(inp);
    }
  }

  if (this->submit_label_) {
    lv_obj_t *btn = lv_obj_create(this->page_obj_);
    lv_group_remove_obj(btn);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_size(btn, 160, 44);
    lv_obj_set_style_bg_color(btn, lv_color_make(255, 255, 255), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 2, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_obj_align(btn_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(btn_lbl, this->submit_label_);
    if (this->font_)
      lv_obj_set_style_text_font(btn_lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(btn_lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

    auto *save_inp = new ActionInput(btn, this);
    this->inputs_.push_back(save_inp);
  }

  lv_obj_add_event_cb(this->page_obj_, on_load_event_, LV_EVENT_SCREEN_LOADED, this);
  lv_obj_add_event_cb(this->page_obj_, on_unload_event_, LV_EVENT_SCREEN_UNLOADED, this);
}

void Page::on_page_load() {
  ESP_LOGD(PAGE_TAG, "on_page_load: %s (inputs: %d)", this->title_ ? this->title_ : "?", (int)this->inputs_.size());
  Page::active = this;
  this->page_active_ = true;
  this->focused_idx_ = 0;
  this->editing_ = false;
  this->apply_page_active_state_();
}

void Page::apply_page_active_state_() {
  for (auto *inp : this->inputs_)
    inp->sync_from_cache();
  if (!this->inputs_.empty())
    this->focus(this->focused_idx_);
}

void Page::on_page_unload() {
  if (Page::active == this)
    Page::active = nullptr;
  this->page_active_ = false;
  this->editing_ = false;
}

void Page::on_confirm() {
  if (this->inputs_.empty())
    return;
  auto *inp = this->inputs_[this->focused_idx_];
  if (this->editing_) {
    this->editing_ = false;
    inp->set_editing(false);
    inp->set_focused(true);
  } else if (inp->is_editable()) {
    this->editing_ = true;
    inp->set_editing(true);
  } else if (inp->is_activatable()) {
    inp->activate();
    if (!this->submit_label_)
      inp->save();
  }
}

void Page::on_up() {
  if (this->inputs_.empty())
    return;
  if (this->editing_)
    this->inputs_[this->focused_idx_]->increment();
  else
    this->navigate(-1);
}

void Page::on_down() {
  if (this->inputs_.empty())
    return;
  if (this->editing_)
    this->inputs_[this->focused_idx_]->decrement();
  else
    this->navigate(1);
}

void Page::on_submit() {
  for (auto *inp : this->inputs_)
    inp->save();
}

}  // namespace page
}  // namespace esphome
