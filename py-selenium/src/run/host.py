# coding=utf-8

import time
import unittest

from src.browser.browser import BrowserEngine
from src.pages.menu import MenuPage

from src.tests.login import TestLogin
from src.tests.host import TestHost

login1 = ("", "")
login2 = ("", "rocky")
login3 = ("admin", "asa")
login4 = ("admin", "rocky")

deployHost = {"ip":"192.168.22.61", "gateway":"192.168.22.254", "passwd":"root", "n_ip":"", "n_gateway":""}
newHost = {"name":"node182", "description":"182", "ip":"192.168.22.182", "spm":"中", "passwd":"root"}
editHost = {"host":"node1", "name":"Node1", "description":"61", "spm":"中"}
maintenanceHost = {"host":"Node"}
activeHost = {"host":"Node"}
deleteHost = {"host":"Node"}


class HostTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        """
        测试固件的setUp()的代码，主要是测试的前提准备工作
        :return:
        """
        browse = BrowserEngine(self)
        self.driver = browse.open_browser(self)
        self.logger = BrowserEngine.logger

        self.login = TestLogin(self.driver)
        self.menu = MenuPage(self.driver)
        self.host = TestHost(self.driver)


    @classmethod
    def tearDownClass(self):
        """
        测试结束后的操作
        :return:
        """
        # self.driver.quit()


    # 主机模块测试
    def test_login(self):
        self.logger.info("登录：账号、密码正确匹配")
        self.login.test_login4()
        self.login.sleep(1)
        self.menu.send_click_host()
        self.host.sleep(1)
        self.driver.switch_to.frame("host")

        self.logger.info("测试编辑主机")
        self.host.test_edit_host(editHost["host"], editHost["name"], editHost["description"], editHost["spm"])



if __name__ == '__main__':
    unittest.main()