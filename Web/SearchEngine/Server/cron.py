# -*- coding: utf-8 -*-
'''
Created on 2016-12-12

@author: Polly
'''
import plugins

def test():
    for i in range(10):
        print "%s times has been done!"%i
        
def runDirectoryAnalysisProgram():
    directory_list = plugins.getDirectoryList()
	file_list = plugins.getFileListFromDirectoryList()
    plugins.importFileInfoToDatabase(file_list)
    plugins.makeAlgorithmConfigFiles()
    plugins.runIndexCreateProgram()
    
