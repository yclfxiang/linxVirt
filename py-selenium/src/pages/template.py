# coding=utf-8

from src.pages.base import BasePage


class TempPage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("template")
    edit = "id=>editTemplate"
    delete = "id=>removeTemplate"
    load = "id=>importTemplate"
    export = "id=>exportTemplate"

# temp info table
    table = "xpath=>//*[@id='iTemplateTable']"

# edit temp
    e_description = "name=>description"
    e_close = "xpath=>//*[@id='cancle']"
    e_submit = "xpath=>/html/body/form/div[2]/button[2]"
    e_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# delete temp
    d_close = "xpath=>//*[@id='cancle']"
    d_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    d_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# import temp
    ip_table = "xpath=>//*[@id='content-table']"
    ip_close = "xpath=>//*[@id='cancle']"
    ip_submit = "xpath=>//*[@id='importTemp']"
    ip_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# export temp
    ep_close = "xpath=>//*[@id='cancle']"
    ep_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    ep_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# to execute menu
    def send_click_edit(self):
        self.click(self.edit)

    def send_click_delete(self):
        self.click(self.delete)

    def send_click_import(self):
        self.click(self.load)

    def send_click_export(self):
        self.click(self.export)

# to edit temp
    def send_input_descriptionE(self, text):
        self.input(self.e_description, text)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to delete temp
    def send_click_closeD(self):
        self.click(self.d_close)

    def send_click_submitD(self):
        self.click(self.d_submit)

# to import temp
    def send_click_closeIP(self):
        self.click(self.ip_close)

    def send_click_submitIP(self):
        self.click(self.ip_submit)

# to export temp
    def send_click_closeEP(self):
        self.click(self.ep_close)

    def send_click_submitEP(self):
        self.click(self.ep_submit)

# to select temp from table
    def select_temp(self, temp):
         return self.select_table_row(self.table, temp)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)