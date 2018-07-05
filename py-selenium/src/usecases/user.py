# coding=utf-8

from src.pages.user import UserPage


class UserCase(UserPage):

    # 新建用户
    def new_user(self, name, passwd):
        self.send_input_nameNU(name)
        self.sleep(1)
        self.send_input_passwdNU(passwd)
        self.sleep(1)
        self.send_input_passwddNU(passwd)
        self.sleep(1)
        self.send_click_submitNU()

    # 删除用户
    def delete_user(self):
        self.send_click_submitDU()

    # 修改用户密码
    def update_user_passwd(self, passwd):
        self.send_input_passwdU(passwd)
        self.sleep(1)
        self.send_input_passwddU(passwd)
        self.sleep(1)
        self.send_click_submitU()

    # 新建用户组
    def new_group(self, name, description):
        self.send_input_nameNG(name)
        self.sleep(1)
        self.send_input_descriptionNG(description)
        self.sleep(1)
        self.send_click_submitNG()

    # 删除用户组
    def delete_group(self):
        self.send_click_submitDG()

    # 为用户分配虚拟机
    def allocate_vm_user(self):
        self.send_click_submitAVU()

    # 移除分配给用户的配虚拟机
    def remove_vm_user(self):
        self.send_click_submitRVU()

    # 为用户组分配虚拟机
    def allocate_vm_group(self):
        self.send_click_submitAVG()

    # 移除分配给用户组的配虚拟机
    def remove_vm_group(self):
        self.send_click_submitRVG()

    # 为用户组分配用户
    def allocate_user_group(self):
        self.send_click_submitAUG()

    # 移除分配给用户组的用户
    def remove_user_group(self):
        self.send_click_submitRUG()