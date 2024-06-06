#include <boost/json.hpp>
#include "handler_serializer.h"

namespace http_handler {

void tag_invoke(boost::json::value_from_tag, boost::json::value& val, const ErrMsg& err_msg) {
    val = {{"code", err_msg.code}, {"message", err_msg.message}};
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& val, const JoinMsg& join_msg) {
    val = {{"authToken", join_msg.auth_token}, {"playerId", join_msg.player_id}};
}

} // namespace http_handler