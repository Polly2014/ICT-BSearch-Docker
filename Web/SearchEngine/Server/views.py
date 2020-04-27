# -*- coding: utf-8 -*-
from django.shortcuts import render, render_to_response
from django.http import HttpResponse, HttpResponseRedirect
from django.views.decorators.csrf import csrf_protect, csrf_exempt
from django.contrib.auth.decorators import login_required
from django.contrib.auth.models import User as AuthUser
from django.contrib import auth

from models import DirectoryInfo, FileInfo
from forms import addDirectoryForm
import plugins
from plugins import getFileFromDirectory, getDirectoryStatusList, importFileInfoToDatabase, importDirInfoToDatabase, makeAlgorithmConfigFiles, runIndexCreateProgram, runIndexSearchProgram, formatIndexSearchResult
# Create your views here.
import subprocess
import json
import math
import time
import os

ISOTIMEFORMAT = '%Y-%m-%d %X'

# import sys
# reload(sys)
# sys.setdefaultencoding("utf-8")
# print sys.getdefaultencoding()

def index(request):
	#directory_list = makeAlgorithmConfigFiles()
	#directory_list = runIndexCreateProgram()
	#message = request.GET.get("message", "")
	error_message = request.session.get("error_message", "")
	return render_to_response('index.html', {"error_message":error_message})
	#result = runIndexCreateProgram()
	#return HttpResponse(result)

@csrf_exempt
def login(request):
	errors = []
	username = request.POST.get("username", "")
	password = request.POST.get("password", "")
	user = auth.authenticate(username=username, password=password)
	if user is not None and user.is_active:
		auth.login(request, user)
		return HttpResponseRedirect('/dashboard/')
	else:
		return render_to_response('login.html', {"errors":errors})

@login_required
def logout(request):
	auth.logout(request)
	return HttpResponseRedirect('/')

@csrf_exempt
def register(request):
	if request.method=="POST":
		username = request.POST.get("username", "")
		password = request.POST.get("password", "")
		email = request.POST.get("email", "")
		errors = []

		tag = len(AuthUser.objects.filter(username=username))
		if tag>0:
			errors.append(u"Username Already exist!")
			return render_to_response("register.html", {"errors":errors})
		else:
			u = AuthUser.objects.create_user(username=username, password=password, email=email)
			u.is_staff = True
			u.save()
			return HttpResponseRedirect('/login/')
	return render_to_response("register.html", {})

@csrf_exempt
def changeString(request):
	option = request.POST.get("option", "")
	scale = int(request.POST.get("scale", 2))
	searchString = request.POST.get("searchString", "")
	searchString = plugins.changeString(searchString, scale, option)
	return HttpResponse(searchString)

@csrf_exempt
def validString(request):
	scale = int(request.POST.get("scale", 2))
	searchString = request.POST.get("searchString", "")
	return HttpResponse(True if plugins.validString(searchString, scale) else False)

@csrf_exempt
def validMultiString(request):
	searchString1 = request.POST.get("firstSearchString", "")
	searchString2 = request.POST.get("secondSearchString", "")
	return HttpResponse(True if plugins.validMultiString([searchString1, searchString2]) else False)

@login_required(login_url='/login/')
def dashboard(request, status="creating"):
	directory_list = getDirectoryStatusList()
	return render_to_response("{}.html".format(status), {"dir_list":directory_list})

@csrf_exempt
def directoryAddition(request):
	directory_string = request.POST.get('directory', '')
	result = plugins.directoryValidAndImport(directory_string)
	if result["code"]==0:
		result = plugins.directoryAnalysisProgram(directory_string)
		if result["code"]==0:
			result = plugins.makeAlgorithmConfigFiles()
			if result["code"]==0:
				result = plugins.runIndexCreateProgram()
				if result["code"]==0:
					return HttpResponseRedirect('/dashboard/')
					#return HttpResponse(json.dumps(result), content_type="application/json")
				else:
					#return HttpResponse(json.dumps(result), content_type="application/json")
					return HttpResponseRedirect('/dashboard/')
			else:
				#return HttpResponse(json.dumps(result), content_type="application/json")
				return HttpResponseRedirect('/dashboard/')
		else:
			#return HttpResponse(json.dumps(result), content_type="application/json")
			return HttpResponseRedirect('/dashboard/')
	else:
		#return HttpResponse(json.dumps(result), content_type="application/json")
		return HttpResponseRedirect('/dashboard/')

def history(request):
	plugins.runHistorySearchStatisticUpdateProgram()
	search_string = request.GET.get("searchString", "")
	if search_string:
		return render_to_response("historyDetail.html", {"request": request, "search_string": search_string})
	else:
		history_list = plugins.getHistorySearchList()
		return render_to_response("history.html", {"history_list":history_list})

@csrf_exempt
def getHistoryDetail(request):
	search_string = request.POST.get("searchString", "")
	if search_string:
		result = plugins.getHistorySearchDetail(search_string)
		return HttpResponse(json.dumps(result))
	else:
		HttpResponseRedirect('/history/')

@csrf_exempt
def addDirectorytoDatabase(request):
	dirs_string = request.POST['directories']
	dir_list = dirs_string.split(';')
	dir_list = [d for d in dir_list if os.path.exists(d.strip())]
	importDirInfoToDatabase(dir_list)
	file_list = []
	for dir in dir_list:
		tmp = dir.strip()
		file_list += getFileFromDirectory(tmp)
	importFileInfoToDatabase(file_list)
	config_file_list = makeAlgorithmConfigFiles()
	runIndexCreateProgram()
	return HttpResponseRedirect('/dashboard/')
	#return HttpResponse(file_list)
	#return render_to_response('dashboard.html',{})
	'''
	if request.method == "POST":
		form = addDirectoryForm(request.POST)
		if form.is_valid():
			data = form.cleaned_data
			dirs_string = data['directories']
			dir_list = dirs_string.split(';')
			#importDirInfoToDatabase(dir_list)
			file_list = []
			for dir in dir_list:
				tmp = dir.strip()
				file_list += getFileFromDirectory(tmp)
			#importFileInfoToDatabase(file_list)
			return HttpResponse(file_list)
			#return HttpResponseRedirect('/dashboard')
	else:
		form = addDirectoryForm()
	return render_to_response('dashboard.html',{'form':form})
	'''

@csrf_exempt
def search(request):
	result = []
	search_string = request.GET.get("searchString","")
	result = plugins.searchStringValid(search_string)
	if result["code"] == 0:
		#return HttpResponse(json.dumps(result), content_type="application/json")
		request.session["error_message"] = ""
		return render_to_response('searchResult.html',{"searchString":search_string})
	else:
		#return HttpResponse(json.dumps(result), content_type="application/json")
		request.session["error_message"] = result["message"]
		#return HttpResponseRedirect('/?message=inputError')
		return HttpResponseRedirect('/')
	# if len(search_string)>14:
	#     '''
	#     #search_string = "1010101010101010101011111"
	#     time_start = time.time()
	#     result_list = runIndexSearchProgram(search_string)
	#     time_end = time.time()
	#     format_result = formatIndexSearchResult(result_list)
	#     format_result["summary"]["time_cost"] = "{:.4}".format(time_end-time_start)
	#     #return HttpResponse(json.dumps(format_result))
	#     return render_to_response('searchResult.html',{"format_result":format_result, "searchString":search_string})
	#     '''
	#     return render_to_response('searchResult.html',{"searchString":search_string})
	# else:
	#     #return render_to_response('index.html',{"errors":errors})
	#     return HttpResponseRedirect('/?message=inputError')

def test(request):
	# search_string = request.GET.get("searchString","")
	# return render_to_response('bsearch.html',{"searchString":search_string})
	#result = plugins.runHistorySearchStatisticUpdateProgram()
	
	# result = plugins.getDetailMatchInfo()
	# result = unicode(result, errors="replace")
	# print result
	#return HttpResponse(result)
	
	return render_to_response("cluster.html", {})

def testCluster(request):
	return render_to_response("cluster.html", {})

def testSingleNode(request):
	return render_to_response("singleNode.html", {})

@csrf_exempt
def getDirectoryBrowserInfo(request):
	base_dir = request.POST.get("currentPath", "")
	result = plugins.getDirectoryBrowserInfo(base_dir) if base_dir else plugins.getDirectoryBrowserInfo()
	return result

@csrf_exempt
def getDetailMatchInfo(request):

	file_name = request.POST.get("name", "")
	matches = json.loads(request.POST.get("matches", ""))
	pageNum = int(request.POST.get("pageNum", ""))

	match_list = matches[(pageNum-1)*5:pageNum*5]
	for i in match_list:
		i["length_bit"] = i["length"]

	result = plugins.getDetailMatchInfo(file_name, match_list)
	#result = unicode(result, errors="ignore")
	#print result
	#return HttpResponse(json.dumps(result), content_type="application/json")
	return HttpResponse(result)

@csrf_exempt
def getHDFSDirectoryBrowserInfo(request):
	base_dir = request.POST.get("currentPath", "")
	result = plugins.getHDFSBrowserInfo(base_dir) if base_dir else plugins.getHDFSBrowserInfo()
	return HttpResponse(result)

@csrf_exempt
def getRemoteDirectoryBrowserInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		currentPath = request.POST.get("currentPath", "")
		node_ip = request.POST.get("nodeIP", "")
		pay_load = {'currentPath': currentPath}
		result = plugins.getRemoteDirectoryBrowserInfo(node_ip, pay_load)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_getDirectoryBrowserInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		base_dir = request.POST.get("currentPath", "")
		result = plugins.api_getDirectoryBrowserInfo(base_dir) if base_dir else plugins.api_getDirectoryBrowserInfo()
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_indexCreate(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		directory_string = request.POST.get('directory', '')
		result = plugins.api_indexCreate(directory_string)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_indexSearch(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		scale = int(request.POST.get("scale", 0))
		search_type = "BSearchString" if scale==2 else "HSearchString"
		search_string = request.POST.get(search_type)
		result = plugins.api_indexSearch(search_string, scale)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_indexSearchStatus(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		scale = int(request.POST.get("scale", 0))
		search_string = request.POST.get("searchString", "")
		if scale==0 or search_string=="":
			result['message'] = 'Wrong Scale or SearchString Input'
		else:
			result = plugins.api_indexSearchStatus(search_string, scale)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_indexCreateStatus(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		result = plugins.api_indexCreateStatus()
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_multiIndexSearch(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		searchString1 = request.POST.get("FirstSearchString", "").encode("utf-8")
		searchString2 = request.POST.get("SecondSearchString", "").encode("utf-8")
		relation = request.POST.get('relation', 'AND')
		searchStringList = [searchString1, searchString2]
		result = plugins.multiIndexSearch(searchStringList, relation)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_multiIndexSearchStatus(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		searchStringList = json.loads(request.POST.get("searchStringList", '[]'))
		relation = request.POST.get("relation", "")
		if len(searchStringList)==0 or relation=="":
			result['message'] = 'Wrong Relation or SearchString Input'
		else:
			result = plugins.api_multiIndexSearchStatus(searchStringList, relation)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_initFileContent(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		_matches = json.loads(request.POST.get('_matches', ''))
		_node_ip = request.POST.get('_node_ip', '')
		result = plugins.api_initFileContent(_uri, _matches)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_getBlockContent(request):
	result = {'offset':0, 'data':[], 'match_array':[], 'bytemap':[]}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		_start = int(request.POST.get('_start', 0))
		_length = int(request.POST.get('_length', 0))
		result = plugins.api_getBlockContent(_uri, _start, _length)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_addNodeInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		hostIP = request.POST.get('hostIP', '')
		hostName = request.POST.get('hostName', u'匿名')
		hostIsAlive = request.POST.get('isAlive', True)
		hostDefaultDirectory = request.POST.get('defaultDirectory', '/dataFile')
		result = plugins.api_addNodeInfo(hostIP, hostName, hostIsAlive, hostDefaultDirectory)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_deleteNodeInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		hostIP = request.POST.get('hostIP', '')
		result = plugins.api_deleteNodeInfo(hostIP)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def indexCreate(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		directory_string = request.POST.get('directory', '')
		node_ip = request.POST.get('nodeIP', '')
		result = plugins.distributedIndexCreate(directory_string, node_ip)
		if result['code']==0:
			return HttpResponseRedirect('/testSingleNode/?ip='+node_ip)
		else:
			return HttpResponseRedirect('/testSingleNode/?ip='+node_ip)
	else:
		return HttpResponseRedirect('/testSingleNode/?ip='+node_ip)

@csrf_exempt
def indexSearch(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		scale = int(request.POST.get("scale", 0))
		search_type = "BSearchString" if scale==2 else "HSearchString"
		search_string = request.POST.get(search_type)
		result = plugins.distributedIndexSearch(search_string, scale)
		# return HttpResponse(json.dumps(result), content_type='application/json')
		if result['code']==0:
			return render_to_response('searchResult.html',{"searchString":search_string, "scale":scale})
		else:
			return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def multiIndexSearch(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		searchString1 = request.POST.get("FirstSearchString", "").encode("utf-8")
		searchString2 = request.POST.get("SecondSearchString", "").encode("utf-8")
		relation = request.POST.get('relation', 'AND')
		searchStringList = [searchString1, searchString2]
		result = plugins.multiIndexSearch(searchStringList, relation)
		# return HttpResponse(json.dumps(result), content_type='application/json')
		return render_to_response('multiSearchResult.html',{"searchStringList":searchStringList, "relation":relation})
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def getBlockInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		_start = int(request.POST.get('_start', 0))
		_length = int(request.POST.get('_length', 0))
		result = plugins.getBlockInfo(_uri, _start, _length)
		# print "------------------------------"
		# print "Server Result:{}".format(result)
		# print "------------------------------"
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def getFileLength(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		result = plugins.getFileLength(_uri)
		return HttpResponse(result)
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def getFileMatchesInfo(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('name', '')
		_matches = json.loads(request.POST.get('matches', ''))
		result = plugins.getFileMatchesInfo(_uri, _matches)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
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

@csrf_exempt
def fileContentBrowser(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_uri = request.POST.get('_uri', '')
		_matches = json.loads(request.POST.get('_matches', ''))
		_node_ip = request.POST.get('_node_ip', '')
		result = plugins.getRemoteFileContent(_node_ip, _uri, _matches)
		if type(result['message'])==dict:
			result['message']['nodeIP'] = _node_ip
			return render_to_response('fileContentBrowser.html', result['message'])
		else:
			return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def blockContentBrowser(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		_node_ip = request.POST.get('_node_ip', '')
		_uri = request.POST.get('_uri', '')
		_start = int(request.POST.get('_start', 0))
		_length = int(request.POST.get('_length', 0))
		result = plugins.getRemoteBlockContent(_node_ip, _uri, _start, _length)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')


# --------------------------------------------------- #
# Added For New Interface Request
# Added By Polly
# Added in 2020-04-24
@csrf_exempt
def api_binarySearchResult(request):
	result = {'code':1, 'message':'Only Support POST Method'}
	if request.method=='POST':
		scale = int(request.POST.get("scale", 0))
		search_string = request.POST.get("search_string", "")
		pageNum = int(request.POST.get('pageNum', 1))
		pageSize = int(request.POST.get('pageSize', 50))
		order = int(request.POST.get('order', 0))
		expandedName = request.POST.get('expandedName', '')
		if scale==0 or search_string=="":
			result['message'] = 'Wrong Scale or SearchString Input'
		else:
			pay_load = {'search_string': search_string, 'scale':scale, 'pageNum':pageNum, 'pageSize':pageSize, 'order':order, 'expandedName':expandedName}
			result = plugins.api_binarySearchResult(pay_load)
		return HttpResponse(json.dumps(result), content_type='application/json')
	else:
		return HttpResponse(json.dumps(result), content_type='application/json')

@csrf_exempt
def api_binarySearch(request):
	data = {'total':0, 'records':[]}
	result = {'resultCode':0, 'message':u'响应成功', 'data':{}}
	if request.method=='POST':
		# Get Parameters
		keyword = request.POST.get('keyword', '')
		operateType = int(request.POST.get('operateType', 0))
		pageNum = int(request.POST.get('pageNum', 1))
		pageSize = int(request.POST.get('pageSize', 50))
		order = int(request.POST.get('order', 0))
		expandedName = request.POST.get('expandedName', '')
		binaryType = int(request.POST.get('binaryType', 0))
		extention = request.POST.get('extention', '')

		# Parameters Judge
		if not keyword:
			result['resultCode'] = 2002
			result['message'] = u'[参数错误] No keyword'
			return result
		if not operateType in range(0,4):
			result['resultCode']= 2002
			result['message'] = u'[参数错误] operateType should be in [0-3]'
			return result
		if not order in range(0,2):
			result['resultCode']= 2002
			result['message'] = u'[参数错误] order should be in [0-1]'
			return result
		if not pageNum>0:
			result['resultCode']= 2002
			result['message'] = u'[参数错误] pageNum should greater than 1'
			return result
		if not binaryType in range(0,2):
			result['resultCode']= 2002
			result['message'] = u'[参数错误] binaryType should be in [0-1]'
			return result

		# Nomalize Parameters
		LIST_OPTION = ['no','negate','reverse','HLreverse']
		scale = 2 if binaryType==0 else 16
		option = LIST_OPTION[operateType]
		search_string = plugins.changeString(keyword, scale, option)

		# Search Program Run
		r_search = plugins.distributedIndexSearch(search_string, scale)
		if r_search['code']==0:
			# Gether Result From All Nodes
			pay_load = {'search_string': search_string, 'scale':scale, 'pageNum':pageNum, 'pageSize':pageSize, 'order':order, 'expandedName':expandedName}
			r_return = plugins.api_getBinarySearchResult(pay_load)
			result = r_return
		else:
			result['message'] = r
	else:
		result['resultCode'] = 2005
		result['message'] = u'[请求错误] HTTP方法不支持'
	return HttpResponse(json.dumps(result), content_type='application/json')
