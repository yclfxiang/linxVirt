# coding=utf-8

from src.pages.login import LoginPage

class LoginCase(LoginPage):

    # 输入用户和密码
    def login(self, name, passwd):
        self.send_iput_user(name)
        self.sleep(1)
        self.send_iput_passwd(passwd)
        self.sleep(1)
        self.send_click_submit()