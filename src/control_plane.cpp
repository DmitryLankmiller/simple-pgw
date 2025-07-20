#include <control_plane.h>
#include <vector>

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    auto it = _pdns.find(cp_teid);
    return it != _pdns.end() ? it->second : std::shared_ptr<pdn_connection>{};
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    auto it = _pdns_by_ue_ip_addr.find(ip);
    return it != _pdns_by_ue_ip_addr.end() ? it->second : std::shared_ptr<pdn_connection>{};
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    auto it = _bearers.find(dp_teid);
    return it != _bearers.end() ? it->second : std::shared_ptr<bearer>{};
}

uint32_t get_next_cp_teid() {
    static uint32_t cp_teid = 1;
    return cp_teid++;
}

boost::asio::ip::address_v4 get_next_ue_ip_addr() {
    static uint32_t counter = 1;
    return boost::asio::ip::make_address_v4("10.0.0." + std::to_string(counter++));
}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
                                                                     boost::asio::ip::address_v4 sgw_addr,
                                                                     uint32_t sgw_cp_teid) {
    auto apns_it = _apns.find(apn);
    if (apns_it == _apns.end()) {
        return std::shared_ptr<pdn_connection>{};
    }
    auto apn_gw = apns_it->second;

    auto cp_teid = get_next_cp_teid();
    boost::asio::ip::address_v4 ue_ip_addr = get_next_ue_ip_addr();

    auto pdn = pdn_connection::create(cp_teid, apn_gw, ue_ip_addr);

    pdn->set_sgw_cp_teid(sgw_cp_teid);
    pdn->set_sgw_addr(sgw_addr);

    _pdns[cp_teid] = pdn;
    _pdns_by_ue_ip_addr[ue_ip_addr] = pdn;
    return pdn;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    auto pdns_it = _pdns.find(cp_teid);
    if (pdns_it == _pdns.end()) {
        return;
    }
    auto pdn = pdns_it->second;
    _pdns_by_ue_ip_addr.erase(pdn->get_ue_ip_addr());

    std::vector<uint32_t> bearer_teids_to_delete;

    bearer_teids_to_delete.push_back(pdn->get_default_bearer()->get_dp_teid());

    for (const auto &[dp_teid, bearer]: pdn->_bearers) {
        bearer_teids_to_delete.push_back(dp_teid);
    }

    for (const auto dp_teid: bearer_teids_to_delete) {
        delete_bearer(dp_teid);
    }

    _pdns.erase(pdns_it);
}

uint32_t get_next_dp_teid() {
    static uint32_t dp_teid = 1;
    return dp_teid++;
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    uint32_t dp_teid = get_next_dp_teid();

    auto new_bearer = std::make_shared<bearer>(dp_teid, *pdn);

    new_bearer->set_sgw_dp_teid(sgw_teid);

    pdn->add_bearer(new_bearer);
    _bearers[dp_teid] = new_bearer;

    return new_bearer;
}

void control_plane::delete_bearer(uint32_t dp_teid) {
    auto bearers_it = _bearers.find(dp_teid);
    if (bearers_it == _bearers.end()) {
        return;
    }

    auto bearer = bearers_it->second;

    bearer->get_pdn_connection()->remove_bearer(dp_teid);
    _bearers.erase(bearers_it);
}

void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns[apn_name] = apn_gateway;
}
