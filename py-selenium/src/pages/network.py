# coding=utf-8

from src.pages.base import BasePage


class NetworkPage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("network")
    new = "xpath=>//*[@id='setNetwork']"
    edit = "xpath=>//*[@id='modifyNetwork']"

# storage info table
    table = "xpath=>//*[@id='inetworkTable']"

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# new create interface bond
    n_name = "name=>bond"
    n_type = "name=>mode"
    n_eth0 = 'xpath=>//*[@id="netInterfacesId"]/div[1]/i'
    n_eth1 = 'xpath=>//*[@id="netInterfacesId"]/input[2]'
    n_close = "xpath=>//*[@id='cancle']"
    n_submit = "xpath=>/html/body/form/div[2]/button[2]"
    n_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# edit interface bond
    e_eth0 = 'xpath=>//*[@id="netInterfacesId"]/div[1]/i'
    e_eth1 = 'xpath=>//*[@id="netInterfacesId"]/div[2]/i'
    e_close = "xpath=>//*[@id='cancle']"
    e_submit = "xpath=>/html/body/form/div[2]/button[2]"
    e_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# to execute menu
    def send_click_new(self):
        self.click(self.new)

    def send_click_edit(self):
        self.click(self.edit)

# to new create interface bond
    def send_select_eth0N(self):
        self.check(self.n_eth0)

    def send_select_eth1N(self):
        self.check(self.n_eth1)

    def send_click_closeN(self):
        self.click(self.n_close)

    def send_click_submitN(self):
        self.click(self.n_submit)

# to edit interface bond
    def send_select_eth0E(self):
        self.check(self.e_eth0)

    def send_select_eth1E(self):
        self.check(self.e_eth1)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to select host from table
    def select_host(self, host):
        return self.select_table_row(self.table, host)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)