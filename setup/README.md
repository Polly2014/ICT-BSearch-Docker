# Binary Search系统说明

##  文件说明

- Docker-18.03.1-ce.tar：Docker安装包（可执行文件集合）
- Docker-compose-Linux-x86_64：Docker Comopose可执行文件
- bsearch_docker.tar：二进制搜索系统Docker镜像文件
- BSearch_Codes.tar.gz：二进制搜索系统代码文件

## 安装步骤
1. 将全部文件上传并至～/Downloads目录

2. 安装docker与docker-compose

   ```sh
   cd ~/Downloads
   # 解压
   tar -xvf docker-18.03.1-ce.tar
   # 修改权限
   chmod +x docker-18.03.1-ce/*
   # 拷贝至系统路径中
   cp docker-18.03.1-ce/* /usr/local/bin
   # 创建docker service系统服务文件
   cat > /usr/lib/systemd/system/docker.service <<"EOF"
   [Unit]
   Description=Docker Application Container Engine
   Documentation=http://docs.docker.io
   [Service]
   Environment="PATH=/usr/local/bin:/bin:/sbin:/usr/bin:/usr/sbin"
   EnvironmentFile=-/run/flannel/docker
   ExecStart=/usr/local/bin/dockerd --log-level=error $DOCKER_NETWORK_OPTIONS
   ExecReload=/bin/kill -s HUP $MAINPID
   Restart=on-failure
   RestartSec=5
   LimitNOFILE=infinity
   LimitNPROC=infinity
   LimitCORE=infinity
   Delegate=yes
   KillMode=process
   
   [Install]
   WantedBy=multi-user.target
   EOF
   # 重载内核
   systemctl daemon-reload 
   # 启动docker
   systemctl restart docker
   # 设置docker开机启动
   systemctl enable docker
   # 拷贝docker-compose至系统路径中
   cp docker-compose-Linux-x86_64 /usr/local/bin/docker-compose
   # 修改docker-compose权限
   chmod +x /usr/local/bin/docker-compose
   ```

3. 导入docker镜像，二进制搜索系统授权注册
  ```sh
  cd ~/Downloads
  # 导入bsearch_docker镜像到Docker
  docker load < bsearch_docker.tar
  # 新建文件夹，存放二进制搜索系统站点文件（无需注册新用户）
  mkdir /home/BSearch
  # 拷贝Search_Codes.tar.gz到/home/BSearch，并解压
  cp BSearch_Codes.tar.gz /home/BSearch
  cd /home/BSearch
  tar zxvf BSearch_Codes.tar.gz
  # 启动二进制搜索系统
  docker-compose up
  # 此时应会提示无License警告，并给出机器号（警告如下）
  # [WARNING] System Not Registed Or Wrong License, Please Send (<host_number>) to Developer To Get License. Then Update The License To File .setup/LICENSE
  # 将host_number发给计算所相关人员，获得License并写入/home/BSearch/setup/LICENSE文件
  sudo echo '<License>' >> /home/BSearch/setup/LICENSE
  ```
  
4. 启动系统

   ```sh
   cd /home/BSearch
   # 重新启动系统，并设置后台运行
   docker-compose up -d
   # 浏览器访问二进制搜索系统
   curl http://localhost
   ```

5. 测试二进制搜索系统

   ```sh
   # 索引创建测试
   cd /home/BSearch/dataFiles
   mkdir test_1
   # 拷贝任意些带待建索引的文件至/home/BSearch/dataFiles/test_1文件夹中
   cp ~/Downloads/docker-18.03.1-ce.tar /home/BSearch/dataFiles/test_1
   # 打开浏览器，访问"http://localhost/testCluster"页面
   # 选择“节点管理”->“添加索引"，选择"test_1"文件夹，等待系统创建完毕
   #--------------------华丽分割线--------------------
   # 索引搜索测试
   # 打开浏览器，访问"http://localhost"页面
   # 输入任意二进制字符串(大于24位)，如"101010101010101001010101010101010"，点击搜索按钮
   ```