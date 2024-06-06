#ifndef GAME_SERVER_HANDLER_SERIALIZER_H
#define GAME_SERVER_HANDLER_SERIALIZER_H

namespace http_handler {

struct ErrMsg {
    std::string code;
    std::string message;
};

struct JoinMsg {
    uint64_t player_id;
    std::string auth_token;
};

void tag_invoke(boost::json::value_from_tag, boost::json::value& val, const ErrMsg& err_msg);
void tag_invoke(boost::json::value_from_tag, boost::json::value& val, const JoinMsg& join_msg);

} // namespace http_handler

#endif //GAME_SERVER_HANDLER_SERIALIZER_H
