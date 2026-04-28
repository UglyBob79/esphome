import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components.lvgl.types import lv_page_t
from esphome.components.lvgl.helpers import add_lv_use
from esphome.components.lvgl.lvcode import LvglComponent
from esphome.components.lvgl.defines import CONF_LVGL_ID
from esphome.components.font import Font

add_lv_use("switch")
add_lv_use("flex")

CODEOWNERS = []
MULTI_CONF = True
DEPENDENCIES = ["lvgl", "api", "font"]

page_ns = cg.esphome_ns.namespace("page")
Page = page_ns.class_("Page", cg.Component)

CONF_CONTENT = "content"
CONF_CONTENT_TYPE = "type"
CONF_TITLE = "title"
CONF_FONT = "font"
CONF_ROWS = "rows"
CONF_ROW_TYPE = "type"
CONF_LABEL = "label"
CONF_ENTITY = "entity"
CONF_ENTITY_HOUR = "entity_hour"
CONF_ENTITY_MINUTE = "entity_minute"
CONF_FORMAT = "format"
CONF_TEXT = "text"
CONF_SUBMIT = "submit"
CONF_PAGE_ID = "page_id"

TOGGLE_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("toggle", lower=True),
        cv.Required(CONF_LABEL): cv.string,
        cv.Required(CONF_ENTITY): cv.entity_id,
    }
)

TIME_INPUT_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("time_input", lower=True),
        cv.Required(CONF_LABEL): cv.string,
        cv.Required(CONF_ENTITY_HOUR): cv.entity_id,
        cv.Required(CONF_ENTITY_MINUTE): cv.entity_id,
    }
)

SENSOR_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("sensor", lower=True),
        cv.Required(CONF_ENTITY): cv.entity_id,
        cv.Required(CONF_FORMAT): cv.string,
    }
)

LABEL_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("label", lower=True),
        cv.Required(CONF_TEXT): cv.string,
    }
)


def validate_row(value):
    if not isinstance(value, dict) or CONF_ROW_TYPE not in value:
        raise cv.Invalid("Row must have a 'type' field")
    t = value[CONF_ROW_TYPE]
    if t == "toggle":
        return TOGGLE_ROW_SCHEMA(value)
    if t == "time_input":
        return TIME_INPUT_ROW_SCHEMA(value)
    if t == "sensor":
        return SENSOR_ROW_SCHEMA(value)
    if t == "label":
        return LABEL_ROW_SCHEMA(value)
    raise cv.Invalid(f"Unknown row type: '{t}'")


def validate_content(config):
    content_type = config[CONF_CONTENT_TYPE]
    if content_type == "form" and CONF_SUBMIT not in config:
        raise cv.Invalid("content type 'form' requires a 'submit' section")
    if content_type == "panel" and CONF_SUBMIT in config:
        raise cv.Invalid("content type 'panel' does not use 'submit'")
    return config


CONTENT_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_CONTENT_TYPE): cv.one_of("form", "panel", lower=True),
            cv.Required(CONF_ROWS): cv.ensure_list(validate_row),
            cv.Optional(CONF_SUBMIT): cv.Schema(
                {
                    cv.Required(CONF_LABEL): cv.string,
                }
            ),
        }
    ),
    validate_content,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Page),
        cv.GenerateID(CONF_PAGE_ID): cv.declare_id(lv_page_t),
        cv.GenerateID(CONF_LVGL_ID): cv.use_id(LvglComponent),
        cv.Required(CONF_TITLE): cv.string,
        cv.Required(CONF_FONT): cv.use_id(Font),
        cv.Required(CONF_CONTENT): CONTENT_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    lvgl_var = await cg.get_variable(config[CONF_LVGL_ID])
    page_var = cg.new_Pvariable(config[CONF_PAGE_ID], False)
    cg.add(lvgl_var.add_page(page_var))
    cg.add(var.set_page_obj(cg.RawExpression(f"{page_var}->obj")))

    font_var = await cg.get_variable(config[CONF_FONT])
    cg.add(var.set_font(font_var.get_lv_font()))

    cg.add(var.set_title(config[CONF_TITLE]))

    content = config[CONF_CONTENT]
    for row in content[CONF_ROWS]:
        row_type = row[CONF_ROW_TYPE]
        if row_type == "toggle":
            cg.add(var.add_toggle_row(row[CONF_LABEL], row[CONF_ENTITY]))
        elif row_type == "time_input":
            cg.add(
                var.add_time_input_row(
                    row[CONF_LABEL],
                    row[CONF_ENTITY_HOUR],
                    row[CONF_ENTITY_MINUTE],
                )
            )
        elif row_type == "sensor":
            cg.add(var.add_sensor_row(row[CONF_ENTITY], row[CONF_FORMAT]))
        elif row_type == "label":
            cg.add(var.add_label_row(row[CONF_TEXT]))

    if CONF_SUBMIT in content:
        cg.add(var.set_submit_label(content[CONF_SUBMIT][CONF_LABEL]))

    cg.add_define("USE_API_HOMEASSISTANT_STATES")
    cg.add_define("USE_API_HOMEASSISTANT_SERVICES")
