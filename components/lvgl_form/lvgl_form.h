#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/string_ref.h"
#include "esphome/components/api/api_server.h"
#include "esphome/components/api/api_pb2.h"
#include <lvgl.h>
#include <vector>

static const char *const LVGL_FORM_TAG = "lvgl_form";

namespace esphome {
namespace lvgl_form {

// ---------------------------------------------------------------------------
// BoundInput — base class for a form field
// ---------------------------------------------------------------------------
class BoundInput {
 public:
  virtual ~BoundInput() = default;
  virtual void subscribe_ha_state() {}
  virtual void sync_from_cache() = 0;
  virtual void set_focused(bool focused) = 0;
  virtual void set_editing(bool editing) = 0;
  virtual bool is_editable() const { return false; }
  virtual bool is_activatable() const { return false; }
  virtual void increment() {}
  virtual void decrement() {}
  virtual void activate() {}
  virtual void save() {}

  void set_page_active_ptr(const bool *ptr) { this->page_active_ = ptr; }

 protected:
  const bool *page_active_{nullptr};
};

// ---------------------------------------------------------------------------
// NumberInput — editable integer field that subscribes to a HA number entity
// ---------------------------------------------------------------------------
class NumberInput : public BoundInput {
 public:
  NumberInput(lv_obj_t *box, lv_obj_t *label, int min_val, int max_val)
      : box_(box), label_(label), min_val_(min_val), max_val_(max_val) {}

  void set_entity_id(const char *entity_id) { this->entity_id_ = entity_id; }

  void subscribe_ha_state() override {
    if (!this->entity_id_ || !api::global_api_server)
      return;
    api::global_api_server->subscribe_home_assistant_state(
        this->entity_id_, nullptr, [this](StringRef state) {
          auto val = parse_number<float>(state.c_str());
          if (val.has_value()) {
            this->ha_value_ = (int) *val;
            if (this->page_active_ && *this->page_active_)
              this->sync_from_cache();
          }
        });
  }

  void sync_from_cache() override {
    this->value_ = this->ha_value_;
    this->update_label(this->value_);
  }

  int current_val() const { return this->value_; }

  bool is_editable() const override { return true; }

  void set_focused(bool f) override { this->apply_state(f ? 1 : 0); }
  void set_editing(bool e) override { this->apply_state(e ? 2 : 1); }

  void increment() override {
    int range = this->max_val_ - this->min_val_ + 1;
    this->value_ = this->min_val_ + (this->value_ - this->min_val_ + range - 1) % range;
    this->update_label(this->value_);
  }

  void decrement() override {
    int range = this->max_val_ - this->min_val_ + 1;
    this->value_ = this->min_val_ + (this->value_ - this->min_val_ + 1) % range;
    this->update_label(this->value_);
  }

  void save() override {
    if (!this->entity_id_ || !api::global_api_server)
      return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", this->value_);
    api::HomeassistantActionRequest req;
    req.service = StringRef("number.set_value");
    req.data.init(2);
    auto &kv0 = req.data.emplace_back();
    kv0.key = StringRef("entity_id");
    kv0.value = StringRef(this->entity_id_);
    auto &kv1 = req.data.emplace_back();
    kv1.key = StringRef("value");
    kv1.value = StringRef(buf);
    api::global_api_server->send_homeassistant_action(req);
  }

 protected:
  lv_obj_t *box_;
  lv_obj_t *label_;
  int min_val_;
  int max_val_;
  int value_{0};
  int ha_value_{0};
  const char *entity_id_{nullptr};

  void update_label(int v) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", v);
    lv_label_set_text(this->label_, buf);
  }

  void apply_state(int state) {
    lv_color_t bg = (state == 2) ? lv_color_make(0, 0, 0) : lv_color_make(255, 255, 255);
    lv_color_t fg = (state == 2) ? lv_color_make(255, 255, 255) : lv_color_make(0, 0, 0);
    lv_obj_set_style_bg_color(this->box_, bg, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(this->label_, fg, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(this->box_, (state == 1) ? 4 : 2, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  }
};

// ---------------------------------------------------------------------------
// ToggleInput — binary toggle that subscribes to a HA switch entity
// ---------------------------------------------------------------------------
class ToggleInput : public BoundInput {
 public:
  explicit ToggleInput(lv_obj_t *sw) : sw_(sw) {
    // Track
    lv_obj_set_style_bg_color(this->sw_, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(this->sw_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(this->sw_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(this->sw_, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_CHECKED);
    lv_obj_set_style_border_width(this->sw_, 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_CHECKED);
    // Indicator: invisible
    lv_obj_set_style_bg_opa(this->sw_, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(this->sw_, LV_OPA_TRANSP,
                            (uint32_t) LV_PART_INDICATOR | (uint32_t) LV_STATE_CHECKED);
    lv_obj_set_style_border_width(this->sw_, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(this->sw_, 0,
                                  (uint32_t) LV_PART_INDICATOR | (uint32_t) LV_STATE_CHECKED);
    // Knob: always white
    lv_obj_set_style_bg_color(this->sw_, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(this->sw_, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(this->sw_, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_color(this->sw_, lv_color_make(255, 255, 255),
                               (uint32_t) LV_PART_KNOB | (uint32_t) LV_STATE_CHECKED);
    lv_obj_set_style_border_width(this->sw_, 0, (uint32_t) LV_PART_KNOB | (uint32_t) LV_STATE_CHECKED);
  }

  void set_entity_id(const char *entity_id) { this->entity_id_ = entity_id; }

  void subscribe_ha_state() override {
    if (!this->entity_id_ || !api::global_api_server)
      return;
    api::global_api_server->subscribe_home_assistant_state(
        this->entity_id_, nullptr, [this](StringRef state) {
          std::string s = state.str();
          this->ha_value_ = (s == "on" || s == "ON" || s == "true" || s == "1");
          if (this->page_active_ && *this->page_active_)
            this->sync_from_cache();
        });
  }

  void sync_from_cache() override {
    this->value_ = this->ha_value_;
    this->apply_checked(this->value_);
  }

  bool current_val() const { return this->value_; }

  bool is_activatable() const override { return true; }

  void set_focused(bool f) override {
    lv_obj_set_style_outline_width(this->sw_, f ? 3 : 0, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(this->sw_, lv_color_make(0, 0, 0), (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(this->sw_, 2, (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  }
  void set_editing(bool) override {}

  void activate() override {
    this->value_ = !this->value_;
    this->apply_checked(this->value_);
  }

  void save() override {
    if (!this->entity_id_ || !api::global_api_server)
      return;
    api::HomeassistantActionRequest req;
    req.service = StringRef(this->value_ ? "switch.turn_on" : "switch.turn_off");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = StringRef("entity_id");
    kv.value = StringRef(this->entity_id_);
    api::global_api_server->send_homeassistant_action(req);
  }

 protected:
  lv_obj_t *sw_;
  bool value_{false};
  bool ha_value_{false};
  const char *entity_id_{nullptr};

  void apply_checked(bool v) {
    if (v)
      lv_obj_add_state(this->sw_, LV_STATE_CHECKED);
    else
      lv_obj_clear_state(this->sw_, LV_STATE_CHECKED);
  }
};

// ---------------------------------------------------------------------------
// Forward declare FormPage so ActionInput can reference it
// ---------------------------------------------------------------------------
class FormPage;

// ---------------------------------------------------------------------------
// ActionInput — submit button that calls FormPage::on_submit()
// ---------------------------------------------------------------------------
class ActionInput : public BoundInput {
 public:
  explicit ActionInput(lv_obj_t *btn, FormPage *form) : btn_(btn), form_(form) {}

  void sync_from_cache() override {}
  bool is_activatable() const override { return true; }

  void set_focused(bool f) override {
    lv_obj_set_style_bg_color(this->btn_, f ? lv_color_make(0, 0, 0) : lv_color_make(255, 255, 255),
                               (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_t *lbl = lv_obj_get_child(this->btn_, 0);
    if (lbl)
      lv_obj_set_style_text_color(lbl, f ? lv_color_make(255, 255, 255) : lv_color_make(0, 0, 0),
                                   (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT);
  }
  void set_editing(bool) override {}
  void activate() override;

 protected:
  lv_obj_t *btn_;
  FormPage *form_;
};

// ---------------------------------------------------------------------------
// Row definitions — built from Python config, consumed in build_ui()
// ---------------------------------------------------------------------------
enum class RowType : uint8_t { TOGGLE, TIME };

struct ToggleRowDef {
  const char *label;
  const char *entity_id;
};

struct TimeRowDef {
  const char *label;
  const char *entity_hour_id;
  const char *entity_min_id;
};

struct RowDef {
  RowType type;
  ToggleRowDef toggle{};
  TimeRowDef time{};
};

// ---------------------------------------------------------------------------
// FormPage — owns the form, builds LVGL UI, manages encoder and navigation
// ---------------------------------------------------------------------------
class FormPage : public esphome::Component {
 public:
  static FormPage *active;

  void set_page_obj(lv_obj_t *obj) { this->page_obj_ = obj; }
  void set_title(const char *title) { this->title_ = title; }
  void set_font(const lv_font_t *font) { this->font_ = font; }
  void set_submit_label(const char *label) { this->submit_label_ = label; }

  void add_toggle_row(const char *label, const char *entity_id) {
    RowDef r;
    r.type = RowType::TOGGLE;
    r.toggle = {label, entity_id};
    this->rows_.push_back(r);
  }

  void add_time_row(const char *label, const char *entity_hour_id, const char *entity_min_id) {
    RowDef r;
    r.type = RowType::TIME;
    r.time = {label, entity_hour_id, entity_min_id};
    this->rows_.push_back(r);
  }

  void setup() override {
    this->build_ui();
    for (auto *inp : this->inputs_)
      inp->subscribe_ha_state();
  }
  void loop() override {}
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }

  void on_page_load();
  void on_page_unload();
  void on_confirm();
  void on_up();
  void on_down();
  void on_submit();

 protected:
  lv_obj_t *page_obj_{nullptr};
  const char *title_{nullptr};
  const lv_font_t *font_{nullptr};
  const char *submit_label_{nullptr};
  bool page_active_{false};

  std::vector<RowDef> rows_;
  std::vector<BoundInput *> inputs_;
  size_t focused_idx_{0};
  bool editing_{false};

  void build_ui();
  lv_obj_t *make_ticker_box(lv_obj_t *parent, int x, int y, lv_obj_t **label_out);

  static void on_load_event_(lv_event_t *e) {
    static_cast<FormPage *>(lv_event_get_user_data(e))->on_page_load();
  }
  static void on_unload_event_(lv_event_t *e) {
    static_cast<FormPage *>(lv_event_get_user_data(e))->on_page_unload();
  }

  void focus(size_t idx) {
    for (size_t i = 0; i < this->inputs_.size(); i++)
      this->inputs_[i]->set_focused(i == idx);
  }

  void navigate(int delta) {
    this->inputs_[this->focused_idx_]->set_focused(false);
    size_t n = this->inputs_.size();
    this->focused_idx_ = (this->focused_idx_ + n + delta) % n;
    this->inputs_[this->focused_idx_]->set_focused(true);
  }
};

}  // namespace lvgl_form
}  // namespace esphome
