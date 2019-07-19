# ICT-BSearch-Docker
> Binary Search Engine Deployed with Docker Development at ICT duration
## 1.将项目Clone到本地
```sh
cd /home
git clone https://github.com/Polly2014/ICT-BSearch-Docker.git BSearch
```
## 2.目录映射结构
```sh
# Requirements etc.
/home/BSearch/setup/            <-> /home/BSearch/setup/
# Algorithm
/home/BSearch/Algorithm/        <-> /home/BSearch/Algorithm/
# Django
/home/BSearch/Web/              <-> /home/BSearch/Web/
# Mysql
/home/BSearch/Mysql/conf/my.cnf <-> /etc/mysql/my.conf
/home/BSearch/Mysql/data/       <-> /var/lib/mysql/
# Redis
/home/BSearch/Redis/            <-> /usr/local/redis/
# Data
/home/BSearch/indexFiles        <-> /indexFiles/
/home/BSearch/dataFiles         <-> /dataFiles/
```
## 3.端口映射
```sh
Web:   80   <-> 80
Mysql: 3306 <-> 3306
```
## 4.启动代码
```sh
# 拉取镜像
docker pull wangbaoli/bsearch_docker:v2.0
docker run \
  --name bsearch_docker \
  -v /home/BSearch/setup/:/home/BSearch/setup/ \
  -v /home/BSearch/Algorithm/:/home/BSearch/Algorithm/ \
  -v /home/BSearch/Web/:/home/BSearch/Web/ \
  -v /home/BSearch/Mysql/conf/my.cnf:/etc/mysql/my.cnf \
  -v /home/BSearch/Mysql/data/:/var/lib/mysql/ \
  -v /home/BSearch/indexFiles/:/indexFiles/ \
  -v /home/BSearch/dataFiles/:/dataFiles/ \
  -v /home/BSearch/configFiles/:/configFiles/ \
  -p 80:80 \
  -p 3306:3306 \
  -it wangbaoli/bsearch_docker:v2.0 \
  /bin/bash
```
