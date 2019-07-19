from django.test import TestCase, Client
from django.contrib.auth.models import User as AuthUser
import plugins
import json, os, logging, time
from models import *
# Create your tests here.

TEST_DIRECTORY_PATH = r"/home/polly/test/func_test_set"
TEST_CONFIG_FILE_PATH = r"/home/polly/test/func_test_config"
TEST_LOG_FILE_PATH = r"/home/polly/test/"

SEARCH_STRING_SET_LIST = [
	{
		"set_name": "short_string_set",
		"set_return_code": 1,
		"set_data_list": [
			"1101001","......",".011.0"
		]
	},{
		"set_name": "normal_string_set",
		"set_return_code": 0,
		"set_data_list": [
			"10101001010101010",".101.01.01.01.01.010"
		]
	},{
		"set_name": "exact_string_set",
		"set_return_code": 0,
		"set_data_list": [
			"101001010101010101010"
		]
	},{
		"set_name": "fuzzy_string_set",
		"set_return_code": 1,
		"set_data_list": [
			"..................."
		]
	},{
		"set_name": "mix_string_set",
		"set_return_code": 0,
		"set_data_list": [
			"01.1.0.01.011.010.01."
		]
	},{
		"set_name": "boundary_string_set",
		"set_return_code": 0,
		"set_data_list": [
			"000000000000000000000"
		]
	},{
		"set_name": "except_string_set",
		"set_return_code": 1,
		"set_data_list": [
			"1@2038120#983120","siaofuoias123"
		]
	}
]

DIRECTORY_LIST = [
	{
		"directory_name": "test_null",
		"directory_return_code": 0
	},{
		"directory_name": "test_single",
		"directory_return_code": 0
	},{
		"directory_name": "test_multi_brand",
		"directory_return_code": 0
	}
]


def init_logger(log_name=''):
	logger = logging.getLogger(log_name)
	logger.setLevel(logging.DEBUG)
	fh = logging.FileHandler(TEST_LOG_FILE_PATH+log_name+".log")
	fh.setLevel(logging.DEBUG)
	formatter = logging.Formatter('[%(asctime)s] %(name)s: %(levelname)s: %(message)s')
	fh.setFormatter(formatter)
	logger.addHandler(fh)
	return logger

def init_directory_list():
	file_name = os.path.join(TEST_CONFIG_FILE_PATH, "directory_list.json")
	with open(file_name) as f:
		result = json.loads(f.read())
	return result

def init_search_string_set_list():
	file_name = os.path.join(TEST_CONFIG_FILE_PATH, "search_string_set_list.json")
	with open(file_name) as f:
		result = json.loads(f.read())
	return result


class IndexCreateTestCase(TestCase):
	"""
		Test Name: IndexCreateTest
	"""
	def setUp(self):
		self.directory_list = init_directory_list()
		self.logger = init_logger("indexCreateTestReport")

	def test_directoryValidAndImport(self):
		self.logger.info("Directory Valid And Import Test")
		for d in self.directory_list:
			result = plugins.directoryValidAndImport(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
			log_info = "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
			self.logger.info(log_info)
			'''
			if result["code"]==d["directory_return_code"]:
				print "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
			else:
				print "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
			'''
			self.assertEqual(result["code"], d["directory_return_code"])
			

	def test_directoryAnalysisProgram(self):
		# self.test_directoryValidAndImport()
		# directory_query_set = DirectoryInfo.objects.all()
		# print directory_query_set
		self.logger.info("Directory Analysis Program Test")
		for d in self.directory_list:
			result = plugins.directoryValidAndImport(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
			if result["code"]==0:
				result = plugins.directoryAnalysisProgram(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
				log_info = "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
				self.logger.info(log_info)
				'''
				print "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
				'''
				self.assertEqual(result["code"], d["directory_return_code"])

	def test_makeAlgorithmConfigFiles(self):
		self.logger.info("Algorithm Config Files make Test")
		for d in self.directory_list:
			result = plugins.directoryValidAndImport(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
			if result["code"]==0:
				result = plugins.directoryAnalysisProgram(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
				if result["code"]==0:
					result = plugins.makeAlgorithmConfigFiles()
					log_info = "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
					self.logger.info(log_info)
					'''
					print "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
					'''
					self.assertEqual(result["code"], d["directory_return_code"])

	def test_runIndexCreatePrograms(self):
		self.logger.info("Index Create Programs Test")
		for d in self.directory_list:
			result = plugins.directoryValidAndImport(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
			if result["code"]==0:
				result = plugins.directoryAnalysisProgram(os.path.join(TEST_DIRECTORY_PATH, d["directory_name"]))
				if result["code"]==0:
					result = plugins.makeAlgorithmConfigFiles()
					if result["code"]==0:
						result = plugins.runIndexCreateProgram()
						# log_info = "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
						# self.logger.info(log_info)
						'''
						print "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
						'''
						if result["code"]==0:
							config_query_set = AlgorithmConfigInfo.objects.all()
							total = len(config_query_set)
							finished = len(config_query_set.filter(config_flag=1))
							while finished<total:
								self.logger.info("index creating...")
								time.sleep(10)
								finished = AlgorithmConfigInfo.objects.filter(config_flag=1)
							else:
								log_info = "{} - {} - {}".format(result["code"], d["directory_return_code"], result["message"])
								self.logger.info(log_info)
						self.assertEqual(result["code"], d["directory_return_code"])


class IndexSearchTestCase(TestCase):
	"""
		Test Name: IndexSearchTest
	"""
	def setUp(self):
		# Every test need the search_string as input
		self.string_set_list = init_search_string_set_list()
		#self.string_set_list = SEARCH_STRING_SET_LIST
		self.logger = init_logger("indexSearchTestReport")

	def test_searchStringValid(self):
		self.logger.info("Index Search String Valid Test")
		for string_set in self.string_set_list:
			for s in string_set["set_data_list"]:
				result = plugins.searchStringValid(s)
				log_info = "{} - {} - {}".format(result["code"], string_set["set_return_code"], result["message"])
				self.logger.info(log_info)
				'''
				if result["code"]==string_set["set_return_code"]:
					print "{} - {} - {}".format(result["code"], string_set["set_return_code"], result["message"])
				else:
					print "Oh, No!!!"
				'''
				self.assertEqual(result["code"], string_set["set_return_code"])
				

	def test_runIndexSearchProgram(self):
		pass


class LoginTestCase(TestCase):
	"""
		Test Name: LoginTest
	"""
	def setUp(self):
		u_info = {"username":"admin", "password":"123456", "email":"admin@admin.com"}
		u = AuthUser.objects.create_user(username=u_info["username"], password=u_info["password"], email=u_info["email"])
		u.is_staff = True
		u.save()

	def test_UserLogin_Succeed(self):
		c = Client()
		input_list = [
			{"username":"admin", "password":"123456"},
		]
		response_list = [c.post('/login/',i) for i in input_list]
		result_list = [self.assertEqual(r.status_code, 302) for r in response_list]

	def test_UserLogin_Failed(self):
		c = Client()
		input_list = [
			{"username":"admin", "password":""},
			{"username":"admin", "password":"wrongpassword"},
			{"username":"", "password":"sdjafoisjafoi"},
			{"username":"", "password":""},
		]
		response_list = [c.post('/login/',i) for i in input_list]
		output_list = [self.assertEqual(r.status_code, 200) for r in response_list]

class DirectoryAnalysisTestCase(TestCase):
	"""
		Test Name: DirectoryAnalysisTest
	"""
	def setUp(self):
		pass

	def test_GetFileFromDirectory(self):
		input_1 = "/home/polly/test"
		output_1 = plugins.getFileFromDirectory(input_1)
		print output_1
		pass

	def test_ImportFileinfoToDatabase(self):
		pass

class FileAggregationDivisionTestCase(TestCase):
	"""
		Test Name: FileAggregationDivisionTest
	"""
	def setUp(self):
		pass

	def test_MakeConfigFileName(self):
		input_list = [
			r"/home/polly/test",
		]
		output_list = [plugins.makeConfigFileName(i) for i in input_list]
		pass

	def test_MakeConfigFile(self):
		pass

	def test_WriteConfigFilesToDisk(self):
		pass
