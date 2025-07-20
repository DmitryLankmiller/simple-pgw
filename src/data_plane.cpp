#include <data_plane.h>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

void data_plane::handle_uplink(uint32_t dp_teid, Packet &&packet) {
    auto bearer = _control_plane.find_bearer_by_dp_teid(dp_teid);
    if (!bearer) {
        return;
    }

    auto pdn_connection = bearer->get_pdn_connection();
    if (!pdn_connection) {
        return;
    }

    forward_packet_to_apn(pdn_connection->get_apn_gw(), std::move(packet));
}

void data_plane::handle_downlink(const boost::asio::ip::address_v4 &ue_ip, Packet &&packet) {
    auto pdn_connection = _control_plane.find_pdn_by_ip_address(ue_ip);
    if (!pdn_connection) {
        return;
    }

    auto bearer = pdn_connection->get_default_bearer();
    if (!bearer) {
        return;
    }

    forward_packet_to_sgw(pdn_connection->get_sgw_address(), bearer->get_sgw_dp_teid(), std::move(packet));
}
