#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

class http_conn {
public:
    // 文件名最大长度
    static const int FILENAME_LEN = 200;
    // 缓冲区大小
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP请求方法
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    // 解析客户请求时，主状态机所处的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, //解析请求行
        CHECK_STATE_HEADER,          //解析请求头
        CHECK_STATE_CONTENT          //解析消息体,用于POST
    };
    // 服务器处理HTTP请求结果
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    // 行的读取状态：完整 出错 尚不完整
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}

    ~http_conn() {}

public:
    // 初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr);

    // 关闭连接
    void close_conn(bool real_close = true);

    // 处理请求
    void process();

    // 非阻塞读写
    bool read_once();

    bool write();

    sockaddr_in *get_address() {
        return &m_address;
    }

    // 初始化MySQL连接
    void initmysql_result(connection_pool *connPool);

private:
    // 初始化连接
    void init();

    // 解析HTTP请求
    HTTP_CODE process_read();

    // 填充HTTP应答
    bool process_write(HTTP_CODE ret);

    // 被process_read调用解析请求
    HTTP_CODE parse_request_line(char *text);

    HTTP_CODE parse_headers(char *text);

    HTTP_CODE parse_content(char *text);

    HTTP_CODE do_request();

    char *get_line() { return m_read_buf + m_start_line; };

    LINE_STATUS parse_line();

    // 被process_read调用填充应答
    void unmap();

    bool add_response(const char *format, ...);

    bool add_content(const char *content);

    bool add_status_line(int status, const char *title);

    bool add_headers(int content_length);

    bool add_content_type();

    bool add_content_length(int content_length);

    bool add_linger();

    bool add_blank_line();

public:
    // 所有socket的事件都被注册到同一个epoll内核事件表,故epollfd为静态
    static int m_epollfd;
    // 统计用户数量
    static int m_user_count;
    MYSQL *mysql;

private:
    // HTTP连接的socket和对方socket地址
    int m_sockfd;
    sockaddr_in m_address;

    // 读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    // 已读入客户端数据最后一字节的下一位置
    int m_read_idx;
    // 正在分析的字符在缓冲区中位置
    int m_checked_idx;
    // 正在解析的行的起始位置
    int m_start_line;
    // 写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    // 写缓冲区中待发送字节数
    int m_write_idx;

    // 主状态机当前所处状态
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;

    // 客户请求文件的完整路径,等于doc_root+m_url
    char m_real_file[FILENAME_LEN];
    // 客户请求文件名
    char *m_url;
    // HTTP协议版本号
    char *m_version;
    // 主机名
    char *m_host;
    // HTTP请求消息体的长度
    int m_content_length;
    // HTTP请求是否要求保持连接
    bool m_linger;
    // 客户请求的文件被mmap到内存中的起始位置
    char *m_file_address;
    // 目标文件状态：存在？是目录？可读？
    struct stat m_file_stat;
    // 采用writev执行写操作
    struct iovec m_iv[2];
    // 被写内存块数量
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    // 要发的byte数
    int bytes_to_send;
    // 已发的byte数
    int bytes_have_send;
};

#endif
