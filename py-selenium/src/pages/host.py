# coding=utf-8

from src.pages.base import BasePage


class HostPage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("host")
    deploy = "id=>deploymentHost"
    new = "id=>newHost"
    edit = "id=>editHost"
    delete = "id=>removeHost"
    maintenance = "id=>maintenance"
    restart = "id=>restart"
    active = "id=>start"
    stop = "id=>shutdown"
    spm = "id=>setSPM"

# host info table
    table = "xpath=>//*[@id='iHostTable']"

# deploy host
    d_ip = "name=>ip"
    d_gateway = "name=>network"
    d_passwd = "name=>passwd"
    d_ipNew = "name=newIp"
    d_gatewayNew = "name=newNetwork"
    d_close = "id=>cancle"
    d_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    d_result = "xpath=>//*[@id='layui-layer1']/div[2]/text()"

# new create host
    n_name = "name=>name"
    n_description = "name=>description"
    n_ip = "name=>ip"
    n_passwd = "name=>passwd"
    n_spm = "/html/body/form/div[1]/div[4]/div/div"
    n_close = "id=>cancle"
    n_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    n_result = 'xpath=>//*[@id="layui-layer1"]/div[2]'

# edit host
    e_name = "name=>name"
    e_description = "name=>description"
    e_spm = "/html/body/form/div[1]/div[4]/div/div"
    e_close = "id=>cancle"
    e_submit = "xpath=>/html/body/form/div[2]/button[2]"
    e_result = 'xpath=>//*[@id="layui-layer1"]/div[2]'

# delete host
    de_close = "id=>cancle"
    de_submit = 'xpath=>//*[@id="deleteHost"]'
    de_result = 'xpath=>//*[@id="layui-layer1"]/div[2]'

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# to execute menu
    def send_click_deploy(self):
        self.click(self.deploy)

    def send_click_new(self):
        self.click(self.new)

    def send_click_edit(self):
        self.click(self.edit)

    def send_click_delete(self):
        self.click(self.delete)

    def send_click_maintenance(self):
        self.click(self.maintenance)

    def send_click_restart(self):
        self.click(self.restart)

    def send_click_active(self):
        self.click(self.active)

    def send_click_stop(self):
        self.click(self.stop)

    def send_click_spm(self):
        self.click(self.spm)

# to deploy host
    def send_input_ipD(self, text):
        self.input(self.d_ip, text)

    def send_input_gatewayD(self, text):
        self.input(self.d_gateway, text)

    def send_input_passwdD(self, text):
        self.input(self.d_passwd, text)

    def send_input_ipnewD(self, text):
        self.input(self.d_ipNew, text)

    def send_input_gatewaynewD(self, text):
        self.input(self.d_gatewayNew, text)

    def send_click_closeD(self):
        self.click(self.d_close)

    def send_click_submitD(self):
        self.click(self.d_submit)

# to new host
    def send_input_nameN(self, text):
        self.input(self.n_name, text)

    def send_input_descriptionN(self, text):
        self.input(self.n_description, text)

    def send_input_ipN(self, text):
        self.input(self.n_ip, text)

    def send_select_spmN(self, option):
        self.click_select_option(self.n_spm, option)

    def send_input_passwdN(self, text):
        self.input(self.n_passwd, text)

    def send_click_closeN(self):
        self.click(self.n_close)

    def send_click_submitN(self):
        self.click(self.n_submit)

# to edit host
    def send_input_nameE(self, text):
        self.input(self.e_name, text)

    def send_input_descriptionE(self, text):
        self.input(self.e_description, text)

    def send_select_spmE(self, option):
        self.click_select_option(self.e_spm, option)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to delete host
    def send_click_closeDE(self):
        self.click(self.de_close)

    def send_click_submitDE(self):
        self.click(self.de_submit)

# to select host from table
    def select_host(self, host):
        return self.select_table_row(self.table, host)

# get host status from table
    def get_host_status(self, host):
        return self.get_table_unit_status(self.table, host)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)
