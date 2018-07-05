# coding=utf-8

import time
import unittest

from src.browser.browser import BrowserEngine
from src.pages.menu import MenuPage

from src.tests.login import TestLogin
from src.tests.disk import TestDisk


login1 = ("", "")
login2 = ("", "rocky")
login3 = ("admin", "asa")
login4 = ("admin", "rocky")

newDisk = {"name":"test223", "description":"test223", "size":"30"}
editDisk = {"disk":"test223", "description":"223"}
deleteDisk = {"disk":"test223", "cleardisk":"1"}


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
        self.disk = TestDisk(self.driver)


    @classmethod
    def tearDownClass(self):
        """
        测试结束后的操作
        :return:
        """
        self.driver.quit()


    # #登录
    # def test_login(self):
    #     self.logger.info("登录：账号、密码正确匹配")
    #     self.login.test_login4()
    #     self.login.sleep(1)

    # 磁盘模块测试
    def test_login(self):
        self.logger.info("登录：账号、密码正确匹配")
        self.login.test_login4()
        self.login.sleep(1)
        self.menu.send_click_disk()
        self.disk.sleep(1)
        self.driver.switch_to.frame("disk")

        self.logger.info("测试编辑主机")
        self.disk.test_new_disk(newDisk["name"], newDisk["description"], newDisk["size"])
        self.disk.sleep(1)
        self.disk.test_edit_disk(editDisk["disk"], editDisk["description"])
        self.disk.sleep(1)
        self.disk.test_delete_disk(deleteDisk["disk"], deleteDisk["cleardisk"])



if __name__ == '__main__':
    unittest.main()