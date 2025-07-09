import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, time as time_, http_request
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL
)

DEPENDENCIES = ['time', 'xml', 'http_request']

CONF_URL = "url"
CONF_API_KEY = "api_key"
CONF_STOP_ID = "stop_id"
CONF_TIME = "time"
CONF_HTTP = "http"

trias_ns = cg.esphome_ns.namespace('trias')
Trias = trias_ns.class_('Trias', text_sensor.TextSensor, cg.PollingComponent)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(Trias).extend({
    cv.Required(CONF_STOP_ID): cv.string,
    cv.Required(CONF_API_KEY): cv.string,
    cv.Required(CONF_URL): cv.string,
    cv.Required(CONF_TIME): cv.use_id(time_.RealTimeClock),
    cv.Required(CONF_HTTP): cv.use_id(http_request.HttpRequestComponent),
    cv.Optional(CONF_UPDATE_INTERVAL): cv.update_interval,
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield text_sensor.register_text_sensor(var, config)

    yield cg.add(var.set_stop_id(config[CONF_STOP_ID]))
    yield cg.add(var.set_url(config[CONF_URL]))
    yield cg.add(var.set_api_key(config[CONF_API_KEY]))
    time_var = yield cg.get_variable(config[CONF_TIME])
    yield cg.add(var.set_time(time_var))
    http_var = yield cg.get_variable(config[CONF_HTTP])
    yield cg.add(var.set_http(http_var))

    if CONF_UPDATE_INTERVAL in config:
        yield var.set_update_interval(config[CONF_UPDATE_INTERVAL])