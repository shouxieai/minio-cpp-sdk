/**
 * @file minio_client.cpp
 * @author 手写AI
 *         我们的B站：https://space.bilibili.com/1413433465/
 *         我们的博客：http://zifuture.com:8090
 * 
 * @brief  基于CURL提交AWS的访问协议，实现对minio进行操作（创建bucket、查询bucket、上传、下载）
 *         被AWS的cpp-sdk的65w个文件吓怕了，就想传个文件咋就这么难
 * 
 * @date 2021年7月28日 17:56:02
 * 
 *   注意，主要分析手段，是使用minio的js-sdk看他提交http时给的参数是什么，进而估计出C++应该怎么写
 *   minio的js-sdk用的签名方式不同（是AWS4-HMAC-SHA256），如果能完全模拟他，就好了  
 * 
 *   参考：
 *   1. https://github.com/minio/minio/issues/8136
 *   2. https://github.com/kneufeld/minio-put
 *   3. https://docs.min.io/docs/javascript-client-quickstart-guide.html
 *   4. https://github.com/minio/minio-js/blob/master/src/main/minio.js
 */

#ifndef MINIO_CLIENT_HPP
#define MINIO_CLIENT_HPP


#include <string>
#include <vector>

class MinioClient{
public:

    /**
     * @brief 创建一个minio的客户端
     *     这个类实际上没干嘛，仅仅是记录3个字符串避免重复传参数
     *     每个函数都是独立运行的，可以允许多线程
     * 
     * @param server        指定服务器地址，例如：http://127.0.0.1:9000，注意不要多斜杠
     * @param access_key    指定访问的key，例如：F2IHVVX44WVGYUIA1ESX
     * @param secret_key    指定加密的key，例如：UiJuXEG4V6ZLqCZ9ZbD9lqKEG8WwtaKeA3kh7Lui
     */
    MinioClient(const std::string& server, const std::string& access_key, const std::string& secret_key);


    /**
     * @brief 上传文件到minio服务器-通过文件路径
     * 
     * @param remote_path  指定远程路径，bucket也包含在内，例如：/test-bucket/wish/wish235.txt
     * @param file         指定本地的文件路径，例如：echo.txt
     * @return 如果成功返回true，否则返回false并打印消息
     */
    bool upload_file(const std::string& remote_path, const std::string& file);


    /**
     * @brief 上传文件到minio服务器-通过文件数据
     * 
     * @param remote_path  指定远程路径，bucket也包含在内，例如：/test-bucket/wish/wish235.txt
     * @param file         指定本地的文件路径，例如：echo.txt
     * @return 如果成功返回true，否则返回false并打印消息
     */
    bool upload_filedata(const std::string& remote_path, const std::string& file_data);


    /**
     * @brief 上传文件到minio服务器-通过文件数据
     * 
     * @param remote_path  指定远程路径，bucket也包含在内，例如：/test-bucket/wish/wish235.txt
     * @param file         指定本地的文件路径，例如：echo.txt
     * @return 如果成功返回true，否则返回false并打印消息
     */
    bool upload_filedata(const std::string& remote_path, const void* file_data, size_t data_size);


    /**
     * @brief 获取服务器bucket列表
     * 
     * @return std::vector<std::string> 
     */
    std::vector<std::string> get_bucket_list(bool* pointer_success = nullptr);


    /**
     * @brief 创建新的bucket，如果已经存在会报错
     * 
     * @param name           指定bucket的名字，例如：test-bucket
     * @return true 
     * @return false 
     */
    bool make_bucket(const std::string& name);


    /**
     * @brief 下载读取文件数据
     * 
     * @param remote_path     指定远程路径，bucket也包含在内，例如：/test-bucket/wish/wish235.txt
     * @return std::string    返回文件数据，string类型打包的，string.data()是指针，string.size()是长度
     */
    std::string get_file(const std::string& remote_path, bool* pointer_success=nullptr);

private:
    std::string server;
    std::string access_key;
    std::string secret_key;
};

#endif // MINIO_CLIENT_HPP
