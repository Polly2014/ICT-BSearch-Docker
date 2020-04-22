# -*- coding: utf-8 -*-

from Crypto.Cipher import AES
import base64
import os
import sys
PWD = '/home/BSearch'


class License(object):
    def __init__(self, key='wangbaoli@ict.ac.cn'):
        self.flag = sys.version_info[0]
        if len(key) > 32:
            key = key[:32]
        self.key = self.to_16(key)

    def to_16(self, key):
        key = bytes(key, encoding="utf8") if self.flag > 2 else key
        while len(key) % 16 != 0:
            key += b'\0'
        return key  # 返回bytes

    def aes(self):
        return AES.new(self.key, AES.MODE_ECB)  # 初始化加密器

    def make_license(self, text):
        aes = self.aes()
        if self.flag == 2:
            return base64.b64encode(aes.encrypt(self.to_16(text)))
        else:

            return str(base64.encodebytes(aes.encrypt(self.to_16(text))),
                       encoding='utf8').replace('\n', '')  # 加密

    def valid_license(self, text):
        aes = self.aes()
        if self.flag == 2:
            return aes.decrypt(base64.b64decode(text))
        else:
            return str(aes.decrypt(base64.decodebytes(bytes(
                text, encoding='utf-8'))).rstrip(b'\0').decode("utf-8"))  # 解密


def get_host_uuid():
    host_sn = ''
    try:
        p = os.popen('blkid /dev/sda1')
        host_sn = list(
            filter(lambda x: x.startswith('UUID'), p.read().split()))
        if len(host_sn) > 0:
            host_sn = host_sn[0].split('=')[1].strip('"')
    except Exception as e:
        print(Exception, e)
    return host_sn


def check_license():
    result = {'code': 0, 'message': 'Already Registered!'}
    PWD_LICENSE = '{}/setup/LICENSE'.format(PWD)
    if os.path.exists(PWD_LICENSE):
        with open(PWD_LICENSE, 'r') as f:
            license_local = f.read().strip()
        if not license_local:
            result['code'] = 2
            result['message'] = 'Null License'
        else:
            license = License()
            license_real = license.make_license(get_host_uuid())
            if not license_real == license_local:
                result['code'] = 1
                result['message'] = 'Wrong License'
    else:
        f = open(PWD_LICENSE, 'w+')
        f.close()
        result['code'] = 2
        result['message'] = 'No License File, Create one and init!'
        check_license()
    return result


def buid_environment():
    flag = False if os.path.exists(
        '{}/Algorithm/libdivsufsort/build/examples'.format(PWD)) else True
    if flag:
        print('[INFO] Init The System')
        # Algorithm
        PWD_ALGORITHM = '{}/Algorithm/libdivsufsort/build'.format(PWD)
        if not os.path.exists(PWD_ALGORITHM):
            os.makedirs(PWD_ALGORITHM)
        os.chdir(PWD_ALGORITHM)
        os.system(
            'cmake -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_PREFIX="/usr/local" ..')
        os.system('make')
        os.system('make install')
        # MySQL
        os.system('chown -R mysql:mysql /var/lib/mysql')
        os.system('rm -f {}/Algorithm/libdivsufsort/examples'.format(PWD))
    print('[INFO] Restart The System')
    # MySQL
    os.system('service mysql restart')
    # Redis
    os.system('redis-server &')
    # # Django
    PWD_DJANGO = '{}/Web/SearchEngine'.format(PWD)
    os.chdir(PWD_DJANGO)
    os.system('python manage.py runserver 0.0.0.0:80 --insecure')


if __name__ == '__main__':
    host_uuid = get_host_uuid()
    if not host_uuid:
        print('[WARNING] Operation System Not Support, Please Connect Developer For Help.(26716201@qq.com)')
        sys.exit(1)
    result = check_license()
    if result['code'] == 0:
        print('[INFO] License Checked Success')
        buid_environment()
    elif result['code'] == 1:
        print('[WARNING] Wrong License, Please Send ({}) to Developer To Get License. Then Update The License To File .setup/LICENSE'.format(
            host_uuid))
    else:
        print('[WARNING] System Not Registed Or Wrong License, Please Send ({}) to Developer To Get License. Then Update The License To File .setup/LICENSE'.format(
            host_uuid))
