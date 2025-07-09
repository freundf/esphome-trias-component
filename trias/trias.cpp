#include "trias.h"
#include "esphome/core/log.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/core/helpers.h"
#include "esphome/components/xml/xml_util.h"


namespace esphome {
namespace trias {

static const char *const TAG = "trias";

void parseResponse(std::string body, std::vector<Departure> *departures, std::string *stop_name) {
    using namespace tinyxml2;
    xml::parse_xml(body, [&](XMLElement *root) -> bool {
        auto get_time = [](XMLElement *stopEvent) -> ESPTime {
            const char *time_string = stopEvent
                ->FirstChildElement("StopEvent")
                ->FirstChildElement("ThisCall")
                ->FirstChildElement("CallAtStop")
                ->FirstChildElement("ServiceDeparture")
                ->FirstChildElement("TimetabledTime")->GetText();

            struct tm tm { .tm_isdst = -1 };
            strptime(time_string, "%Y-%m-%dT%H:%M:%SZ", &tm);
            auto offset = ESPTime::timezone_offset();
            time_t epoch = mktime(&tm) + offset;

            return ESPTime::from_epoch_local(epoch);
        };

        auto get_line_name = [](XMLElement *stopEvent) -> std::string {
            return stopEvent
                ->FirstChildElement("StopEvent")
                ->FirstChildElement("Service")
                ->FirstChildElement("PublishedLineName")
                ->FirstChildElement("Text")->GetText();
        };

        auto get_stop_name = [](XMLElement *stopEvent) -> std::string {
            return stopEvent
                ->FirstChildElement("StopEvent")
                ->FirstChildElement("ThisCall")
                ->FirstChildElement("CallAtStop")
                ->FirstChildElement("StopPointName")
                ->FirstChildElement("Text")->GetText();
        };

        auto get_destination_name = [](XMLElement *stopEvent) -> std::string {
            return stopEvent
                ->FirstChildElement("StopEvent")
                ->FirstChildElement("Service")
                ->FirstChildElement("DestinationText")
                ->FirstChildElement("Text")->GetText();
        };

        auto *result = root
            ->FirstChildElement("ServiceDelivery")
            ->FirstChildElement("DeliveryPayload")
            ->FirstChildElement("StopEventResponse")
            ->FirstChildElement("StopEventResult");

        *stop_name = get_stop_name(result);

        do {
            auto time = get_time(result);
            auto line = get_line_name(result);
            auto destination = get_destination_name(result);

            departures->push_back({line, destination, time});
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
    std::string xml_body =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<Trias version=\"1.1\" xmlns=\"http://www.vdv.de/trias\" xmlns:siri=\"http://www.siri.org.uk/siri\" xmlns:xsi=\"http://www.w3.o    rg/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.vdv.de/trias file:///C:/development/HEAD/extras/TRIAS/TRIAS_1.1/Trias.xsd\"    >\n"
    "  <ServiceRequest>\n"
    "    <siri:RequestTimeStamp>" + time + "</siri:RequestTimeStamp>\n"
    "    <siri:RequestorRef>" + this->api_key_ + "</siri:RequestorRef>\n"
    "    <RequestPayload>\n"
    "      <StopEventRequest>\n"
    "        <Location>\n"
    "          <LocationRef>\n"
    "            <StopPlaceRef>" + this->stop_id_ + "</StopPlaceRef>\n"
    "          </LocationRef>\n"
    "          <DepArrTime>" + time + "</DepArrTime>\n"
    "        </Location>\n"
    "        <Params>\n"
    "          <NumberOfResults>4</NumberOfResults>\n"
    "        </Params>\n"
    "      </StopEventRequest>\n"
    "    </RequestPayload>\n"
    "  </ServiceRequest>\n"
    "</Trias>";

    const std::list<http_request::Header> header = { { "Content-Type", "application/xml" } };
    auto container = this->http_->post(this->url_, xml_body, header);
    if (container == nullptr) {
        ESP_LOGE(TAG, "HTTP response is null!");
        return;
    }
    if (container->content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", container->content_length);
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
    this->departures.clear();
    parseResponse(response_body, &this->departures, &this->stop_name);
    this->publish_state(
        this->departures[0].line + " -> " +
        this->departures[0].destination + ", " +
        this->departures[0].time.strftime("%Y-%m-%dT%H:%M:%S")
    );

}

void Trias::change_stop(std::string stop_name) {
    std::string time = this->time_->now().strftime("%Y-%m-%dT%H:%M:%S%z");
    std::string xml_body =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<Trias version=\"1.1\" xmlns=\"http://www.vdv.de/trias\" xmlns:siri=\"http://www.siri.org.uk/siri\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.vdv.de/trias file:///C:/development/HEAD/extras/TRIAS/TRIAS_1.1/Trias.xsd\">\n"
        "   <ServiceRequest>\n"
        "       <siri:RequestTimeStamp>" + time + "</siri:RequestTimeStamp>\n"
        "       <siri:RequestorRef>31zFyy7pdRVh</siri:RequestorRef>\n"
        "       <RequestPayload>\n"
        "           <LocationInformationRequest>\n"
        "               <InitialInput>\n"
        "                   <LocationName>" + stop_name + "</LocationName>\n"
        "               </InitialInput>\n"
        "               <Restrictions>\n"
        "                   <Type>stop</Type>\n"
        "                   <NumberOfResults>1</NumberOfResults>\n"
        "               </Restrictions>\n"
        "           </LocationInformationRequest>\n"
        "       </RequestPayload>\n"
        "   </ServiceRequest>\n"
        "</Trias>\n";

    const std::list<http_request::Header> header = { { "Content-Type", "application/xml" } };
    auto container = this->http_->post(this->url_, xml_body, header);
    if (container == nullptr) {
        ESP_LOGE(TAG, "HTTP response is null!");
        return;
    }
    if (container->content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", container->content_length);
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
    parse_stop_response(response_body, &this->stop_id_);
}

}
}
