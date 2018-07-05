# coding=utf-8

from src.pages.base import BasePage


class DiskPage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("disk")
    new = "id=>newDisk"
    edit = "id=>editDisk"
    delete = "id=>removeDisk"

# disk info table
    table = "xpath=>//*[@id='iDiskTable']/tbodyea"

# new create disk
    n_name = "name=>diskAlias"
    n_description = "name=>description"
    n_size = "name=>size"
    n_domain = "xpath=>//*[@id='selectID']"
    n_type = "xpath=>/html/body/form/div[1]/div[6]/div/select"
    n_close = "xpath=>//*[@id='cancle']"
    n_submit = "xpath=>/html/body/form/div[2]/button[2]"
    n_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# edit disk
    e_description = "name=>description"
    e_close = "xpath=>//*[@id='cancle']"
    e_submit = "xpath=>/html/body/form/div[2]/button[2]"
    e_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# delete disk
    d_clear = "xpath=>/html/body/form/div[1]/div/div[2]/div"
    d_close = "xpath=>//*[@id='cancle']"
    d_submit = "xpath=>//*[@id='deleteDisk']"
    d_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# to execute menu
    def send_click_new(self):
        self.click(self.new)

    def send_click_edit(self):
        self.click(self.edit)

    def send_click_delete(self):
        self.click(self.delete)

# to new create disk
    def send_input_nameN(self, text):
        self.input(self.n_name, text)

    def send_input_descriptionN(self, text):
        self.input(self.n_description, text)

    def send_input_sizeN(self, text):
        self.input(self.n_size, text)

    def send_select_domainN(self, text):
        self.select(self.n_domain, text)

    def send_select_typeN(self, text):
        self.select(self.n_type, text)

    def send_click_closeN(self):
        self.click(self.n_close)

    def send_click_submitN(self):
        self.click(self.n_submit)

# to edit disk
    def send_input_descriptionE(self, text):
        self.input(self.e_description, text)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to delete disk
    def send_click_clearD(self):
        self.click(self.d_clear)

    def send_click_closeD(self):
        self.click(self.d_close)

    def send_click_submitD(self):
        self.click(self.d_submit)

# to select disk from table
    def select_disk(self, disk):
        return self.select_table_row(self.table, disk)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)