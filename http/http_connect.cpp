#include "http_connect.h"

locker lock;
locker lock_pipe;

http_conn::http_conn()
{
    
    rec_buf = new char[MAX_BUF];
    snd_buf = new char[MAX_BUF];

    read_idx = 0;
    check_idx = 0;
    rec_content = 0;
    keep_conn = 1;
    snd_len = 0;
    byte_have_send = 0;
    byte_to_send = 0;
    range_start = -1;
    range_end = -1;
    improv = 0;
    interrupt = 0;
    is_range_requst = false;

    memset(rec_buf,0,MAX_BUF);
    memset(snd_buf,0,MAX_BUF);
    
    content_type = "Content-Type: text/html\r\n";
    accept_bytes = "Accept-Ranges: bytes\r\n";

    send_offset = 0;
    send_remaining = 0;
    video_fd = -1;
    mfi = new modfdInfo();
}


void http_conn::init()
{
    status = CHECK_REQUSTLINE;
    read_idx = 0;
    check_idx = 0;
    rec_content = 0;
    keep_conn = 1;
    snd_len = 0;
    method.clear();
    url.clear();
    file_name.clear();
    snd_header.clear();
    range_start = -1;
    range_end = -1;
    improv = 0;
    interrupt = 0;
    is_range_requst = false;
    content_type = "Content-Type: text/html\r\n";

    memset(rec_buf,0,MAX_BUF);
    memset(snd_buf,0,MAX_BUF);
    
    memset(ivc,0,sizeof(ivc));
    memset(&file_stat, 0, sizeof(file_stat)); // Use the member variable file_stat

    send_offset = 0;
    send_remaining = 0;
    if (video_fd != -1) 
    {
        close(video_fd);
        video_fd = -1;
    }
}


http_conn::~http_conn()
{
    delete[] rec_buf;
    delete[] snd_buf;
    delete mfi;
}

bool http_conn::readOnce()
{
    
    while(true)
    {
        int ret = recv(fd,rec_buf + read_idx,MAX_BUF-read_idx,0);
        
        if(ret == -1)
        {
            //如果未读完
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                return false;
            }
        }
        read_idx += ret;
    }
    buf = string(rec_buf,0,read_idx);
    //std::cout << string(rec_buf) << std::endl;
    return true;
}

http_conn::LINE_STATUS http_conn::readLine()
{
    ///std::cout << " run readLine" << ++count << std::endl;
   
    int index = buf.find('\r',check_idx);
    //std::cout << index <<std::endl;

    //未找到
    if(index == string::npos)
    {
        check_idx = buf.size()-1;
        return LINE_OPEN;
    }
    
    else if(index+1 == read_idx)
    {
        return LINE_OPEN;
    }

    else if(buf[index+1] != '\n')
    {
        return LINE_BAD;
    }
    
    check_idx = index + 2;
    

    return LINE_OK; 
}

//解析请求行
http_conn::HTTP_CODE http_conn::praseRequstLine(string text)
{
   //std::cout << " run praseRequstLine" <<std::endl;
   //std::cout << text <<std::endl;

    int method_idx = text.find(' ');
    if(method_idx == string::npos)
    {
        return BAD_REQUEST;
    }
    //std::cout << method_idx <<std::endl;
    method = string(text,0,method_idx-0);
    //text = string(text,method_idx+1,text.size()-1);

    int url_idx = text.find(' ', method_idx + 1);
    if(url_idx == string::npos)
    {
        return BAD_REQUEST;
    }
    url = string(text,method_idx+1,url_idx-method_idx-1);

    //std::cout << method << std::endl;
    //std::cout << url <<std::endl;

    //&& text.find("1.1") == string::npos
    if(text.find("HTTP") == string::npos )
    {
        return BAD_REQUEST;
    }
    status = CHECK_HEAD;
    return GET_REQUEST;
}


// 解析请求头
http_conn::HTTP_CODE http_conn::praseHeader(string text)
{
    //std::cout<<"run praseHeader" <<std::endl;
    //std::cout<<text <<std::endl;
    
    if(text.find("Content-Length") != string::npos)
    {
        int idx = text.find(':');
        if (idx != string::npos) 
        {
            string s = text.substr(idx + 1); 
            s.erase(0, s.find_first_not_of(" \t")); 
            rec_content = atoi(s.c_str());
        }

    }

    else if(text.find("Connection") != string::npos)
    {
        if(text.find("close") != string::npos)
        {
            keep_conn = 0;
        }
        else
        {
            keep_conn = 1;
        }
        //std::cout<<"keep_conn:"<<keep_conn<<std::endl;
    }

    else if(text.rfind("Range:", 0) == 0)
    {
        int idx = text.find("=");
        string temp = text.substr(idx + 1);
        idx = temp.find("-");
        range_start = atoi(string(temp, 0, idx).c_str());
        temp = temp.substr(idx + 1);
        if (!temp.empty() && temp[0] != '\r')
        {
            idx = temp.find_first_of(" \r");
            range_end = atoi(string(temp, 0, idx).c_str());
        }

        else
        {
            range_end = -1;
        }
        is_range_requst = true;

    }

    else if(text[0] == '\r')
    {
        if(rec_content != 0)
        {
            status = CHECK_CONTENT;
            //请求不完全
            return NO_REQUEST;
        }
        //得到完整请求
        return GET_REQUEST;
    }

   
    return NO_REQUEST;
}

//解析请求内容
http_conn::HTTP_CODE http_conn::praseContent(int index)
{
    //cout << "run praseContent" << endl;
    
    if(buf.size()-index < rec_content)
    {
        return NO_REQUEST;
    }

    content = string(buf,index,buf.size()-index);

    if(url == "/register-processing-url")
    {
        int index = content.find('&');
        username = string(content,9,index-9);
        
        content = content.substr(index+1);
        index = content.find('&');
        email = string(content,6,index-6);
        

        content = content.substr(index+1);
        index = content.find('&');
        password = string(content,9,index-9);
        

        content = content.substr(index+1);
        index = content.find('&');
        confirm_password= string(content,17,index-17);
        
    }

    else if(url == "/login-processing-url")
    {
        int index = content.find('&');
        username = string(content,9,index-9);
        //cout <<"用户名："<< username << endl;

        content = content.substr(index+1);
        index = content.find('&');
        password = string(content,9,index-9);
        //cout<<"密码：" << password << endl;

    }

    else if(url == "/set-new-password-processing-url")
    {
        //email=3027260189%40qq.com&username=123&new_password=123&confirm_password=123
        int index = content.find('&');
        email = string(content,6,index-6);
        

        content = content.substr(index+1);
        index = content.find('&');
        username = string(content,9,index-9);
        

        content = content.substr(index+1);
        index = content.find('&');
        password = string(content,13,index-13);
        

        content = content.substr(index+1);
        index = content.find('&');
        confirm_password= string(content,17,index-17);
        
    }

    //cout << content << endl;

    return GET_REQUEST;
}


http_conn::HTTP_CODE http_conn::doRequst()
{

    if (method == "POST")
    {
        if(url == "/register-processing-url")
        {
            if(password == confirm_password)
            {
                string query1 = "select * from user where email = '" + email + "';";
                string query2 = "select * from user where username = '" + username + "';";
                if(queryMysql(query1))
                {
                    file_name = "sourceFile/register_error_email.html";
                }

                else if(queryMysql(query2))
                {
                    file_name = "sourceFile/register_error_username.html";
                }
                
                else
                {
                    file_name = "sourceFile/register_success.html";
                    insertMysql();
                }
            }

            else
            {
                file_name = "sourceFile/register_error_password.html";
            }

        }

        else if(url == "/login-processing-url")
        {


            string query = "select * from user where username = '" + username + "' and passwd = '" + password + "';";
            //cout << query << endl;
            

            if(queryMysql(query))
            {
                file_name = "sourceFile/root/index.html";
                updateTimeIp();
            }

            else
            {
                file_name = "sourceFile/loginError.html";
            }
            
        }

        else if(url == "/set-new-password-processing-url")
        {

            if(updatePassword())
            {
                file_name = "sourceFile/update_password_success.html";
            }
            else
            {
                file_name = "sourceFile/update_password_error.html";
            }
        }

    }

    else if (method == "GET")
    {
        if (url == "/" || url == "/login.html")
        {
            file_name = "sourceFile/login.html";
        }

        // cout << file_name <<endl;
        else if (url == "/favicon.ico")
        {
            file_name = "sourceFile/favicon.ico";
        }

        else if (url == "/forgot-password.html")
        {
            file_name = "sourceFile/forgot-password.html";
        }

        else if (url == "/register.html")
        {
            file_name = "sourceFile/register.html";
        }

        else if (url == "/style.css")
        {
            content_type = "Content-Type: text/css\r\n";
            file_name = "sourceFile/root/style.css";
        }

        else if (url == "/script.js")
        {
            content_type = "Content-Type: application/javascript\r\n";
            file_name = "sourceFile/root/script.js";
        }

        else if (url == "/video.mp4")
        {
            content_type = "Content-Type: video/mp4\r\n";
            file_name = "sourceFile/root/video.mp4";

            if (stat(file_name.c_str(), &file_stat) < 0)
            {
                perror("stat failed");
                return NO_RESOURCE;
            }

            if (S_ISDIR(file_stat.st_mode))
            {
                return BAD_REQUEST;
            }

            if (!(file_stat.st_mode & S_IROTH))
            {
                return FORBIDDEN_REQUEST;
            }
            snd_len = file_stat.st_size;
            

            return FILE_REQUEST;
        }
    }

    if(stat(file_name.c_str(), &file_stat) < 0)
    {
        perror("stat failed");
        return NO_RESOURCE;
    }

    if (S_ISDIR(file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    if(!(file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }

    int file_fd = open(file_name.c_str(),O_RDONLY);
    //cout << fd << endl;

    snd_content = (char*)mmap(NULL,file_stat.st_size,PROT_READ,MAP_SHARED,file_fd,0);

    if (snd_content == MAP_FAILED) {
        perror("mmap failed"); // 打印错误信息
        snd_content = nullptr; // 避免后续使用 MAP_FAILED
        snd_len = 0;
        // 可能需要 unmap 吗？通常不需要，因为映射未成功
        return INTERNAL_ERROR; // 或者其他合适的错误码
    }
    
    snd_len = file_stat.st_size;
    close(file_fd);

    //std::cout<<"run doRequst" <<std::endl;

    return FILE_REQUEST;
}


http_conn::HTTP_CODE http_conn::processRead()
{
    //std::cout << " run processRead" <<std::endl;
    LINE_STATUS l_status = LINE_OK;
    string text;
    //当前解析行的起始位置
    int index = 0;

    HTTP_CODE ret;
    while((status == CHECK_CONTENT && l_status == LINE_OK) || ((l_status = readLine()) == LINE_OK))
    {
        text = string(buf,index,check_idx - index);
        index = check_idx;
        //cout << text << endl;
        switch (status)
        {
        case CHECK_REQUSTLINE:
            ret = praseRequstLine(text);
            if(ret ==BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        case CHECK_HEAD:
            ret = praseHeader(text);
            if(ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }

            else if(ret == GET_REQUEST)
            {
                return doRequst();
            }
            break;
        case CHECK_CONTENT:
        //传进报文的开始下标
            ret = praseContent(index);
            if(ret == GET_REQUEST)
            {           
                return  doRequst();
            }
            l_status = LINE_OPEN;
            break;
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}


void http_conn::processWrite(http_conn::HTTP_CODE ret)
{
    char cwd[100];
    /*if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        cout << cwd << endl;
    } else {
        perror("getcwd failed");
    }*/

    if(ret == NO_RESOURCE)
    {
        cout << file_name ;
        cout << " : No resource" << endl;
    }

    else if(ret == FILE_REQUEST)
    {
        if (url == "/video.mp4")
        {
            if (is_range_requst && range_start != -1 && range_start < file_stat.st_size)
            {
                if(range_end == -1)
                {
                    range_end = snd_len - 1;
                }

                snd_header = snd_header 
                + "HTTP/1.1 206 Partial Content\r\n" 
                + content_type + "Content-Range: bytes " 
                + std::to_string(range_start) + "-" 
                + std::to_string(range_end) + "/" 
                + std::to_string(snd_len) + "\r\n" 
                + "Content-Length: " 
                + std::to_string(range_end - range_start + 1) 
                + "\r\n" + accept_bytes
                +"Cache-Control: public, max-age=86400\r\n\r\n";

                //cout << snd_header << endl;

                return;
            }

            else 
            {
                snd_header =snd_header +  "HTTP/1.1 200 OK\r\n" 
                + content_type 
                + "Content-Length: " + std::to_string(snd_len) + "\r\n"
                +accept_bytes + "\r\n";
                
                range_start = 0;
                range_end = snd_len - 1;
                return ;

            }
        }

        snd_header =snd_header +  "HTTP/1.1 200 OK\r\n" 
        + content_type 
        + "Content-Length: " + std::to_string(snd_len) + "\r\n";
        if (keep_conn == 1)
        {
            snd_header += "Connection: keep-alive\r\n";
        }
        else
        {
            snd_header += "Connection: close\r\n";
        }

    
        snd_header += "\r\n";
        //cout << snd_header << endl;

        ivc[0].iov_base = (char *)snd_header.c_str();
        ivc[0].iov_len = snd_header.size();

        ivc[1].iov_base = snd_content;
        ivc[1].iov_len = snd_len;
        
        //std::cout << string(snd_content) << std::endl;
    }


    byte_to_send = ivc[0].iov_len + ivc[1].iov_len;
}

void http_conn::test()
{
    //readOnce();
    HTTP_CODE ret = processRead();
    if(ret == NO_REQUEST)
    {
        modfd(1);
        //tool.modifyfd(fd,EPOLLIN,true);
        return;
    }

    else if(ret == BAD_REQUEST)
    {
        modfd(3);
        //tool.deletefd(tool.m_epolled,fd);
        return;
    }

    //cout << ret << endl;
    processWrite(ret);

    modfd(2);
    //tool.modifyfd(fd,EPOLLOUT,true);
}

bool http_conn::writefile()
{

    while (true)
    {

        size_t header_bytes_to_send_now = ivc[0].iov_len;

        int ret = writev(fd, ivc, 2);

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(2);    
                return true; 
            }
            perror("writev fail");
            if (snd_content)
            { 
                munmap(snd_content, snd_len);
                snd_content = nullptr; 
            }
            
            return false; 
        }

       
        byte_to_send -= ret;   
        byte_have_send += ret; 

        if (ret < header_bytes_to_send_now)
        {
            
            ivc[0].iov_base = (char *)ivc[0].iov_base + ret;
            ivc[0].iov_len -= ret;
        }
        else
        {
            
            size_t body_bytes_sent = ret - header_bytes_to_send_now;
            ivc[0].iov_len = 0; 

            ivc[1].iov_base = (char *)ivc[1].iov_base + body_bytes_sent;
            ivc[1].iov_len -= body_bytes_sent;
        }
        if (byte_to_send <= 0)
        {
            
            if (snd_content)
            {
                munmap(snd_content, snd_len);
                snd_content = nullptr;
            }

            if (keep_conn == 1)
            {
                modfd(1);    
                init();      
                return true; 
            }
            else
            {
            
                return false; 
            }
        }
        
    } 
}

bool http_conn::sendvideo() {
    // 1. 发送头部
    int len = snd_header.size();
    //cout <<  " snd_header : " << len << endl;
    while (len > 0) {
        int ret = send(fd, snd_header.c_str(), len, 0);
        if (ret <= 0) {
            if (errno == EAGAIN)
            {
                break;
            }
            perror("send header failed");
            return false;
        }
        snd_header = snd_header.substr(ret);
        len -= ret;
    }

    // 2. 头部发送完成后，发送视频文件
    if (!snd_header.empty()) {
        // 头部未发送完成，返回 true 等待下次可写事件
        //tool.modifyfd(fd, EPOLLOUT, true);
        modfd(2);
        return true;
    }

    // 3. 如果是第一次进入发送文件阶段，打开文件
    if (video_fd == -1) {
        video_fd = open(file_name.c_str(), O_RDONLY);
        if (video_fd < 0) {
            perror("open video file failed");
            return false;
        }
        send_offset = range_start;
        send_remaining = range_end - range_start + 1;
    }

    // 4. 发送视频数据
    while (send_remaining > 0) {
        ssize_t sent = sendfile(fd, video_fd, &send_offset, send_remaining);
        //if(sent > 0)
        //{
            //cout << "have send : " << sent <<endl;

        //}
        

        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                modfd(2);
                //tool.modifyfd(fd, EPOLLOUT, true);
                return true;
            }
            perror("sendfile failed");
            close(video_fd);
            video_fd = -1;
            return false;
        }

        else if(sent == 0)
        {
            //tool.modifyfd(fd, EPOLLOUT, true);
            //return true;
            break;
        }

        send_remaining -= sent;
    }

    // 5. 发送完成
    if (send_remaining == 0) {
        close(video_fd);
        video_fd = -1;   // reset!
        modfd(1);
        //tool.modifyfd(fd, EPOLLIN, true);
        init();
        return true;
    }

    return true;
}

bool http_conn::process()
{
    if(url == "/video.mp4")
    {
        return sendvideo();
    }
    else
    {
        return writefile();
    }
}

bool http_conn::queryMysql(string query)
{
    if(!mysql)
    {
        //cout << "check : mysql is null" << endl;
        return false;
    }

    //查询成功
    if(mysql_query(mysql,query.c_str()) == 0)
    {
        MYSQL_RES* result = mysql_store_result(mysql);
        if(mysql_num_rows(result) > 0)
        {
            mysql_free_result(result);
            return true;
        }

        mysql_free_result(result);
    }
    
    return false;
}

bool http_conn::insertMysql()
{
    if(!mysql)
    {
        //cout << "insert : mysql is null" << endl;
        return false;
    }

    char  ip[20];

    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    string s_ip(ip);

    string query = "insert into user(username, passwd, email, time, ip) values('" 
    + username + "','" + password + "','" + email + "','" + tool.getCurrentTime() + "','" + s_ip + "');";

    lock.lock();
    //插入成功
    if(mysql_query(mysql,query.c_str()) == 0)
    {
        lock.unlock();
        return true;
    }
    lock.unlock();
    return false;
}

bool http_conn::updatePassword()
{
    if(!mysql)
    {
        //cout << "update : mysql is null" << endl;
        return false;
    }
    string query = "select * from user where email = '" + email + "' and username = '" + username + "';";

    string query2 = "update user set passwd = '" + password + "' where email = '" + email + "' and username = '" + username + "';";

    //信息正确
    if(password == confirm_password && queryMysql(query))
    {
        lock.lock();
        int ret = mysql_query(mysql,query2.c_str());
        
        if(ret == 0 )
        {
            lock.unlock();
            return true;
        }
        else
        {
            lock.unlock();
            return false;
        }
    }

    
}

bool http_conn::updateTimeIp()
{
    //cout << "run updateTimeIp" << endl;
    if(!mysql)
    {
        //cout << "update time ip : mysql is null" << endl;
        return false;
    }
    char  ip[20];

    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    string s_ip(ip);

    string query = "update user set time = '" + tool.getCurrentTime() + "', ip = '" + s_ip + "' where username = '"
    + username + "';";

    lock.lock();
    int ret = mysql_query(mysql, query.c_str());

    if (ret == 0)
    {
        lock.unlock();
        return true;
    }
    else
    {
        lock.unlock();
        return false;
    }

    lock.unlock();
    return false;
}

void http_conn::modfd(int op)
{
    mfi->fd = fd;
    mfi->modfd = op; // 改为读
    lock_pipe.lock();
    write(modfd_pipe, mfi, sizeof(modfdInfo));
    lock_pipe.unlock();
}