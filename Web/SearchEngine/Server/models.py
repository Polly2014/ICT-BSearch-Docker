# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models

import django.utils.timezone as timezone

# Create your models here.

class User(models.Model):
    username = models.CharField('User Name', max_length=30)
    password = models.CharField('Password', max_length=30)
    
    def __unicode__(self):
        return self.username
    
class DirectoryInfo(models.Model):
    dir_name = models.CharField('Directory Name', max_length=300, default='')
    #dirsize = models.IntegerField('Total Size(MB)', default=0)
    #filenum = models.IntegerField('Num of File', default=0)
    dir_flag = models.IntegerField('Status(0-1)', default=0)
    createtime = models.DateTimeField('Start Time', default=timezone.now)
    finishtime = models.DateTimeField('Finish Time', blank=True, null=True)
    
    def __unicode__(self):
        return self.dir_name
    
class FileInfo(models.Model):
    dir_name = models.CharField('Directory Name', max_length=300)
    directory_info = models.ForeignKey(DirectoryInfo, related_name='files', blank=True, null=True)
    filefullpathname = models.CharField('File Full Path And Name', max_length=500)
    filename = models.CharField('File Name', max_length=200)
    fileflag = models.IntegerField('Status(0-2)', default=0)
    filesize = models.BigIntegerField('File Size(byte)')
    configured = models.IntegerField('Has Config File(0-1)', default=0)
    createtime = models.DateTimeField('Start Time', default=timezone.now)
    finishtime = models.DateTimeField('Finish Time', blank=True, default=timezone.now)
    
    def __unicode__(self):
        return self.filename
    
class AlgorithmConfigInfo(models.Model):
    dir_name = models.CharField('Directory Name', max_length=300)
    directory_info = models.ForeignKey(DirectoryInfo, related_name='configs', blank=True, null=True)
    config_name = models.CharField('Config File Name', max_length=500)
    config_content = models.TextField('Config Content')
    config_content_size = models.BigIntegerField('Config Content Total Size(byte)', default=0)
    config_flag = models.IntegerField('Flag(0-1)', default=0)
    
    def __unicode__(self):
        return self.config_name

class SearchResultCache(models.Model):
    """
    The Cache of User's search result history
    """
    search_string = models.TextField('Search String')
    config_info = models.ForeignKey(AlgorithmConfigInfo, related_name='search_caches')
    search_content = models.TextField('Search Content')

    def __unicode__(self):
        return "{} % {}".format(self.search_string, self.config_info)
        
class GlobalStaticVarible(models.Model):
    varible_label = models.CharField('Varibles Lable', max_length=100)
    varible_content = models.TextField('Varibles Content', default='')

    def __unicode__(self):
        return "{}".format(self.varible_label)

class NodeInfo(models.Model):
    node_name = models.CharField('Node Name', max_length=50)
    node_ip = models.CharField('Node IP', max_length=15, default='127.0.0.1')
    node_active = models.BooleanField('Node Active', default=True)
    node_alive = models.BooleanField('Node Alive', default=True)
    def __unicode__(self):
        return "{}".format(self.node_name)


    """
    struct_info["average"]["num_file"] = sum([s["num_file"] for s in struct_info_list])/float(num_search_string)
    struct_info["average"]["num_match"] = sum([s["num_match"] for s in struct_info_list])/float(num_search_string)
    struct_info["average"]["coverage"] = sum([s["num_directory"] for s in struct_info_list])/float(num_directory*num_search_string)
    struct_info["average"]["MPM"] = sum([s["MPM"] for s in struct_info_list])/float(num_search_string)
    struct_info["average"]["file_size"] = sum([s["file_size"] for s in struct_info_list])/float(num_search_string)
    """