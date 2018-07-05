# coding=utf-8

import time
import unittest
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.pages.template import TempPage

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

    def test_temp(self):
        """
        这里一定要test开头，把测试逻辑代码封装到一个test开头的方法里。
        :return:
        """
        loginpage = LoginPage(self.driver)
        loginpage.iput_user('admin')  # 调用页面对象中的方法
        loginpage.sleep(1)
        loginpage.iput_passwd('rocky')  # 调用页面对象中的方法
        loginpage.sleep(1)
        loginpage.send_submit_btn()  # 调用页面对象类中的点击搜索按钮方法
        loginpage.sleep(1)

        menupage = MenuPage(self.driver)
        menupage.send_click_temp()
        menupage.sleep(1)

        temppage = TempPage(self.driver)
        self.driver.switch_to.frame("template")
        temppage.sleep(1)

        temppage.send_select_temp1()
        temppage.sleep(1)
        #temppage.send_click_edit()
        #temppage.send_click_delete()
        temppage.send_click_import()
        temppage.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        temppage.sleep(1)

        temppage.send_select_domainIP("FCP2(192G/197G)")

        # temppage.send_input_descriptionE("linx")
        # temppage.sleep(2)
        # temppage.send_click_submitE()
        # temppage.sleep(1)
        # temppage.send_click_submitR()

        # temppage.send_click_submitD()
        # temppage.sleep(2)
        # temppage.send_click_submitR()

        # temppage.send_click_typeEP()
        # temppage.sleep(3)
        # temppage.send_click_closeEP()


        #loginpage.get_windows_img()  # 调用基类截图方法



if __name__ == '__main__':
    unittest.main()