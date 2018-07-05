# coding=utf-8

from src.browser.browser import BrowserEngine
from src.usecases.login import LoginCase


logger = BrowserEngine.logger


class TestLogin(LoginCase):

    # 测试不同登录情况
    def test_login1(self, user, passwd):
        self.login(user, passwd)
        self.sleep(1)
        result = self.get_element_text(self.info)
        if "请输入用户名" in result:
            logger.info("****** 登录测试通过(用户名与密码均输入空)！ ******")
        else:
            logger.info("****** 登录测试失败(用户名与密码均输入空)！ ******")

    def test_login2(self, user, passwd):
        self.login(user, passwd)
        self.sleep(1)
        result = self.get_element_text(self.info)
        if "请输入用户名" in result:
            logger.info("****** 登录测试通过(用户名或密码输入空)！ ******")
        else:
            logger.info("****** 登录测试失败(用户名或密码输入空)！ ******")

    def test_login3(self, user, passwd):
        self.login(user, passwd)
        self.sleep(1)
        result = self.get_element_text(self.info)
        if "用户名/密码错误" in result:
            logger.info("****** 登录测试通过(用户名或密码输入错误)！ ******")
        else:
            logger.info("****** 登录测试失败(用户名或密码输入错误)！ ******")
        self.sleep(1)
        self.send_click_submitR()

    def test_login4(self, user, passwd):
        self.login(user, passwd)