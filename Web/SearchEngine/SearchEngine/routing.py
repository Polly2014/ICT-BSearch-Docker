'''
Created on 2016-11-25

@author: Polly
'''

'''
from channels.routing import route
channel_routing = [
    route("http.request", "Server.consumers.http_consumer"),
]
'''

from channels.routing import route
from Server.consumers import *

channel_routing = [
    #route("websocket.receive", ws_message, path="^/chat/"),
    #route("websocket.connect", ws_connect),
    route("websocket.connect", ws_connect),
    route("websocket.receive", ws_receive_create, path="^/indexCreate/"),
    route("websocket.receive", ws_receive_search, path="^/indexSearch/"),
    route("websocket.receive", ws_index_search_status, path="^/indexSearchStatus/"),
    route("websocket.receive", ws_multi_index_search_status, path="^/multiIndexSearchStatus/"),

    route("websocket.receive", api_index_search_status, path="^/api_indexSearchStatus/"),
    route("websocket.receive", api_multi_index_search_status, path="^/api_multiIndexSearchStatus/"),
    route("websocket.receive", api_index_create_status, path="^/api_indexCreateStatus/"),
    
    route("websocket.disconnect", ws_disconnect),
]