# coding=utf-8

from src.usecases.user import UserCase


class TestUser(UserCase):

# 测试新建用户功能
    def test_new_user(self, user, passwd):
        self.send_click_user()
        self.sleep(1)
        self.send_click_newuser()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_user(user, passwd)
        self.sleep(1)
        result = self.get_element_text(self.nu_result)
        if "成功" in result:
            self.logger.info("********** 新建用户功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("user")
        else:
            self.logger.info("新建用户功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeNU()
            self.sleep(1)
            self.driver.switch_to.frame("user")

# 测试删除用户功能
    def test_delete_user(self, user):
        self.send_click_user()
        self.sleep(1)
        if 0 == self.select_user(user):
            self.logger.info("********** 要删除的用户不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_deleteuser()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_user()
        self.sleep(1)
        result = self.get_element_text(self.du_result)
        if "成功" in result:
            self.logger.info("********** 删除用户功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("user")
        else:
            self.logger.info("删除用户功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeDU()
            self.sleep(1)
            self.driver.switch_to.frame("user")

# 测试修改用户密码功能
    def test_update_user_passwd(self, user, passwd):
        self.send_click_user()
        self.sleep(1)
        if 0 == self.select_user(user):
            self.logger.info("********** 要更新密码的用户不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_updatepasswd()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.update_user_passwd(passwd)
        self.sleep(1)
        result = self.get_element_text(self.u_result)
        if "成功" in result:
            self.logger.info("********** 修改用户密码功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("user")
        else:
            self.logger.info("修改用户密码功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeU()
            self.sleep(1)
            self.driver.switch_to.frame("user")

# 测试新建用户组功能
    def test_new_group(self, group, description):
        self.send_click_group()
        self.sleep(1)
        self.send_click_newgroup()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_group(group, description)
        self.sleep(1)
        result = self.get_element_text(self.ng_result)
        if "成功" in result:
            self.logger.info("********** 新建用户组功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("user")
        else:
            self.logger.info("新建用户组功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeNG()
            self.sleep(1)
            self.driver.switch_to.frame("user")

# 测试删除用户组功能
    def test_delete_group(self, group):
        self.send_click_group()
        self.sleep(1)
        if 0 == self.select_user(group):
            self.logger.info("********** 要删除的用户组不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_deletegroup()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_group()
        self.sleep(1)
        result = self.get_element_text(self.dg_result)
        if "成功" in result:
            self.logger.info("********** 删除用户组功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("user")
        else:
            self.logger.info("删除用户组功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeDG()
            self.sleep(1)
            self.driver.switch_to.frame("user")

# 测试为用户分配虚拟机功能
    def test_allocate_vm_user(self, user, vm):
        self.send_click_user()
        self.sleep(1)
        if 0 == self.send_click_allocatevm(self.table, user):
            self.logger.info("********** 要分配虚拟机权限的用户不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.send_click_addvm()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.avu_table, vm):
            self.logger.info("********** 要分配的虚拟机不存在，测试结束! *********")
            self.send_click_closeAVU()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.allocate_vm_user()
        self.sleep(2)
        result = self.get_element_text(self.avu_result)
        if "成功" in result:
            self.logger.info("********** 为用户分配虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("为用户分配虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeAVU()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()

# 测试移除分配给用户的虚拟机功能
    def test_remove_vm_user(self, user, vm):
        self.send_click_user()
        self.sleep(1)
        if 0 == self.send_click_allocatevm(self.table, user):
            self.logger.info("********** 要移除虚拟机权限的用户不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.rvu_table, vm):
            self.logger.info("********** 要移除的虚拟机不存在，测试结束! *********")
            self.send_click_closeRVU()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.send_click_removevm()
        self.sleep(1)
        self.remove_vm_user()
        self.sleep(1)
        result = self.get_element_text(self.rvu_result)
        if "成功" in result:
            self.logger.info("********** 移除分配给用户的虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("移除分配给用户的虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()

# 测试为用户组分配虚拟机功能
    def test_allocate_vm_group(self, group, vm):
        self.send_click_group()
        self.sleep(1)
        if 0 == self.send_click_allocatevm(self.table, group):
            self.logger.info("********** 要分配虚拟机权限的用户组不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.send_click_addvm()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.avg_table, vm):
            self.logger.info("********** 要分配的虚拟机不存在，测试结束! *********")
            self.send_click_closeAVG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.allocate_vm_group()
        self.sleep(2)
        result = self.get_element_text(self.avg_result)
        if "成功" in result:
            self.logger.info("********** 为用户组分配虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("为用户组分配虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeAVG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()

# 测试移除分配给用户组的虚拟机功能
    def test_remove_vm_group(self, group, vm):
        self.send_click_group()
        self.sleep(1)
        if 0 == self.send_click_allocatevm(self.table, group):
            self.logger.info("********** 要移除虚拟机权限的用户组不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.rvg_table, vm):
            self.logger.info("********** 要移除的虚拟机不存在，测试结束! *********")
            self.send_click_closeRVG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.send_click_removevm()
        self.sleep(1)
        self.remove_vm_group()
        self.sleep(1)
        result = self.get_element_text(self.rvg_result)
        if "成功" in result:
            self.logger.info("********** 移除分配给用户组的虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("移除分配给用户组的虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()

# 测试为用户组分配用户功能
    def test_allocate_user_group(self, group, user):
        self.send_click_group()
        self.sleep(1)
        if 0 == self.send_click_allocateuser(self.table, group):
            self.logger.info("********** 要分配用户的用户组不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.send_click_adduser()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.aug_table, user):
            self.logger.info("********** 要分配的用户不存在，测试结束! *********")
            self.send_click_closeAUG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.allocate_user_group()
        self.sleep(2)
        result = self.get_element_text(self.aug_result)
        if "成功" in result:
            self.logger.info("********** 为用户组分配虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("为用户组分配虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeAUG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()

# 测试移除分配给用户组的用户
    def test_remove_user_group(self, group, user):
        self.send_click_group()
        self.sleep(1)
        if 0 == self.send_click_allocateuser(self.table, group):
            self.logger.info("********** 要移除用户的用户组不存在，测试结束! *********")
            return
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.rug_table, user):
            self.logger.info("********** 要移除的用户不存在，测试结束! *********")
            self.send_click_closeRUG()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
            return
        self.sleep(1)
        self.send_click_removeuser()
        self.sleep(1)
        self.remove_user_group()
        self.sleep(1)
        result = self.get_element_text(self.rug_result)
        if "成功" in result:
            self.logger.info("********** 移除分配给用户组的用户功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()
        else:
            self.logger.info("移除分配给用户组的用户功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeR()