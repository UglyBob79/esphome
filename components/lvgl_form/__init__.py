import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components.lvgl.types import lv_page_t
from esphome.components.lvgl.helpers import add_lv_use
from esphome.components.lvgl.lvcode import LvglComponent
from esphome.components.lvgl.defines import CONF_LVGL_ID
from esphome.components.font import Font

add_lv_use("switch")
add_lv_use("btn")

CODEOWNERS = []
MULTI_CONF = True
DEPENDENCIES = ["lvgl", "api", "font"]

lvgl_form_ns = cg.esphome_ns.namespace("lvgl_form")
FormPage = lvgl_form_ns.class_("FormPage", cg.Component)

CONF_PAGE_TYPE = "type"
CONF_TITLE = "title"
CONF_FONT = "font"
CONF_ROWS = "rows"
CONF_ROW_TYPE = "type"
CONF_LABEL = "label"
CONF_ENTITY = "entity"
CONF_ENTITY_HOUR = "entity_hour"
CONF_ENTITY_MINUTE = "entity_minute"
CONF_SUBMIT = "submit"
CONF_PAGE_ID = "page_id"

TOGGLE_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("toggle", lower=True),
        cv.Required(CONF_LABEL): cv.string,
        cv.Required(CONF_ENTITY): cv.entity_id,
    }
)

TIME_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ROW_TYPE): cv.one_of("time", lower=True),
        cv.Required(CONF_LABEL): cv.string,
        cv.Required(CONF_ENTITY_HOUR): cv.entity_id,
        cv.Required(CONF_ENTITY_MINUTE): cv.entity_id,
    }
)


def validate_row(value):
    if not isinstance(value, dict) or CONF_ROW_TYPE not in value:
        raise cv.Invalid("Row must have a 'type' field")
    t = value[CONF_ROW_TYPE]
    if t == "toggle":
        return TOGGLE_ROW_SCHEMA(value)
    if t == "time":
        return TIME_ROW_SCHEMA(value)
    raise cv.Invalid(f"Unknown row type: '{t}'")


def validate_page(config):
    page_type = config[CONF_PAGE_TYPE]
    if page_type == "form" and CONF_SUBMIT not in config:
        raise cv.Invalid("type 'form' requires a 'submit' section")
    if page_type == "panel" and CONF_SUBMIT in config:
        raise cv.Invalid("type 'panel' does not use 'submit'")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FormPage),
            cv.GenerateID(CONF_PAGE_ID): cv.declare_id(lv_page_t),
            cv.GenerateID(CONF_LVGL_ID): cv.use_id(LvglComponent),
            cv.Required(CONF_PAGE_TYPE): cv.one_of("form", "panel", lower=True),
            cv.Required(CONF_TITLE): cv.string,
            cv.Required(CONF_FONT): cv.use_id(Font),
            cv.Required(CONF_ROWS): cv.ensure_list(validate_row),
            cv.Optional(CONF_SUBMIT): cv.Schema(
                {
                    cv.Required(CONF_LABEL): cv.string,
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_page,
)


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

    for row in config[CONF_ROWS]:
        row_type = row[CONF_ROW_TYPE]
        if row_type == "toggle":
            cg.add(var.add_toggle_row(row[CONF_LABEL], row[CONF_ENTITY]))
        elif row_type == "time":
            cg.add(
                var.add_time_row(
                    row[CONF_LABEL],
                    row[CONF_ENTITY_HOUR],
                    row[CONF_ENTITY_MINUTE],
                )
            )

    if CONF_SUBMIT in config:
        cg.add(var.set_submit_label(config[CONF_SUBMIT][CONF_LABEL]))
    cg.add_define("USE_API_HOMEASSISTANT_STATES")
    cg.add_define("USE_API_HOMEASSISTANT_SERVICES")
