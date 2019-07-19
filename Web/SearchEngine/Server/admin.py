from django.contrib import admin

from models import User, DirectoryInfo, FileInfo, AlgorithmConfigInfo, SearchResultCache, GlobalStaticVarible, NodeInfo
# Register your models here.

admin.site.register(User)
admin.site.register(DirectoryInfo)
admin.site.register(FileInfo)
admin.site.register(AlgorithmConfigInfo)
admin.site.register(SearchResultCache)
admin.site.register(GlobalStaticVarible)
admin.site.register(NodeInfo)