# coding=utf-8

import time
import unittest

from src.browser.browser import BrowserEngine
from src.pages.menu import MenuPage

from src.tests.login import TestLogin
from src.tests.host import TestHost
from src.tests.disk import TestDisk
from src.tests.template import TestTemp
from src.tests.user import TestUser
from src.tests.vm import TestVm


class LinxEngine(unittest.TestCase):

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
        self.disk = TestDisk(self.driver)
        self.temp = TestTemp(self.driver)
        self.user = TestUser(self.driver)
        self.vm = TestVm(self.driver)


    @classmethod
    def tearDownClass(self):
        """
        测试结束后的操作
        :return:
        """
        # self.driver.quit()

    #登录测试
    def test_login(self):
        # self.logger.info("登录测试1：账号、密码均为空")
        # self.login.test_login1()
        # self.login.sleep(1)
        # self.logger.info("登录测试2：账号或密码为空")
        # self.login.test_login2()
        # self.login.sleep(1)
        # self.logger.info("登录测试3：账号或密码错误")
        # self.login.test_login3()
        # self.login.sleep(1)
        self.logger.info("登录测试4：账号、密码正确匹配")
        self.login.test_login4("admin", "rocky")
        self.login.sleep(1)

    #主机模块测试
    def test_host(self):
        self.menu.send_click_host()
        self.host.sleep(2)
        self.driver.switch_to.frame("host")

        # self.logger.info("测试部署主机")
        # self.host.sleep(1)
        # self.host.test_deploy_host()
        # self.host.sleep(2)
        #
        # self.host.sleep(15)
        #
        self.logger.info("测试新建主机")
        self.host.sleep(1)
        self.host.test_new_host()
        self.host.sleep(2)

        self.logger.info("***************测试编辑主机***************")
        self.host.test_edit_host()
        self.host.sleep(2)

        self.logger.info("********** 测试维护主机 **********")
        self.host.test_maintenance_host()
        self.host.sleep(2)

        self.logger.info("测试激活主机")
        self.host.active_host()
        self.host.sleep(2)

        self.logger.info("测试删除主机")
        self.host.test_delete_host()
        self.host.sleep(2)

    # 磁盘模块测试
    def test_disk(self):
        self.menu.send_click_disk()
        self.host.sleep(2)
        self.driver.switch_to.frame("disk")

        self.logger.info("测试新建磁盘")
        self.disk.sleep(1)
        self.disk.test_new_disk()
        self.disk.sleep(2)

        self.logger.info("测试编辑磁盘")
        self.disk.sleep(1)
        self.disk.test_edit_disk()
        self.disk.sleep(2)

        self.logger.info("测试删除磁盘")
        self.disk.sleep(1)
        self.disk.test_delete_disk()
        self.disk.sleep(2)

    # 模板模块测试
    def test_temp(self):
        self.menu.send_click_temp()
        self.host.sleep(2)
        self.driver.switch_to.frame("template")

        self.logger.info("测试编辑模板")
        self.temp.sleep(1)
        self.temp.test_edit_temp()
        self.temp.sleep(2)

        self.logger.info("测试删除模板")
        self.temp.sleep(1)
        self.temp.test_delete_temp()
        self.temp.sleep(2)

    # 用户模块测试
    def test_user(self):
        self.menu.send_click_user()
        self.host.sleep(1)
        self.driver.switch_to.frame("user")

        # self.logger.info("测试新建用户")
        # self.user.test_new_user()
        # self.host.sleep(2)
        #
        # self.logger.info("测试删除用户")
        # self.user.test_delete_user()
        # self.host.sleep(2)
        #
        # self.logger.info("测试修改用户密码")
        # self.user.test_update_user_passwd()
        # self.host.sleep(2)
        #
        # self.logger.info("测试新建用户组")
        # self.user.test_new_group()
        # self.host.sleep(2)
        #
        # self.logger.info("测试删除用户组")
        # self.user.test_delete_group()
        # self.host.sleep(2)

        # self.user.test_allocate_vm_user()
        # self.user.sleep(2)
        self.user.test_remove_vm_user()

        # self.user.test_allocate_vm_group()
        # self.user.sleep(2)
        # self.user.test_remove_vm_group()
        #
        # self.user.test_allocate_user_group()
        # self.user.sleep(2)
        # self.user.test_remove_user_group()

    # 虚拟机模块测试
    def test_vm(self):
        self.menu.send_click_vm()
        self.vm.sleep(1)
        self.driver.switch_to.frame("vm")

        self.logger.info("测试新建用户")
        # self.vm.test_new_vm_attach_disk()
        # self.vm.sleep(1)
        # self.vm.test_new_vm_new_disk()
        # self.vm.sleep(1)
        # self.vm.test_new_vm_with_temp()
        # self.vm.test_import_vm()
        # self.vm.test_delete_vm()
        # self.vm.test_runonce_vm()
        # self.vm.test_new_snap()
        self.vm.test_restore_snap()
        # self.vm.test_delete_snap()

if __name__ == '__main__':
    unittest.main()