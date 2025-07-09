#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/http_request/http_request.h"

namespace esphome {
namespace trias {

struct Departure {
    std::string line;
    std::string destination;
    ESPTime time;

    auto operator<=>(const Departure&) const = default;
};

class Trias : public text_sensor::TextSensor, public PollingComponent {
    public:
        void set_url(const std::string &url) { this->url_ = url; }
        void set_api_key(const std::string &api_key) { this->api_key_ = api_key; }
        void set_stop_id(const std::string &stop_id) { this->stop_id_ = stop_id; }
        void set_time(time::RealTimeClock *time_component) { this->time_ = time_component; }
        void set_http(http_request::HttpRequestComponent *http_component) { this->http_ = http_component; }

        void update() override;

        void change_stop(std::string);

        std::vector<Departure> getDepartures() { return this->departures; }
        std::string getStopName() { return this->stop_name; }

    protected:
        std::string url_;
        std::string api_key_;
        std::string stop_id_;
        time::RealTimeClock *time_{nullptr};
        http_request::HttpRequestComponent *http_{nullptr};
        std::vector<Departure> departures;
        std::string stop_name;
};


} // namespace trias
} // namespace esphome