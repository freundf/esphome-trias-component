#include "trias.h"
#include "trias_request.h"
#include "esphome/core/log.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/core/helpers.h"
#include "esphome/components/xml/xml_util.h"


namespace esphome {
namespace trias {

static const char *const TAG = "trias";

ESPTime parse_time(std::string time_string) {
    struct tm tm { .tm_isdst = -1 };
    strptime(time_string.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
    auto offset = ESPTime::timezone_offset();
    time_t epoch = mktime(&tm) + offset;

    return ESPTime::from_epoch_local(epoch);
}

trias::Departure parse_stop_event_result(tinyxml2::XMLElement *stop_event_result) {
    using namespace tinyxml2;
    Departure dep;
    XMLElement *call_at_stop = stop_event_result
        ->FirstChildElement("StopEvent")
        ->FirstChildElement("ThisCall")
        ->FirstChildElement("CallAtStop");

    XMLElement *service = stop_event_result
        ->FirstChildElement("StopEvent")
        ->FirstChildElement("Service");

    dep.line = service
        ->FirstChildElement("PublishedLineName")
        ->FirstChildElement("Text")->GetText();

    dep.destination = service
        ->FirstChildElement("DestinationText")
        ->FirstChildElement("Text")->GetText();

    dep.time = parse_time([](XMLElement *elem) -> std::string {
        auto time_elem = elem
            ->FirstChildElement("ServiceDeparture")
            ->FirstChildElement("EstimatedTime");
        if (!time_elem) {
            time_elem = elem
                ->FirstChildElement("ServiceDeparture")
                ->FirstChildElement("TimetabledTime");
        }
        return time_elem->GetText();
    }(call_at_stop));

    dep.bay = call_at_stop
        ->FirstChildElement("PlannedBay")
        ->FirstChildElement("Text")->GetText();

    dep.route = service
        ->FirstChildElement("RouteDescription")
        ->FirstChildElement("Text")->GetText();

    dep.service_type = service
        ->FirstChildElement("Mode")
        ->FirstChildElement("Name")
        ->FirstChildElement("Text")->GetText();

    return dep;
}


void parse_stop_event_response(std::string body, std::vector<Departure> *departures, std::string *stop_name) {
    using namespace tinyxml2;
    xml::parse_xml(body, [&](XMLElement *root) -> bool {
        auto *result = root
            ->FirstChildElement("ServiceDelivery")
            ->FirstChildElement("DeliveryPayload")
            ->FirstChildElement("StopEventResponse")
            ->FirstChildElement("StopEventResult");

        auto get_stop_name([](XMLElement *elem) -> std::string {
            return elem
                ->FirstChildElement("StopEvent")
                ->FirstChildElement("ThisCall")
                ->FirstChildElement("CallAtStop")
                ->FirstChildElement("StopPointName")
                ->FirstChildElement("Text")->GetText();
        });

        *stop_name = get_stop_name(result);

        do {
            departures->push_back(parse_stop_event_result(result));
        } while (result = result->NextSiblingElement("StopEventResult"));
        return true;
    });
}

void parse_stop_response(std::string body, std::string *stop_id) {
    using namespace tinyxml2;
    xml::parse_xml(body, [&](XMLElement *root) -> bool {
        *stop_id = root
            ->FirstChildElement("ServiceDelivery")
            ->FirstChildElement("DeliveryPayload")
            ->FirstChildElement("LocationInformationResponse")
            ->FirstChildElement("Location")
            ->FirstChildElement("Location")
            ->FirstChildElement("StopPoint")
            ->FirstChildElement("StopPointRef")->GetText();

        return true;
    });
}

void Trias::update() {
    if (this->http_ == nullptr) {
        ESP_LOGE(TAG, "HTTP component is null");
        return;
    }
    ESP_LOGI(TAG, "Sending request to %s", this->url_.c_str());

    std::string time = this->time_->now().strftime("%Y-%m-%dT%H:%M:%S%z");
    std::string body = stop_event_request(time, this->api_key_, this->stop_id_);
    const std::list<http_request::Header> header = { { "Content-Type", "application/xml" } };
    std::string response = this->post_request(body, header);

    this->departures.clear();
    parse_stop_event_response(response, &this->departures, &this->stop_name);
    if (departures.empty()) {
        ESP_LOGI(TAG, "no departures found");
        return;
    }

    std::sort(this->departures.begin(), this->departures.end(), [](Departure &a, Departure &b) {
        return a.time < b.time;
    });

    this->publish_state(
        this->departures[0].line + " -> " +
        this->departures[0].destination + ", " +
        this->departures[0].time.strftime("%Y-%m-%dT%H:%M:%S")
    );

}

void Trias::change_stop(std::string stop_name) {
    std::string time = this->time_->now().strftime("%Y-%m-%dT%H:%M:%S%z");
    std::string body = location_information_request(time, this->api_key_, stop_name);
    const std::list<http_request::Header> header = { { "Content-Type", "application/xml" } };
    std::string response = this->post_request(body, header);
    parse_stop_response(response, &this->stop_id_);
    ESP_LOGI(TAG, "changed stop to %s", this->stop_id_.c_str());
}

std::string Trias::post_request(std::string body, std::list<http_request::Header> headers) {
    auto container = this->http_->post(this->url_, body, headers);
    if (container == nullptr) {
        ESP_LOGE(TAG, "HTTP response is null!");
        return "";
    }
    if (container->content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", container->content_length);
        return "";
    }

    size_t max_length = container->content_length;

    std::string response_body;
    RAMAllocator<uint8_t> allocator;
    uint8_t *buf = allocator.allocate(max_length);
    if (buf != nullptr) {
        size_t read_index = 0;
        while (container->get_bytes_read() < max_length) {
            int read = container->read(buf + read_index, std::min<size_t>(max_length - read_index, 512));
            yield();
            read_index += read;
        }
        response_body.reserve(read_index);
        response_body.assign((char *) buf, read_index);
        allocator.deallocate(buf, max_length);
    }

    container->end();
    return response_body;
}

}
}
