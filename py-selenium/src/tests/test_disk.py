# coding=utf-8

import time
import unittest
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.pages.disk import DiskPage

from selenium.webdriver.support import expected_conditions as EC


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

    def test_disk(self):
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
        menupage.send_click_disk()
        menupage.sleep(1)

        diskpage = DiskPage(self.driver)
        self.driver.switch_to.frame("disk")
        diskpage.sleep(1)

        # table = self.driver.find_element_by_xpath("//*[@id='iDiskTable']/tbody")
        # rows = table.find_elements_by_tag_name("tr")
        #
        # print (table.text)
        # print ("******* len = %d *********" % len(rows))
        #
        flag  = diskpage.select_table_row(diskpage.table, "aa")

        # result = EC.text_to_be_present_in_element(("xpath", "//*[@id='iDiskTable']/tbody"), "hhsi8991")(self.driver)

        print ("************** flag == %d ***************" % flag)


        # loginpage.get_windows_img()  # 调用基类截图方法



if __name__ == '__main__':
    unittest.main()