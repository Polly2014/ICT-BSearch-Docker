# -*- coding:utf-8 -*-
import requests

host_url = 'http://159.226.40.82:33380/api/search/binarySearch'
payload = {
    'keyword': '1010101010101001010',
    'operateType': 0,
    'pageNum': 1,
    'pageSize': 5,
    'order': 0,
    'expandedName': '',
    'binaryType': 0,
    'extention': ''
}
# host_url = 'http://159.226.40.82:33380/api_binarySearchResult/'
# payload = {
#     'search_string': '1010101010101001010',
#     'operateType': 0,
#     'pageNum': 1,
#     'pageSize': 50,
#     'order': 0,
#     'expandedName': '',
#     'scale': 2,
#     'extention': ''
# }


def main():
    r = requests.post(host_url, data=payload)
    print(r.status_code)
    print(r.json())


if __name__ == '__main__':
    main()
