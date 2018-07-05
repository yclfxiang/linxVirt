# coding=utf-8

from src.pages.base import BasePage


class LoginPage(BasePage):

    user = "name=>username"
    passwd = "name=>password"
    login_bun = "id=>loginUser"
    info = "xpath=>//*[@id='promptBox']"
    passwd_error = "xpath=>//*[@id='layui-layer1']/div[2]"

    # execute result submit
    r_submit = "class_name=>layui-layer-btn0"

    def send_iput_user(self, text):
        self.input(self.user, text)

    def send_iput_passwd(self, text):
        self.input(self.passwd, text)

    def send_click_submit(self):
        self.click(self.login_bun)

    def send_click_submitR(self):
        self.click(self.r_submit)
