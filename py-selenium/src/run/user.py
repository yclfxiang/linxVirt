# coding=utf-8

import time
import unittest

from src.browser.browser import BrowserEngine
from src.pages.menu import MenuPage

from src.tests.login import TestLogin
from src.tests.user import TestUser


login1 = ("", "")
login2 = ("", "rocky")
login3 = ("admin", "asa")
login4 = ("admin", "123")

newUser = {"user":"test123", "passwd":"123"}
deleteUser = {"user":"test123"}
updateUser = {"user":"linx", "passwd":"123"}
newGroup = {"group":"test123", "description":"123"}
deleteGroup = {"group":"test123"}
allocateVmUser = {"user":"lfxtest", "vm":"lfx-linx-1"}
removeVmUser = {"user":"lfxtest", "vm":"lfx-linx-1"}
allocateVmGroup = {"group":"lfxtest", "vm":"lfx-linx-1"}
removeVmGroup = {"group":"lfxtest", "vm":"lfx-linx-1"}
allocateUserGroup = {"group":"test", "user":"test"}
removeUserGroup = {"group":"test", "user":"test"}


class HostUser(unittest.TestCase):

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
        self.user = TestUser(self.driver)


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

    # 用户模块测试
    def test_login(self):
        self.logger.info("登录：账号、密码正确匹配")
        self.login.test_login4("admin", "rocky")
        self.login.sleep(1)
        self.menu.send_click_user()
        self.user.sleep(1)
        self.driver.switch_to.frame("user")

        self.user.test_allocate_vm_user(allocateVmUser["user"], allocateVmUser["vm"])
        self.user.sleep(2)
        self.user.test_remove_vm_user(removeVmUser["user"], removeVmUser["vm"])

if __name__ == '__main__':
    unittest.main()