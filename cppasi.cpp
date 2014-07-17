//
//  Created by Yagita Hiroki on 2014/02/21.
//  Copyright (c) 2014 Hiroki Yagita. All rights reserved.
//

#include <algorithm>
#include <cerrno>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <stdexcept>
#include <syslog.h>
#include <unistd.h>
#include <utility>
#include "cppasi.h"



void __dispatch(struct evhttp_request *request, void *args)
{
    cppasi::ASI *asi = reinterpret_cast<cppasi::ASI *>(args);
    asi->routing(request);
}



namespace cppasi
{
    
    ASI cppasi;
    
    const std::string ASI::kSERVER_NAME  = "server_name";
    const std::string ASI::kBIND_ADDRESS = "bind_address";
    const std::string ASI::dBIND_ADDRESS = "0.0.0.0";
    const std::string ASI::kPORT = "port";
    const std::string ASI::dPORT = "7077";

    void
    ASI::run(ApplicationInterface *application, const config_t *config)
    {
        if (!application) {
            syslog(LOG_ERR, "Failed %s", __FUNCTION__);
            throw std::runtime_error("");
        }

        syslog(LOG_INFO, "Succeeded %s", __FUNCTION__);
        
        application_.reset(application);
        
        configure_by(config);
        
        struct event_base *base = event_base_new();
        struct evhttp *httpd = evhttp_new(base);
        evhttp_bind_socket(httpd,
                           config_.at(kBIND_ADDRESS).c_str(),
                           std::stoi(config_.at(kPORT)));
        evhttp_set_allowed_methods(httpd,
                                   EVHTTP_REQ_GET|EVHTTP_REQ_POST);
        evhttp_set_gencb(httpd, __dispatch, this);
        event_base_dispatch(base);
        evhttp_free(httpd);
        
        syslog(LOG_INFO, "Exit %s", __FUNCTION__);
    }
    
    std::unique_ptr<version_t>
    ASI::version() const
    {
        std::unique_ptr<version_t> v(new version_t());
        v->push_back(0);
        v->push_back(1);
        return v;
    }
    
    void
    ASI::routing(void *object)
    {
        struct evhttp_request *request = reinterpret_cast<struct evhttp_request *>(object);
        const char *uristr = evhttp_request_get_uri(request);
        const struct evhttp_uri *uri = evhttp_request_get_evhttp_uri(request);
        enum evhttp_cmd_type cmd = evhttp_request_get_command(request);
        
        syslog(LOG_INFO, "Enter api_receiver_master_router URI=%s, CMD=%04x",
               uristr, cmd);
        
        environment_t environment;
        
        std::string request_method = (cmd & EVHTTP_REQ_GET) ? "GET" : "POST";
        environment.insert(std::make_pair("REQUEST_METHOD", request_method));
        environment.insert(std::make_pair("SCRIPT_NAME", ""));
        const char *path = evhttp_uri_get_path(uri);
        environment.insert(std::make_pair("PATH_INFO", (path) ? path : ""));
        const char *query = evhttp_uri_get_query(uri);
        environment.insert(std::make_pair("QUERY_STRING", (query) ? query : ""));
        environment.insert(std::make_pair("SERVER_NAME", config_.at(kSERVER_NAME)));
        environment.insert(std::make_pair("SERVER_PORT", config_.at(kPORT)));

        int response_status_code = 0;
        std::string response_body;
        headers_t response_headers;

        application_->call(environment, response_status_code, response_body, response_headers);
    
        struct evbuffer *buf = evbuffer_new();
        if (!buf) {
            evhttp_send_error(request, HTTP_INTERNAL, "Internal Server Error.");
            return;
        }
        struct evkeyvalq *output_headers = evhttp_request_get_output_headers(request);
        std::for_each(response_headers.begin(), response_headers.end(), [&](headers_t::value_type &kv) {
            evhttp_add_header(output_headers, kv.first.c_str(), kv.second.c_str());
        });
        
        evbuffer_add_printf(buf, "Response: code:%d, body:%s", response_status_code, response_body.c_str());
        evhttp_send_reply(request, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
    }

    void
    ASI::configure_by(const config_t *config)
    {
        if (config) {
            config_ = *config;
        }
        
        if (config_.find(kSERVER_NAME) == config_.end()) {
            char name[256];
            if (::gethostname(name, sizeof(name)) == 0) {
                config_.insert(std::make_pair(kSERVER_NAME, name));
            } else {
                syslog(LOG_ERR, "%s(): failed call gethostname(), errno=%d", __FUNCTION__, errno);
            }
        }
        if (config_.find(kBIND_ADDRESS) == config_.end()) {
            config_.insert(std::make_pair(kBIND_ADDRESS, dBIND_ADDRESS));
        }
        if (config_.find(kPORT) == config_.end()) {
            config_.insert(std::make_pair(kPORT, dPORT));
        }
    }

    
} // namespace cppsgi

