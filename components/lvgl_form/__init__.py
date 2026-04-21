import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = []
MULTI_CONF = True
DEPENDENCIES = ["lvgl"]

lvgl_form_ns = cg.esphome_ns.namespace("lvgl_form")
FormPage = lvgl_form_ns.class_("FormPage", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FormPage),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
