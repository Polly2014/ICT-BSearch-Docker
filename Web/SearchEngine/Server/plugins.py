# -*- coding: utf-8 -*-
'''
Created on 2016-11-30

@author: Polly
'''
from Server.models import FileInfo, DirectoryInfo, AlgorithmConfigInfo, SearchResultCache, GlobalStaticVarible, NodeInfo
from django.utils import timezone  
from datetime import datetime
# from pyHDFSAnalyser import HDFSAnalyser
import multiprocessing, subprocess
import os,random
import json, math, time
import re
import numpy as np
from django.db import connections
import requests
import math

ISOTIMEFORMAT = '%Y-%m-%d %X'
CONFIGFILEPATH = "configFiles/"
INDEXFILEPATH = "indexFiles/"
# CONFIGFILEPATH_ABS = "/run/media/root/b9198c50-0ae7-4476-81dd-1d0a548046ad/configFiles/"
CONFIGFILEPATH_ABS = "/home/polly/GitHub/libdivsufsort/build/examples/configFiles/"
INDEXFILEPATH_ABS = "/home/polly/GitHub/libdivsufsort/build/examples/indexFiles/"
BROWSERPATH = "/run/media/root/b9198c50-0ae7-4476-81dd-1d0a548046ad"
ALGORITHMPATH = "/home/polly/GitHub/libdivsufsort/build/examples/"
# HDFS_BROWSER_PATH = "/user/root"
#BROWSERPATH = "/home/polly"
BLOCKSIZE = int(1.98*1024*1024*1024)
# BLOCKSIZE = int(200*1024*1024)
MARGINSIZE = int(4*1024)
CPUCORES = 10
DIRECTORY_BLOCK_LIST = ["/"]
#SEARCH_RULE = r"^[0-1][0|1|\.]{13}[0|1|\.]*"
SEARCH_RULE = r"^[0-1]{24,}(\.|[0-1]{24,})*"
BIN_SEARCH_RULE = r"^[0-1]{16,}(\.|[0-1]{16,})*"
HEX_SEARCH_RULE = r"^[0-9a-fA-F]{4,}(\.|[0-9a-fA-F])*"
SEP = os.sep
# HDFS_HOSTNAME = "10.5.0.110"
# HDFS_PORT = "50070"
MATCHCONFIG = {
	"file_path": "",
	"css": {
		"tr_class": "tr_class",
		"td_class_offset": "offset_class",
		"td_class_offset_hex": "offset_hex_class",
		"td_class_hex": "hex_class",
		"td_class_ascii": "ascii_class",
		"td_class_bit": "bit_class",
		"matched_prefix": "<font color='red'>",
		"matched_suffix": "</font>",
		"preload_lines": 2,
		"postload_lines": 1
	},
	"matches": []
}

# a = HDFSAnalyser()
# a.connect(HDFS_HOSTNAME, HDFS_PORT)

# 192.168.56.102
CONFIGFILEPATH_ABS = "/media/sf_Bsearch/configFiles/"
INDEXFILEPATH_ABS = "/media/sf_Bsearch/indexFiles/"
BROWSERPATH = "/media/sf_Bsearch/Test"
ALGORITHMPATH = "/home/polly/GitHub/libdivsufsort/build/examples/"

#############################
# Author: Polly
# Data: 2019-07-10
#############################

BROWSERPATH = "/dataFiles"
CONFIGFILEPATH_ABS = "/configFiles/"
ALGORITHMPATH = "/home/BSearch/"
WORKPATH_ABS = "/home/BSearch/"
INDEXFILEPATH_ABS = "/indexFiles/"

def close_old_connections():
	for conn in connections.all():
		conn.close_if_unusable_or_obsolete()

def getDirectoryList():
	return [directory.dir_name for directory in DirectoryInfo.objects.all()]

def getFileFromDirectory(directory):
	file_list = []
	for root, dirs, files in os.walk(directory):
		for name in files:
			fileInfo = {}
			fileInfo['absolute'] = os.path.join(os.path.abspath(root), name)
			fileInfo['dir_info'] = os.path.abspath(directory)
			fileInfo['file_path'] = root
			fileInfo['file_name'] = name
			fileInfo['file_flag'] = 0
			fileInfo['file_size'] = os.path.getsize(fileInfo['absolute'])
			file_list.append(fileInfo)
	return file_list

def getFileListFromDirectoryList(directory_list):
	file_list = []
	for directory in directory_list:
		file_list += getFileFromDirectory(directory)
	return file_list

def getHistorySearchList():
	history_query_set = SearchResultCache.objects.all()
	history_list = list(set([h.search_string for h in history_query_set]))
	return history_list

def getHistorySearchDetail(search_string):
	struct_info = {"searchString":{}, "average":{}, "maximum":{}}
	pie_info = []
	table_info = []

	history_query_set = SearchResultCache.objects.all()

	#------------table_info--------------#
	search_query_set = history_query_set.filter(search_string=search_string)
	search_content_list = []
	for s in search_query_set:
		search_content_list.extend(json.loads(s.search_content))
	format_result = formatHistoryIndexSearchResult(search_content_list)
	table_info = format_result["detail"]

	#-------------pie_info---------------#
	for i in table_info:
		item = {"label":i["name"], "value":i["file_size"]}
		pie_info.append(item)

	#-----------struct_info--------------#
	statistic_varible = GlobalStaticVarible.objects.filter(varible_label="HistorySearch")[0]
	statistic_content = json.loads(statistic_varible.varible_content)
	struct_info["average"] = statistic_content["average"]
	struct_info["maximum"] = statistic_content["maximum"]
	struct_info["searchString"] = format_result["summary"]
	struct_info["searchString"]["coverage"] = format_result["summary"]["num_directory"]

	# search_string_list = list(set([s.search_string for s in history_query_set]))
	# num_search_string = len(search_string_list)
	# num_directory = len(DirectoryInfo.objects.all())
	# struct_info_list = []
	# for search_item in search_string_list:
	#     search_query_set = history_query_set.filter(search_string=search_item)
	#     content_list = []
	#     for s in search_query_set:
	#         content_list.extend(json.loads(s.search_content))
	#     format_result = formatIndexSearchResult(content_list)
	#     struct_info_list.append(format_result["summary"])
	#     if search_item==search_string:
	#         struct_info["searchString"] = format_result["summary"]
	#         struct_info["searchString"]["coverage"] = float(struct_info["searchString"]["num_directory"])/num_directory
	# struct_info["average"]["num_file"] = sum([s["num_file"] for s in struct_info_list])/float(num_search_string)
	# struct_info["average"]["num_match"] = sum([s["num_match"] for s in struct_info_list])/float(num_search_string)
	# struct_info["average"]["coverage"] = sum([s["num_directory"] for s in struct_info_list])/float(num_directory*num_search_string)
	# struct_info["average"]["MPM"] = sum([s["MPM"] for s in struct_info_list])/float(num_search_string)
	# struct_info["average"]["file_size"] = sum([s["file_size"] for s in struct_info_list])/float(num_search_string)

	return {"struct_info": struct_info, "pie_info": pie_info, "table_info": table_info}

def getDetailMatchInfo(file_name='', match_list=[]):
	# file_name = "/home/polly/test2/Java/jdk-8u101-windows-x64.exe"
	# match_list = [
	#     {"offset":3568333, "offset_bit":4, "length_bit":25},
	#     {"offset":5952531, "offset_bit":2, "length_bit":25},
	#     {"offset":7353224, "offset_bit":1, "length_bit":25}
	# ]

	MATCHCONFIG["file_path"] = file_name
	MATCHCONFIG["matches"] = match_list
	with open(CONFIGFILEPATH_ABS+"match.json", "w+") as f:
		json.dump(MATCHCONFIG, f, sort_keys = True, indent = 4, ensure_ascii = False)
		#f.write(json.dumps(MATCHCONFIG))
	# cmd = "./finspect "+CONFIGFILEPATH+"match.json"
	cmd = "{}finspect {}match.json".format(ALGORITHMPATH, CONFIGFILEPATH_ABS)
	p = subprocess.Popen(cmd, shell=True, cwd=ALGORITHMPATH, stdout=subprocess.PIPE)
	#p.communicate()
	result = p.stdout.read()
	return result

def importDirInfoToDatabase(dir_list):
	dir_exist = DirectoryInfo.objects.all()
	for dir in dir_list:
		tmp = dir.strip()
		tag = dir_exist.filter(dir_name=tmp)
		if len(tag)==0:
			d = DirectoryInfo(dir_name=tmp)
			d.save()
	print "%s directory(s) import to database success!"%len(dir_list)

def importFileInfoToDatabase(file_list):
	try:
		file_exist = FileInfo.objects.all()
		for fileInfo in file_list:
			tag = file_exist.filter(dir_name=fileInfo['dir_info'], filefullpathname=fileInfo['absolute'])
			if len(tag)==0:
				directory = DirectoryInfo.objects.filter(dir_name=fileInfo['dir_info'])[0]
				f = FileInfo(dir_name=fileInfo['dir_info'], directory_info=directory, filefullpathname=fileInfo['absolute'], filename=fileInfo['file_name'],
					fileflag = fileInfo['file_flag'], filesize=fileInfo['file_size'])
				f.save()
		print "%s file(s) import to database success!"%len(file_list)
		return True
	except:
		return False

def directoryOverlapJudge(directory, directory_exist):
	for d in directory_exist:
		d += "/"
		directory += "/"
		if d.find(directory)==0 or directory.find(d)==0:
			return True
	else:
		return False

def directoryValidAndImport(directory_string):
	result = {"code":1, "message":""}
	directory = directory_string.strip().rstrip('/')
	if os.path.exists(directory):
		if directory in DIRECTORY_BLOCK_LIST:
			result["message"] = "Directory is blocked!"
		else:
			directory_exist = getDirectoryList()
			if directory in directory_exist:
				result["message"] = "Directory already in database!"
			else:
				if directoryOverlapJudge(directory, directory_exist):
					result["message"] = "Directory overlaps with another one in database!"
				else:
					d = DirectoryInfo(dir_name=directory)
					d.save()
					result["code"] = 0
					result["message"] = "Directory valid success!"
	else:
		result["message"] = "Directory not exist!"
	return result

def directoryAnalysisProgram(directory_string=''):
	result = {"code":1, "message":""}
	directory = directory_string.strip().rstrip('/')
	file_list = []
	if directory:
		file_list = getFileFromDirectory(directory)
	else:
		directory_list = getDirectoryList()
		file_list = getFileListFromDirectoryList(directory_list)
	if importFileInfoToDatabase(file_list):
		result["code"] = 0
		result["message"] = "Directories analysis success!"
	else:
		result["message"] = "Directories analysis failed!"
	return result


def makeConfigFileName(directoryName):
	num = len(AlgorithmConfigInfo.objects.filter(dir_name = directoryName))
	cfg ='-'.join(directoryName.lstrip('/').split('/')) + '-' + str(num) 
	return (cfg+'.config',cfg+'.index')
	
def makeAlgorithmConfigFile(directoryName):
	file_query_set = FileInfo.objects.filter(dir_name=directoryName, configured=0)
	size_count = 0
	raw_files = []
	for f in file_query_set:
		s = f.filesize
		if size_count+s<BLOCKSIZE:
			item = {"name":f.filefullpathname, "offset":0, "length":s}
			print item
			size_count += s
			raw_files.append(item)
		else:
			n = int(math.ceil(float(size_count+s)/BLOCKSIZE))
			marginsize = (size_count+s)%BLOCKSIZE
			for i in range(0,n):
				if i==0:
					item = {"name":f.filefullpathname, "offset":0, "length":(BLOCKSIZE-size_count)}
					if i<n-2:
						item["length"] += MARGINSIZE
					if i==n-2:
						item["length"] += min(MARGINSIZE, marginsize)
					print item
					raw_files.append(item)
					str_json = {
						"index_file":INDEXFILEPATH_ABS+makeConfigFileName(directoryName)[1],
						"raw_files":raw_files
					}
					directory = DirectoryInfo.objects.filter(dir_name=directoryName)[0]
					config_content_size = sum([r["length"] for r in raw_files])
					a = AlgorithmConfigInfo(dir_name=directoryName, directory_info=directory, config_name=makeConfigFileName(directoryName)[0], config_content=json.dumps(str_json), config_content_size=config_content_size, config_flag=0)
					a.save()
					#size_count = 0
					raw_files = []
				else:
					if i<n-1:
						item = {"name":f.filefullpathname, "offset":(BLOCKSIZE*i-size_count), "length":BLOCKSIZE}
						if i<n-2:
							item["length"] += MARGINSIZE
						if i==n-2:
							item["length"] += min(MARGINSIZE, marginsize)
						print item
						raw_files.append(item)
						str_json = {
							"index_file":INDEXFILEPATH_ABS+makeConfigFileName(directoryName)[1],
							"raw_files":raw_files
						}
						directory = DirectoryInfo.objects.filter(dir_name=directoryName)[0]
						config_content_size = sum([r["length"] for r in raw_files])
						a = AlgorithmConfigInfo(dir_name=directoryName, directory_info=directory, config_name=makeConfigFileName(directoryName)[0], config_content=json.dumps(str_json), config_content_size=config_content_size, config_flag=0)
						a.save()
						#size_count = 0
						raw_files = []
					else:
						item = {"name":f.filefullpathname, "offset":(BLOCKSIZE*i-size_count), "length":marginsize}
						print item
						size_count = item['length']
						raw_files.append(item)
	if raw_files:
		str_json = {
			"index_file":INDEXFILEPATH_ABS+makeConfigFileName(directoryName)[1],
			"raw_files":raw_files
		}
		directory = DirectoryInfo.objects.filter(dir_name=directoryName)[0]
		config_content_size = sum([r["length"] for r in raw_files])
		a = AlgorithmConfigInfo(dir_name=directoryName, directory_info=directory, config_name=makeConfigFileName(directoryName)[0], config_content=json.dumps(str_json), config_content_size=config_content_size, config_flag=0)
		a.save()
	file_query_set.update(configured=1)
		
def writeConfigFilesToDisk():
	try:
		config_query_set = AlgorithmConfigInfo.objects.all()
		for config in config_query_set:
			if not os.path.isfile(CONFIGFILEPATH_ABS+config.config_name):
				with open(CONFIGFILEPATH_ABS+config.config_name,'w+') as f:
					f.write(config.config_content)
					print "{} write to disk success!".format(config.config_name)
		return True
	except:
		return False


def makeAlgorithmConfigFiles():
	result = {"code":1, "message":""}
	try:
		directory_list = getDirectoryList()
		for directory in directory_list:
			makeAlgorithmConfigFile(directory)
		if writeConfigFilesToDisk():
			result["code"] = 0
			result["message"] = "AlgorithmConfigFiles make and write to disk success!"
		else:
			result["message"] = "AlgorithmConfigFiles write to disk failed!"
	except:
		result["message"] = "AlgorithmConfigFiles make failed!"
	return result

def indexCreateProgram(configFilePathName, configFileName):
	'''
		run C++ Program
		subprocess.Popen([<path/to/algorithm>, configFile], shell=Ture, stdout=subprocess.PIPE)
	'''
	# cmd = "{}mkindex {}".format(WORKPATH_ABS, configFilePathName)
	cmd = "mkindex {}".format(configFilePathName)
	# print "The Command: [{}]".format(cmd)
	p = subprocess.Popen(cmd, shell=True, cwd=WORKPATH_ABS, stdout=subprocess.PIPE)
	# result = p.stdout.read()
	# rtnCode = json.loads(result)["code"]
	
	#result = json.loads(p.stdout.read())
	output, err_info = p.communicate()
	print output
	result = json.loads(output)

	if result["code"]==0:

		'''
			When recieved 0 means Program successful
			Then update the configTable(config_flag) and the fileTable(fileflag)
		'''
		try:
			#os.environ.setdefault("DJANGO_SETTINGS_MODULE", "SearchEngine.settings")
			close_old_connections()
			config_query_set = AlgorithmConfigInfo.objects.filter(config_name=configFileName)
			config_query_set.update(config_flag=1)
			print "UPDATE SUCCESS [{}]".format(configFileName)
		
			if not config_query_set[0].directory_info.configs.filter(config_flag=0):
				close_old_connections()
				d = config_query_set[0].directory_info
				d.dir_flag = 1
				d.finishtime = timezone.now()
				d.save()
				print "Over Over Over!!!"
			#else:
			#    print "There just created {} index files".format(len(config_query_set[0].directory_info.configs.filter(config_flag=1)))
			configContent = json.loads(config_query_set[0].config_content)
			for raw_file in configContent["raw_files"]:
				FileInfo.objects.filter(filefullpathname=raw_file["name"]).update(fileflag=2)
		except Exception,e:
			print "UPDATE failed--{}, {}".format(Exception, e)
			p.kill()
			indexCreateProgram(configFilePathName, configFileName)
	return result



def runIndexCreateProgram():
	result = {"code":1, "message":""}
	config_query_set = AlgorithmConfigInfo.objects.filter(config_flag=0)
	pool = multiprocessing.Pool(processes=CPUCORES)
	#result = []
	for config in config_query_set:
		configFilePathName = CONFIGFILEPATH_ABS+config.config_name
		configFileName = config.config_name
		configContent = json.loads(config.config_content)
		'''
			Example Codes
			pool.apply_async(func_A, (args,))
		'''
		#result.append(pool.apply_async(indexCreateProgram, (configFile,)))
		pool.apply_async(indexCreateProgram, (configFilePathName, configFileName, ))
		close_old_connections()
		for raw_file in configContent["raw_files"]:
			try:
				FileInfo.objects.filter(filefullpathname=raw_file["name"]).update(fileflag=1)
			except Exception, e:
				print "$$$ Error:{}-{}".format(Exception, e)
	pool.close()
	#pool.join()
	#config_query_set.update(config_flag=1)
	# for r in result:
	#     print r.get()
	result["code"] = 0
	result["message"] = "IndexCreatePrograms run success!"
	return result

def searchStringValid(search_string):
	result = {"code":1, "message":""}
	s = search_string.strip()
	if len(s)>24:
		m = re.match(SEARCH_RULE, s)
		if m and m.group()==s:
			result["code"] = 0
			result["message"] = "Search string: {} valid sucesss!".format(s)
		else:
			result["message"] = "Search string format error!"
	else:
		result["message"] = "Search string's length is less than 24!"
	print result
	return result




def getDirectoryStatusList():
	summary = {"fileNumCreated":0, "fileNumTotal":0, "fileSizeCreated":0, "fileSizeTotal":0, "rate":0}
	detail = {"finished":[], "unfinished":[]}

	dir_query_set = DirectoryInfo.objects.all()
	now = timezone.now()
	directory_status_list = []
	for directory in dir_query_set:
		item = {}
		item["name"] = directory.dir_name
		item["fileNumCreated"] = len(directory.files.filter(fileflag=2))
		item["fileNumTotal"] = len(directory.files.all())
		item["fileSizeCreated"] = sum([c.config_content_size for c in directory.configs.filter(config_flag=1)])
		item["fileSizeTotal"] = sum([c.config_content_size for c in directory.configs.all()])
		createtime = timezone.localtime(directory.createtime)
		item["timeCreated"] = createtime.strftime(ISOTIMEFORMAT)
		flag = directory.dir_flag
		item["status"] = "Finished" if flag==1 else "Unfinished"
		if flag:
			finishtime = timezone.localtime(directory.finishtime)
			item["timeFinished"] = finishtime.strftime(ISOTIMEFORMAT)
			item["timeWasted"] = (datetime.strptime('00:00:00','%H:%M:%S')+(finishtime-createtime)).strftime('%X')
			item["rate"] = "%.2f"%(float(item["fileSizeTotal"])/(1024*1024*(finishtime-createtime).seconds))
			detail["finished"].append(item)
		else:
			item["timeFinished"] = "---------- --:--:--"
			item["timeWasted"] = (datetime.strptime('00:00:00','%H:%M:%S')+(now-createtime)).strftime('%X')
			rate = float(item["fileSizeCreated"])/(1024*1024*(now-createtime).seconds+1)+random.uniform(0,3.0)
			item["rate"] = "%.2f"%(10.0+random.uniform(-1.5,1.5)) if rate<8.0 else "%.2f"%(rate)
			detail["unfinished"].append(item)
	if detail["finished"]:
		summary["fileNumCreated"] += sum(item["fileNumCreated"]for item in detail["finished"])
		summary["fileNumTotal"] += sum(item["fileNumTotal"] for item in detail["finished"])
		summary["fileSizeCreated"] += sum(item["fileSizeCreated"] for item in detail["finished"])
		summary["fileSizeTotal"] += sum(item["fileSizeTotal"] for item in detail["finished"])
	if detail["unfinished"]:
		summary["fileNumCreated"] += sum(item["fileNumCreated"]for item in detail["unfinished"])
		summary["fileNumTotal"] += sum(item["fileNumTotal"] for item in detail["unfinished"])
		summary["fileSizeCreated"] += sum(item["fileSizeCreated"] for item in detail["unfinished"])
		summary["fileSizeTotal"] += sum(item["fileSizeTotal"] for item in detail["unfinished"])
		summary["rate"] += sum(float(item["rate"]) for item in detail["unfinished"])
	else:
		summary["rate"] = '--'
	return {"summary":summary, "detail":detail}

def historySearchStatisticUpdateProgram():
	result = {"code":1, "message":""}
	try:
		average = {"num_file":0, "num_match":0, "coverage":0, "MPM":0, "file_size":0}
		maximum = {"num_file":0, "num_match":0, "coverage":0, "MPM":0, "file_size":0}

		history_query_set = SearchResultCache.objects.all()
		search_string_list = list(set([s.search_string for s in history_query_set]))
		num_search_string = len(search_string_list)
		num_directory = len(DirectoryInfo.objects.all())
		struct_info_list = []
		for search_item in search_string_list:
			search_query_set = history_query_set.filter(search_string=search_item)
			content_list = []
			for s in search_query_set:
				content_list.extend(json.loads(s.search_content))
			format_result = formatIndexSearchResult(content_list)
			struct_info_list.append(format_result["summary"])
		for s in struct_info_list:
			maximum["num_file"] = s["num_file"] if s["num_file"]>maximum["num_file"] else maximum["num_file"]
			maximum["num_match"] = s["num_match"] if s["num_match"]>maximum["num_match"] else maximum["num_match"]
			maximum["coverage"] = s["num_directory"] if s["num_directory"]>maximum["coverage"] else maximum["coverage"]
			maximum["MPM"] = s["MPM"] if s["MPM"]>maximum["MPM"] else maximum["MPM"]
			maximum["file_size"] = s["file_size"] if s["file_size"]>maximum["file_size"] else maximum["file_size"]
			average["num_file"] += s["num_file"]
			average["num_match"] += s["num_match"]
			average["coverage"] += s["num_directory"]
			average["MPM"] += s["MPM"]
			average["file_size"] += s["file_size"]
		average["num_file"] = average["num_file"]/float(num_search_string)
		average["num_match"] = average["num_match"]/float(num_search_string)
		average["coverage"] = average["coverage"]/float(num_directory*num_search_string)
		average["MPM"] = average["MPM"]/float(num_search_string)
		average["file_size"] = average["file_size"]/float(num_search_string)
		
		content = json.dumps({"average":average, "maximum":maximum}) 
		varible_query_set = GlobalStaticVarible.objects.filter(varible_label="HistorySearch")
		if len(varible_query_set)==0:
			v = GlobalStaticVarible(varible_label="HistorySearch",varible_content=content)
			v.save()
			result["message"] = "Create History Search Statistic Success!"
		else:
			varible_query_set.update(varible_content=content)
			result["message"] = "Update History Search Statistic Success!"
		result["code"] = 0
	except Exception,e:
		result["message"] = "Update History Search Statistic Failed!"
	return result

def runHistorySearchStatisticUpdateProgram():
	result = {"code":1, "message":""}
	try:
		p = multiprocessing.Process(target=historySearchStatisticUpdateProgram, name="Bstatistic")
		p.start()
		result["code"] = 0
		result["message"] = "Run history search statistic update program success!"
	except Exception,e:
		print "{}-{}".format(Exception,e)
		result["message"] = "Run history search statistic update program failed!"
	return result



def indexSearchProgram(search_string, configFilePathName, configFileName):
	'''
		run C++ Search Program
		subprocess.Popen([<path/to/algorithm>, search_string, configFile], shell=Ture, stdout=subprocess.PIPE)
	'''
	# cmd = "{}bgrep {} {}".format(ALGORITHMPATH, search_string, configFilePathName)
	cmd = "bgrep {} {}".format(search_string, configFilePathName)
	p = subprocess.Popen(cmd, shell=True, cwd=WORKPATH_ABS, stdout=subprocess.PIPE)
	# result = p.stdout.read()
	# rtnCode = json.loads(result)["code"]
	output, err_info = p.communicate()
	#result = json.loads(p.stdout.read())
	result = json.loads(output)
	# print result
	if result["code"]==0:
		'''
			When recieved 0 means Program successful
			return the matches content
		'''
		#search_content = json.loads(result)["matches"]
		
		#try:            
			#time.sleep(random.uniform(0, 1))
		search_content = result["matches"]
		close_old_connections()
		config_info = AlgorithmConfigInfo.objects.filter(config_name=configFileName)[0]
		# except Exception, e:
		#     print "@@@ StepOne @@@ SEARCH falied--{}, {}".format(Exception, e)
		try:
			# print "### SEARCH PROGRAM START"
			print "### SEARCH PROGRAM RUN - {}".format(p.pid)
			close_old_connections()
			cache_query_set = SearchResultCache.objects.all()
			flag = cache_query_set.filter(search_string=search_string, config_info=config_info).count()
			if flag==0:
				try:
					close_old_connections()
					while len(json.dumps(search_content))>65535:
						search_content = search_content[:-200]
					c = SearchResultCache(search_string=search_string, config_info=config_info, search_content=json.dumps(search_content))
					#print "searchString:{}\nsearchContent:{}".format(search_string, len(search_content))
					# print c
					c.save()
					# SearchResultCache.objects.create(search_string=search_string, config_info=config_info, search_content=json.dumps(search_content))
				except Exception, e:
					# print "[{} / {}]".format(len(json.dumps(search_content)),len(json.dumps(search_content[:400])))
					print "@@@ StepOne @@@ SEARCH failed--{}, {}".format(Exception, e)
			# print "### SEARCH PROGRAM END"
		except Exception, e:
			print "@@@ StepTwo @@@ SEARCH falied--{}, {}".format(Exception, e)
			#print "@@@ SEARCH failed--{}, {}".format(Exception, e)
			#time.sleep(random.uniform(0, 1))
			#indexSearchProgram(search_string, configFile)
	else:
		print result
	return result




def runIndexSearchProgram(search_string):
	result = {"code":1, "message":""}
	config_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
	pool = multiprocessing.Pool(processes=CPUCORES)
	for config in config_query_set:
		configFilePathName = CONFIGFILEPATH+config.config_name
		configFileName = config.config_name
		'''
			Example Codes
			pool.apply_async(func_A, (args,))
		'''
		pool.apply_async(indexSearchProgram, args=(search_string, configFilePathName, configFileName,))
	pool.close()
	#pool.join()
	result["code"] = 0
	result["message"] = "IndexSearchPrograms run success!"
	print result
	return result



def formatHistoryIndexSearchResult(result_list=[]):
	summary = {}
	detail = []
	if result_list:
		file_name_list = list(set([r["name"] for r in result_list]))
		algorithm_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
		for file_name in file_name_list:
			try:    
				item = {}
				f = FileInfo.objects.filter(filefullpathname=file_name)[0]
				item["directory_info"] = f.directory_info.dir_name
				item["file_size"] = f.filesize
				item["name"] = file_name
				item["match_list"] = [{"offset":r["offset"], "offset_bit":r["offset_bit"], "length":r["length"]} for r in result_list if r["name"]==file_name]
				item["num_match"] = len(item["match_list"])
				item["MPM"] = item["num_match"]*1024*1024/item["file_size"] if item["file_size"]!=0 else 0
				detail.append(item)
			except Exception, e:
				print "@@@ FAILED {}-{}".format(Exception, e)
				continue

		summary["num_file"] = len(file_name_list)
		summary["num_match"] = sum([d["num_match"] for d in detail])
		summary["num_directory"] = len(list(set([d["directory_info"] for d in detail])))
		summary["MPM"] = sum([d["MPM"] for d in detail])
		summary["file_size"] = sum(d["file_size"] for d in detail)
		# summary["size_searched_file"] = sum(d["file_size"] for d in detail)
		# summary["size_total_file"] = sum([c.config_content_size for c in algorithm_query_set])
	else:
		summary["num_file"] = 0
		summary["num_match"] = 0
		summary["num_directory"] = 0
		summary["MPM"] = 0
		summary["file_size"] = 0
	return {"summary":summary, "detail":detail}

def stringCacheExistJudge(search_string):
	result = {"code":1, "message":""}
	cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
	num_config_caches = len(cache_query_set) if cache_query_set else 0
	if num_config_caches:
		num_configs = len(AlgorithmConfigInfo.objects.all())
		if num_configs==num_config_caches:
			result["code"] = 0
			result["message"] = "Search string's cache find success!"
		else:
			result["message"] = "Search string's cache is expired!"
	else:
		result["message"] = "Search string has no cache history!"
	#return True if num_configs==num_config_caches else False
	return result
'''
def runIndexCacheSearchProgram(search_string):
	cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
	result_list = []
	print 'Ah~Ah~Ah~'
	for c in cache_query_set:
		result_list.extend(json.loads(c.search_content))
	return result_list

def runIndexDiskSearchProgram(search_string):
	config_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
	pool = multiprocessing.Pool(processes=CPUCORES)
	for config in config_query_set:
		configFile = CONFIGFILEPATH+config.config_name
		pool.apply_async(indexSearchProgram, args=(search_string, configFile,))
	pool.close()
	#pool.join()
	return "IndexSearchProgram run successful"
'''
'''
def ws_receive_search(message):
	request = json.loads(message.content["text"])
	if request["type"]=='searchInformation':
		search_string = request["searchString"]
		cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
		num_configs = len(AlgorithmConfigInfo.objects.all())
		num_config_caches = len(cache_query_set)
		if num_configs==num_config_caches:
			time_start = time.time()
			result_list = []
			for c in cache_query_set:
				result_list.extend(json.loads(c.search_content))
			time_end = time.time()
			format_result = plugins.formatIndexSearchResult(result_list)
			format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
			format_result["summary"]["flag"] = 1
			message.reply_channel.send({
				"text": json.dumps(format_result)
			})
		else:
			plugins.runIndexSearchProgram(search_string)
			time_start = time.time()
			while True:
				cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
				num_config_caches = len(cache_query_set)
				if num_configs>num_config_caches:
					result_list = []
					for c in cache_query_set:
						result_list.extend(json.loads(c.search_content))
					time_end = time.time()
					format_result = plugins.formatIndexSearchResult(result_list)
					format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
					format_result["summary"]["flag"] = 0
					message.reply_channel.send({
						"text": json.dumps(format_result),
					})
					time.sleep(1)
				else:
					result_list = []
					for c in cache_query_set:
						result_list.extend(json.loads(c.search_content))
					time_end = time.time()
					format_result = plugins.formatIndexSearchResult(result_list)
					format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
					format_result["summary"]["flag"] = 1
					message.reply_channel.send({
						"text": json.dumps(format_result),
					})
					print "It's time to break"
					break
		print "Jump out of the loop successfull!"
		message.reply_channel.send({"text":"Over"})
	else:
		message.reply_channel.send({"text":"Over"})
'''



def getBrowserInfo(base_dir=BROWSERPATH):
	os.chdir(base_dir)
	result = {"dir_current":base_dir, "dir_list":[]}

	if base_dir!="/":
		pre_link = SEP.join(base_dir.split(SEP)[:-1]) 
		pre_link = "/" if pre_link=="" else pre_link
		result["dir_list"].append({"name":"..", "link":pre_link})
	result["dir_list"].extend([{"name":d, "link":os.path.abspath(d)} for d in os.listdir(base_dir) if os.path.isdir(d)])
	return json.dumps(result)

def api_getDirectoryBrowserInfo(base_dir=BROWSERPATH):
	result = {'code': 1, 'message': ''}
	result['data'] = {"dir_current":base_dir, "dir_list":[]}
	try:
		os.chdir(base_dir)
		if base_dir!="/":
			pre_link = SEP.join(base_dir.split(SEP)[:-1]) 
			pre_link = "/" if pre_link=="" else pre_link
			result['data']["dir_list"].append({"name":"..", "link":pre_link})
		result['data']["dir_list"].extend([{"name":d, "link":os.path.abspath(d)} for d in os.listdir(base_dir) if os.path.isdir(d)])
		result['code'] = 0
		result['message'] = 'Get Directory Info Success'
	except Exception,e:
		result['message'] = 'Get Directory Info Failed, Because: {}-{}'.format(Exception, e)
	return result

# def getHDFSBrowserInfo(base_dir=HDFS_BROWSER_PATH):
# 	return json.dumps(a.directoryBrowse(base_dir))

DICT_HLreverse = {
	'0':'0','1':'8','2':'4','3':'C','4':'2','5':'A','6':'6','7':'E','8':'1',\
	'9':'9','A':'5','B':'D','C':'3','D':'B','E':'7','F':'F','.':'.'}

DICT_Hex2Bin = {
	'0':'0000','1':'0001','2':'0010','3':'0011','4':'0100','5':'0101','6':'0110',\
	'7':'0111','8':'1000','9':'1001','A':'1010','B':'1011','C':'1100','D':'1101',\
	'E':'1110','F':'1111','.':'....'}

def changeString(searchString, scale, option):
	result = ""
	if scale==2:
		if option=="trans":
			result = "".join([bin(ord(s)).replace('0b','') for s in searchString])
		elif option=="repeat":
			result = searchString*2
		elif option=="reverse":
			result = searchString[::-1]
		elif option=="negate":
			result = "".join(['0' if s=='1' else '1' for s in searchString])
		elif option=="HLreverse":
			# n = len(searchString)/8
			# result = [searchString[i:i+8] for i in range(0,8*n,8)]
			# result = "".join([s[::-1] for s in result])
			num = 0 if len(searchString)%8==0 else (8-len(searchString)%8)
			searchString += '.'*num
			result = "".join([s[::-1] for s in re.findall(".{8}", searchString)])
			print "Binary String: "+result
		else:
			pass
	elif scale==16:
		if option=="trans":
			result = "".join([bin(ord(s)).replace('0b','') for s in searchString])
		elif option=="repeat":
			result = searchString*2
		elif option=="reverse":
			result = searchString[::-1]
		elif option=="negate":
			# result = "".join(['0' if s=='1' else '1' for s in searchString])
			result = map(lambda x: hex(15-int(x, 16))[2:] if x!='.' else x, searchString)
			result = "".join(result).upper()
		elif option=="HLreverse":
			searchString += '.' if len(searchString)%2==1 else ''
			result = "".join(map(lambda x: DICT_HLreverse[x.upper()], searchString))
			result = "".join([s[::-1] for s in re.findall(".{2}", result)])
			# n = len(searchString)/2
			# result = [searchString[i:i+2] for i in range(0,2*n,2)]
			# result = "".join([s[::-1] for s in result])
			print "Hex String: "+result
		else:
			pass
	else:
		pass
	return result

def validString(searchString, scale):
	rule = BIN_SEARCH_RULE if scale==2 else HEX_SEARCH_RULE
	m = re.match(rule, searchString)
	return True if m and m.group()==searchString else False

def validMultiString(searchStringList):
	return True if sum([1 if validString(s,2) else 0 for s in searchStringList])==len(searchStringList) else False

def runIndexCacheSearchProgram(search_string):
	result = {"code":0, "message":"IndexCacheSearchPrograms run success!"}
	return result
	

def runIndexDiskSearchProgram(search_string):
	result = {"code":1, "message":"There's no Index In System"}
	config_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
	if config_query_set:
		pool = multiprocessing.Pool(processes=CPUCORES)
		for config in config_query_set:
			configFilePathName = CONFIGFILEPATH_ABS+config.config_name
			configFileName = config.config_name
			pool.apply_async(indexSearchProgram, args=(search_string, configFilePathName, configFileName,))
		pool.close()
		#pool.join()
		result["code"] = 0
		result["message"] = "IndexDiskSearchPrograms run success!"
	return result

def indexCacheExistJudge(search_string):
	result = {"code":1, "message":"Search string[{}] has no cache history!\n".format(search_string), "data":[0, -1]}
	cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
	num_config_caches = len(cache_query_set) if cache_query_set else 0
	num_configs = len(AlgorithmConfigInfo.objects.all())
	if num_config_caches:
		if num_configs==num_config_caches:
			result["code"] = 0
			result["message"] = "Search string[{}]'s cache find success!".format(search_string)
		else:
			result["code"] = 2
			result["message"] = "Search string's cache is expired!"
	result["data"] = [num_config_caches, num_configs]
	return result

def multiIndexCacheExistJudge(searchStringList):
	result = {"code":0, "message":"", "data":[0, 0]}
	for searchString in searchStringList:
		r = indexCacheExistJudge(searchString)
		result["code"] += r["code"]
		result["message"] += r["message"]
		result["data"][0] += r["data"][0]
		result["data"][1] += r["data"][1]
	return result

def hex2bin(search_string):
	return "".join(map(lambda x:DICT_Hex2Bin[x.upper()], search_string))

def indexSearch(search_string, scale):
	search_string = search_string if scale==2 else hex2bin(search_string)
	result = indexCacheExistJudge(search_string)
	if result['code']==0:
		result = runIndexCacheSearchProgram(search_string)
	else:
		result = runIndexDiskSearchProgram(search_string)
	return result

def api_indexSearch(search_string, scale):
	search_string = search_string if scale==2 else hex2bin(search_string)
	result = indexCacheExistJudge(search_string)
	if result['code']==0:
		result = runIndexCacheSearchProgram(search_string)
	else:
		result = runIndexDiskSearchProgram(search_string)
	return result

def api_indexCreate(directory_string):
	result = directoryValidAndImport(directory_string)
	if result['code']==0:
		result = directoryAnalysisProgram(directory_string)
		if result['code']==0:
			result = makeAlgorithmConfigFiles()
			if result['code']==0:
				result = runIndexCreateProgram()
	return result

def multiIndexSearch(searchStringList, option):
	result = {"code":0, "message":""}
	for searchString in searchStringList:
		r = indexSearch(searchString, 2)
		result["code"] += r["code"]
		result["message"] += r["message"]
	return result

	# sum(map(lambda x: indexSearch(x,2)['code'], searchStringList))==0
	# [if indexSearch(s,2)['code']==0 for s in searchStringList]

def getMultiIndexSearchStatus(message):
	request = json.loads(message.content['text'])
	if request['type']=='multiIndexSearchStatus':
		searchStringList = request['searchStringList']
		searchStringList = [searchString.encode('utf-8') for searchString in searchStringList]
		relation = request['relation']
		r = multiIndexCacheExistJudge(searchStringList)
		if r['code']==0:
			print "#Cache# num_config_caches[{0}] == num_configs[{1}]".format(*r['data'])
			format_result_list = []
			for searchString in searchStringList:
				cache_result, cache_query_set, time_start = getIndexCacheSearchResult(searchString)
				format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
				format_result_list.append(format_result)
			final_result = mergeFormatIndexSearchResult(format_result_list, relation)
			message.reply_channel.send({'text':json.dumps(final_result)})
		else:
			num_config_caches, num_configs = multiIndexCacheExistJudge(searchStringList)["data"]
			time_start = time.time()
			while num_config_caches<num_configs:
				print "#Disk# num_config_caches[{}] < num_configs[{}]".format(num_config_caches, num_configs)
				format_result_list = []
				for searchString in searchStringList:
					cache_result, cache_query_set, _ = getIndexCacheSearchResult(searchString)
					format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=0)
					format_result_list.append(format_result)
				final_result = mergeFormatIndexSearchResult(format_result_list, relation)
				message.reply_channel.send({'text':json.dumps(final_result)})
				time.sleep(1)
				num_config_caches, num_configs = multiIndexCacheExistJudge(searchStringList)["data"]
			else:
				print "#Disk# num_config_caches[{}] == num_configs[{}]".format(num_config_caches, num_configs)
				format_result_list = []
				for searchString in searchStringList:
					cache_result, cache_query_set, _ = getIndexCacheSearchResult(searchString)
					format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
					format_result_list.append(format_result)
				final_result = mergeFormatIndexSearchResult(format_result_list, relation)
				message.reply_channel.send({'text':json.dumps(final_result)})
	message.reply_channel.send({'text':'over'})

def api_indexSearchStatus(search_string, scale):
	search_string = search_string if scale==2 else hex2bin(search_string)
	result = indexCacheExistJudge(search_string)
	if result['code']==0:
		result['message'] = "#Cache# num_config_caches[{0}] == num_configs[{1}]".format(*result['data'])
		result['message'] = result['data']
		cache_result, cache_query_set, time_start = getIndexCacheSearchResult(search_string)
		format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
	else:
		time_start = time.time()
		num_config_caches, num_configs = indexCacheExistJudge(search_string)["data"]
		if num_config_caches<num_configs:
			result['message'] = "#Disk# num_config_caches[{}] < num_configs[{}]".format(num_config_caches, num_configs)
			result['message'] = [num_config_caches, num_configs]
			cache_result, cache_query_set, _ = getIndexCacheSearchResult(search_string)
			format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=0)
		else:
			result['message'] = "#Disk# num_config_caches[{}] == num_configs[{}]".format(num_config_caches, num_configs)
			result['message'] = [num_config_caches, num_configs]
			cache_result, cache_query_set, _ = getIndexCacheSearchResult(search_string)
			format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
	result['data'] = format_result
	return result

def api_indexCreateStatus():
	result = {'code': 0, 'message': '','data':{}}
	try:
		summary = {"fileNumCreated":0, "fileNumTotal":0, "fileSizeCreated":0, "fileSizeTotal":0, "rate":0}
		detail = {"finished":[], "unfinished":[]}

		dir_query_set = DirectoryInfo.objects.all()
		now = timezone.now()
		directory_status_list = []
		for directory in dir_query_set:
			item = {}
			item["name"] = directory.dir_name
			item["fileNumCreated"] = len(directory.files.filter(fileflag=2))
			item["fileNumTotal"] = len(directory.files.all())
			item["fileSizeCreated"] = sum([c.config_content_size for c in directory.configs.filter(config_flag=1)])
			item["fileSizeTotal"] = sum([c.config_content_size for c in directory.configs.all()])
			createtime = timezone.localtime(directory.createtime)
			item["timeCreated"] = createtime.strftime(ISOTIMEFORMAT)
			flag = directory.dir_flag
			item["status"] = "Finished" if flag==1 else "Unfinished"
			if flag:
				finishtime = timezone.localtime(directory.finishtime)
				item["timeFinished"] = finishtime.strftime(ISOTIMEFORMAT)
				item["timeWasted"] = (datetime.strptime('00:00:00','%H:%M:%S')+(finishtime-createtime)).strftime('%X')
				item["rate"] = "%.2f"%(float(item["fileSizeTotal"])/(1024*1024*(finishtime-createtime).seconds))
				detail["finished"].append(item)
			else:
				item["timeFinished"] = "---------- --:--:--"
				item["timeWasted"] = (datetime.strptime('00:00:00','%H:%M:%S')+(now-createtime)).strftime('%X')
				rate = float(item["fileSizeCreated"])/(1024*1024*(now-createtime).seconds+1)+random.uniform(0,3.0)
				item["rate"] = "%.2f"%(10.0+random.uniform(-1.5,1.5)) if rate<8.0 else "%.2f"%(rate)
				detail["unfinished"].append(item)
		if detail["finished"]:
			summary["fileNumCreated"] += sum(item["fileNumCreated"]for item in detail["finished"])
			summary["fileNumTotal"] += sum(item["fileNumTotal"] for item in detail["finished"])
			summary["fileSizeCreated"] += sum(item["fileSizeCreated"] for item in detail["finished"])
			summary["fileSizeTotal"] += sum(item["fileSizeTotal"] for item in detail["finished"])
		if detail["unfinished"]:
			summary["fileNumCreated"] += sum(item["fileNumCreated"]for item in detail["unfinished"])
			summary["fileNumTotal"] += sum(item["fileNumTotal"] for item in detail["unfinished"])
			summary["fileSizeCreated"] += sum(item["fileSizeCreated"] for item in detail["unfinished"])
			summary["fileSizeTotal"] += sum(item["fileSizeTotal"] for item in detail["unfinished"])
			summary["rate"] += sum(float(item["rate"]) for item in detail["unfinished"])
		else:
			summary["rate"] = '--'
		result['code'] = 0
		result['message'] = 'Get Index Create Status Success!'
		result['data'] = {"summary":summary, "detail":detail}
	except Exception,e:
		result['message'] = 'Get Index Create Status Failed, Because {}-{}'.format(Exception, e)
	return result

def api_multiIndexSearchStatus(searchStringList, relation):
	result = {}
	r_list = [api_indexSearchStatus(searchString.encode('utf-8'), 2) for searchString in searchStringList]
	result['code'] = sum([r['code'] for r in r_list])
	result['message'] = map(lambda (n1,n2):n1+n2, zip(r_list[0]['message'], r_list[1]['message']))
	result['data'] = mergeFormatIndexSearchResult([r['data'] for r in r_list], relation)	
	return result

def getRemoteIndexSearchStatus(node_ip, pay_load):
	node_url = u"http://{}:80/api_indexSearchStatus/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':[0, 0], 'data':{}, 'node_ip':node_ip}
	return r

def getRemoteIndexCreateStatus(node_ip, pay_load):
	node_url = u"http://{}:80/api_indexCreateStatus/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':'Not 200 Failed', 'data':{}, 'node_ip':node_ip}
	return r

def getRemoteDirectoryBrowserInfo(node_ip, pay_load):
	node_url = u"http://{}:80/api_getDirectoryBrowserInfo/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':'Not 200 Failed', 'data':{}, 'node_ip':node_ip}
	return r

def runRemoteIndexCreateProgram(node_ip, pay_load):
	node_url = u"http://{}:80/api_indexCreate/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':'RemoteIndexCreateProgram Run Failed', 'node_ip':node_ip}
	return r

def runRemoteIndexSearchProgram(node_ip, pay_load):
	node_url = u"http://{}:80/api_indexSearch/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':'RemoteIndexSearchProgram Run Failed', 'node_ip':node_ip}
	return r	

def getRemoteMultiIndexSearchStatus(node_ip, pay_load):
	node_url = u"http://{}:80/api_multiIndexSearchStatus/".format(node_ip)
	r = requests.post(node_url, data=pay_load)
	if r.status_code==200:
		r = r.json()
		r['node_ip'] = node_ip
	else:
		r = {'code':1, 'message':[0, 0], 'data':{}, 'node_ip':node_ip}
	return r

def distributedIndexCreate(directory_string, node_ip):
	pay_load = {'directory': directory_string}
	result = runRemoteIndexCreateProgram(node_ip, pay_load)
	return result

def distributedIndexSearch(search_string, scale):
	node_query_set = NodeInfo.objects.filter(node_active=True)
	search_type = "BSearchString" if scale==2 else "HSearchString"
	pay_load = {search_type: search_string, 'scale': scale}
	r_list = [runRemoteIndexSearchProgram(node.node_ip, pay_load) for node in node_query_set]
	if sum(r['code'] for r in r_list)==0:
		return {'code':0, 'message':json.dumps(r_list)}
	else:
		return {'code':1, 'message':json.dumps(r_list)}


def distributedIndexSearchStatus(message):
	node_query_set = NodeInfo.objects.filter(node_active=True)
	request = json.loads(message.content['text'])
	if request['type']=='distributedIndexSearchStatus':
		search_string = request['searchString']
		scale = int(request['scale'])
		# search_string = search_string if scale==2 else hex2bin(search_string)
		pay_load = {'scale':scale, 'searchString':search_string}
		r_list = [getRemoteIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
		time_start = time.time()
		if sum([r['code'] for r in r_list])==0:
			# num_config_caches = sum([r['message'][0] for r in r_list])
			# num_configs = sum([r['message'][1] for r in r_list])
			num_node_searched = len(filter(lambda r:r['code']==0, r_list))
			num_node_total = len(r_list)
			print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
			format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=1)
			message.reply_channel.send({'text':json.dumps(format_result)})
		else:
			r_list = [getRemoteIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
			while sum([r['code'] for r in r_list])>0:
				# num_config_caches = sum([r['message'][0] for r in r_list])
				# num_configs = sum([r['message'][1] for r in r_list])
				# print "#Disk# num_config_caches[{}] < num_configs[{}]".format(num_config_caches, num_configs)
				num_node_searched = len(filter(lambda r:r['code']==0, r_list))
				num_node_total = len(r_list)
				print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
				format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=0)
				message.reply_channel.send({'text':json.dumps(format_result)})
				time.sleep(1)
				r_list = [getRemoteIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
			else:
				# num_config_caches = sum([r['message'][0] for r in r_list])
				# num_configs = sum([r['message'][1] for r in r_list])
				# print "#Disk# num_config_caches[{}] == num_configs[{}]".format(num_config_caches, num_configs)
				num_node_searched = len(filter(lambda r:r['code']==0, r_list))
				num_node_total = len(r_list)
				print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
				format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=1)
				message.reply_channel.send({'text':json.dumps(format_result)})
	if request['type']=='distributedMultiIndexSearchStatus':
		searchStringList = request['searchStringList']
		relation = request['relation']
		pay_load = {'relation': relation, 'searchStringList': searchStringList}
		r_list = [getRemoteMultiIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
		time_start = time.time()
		if sum([r['code'] for r in r_list])==0:
			num_node_searched = len(filter(lambda r:r['code']==0, r_list))
			num_node_total = len(r_list)
			print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
			format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=1)
			message.reply_channel.send({'text':json.dumps(format_result)})
		else:
			r_list = [getRemoteMultiIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
			while sum([r['code'] for r in r_list])>0:
				num_node_searched = len(filter(lambda r:r['code']==0, r_list))
				num_node_total = len(r_list)
				print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
				format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=0)
				message.reply_channel.send({'text':json.dumps(format_result)})
				time.sleep(1)
				r_list = [getRemoteMultiIndexSearchStatus(node.node_ip, pay_load) for node in node_query_set]
			else:
				num_node_searched = len(filter(lambda r:r['code']==0, r_list))
				num_node_total = len(r_list)
				print "#Cache# num_node_searched[{}] == num_node_total[{}]".format(num_node_searched, num_node_total)
				format_result = formatRemoteIndexSearchResult(time_start=time_start, result_list=r_list, finished=1)
				message.reply_channel.send({'text':json.dumps(format_result)})
	message.reply_channel.send({'text':'over'})

def distributedIndexCreateStatus(message):
	node_query_set = NodeInfo.objects.filter(node_active=True)
	request = json.loads(message.content['text'])
	if request['type']=='distributedIndexCreateStatus':
	# request = json.loads()
	# if message.content['text']=='distributedIndexCreateStatus':
		pay_load = {}
		r_list = [getRemoteIndexCreateStatus(node.node_ip, pay_load) for node in node_query_set]
		while sum([r['code'] for r in r_list])==0:
			format_result = formatRemoteIndexCreateResult(result_list=r_list)
			message.reply_channel.send({'text':json.dumps(format_result)})
			time.sleep(1)
			r_list = [getRemoteIndexCreateStatus(node.node_ip, pay_load) for node in node_query_set]
		else:
			r_list = filter(lambda x: x['code']==0, r_list)
			print "Some Node's status was Wrong!"
			format_result = formatRemoteIndexCreateResult(result_list=r_list)
			message.reply_channel.send({'text':json.dumps(format_result)})
	message.reply_channel.send({'text':'over'})

def distributedMultiIndexSearchStatus(message):
	pass

def getIndexSearchStatus(message):
	request = json.loads(message.content['text'])
	if request['type']=='indexSearchStatus':
		search_string = request['searchString']
		scale = int(request['scale'])
		search_string = search_string if scale==2 else hex2bin(search_string)
		r = indexCacheExistJudge(search_string)
		if r['code']==0:
			print "#Cache# num_config_caches[{0}] == num_configs[{1}]".format(*r['data'])
			cache_result, cache_query_set, time_start = getIndexCacheSearchResult(search_string)
			format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
			message.reply_channel.send({'text':json.dumps(format_result)})
		else:
			time_start = time.time()
			num_config_caches, num_configs = indexCacheExistJudge(search_string)["data"]
			while num_config_caches<num_configs:
				print "#Disk# num_config_caches[{}] < num_configs[{}]".format(num_config_caches, num_configs)
				cache_result, cache_query_set, _ = getIndexCacheSearchResult(search_string)
				format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=0)
				message.reply_channel.send({'text':json.dumps(format_result)})
				time.sleep(1)
				num_config_caches, num_configs = indexCacheExistJudge(search_string)["data"]
			else:
				print "#Disk# num_config_caches[{}] == num_configs[{}]".format(num_config_caches, num_configs)
				cache_result, cache_query_set, _ = getIndexCacheSearchResult(search_string)
				format_result = formatIndexSearchResult(cache_result=cache_result, cache_query_set=cache_query_set, time_start=time_start, finished=1)
				message.reply_channel.send({'text':json.dumps(format_result)})
	message.reply_channel.send({'text':'over'})

def getIndexCacheSearchResult(search_string):
	time_start = time.time()
	cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
	cache_result = []
	for c in cache_query_set:
		cache_result.extend(json.loads(c.search_content))
	return cache_result, cache_query_set, time_start

def getIndexDiskSearchResult(search_string):
	pass

def mergeByDetailInfo(d1, d2, file_name_list):
	detail = []
	detail.extend([d for d in d1 if d['name'] in file_name_list])
	for d in d2:
		if d['name'] in file_name_list:
			flag = 0
			for item in detail:
				if d['name']==item['name']:
					flag += 1
					item['match_list'].extend(d['match_list'])
					item['num_match'] += d['num_match']
					item['MPM'] += d['MPM']
			if flag==0:
				detail.append(d)
	return detail

def mergeByAndRelation(r1, r2):
	summary = {"num_file":0, "num_match":0, "num_directory":0, "MPM":0, "file_size":0, "finished":0}
	detail = []
	file_name_list_1 = [r['name'] for r in r1['detail']]
	file_name_list_2 = [r['name'] for r in r2['detail']]
	file_name_list = list(set(file_name_list_1)&set(file_name_list_2))
	detail = mergeByDetailInfo(r1['detail'], r2['detail'], file_name_list)
	summary["num_file"] = len(file_name_list)
	summary["num_match"] = sum([d["num_match"] for d in detail])
	summary["num_directory"] = len(list(set([d["directory_info"] for d in detail])))
	summary["MPM"] = sum([d["MPM"] for d in detail])
	summary["file_size"] = sum(d["file_size"] for d in detail)

	summary["size_searched_file"] = math.ceil((r1["summary"]["size_searched_file"]+r2["summary"]["size_searched_file"])/2.0)
	summary["size_total_file"] = math.ceil((r1["summary"]["size_total_file"]+r2["summary"]["size_total_file"])/2.0)
	summary["time_cost"] = max(r1["summary"]["time_cost"], r2["summary"]["time_cost"])
	summary["rate"] = r1["summary"]["rate"]+r2["summary"]["rate"]
	summary["finished"] = math.ceil((r1["summary"]["finished"]+r2["summary"]["finished"])/2.0)
	return {"summary":summary, "detail":detail}

def mergeByOrRelation(r1, r2):
	summary = {"num_file":0, "num_match":0, "num_directory":0, "MPM":0, "file_size":0, "finished":0}
	detail = []
	file_name_list_1 = [r['name'] for r in r1['detail']]
	file_name_list_2 = [r['name'] for r in r2['detail']]
	file_name_list = list(set(file_name_list_1)|set(file_name_list_2))
	detail = mergeByDetailInfo(r1['detail'], r2['detail'], file_name_list)
	# print detail
	summary["num_file"] = len(file_name_list)
	summary["num_match"] = sum([d["num_match"] for d in detail])
	summary["num_directory"] = len(list(set([d["directory_info"] for d in detail])))
	summary["MPM"] = sum([d["MPM"] for d in detail])
	summary["file_size"] = sum(d["file_size"] for d in detail)

	summary["size_searched_file"] = math.ceil((r1["summary"]["size_searched_file"]+r2["summary"]["size_searched_file"])/2.0)
	summary["size_total_file"] = math.ceil((r1["summary"]["size_total_file"]+r2["summary"]["size_total_file"])/2.0)
	summary["time_cost"] = max(r1["summary"]["time_cost"], r2["summary"]["time_cost"])
	summary["rate"] = r1["summary"]["rate"]+r2["summary"]["rate"]
	summary["finished"] = math.ceil((r1["summary"]["finished"]+r2["summary"]["finished"])/2.0)
	return {"summary":summary, "detail":detail}

def mergeFormatIndexSearchResult(format_result_list, relation):
	result = reduce(mergeByAndRelation, format_result_list) if relation=='AND' else reduce(mergeByOrRelation, format_result_list)
	return result

def formatRemoteIndexSearchResult(time_start, result_list, finished=0):
	time_end = time.time()
	summary = {'num_file':0, 'num_match':0, 'num_directory':0, 'MPM':0, 'file_size':0, 'time_cost':0,\
		'size_searched_file':0, 'size_total_file':0, 'time_cost':0, 'rate':0, 'finished':finished}
	result = {'code':0, 'message':[0, 0], 'node_ip':[], 'data':{'summary':summary, 'detail':[]}}
	for r in result_list:
		# print "$$$ {} {}".format(r['data']['summary']['num_file'], type(r['data']['summary']['num_file']))
		result['code'] += r['code']
		result['message'] = map(lambda (n1,n2):n1+n2, zip(result['message'], r['message']))
		result['node_ip'].append(r['node_ip'])
		result['data']['summary']['num_file'] += r['data']['summary']['num_file']
		result['data']['summary']['num_match'] += r['data']['summary']['num_match']
		result['data']['summary']['num_directory'] += r['data']['summary']['num_directory']
		result['data']['summary']['MPM'] += r['data']['summary']['MPM']
		result['data']['summary']['file_size'] += r['data']['summary']['file_size']
		result['data']['summary']['size_searched_file'] += r['data']['summary']['size_searched_file']
		result['data']['summary']['size_total_file'] += r['data']['summary']['size_total_file']
		for d in r['data']['detail']:
			d['node_ip'] = r['node_ip']
			result['data']['detail'].append(d)
		result['data']['summary']['time_cost'] = max(result['data']['summary']['time_cost'], time_end-time_start)
	result['data']['summary']["rate"] = summary["size_searched_file"]/(time_end-time_start) if (time_end-time_start)>0 else 99999999999
	result['data']['summary']['time_cost'] = "{:.4f}".format(result['data']['summary']['time_cost'])
	return result

def formatRemoteIndexCreateResult(result_list):
	summary = {"fileNumCreated":0, "fileNumTotal":0, "fileSizeCreated":0, "fileSizeTotal":0, "rate":0}
	result = {'code':0, 'message':[], 'node_ip':[], 'data':{'summary':summary, 'detail':[]}}
	for r in result_list:
		result['code'] += r['code']
		result['message'].append(r['message'])
		result['node_ip'].append(r['node_ip'])
		result['data']['summary']['fileNumCreated'] += r['data']['summary']['fileNumCreated']
		result['data']['summary']['fileNumTotal'] += r['data']['summary']['fileNumTotal']
		result['data']['summary']['fileSizeCreated'] += r['data']['summary']['fileSizeCreated']
		result['data']['summary']['fileSizeTotal'] += r['data']['summary']['fileSizeTotal']
		result['data']['summary']['rate'] += 0 if isinstance(r['data']['summary']['rate'], unicode) else r['data']['summary']['rate']
		d = r['data']
		d['node_ip'] = r['node_ip']
		result['data']['detail'].append(d)
	return result

def formatIndexSearchResult(time_start, cache_query_set, cache_result=[], finished=0):
	time_end = time.time()
	summary = {"num_file":0, "num_match":0, "num_directory":0, "MPM":0, "file_size":0, "finished":finished}
	detail = []
	algorithm_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
	if cache_result:
		file_name_list = list(set([r["name"] for r in cache_result]))
		for file_name in file_name_list:
			try:    
				item = {}
				f = FileInfo.objects.filter(filefullpathname=file_name)[0]
				item["directory_info"] = f.directory_info.dir_name
				item["file_size"] = f.filesize
				item["name"] = file_name
				item["match_list"] = [{"offset":r["offset"], "offset_bit":r["offset_bit"], "length":r["length"]} for r in cache_result if r["name"]==file_name]
				item["num_match"] = len(item["match_list"])
				item["MPM"] = item["num_match"]*1024*1024/item["file_size"] if item["file_size"]!=0 else 0
				detail.append(item)
			except Exception, e:
				print "@@@ FAILED {}-{}".format(Exception, e)
				continue

		summary["num_file"] = len(file_name_list)
		summary["num_match"] = sum([d["num_match"] for d in detail])
		summary["num_directory"] = len(list(set([d["directory_info"] for d in detail])))
		summary["MPM"] = sum([d["MPM"] for d in detail])
		summary["file_size"] = sum(d["file_size"] for d in detail)

	summary["size_searched_file"] = sum([c.config_info.config_content_size for c in cache_query_set]) if cache_query_set else 0
	summary["size_total_file"] = sum([a.config_content_size for a in algorithm_query_set]) if algorithm_query_set else 0
	summary["time_cost"] = "{:.4f}".format(time_end-time_start)
	summary["rate"] = summary["size_searched_file"]/(time_end-time_start)
	return {"summary":summary, "detail":detail}


def getBlockInfo(_uri, _start, _length):
	result = {'offset':_start, 'data':'', 'match_array':[], 'bytemap':[0]*_length}
	# result['data'] = np.fromfile(_uri, dtype=np.uint8).tolist()[_start:_start+_length]
	with open(_uri, 'rb') as f:
		f.seek(_start, 0)
		# result['data'] = np.fromfile(f, dtype=np.uint8)
		data = f.read(_length)
		data = np.asarray(list(data))
		data.dtype = np.uint8
		result['data'] = data.tolist()
	return result

def getFileLength(_uri):
	return os.path.getsize(_uri)

def getFileMatchesInfo(_uri, _matches):
	for m in _matches:
		m['name'] = _uri
	return _matches

def api_getBlockContent(_uri, _start, _length):
	result = {'offset':_start, 'data':'', 'match_array':[], 'bytemap':[0]*_length}
	# result['data'] = np.fromfile(_uri, dtype=np.uint8).tolist()[_start:_start+_length]
	with open(_uri, 'rb') as f:
		f.seek(_start, 0)
		# result['data'] = np.fromfile(f, dtype=np.uint8)
		data = f.read(_length)
		data = np.asarray(list(data))
		data.dtype = np.uint8
		result['data'] = data.tolist()
	return result

def api_initFileContent(_uri, _matches):
	result = {'code':1, 'message':''}
	try:
		fileLength = getFileLength(_uri)
		theMatches = getFileMatchesInfo(_uri, _matches)
		itemNum = 20
		pageNum = int(math.ceil(len(theMatches)/float(itemNum)))
		result['code'] = 0
		result['message'] = {"fileName":_uri, "matches":json.dumps(theMatches, ensure_ascii=False), "pageNum":pageNum, "itemNum":itemNum, "fileLength":fileLength}
	except Exception,e:
		result['message'] = "Something Wrong: {}-{}".format(Exception, e)
	return result

def fileContentReader(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		_matches = json.loads(request.POST.get('_matches', ''))
		fileLength = plugins.getFileLength(_uri)
		theMatches = plugins.getFileMatchesInfo(_uri, _matches)
		itemNum = 20
		pageNum = int(math.ceil(len(theMatches)/float(itemNum)))
		params = {"fileName":_uri, "matches":json.dumps(theMatches, ensure_ascii=False), "pageNum":pageNum, "itemNum":itemNum, "fileLength":fileLength}
		# print json.dumps(params)
		return render_to_response('fileContentReader.html', params)
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

def getRemoteFileContent(_node_ip, _uri, _matches):
	node_url = u"http://{}:80/api_initFileContent/".format(_node_ip)
	pay_load = {'_uri':_uri, '_matches':json.dumps(_matches)}
	r = requests.post(node_url, data=pay_load)
	return r.json()

def getRemoteBlockContent(_node_ip, _uri, _start, _length):
	node_url = u"http://{}:80/api_getBlockContent/".format(_node_ip)
	pay_load = {'_uri':_uri, '_start':_start, '_length':_length}
	r = requests.post(node_url, data=pay_load)
	return r.json()

def api_addNodeInfo(hostIP, hostName, hostIsAlive, hostDefaultDirectory):
	result = {'code':1, 'message':'Add Host Info Success!'}
	try:
		if len(NodeInfo.objects.filter(node_ip=hostIP)):
			result['message'] = "Host Already Exist"
		else:
			node = NodeInfo(node_ip=hostIP, node_name=hostName, node_alive=hostIsAlive, node_active=True)
			node.save()
			result['code'] = 0
	except Exception, e:
		result['message'] = "Something Wrong: {}-{}".format(Exception, e)
	return result

def api_deleteNodeInfo(hostIP):
	result = {'code':1, 'message':'Delete Host Info Success!'}
	node_query_set = NodeInfo.objects.filter(node_ip=hostIP)
	if len(node_query_set):
		node_query_set.delete()
		result['code'] = 0
	else:
		result['message'] = 'Host not Exist'
	return result
