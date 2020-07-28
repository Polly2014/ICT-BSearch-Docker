"""SearchEngine URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/1.9/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  url(r'^$', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  url(r'^$', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.conf.urls import url, include
    2. Add a URL to urlpatterns:  url(r'^blog/', include('blog.urls'))
"""
from django.conf.urls import url
from django.contrib import admin
from Server import views as se_views
import settings

urlpatterns = [
    url(r'^admin/', admin.site.urls),
    url(r'^$', se_views.index, name='default'),
    url(r'^dashboard/$', se_views.dashboard, name='dashboard_index'),
    url(r'^dashboard/(creating|created)/$',
        se_views.dashboard, name='dashboard'),
    url(r'^login/$', se_views.login, name='login'),
    url(r'^logout/$', se_views.logout, name='logout'),
    url(r'^register/$', se_views.register, name='register'),
    url(r'^search/$', se_views.search, name="search"),
    url(r'^history/$', se_views.history, name="history"),
    url(r'^getHistoryDetail/$', se_views.getHistoryDetail, name="getHistoryDetail"),
    url(r'^getDetailMatchInfo/$', se_views.getDetailMatchInfo,
        name="getDetailMatchInfo"),
    url(r'^getDirectoryBrowserInfo/$', se_views.getDirectoryBrowserInfo,
        name="getDirectoryBrowserInfo"),
    url(r'^addDirectoryToDatabase/$', se_views.addDirectorytoDatabase,
        name='addDirectoryToDatabase'),
    url(r'^directoryAddition/$', se_views.directoryAddition,
        name='directoryAddition'),
    url(r'^test/$', se_views.test, name='test'),
    url(r'^testCluster/$', se_views.testCluster, name='testCluster'),
    url(r'^testSingleNode/$', se_views.testSingleNode, name='testSingleNode'),
    url(r'^changeString/$', se_views.changeString, name='changeString'),
    url(r'^validString/$', se_views.validString, name='validString'),
    url(r'^validMultiString/$', se_views.validMultiString, name='validMultiString'),

    url(r'^indexSearch/$', se_views.indexSearch, name='indexSearch'),
    url(r'^indexCreate/$', se_views.indexCreate, name='indexCreate'),
    url(r'^multiIndexSearch/$', se_views.multiIndexSearch, name='multiIndexSearch'),
    url(r'^getBlockInfo/$', se_views.getBlockInfo, name='getBlockInfo'),
    url(r'^getFileLength/$', se_views.getFileLength, name='getFileLength'),
    url(r'^getFileMatchesInfo/$', se_views.getFileMatchesInfo,
        name='getFileMatchesInfo'),
    url(r'^fileContentReader/$', se_views.fileContentReader,
        name='fileContentReader'),
    url(r'^fileContentBrowser/$', se_views.fileContentBrowser,
        name='fileContentBrowser'),
    url(r'^blockContentBrowser/$', se_views.blockContentBrowser,
        name='blockContentBrowser'),
    url(r'^getRemoteDirectoryBrowserInfo/$',
        se_views.getRemoteDirectoryBrowserInfo, name='getRemoteDirectoryBrowserInfo'),

    url(r'^api_indexSearch/$', se_views.api_indexSearch, name='api_indexSearch'),
    url(r'^api_multiIndexSearch/$', se_views.api_multiIndexSearch,
        name='api_multiIndexSearch'),
    url(r'^api_indexSearchStatus/$', se_views.api_indexSearchStatus,
        name='api_indexSearchStatus'),
    url(r'^api_multiIndexSearchStatus/$',
        se_views.api_multiIndexSearchStatus, name='api_multiIndexSearchStatus'),
    url(r'^api_initFileContent/$', se_views.api_initFileContent,
        name='api_initFileContent'),
    url(r'^api_getBlockContent/$', se_views.api_getBlockContent,
        name='api_getBlockContent'),
    url(r'^api_indexCreate/$', se_views.api_indexCreate, name='api_indexCreate'),
    url(r'^api_indexCreateStatus/$', se_views.api_indexCreateStatus,
        name='api_indexCreateStatus'),
    url(r'^api_getDirectoryBrowserInfo/$',
        se_views.api_getDirectoryBrowserInfo, name='api_getDirectoryBrowserInfo'),
    url(r'^api_addNodeInfo/$', se_views.api_addNodeInfo, name='api_addNodeInfo'),
    url(r'^api_deleteNodeInfo/$', se_views.api_deleteNodeInfo,
        name='api_deleteNodeInfo'),
    #url(r'^static/(?P<path>.*)$', 'django.views.static.serve',{'document_root': settings.STATIC_ROOT }),

    url(r'^api/search/binarySearch$',
        se_views.api_binarySearch, name='api_binarySearch'),
    url(r'api_binarySearchResult/$', se_views.api_binarySearchResult,
        name='api_binarySearchResult'),

    url(r'^api/search/binaryDetails$',
        se_views.api_binarySearchDetail, name='api_binarySearchDetail'),
]
