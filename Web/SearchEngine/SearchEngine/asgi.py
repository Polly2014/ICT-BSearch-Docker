'''
Created on 2016-11-26

@author: Polly
'''

import os

import channels.asgi
 
os.environ.setdefault("DJANGO_SETTINGS_MODULE", "SearchEngine.settings")

channel_layer = channels.asgi.get_channel_layer()