#ifndef REQUESTDATA
#define REQUESTDATA
#include <string>
#include <cstring>
#include <unordered_map>
#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>
#include <iostream>
using namespace std;

const int STATE_PARSE_URI = 1;       //状态解析 URI（统一资源标识符）
const int STATE_PARSE_HEADERS = 2;    //解析 HTTP 请求头
const int STATE_RECV_BODY = 3;        // 
const int STATE_ANALYSIS = 4;         // 状态分析
const int STATE_FINISH = 5;           // 解析结束
const int MAX_BUFF = 4096;            // 最大缓存

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int AGAIN_MAX_TIMES = 200;       // 最大尝试 200 次

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;


/* MIME (Multipurpose Internet Mail Extensions) 是描述消息内容类型的因特网标准，
    说白了也就是文件的媒体类型。
    浏览器可以根据它来区分文件，然后决定什么内容用什么形式来显示。 */

class MimeType
{
private:
    static pthread_mutex_t lock;
    static std::unordered_map<std::string, std::string> mime;    // 散列表，键、值均为字符串
    MimeType();
    MimeType(const MimeType &m);
public:
    static std::string getMime(const std::string &suffix);  // 获取媒体类型
};

enum HeadersState
{
    h_start = 0,
    h_key,
    h_colon,        // 冒号
    h_spaces_after_colon,
    h_value,
    h_CR,     // 回车
    h_LF,     // 换行
    h_end_CR,
    h_end_LF
};

struct mytimer;          // 定时器，用于删除 超时事件 等等。
struct requestData;     // 将就绪 IO 套接字发过来的请求 封装成一个 requestData 对象。    

struct requestData
{
private:
    int againTimes;
    std::string path;
    int fd;
    int epollfd;
    // content的内容用完就清
    std::string content;
    int method;
    int HTTPversion;
    std::string file_name;
    int now_read_pos;
    int state;
    int h_state;
    bool isfinish;
    bool keep_alive;
    std::unordered_map<std::string, std::string> headers;
    mytimer *timer;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

public:

    requestData();
    requestData(int _epollfd, int _fd, std::string _path);
    ~requestData();
    void addTimer(mytimer *mtimer);
    void reset();
    void seperateTimer();
    int getFd();
    void setFd(int _fd);
    void handleRequest();
    void handleError(int fd, int err_num, std::string short_msg);
};

struct mytimer
{
    bool deleted;
    size_t expired_time;
    requestData *request_data;

    mytimer(requestData *_request_data, int timeout);
    ~mytimer();
    void update(int timeout);
    bool isvalid();
    void clearReq();
    void setDeleted();
    bool isDeleted() const;
    size_t getExpTime() const;
};

struct timerCmp          // 比较两个定时器
{
    bool operator()(const mytimer *a, const mytimer *b) const;
};

#endif