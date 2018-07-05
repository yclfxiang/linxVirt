# -*- coding:utf-8 -*-

import configparser
import os.path
from selenium import webdriver
from src.log.logger import Logger

waittime = 300


class BrowserEngine(object):

    logger = Logger(logger="BrowserEngine").getlog()

    def __init__(self, driver):
        self.driver = driver

    # 读取 config.ini 配置文件, 打开并获取浏览器操作 driver
    def open_browser(self, driver):
        config = configparser.ConfigParser()
        file_path = os.path.dirname(os.path.abspath('.'))
        file_path = os.path.dirname(file_path)
        file_path = file_path + '/config/config.ini'
        config.read(file_path)

        browser = config.get("browser", "browser")
        self.logger.info("选择使用 %s 浏览器" % browser)
        url = config.get("url", "url")
        self.logger.info("测试的url地址: %s" % url)

        if browser == "Firefox":
            driver = webdriver.Firefox()
            self.logger.info("打开 firefox 浏览器")
        elif browser == "Chrome":
            driver = webdriver.Chrome()
            self.logger.info("打开 Chrome 浏览器")
        elif browser == "IE":
            driver = webdriver.Ie()
            self.logger.info("打开 IE 浏览器")

        driver.get(url)
        self.logger.info("打开 url 地址: %s" % url)
        driver.maximize_window()
        self.logger.info("最大化浏览器窗口")
        driver.implicitly_wait(waittime)
        self.logger.info("设置查找页面元素的隐式等待时间: %s (秒)" % waittime)
        return driver

    def quit_browser(self):
        self.logger.info("关闭并退出浏览器")
        self.driver.quit()