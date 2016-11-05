# GameWebService

# ubuntu部署
## 服务器编译
1. 删除boost与mysql相关库文件
2. 按照顺序安装依赖库
   - libboost-dev
   - libmysqlcppconn-dev
3. 添加链接库
   - -lpthread
   - -llibmysqlcppconn
4. 添加编译选项
   - 使用64位编译（m64）
   - 支持c++11
5. 添加库目录
   - ./ 即当前目录
6. 注意   
不要使用右斜线“\\”包含头文件，在linux下不支持。
