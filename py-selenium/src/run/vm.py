# coding=utf-8

import time
import unittest

from src.browser.browser import BrowserEngine
from src.pages.menu import MenuPage

from src.tests.login import TestLogin
from src.tests.vm import TestVm

login1 = ("", "")
login2 = ("", "rocky")
login3 = ("admin", "asa")
login4 = ("admin", "123")

newVmAttachDisk = {"name":"nod", "description":"12", "memsize":"500", "maxszie":"2000", "cpu":"1", "disk":"linx"}
newVmNewDisk = {"vmname":"www", "vmdescription":"12", "memsize":"500",
                "maxszie":"2000", "cpu":"1", "diskname":"www-disk", "diskdescription":"12", "size":"30"}
newVmTemp = {"number":"2", "name":"mmm", "description":"12", "memsize":"500", "maxszie":"2000", "cpu":"1"}
importVm = {"vm":"mmm-1"}
editVm = {"vm":"", "name":"", "mac":"", "description":"", "memsize":"", "maxsize":"", "cpu":"", "kcpu":"", "tcpu":""}
deleteVm = {"vm":"www", "deldisk":"0", "cleardisk":"0"}
runonceVm = {"vm":"test", "boot":"PXE", "iso":""}
startVm = {"vm":"test"}
pauseVm = {"vm":"test"}
restartVm = {"vm":"test"}
stopVm = {"vm":"test"}
migrateVm = {"vm":"test", "bandwidth":"500"}
exportVm = {"vm":"mmm-1"}
turnVmTemp = {"vm":"", "name":"", "description":""}
changeCd = {"vm":"", "iso":""}
newSnap = {"vm":"test", "name":"snap1", "description":"123"}
restoreSnap = {"vm":"test", "snap":"1"}
deleteSnap = {"vm":"node-1", "snap":"1"}

class VmTest(unittest.TestCase):

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
        self.vm = TestVm(self.driver)


    @classmethod
    def tearDownClass(self):
        """
        测试结束后的操作
        :return:
        """
        # self.driver.quit()


    #登录
    def test_login(self):
        self.logger.info("登录：账号、密码正确匹配")
        self.login.test_login4(login4[0], login4[1])
        self.login.sleep(1)

    # 虚拟机模块测试
    def new(self):
        self.menu.send_click_vm()
        self.vm.sleep(1)
        self.driver.switch_to.frame("vm")

        self.logger.info("测试新建用户")


if __name__ == '__main__':
    unittest.main()