# coding=utf-8

import time
import unittest
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.tests.host import TestHost


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
        loginpage.sleep(2)

        menupage = MenuPage(self.driver)
        menupage.send_click_host()
        menupage.sleep(1)
        self.driver.switch_to.frame("host")
        menupage.sleep(1)
        host = TestHost(self.driver)
        menupage.sleep(1)
        host.select_table_row(host.table, "node1")
        menupage.sleep(1)
        host.send_click_edit()
        menupage.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        menupage.sleep(1)

        menupage.click_select_option("/html/body/form/div[1]/div[4]/div/div", "低")

        # self.driver.find_element_by_xpath('/html/body/form/div[1]/div[4]/div//div').click()
        # menupage.sleep(1)
        # dl = self.driver.find_element_by_xpath("/html/body/form/div[1]/div[4]/div/div/dl/dd[text()='高']")
        # dl.click()
        # menupage.sleep(1)


if __name__ == '__main__':
    unittest.main()