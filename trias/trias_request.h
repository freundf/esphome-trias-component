
inline std::string stop_event_request(std::string time, std::string ref, std::string stop_id) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<Trias version=\"1.1\" xmlns=\"http://www.vdv.de/trias\" xmlns:siri=\"http://www.siri.org.uk/siri\" xmlns:xsi=\"http://www.w3.o    rg/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.vdv.de/trias file:///C:/development/HEAD/extras/TRIAS/TRIAS_1.1/Trias.xsd\"    >\n"
    "  <ServiceRequest>\n"
    "    <siri:RequestTimeStamp>" + time + "</siri:RequestTimeStamp>\n"
    "    <siri:RequestorRef>" + ref + "</siri:RequestorRef>\n"
    "    <RequestPayload>\n"
    "      <StopEventRequest>\n"
    "        <Location>\n"
    "          <LocationRef>\n"
    "            <StopPlaceRef>" + stop_id + "</StopPlaceRef>\n"
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
}

inline std::string location_information_request(std::string time, std::string ref, std::string stop_name) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<Trias version=\"1.1\" xmlns=\"http://www.vdv.de/trias\" xmlns:siri=\"http://www.siri.org.uk/siri\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.vdv.de/trias file:///C:/development/HEAD/extras/TRIAS/TRIAS_1.1/Trias.xsd\">\n"
    "   <ServiceRequest>\n"
    "       <siri:RequestTimeStamp>" + time + "</siri:RequestTimeStamp>\n"
    "       <siri:RequestorRef>" + ref + "</siri:RequestorRef>\n"
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
}
