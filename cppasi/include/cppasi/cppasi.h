//
//  Created by Yagita Hiroki on 2014/02/21.
//  Copyright (c) 2014 Hiroki Yagita. All rights reserved.
//

#ifndef cppasi_cppasi
#define cppasi_cppasi

#include <map>
#include <memory>
#include <string>
#include <vector>


/**
 @mainpage
 
 <h1>CPPASI</h1>
 
 <h2>Overview for C++ API Server Interface</h2>
 
 Client - Server な構成で
 * Client から Server へのアクセスは HTTP ベース
 * Server 側は REST ベースの API を提供
 というシステムを構築するための WSGI に相当するフレームワークです。
 
 <h2>The Environment</h2>
 
 The environment provides by @c environment parameter of @c call() method of @c ApplicationInterface interface
 
 @c environment includes the following keys:
 - "REQUEST_METHOD" : The HTTP request method, such as "GET" or "POST".
 - "SCRIPT_NAME" : Always empty string.
 - "PATH_INFO" : The portion of the request URL that accord the path component.
 - "QUERY_STRING" : The portion of the request URL that follows the ?.
 - "SERVER_NAME" : The portion of the request URL that accord the host name.
 - "SERVER_PORT" : The portion of the request URL that accord the port.
 
 
 <h2>For Application Layer</h2>
 
 Application Layer で最小限に行わなければいけないことは
 
 - ApplicationInterface を実装する
 - ApplicationInterface のインスタンスを生成する
 - C++ ASI 側に ApplicationInterface のインスタンスを渡す
 
 となります。
 
 サーバー設定の初期値は cppasi::cppasi.run() の第二引数で渡します。
 第二引数に NULL を渡すとデフォルト動作になります。
 
 - "server_name" : サーバーのホスト名を指定します。デフォルトは gethostname(2) の結果です。
 - "bind_address" : サーバーの bind アドレスを "0.0.0.0" 等の形式で指定します。デフォルトは "0.0.0.0" です。
 - "port" : サーバーのポートを "80" 等で指定します。デフォルトは "7077" です。
 
 <h2>Sample Code</h2>
 ASI を使った最小限のサンプル実装は以下となります。
 
 @code
 
 class MyApplication : public cppasi::ApplicationInterface
 {
 public:
     virtual void
     call(const cppasi::environment_t &environment,
          int &response_status_code,
          std::string &response_body,
          cppasi::headers_t &response_headers);
 };
 
 void
 MyApplication::call(const cppasi::environment_t &environment,
                     int &response_status_code,
                     std::string &response_body,
                     cppasi::headers_t &response_headers)
 {
     response_status_code = 200;
     response_headers.insert(std::make_pair("Content-Type", "text/plain"));
     response_body.assign("Hello, ASI!!");
 }
 
 int main(int argc, char *argv[])
 {
    cppasi::cppasi.run(new MyApplication());
 }
 
 @endcode
 
 実行例は以下となります。
 @code
 $ curl http://localhost:7077
 Hello, ASI!!
 @endcode
 
 <h2>Dependency</h2>
 外部依存ライブラリは以下のとおりです。
 - libevent: http://libevent.org/
   - 2.0.21 でテストしています
 */


/**
 ASI namespace
 */
namespace cppasi
{
    class ASI;
    class ApplicationInterface;
    
    /**
     @see ASI::version()
     */
    typedef std::vector<int> version_t;
    
    /**
     @see ApplicationInterface::call()
     */
    typedef std::map<std::string, std::string> environment_t;
    
    /**
     @see ASI::run()
     */
    typedef std::map<std::string, std::string> config_t;
    
    /**
     @see ApplicationInterface::call()
     */
    typedef std::map<std::string, std::string> headers_t;
    
    /**
     アプリケーション <=> ASI 間のインターフェイスになるオブジェクトです。
     ASI が規定するインターフェイスはこのオブジェクト経由で呼び出してください。
     */
    extern ASI cppasi;
    
    /**
     ASI のコアクラスです。
     */
    class ASI
    {
    public:
        
        // for config prameters in run() method
        static const std::string kSERVER_NAME;  ///< server_name key
        static const std::string kBIND_ADDRESS; ///< bind_address key
        static const std::string dBIND_ADDRESS; ///< bind_address default value
        static const std::string kPORT;         ///< port key
        static const std::string dPORT;         ///< port default value
        
        ASI() = default;
        virtual ~ASI() = default;
        
        /**
         The user application is invoked by ASI calls application.call() method.
         
         @param[in,out] application アプリケーション側で実装した ApplicationInterface のインスタンスを渡してください。
         @param[in] config 初期設定を変更する場合に指定してください。NULL を指定した場合、デフォルト動作を意味します。
         */
        virtual void run(ApplicationInterface *application, const config_t *config = NULL);
        
        /**
         Return cppasi framework version.
         
         @return
         - index 0 : major version
         - index 1 : minor version
         */
        std::unique_ptr<version_t> version() const;
        
        /** @internal */
        void routing(void *object);
        
    protected:
        
        std::unique_ptr<ApplicationInterface> application_;
        config_t config_;
        
    private:
        
        ASI(ASI const &) = delete;
        ASI(ASI &&) = delete;
        ASI & operator = (ASI const&) = delete;
        ASI & operator = (ASI &&) = delete;
        
        void configure_by(const config_t *);
        
    };
    
    
    /**
     @brief CPPASI 上で動作するアプリケーションが実装しなければならないインターフェイスを定義しています。
     */
    class ApplicationInterface
    {
    public:
        
        /**
         クライアントからのリクエストを受け取った後に ASI => アプリケーション側へ通知する際に呼ばれるインターフェイスです。
         
         アプリケーションはこのメソッドを実装することによってアプリケーションとしての振る舞いを定義することができます。
         
         @param[in] environment クライアントからのリクエストを基に生成した各種変数を設定した key-value を ASI が設定した上でアプリケーション側に渡すためのオブジェクトです。
         @param[out] response_status_code アプリケーションとしての処理結果を HTTP Status Code で指定してください。
         @param[out] response_body アプリケーションとしての処理結果のうち、レスポンスのボディ部分で返却する内容を指定してください。
         @param[out] response_headers アプリケーションとしての処理結果のうち、HTTP Header 部分で返却する内容を指定してください。
         */
        virtual void
        call(const environment_t &environment,
             int &response_status_code,
             std::string &response_body,
             headers_t &response_headers) = 0;
        
    };
    
}

#endif
