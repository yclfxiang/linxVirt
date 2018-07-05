# coding=utf-8

import time
import unittest


from selenium.webdriver.support.select import Select
from selenium.webdriver.common.action_chains import ActionChains
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.pages.vm import VmPage


class BaiduSearch(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """
        测试固件的setUp()的代码，主要是测试的前提准备工作
        :return:
        """
        browse = BrowserEngine(cls)
        cls.driver = browse.open_browser(cls)

    @classmethod
    def tearDownClass(cls):
        """
        测试结束后的操作，这里基本上都是关闭浏览器
        :return:
        """
        #cls.driver.quit()

    def test_vm(self):
        """
        这里一定要test开头，把测试逻辑代码封装到一个test开头的方法里。
        :return:
        """
        loginpage = LoginPage(self.driver)
        loginpage.send_iput_user('admin')  # 调用页面对象中的方法
        loginpage.sleep(1)
        loginpage.send_iput_passwd('123')  # 调用页面对象中的方法
        loginpage.sleep(1)
        loginpage.send_click_submit()  # 调用页面对象类中的点击搜索按钮方法
        loginpage.sleep(1)

        menupage = MenuPage(self.driver)
        menupage.send_click_vm()
        menupage.sleep(1)

        vmpage = VmPage(self.driver)
        self.driver.switch_to.frame("vm")
        vmpage.sleep(1)

        vmpage.select_table_row(vmpage.table, "ttt-1")
        vmpage.sleep(1)






        #loginpage.get_windows_img()  # 调用基类截图方法


if __name__ == '__main__':
    unittest.main()