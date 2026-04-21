#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <lvgl.h>
#include <vector>
#include <functional>

namespace esphome {
namespace lvgl_form {

class BoundInput {
 public:
  virtual ~BoundInput() = default;
  virtual void sync_from_ha() = 0;
  virtual void set_focused(bool focused) = 0;
  virtual void set_editing(bool editing) = 0;
  virtual bool is_editable() const { return false; }
  virtual bool is_activatable() const { return false; }
  virtual void increment() {}
  virtual void decrement() {}
  virtual void activate() {}
};

class NumberInput : public BoundInput {
 public:
  NumberInput(lv_obj_t* box, lv_obj_t* label, int min_val, int max_val)
      : box_(box), label_(label), min_val_(min_val), max_val_(max_val) {}

  void set_value_ptr(int* ptr) { value_ptr_ = ptr; }
  void set_ha_sensor(sensor::Sensor* s) { ha_sensor_ = s; }
  void set_on_save(std::function<void(int)> cb) { on_save_ = cb; }

  void on_ha_update(float v) {
    ha_val_ = (int)v;
    if (value_ptr_) *value_ptr_ = ha_val_;
    update_label(ha_val_);
  }

  void sync_from_ha() override {
    int v = (ha_sensor_ && ha_sensor_->has_state()) ? (int)ha_sensor_->state : ha_val_;
    if (value_ptr_) *value_ptr_ = v;
    update_label(v);
  }

  bool is_editable() const override { return true; }

  void set_focused(bool f) override { apply_state(f ? 1 : 0); }
  void set_editing(bool e) override { apply_state(e ? 2 : 1); }

  void increment() override {
    if (!value_ptr_) return;
    int range = max_val_ - min_val_ + 1;
    *value_ptr_ = min_val_ + (*value_ptr_ - min_val_ + range - 1) % range;
    update_label(*value_ptr_);
  }

  void decrement() override {
    if (!value_ptr_) return;
    int range = max_val_ - min_val_ + 1;
    *value_ptr_ = min_val_ + (*value_ptr_ - min_val_ + 1) % range;
    update_label(*value_ptr_);
  }

 protected:
  lv_obj_t* box_;
  lv_obj_t* label_;
  int min_val_, max_val_;
  int ha_val_{0};
  int* value_ptr_{nullptr};
  sensor::Sensor* ha_sensor_{nullptr};
  std::function<void(int)> on_save_;

  void update_label(int v) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", v);
    lv_label_set_text(label_, buf);
  }

  void apply_state(int state) {
    // 0=default, 1=focused, 2=editing
    lv_color_t bg = (state == 2) ? lv_color_make(0, 0, 0) : lv_color_make(255, 255, 255);
    lv_color_t fg = (state == 2) ? lv_color_make(255, 255, 255) : lv_color_make(0, 0, 0);
    lv_obj_set_style_bg_color(box_, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_, fg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_, (state == 1) ? 4 : 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
};

class ToggleInput : public BoundInput {
 public:
  explicit ToggleInput(lv_obj_t* sw) : sw_(sw) {
    // Track: always black, 2px black border
    lv_obj_set_style_bg_color(sw_, lv_color_make(0,0,0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(sw_, lv_color_make(0,0,0), LV_PART_MAIN);
    lv_obj_set_style_border_width(sw_, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_, lv_color_make(0,0,0), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(sw_, lv_color_make(0,0,0), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(sw_, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    // Indicator: always invisible
    lv_obj_set_style_bg_opa(sw_, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sw_, LV_OPA_TRANSP, (uint32_t)LV_PART_INDICATOR | (uint32_t)LV_STATE_CHECKED);
    lv_obj_set_style_border_width(sw_, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(sw_, 0, (uint32_t)LV_PART_INDICATOR | (uint32_t)LV_STATE_CHECKED);
    // Knob: always white
    lv_obj_set_style_bg_color(sw_, lv_color_make(255,255,255), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(sw_, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(sw_, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_color(sw_, lv_color_make(255,255,255), (uint32_t)LV_PART_KNOB | (uint32_t)LV_STATE_CHECKED);
    lv_obj_set_style_border_width(sw_, 0, (uint32_t)LV_PART_KNOB | (uint32_t)LV_STATE_CHECKED);
  }

  void set_value_ptr(bool* ptr) { value_ptr_ = ptr; }
  void set_ha_sensor(binary_sensor::BinarySensor* s) { ha_sensor_ = s; }

  void on_ha_state(bool v) {
    ha_val_ = v;
    if (value_ptr_) *value_ptr_ = v;
    apply_checked(v);
  }

  void sync_from_ha() override {
    bool v = (ha_sensor_ && ha_sensor_->has_state()) ? ha_sensor_->state : ha_val_;
    if (value_ptr_) *value_ptr_ = v;
    apply_checked(v);
  }

  bool is_activatable() const override { return true; }

  void set_focused(bool f) override {
    lv_obj_set_style_outline_width(sw_, f ? 3 : 0, LV_PART_MAIN);
    lv_obj_set_style_outline_color(sw_, lv_color_make(0,0,0), LV_PART_MAIN);
    lv_obj_set_style_outline_pad(sw_, 2, LV_PART_MAIN);
  }
  void set_editing(bool) override {}

  void activate() override {
    bool new_val = value_ptr_ ? !(*value_ptr_) : false;
    if (value_ptr_) *value_ptr_ = new_val;
    apply_checked(new_val);
  }

 protected:
  lv_obj_t* sw_;
  bool ha_val_{false};
  bool* value_ptr_{nullptr};
  binary_sensor::BinarySensor* ha_sensor_{nullptr};

  void apply_checked(bool v) {
    if (v) lv_obj_add_state(sw_, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw_, LV_STATE_CHECKED);
  }
};

class ActionInput : public BoundInput {
 public:
  explicit ActionInput(lv_obj_t* btn) : btn_(btn) {}

  void sync_from_ha() override {}
  bool is_activatable() const override { return true; }

  void set_focused(bool f) override {
    if (f) lv_obj_add_state(btn_, LV_STATE_FOCUSED);
    else lv_obj_clear_state(btn_, LV_STATE_FOCUSED);
  }
  void set_editing(bool) override {}

  void activate() override {
    lv_event_send(btn_, LV_EVENT_CLICKED, nullptr);
  }

 protected:
  lv_obj_t* btn_;
};

class FormPage : public esphome::Component {
 public:
  static FormPage* active;

  void set_page(lv_obj_t* page_obj, lv_group_t* restore_group = nullptr) {
    restore_group_ = restore_group;
    lv_obj_add_event_cb(page_obj, on_load_event_, LV_EVENT_SCREEN_LOADED, this);
    lv_obj_add_event_cb(page_obj, on_unload_event_, LV_EVENT_SCREEN_UNLOADED, this);
  }

  FormPage& add_input(BoundInput* input) {
    inputs_.push_back(input);
    return *this;
  }

  void on_page_load() {
    active = this;
    focused_idx_ = 0;
    editing_ = false;

    lv_indev_t* enc = lv_indev_get_next(nullptr);
    while (enc) {
      if (lv_indev_get_type(enc) == LV_INDEV_TYPE_ENCODER) {
        encoder_indev_ = enc;
        break;
      }
      enc = lv_indev_get_next(enc);
    }
    if (encoder_indev_) lv_indev_set_group(encoder_indev_, nullptr);

    for (auto* inp : inputs_) inp->sync_from_ha();
    focus(focused_idx_);
  }

  void on_page_unload() {
    active = nullptr;
    editing_ = false;
    if (encoder_indev_ && restore_group_)
      lv_indev_set_group(encoder_indev_, restore_group_);
    encoder_indev_ = nullptr;
  }

  void on_confirm() {
    if (inputs_.empty()) return;
    auto* inp = inputs_[focused_idx_];
    if (editing_) {
      editing_ = false;
      inp->set_editing(false);
      inp->set_focused(true);
    } else if (inp->is_editable()) {
      editing_ = true;
      inp->set_editing(true);
    } else if (inp->is_activatable()) {
      inp->activate();
    }
  }

  void on_up() {
    if (inputs_.empty()) return;
    if (editing_) inputs_[focused_idx_]->increment();
    else navigate(-1);
  }

  void on_down() {
    if (inputs_.empty()) return;
    if (editing_) inputs_[focused_idx_]->decrement();
    else navigate(1);
  }

  void setup() override {}
  void loop() override {}
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }

 protected:
  std::vector<BoundInput*> inputs_;
  size_t focused_idx_{0};
  bool editing_{false};
  lv_indev_t* encoder_indev_{nullptr};
  lv_group_t* restore_group_{nullptr};

  static void on_load_event_(lv_event_t* e) {
    static_cast<FormPage*>(lv_event_get_user_data(e))->on_page_load();
  }
  static void on_unload_event_(lv_event_t* e) {
    static_cast<FormPage*>(lv_event_get_user_data(e))->on_page_unload();
  }

  void focus(size_t idx) {
    for (size_t i = 0; i < inputs_.size(); i++)
      inputs_[i]->set_focused(i == idx);
  }

  void navigate(int delta) {
    inputs_[focused_idx_]->set_focused(false);
    size_t n = inputs_.size();
    focused_idx_ = (focused_idx_ + n + delta) % n;
    inputs_[focused_idx_]->set_focused(true);
  }
};

}  // namespace lvgl_form
}  // namespace esphome
