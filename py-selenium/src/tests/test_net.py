# coding=utf-8

import time
import unittest
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.pages.network import NetworkPage

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

    def test_host(self):
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
        menupage.send_click_network()
        menupage.sleep(1)

        networkpage = NetworkPage(self.driver)
        self.driver.switch_to.frame("network")
        networkpage.sleep(1)
        networkpage.select_table_row(networkpage.table, "Node1")
        networkpage.sleep(1)
        networkpage.send_click_new()
        networkpage.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        networkpage.send_select_eth0N()
        networkpage.sleep(1)
        networkpage.send_select_eth1N()
        networkpage.sleep(1)
        networkpage.send_click_closeN()