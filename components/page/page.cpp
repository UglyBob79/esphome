#include "page.h"

namespace esphome {
namespace page {

Page *Page::active = nullptr;

void ActionInput::activate() { this->page_->on_submit(); }

static constexpr int PAGE_WIDTH = 400;
static constexpr int DIVIDER_Y = 48;
static constexpr int ROW_START_Y = 62;
static constexpr int ROW_STEP = 58;
static constexpr int ROW_H = 38;
static constexpr int TICKER_W = 72;
static constexpr int TICKER_HOUR_X = 185;
static constexpr int COLON_X = 261;
static constexpr int TICKER_MIN_X = 277;
static constexpr int SWITCH_W = 60;
static constexpr int SWITCH_H = 30;

lv_obj_t *Page::make_ticker_box(lv_obj_t *parent, int x, int y, lv_obj_t **label_out) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_pos(box, x, y);
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

  // Rows
  int row_y = ROW_START_Y;
  for (auto &row : this->rows_) {
    if (row.type == RowType::TOGGLE) {
      lv_obj_t *lbl = lv_label_create(this->page_obj_);
      lv_obj_set_pos(lbl, 20, row_y + 5);
      lv_label_set_text(lbl, row.toggle.label);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      lv_obj_t *sw = lv_switch_create(this->page_obj_);
      lv_obj_set_pos(sw, PAGE_WIDTH - 20 - SWITCH_W, row_y + 4);
      lv_obj_set_size(sw, SWITCH_W, SWITCH_H);
      lv_group_remove_obj(sw);

      auto *inp = new ToggleInput(sw);
      inp->set_entity_id(row.toggle.entity_id);
      inp->set_page_active_ptr(&this->page_active_);
      this->inputs_.push_back(inp);

    } else if (row.type == RowType::TIME_INPUT) {
      lv_obj_t *lbl = lv_label_create(this->page_obj_);
      lv_obj_set_pos(lbl, 20, row_y + 5);
      lv_label_set_text(lbl, row.time_input.label);
      if (this->font_)
        lv_obj_set_style_text_font(lbl, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      lv_obj_t *h_lbl = nullptr;
      lv_obj_t *h_box = this->make_ticker_box(this->page_obj_, TICKER_HOUR_X, row_y, &h_lbl);
      lv_group_remove_obj(h_box);

      lv_obj_t *colon = lv_label_create(this->page_obj_);
      lv_obj_set_pos(colon, COLON_X, row_y + 5);
      lv_label_set_text(colon, ":");
      if (this->font_)
        lv_obj_set_style_text_font(colon, this->font_, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(colon, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);

      lv_obj_t *m_lbl = nullptr;
      lv_obj_t *m_box = this->make_ticker_box(this->page_obj_, TICKER_MIN_X, row_y, &m_lbl);
      lv_group_remove_obj(m_box);

      auto *h_inp = new NumberInput(h_box, h_lbl, 0, 23);
      h_inp->set_entity_id(row.time_input.entity_hour_id);
      h_inp->set_page_active_ptr(&this->page_active_);

      auto *m_inp = new NumberInput(m_box, m_lbl, 0, 59);
      m_inp->set_entity_id(row.time_input.entity_min_id);
      m_inp->set_page_active_ptr(&this->page_active_);

      this->inputs_.push_back(h_inp);
      this->inputs_.push_back(m_inp);
    }

    row_y += ROW_STEP;
  }

  if (this->submit_label_) {
    lv_obj_t *btn = lv_btn_create(this->page_obj_);
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
  // Guard: LVGL fires SCREEN_LOADED on the incoming page before SCREEN_UNLOADED on us,
  // so active may already point to the new page by the time we get here.
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
