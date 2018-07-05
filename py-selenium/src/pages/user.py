# coding=utf-8

from src.pages.base import BasePage
from selenium.webdriver.common.action_chains import ActionChains

class UserPage(BasePage):
    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("user")
    user = "id=>normalUser"
    group = "id=>userGroup"

    newUser = "id=>newUser"
    deleteUser = "id=>removeUser"
    newGroup = "id=>newGroup"
    deleteGroup = "id=>removeGroup"
    updatePasswd = "id=>changePassword"

    addvm = "id=>addVm"
    removevm = "id=>removeVm"
    adduser = "id=>addUser"
    removeuser = "id=>removeUser"

# user info table
    table = 'xpath=>//*[@id="iUserTable"]/tbody'

# add new user
    nu_name = "name=>username"
    nu_passwd = "name=>password"
    nu_passwdd = "name=>repassword"
    nu_close = "xpath=>//*[@id='cancle']"
    nu_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    nu_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# delete user
    du_close = "xpath=>//*[@id='cancle']"
    du_submit = "xpath=>//*[@id='deleteUser']"
    du_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# update user's password
    u_passwd = "name=>password"
    u_passwdd = "name=>repassword"
    u_close = "xpath=>//*[@id='cancle']"
    u_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    u_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# add new group
    ng_name = "name=>grpname"
    ng_description = "name=>grpdesc"
    ng_close = "xpath=>//*[@id='cancle']"
    ng_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    ng_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# delete group
    dg_close = "xpath=>//*[@id='cancle']"
    dg_submit = "xpath=>//*[@id='deleteGroup']"
    dg_result = "xpath=>//*[@id='layui-layer1']/div[2]"

# allocate vm to user
    avu_table = "xpath=>//*[@id='ivmTable']/tbody"
    avu_close = "id=>cancle"
    avu_submit = "id=>addVmtoUser"
    avu_result = "xpath=>/html/body/div[5]/div[2]"

# remove vm from user
    rvu_table = "xpath=>//*[@id='userVmTable']/tbody"
    rvu_close = "class_name=>layui-layer-btn0"
    rvu_submit = "class_name=>layui-layer-btn1"
    rvu_result = "xpath=>/html/body/div[5]/div[2]"

# allocate vm to group
    avg_table = "xpath=>//*[@id='ivmTable']/tbody"
    avg_close = "id=>cancle"
    avg_submit = "id=>addVmtoGroup"
    avg_result = "xpath=>/html/body/div[5]/div[2]"

# remove vm from group
    rvg_table = "xpath=>//*[@id='groupVmTable']/tbody"
    rvg_close = "class_name=>layui-layer-btn0"
    rvg_submit = "class_name=>layui-layer-btn1"
    rvg_result = "xpath=>/html/body/div[5]/div[2]"

# allocate user to group
    aug_table = "xpath=>//*[@id='iUserTable']/tbody"
    aug_close = "id=>cancle"
    aug_submit = "id=>addUsertoGroup"
    aug_result = "xpath=>/html/body/div[5]/div[2]"

# remove user from group
    rug_table = "xpath=>//*[@id='userTable']/tbody"
    rug_close = "class_name=>layui-layer-btn0"
    rug_submit = "class_name=>layui-layer-btn1"
    rug_result = "xpath=>/html/body/div[5]/div[2]"

# execute result submit
    r_close = "xpath=>/html/body/div[2]/span[1]/a[3]"  #//*[@id="layui-layer2"]/span[1]/a[3] /html/body/div[3]/span[1]/a[3]
    r_submit = "class_name=>layui-layer-btn0"

# to execute menu
    def send_click_user(self):
        self.click(self.user)

    def send_click_group(self):
        self.click(self.group)

    def send_click_newuser(self):
        self.click(self.newUser)

    def send_click_newgroup(self):
        self.click(self.newGroup)

    def send_click_deleteuser(self):
        self.click(self.deleteUser)

    def send_click_deletegroup(self):
        self.click(self.deleteGroup)

    def send_click_updatepasswd(self):
        self.click(self.updatePasswd)

    def send_click_addvm(self):
        self.click(self.addvm)

    def send_click_removevm(self):
        self.click(self.removevm)

    def send_click_adduser(self):
        self.click(self.adduser)

    def send_click_removeuser(self):
        self.click(self.removeuser)

# to add new user
    def send_input_nameNU(self, text):
        self.input(self.nu_name, text)

    def send_input_passwdNU(self, text):
        self.input(self.nu_passwd, text)

    def send_input_passwddNU(self, text):
        self.input(self.nu_passwdd, text)

    def send_click_closeNU(self):
        self.click(self.nu_close)

    def send_click_submitNU(self):
        self.click(self.nu_submit)

# to delete user
    def send_click_closeDU(self):
        self.click(self.du_close)

    def send_click_submitDU(self):
        self.click(self.du_submit)

# to update user's password
    def send_input_passwdU(self, text):
        self.input(self.u_passwd, text)

    def send_input_passwddU(self, text):
        self.input(self.u_passwdd, text)

    def send_click_closeU(self):
        self.click(self.u_close)

    def send_click_submitU(self):
        self.click(self.u_submit)

# to add new group
    def send_input_nameNG(self, text):
        self.input(self.ng_name, text)

    def send_input_descriptionNG(self, text):
        self.input(self.ng_description, text)

    def send_click_closeNG(self):
        self.click(self.ng_close)

    def send_click_submitNG(self):
        self.click(self.ng_submit)

# to delete group
    def send_click_closeDG(self):
        self.click(self.dg_close)

    def send_click_submitDG(self):
        self.click(self.dg_submit)

# to allocate vm to user
    def send_click_closeAVU(self):
        self.click(self.avu_close)

    def send_click_submitAVU(self):
        self.click(self.avu_submit)

# to remove vm from user
    def send_click_closeRVU(self):
        self.click(self.rvu_close)

    def send_click_submitRVU(self):
        self.click(self.rvu_submit)

# to allocate vm to group
    def send_click_closeAVG(self):
        self.click(self.avg_close)

    def send_click_submitAVG(self):
        self.click(self.avg_submit)

# to remove vm from group
    def send_click_closeRVG(self):
        self.click(self.rvg_close)

    def send_click_submitRVG(self):
        self.click(self.rvg_submit)

# to allocate user to group
    def send_click_closeAUG(self):
        self.click(self.aug_close)

    def send_click_submitAUG(self):
        self.click(self.aug_submit)

# to remove vm user group
    def send_click_closeRUG(self):
        self.click(self.rug_close)

    def send_click_submitRUG(self):
        self.click(self.rug_submit)

# to select user from table
    def select_user(self, user):
        return self.select_table_row(self.table, user)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)

    def send_click_closeR(self):
        self.click(self.r_close)

# allocate vm to user or group
    def send_click_allocatevm(self, selector, option):
        table = self.find_element(selector)
        rows = table.find_elements_by_tag_name("tr")
        for r in rows:
            els = r.find_elements_by_tag_name("td")
            if option == els[1].text:
                self.sleep(1)
                els[5].find_element_by_tag_name("div").find_element_by_tag_name("button").click()
                return 1
        return 0

# allocate user to group
    def send_click_allocateuser(self, selector, option):
        table = self.find_element(selector)
        rows = table.find_elements_by_tag_name("tr")
        for r in rows:
            els = r.find_elements_by_tag_name("td")
            if option == els[1].text:
                self.sleep(1)
                els[4].find_element_by_tag_name("div").find_element_by_tag_name("button").click()
                return 1
        return 0