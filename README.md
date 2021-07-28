# minio-cpp-sdk，minio上传的C++API
- 基于调用libcurl和openssl，实现对minio操作
- 提交AWS的访问协议，实现对minio（创建bucket、查询bucket、上传、下载）
- 被AWS的cpp-sdk的65w个文件吓怕了，就想传个文件咋就这么难

# 示例代码
```C++
const char server     = "https://play.min.io:9000";
const char access_key = "Q3AM3UQ867SPQQA43P2F";
const char secret_key = "zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG";
MinioClient minio(server, access_key, secret_key);

const char file       = "echo.txt";
minio.upload_file("/test-bucket/echo.txt", file);
auto data = minio.get_file("/test-bucket/echo.txt");
```

# 使用
1. 配置openssl，下载并编译
2. 配置curl，配置具有ssl支持的curl
3. 修改Makefile中的openssl和curl路径，运行`make run`
```bash
cd minio-cpp-sdk
make run -j6
```

# 关于我们-手写AI
- 我们的B站：https://space.bilibili.com/1413433465/
- 我们的博客：http://zifuture.com:8090

# 参考：
- 注意，主要分析手段，是使用minio的js-sdk看他提交http时给的参数是什么，进而估计出C++应该怎么写
- minio的js-sdk用的签名方式不同（是AWS4-HMAC-SHA256），如果能完全模拟他，就更好了  
1. https://github.com/minio/minio/issues/8136
2. https://github.com/kneufeld/minio-put
3. https://docs.min.io/docs/javascript-client-quickstart-guide.html
4. https://github.com/minio/minio-js/blob/master/src/main/minio.js