# coding=utf-8

import time
from selenium.common.exceptions import NoSuchElementException
import os.path
from selenium.webdriver.support.select import Select
from src.browser.browser import BrowserEngine

class BasePage(object):
    """
    定义一个页面基类，让所有页面都继承这个类，封装一些常用的页面操作方法
    """

    def __init__(self, driver):
        self.driver = driver
        self.logger = BrowserEngine.logger

    # quit browser and end testing
    def quit_browser(self):
        self.driver.quit()

    # 浏览器前进操作
    def forward(self):
        self.logger.info("转到下一页")
        self.driver.forward()

    # 浏览器后退操作
    def back(self):
        self.logger.info("转到上一页")
        self.driver.back()

    # 点击关闭当前窗口
    def close(self):
        try:
            self.logger.info("关闭当前页面")
            self.driver.close()
        except NameError as e:
            self.logger.info("关闭当前页面失败： %s" % e)
            self.get_windows_img()

    # 保存图片
    def get_windows_img(self):
        """
        保存到项目根目录的一个文件夹screenshots下
        """
        file_path = os.path.dirname(os.path.abspath('.'))
        file_path = os.path.dirname(file_path)
        file_path = file_path + '/screenshots/'
        if not os.path.exists(file_path):
            os.makedirs(file_path)
        rq = time.strftime('%Y%m%d%H%M', time.localtime(time.time()))
        screen_name = file_path + rq + '.png'

        try:
            self.driver.get_screenshot_as_file(screen_name)
            self.logger.info("保存页面截图到: /screenshots")
        except NameError as e:
            self.logger.info("截取页面失败 %s" % e)
            self.get_windows_img()

    # 定位元素方法
    def find_element(self, selector):
        """
        :param selector:
        :return: element
        """
        element = ''
        if '=>' not in selector:
            return self.driver.find_element_by_id(selector)
        selector_by = selector.split('=>')[0]
        selector_value = selector.split('=>')[1]

        if selector_by == "i" or selector_by == 'id':
            try:
                element = self.driver.find_element_by_id(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "n" or selector_by == 'name':
            try:
                element = self.driver.find_element_by_name(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "c" or selector_by == 'class_name':
            try:
                element = self.driver.find_element_by_class_name(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "l" or selector_by == 'link_text':
            try:
                element = self.driver.find_element_by_link_text(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "p" or selector_by == 'partial_link_text':
            try:
                element = self.driver.find_element_by_partial_link_text(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "t" or selector_by == 'tag_name':
            try:
                element = self.driver.find_element_by_tag_name(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "x" or selector_by == 'xpath':
            try:
                element = self.driver.find_element_by_xpath(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        elif selector_by == "s" or selector_by == 'selector_selector':
            try:
                element = self.driver.find_element_by_css_selector(selector_value)
                self.logger.info("成功查找到元素 \' %s \'，"
                                 "by %s value: %s " % (element.text, selector_by, selector_value))
            except NoSuchElementException as e:
                self.logger.error("查找元素失败: %s" % e)
                self.get_windows_img()

        else:
            self.logger.info("无效的查询方式！")
            raise NameError("请指定一个有效的查询方式！")

        return element

    # 输入
    def input(self, selector, text):
        el = self.find_element(selector)
        el.clear()
        try:
            self.logger.info("在文本框输入信息： \' %s \' " % text)
            el.send_keys(text)
        except NameError as e:
            self.logger.error("输入失败： %s" % e)
            self.get_windows_img()

    # 清除文本框
    def clear(self, selector):
        el = self.find_element(selector)
        try:
            self.logger.info("清除文本框中数据！")
            el.clear()
        except NameError as e:
            self.logger.error("清除文本框中数据失败： %s" % e)
            self.get_windows_img()

    # 点击元素
    def click(self, selector):
        el = self.find_element(selector)
        self.sleep(1)
        try:
            self.logger.info("点击元素 \' %s \' " % el.text)
            el.click()
        except NameError as e:
            self.logger.error("点击元素失败： %s" % e)

    # 选择select元素
    def select(self, selector, option):
        el = self.find_element(selector)
        try:
            Select(el).select_by_visible_text(option)
            self.logger.info("选择了 \' %s \' 的 \' %s \' 项." % el.text, option)
        except NameError as e:
            self.logger.error("选择\' %s \选项失败 %s" % option, e)

    # 选择checkbox、radio元素
    def check(self, selector):
        el = self.find_element(selector)
        try:
            self.logger.info("选择了 \' %s \' 项." % el.text)
            if el.is_selected():
                print ("*********************************************")
                return
            else:
                el.click()
        except NameError as e:
            self.logger.error("选择选项失败 %s" % e)

    # 点击表格指定行
    def select_table_row(self, selector, option):
        table = self.find_element(selector)
        rows = table.find_elements_by_tag_name("tr")
        for r in rows:
            els = r.find_elements_by_tag_name("td")
            if option == els[1].text:
                self.sleep(1)
                r.click()
                return 1
        return 0

    # 选择select指定项(使用dl标签option隐藏型select)
    def click_select_option(self, selector, option):
        select = self.driver.find_element_by_xpath(selector)
        select.click()
        self.sleep(1)
        dd = '%s%s%s%s%s%s' % (selector, "/dl/dd[text()=", '\"', option, '\"', "]")
        dl = self.driver.find_element_by_xpath(dd)
        dl.click()

    # 获取元素文本内容
    def get_element_text(self, selector):
        el = self.find_element(selector)
        self.sleep(1)
        return el.text

    # 获取表格指定元素的状态(适用于状态栏在第三列的table：主机、存储、虚拟机)
    def get_table_unit_status(self, selector, option):
        table = self.find_element(selector)
        rows = table.find_elements_by_tag_name("tr")
        for r in rows:
            els = r.find_elements_by_tag_name("td")
            if option == els[1].text:
                return els[3].text
        return ""


    # 获取网页标题
    def get_page_title(self):
        self.logger.info("当前页面的标题是： %s" % self.driver.title)
        return self.driver.title

    def sleep(self, seconds):
        time.sleep(seconds)
        self.logger.info("Sleep for %d seconds" % seconds)