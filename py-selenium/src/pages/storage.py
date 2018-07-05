# coding=utf-8

from src.pages.base import BasePage


class StoragePage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("storage")
    new = "id=>newStorage"
    edit = "id=>editStorage"
    delete = "id=>removeStorage"
    loadiso = "id=>putISO"
    maintenance = "id=>maintenance"
    attach = "id=>attach"
    active = "id=>active"
    detach = "id=>detach"

# storage info table
    table = "xpath=>//*[@id='iStorageTable']"

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# new create storage
    n_name = "name=>storageName"
    n_description = "name=>description"
    n_use = "xpath=>/html/body/form/div[1]/div[2]/div[2]/div/select"
    n_host = "xpath=>/html/body/form/div[1]/div[3]/div[1]/div/div"
    n_type = "xpath=>/html/body/form/div[1]/div[2]/div[2]/div/div"
    n_lun = "xpath=>//*[@id='selectFClun']"
    n_path = "xpath=>/html/body/form/div[1]/div[4]/div[2]/div/input"
    n_close = "id=>cancle"
    n_submit = "xpath=>/html/body/form/div[2]/button[2]"
    # new create storage select lun
    lun_table = "id=>lunTableId"
    s_close = "xpath=>//*[@id='cancle']"
    s_submit = "xpath=>//*[@id='lunselect']"

# edit storage
    e_name = "name=>domainName"
    e_description = "name=>description"
    e_close = "id=>cancle"
    e_submit = "xpath=>/html/body/form/div[2]/button[2]"
    e_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# delete storage
    d_clearStorage = "xpath=>/html/body/form/div[1]/div/div[2]/div/i"
    d_close = "id=>cancle"
    d_submit = "xpath=>/html/body/form/div[2]/button[2]"
    d_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# load iso
    file = "id=>fileField"
    push = "id=>upload"
    push_result = "xpath=>//*[@id='uploaderList']/li/div[2]/div[3]/span"
    push_close = "xpath=>/html/body/div[3]/span[1]"

# to execute menu
    def send_click_new(self):
        self.click(self.new)

    def send_click_edit(self):
        self.click(self.edit)

    def send_click_delete(self):
        self.click(self.delete)

    def send_click_loadiso(self):
        self.click(self.loadiso)

    def send_click_maintenance(self):
        self.click(self.maintenance)

    def send_click_attach(self):
        self.click(self.attach)

    def send_click_active(self):
        self.click(self.active)

    def send_click_detach(self):
        self.click(self.detach)

# to new create storage
    def send_input_nameN(self, text):
        self.input(self.n_name, text)

    def send_input_descriptionN(self, text):
        self.input(self.n_description, text)

    def send_select_typeN(self, option):
        self.click_select_option(self.n_type,option)

    def send_select_hostN(self, option):
        self.click_select_option(self.n_host,option)

    def send_input_pathN(self, text):
        self.input(self.n_description, text)

    def send_click_lunN(self):
        self.click(self.n_lun)

    def send_select_lunN(self, lun):
        return self.select_table_row(self.n_lun, lun)

    def send_click_closeN(self):
        self.click(self.n_close)

    def send_click_submitN(self):
        self.click(self.n_submit)

    # new create storage select lun
    def send_click_closeS(self):
        self.click(self.s_close)

    def send_click_submitS(self):
        self.click(self.s_submit)

# to edit storage
    def send_input_nameE(self, text):
        self.input(self.e_name, text)

    def send_input_descriptionE(self, text):
        self.input(self.e_description, text)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to upload iso
    def send_input_file(self, iso):
        self.input(self.file, iso)

    def send_click_push(self):
        self.click(self.push)

    def send_click_closeP(self):
        self.click(self.push_close)

# to delete storage
    def send_click_clearstorageD(self):
        self.click(self.d_clearStorage)

    def send_click_closeD(self):
        self.click(self.d_close)

    def send_click_submitD(self):
        self.click(self.d_submit)

# to select storage from table
    def select_storage(self, storage):
        return self.select_table_row(self.table, storage)

# get storage status from table
    def get_storage_status(self, storage):
        return self.get_table_unit_status(self.table, storage)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)