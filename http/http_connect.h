#pragma once
#include <string>
#include <string.h>
#include <iostream>
#include "../tool.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <arpa/inet.h> 
#include <mysql/mysql.h>
#include <sys/sendfile.h>
#include "../locker/locker.h"
using std::string;
using std::cout;
using std::endl;

#define MAX_BUF 4096
class http_conn
{
public:
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    
    enum CHECK_STATUS
    {
        CHECK_REQUSTLINE = 0,
        CHECK_HEAD,
        CHECK_CONTENT
    };

    enum HTTP_CODE
    {
        NO_REQUEST,//请求不完全
        GET_REQUEST,//完整解析请求
        BAD_REQUEST,//请求格式错误
        NO_RESOURCE,//资源不存在
        FORBIDDEN_REQUEST,//拒绝访问
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    //初始化
    void init();
    //一次性读完
    bool readOnce();
    HTTP_CODE processRead();
    void processWrite(HTTP_CODE ret);
    HTTP_CODE praseRequstLine(string text);
    HTTP_CODE praseHeader(string text);
    HTTP_CODE praseContent(int index);
    LINE_STATUS readLine();
    //写响应报文
    HTTP_CODE doRequst();
    void test();
    bool write();
    bool process();
    bool sendvideo();

    http_conn();
    ~http_conn();

    struct sockaddr_in addr; 
    CHECK_STATUS status;
    char* rec_buf;
    char* snd_buf;
    string buf;
    //报文
    string content;

    struct iovec ivc[2];

    //登录信息
    string username;
    string email;
    string password;
    string confirm_password;

    //已经读入的数据
    int read_idx;
    //当前已经从buf中读取的下标
    int check_idx;
    //是否保持连接
    int keep_conn;
    //接受的报文长度
    int rec_content;
    //发送的报文长度
    long long  snd_len;
    int byte_to_send;
    int byte_have_send;
   
    //GET或POST
    string method;
    string url;

    //所要访问的文件名
    string file_name;
    //char  filename[100];
    struct stat file_stat;
    //发送响应头
    string snd_header;
    //发送响应内容
    char* snd_content;

    long long range_start;
    long long  range_end;
    bool is_range_requst;
    
    string content_type;
    string accept_bytes;
    Tool  tool;
    //该连接的文件描述符
    int fd;

    MYSQL* mysql;
    bool queryMysql(string query);
    bool insertMysql();
    bool updatePassword();
    bool updateTimeIp();


    int video_fd;
    off_t send_offset;
    long long send_remaining;

    //读还是写
    int flag;
    //状态是否改变
    int improv;
    int interrupt;
};