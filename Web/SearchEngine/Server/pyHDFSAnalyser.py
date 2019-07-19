#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Date    : 2018-01-15 22:00:35
# @Author  : Polly
# @Link    : wangbaoli@ict.ac.cn
# @Version : Beta 1.0
import os
from hdfs.client import Client


class HDFSAnalyser(object):
	"""docstring for HDFSAnalysis"""
	def __init__(self):
		self.client = None
		self.SEP = os.sep

	def connect(self, host, port):
		conn_url = "http://{}:{}".format(host, port)
		try:
			self.client = Client(conn_url)
			return True
		except Exception, e:
			print "Connect Failed:{}-{}".format(Exception, e)
			return False

	def directoryAnalysis(self, directory='/user/root'):
		file_list = []
		for root, dirs, files in self.client.walk(directory):
			for f in files:
				fileInfo = {}
				fileInfo['absolute'] = os.path.join(root, f)
				fileInfo['dir_info'] = directory
				fileInfo['file_path'] = root
				fileInfo['file_name'] = f
				fileInfo['file_flag'] = 0
				fileInfo['file_size'] = self.client.status(fileInfo['absolute']).get('length', 0)
				file_list.append(fileInfo)
		return file_list

	def directoryBrowse(self, base_dir='/user/root'):
		self.client.root = base_dir
		result = {"dir_current":base_dir, "dir_list":[]}
		if base_dir!="/":
			pre_link = self.SEP.join(base_dir.split(self.SEP)[:-1])
			pre_link = "/" if pre_link=="" else pre_link
			result["dir_list"].append({"name":"..", "link":pre_link})
		result["dir_list"].extend({"name":d, "link":self.client.resolve(d)} for d in self.client.list(base_dir) if self.client.status(d)['type']=="DIRECTORY")
		return result



# if __name__=="__main__":
# 	a = HDFSAnalyser()
# 	if a.connect("10.5.0.110", 50070):
# 		print a.directoryAnalysis()
# 		print a.directoryBrowse()

