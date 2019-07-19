# -*- coding: utf-8 -*-
'''
Created on 2016-12-5

@author: Polly
'''

from django import forms

class addDirectoryForm(forms.Form):
    directories = forms.CharField(widget=forms.Textarea)
    