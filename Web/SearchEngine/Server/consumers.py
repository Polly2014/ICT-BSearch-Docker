# -*- coding: utf-8 -*-
'''
Created on 2016-11-25

@author: Polly
'''
from django.http import HttpResponse
from channels.handler import AsgiHandler
'''
def http_consumer(message):
	# Make standard HTTP response - access ASGI path attribute directly
	response = HttpResponse("Hello world! You asked for %s" % message.content['path'])
	# Encode that response into message format (ASGI)
	for chunk in AsgiHandler.encode_response(response):
		message.reply_channel.send(chunk)
'''
'''
def ws_message(message):
	# ASGI WebSocket packet-received and send-packet message types
	# both have a "text" key for their textual data.
	message.reply_channel.send({
		"text": message.content['text'],
	})
'''

from models import FileInfo, DirectoryInfo, SearchResultCache, AlgorithmConfigInfo
from datetime import datetime
import time
import pprint
from channels import Group, Channel
import json
import plugins

ISOTIMEFORMAT = '%Y-%m-%d %X'


# Connected to websocket.connect
def ws_connect(message):
	pass
	#message.reply_channel.send({"text":"connect"})
	#message.reply_channel.send({"accept": True})
	#print message.content
	
	#Group("chat").add(message.reply_channel)
	#Group("chat").send({
	#    "text": "Hello world",
	#})
	
	
def ws_receive_create(message):
	if message.content['text']=='createInformation':
		while True:
			progressInformation = plugins.getDirectoryStatusList()
			message.reply_channel.send({
				"text": json.dumps(progressInformation)
			})
			time.sleep(1)
	else:
		message.reply_channel.send({"text":''})

def ws_index_search_status(message):
	plugins.getIndexSearchStatus(message)

def ws_multi_index_search_status(message):
	plugins.getMultiIndexSearchStatus(message)

def api_index_search_status(message):
	plugins.distributedIndexSearchStatus(message)

def api_multi_index_search_status(message):
	plugins.distributedIndexSearchStatus(message)

def api_index_create_status(message):
	plugins.distributedIndexCreateStatus(message)

def ws_receive_search(message):
	request = json.loads(message.content["text"])
	if request["type"]=='searchInformation':
		search_string = request["searchString"]
		cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
		algorithm_query_set = AlgorithmConfigInfo.objects.filter(config_flag=1)
		num_configs = len(algorithm_query_set)
		num_config_caches = len(cache_query_set)
		if num_configs==num_config_caches:
			print "### num_configs == num_config_caches"
			time_start = time.time()
			result_list = []
			for c in cache_query_set:
				result_list.extend(json.loads(c.search_content))
			time_end = time.time()
			format_result = plugins.formatIndexSearchResult(result_list)
			format_result["summary"]["size_searched_file"] = sum([c.config_info.config_content_size for c in cache_query_set])
			format_result["summary"]["size_total_file"] = sum([a.config_content_size for a in algorithm_query_set])
			format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
			format_result["summary"]["rate"] = format_result["summary"]["size_searched_file"]/(time_end-time_start)
			format_result["summary"]["flag"] = 1
			message.reply_channel.send({
				"text": json.dumps(format_result)
			})
		else:
			print "### num_configs != num_config_caches"
			plugins.runIndexSearchProgram(search_string)
			time_start = time.time()
			print "### start search loop"
			while True:
				plugins.close_old_connections()
				cache_query_set = SearchResultCache.objects.filter(search_string=search_string)
				try:
					num_config_caches = cache_query_set.count()
				except Exception,e:
					print "@@@@ Exception: {}-{}".format(Exception, e)
				if num_configs>num_config_caches:
					print "### num_configs[{}] > num_config_caches[{}]".format(num_configs, num_config_caches)
					try:
						result_list = []
						for c in cache_query_set:
							result_list.extend(json.loads(c.search_content))
						time_end = time.time()
						format_result = plugins.formatIndexSearchResult(result_list)
						format_result["summary"]["size_searched_file"] = sum([c.config_info.config_content_size for c in cache_query_set])
						format_result["summary"]["size_total_file"] = sum([a.config_content_size for a in algorithm_query_set])
						format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
						format_result["summary"]["rate"] = format_result["summary"]["size_searched_file"]/(time_end-time_start)
						format_result["summary"]["flag"] = 0
						message.reply_channel.send({
							"text": json.dumps(format_result),
						})
						time.sleep(1)
					except Exception,e:
						print "@@@@@ New Error:{}-{}".format(Exception,e)
				else:
					print "### num_configs == num_config_caches"
					result_list = []
					for c in cache_query_set:
						result_list.extend(json.loads(c.search_content))
					time_end = time.time()
					format_result = plugins.formatIndexSearchResult(result_list)
					format_result["summary"]["size_searched_file"] = sum([c.config_info.config_content_size for c in cache_query_set])
					format_result["summary"]["size_total_file"] = sum([a.config_content_size for a in algorithm_query_set])
					format_result["summary"]["time_cost"] = "{:.4f}".format(time_end-time_start)
					format_result["summary"]["rate"] = format_result["summary"]["size_searched_file"]/(time_end-time_start)
					format_result["summary"]["flag"] = 1
					message.reply_channel.send({
						"text": json.dumps(format_result),
					})
					print "### It's time to break"
					break
		print "Jump out of the loop successfull!"
		message.reply_channel.send({"text":"Over"})
	else:
		message.reply_channel.send({"text":"Over"})

# Connected to websocket.receive
def ws_receive(message):
	#while True:
	files = FileInfo.objects.all()
#     Group("chat").send({
#         "text": "[user] %s" % message.content['text'],
#     })
	if message.content['text']=='processInformation':
		while True:
			progressInformation = getProgressInformation()
			#pprint.pprint(dirs)
#             Group("chat").send({
#                 #"text": "[user] %s" % message.content['text'],
#                 "text": json.dumps(dirs),
#             })
			#print [k for k in message.content.items()]
			message.reply_channel.send({
				"text": json.dumps(progressInformation)
			})
			time.sleep(1)
	elif message.content['text']=='searchInformation':
		search_string = "1010101010101010101011111"
		result_list = plugins.runIndexSearchProgram(search_string)
		format_result = plugins.formatIndexSearchResult(result_list)
		message.reply_channel.send({
			"text": json.dumps(format_result)
		})
	else:
		message.reply_channel.send({"text":''})

# Connected to websocket.disconnect
def ws_disconnect(message):
	#Group("chat").discard(message.reply_channel)
	message.reply_channel.send({"text":"disconnect"})
