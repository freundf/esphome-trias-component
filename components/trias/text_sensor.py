import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, time as time_
from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent
from esphome.const import (
    CONF_ID,
)

DEPENDENCIES = ['time', 'xml', 'http_request']

CONF_URL = "url"
CONF_API_KEY = "api_key"
CONF_STOP_ID = "stop_id"
CONF_TIME = "time"

trias_ns = cg.esphome_ns.namespace('trias')
Trias = trias_ns.class_('Trias', text_sensor.TextSensor, cg.PollingComponent)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(Trias).extend({
    cv.Required(CONF_STOP_ID): cv.string,
    cv.Required(CONF_API_KEY): cv.string,
    cv.Required(CONF_URL): cv.string,
    cv.Required(CONF_TIME): cv.use_id(time_.RealTimeClock),
    cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
}).extend(cv.polling_component_schema('60s'))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)

    cg.add(var.set_stop_id(config[CONF_STOP_ID]))
    cg.add(var.set_url(config[CONF_URL]))
    cg.add(var.set_api_key(config[CONF_API_KEY]))
    time_var = await cg.get_variable(config[CONF_TIME])
    cg.add(var.set_time(time_var))
    http_var = await cg.get_variable(config[CONF_HTTP_REQUEST_ID])
    cg.add(var.set_http(http_var))
