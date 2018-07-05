# coding=utf-8

import time
import unittest
from src.browser.browser import BrowserEngine
from src.pages.login import LoginPage
from src.pages.menu import MenuPage
from src.pages.storage import StoragePage

from selenium.webdriver.support.select import Select

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

    def test_storage(self):
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
        menupage.send_click_storage()
        menupage.sleep(1)

        storagepage = StoragePage(self.driver)
        self.driver.switch_to.frame("storage")
        storagepage.sleep(1)

        # storagepage.send_click_new()
        # self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        # storagepage.sleep(1)
        # #self.driver.find_element_by_xpath("/html/body/form/div[1]/div[2]/div[2]/div").click()
        # storagepage.sleep(1)
        # js = "var options=document.getElementsByName('domainFunc').children; options[1].selected=false;options[2].selected=true;"
        # self.driver.execute_script(js)
        # storagepage.sleep(1)
        # # sl = Select(self.driver.find_element_by_xpath("/html/body/form/div[1]/div[2]/div[2]/div/select"))
        # # storagepage.sleep(2)
        # #sl.deselect_by_index(0)
        # #storagepage.sleep(2)
        # #self.driver.find_element_by_xpath("/html/body/form/div[1]/div[2]/div[2]/div/div/dl/dd[3]").click()
        # #sl.select_by_index(2)

        storagepage.select_table_row(storagepage.table, "镜像域")
        storagepage.sleep(1)
        storagepage.send_click_loadiso()
        storagepage.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        storagepage.sleep(1)
        path = self.driver.find_element_by_id("fileField")
        storagepage.sleep(1)
        path.send_keys("/home/lfx/iso/mini.iso")
        storagepage.sleep(1)
        # self.driver.switch_to.frame("storage")
        storagepage.sleep(1)
        close = self.driver.find_element_by_("span")
        close.click()

        # push = self.driver.find_element_by_id("upload")
        # storagepage.sleep(1)
        # push.click()
        # storagepage.sleep(1)
        # text = storagepage.get_element_text(storagepage.push_result)
        # print ("****** text = %s ******" % text)

        #loginpage.get_windows_img()  # 调用基类截图方法



if __name__ == '__main__':
    unittest.main()